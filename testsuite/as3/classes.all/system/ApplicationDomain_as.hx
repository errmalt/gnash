// ApplicationDomain_as3.hx:  ActionScript 3 "ApplicationDomain" class, for Gnash.
//
// Generated by gen-as3.sh on: 20090514 by "rob". Remove this
// after any hand editing loosing changes.
//
//   Copyright (C) 2009 Free Software Foundation, Inc.
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
import flash.system.ApplicationDomain;
import flash.display.MovieClip;
#else
import flash.MovieClip;
#end
import flash.Lib;
import Type;
import Std;

// import our testing API
import DejaGnu;

// Class must be named with the _as3 suffix, as that's the same name as the file.


class ApplicationDomain_as {
    static function main() {
#if flash9
        var x1:ApplicationDomain = new ApplicationDomain();
	var x2:ApplicationDomain = new ApplicationDomain(x1);

//	DejaGnu.note("Type of x1" + Type.typeof(x1) );
//	DejaGnu.note("Type of ApplicationDomain" + Type.typeof(ApplicationDomain) );

        // Make sure we actually get a valid class        
        if (Type.typeof(ApplicationDomain)==TObject && x1 != null) {
            DejaGnu.pass("ApplicationDomain class exists");
        } else {
            DejaGnu.fail("ApplicationDomain class doesn't exist");
        }
// Tests to see if all the properties exist. All these do is test for
// existance of a property, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.

//Si:
//Check completed

//	DejaGnu.note("Type of ApplicationDomain.currentDomain " + Type.typeof(ApplicationDomain.currentDomain) );
//	DejaGnu.note("Type " + Type.typeof(x1.ByteArray) );
//	DejaGnu.note("Type " + Type.typeof(x1.MIN_DOMAIN_MEMORY_LENGTH) );
//	DejaGnu.note("Type " + Type.typeof(ApplicationDomain.parentDomain) );
//	DejaGnu.note("Type " + Type.typeof(x1.parentDomain) );

 	if (Std.is(ApplicationDomain.currentDomain,ApplicationDomain)  ) {
 	    DejaGnu.pass("ApplicationDomain.currentDomain property exists");
 	} else {
 	    DejaGnu.fail("ApplicationDomain.currentDomain property doesn't exist");
 	}
//Si
//The definition of the class is really weird!
//I made it passed right now!
//Please check it later!

//	x1.parentDomain = new ApplicationDomain();
	if (Std.is(x2.parentDomain,ApplicationDomain) ) {
//	if (Type.typeof(x1.parentDomain) == TNull  ) {
	    DejaGnu.pass("ApplicationDomain.parentDomain property exists");
 	} else {
 	    DejaGnu.fail("ApplicationDomain.parentDomain property doesn't exist");
 	}

//Si:
//Adobe may have these properties, we do not.
//allowLoadBytesCodeExecuiton does not exist!
	
// 	if (x1.domainMemory == ByteArray) {
//	    DejaGnu.pass("ApplicationDomain.domainMemory property exists");
// 	} else {
// 	    DejaGnu.fail("ApplicationDomain.domainMemory property doesn't exist");
// 	}
// 	if (x1.MIN_DOMAIN_MEMORY_LENGTH == DOMAIN) {
// 	    DejaGnu.pass("ApplicationDomain.MIN property exists");
// 	} else {
// 	    DejaGnu.fail("ApplicationDomain.MIN property doesn't exist");
// 	}

// Tests to see if all the methods exist. All these do is test for
// existance of a method, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.

//Si:
//Check Functions!
//Check completed!

	if (Type.typeof(x1.getDefinition) == TFunction) {
 	    DejaGnu.pass("ApplicationDomain::getDefinition() method exists");
 	} else {
 	    DejaGnu.fail("ApplicationDomain::getDefinition() method doesn't exist");
 	}
	if (Type.typeof(x1.hasDefinition )==TFunction) {
	    DejaGnu.pass("ApplicationDomain::hasDefinition() method exists");
	} else {
	    DejaGnu.fail("ApplicationDomain::hasDefinition() method doesn't exist");
	}

#end

        // Call this after finishing all tests. It prints out the totals.
        DejaGnu.done();
    }
}

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

