// NetStreamFfmpeg.cpp:  Network streaming for FFMPEG video library, for Gnash.
// 
//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//


#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#ifdef USE_FFMPEG

#include "NetStreamFfmpeg.h"
#include "log.h"
#include "fn_call.h"
#include "NetStream.h"
#include "render.h"	
#include "movie_root.h"
#include "sound_handler.h"
#include "VideoDecoderFfmpeg.h"
#include "ClockTime.h" // TODO: use the VirtualClock instead ?

#include "FLVParser.h" 

#include <boost/scoped_array.hpp>
#include <algorithm> // std::min


#if defined(_WIN32) || defined(WIN32)
# include <windows.h>	// for sleep()
# define usleep(x) Sleep(x/1000)
#else
# include "unistd.h" // for usleep()
#endif

/// Define this to add debugging prints for locking
//#define GNASH_DEBUG_THREADS

// Define the following macro to have status notification handling debugged
//#define GNASH_DEBUG_STATUS

// Used to free data in the AVPackets we create our self
static void avpacket_destruct(AVPacket* av)
{
	delete [] av->data;
}


namespace gnash {


NetStreamFfmpeg::NetStreamFfmpeg():

	_playback_state(PLAY_NONE),
	_decoding_state(DEC_NONE),

	m_video_index(-1),
	m_audio_index(-1),

	m_VCodecCtx(NULL),
	m_ACodecCtx(NULL),
	m_FormatCtx(NULL),
	m_Frame(NULL),

	_decodeThread(NULL),
	_decodeThreadBarrier(2), // main and decoder threads

	m_last_video_timestamp(0),
	m_last_audio_timestamp(0),
	m_current_timestamp(0),
	m_unqueued_data(NULL),
	m_time_of_pause(0),

	_decoderBuffer(0),
	_soundHandler(get_sound_handler())
{

	ByteIOCxt.buffer = NULL;
}

NetStreamFfmpeg::~NetStreamFfmpeg()
{
	if ( _decoderBuffer ) delete [] _decoderBuffer;

	close(); // close will also detach from sound handler

	delete m_imageframe;
}


void NetStreamFfmpeg::pause( PauseMode mode )
{
	log_debug("::pause(%d) called ", mode);
  switch ( mode ) {
    case pauseModeToggle:
			if ( playbackStatus() == PLAY_PAUSED ) {
			  unpausePlayback();
			} else {
			  pausePlayback();
			}
			break;
    case pauseModePause:
			pausePlayback();
			break;
    case pauseModeUnPause:
			unpausePlayback();
			break;
    default:
			break;
  }

}

void NetStreamFfmpeg::close()
{

	if (decodingStatus() != DEC_STOPPED && decodingStatus() != DEC_NONE)
	{
		// request decoder thread termination
		decodingStatus(DEC_STOPPED);

		// resume the decoder, if waiting
		_qFillerResume.notify_all();

		// wait till thread is complete before main continues
		_decodeThread->join();

		delete _decodeThread;

	}

	// When closing gnash before playback is finished, the soundhandler 
	// seems to be removed before netstream is destroyed.
	if (_soundHandler)
	{
		_soundHandler->detach_aux_streamer(this);
	}

	if (m_Frame) av_free(m_Frame);
	m_Frame = NULL;

  if ( m_VCodecCtx ) {
    avcodec_close( m_VCodecCtx );
  }
  m_VCodecCtx = NULL;

  if ( m_ACodecCtx ) {
    avcodec_close( m_ACodecCtx );
  }
  m_ACodecCtx = NULL;

	if (m_FormatCtx)
	{
		m_FormatCtx->iformat->flags = AVFMT_NOFILE;
		av_close_input_file(m_FormatCtx);
		m_FormatCtx = NULL;
	}

	delete m_imageframe;
	m_imageframe = NULL;
	delete m_unqueued_data;
	m_unqueued_data = NULL;

	boost::mutex::scoped_lock lock(_qMutex);

	m_qvideo.clear();
	m_qaudio.clear();

	delete [] ByteIOCxt.buffer;

}

// ffmpeg callback function
int 
NetStreamFfmpeg::readPacket(void* opaque, boost::uint8_t* buf, int buf_size)
{

	NetStreamFfmpeg* ns = static_cast<NetStreamFfmpeg*>(opaque);
	boost::intrusive_ptr<NetConnection> nc = ns->_netCon;

	size_t ret = nc->read(static_cast<void*>(buf), buf_size);
	ns->inputPos += ret;
	return ret;

}

// ffmpeg callback function
offset_t 
NetStreamFfmpeg::seekMedia(void *opaque, offset_t offset, int whence)
{

	NetStreamFfmpeg* ns = static_cast<NetStreamFfmpeg*>(opaque);
	boost::intrusive_ptr<NetConnection> nc = ns->_netCon;


	// Offset is absolute new position in the file
	if (whence == SEEK_SET)
	{	
		nc->seek(offset);
		ns->inputPos = offset;

	// New position is offset + old position
	}
	else if (whence == SEEK_CUR)
	{
		nc->seek(ns->inputPos + offset);
		ns->inputPos = ns->inputPos + offset;

	// New position is offset + end of file
	}
	else if (whence == SEEK_END)
	{
		// This is (most likely) a streamed file, so we can't seek to the end!
		// Instead we seek to 50.000 bytes... seems to work fine...
		nc->seek(50000);
		ns->inputPos = 50000;

	}

	return ns->inputPos;
}

void
NetStreamFfmpeg::play(const std::string& c_url)
{

	// Is it already playing ?
	if (playbackStatus() != PLAY_NONE && playbackStatus() != PLAY_STOPPED)
	{
		log_error("NetStream.play() called already playing ?"); // TODO: fix this case
		unpausePlayback(); // will check for playbackStatus itself..
		return;
	}

	// Does it have an associated NetConnection ?
	if ( ! _netCon )
	{
		IF_VERBOSE_ASCODING_ERRORS(
		log_aserror(_("No NetConnection associated with this NetStream, won't play"));
		);
		return;
	}

	if (url.size() == 0) url += c_url;
	// Remove any "mp3:" prefix. Maybe should use this to mark as audio-only
	if (url.compare(0, 4, std::string("mp3:")) == 0)
	{
		url = url.substr(4);
	}

	decodingStatus(DEC_BUFFERING);
	unpausePlayback();

	// This starts the decoding thread
	_decodeThread = new boost::thread(boost::bind(NetStreamFfmpeg::av_streamer, this)); 
	_decodeThreadBarrier.wait();

	return;
}

/// Finds a decoder, allocates a context and initializes it.
//
/// @param codec_id the codec ID to find
/// @return the initialized context, or NULL on failure. The caller is 
///         responsible for deallocating!
static AVCodecContext*
initContext(enum CodecID codec_id)
{

	AVCodec* codec = avcodec_find_decoder(codec_id);
	if (!codec)
	{
		log_error(_("libavcodec couldn't find decoder"));
		return NULL;
	}

	AVCodecContext * context = avcodec_alloc_context();
	if (!context)
	{
		log_error(_("libavcodec couldn't allocate context"));
		return NULL;
	}

	int rv = avcodec_open(context, codec);
	if (rv < 0) 
	{
		avcodec_close(context);
		log_error(_("libavcodec failed to initialize codec"));
		return NULL;
	}

	return context;
}

/// Gets video info from the parser and initializes the codec.
//
/// @param parser the parser to use to get video information.
/// @return the initialized context, or NULL on failure. The caller
///         is responsible for deallocating this pointer.
static AVCodecContext* 
initFlvVideo(FLVParser& parser)
{
	// Get video info from the parser
	FLVVideoInfo* videoInfo = parser.getVideoInfo();
	if (!videoInfo)
	{
		return NULL;
	}

	enum CodecID codec_id;

	// Find the decoder and init the parser
	switch(videoInfo->codec)
	{
		case media::VIDEO_CODEC_H263:
			codec_id = CODEC_ID_FLV1;
			break;
#ifdef FFMPEG_VP6
		case media::VIDEO_CODEC_VP6:
			codec_id = CODEC_ID_VP6F;
			break;
#endif
		case media::VIDEO_CODEC_SCREENVIDEO:
			codec_id = CODEC_ID_FLASHSV;
			break;
		default:
			log_error(_("Unsupported video codec %d"), (int) videoInfo->codec);
			return NULL;
	}

	return initContext(codec_id);
}


/// Like initFlvVideo, but for audio.
static AVCodecContext*
initFlvAudio(FLVParser& parser)
{
	// Get audio info from the parser
	FLVAudioInfo* audioInfo =  parser.getAudioInfo();
	if (!audioInfo)
	{
		return NULL;
	}

	enum CodecID codec_id;

	switch(audioInfo->codec)
	{
		case media::AUDIO_CODEC_RAW:
			codec_id = CODEC_ID_PCM_U16LE;
			break;
		case media::AUDIO_CODEC_ADPCM:
			codec_id = CODEC_ID_ADPCM_SWF;
			break;
		case media::AUDIO_CODEC_MP3:
			codec_id = CODEC_ID_MP3;
			break;
		default:
			log_error(_("Unsupported audio codec %d"), (int)audioInfo->codec);
			return NULL;
	}

	return initContext(codec_id);
}


/// Probe the stream and try to figure out what the format is.
//
/// @param ns the netstream to use for reading
/// @return a pointer to the AVInputFormat structure containing
///         information about the input format, or NULL.
static AVInputFormat*
probeStream(NetStreamFfmpeg* ns)
{
	boost::scoped_array<boost::uint8_t> buffer(new boost::uint8_t[2048]);

	// Probe the file to detect the format
	AVProbeData probe_data;
	probe_data.filename = "";
	probe_data.buf = buffer.get();
	probe_data.buf_size = 2048;

	if (ns->readPacket(ns, probe_data.buf, probe_data.buf_size) < 1)
	{
 		log_error(_("Gnash could not read from movie url"));
 		return NULL;
	}

	return av_probe_input_format(&probe_data, 1);
}

bool
NetStreamFfmpeg::startPlayback()
{

	boost::intrusive_ptr<NetConnection> nc = _netCon;
	assert(nc);

	// Pass stuff from/to the NetConnection object.
	if ( !nc->openConnection(url) )
	{
		log_error(_("Gnash could not open movie: %s"), url.c_str());
		setStatus(streamNotFound);
		return false;
	}

	nc->seek(0);
	inputPos = 0;

	// Check if the file is a FLV, in which case we use our own parser
	char head[4] = {0, 0, 0, 0};
	if (nc->read(head, 3) < 3)
	{
		setStatus(streamNotFound);
		return false;
	}

	nc->seek(0);
	if (std::string(head) == "FLV")
	{
		m_isFLV = true;
		if (!m_parser.get())
		{
			m_parser = nc->getConnectedParser();
			if (! m_parser.get() )
			{
				setStatus(streamNotFound);
				log_error(_("Gnash could not open FLV movie: %s"), url.c_str());
				return false;
			}
		}

		// Init the avdecoder-decoder
		avcodec_init();
		avcodec_register_all();

		m_VCodecCtx = initFlvVideo(*m_parser);
		if (!m_VCodecCtx)
		{
			log_error(_("Failed to initialize FLV video codec"));
			return false;
		}

		m_ACodecCtx = initFlvAudio(*m_parser);
		if (!m_ACodecCtx)
		{
			log_error(_("Failed to initialize FLV audio codec"));
			return false;
		}

		// We just define the indexes here, they're not really used when
		// the file format is FLV
		m_video_index = 0;
		m_audio_index = 1;

		m_start_onbuffer = true;

		// Allocate a frame to store the decoded frame in
		m_Frame = avcodec_alloc_frame();
		return true;
	}

	// This registers all available file formats and codecs 
	// with the library so they will be used automatically when
	// a file with the corresponding format/codec is opened
	// XXX should we call avcodec_init() first?
	av_register_all();

	AVInputFormat* inputFmt = probeStream(this);
	if (!inputFmt)
	{
		log_error(_("Couldn't determine stream input format from URL %s"), url.c_str());
		return false;
	}

	// After the format probe, reset to the beginning of the file.
	nc->seek(0);

	// Setup the filereader/seeker mechanism. 7th argument (NULL) is the writer function,
	// which isn't needed.
	init_put_byte(&ByteIOCxt, new boost::uint8_t[500000], 500000, 0, this, NetStreamFfmpeg::readPacket, NULL, NetStreamFfmpeg::seekMedia);
	ByteIOCxt.is_streamed = 1;

	m_FormatCtx = av_alloc_format_context();

	// Open the stream. the 4th argument is the filename, which we ignore.
	if(av_open_input_stream(&m_FormatCtx, &ByteIOCxt, "", inputFmt, NULL) < 0)
	{
		log_error(_("Couldn't open file '%s' for decoding"), url.c_str());
		setStatus(streamNotFound);
		return false;
	}

	// Next, we need to retrieve information about the streams contained in the file
	// This fills the streams field of the AVFormatContext with valid information
	int ret = av_find_stream_info(m_FormatCtx);
	if (ret < 0)
	{
		log_error(_("Couldn't find stream information from '%s', error code: %d"), url.c_str(), ret);
		return false;
	}

//	m_FormatCtx->pb.eof_reached = 0;
//	av_read_play(m_FormatCtx);

	// Find the first video & audio stream
	m_video_index = -1;
	m_audio_index = -1;
	//assert(m_FormatCtx->nb_streams >= 0); useless assert. 
	for (unsigned int i = 0; i < (unsigned)m_FormatCtx->nb_streams; i++)
	{
		AVCodecContext* enc = m_FormatCtx->streams[i]->codec; 

		switch (enc->codec_type)
		{
			case CODEC_TYPE_AUDIO:
				if (m_audio_index < 0)
				{
					m_audio_index = i;
					m_audio_stream = m_FormatCtx->streams[i];
				}
				break;

			case CODEC_TYPE_VIDEO:
				if (m_video_index < 0)
				{
					m_video_index = i;
					m_video_stream = m_FormatCtx->streams[i];
				}
				break;
			default:
				break;
		}
	}

	if (m_video_index < 0)
	{
		log_error(_("Didn't find a video stream from '%s'"), url.c_str());
		return false;
	}

	// Get a pointer to the codec context for the video stream
	m_VCodecCtx = m_FormatCtx->streams[m_video_index]->codec;

	// Find the decoder for the video stream
	AVCodec* pCodec = avcodec_find_decoder(m_VCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		m_VCodecCtx = NULL;
		log_error(_("Video decoder %d not found"), 
			m_VCodecCtx->codec_id);
		return false;
	}

	// Open codec
	if (avcodec_open(m_VCodecCtx, pCodec) < 0)
	{
		log_error(_("Could not open codec %d"),
			m_VCodecCtx->codec_id);
	}

	// Allocate a frame to store the decoded frame in
	m_Frame = avcodec_alloc_frame();

	if ( m_audio_index >= 0 && _soundHandler )
	{
		// Get a pointer to the audio codec context for the video stream
		m_ACodecCtx = m_FormatCtx->streams[m_audio_index]->codec;

		// Find the decoder for the audio stream
		AVCodec* pACodec = avcodec_find_decoder(m_ACodecCtx->codec_id);
		if (pACodec == NULL)
		{
			log_error(_("No available audio decoder %d to process MPEG file: '%s'"), 
				m_ACodecCtx->codec_id, url.c_str());
			return false;
		}
        
		// Open codec
		if (avcodec_open(m_ACodecCtx, pACodec) < 0)
		{
			log_error(_("Could not open audio codec %d for %s"),
				m_ACodecCtx->codec_id, url.c_str());
			return false;
		}

	}

	unpausePlayback();
	return true;
}


/// Copy RGB data from a source raw_mediadata_t to a destination image::rgb.
/// @param dst the destination image::rgb, which must already be initialized
///            with a buffer of size of at least src.m_size.
/// @param src the source raw_mediadata_t to copy data from. The m_size member
///            of this structure must be initialized.
/// @param width the width, in bytes, of a row of video data.
static void
rgbcopy(image::rgb* dst, media::raw_mediadata_t* src, int width)
{
  assert( src->m_size <= static_cast<boost::uint32_t>(dst->width() * dst->height() * 3) ); 

  boost::uint8_t* dstptr = dst->data();

  boost::uint8_t* srcptr = src->m_data;
  boost::uint8_t* srcend = src->m_data + src->m_size;

  while (srcptr < srcend) {
    memcpy(dstptr, srcptr, width);
    dstptr += dst->pitch();
    srcptr += width;
  }
}

// decoder thread
void NetStreamFfmpeg::av_streamer(NetStreamFfmpeg* ns)
{
	//GNASH_REPORT_FUNCTION;

	ns->_decodeThreadBarrier.wait();

	if (!ns->m_ACodecCtx && !ns->m_VCodecCtx && !ns->m_FormatCtx)
	{
		if (!ns->startPlayback())
		{
			log_debug("av_streamer: !ns->startPlayback, returning");
			return;
		}
	}
	else
	{
		// We need to restart the audio
		if (ns->_soundHandler)
		{
			ns->_soundHandler->attach_aux_streamer(audio_streamer, ns);
		}
	}

	ns->setStatus(playStart);

	ns->m_last_video_timestamp = 0;
	ns->m_last_audio_timestamp = 0;
	ns->m_current_timestamp = 0;

	ns->m_start_clock = clocktime::getTicks();

	ns->m_unqueued_data = NULL;

	// Loop while we're playing or buffering
	while (ns->decodingStatus() != DEC_STOPPED)
	{
		unsigned long int sleepTime = 1000;

		{
#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: waiting for lock in av_streamer");
#endif
		boost::mutex::scoped_lock lock(ns->_qMutex);
#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: lock obtained in av_streamer");
#endif

#ifdef GNASH_DEBUG_THREADS
		log_debug("Decoding iteration. bufferTime=%lu, bufferLen=%lu, videoFrames=%lu, audioFrames=%lu",
			ns->bufferTime(), ns->bufferLength(), ns->m_qvideo.size(), ns->m_qaudio.size());
#endif

		if (ns->m_isFLV)
		{
			// If any of the two queues are full don't bother fetching more
			// (next consumer will wake us up)
			//
      			if ( ns->m_qvideo.full() || ns->m_qaudio.full() )
			{
				ns->decodingStatus(DEC_DECODING); // that's to say: not buffering anymore

				// Instead wait till waked up by short-queues event
				log_debug("Queues full, waiting on qNeedRefill condition");
				ns->_qFillerResume.wait(lock);
			}
			else
			{
				log_debug("Calling decodeFLVFrame");
				bool successDecoding = ns->decodeFLVFrame();
				//log_debug("decodeFLVFrame returned %d", successDecoding);
				if ( ! successDecoding )
				{
					// Possible failures:
					// 1. could not decode frame... lot's of possible
					//    reasons...
					// 2. EOF reached
					if ( ns->m_videoFrameFormat != render::NONE )
					{
						log_error("Could not decode FLV frame");
					}
					// else it's expected, we'll keep going anyway
				}

			}

		}
		else
		{

			// If we have problems with decoding - break
			if (ns->decodeMediaFrame() == false && ns->m_start_onbuffer == false && ns->m_qvideo.size() == 0 && ns->m_qaudio.size() == 0)
			{
				break;
			}

		}

#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: releasing lock in av_streamer");
#endif
		}

		//log_debug("Sleeping %d microseconds", sleepTime);
		usleep(sleepTime); // Sleep 1ms to avoid busying the processor.

	}

//#ifdef GNASH_DEBUG_THREADS
	log_debug("Out of decoding loop. playbackStatus:%d, decodingStatus:%d", ns->playbackStatus(), ns->decodingStatus());
//#endif
	ns->decodingStatus(DEC_STOPPED);

}

// audio callback is running in sound handler thread
bool NetStreamFfmpeg::audio_streamer(void *owner, boost::uint8_t *stream, int len)
{
	//GNASH_REPORT_FUNCTION;

	NetStreamFfmpeg* ns = static_cast<NetStreamFfmpeg*>(owner);

	if (ns->playbackStatus() == PLAY_PAUSED)
	{
		log_debug("playback status is paused, won't consume audio frames");
		return false;
	}

	while (len > 0 && ! ns->m_qaudio.empty())
	{
#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: waiting for lock in audio_streamer");
#endif
		boost::mutex::scoped_lock lock(ns->_qMutex);
#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: lock obtained in audio_streamer");
#endif

    		media::raw_mediadata_t* samples = ns->m_qaudio.front();

		int n = std::min<int>(samples->m_size, len);
		memcpy(stream, samples->m_ptr, n);
		stream += n;
		samples->m_ptr += n;
		samples->m_size -= n;
		len -= n;

		ns->m_current_timestamp = samples->m_pts;

		if (samples->m_size == 0)
		{
			ns->m_qaudio.pop();
			delete samples;

			// wake up filler (TODO: do only if decoder is running)
			ns->_qFillerResume.notify_all();
		}

#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: releasing lock in audio_streamer");
#endif
	}
	return true;
}

bool NetStreamFfmpeg::decodeFLVFrame()
{
	FLVFrame* frame = m_parser->nextMediaFrame(); // we don't care which one, do we ?

	if (frame == NULL)
	{
		assert ( _netCon->loadCompleted() );
		decodingStatus(DEC_STOPPED);
		return true;
	}

  	AVPacket packet;

  	packet.destruct = avpacket_destruct;
  	packet.size = frame->dataSize;
  	packet.data = frame->data;
  	// FIXME: is this the right value for packet.dts?
  	packet.pts = packet.dts = static_cast<boost::int64_t>(frame->timestamp);

	if (frame->type == videoFrame)
	{
    		packet.stream_index = 0;
    		return decodeVideo(&packet);
	}
	else
	{
		assert(frame->type == audioFrame);
    		packet.stream_index = 1;
    		return decodeAudio(&packet);
	}

}


bool NetStreamFfmpeg::decodeAudio( AVPacket* packet )
{
	if (!m_ACodecCtx) return false;

	int frame_size;
	//static const unsigned int bufsize = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
	static const unsigned int bufsize = AVCODEC_MAX_AUDIO_FRAME_SIZE;

	if ( ! _decoderBuffer ) _decoderBuffer = new boost::uint8_t[bufsize];

	boost::uint8_t* ptr = _decoderBuffer;

#ifdef FFMPEG_AUDIO2
	frame_size = bufsize; // TODO: is it safe not initializing this ifndef FFMPEG_AUDIO2 ?
	if (avcodec_decode_audio2(m_ACodecCtx, (boost::int16_t*) ptr, &frame_size, packet->data, packet->size) >= 0)
#else
	if (avcodec_decode_audio(m_ACodecCtx, (boost::int16_t*) ptr, &frame_size, packet->data, packet->size) >= 0)
#endif
	{

		bool stereo = m_ACodecCtx->channels > 1 ? true : false;
		int samples = stereo ? frame_size >> 2 : frame_size >> 1;
		
		if (_resampler.init(m_ACodecCtx))
		{
			// Resampling is needed.
			
			// Compute new size based on frame_size and
			// resampling configuration
			double resampleFactor = (44100.0/m_ACodecCtx->sample_rate) * (2.0/m_ACodecCtx->channels);
			int resampledFrameSize = int(ceil(frame_size*resampleFactor));

			// Allocate just the required amount of bytes
			boost::uint8_t* output = new boost::uint8_t[resampledFrameSize];
			
			samples = _resampler.resample(reinterpret_cast<boost::int16_t*>(ptr), 
							 reinterpret_cast<boost::int16_t*>(output), 
							 samples);

			if (resampledFrameSize < samples*2*2)
			{
				log_error(" --- Computation of resampled frame size (%d) < then the one based on samples (%d)",
					resampledFrameSize, samples*2*2);

				log_debug(" input frame size: %d", frame_size);
				log_debug(" input sample rate: %d", m_ACodecCtx->sample_rate);
				log_debug(" input channels: %d", m_ACodecCtx->channels);
				log_debug(" input samples: %d", samples);

				log_debug(" output sample rate (assuming): %d", 44100);
				log_debug(" output channels (assuming): %d", 2);
				log_debug(" output samples: %d", samples);

				abort(); // the call to resample() likely corrupted memory...
			}

			frame_size = samples*2*2;

			// ownership of memory pointed-to by 'ptr' will be
			// transferred below
			ptr = reinterpret_cast<boost::uint8_t*>(output);

			// we'll reuse _decoderBuffer 
		}
		else
		{
			// ownership of memory pointed-to by 'ptr' will be
			// transferred below, so we reset _decoderBuffer here.
			// Doing so, next time we'll need to decode we'll create
			// a new buffer
			_decoderBuffer=0;
		}
		
    		media::raw_mediadata_t* raw = new media::raw_mediadata_t();
		
		raw->m_data = ptr; // ownership of memory pointed by 'ptr' transferred here
		raw->m_ptr = raw->m_data;
		raw->m_size = frame_size;
		raw->m_stream_index = m_audio_index;

		// set presentation timestamp
		if (packet->dts != static_cast<signed long>(AV_NOPTS_VALUE))
		{
			if (!m_isFLV) raw->m_pts = static_cast<boost::uint32_t>(as_double(m_audio_stream->time_base) * packet->dts * 1000.0);
			else raw->m_pts = static_cast<boost::uint32_t>((as_double(m_ACodecCtx->time_base) * packet->dts) * 1000.0);
		}

		if (raw->m_pts != 0)
		{	
			// update audio clock with pts, if present
			m_last_audio_timestamp = raw->m_pts;
		}
		else
		{
			raw->m_pts = m_last_audio_timestamp;
		}

		// update video clock for next frame
		boost::uint32_t frame_delay;
		if (!m_isFLV)
		{
			frame_delay = static_cast<boost::uint32_t>((as_double(m_audio_stream->time_base) * packet->dts) * 1000.0);
		}
		else
		{
			frame_delay = m_parser->audioFrameDelay();
		}

		m_last_audio_timestamp += frame_delay;

		if (m_isFLV)
		{
			if ( ! m_qaudio.push(raw) )
			{
				log_error("Audio queue full!");
			}
		}
		else m_unqueued_data = m_qaudio.push(raw) ? NULL : raw;
	}
	return true;
}


bool NetStreamFfmpeg::decodeVideo(AVPacket* packet)
{
	if (!m_VCodecCtx) return false;

	int got = 0;
	avcodec_decode_video(m_VCodecCtx, m_Frame, &got, packet->data, packet->size);
	if (!got) return false;

	// This tmpImage is really only used to compute proper size of the video data...
	// stupid isn't it ?
	std::auto_ptr<image::image_base> tmpImage;
	if (m_videoFrameFormat == render::YUV)
	{
		tmpImage.reset( new image::yuv(m_VCodecCtx->width, m_VCodecCtx->height) );
	}
	else if (m_videoFrameFormat == render::RGB)
	{
		tmpImage.reset( new image::rgb(m_VCodecCtx->width, m_VCodecCtx->height) );
	}

	AVPicture rgbpicture;

	if (m_videoFrameFormat == render::NONE)
	{
		// NullGui?
		return false;

	}
	else if (m_videoFrameFormat == render::YUV && m_VCodecCtx->pix_fmt != PIX_FMT_YUV420P)
	{
		assert( 0 );	// TODO
		//img_convert((AVPicture*) pFrameYUV, PIX_FMT_YUV420P, (AVPicture*) pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
		// Don't use depreceted img_convert, use sws_scale

	}
	else if (m_videoFrameFormat == render::RGB && m_VCodecCtx->pix_fmt != PIX_FMT_RGB24)
	{
		rgbpicture = media::VideoDecoderFfmpeg::convertRGB24(m_VCodecCtx, *m_Frame);
		if (!rgbpicture.data[0])
		{
			return false;
		}
	}

	media::raw_mediadata_t* video = new media::raw_mediadata_t();

	video->m_data = new boost::uint8_t[tmpImage->size()];
	video->m_ptr = video->m_data;
	video->m_stream_index = m_video_index;
	video->m_pts = 0;

	// set presentation timestamp
	if (packet->dts != static_cast<signed long>(AV_NOPTS_VALUE))
	{
		if (!m_isFLV)	video->m_pts = static_cast<boost::uint32_t>((as_double(m_video_stream->time_base) * packet->dts) * 1000.0);
		else video->m_pts = static_cast<boost::uint32_t>((as_double(m_VCodecCtx->time_base) * packet->dts) * 1000.0);
	}

	if (video->m_pts != 0)
	{	
		// update video clock with pts, if present
		m_last_video_timestamp = video->m_pts;
	}
	else
	{
		video->m_pts = m_last_video_timestamp;
	}

	// update video clock for next frame
	boost::uint32_t frame_delay;
	if (!m_isFLV) frame_delay = static_cast<boost::uint32_t>(as_double(m_video_stream->codec->time_base) * 1000.0);
	else frame_delay = m_parser->videoFrameDelay();

	// for MPEG2, the frame can be repeated, so we update the clock accordingly
	frame_delay += static_cast<boost::uint32_t>(m_Frame->repeat_pict * (frame_delay * 0.5) * 1000.0);

	m_last_video_timestamp += frame_delay;

	if (m_videoFrameFormat == render::YUV)
	{
		image::yuv* yuvframe = static_cast<image::yuv*>(tmpImage.get());
		unsigned int copied = 0;
		boost::uint8_t* ptr = video->m_data;
		for (int i = 0; i < 3 ; i++)
		{
			int shift = (i == 0 ? 0 : 1);
			boost::uint8_t* yuv_factor = m_Frame->data[i];
			int h = m_VCodecCtx->height >> shift;
			int w = m_VCodecCtx->width >> shift;
			for (int j = 0; j < h; j++)
			{
				copied += w;
				assert(copied <= yuvframe->size());
				memcpy(ptr, yuv_factor, w);
				yuv_factor += m_Frame->linesize[i];
				ptr += w;
			}
		}
		video->m_size = copied;
	}
	else if (m_videoFrameFormat == render::RGB)
	{
		AVPicture* src;
		if (m_VCodecCtx->pix_fmt != PIX_FMT_RGB24)
		{
			src = &rgbpicture;
		} else
		{
			src = (AVPicture*) m_Frame;
		}
	
		boost::uint8_t* srcptr = src->data[0];		  
		boost::uint8_t* srcend = srcptr + rgbpicture.linesize[0] * m_VCodecCtx->height;
		boost::uint8_t* dstptr = video->m_data;
		unsigned int srcwidth = m_VCodecCtx->width * 3;

		video->m_size = 0;

		while (srcptr < srcend) {
			memcpy(dstptr, srcptr, srcwidth);
			srcptr += src->linesize[0];
			dstptr += srcwidth;
			video->m_size += srcwidth;
		}
		
		if (m_VCodecCtx->pix_fmt != PIX_FMT_RGB24) {
			delete [] rgbpicture.data[0];
		}

	}

	// NOTE: Caller is assumed to have locked _qMutex already
	if (m_isFLV)
	{
		if ( ! m_qvideo.push(video) )
		{
			log_error("Video queue full !");
		}
	}
	else m_unqueued_data = m_qvideo.push(video) ? NULL : video;

	return true;
}

bool NetStreamFfmpeg::decodeMediaFrame()
{

	if (m_unqueued_data)
	{
		if (m_unqueued_data->m_stream_index == m_audio_index)
		{
			if (_soundHandler)
			{
				m_unqueued_data = m_qaudio.push(m_unqueued_data) ? NULL : m_unqueued_data;
			}
		}
		else if (m_unqueued_data->m_stream_index == m_video_index)
		{
			m_unqueued_data = m_qvideo.push(m_unqueued_data) ? NULL : m_unqueued_data;
		}
		else
		{
			log_error(_("read_frame: not audio & video stream"));
		}
		return true;
	}

  	AVPacket packet;
	
  	int rc = av_read_frame(m_FormatCtx, &packet);

	if (rc >= 0)
	{
		if (packet.stream_index == m_audio_index && _soundHandler)
		{
      			if (!decodeAudio(&packet)) 
			{
				log_error(_("Problems decoding audio frame"));
				return false;
			}
		}
		else
		if (packet.stream_index == m_video_index)
		{
      			if (!decodeVideo(&packet)) 
			{
				log_error(_("Problems decoding video frame"));
				return false;
			}
		}
		av_free_packet(&packet);
	}
	else
	{
		log_error(_("Problems decoding frame"));
		return false;
	}

	return true;
}

void
NetStreamFfmpeg::seek(boost::uint32_t pos)
{
	long newpos = 0;
	double timebase = 0;

	// Seek to new position
	if (m_isFLV)
	{
		if (m_parser.get())
		{
			newpos = m_parser->seek(pos);
		}
		else
		{
			newpos = 0;
		}
	}
	else if (m_FormatCtx)
	{

		AVStream* videostream = m_FormatCtx->streams[m_video_index];
    		timebase = static_cast<double>(videostream->time_base.num / videostream->time_base.den);
		newpos = static_cast<long>(pos / timebase);
		
		if (av_seek_frame(m_FormatCtx, m_video_index, newpos, 0) < 0)
		{
			log_error(_("%s: seeking failed"), __FUNCTION__);
			return;
		}
	}
	else
	{
		// TODO: should we log_debug ??
		return;
	}

	// This is kindof hackish and ugly :-(
	if (newpos == 0)
	{
		m_last_video_timestamp = 0;
		m_last_audio_timestamp = 0;
		m_current_timestamp = 0;

		m_start_clock = clocktime::getTicks();

	}
	else if (m_isFLV)
	{

		if (m_VCodecCtx) m_start_clock += m_last_video_timestamp - newpos;
		else m_start_clock += m_last_audio_timestamp - newpos;

		if (m_ACodecCtx) m_last_audio_timestamp = newpos;
		if (m_VCodecCtx) m_last_video_timestamp = newpos;
		m_current_timestamp = newpos;
	}
	else
	{
    		AVPacket Packet;
    		av_init_packet(&Packet);
		double newtime = 0;
		while (newtime == 0)
		{
      			if (av_read_frame(m_FormatCtx, &Packet) < 0) 
			{
				av_seek_frame(m_FormatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
				av_free_packet( &Packet );
				return;
			}

			newtime = timebase * (double)m_FormatCtx->streams[m_video_index]->cur_dts;
		}

    		av_free_packet( &Packet );

		av_seek_frame(m_FormatCtx, m_video_index, newpos, 0);
		boost::uint32_t newtime_ms = static_cast<boost::int32_t>(newtime / 1000.0);
		m_start_clock += m_last_audio_timestamp - newtime_ms;

		m_last_audio_timestamp = newtime_ms;
		m_last_video_timestamp = newtime_ms;
		m_current_timestamp = newtime_ms;
	}
	
	// Flush the queues
	m_qvideo.clear();
	m_qaudio.clear();

}

void
NetStreamFfmpeg::refreshVideoFrame()
{
#ifdef GNASH_DEBUG_THREADS
	log_debug("qMutex: waiting for lock in refreshVideoFrame");
#endif
	boost::mutex::scoped_lock lock(_qMutex);
#ifdef GNASH_DEBUG_THREADS
	log_debug("qMutex: lock obtained in refreshVideoFrame");
#endif

	// If we're paused (and we got the first imageframe), there is no need to do this
	if (playbackStatus() == PLAY_PAUSED && m_imageframe)
	{
		log_debug("refreshVideoFrame doing nothing as playback is paused and we have an image frame already");
#ifdef GNASH_DEBUG_THREADS
		log_debug("qMutex: releasing lock in refreshVideoFrame");
#endif
		return;
	}

	// Loop until a good frame is found
	do
	{
		// Get video frame from queue, will have the lowest timestamp
		// will return NULL if empty(). See multithread_queue::front
    		media::raw_mediadata_t* video = m_qvideo.front();

		// If the queue is empty either we're waiting for more data
		// to be decoded or we're out of data
		if (!video)
		{
			log_debug("refreshVideoFrame:: No more video frames in queue");

			if ( decodingStatus() == DEC_STOPPED )
			{
				if ( playbackStatus() != PLAY_STOPPED )
				{
					playbackStatus(PLAY_STOPPED);
//#ifdef GNASH_DEBUG_STATUS
					log_debug("Setting playStop status");
//#endif
					setStatus(playStop);
				}
			}
			else
			{
				// There no video but decoder is still running
				// not much to do here except wait for next call
				//assert(decodingStatus() == DEC_BUFFERING);
			}

			break;
		}

		// Caclulate the current time
		boost::uint32_t current_clock;
		if (m_ACodecCtx && _soundHandler)
		{
			current_clock = m_current_timestamp;
		}
		else
		{
			current_clock = clocktime::getTicks() - m_start_clock;
			m_current_timestamp = current_clock;
		}

		boost::uint32_t video_clock = video->m_pts;

		// If the timestamp on the videoframe is smaller than the
		// current time, we put it in the output image.
		if (current_clock >= video_clock)
		{

			if (m_videoFrameFormat == render::YUV)
			{
				if ( ! m_imageframe ) m_imageframe  = new image::yuv(m_VCodecCtx->width, m_VCodecCtx->height);
				// XXX m_imageframe might be a byte aligned buffer, while video is not!
				static_cast<image::yuv*>(m_imageframe)->update(video->m_data);
			}
			else if (m_videoFrameFormat == render::RGB)
			{
				if ( ! m_imageframe ) m_imageframe  = new image::rgb(m_VCodecCtx->width, m_VCodecCtx->height);
				image::rgb* imgframe = static_cast<image::rgb*>(m_imageframe);
				rgbcopy(imgframe, video, m_VCodecCtx->width * 3);
			}

			// Delete the frame from the queue
			m_qvideo.pop();
			delete video;

			// wake up filler (TODO: do only if decoder is running)
			// TODO2: resume only at end of loop ?
			_qFillerResume.notify_all();

			// A frame is ready for pickup
			m_newFrameReady = true;

		}
		else
		{
			// The timestamp on the first frame in the queue is greater
			// than the current time, so no need to do anything.
			break;
		}

	} while(!m_qvideo.empty());

#ifdef GNASH_DEBUG_THREADS
	log_debug("qMutex: releasing lock in refreshVideoFrame");
#endif
}


void
NetStreamFfmpeg::advance()
{
	//log_debug("advance");

	// Check if there are any new status messages, and if we should
	// pass them to a event handler
	processStatusNotifications();

	// Find video frame with the most suited timestamp in the video queue,
	// and put it in the output image frame.
	refreshVideoFrame();
}

boost::int32_t
NetStreamFfmpeg::time()
{

	if (m_FormatCtx && m_FormatCtx->nb_streams > 0)
	{
    		double time = (double)m_FormatCtx->streams[0]->time_base.num / (double)m_FormatCtx->streams[0]->time_base.den * (double)m_FormatCtx->streams[0]->cur_dts;
		return static_cast<boost::int32_t>(time);
	}
	else if
	(m_isFLV)
	{
		return m_current_timestamp;
	}
	else
	{
		return 0;
	}
}

void NetStreamFfmpeg::pausePlayback()
{
	GNASH_REPORT_FUNCTION;

	if (playbackStatus() == PLAY_PAUSED) return;

	playbackStatus(PLAY_PAUSED);

	// Save the current time so we later can tell how long the pause lasted
	m_time_of_pause = clocktime::getTicks();

	// Disconnect the soundhandler so we don't play while paused
	if ( _soundHandler ) _soundHandler->detach_aux_streamer((void*)this);
}

void NetStreamFfmpeg::unpausePlayback()
{
	GNASH_REPORT_FUNCTION;

	if (playbackStatus() == PLAY_PLAYING) // already playing
	{
		log_debug("unpausePlayback: already playing");
		return;
	}

	playbackStatus(PLAY_PLAYING);

	if (m_current_timestamp == 0)
	{
		m_start_clock = clocktime::getTicks();
	}
	else
	{
		// Add the paused time to the start time so that the playhead doesn't
		// noticed that we have been paused
		m_start_clock += clocktime::getTicks() - m_time_of_pause;
	}

	// (re)-connect to the soundhandler.
	// It was disconnected in ::pausePlayback to avoid to keep playing sound while paused
	if ( _soundHandler ) _soundHandler->attach_aux_streamer(audio_streamer, (void*) this);
}


long
NetStreamFfmpeg::bytesLoaded ()
{
  	long ret_val = 0;

  	if ( _netCon ) 
	{
    		ret_val = _netCon->getBytesLoaded();
  	}

  	return ret_val;
}


long
NetStreamFfmpeg::bytesTotal ()
{
  	long ret_val = 0;

  	if ( _netCon ) 
	{
    		ret_val = _netCon->getBytesTotal();
  	}

  	return ret_val;
}

NetStreamFfmpeg::PlaybackState
NetStreamFfmpeg::playbackStatus(PlaybackState newstate)
{
	boost::mutex::scoped_lock lock(_state_mutex);

	if (newstate != PLAY_NONE) {
		_playback_state = newstate;
	}

	return _playback_state;
}

NetStreamFfmpeg::DecodingState
NetStreamFfmpeg::decodingStatus(DecodingState newstate)
{
	boost::mutex::scoped_lock lock(_state_mutex);

	if (newstate != DEC_NONE) {
		_decoding_state = newstate;
	}

	return _decoding_state;
}


} // gnash namespcae

#endif // USE_FFMPEG

