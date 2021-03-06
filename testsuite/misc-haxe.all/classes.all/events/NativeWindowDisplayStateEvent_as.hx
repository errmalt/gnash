// NativeWindowDisplayStateEvent_as.hx:  ActionScript 3 "NativeWindowDisplayStateEvent" class, for Gnash.
//
// Generated by gen-as3.sh on: 20090515 by "rob". Remove this
// after any hand editing loosing changes.
//
//   Copyright (C) 2009, 2010 Free Software Foundation, Inc.
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

// This test case must be processed by CPP before compiling to include the
//  DejaGnu.hx header file for the testing framework support.

#if flash9
import flash.events.NativeWindowDisplayStateEvent;
import flash.display.MovieClip;
#else
import flash.NativeWindowDisplayStateEvent;
import flash.MovieClip;
#end
import flash.Lib;
import Type;

// import our testing API
import DejaGnu;

// Class must be named with the _as suffix, as that's the same name as the file.
class NativeWindowDisplayStateEvent_as {
    static function main() {
        var x1:NativeWindowDisplayStateEvent = new NativeWindowDisplayStateEvent();

        // Make sure we actually get a valid class        
        if (x1 != null) {
            DejaGnu.pass("NativeWindowDisplayStateEvent class exists");
        } else {
            DejaGnu.fail("NativeWindowDisplayStateEvent class doesn't exist");
        }
// Tests to see if all the properties exist. All these do is test for
// existance of a property, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (x1.afterDisplayState == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent.afterDisplayState property exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent.afterDisplayState property doesn't exist");
	}
	if (x1.beforeDisplayState == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent.beforeDisplayState property exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent.beforeDisplayState property doesn't exist");
	}

// Tests to see if all the methods exist. All these do is test for
// existance of a method, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (x1.NativeWindowDisplayStateEvent == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::NativeWindowDisplayStateEvent() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::NativeWindowDisplayStateEvent() method doesn't exist");
	}
	if (x1.clone == Event) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::clone() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::clone() method doesn't exist");
	}
	if (x1.toString == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::toString() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::toString() method doesn't exist");
	}
	if (x1.ACTIVATE == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::ACTIVATE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::ACTIVATE() method doesn't exist");
	}
	if (x1.ADDED == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::ADDED() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::ADDED() method doesn't exist");
	}
	if (x1.ADDED == TO) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::ADDED() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::ADDED() method doesn't exist");
	}
	if (x1.CANCEL == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::CANCEL() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::CANCEL() method doesn't exist");
	}
	if (x1.CHANGE == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::CHANGE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::CHANGE() method doesn't exist");
	}
	if (x1.CLOSE == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::CLOSE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::CLOSE() method doesn't exist");
	}
	if (x1.CLOSING == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::CLOSING() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::CLOSING() method doesn't exist");
	}
	if (x1.COMPLETE == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::COMPLETE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::COMPLETE() method doesn't exist");
	}
	if (x1.CONNECT == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::CONNECT() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::CONNECT() method doesn't exist");
	}
	if (x1.DEACTIVATE == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::DEACTIVATE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::DEACTIVATE() method doesn't exist");
	}
	if (x1.DISPLAYING == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::DISPLAYING() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::DISPLAYING() method doesn't exist");
	}
	if (x1.DISPLAY == STATE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::DISPLAY() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::DISPLAY() method doesn't exist");
	}
	if (x1.DISPLAY == STATE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::DISPLAY() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::DISPLAY() method doesn't exist");
	}
	if (x1.ENTER == FRAME) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::ENTER() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::ENTER() method doesn't exist");
	}
	if (x1.EXITING == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::EXITING() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::EXITING() method doesn't exist");
	}
	if (x1.FULLSCREEN == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::FULLSCREEN() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::FULLSCREEN() method doesn't exist");
	}
	if (x1.HTML == BOUNDS) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::HTML() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::HTML() method doesn't exist");
	}
	if (x1.HTML == DOM) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::HTML() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::HTML() method doesn't exist");
	}
	if (x1.HTML == RENDER) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::HTML() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::HTML() method doesn't exist");
	}
	if (x1.ID3 == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::ID3() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::ID3() method doesn't exist");
	}
	if (x1.INIT == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::INIT() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::INIT() method doesn't exist");
	}
	if (x1.LOCATION == CHANGE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::LOCATION() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::LOCATION() method doesn't exist");
	}
	if (x1.MOUSE == LEAVE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::MOUSE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::MOUSE() method doesn't exist");
	}
	if (x1.NETWORK == CHANGE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::NETWORK() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::NETWORK() method doesn't exist");
	}
	if (x1.OPEN == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::OPEN() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::OPEN() method doesn't exist");
	}
	if (x1.REMOVED == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::REMOVED() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::REMOVED() method doesn't exist");
	}
	if (x1.REMOVED == FROM) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::REMOVED() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::REMOVED() method doesn't exist");
	}
	if (x1.RENDER == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::RENDER() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::RENDER() method doesn't exist");
	}
	if (x1.RESIZE == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::RESIZE() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::RESIZE() method doesn't exist");
	}
	if (x1.SCROLL == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::SCROLL() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::SCROLL() method doesn't exist");
	}
	if (x1.SELECT == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::SELECT() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::SELECT() method doesn't exist");
	}
	if (x1.SOUND == COMPLETE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::SOUND() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::SOUND() method doesn't exist");
	}
	if (x1.TAB == CHILDREN) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::TAB() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::TAB() method doesn't exist");
	}
	if (x1.TAB == ENABLED) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::TAB() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::TAB() method doesn't exist");
	}
	if (x1.TAB == INDEX) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::TAB() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::TAB() method doesn't exist");
	}
	if (x1.UNLOAD == null) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::UNLOAD() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::UNLOAD() method doesn't exist");
	}
	if (x1.USER == IDLE) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::USER() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::USER() method doesn't exist");
	}
	if (x1.USER == PRESENT) {
	    DejaGnu.pass("NativeWindowDisplayStateEvent::USER() method exists");
	} else {
	    DejaGnu.fail("NativeWindowDisplayStateEvent::USER() method doesn't exist");
	}

        // Call this after finishing all tests. It prints out the totals.
        DejaGnu.done();
    }
}

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

