// ExternalInterface_as.h:  ActionScript "ExternalInterface" class, for Gnash.
//
//   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
//   Foundation, Inc
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

#ifndef GNASH_ASOBJ_EXTERNALINTERFACE_H
#define GNASH_ASOBJ_EXTERNALINTERFACE_H

#include <string>
#include <vector>

#include "Relay.h" // for inheritance

namespace gnash {

class as_object;
class as_value;
class ObjectURI;

class ExternalInterface_as : public ActiveRelay
{
public:
    ExternalInterface_as(as_object* owner);
    ~ExternalInterface_as();

    /// Add an ActionScript function as a callback by JavaScript
    // in the browser.
    bool addCallback(const std::string &name, as_object *method);

    // This is a flag that specifies wether exceptions in ActionScript
    // should be propogated to JavaScript in the browser.
    void marshallExceptions(bool flag) { _exceptions = flag; };
    bool marshallExceptions() { return _exceptions; };

    // Returns the id attribute of the object tag in Internet Explorer,
    // or the name attribute of the embed tag in Netscape. 
    std::string &objectID() { return _objectid; };
    
    /// Call a callback if it's registered already.
    bool call(as_object* asCallback, const std::string& methodName,
              const std::vector<as_value>& args, size_t firstArg);

    // These appear to be undocumented helper functions of this class
    // that while propably designed to be used internally, get used
    // by ActionScript coders.

// check(EI.hasOwnProperty("_argumentsToXML"));
// check(EI.hasOwnProperty("_argumentsToAS"));
    
    std::string objectToXML(as_object &obj);
    std::string objectToJS(as_object &obj);
    std::string objectToAS(as_object &obj);
    
// check(EI.hasOwnProperty("_arrayToXML"));
// check(EI.hasOwnProperty("_arrayToJS"));
// check(EI.hasOwnProperty("_arrayToAS"));

// check(EI.hasOwnProperty("_toXML"));
// check(EI.hasOwnProperty("_toJS"));
// check(EI.hasOwnProperty("_toAS"));

// check(EI.hasOwnProperty("_jsQuoteString"));
// check(EI.hasOwnProperty("_initJS"));
// check(EI.hasOwnProperty("_evalJS"));
    
// check(EI.hasOwnProperty("_callOut"));
// check(EI.hasOwnProperty("_callIn"));

    std::string objectID(as_object &obj);
    std::string toXML(as_object &obj);
    
    std::string escapeXML(as_object &obj);
    std::string unescapeXML(as_object &obj);
    
private:
    std::string _objectid;
    bool        _exceptions;
};

/// Initialize the global ExternalInterface class
void externalinterface_class_init(as_object& where, const ObjectURI& uri);

} // end of gnash namespace

// __GNASH_ASOBJ_EXTERNALINTERFACE_H__
#endif

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
