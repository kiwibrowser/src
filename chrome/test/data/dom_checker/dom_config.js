/*

   DOM checker - configuration parameters
   --------------------------------------

   Please be sure to update these to reflect the realities of the place where
   you host the program.

   Authors: Michal Zalewski <lcamtuf@google.com>
            Filipe Almeida <filipe@google.com>

   Copyright 2008 by Google Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/


/* Host name where you intend to put the script: */
var main_host = 'localhost:8000';

/* Subdirectory for DOM checker files: */
var main_dir = 'dom_checker';

/* An alternative way to call the same resource in a manner that
   appears to the browser as completely unrelated to main_host
   (try IP address): */
var alt_host  = '127.0.0.1:8000';

/* Subdirectory for DOM checker files: */
var alt_dir = 'dom_checker';

/* DOM properties or hierarchies we do not want to enumerate and
   randomly write during primary checks because of their disruptive
   nature. */

var write_blacklist = {
  'location': 1
};


/* DOM properties or hierarchies we do not want to attempt to read,
   and methods we do not want to call, because they either have no
   security impact at all, or the ability to read/access does not
   reliably imply any privileges. */

var read_blacklist = {
  'top' : 2,		// Calling frame
  'parent' : 3,		// Calling frame
  'frames' : 4,		// Lower level access not implied
  'document' : 5,	// Lower level access not implied
  'self' : 6,		// Lower level access not implied
  'history' : 7,	// Lower level access not implied
  'close' : 8,		// Access does not imply success
  'focus' : 9,		// Access does not imply success
  'blur'  : 10,		// Access does not imply success
  'closed' : 11,	// Not very revealing
  'opener' : 12,	// Ditto.
  'window' : 13,	// Ditto.
  'open' : 14		// Firefox oddity, but deemed harmless.
};
