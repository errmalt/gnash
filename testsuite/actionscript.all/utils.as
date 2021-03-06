// 
//   Copyright (C) 2005, 2006, 2007, 2009, 2010 Free Software
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

// Utilities for ActionScript tests

//
// Return a string containing a dump of the given object
//
// Usage: 
//	var o = new Array();
//	trace(dumpObject(o));
//
//
dumpObject = function(obj, indent)
{
        var s = '';
        //if ( typeof(obj) == 'object' )

        if ( indent == undefined ) indent = 0;

        for (var i in obj)
        {
                var value = obj[i];
                for (var j=0; j<indent; j++) s += ' ';
                if ( typeof(value) == 'object' ) {
                        s += i+" (object):\n";
                        s += dumpObject(value, indent+1);
                } else {
                        s += i+'='+value+' ('+typeof(value)+')\n';
                }
        }
        return s;
};

