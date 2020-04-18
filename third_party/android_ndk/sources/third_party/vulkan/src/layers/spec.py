#!/usr/bin/python3 -i

import sys
import xml.etree.ElementTree as etree
import urllib2

#############################
# spec.py script
#
# Overview - this script is intended to generate validation error codes and message strings from the xhtml version of
#  the specification. In addition to generating the header file, it provides a number of corrollary services to aid in
#  generating/updating the header.
#
# Ideal flow - Not there currently, but the ideal flow for this script would be that you run the script, it pulls the
#  latest spec, compares it to the current set of generated error codes, and makes any updates as needed
#
# Current flow - the current flow acheives all of the ideal flow goals, but with more steps than are desired
#  1. Get the spec - right now spec has to be manually generated or pulled from the web
#  2. Generate header from spec - This is done in a single command line
#  3. Generate database file from spec - Can be done along with step #2 above, the database file contains a list of
#      all error enums and message strings, along with some other info on if those errors are implemented/tested
#  4. Update header using a given database file as the root and a new spec file as goal - This makes sure that existing
#      errors keep the same enum identifier while also making sure that new errors get a unique_id that continues on
#      from the end of the previous highest unique_id.
#
# TODO:
#  1. Improve string matching to add more automation for figuring out which messages are changed vs. completely new
#
#############################


spec_filename = "vkspec.html" # can override w/ '-spec <filename>' option
out_filename = "vk_validation_error_messages.h" # can override w/ '-out <filename>' option
db_filename = "vk_validation_error_database.txt" # can override w/ '-gendb <filename>' option
gen_db = False # set to True when '-gendb <filename>' option provided
spec_compare = False # set to True with '-compare <db_filename>' option
# This is the root spec link that is used in error messages to point users to spec sections
#old_spec_url = "https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html"
spec_url = "https://www.khronos.org/registry/vulkan/specs/1.0-extensions/xhtml/vkspec.html"
# After the custom validation error message, this is the prefix for the standard message that includes the
#  spec valid usage language as well as the link to nearest section of spec to that language
error_msg_prefix = "For more information refer to Vulkan Spec Section "
ns = {'ns': 'http://www.w3.org/1999/xhtml'}
validation_error_enum_name = "VALIDATION_ERROR_"
# Dict of new enum values that should be forced to remap to old handles, explicitly set by -remap option
remap_dict = {}

def printHelp():
    print "Usage: python spec.py [-spec <specfile.html>] [-out <headerfile.h>] [-gendb <databasefile.txt>] [-compare <databasefile.txt>] [-update] [-remap <new_id-old_id,count>] [-help]"
    print "\n Default script behavior is to parse the specfile and generate a header of unique error enums and corresponding error messages based on the specfile.\n"
    print "  Default specfile is from online at %s" % (spec_url)
    print "  Default headerfile is %s" % (out_filename)
    print "  Default databasefile is %s" % (db_filename)
    print "\nIf '-gendb' option is specified then a database file is generated to default file or <databasefile.txt> if supplied. The database file stores"
    print "  the list of enums and their error messages."
    print "\nIf '-compare' option is specified then the given database file will be read in as the baseline for generating the new specfile"
    print "\nIf '-update' option is specified this triggers the master flow to automate updating header and database files using default db file as baseline"
    print "  and online spec file as the latest. The default header and database files will be updated in-place for review and commit to the git repo."
    print "\nIf '-remap' option is specified it supplies forced remapping from new enum ids to old enum ids. This should only be specified along with -update"
    print "  option. Starting at newid and remapping to oldid, count ids will be remapped. Default count is '1' and use ':' to specify multiple remappings."

class Specification:
    def __init__(self):
        self.tree   = None
        self.val_error_dict = {} # string for enum is key that references 'error_msg' and 'api'
        self.error_db_dict = {} # dict of previous error values read in from database file
        self.delimiter = '~^~' # delimiter for db file
        self.copyright = """/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Tobin Ehlis <tobine@google.com>
 */"""
    def _checkInternetSpec(self):
        """Verify that we can access the spec online"""
        try:
            online = urllib2.urlopen(spec_url,timeout=1)
            return True
        except urllib2.URLError as err:
            return False
        return False
    def loadFile(self, online=True, spec_file=spec_filename):
        """Load an API registry XML file into a Registry object and parse it"""
        # Check if spec URL is available
        if (online and self._checkInternetSpec()):
            print "Using spec from online at %s" % (spec_url)
            self.tree = etree.parse(urllib2.urlopen(spec_url))
        else:
            print "Using local spec %s" % (spec_file)
            self.tree = etree.parse(spec_file)
        #self.tree.write("tree_output.xhtml")
        #self.tree = etree.parse("tree_output.xhtml")
        self.parseTree()
    def updateDict(self, updated_dict):
        """Assign internal dict to use updated_dict"""
        self.val_error_dict = updated_dict
    def parseTree(self):
        """Parse the registry Element, once created"""
        print "Parsing spec file..."
        unique_enum_id = 0
        self.root = self.tree.getroot()
        #print "ROOT: %s" % self.root
        prev_heading = '' # Last seen section heading or sub-heading
        prev_link = '' # Last seen link id within the spec
        api_function = '' # API call that a check appears under
        error_strings = set() # Flag any exact duplicate error strings and skip them
        for tag in self.root.iter(): # iterate down tree
            # Grab most recent section heading and link
            if tag.tag in ['{http://www.w3.org/1999/xhtml}h2', '{http://www.w3.org/1999/xhtml}h3']:
                if tag.get('class') != 'title':
                    continue
                #print "Found heading %s" % (tag.tag)
                prev_heading = "".join(tag.itertext())
                # Insert a space between heading number & title
                sh_list = prev_heading.rsplit('.', 1)
                prev_heading = '. '.join(sh_list)
                prev_link = tag[0].get('id')
                #print "Set prev_heading %s to have link of %s" % (prev_heading.encode("ascii", "ignore"), prev_link.encode("ascii", "ignore"))
            elif tag.tag == '{http://www.w3.org/1999/xhtml}a': # grab any intermediate links
                if tag.get('id') != None:
                    prev_link = tag.get('id')
                    #print "Updated prev link to %s" % (prev_link)
            elif tag.tag == '{http://www.w3.org/1999/xhtml}pre' and tag.get('class') == 'programlisting':
                # Check and see if this is API function
                code_text = "".join(tag.itertext()).replace('\n', '')
                code_text_list = code_text.split()
                if len(code_text_list) > 1 and code_text_list[1].startswith('vk'):
                    api_function = code_text_list[1].strip('(')
                    print "Found API function: %s" % (api_function)
            elif tag.tag == '{http://www.w3.org/1999/xhtml}div' and tag.get('class') == 'sidebar':
                # parse down sidebar to check for valid usage cases
                valid_usage = False
                for elem in tag.iter():
                    if elem.tag == '{http://www.w3.org/1999/xhtml}strong' and None != elem.text and 'Valid Usage' in elem.text:
                        valid_usage = True
                    elif valid_usage and elem.tag == '{http://www.w3.org/1999/xhtml}li': # grab actual valid usage requirements
                        error_msg_str = "%s '%s' which states '%s' (%s#%s)" % (error_msg_prefix, prev_heading, "".join(elem.itertext()).replace('\n', ''), spec_url, prev_link)
                        # Some txt has multiple spaces so split on whitespace and join w/ single space
                        error_msg_str = " ".join(error_msg_str.split())
                        if error_msg_str in error_strings:
                            print "WARNING: SKIPPING adding repeat entry for string. Please review spec and file issue as appropriate. Repeat string is: %s" % (error_msg_str)
                        else:
                            error_strings.add(error_msg_str)
                            enum_str = "%s%05d" % (validation_error_enum_name, unique_enum_id)
                            # TODO : '\' chars in spec error messages are most likely bad spec txt that needs to be updated
                            self.val_error_dict[enum_str] = {}
                            self.val_error_dict[enum_str]['error_msg'] = error_msg_str.encode("ascii", "ignore").replace("\\", "/")
                            self.val_error_dict[enum_str]['api'] = api_function
                            unique_enum_id = unique_enum_id + 1
        #print "Validation Error Dict has a total of %d unique errors and contents are:\n%s" % (unique_enum_id, self.val_error_dict)
    def genHeader(self, header_file):
        """Generate a header file based on the contents of a parsed spec"""
        print "Generating header %s..." % (header_file)
        file_contents = []
        file_contents.append(self.copyright)
        file_contents.append('\n#pragma once')
        file_contents.append('#include <unordered_map>')
        file_contents.append('\n// enum values for unique validation error codes')
        file_contents.append('//  Corresponding validation error message for each enum is given in the mapping table below')
        file_contents.append('//  When a given error occurs, these enum values should be passed to the as the messageCode')
        file_contents.append('//  parameter to the PFN_vkDebugReportCallbackEXT function')
        enum_decl = ['enum UNIQUE_VALIDATION_ERROR_CODE {']
        error_string_map = ['static std::unordered_map<int, char const *const> validation_error_map{']
        for enum in sorted(self.val_error_dict):
            #print "Header enum is %s" % (enum)
            enum_decl.append('    %s = %d,' % (enum, int(enum.split('_')[-1])))
            error_string_map.append('    {%s, "%s"},' % (enum, self.val_error_dict[enum]['error_msg']))
        enum_decl.append('    %sMAX_ENUM = %d,' % (validation_error_enum_name, int(enum.split('_')[-1]) + 1))
        enum_decl.append('};')
        error_string_map.append('};\n')
        file_contents.extend(enum_decl)
        file_contents.append('\n// Mapping from unique validation error enum to the corresponding error message')
        file_contents.append('// The error message should be appended to the end of a custom error message that is passed')
        file_contents.append('// as the pMessage parameter to the PFN_vkDebugReportCallbackEXT function')
        file_contents.extend(error_string_map)
        #print "File contents: %s" % (file_contents)
        with open(header_file, "w") as outfile:
            outfile.write("\n".join(file_contents))
    def analyze(self):
        """Print out some stats on the valid usage dict"""
        # Create dict for # of occurences of identical strings
        str_count_dict = {}
        unique_id_count = 0
        for enum in self.val_error_dict:
            err_str = self.val_error_dict[enum]['error_msg']
            if err_str in str_count_dict:
                print "Found repeat error string"
                str_count_dict[err_str] = str_count_dict[err_str] + 1
            else:
                str_count_dict[err_str] = 1
            unique_id_count = unique_id_count + 1
        print "Processed %d unique_ids" % (unique_id_count)
        repeat_string = 0
        for es in str_count_dict:
            if str_count_dict[es] > 1:
                repeat_string = repeat_string + 1
                print "String '%s' repeated %d times" % (es, repeat_string)
        print "Found %d repeat strings" % (repeat_string)
    def genDB(self, db_file):
        """Generate a database of check_enum, check_coded?, testname, error_string"""
        db_lines = []
        # Write header for database file
        db_lines.append("# This is a database file with validation error check information")
        db_lines.append("# Comments are denoted with '#' char")
        db_lines.append("# The format of the lines is:")
        db_lines.append("# <error_enum>%s<check_implemented>%s<testname>%s<api>%s<errormsg>%s<note>" % (self.delimiter, self.delimiter, self.delimiter, self.delimiter, self.delimiter))
        db_lines.append("# error_enum: Unique error enum for this check of format %s<uniqueid>" % validation_error_enum_name)
        db_lines.append("# check_implemented: 'Y' if check has been implemented in layers, 'U' for unknown, or 'N' for not implemented")
        db_lines.append("# testname: Name of validation test for this check, 'Unknown' for unknown, or 'None' if not implmented")
        db_lines.append("# api: Vulkan API function that this check is related to")
        db_lines.append("# errormsg: The unique error message for this check that includes spec language and link")
        db_lines.append("# note: Free txt field with any custom notes related to the check in question")
        for enum in sorted(self.val_error_dict):
            # Default to unknown if check or test are implemented, then update below if appropriate
            implemented = 'U'
            testname = 'Unknown'
            note = ''
            # If we have an existing db entry for this enum, use its implemented/testname values
            if enum in self.error_db_dict:
                implemented = self.error_db_dict[enum]['check_implemented']
                testname = self.error_db_dict[enum]['testname']
                note = self.error_db_dict[enum]['note']
            #print "delimiter: %s, id: %s, str: %s" % (self.delimiter, enum, self.val_error_dict[enum])
            # No existing entry so default to N for implemented and None for testname
            db_lines.append("%s%s%s%s%s%s%s%s%s%s%s" % (enum, self.delimiter, implemented, self.delimiter, testname, self.delimiter, self.val_error_dict[enum]['api'], self.delimiter, self.val_error_dict[enum]['error_msg'], self.delimiter, note))
        print "Generating database file %s" % (db_file)
        with open(db_file, "w") as outfile:
            outfile.write("\n".join(db_lines))
            outfile.write("\n")
    def readDB(self, db_file):
        """Read a db file into a dict, format of each line is <enum><implemented Y|N?><testname><errormsg>"""
        db_dict = {} # This is a simple db of just enum->errormsg, the same as is created from spec
        max_id = 0
        with open(db_file, "r") as infile:
            for line in infile:
                if line.startswith('#'):
                    continue
                line = line.strip()
                db_line = line.split(self.delimiter)
                if len(db_line) != 6:
                    print "ERROR: Bad database line doesn't have 6 elements: %s" % (line)
                error_enum = db_line[0]
                implemented = db_line[1]
                testname = db_line[2]
                api = db_line[3]
                error_str = db_line[4]
                note = db_line[5]
                db_dict[error_enum] = error_str
                # Also read complete database contents into our class var for later use
                self.error_db_dict[error_enum] = {}
                self.error_db_dict[error_enum]['check_implemented'] = implemented
                self.error_db_dict[error_enum]['testname'] = testname
                self.error_db_dict[error_enum]['api'] = api
                self.error_db_dict[error_enum]['error_string'] = error_str
                self.error_db_dict[error_enum]['note'] = note
                unique_id = int(db_line[0].split('_')[-1])
                if unique_id > max_id:
                    max_id = unique_id
        return (db_dict, max_id)
    # Compare unique ids from original database to data generated from updated spec
    # 1. If a new id and error code exactly match original, great
    # 2. If new id is not in original, but exact error code is, need to use original error code
    # 3. If new id and new error are not in original, make sure new id picks up from end of original list
    # 4. If new id in original, but error strings don't match then:
    #   4a. If error string has exact match in original, update new to use original
    #   4b. If error string not in original, may be updated error message, manually address
    def compareDB(self, orig_error_msg_dict, max_id):
        """Compare orig database dict to new dict, report out findings, and return potential new dict for parsed spec"""
        # First create reverse dicts of err_strings to IDs
        next_id = max_id + 1
        orig_err_to_id_dict = {}
        # Create an updated dict in-place that will be assigned to self.val_error_dict when done
        updated_val_error_dict = {}
        for enum in orig_error_msg_dict:
            orig_err_to_id_dict[orig_error_msg_dict[enum]] = enum
        new_err_to_id_dict = {}
        for enum in self.val_error_dict:
            new_err_to_id_dict[self.val_error_dict[enum]['error_msg']] = enum
        ids_parsed = 0
        # Values to be used for the update dict
        update_enum = ''
        update_msg = ''
        update_api = ''
        # Now parse through new dict and figure out what to do with non-matching things
        for enum in sorted(self.val_error_dict):
            ids_parsed = ids_parsed + 1
            enum_list = enum.split('_') # grab sections of enum for use below
            # Default update values to be the same
            update_enum = enum
            update_msg = self.val_error_dict[enum]['error_msg']
            update_api = self.val_error_dict[enum]['api']
            # Any user-forced remap takes precendence
            if enum_list[-1] in remap_dict:
                enum_list[-1] = remap_dict[enum_list[-1]]
                new_enum = "_".join(enum_list)
                print "NOTE: Using user-supplied remap to force %s to be %s" % (enum, new_enum)
                update_enum = new_enum
            elif enum in orig_error_msg_dict:
                if self.val_error_dict[enum]['error_msg'] == orig_error_msg_dict[enum]:
                    print "Exact match for enum %s" % (enum)
                    # Nothing to see here
                    if enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (enum)
                elif self.val_error_dict[enum]['error_msg'] in orig_err_to_id_dict:
                    # Same value w/ different error id, need to anchor to original id
                    print "Need to switch new id %s to original id %s" % (enum, orig_err_to_id_dict[self.val_error_dict[enum]['error_msg']])
                    # Update id at end of new enum to be same id from original enum
                    enum_list[-1] = orig_err_to_id_dict[self.val_error_dict[enum]['error_msg']].split('_')[-1]
                    new_enum = "_".join(enum_list)
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    update_enum = new_enum
                else:
                    # No error match:
                    #  First check if only link has changed, in which case keep ID but update message
                    orig_msg_list = orig_error_msg_dict[enum].split('(', 1)
                    new_msg_list = self.val_error_dict[enum]['error_msg'].split('(', 1)
                    if orig_msg_list[0] == new_msg_list[0]: # Msg is same bug link has changed, keep enum & update msg
                        print "NOTE: Found that only spec link changed for %s so keeping same id w/ new link" % (enum)
                    #  This seems to be a new error so need to pick it up from end of original unique ids & flag for review
                    else:
                        enum_list[-1] = "%05d" % (next_id)
                        new_enum = "_".join(enum_list)
                        next_id = next_id + 1
                        print "MANUALLY VERIFY: Updated new enum %s to be unique %s. Make sure new error msg is actually unique and not just changed" % (enum, new_enum)
                        print "   New error string: %s" % (self.val_error_dict[enum]['error_msg'])
                        if new_enum in updated_val_error_dict:
                            print "ERROR: About to overwrite entry for %s" % (new_enum)
                        update_enum = new_enum
            else: # new enum is not in orig db
                if self.val_error_dict[enum]['error_msg'] in orig_err_to_id_dict:
                    print "New enum %s not in orig dict, but exact error message matches original unique id %s" % (enum, orig_err_to_id_dict[self.val_error_dict[enum]['error_msg']])
                    # Update new unique_id to use original
                    enum_list[-1] = orig_err_to_id_dict[self.val_error_dict[enum]['error_msg']].split('_')[-1]
                    new_enum = "_".join(enum_list)
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    update_enum = new_enum
                else:
                    enum_list[-1] = "%05d" % (next_id)
                    new_enum = "_".join(enum_list)
                    next_id = next_id + 1
                    print "Completely new id and error code, update new id from %s to unique %s" % (enum, new_enum)
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    update_enum = new_enum
            updated_val_error_dict[update_enum] = {}
            updated_val_error_dict[update_enum]['error_msg'] = update_msg
            updated_val_error_dict[update_enum]['api'] = update_api
        # Assign parsed dict to be the udpated dict based on db compare
        print "In compareDB parsed %d entries" % (ids_parsed)
        return updated_val_error_dict
    def validateUpdateDict(self, update_dict):
        """Compare original dict vs. update dict and make sure that all of the checks are still there"""
        # Currently just make sure that the same # of checks as the original checks are there
        #orig_ids = {}
        orig_id_count = len(self.val_error_dict)
        #update_ids = {}
        update_id_count = len(update_dict)
        if orig_id_count != update_id_count:
            print "Original dict had %d unique_ids, but updated dict has %d!" % (orig_id_count, update_id_count)
            return False
        print "Original dict and updated dict both have %d unique_ids. Great!" % (orig_id_count)
        return True
        # TODO : include some more analysis

# User passes in arg of form <new_id1>-<old_id1>[,count1]:<new_id2>-<old_id2>[,count2]:...
#  new_id# = the new enum id that was assigned to an error
#  old_id# = the previous enum id that was assigned to the same error
#  [,count#] = The number of ids to remap starting at new_id#=old_id# and ending at new_id[#+count#-1]=old_id[#+count#-1]
#     If not supplied, then ,1 is assumed, which will only update a single id
def updateRemapDict(remap_string):
    """Set up global remap_dict based on user input"""
    remap_list = remap_string.split(":")
    for rmap in remap_list:
        count = 1 # Default count if none supplied
        id_count_list = rmap.split(',')
        if len(id_count_list) > 1:
            count = int(id_count_list[1])
        new_old_id_list = id_count_list[0].split('-')
        for offset in range(count):
            remap_dict["%05d" % (int(new_old_id_list[0]) + offset)] = "%05d" % (int(new_old_id_list[1]) + offset)
    for new_id in sorted(remap_dict):
        print "Set to remap new id %s to old id %s" % (new_id, remap_dict[new_id])

if __name__ == "__main__":
    i = 1
    use_online = True # Attempt to grab spec from online by default
    update_option = False
    while (i < len(sys.argv)):
        arg = sys.argv[i]
        i = i + 1
        if (arg == '-spec'):
            spec_filename = sys.argv[i]
            # If user specifies local specfile, skip online
            use_online = False
            i = i + 1
        elif (arg == '-out'):
            out_filename = sys.argv[i]
            i = i + 1
        elif (arg == '-gendb'):
            gen_db = True
            # Set filename if supplied, else use default
            if i < len(sys.argv) and not sys.argv[i].startswith('-'):
                db_filename = sys.argv[i]
                i = i + 1
        elif (arg == '-compare'):
            db_filename = sys.argv[i]
            spec_compare = True
            i = i + 1
        elif (arg == '-update'):
            update_option = True
            spec_compare = True
            gen_db = True
        elif (arg == '-remap'):
            updateRemapDict(sys.argv[i])
            i = i + 1
        elif (arg in ['-help', '-h']):
            printHelp()
            sys.exit()
    if len(remap_dict) > 1 and not update_option:
        print "ERROR: '-remap' option can only be used along with '-update' option. Exiting."
        sys.exit()
    spec = Specification()
    spec.loadFile(use_online, spec_filename)
    #spec.parseTree()
    #spec.genHeader(out_filename)
    spec.analyze()
    if (spec_compare):
        # Read in old spec info from db file
        (orig_err_msg_dict, max_id) = spec.readDB(db_filename)
        # New spec data should already be read into self.val_error_dict
        updated_dict = spec.compareDB(orig_err_msg_dict, max_id)
        update_valid = spec.validateUpdateDict(updated_dict)
        if update_valid:
            spec.updateDict(updated_dict)
        else:
            sys.exit()
    if (gen_db):
        spec.genDB(db_filename)
    print "Writing out file (-out) to '%s'" % (out_filename)
    spec.genHeader(out_filename)

##### Example dataset
# <div class="sidebar">
#   <div class="titlepage">
#     <div>
#       <div>
#         <p class="title">
#           <strong>Valid Usage</strong> # When we get to this guy, we know we're under interesting sidebar
#         </p>
#       </div>
#     </div>
#   </div>
# <div class="itemizedlist">
#   <ul class="itemizedlist" style="list-style-type: disc; ">
#     <li class="listitem">
#       <em class="parameter">
#         <code>device</code>
#       </em>
#       <span class="normative">must</span> be a valid
#       <code class="code">VkDevice</code> handle
#     </li>
#     <li class="listitem">
#       <em class="parameter">
#         <code>commandPool</code>
#       </em>
#       <span class="normative">must</span> be a valid
#       <code class="code">VkCommandPool</code> handle
#     </li>
#     <li class="listitem">
#       <em class="parameter">
#         <code>flags</code>
#       </em>
#       <span class="normative">must</span> be a valid combination of
#       <code class="code">
#         <a class="link" href="#VkCommandPoolResetFlagBits">VkCommandPoolResetFlagBits</a>
#       </code> values
#     </li>
#     <li class="listitem">
#       <em class="parameter">
#         <code>commandPool</code>
#       </em>
#       <span class="normative">must</span> have been created, allocated, or retrieved from
#       <em class="parameter">
#         <code>device</code>
#       </em>
#     </li>
#     <li class="listitem">All
#       <code class="code">VkCommandBuffer</code>
#       objects allocated from
#       <em class="parameter">
#         <code>commandPool</code>
#       </em>
#       <span class="normative">must</span> not currently be pending execution
#     </li>
#   </ul>
# </div>
# </div>
##### Second example dataset
# <div class="sidebar">
#   <div class="titlepage">
#     <div>
#       <div>
#         <p class="title">
#           <strong>Valid Usage</strong>
#         </p>
#       </div>
#     </div>
#   </div>
#   <div class="itemizedlist">
#     <ul class="itemizedlist" style="list-style-type: disc; ">
#       <li class="listitem">The <em class="parameter"><code>queueFamilyIndex</code></em> member of any given element of <em class="parameter"><code>pQueueCreateInfos</code></em> <span class="normative">must</span> be unique within <em class="parameter"><code>pQueueCreateInfos</code></em>
#       </li>
#     </ul>
#   </div>
# </div>
# <div class="sidebar">
#   <div class="titlepage">
#     <div>
#       <div>
#         <p class="title">
#           <strong>Valid Usage (Implicit)</strong>
#         </p>
#       </div>
#     </div>
#   </div>
#   <div class="itemizedlist"><ul class="itemizedlist" style="list-style-type: disc; "><li class="listitem">
#<em class="parameter"><code>sType</code></em> <span class="normative">must</span> be <code class="code">VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO</code>
#</li><li class="listitem">
#<em class="parameter"><code>pNext</code></em> <span class="normative">must</span> be <code class="literal">NULL</code>
#</li><li class="listitem">
#<em class="parameter"><code>flags</code></em> <span class="normative">must</span> be <code class="literal">0</code>
#</li><li class="listitem">
#<em class="parameter"><code>pQueueCreateInfos</code></em> <span class="normative">must</span> be a pointer to an array of <em class="parameter"><code>queueCreateInfoCount</code></em> valid <code class="code">VkDeviceQueueCreateInfo</code> structures
#</li>