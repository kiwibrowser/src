#!/usr/bin/env python3
#
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
# Author: Tobin Ehlis <tobin@lunarg.com>

import argparse
import os
import sys
import re
import vulkan
from source_line_info import sourcelineinfo

# vk_helper.py overview
# This script generates code based on vulkan input header
#  It generate wrappers functions that can be used to display
#  structs in a human-readable txt format, as well as utility functions
#  to print enum values as strings

def handle_args():
    parser = argparse.ArgumentParser(description='Perform analysis of vogl trace.')
    parser.add_argument('input_file', help='The input header file from which code will be generated.')
    parser.add_argument('--rel_out_dir', required=False, default='vktrace_gen', help='Path relative to exec path to write output files. Will be created if needed.')
    parser.add_argument('--abs_out_dir', required=False, default=None, help='Absolute path to write output files. Will be created if needed.')
    parser.add_argument('--gen_enum_string_helper', required=False, action='store_true', default=False, help='Enable generation of helper header file to print string versions of enums.')
    parser.add_argument('--gen_struct_wrappers', required=False, action='store_true', default=False, help='Enable generation of struct wrapper classes.')
    parser.add_argument('--gen_struct_sizes', required=False, action='store_true', default=False, help='Enable generation of struct sizes.')
    parser.add_argument('--gen_cmake', required=False, action='store_true', default=False, help='Enable generation of cmake file for generated code.')
    parser.add_argument('--gen_graphviz', required=False, action='store_true', default=False, help='Enable generation of graphviz dot file.')
    #parser.add_argument('--test', action='store_true', default=False, help='Run simple test.')
    return parser.parse_args()

# TODO : Ideally these data structs would be opaque to user and could be wrapped
#   in their own class(es) to make them friendly for data look-up
# Dicts for Data storage
# enum_val_dict[value_name] = dict keys are the string enum value names for all enums
#    |-------->['type'] = the type of enum class into which the value falls
#    |-------->['val'] = the value assigned to this particular value_name
#    '-------->['unique'] = bool designating if this enum 'val' is unique within this enum 'type'
enum_val_dict = {}
# enum_type_dict['type'] = the type or class of of enum
#  '----->['val_name1', 'val_name2'...] = each type references a list of val_names within this type
enum_type_dict = {}
# struct_dict['struct_basename'] = the base (non-typedef'd) name of the struct
#  |->[<member_num>] = members are stored via their integer placement in the struct
#  |    |->['name'] = string name of this struct member
# ...   |->['full_type'] = complete type qualifier for this member
#       |->['type'] = base type for this member
#       |->['ptr'] = bool indicating if this member is ptr
#       |->['const'] = bool indicating if this member is const
#       |->['struct'] = bool indicating if this member is a struct type
#       |->['array'] = bool indicating if this member is an array
#       |->['dyn_array'] = bool indicating if member is a dynamically sized array
#       '->['array_size'] = For dyn_array, member name used to size array, else int size for static array
struct_dict = {}
struct_order_list = [] # struct names in order they're declared
# Store struct names that require #ifdef guards
#  dict stores struct and enum definitions that are guarded by ifdef as the key
#  and the txt of the ifdef is the value
ifdef_dict = {}
# typedef_fwd_dict stores mapping from orig_type_name -> new_type_name
typedef_fwd_dict = {}
# typedef_rev_dict stores mapping from new_type_name -> orig_type_name
typedef_rev_dict = {} # store new_name -> orig_name mapping
# types_dict['id_name'] = identifier name will map to it's type
#              '---->'type' = currently either 'struct' or 'enum'
types_dict = {}   # store orig_name -> type mapping


# Class that parses header file and generates data structures that can
#  Then be used for other tasks
class HeaderFileParser:
    def __init__(self, header_file=None):
        self.header_file = header_file
        # store header data in various formats, see above for more info
        self.enum_val_dict = {}
        self.enum_type_dict = {}
        self.struct_dict = {}
        self.typedef_fwd_dict = {}
        self.typedef_rev_dict = {}
        self.types_dict = {}
        self.last_struct_count_name = ''

    def setHeaderFile(self, header_file):
        self.header_file = header_file

    def get_enum_val_dict(self):
        return self.enum_val_dict

    def get_enum_type_dict(self):
        return self.enum_type_dict

    def get_struct_dict(self):
        return self.struct_dict

    def get_typedef_fwd_dict(self):
        return self.typedef_fwd_dict

    def get_typedef_rev_dict(self):
        return self.typedef_rev_dict

    def get_types_dict(self):
        return self.types_dict

    # Parse header file into data structures
    def parse(self):
        # parse through the file, identifying different sections
        parse_enum = False
        parse_struct = False
        member_num = 0
        # TODO : Comment parsing is very fragile but handles 2 known files
        block_comment = False
        prev_count_name = ''
        ifdef_txt = ''
        ifdef_active = 0
        exclude_struct_list = ['VkPlatformHandleXcbKHR', 'VkPlatformHandleX11KHR']
        with open(self.header_file) as f:
            for line in f:
                if True in [ifd_txt in line for ifd_txt in ['#ifdef ', '#ifndef ']]:
                    ifdef_txt = line.split()[1]
                    ifdef_active = ifdef_active + 1
                    continue
                if ifdef_active != 0 and '#endif' in line:
                    ifdef_active = ifdef_active - 1
                if block_comment:
                    if '*/' in line:
                        block_comment = False
                    continue
                if '/*' in line:
                    if '*/' in line: # single line block comment
                        continue
                    block_comment = True
                elif 0 == len(line.split()):
                    #print("Skipping empty line")
                    continue
                elif line.split()[0].strip().startswith("//"):
                    #print("Skipping commented line %s" % line)
                    continue
                elif 'typedef enum' in line:
                    (ty_txt, en_txt, base_type) = line.strip().split(None, 2)
                    #print("Found ENUM type %s" % base_type)
                    if '{' == base_type:
                        base_type = 'tmp_enum'
                    parse_enum = True
                    default_enum_val = 0
                    self.types_dict[base_type] = 'enum'
                elif 'typedef struct' in line or 'typedef union' in line:
                    if True in [ex_type in line for ex_type in exclude_struct_list]:
                        continue

                    (ty_txt, st_txt, base_type) = line.strip().split(None, 2)
                    if ' ' in base_type:
                        (ignored, base_type) = base_type.strip().split(None, 1)

                    #print("Found STRUCT type: %s" % base_type)
                    # Note:  This really needs to be updated to handle one line struct definition, like
                    #        typedef struct obj##_T { uint64_t handle; } obj;
                    if ('{' == base_type or not (' ' in base_type)):
                        base_type = 'tmp_struct'
                        parse_struct = True
                        self.types_dict[base_type] = 'struct'
#                elif 'typedef union' in line:
#                    (ty_txt, st_txt, base_type) = line.strip().split(None, 2)
#                    print("Found UNION type: %s" % base_type)
#                    parse_struct = True
#                    self.types_dict[base_type] = 'struct'
                elif '}' in line and (parse_enum or parse_struct):
                    if len(line.split()) > 1: # deals with embedded union in one struct
                        parse_enum = False
                        parse_struct = False
                        self.last_struct_count_name = ''
                        member_num = 0
                        (cur_char, targ_type) = line.strip().split(None, 1)
                        if 'tmp_struct' == base_type:
                            base_type = targ_type.strip(';')
                            if True in [ex_type in base_type for ex_type in exclude_struct_list]:
                                del self.struct_dict['tmp_struct']
                                continue
                            #print("Found Actual Struct type %s" % base_type)
                            self.struct_dict[base_type] = self.struct_dict['tmp_struct']
                            self.struct_dict.pop('tmp_struct', 0)
                            struct_order_list.append(base_type)
                            self.types_dict[base_type] = 'struct'
                            self.types_dict.pop('tmp_struct', 0)
                        elif 'tmp_enum' == base_type:
                            base_type = targ_type.strip(';')
                            #print("Found Actual ENUM type %s" % base_type)
                            for n in self.enum_val_dict:
                                if 'tmp_enum' == self.enum_val_dict[n]['type']:
                                    self.enum_val_dict[n]['type'] = base_type
#                            self.enum_val_dict[base_type] = self.enum_val_dict['tmp_enum']
#                            self.enum_val_dict.pop('tmp_enum', 0)
                            self.enum_type_dict[base_type] = self.enum_type_dict['tmp_enum']
                            self.enum_type_dict.pop('tmp_enum', 0)
                            self.types_dict[base_type] = 'enum'
                            self.types_dict.pop('tmp_enum', 0)
                        if ifdef_active:
                            ifdef_dict[base_type] = ifdef_txt
                        self.typedef_fwd_dict[base_type] = targ_type.strip(';')
                        self.typedef_rev_dict[targ_type.strip(';')] = base_type
                        #print("fwd_dict: %s = %s" % (base_type, targ_type))
                elif parse_enum:
                    #if 'VK_MAX_ENUM' not in line and '{' not in line:
                    if True not in [ens in line for ens in ['{', '_MAX_ENUM', '_BEGIN_RANGE', '_END_RANGE', '_NUM = ', '_ENUM_RANGE']]:
                        self._add_enum(line, base_type, default_enum_val)
                        default_enum_val += 1
                elif parse_struct:
                    if ';' in line:
                        self._add_struct(line, base_type, member_num)
                        member_num = member_num + 1

    # populate enum dicts based on enum lines
    def _add_enum(self, line_txt, enum_type, def_enum_val):
        #print("Parsing enum line %s" % line_txt)
        if '=' in line_txt:
            (enum_name, eq_char, enum_val) = line_txt.split(None, 2)
        else:
            enum_name = line_txt.split(',')[0]
            enum_val = str(def_enum_val)
        self.enum_val_dict[enum_name] = {}
        self.enum_val_dict[enum_name]['type'] = enum_type
        # strip comma and comment, then extra split in case of no comma w/ comments
        enum_val = enum_val.strip().split(',', 1)[0]
        self.enum_val_dict[enum_name]['val'] = enum_val.split()[0]
        # Perform conversion of VK_BIT macro
        if 'VK_BIT' in self.enum_val_dict[enum_name]['val']:
            vk_bit_val = self.enum_val_dict[enum_name]['val']
            bit_shift = int(vk_bit_val[vk_bit_val.find('(')+1:vk_bit_val.find(')')], 0)
            self.enum_val_dict[enum_name]['val'] = str(1 << bit_shift)
        else:
            # account for negative values surrounded by parens
            self.enum_val_dict[enum_name]['val'] = self.enum_val_dict[enum_name]['val'].strip(')').replace('-(', '-')
        # Try to cast to int to determine if enum value is unique
        try:
            #print("ENUM val:", self.enum_val_dict[enum_name]['val'])
            int(self.enum_val_dict[enum_name]['val'], 0)
            self.enum_val_dict[enum_name]['unique'] = True
            #print("ENUM has num value")
        except ValueError:
            self.enum_val_dict[enum_name]['unique'] = False
            #print("ENUM is not a number value")
        # Update enum_type_dict as well
        if not enum_type in self.enum_type_dict:
            self.enum_type_dict[enum_type] = []
        self.enum_type_dict[enum_type].append(enum_name)

    # Return True if struct member is a dynamic array
    # RULES : This is a bit quirky based on the API
    # NOTE : Changes in API spec may cause these rules to change
    #  1. There must be a previous uint var w/ 'count' in the name in the struct
    #  2. Dynam array must have 'const' and '*' qualifiers
    #  3a. Name of dynam array must end in 's' char OR
    #  3b. Name of count var minus 'count' must be contained in name of dynamic array
    def _is_dynamic_array(self, full_type, name):
        exceptions = ['pEnabledFeatures', 'pWaitDstStageMask', 'pSampleMask']
        if name in exceptions:
            return False
        if '' != self.last_struct_count_name:
            if 'const' in full_type and '*' in full_type:
                if name.endswith('s') or self.last_struct_count_name.lower().replace('count', '') in name.lower():
                    return True

                # VkWriteDescriptorSet
                if self.last_struct_count_name == "descriptorCount":
                    return True

        return False

    # populate struct dicts based on struct lines
    # TODO : Handle ":" bitfield, "**" ptr->ptr and "const type*const*"
    def _add_struct(self, line_txt, struct_type, num):
        #print("Parsing struct line %s" % line_txt)
        if '{' == struct_type:
            print("Parsing struct '{' w/ line %s" % line_txt)
        if not struct_type in self.struct_dict:
            self.struct_dict[struct_type] = {}
        members = line_txt.strip().split(';', 1)[0] # first strip semicolon & comments
        # TODO : Handle bitfields more correctly
        members = members.strip().split(':', 1)[0] # strip bitfield element
        (member_type, member_name) = members.rsplit(None, 1)
        # Store counts to help recognize and size dynamic arrays
        if 'count' in member_name.lower() and 'samplecount' != member_name.lower() and 'uint' in member_type:
            self.last_struct_count_name = member_name
        self.struct_dict[struct_type][num] = {}
        self.struct_dict[struct_type][num]['full_type'] = member_type
        self.struct_dict[struct_type][num]['dyn_array'] = False
        if '*' in member_type:
            self.struct_dict[struct_type][num]['ptr'] = True
            # TODO : Need more general purpose way here to reduce down to basic type
            member_type = member_type.replace(' const*', '')
            member_type = member_type.strip('*')
        else:
            self.struct_dict[struct_type][num]['ptr'] = False
        if 'const' in member_type:
            self.struct_dict[struct_type][num]['const'] = True
            member_type = member_type.replace('const', '').strip()
        else:
            self.struct_dict[struct_type][num]['const'] = False
        # TODO : There is a bug here where it seems that at the time we do this check,
        #    the data is not in the types or typedef_rev_dict, so we never pass this if check
        if is_type(member_type, 'struct'):
            self.struct_dict[struct_type][num]['struct'] = True
        else:
            self.struct_dict[struct_type][num]['struct'] = False
        self.struct_dict[struct_type][num]['type'] = member_type
        if '[' in member_name:
            (member_name, array_size) = member_name.split('[', 1)
            #if 'char' in member_type:
            #    self.struct_dict[struct_type][num]['array'] = False
            #    self.struct_dict[struct_type][num]['array_size'] = 0
            #    self.struct_dict[struct_type][num]['ptr'] = True
            #else:
            self.struct_dict[struct_type][num]['array'] = True
            self.struct_dict[struct_type][num]['array_size'] = array_size.strip(']')
        elif self._is_dynamic_array(self.struct_dict[struct_type][num]['full_type'], member_name):
            #print("Found dynamic array %s of size %s" % (member_name, self.last_struct_count_name))
            self.struct_dict[struct_type][num]['array'] = True
            self.struct_dict[struct_type][num]['dyn_array'] = True
            self.struct_dict[struct_type][num]['array_size'] = self.last_struct_count_name
        elif not 'array' in self.struct_dict[struct_type][num]:
            self.struct_dict[struct_type][num]['array'] = False
            self.struct_dict[struct_type][num]['array_size'] = 0
        self.struct_dict[struct_type][num]['name'] = member_name

# check if given identifier is of specified type_to_check
def is_type(identifier, type_to_check):
    if identifier in types_dict and type_to_check == types_dict[identifier]:
        return True
    if identifier in typedef_rev_dict:
        new_id = typedef_rev_dict[identifier]
        if new_id in types_dict and type_to_check == types_dict[new_id]:
            return True
    return False

# This is a validation function to verify that we can reproduce the original structs
def recreate_structs():
    for struct_name in struct_dict:
        sys.stdout.write("typedef struct %s\n{\n" % struct_name)
        for mem_num in sorted(struct_dict[struct_name]):
            sys.stdout.write("    ")
            if struct_dict[struct_name][mem_num]['const']:
                sys.stdout.write("const ")
            #if struct_dict[struct_name][mem_num]['struct']:
            #    sys.stdout.write("struct ")
            sys.stdout.write (struct_dict[struct_name][mem_num]['type'])
            if struct_dict[struct_name][mem_num]['ptr']:
                sys.stdout.write("*")
            sys.stdout.write(" ")
            sys.stdout.write(struct_dict[struct_name][mem_num]['name'])
            if struct_dict[struct_name][mem_num]['array']:
                sys.stdout.write("[")
                sys.stdout.write(struct_dict[struct_name][mem_num]['array_size'])
                sys.stdout.write("]")
            sys.stdout.write(";\n")
        sys.stdout.write("} ")
        sys.stdout.write(typedef_fwd_dict[struct_name])
        sys.stdout.write(";\n\n")

#
# TODO: Fix construction of struct name
def get_struct_name_from_struct_type(struct_type):
    # Note: All struct types are now camel-case
    # Debug Report has an inconsistency - so need special case.
    if ("VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT" == struct_type):
        return "VkDebugReportCallbackCreateInfoEXT"
    caps_struct_name = struct_type.replace("_STRUCTURE_TYPE", "")
    char_idx = 0
    struct_name = ''
    for char in caps_struct_name:
        if (0 == char_idx) or (caps_struct_name[char_idx-1] == '_'):
            struct_name += caps_struct_name[char_idx]
        elif (caps_struct_name[char_idx] == '_'):
            pass
        else:
            struct_name += caps_struct_name[char_idx].lower()
        char_idx += 1

    return struct_name

# Emit an ifdef if incoming func matches a platform identifier
def add_platform_wrapper_entry(list, func):
    if (re.match(r'.*Xlib.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_XLIB_KHR")
    if (re.match(r'.*Xcb.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_XCB_KHR")
    if (re.match(r'.*Wayland.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_WAYLAND_KHR")
    if (re.match(r'.*Mir.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_MIR_KHR")
    if (re.match(r'.*Android.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_ANDROID_KHR")
    if (re.match(r'.*Win32.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_WIN32_KHR")

# Emit an endif if incoming func matches a platform identifier
def add_platform_wrapper_exit(list, func):
    if (re.match(r'.*Xlib.*', func)):
        list.append("#endif //VK_USE_PLATFORM_XLIB_KHR")
    if (re.match(r'.*Xcb.*', func)):
        list.append("#endif //VK_USE_PLATFORM_XCB_KHR")
    if (re.match(r'.*Wayland.*', func)):
        list.append("#endif //VK_USE_PLATFORM_WAYLAND_KHR")
    if (re.match(r'.*Mir.*', func)):
        list.append("#endif //VK_USE_PLATFORM_MIR_KHR")
    if (re.match(r'.*Android.*', func)):
        list.append("#endif //VK_USE_PLATFORM_ANDROID_KHR")
    if (re.match(r'.*Win32.*', func)):
        list.append("#endif //VK_USE_PLATFORM_WIN32_KHR")

# class for writing common file elements
# Here's how this class lays out a file:
#  COPYRIGHT
#  HEADER
#  BODY
#  FOOTER
#
# For each of these sections, there's a "set*" function
# The class as a whole has a generate function which will write each section in order
class CommonFileGen:
    def __init__(self, filename=None, copyright_txt="", header_txt="", body_txt="", footer_txt=""):
        self.filename = filename
        self.contents = {'copyright': copyright_txt, 'header': header_txt, 'body': body_txt, 'footer': footer_txt}
        # TODO : Set a default copyright & footer at least

    def setFilename(self, filename):
        self.filename = filename

    def setCopyright(self, c):
        self.contents['copyright'] = c

    def setHeader(self, h):
        self.contents['header'] = h

    def setBody(self, b):
        self.contents['body'] = b

    def setFooter(self, f):
        self.contents['footer'] = f

    def generate(self):
        #print("Generate to file %s" % self.filename)
        with open(self.filename, "w") as f:
            f.write(self.contents['copyright'])
            f.write(self.contents['header'])
            f.write(self.contents['body'])
            f.write(self.contents['footer'])

# class for writing a wrapper class for structures
# The wrapper class wraps the structs and includes utility functions for
#  setting/getting member values and displaying the struct data in various formats
class StructWrapperGen:
    def __init__(self, in_struct_dict, prefix, out_dir):
        self.struct_dict = in_struct_dict
        self.include_headers = []
        self.lineinfo = sourcelineinfo()
        self.api = prefix
        if prefix.lower() == "vulkan":
            self.api_prefix = "vk"
        else:
            self.api_prefix = prefix
        self.header_filename = os.path.join(out_dir, self.api_prefix+"_struct_wrappers.h")
        self.class_filename = os.path.join(out_dir, self.api_prefix+"_struct_wrappers.cpp")
        self.safe_struct_header_filename = os.path.join(out_dir, self.api_prefix+"_safe_struct.h")
        self.safe_struct_source_filename = os.path.join(out_dir, self.api_prefix+"_safe_struct.cpp")
        self.string_helper_filename = os.path.join(out_dir, self.api_prefix+"_struct_string_helper.h")
        self.string_helper_no_addr_filename = os.path.join(out_dir, self.api_prefix+"_struct_string_helper_no_addr.h")
        self.string_helper_cpp_filename = os.path.join(out_dir, self.api_prefix+"_struct_string_helper_cpp.h")
        self.string_helper_no_addr_cpp_filename = os.path.join(out_dir, self.api_prefix+"_struct_string_helper_no_addr_cpp.h")
        self.validate_helper_filename = os.path.join(out_dir, self.api_prefix+"_struct_validate_helper.h")
        self.no_addr = False
        # Safe Struct (ss) header and source files
        self.ssh = CommonFileGen(self.safe_struct_header_filename)
        self.sss = CommonFileGen(self.safe_struct_source_filename)
        self.hfg = CommonFileGen(self.header_filename)
        self.cfg = CommonFileGen(self.class_filename)
        self.shg = CommonFileGen(self.string_helper_filename)
        self.shcppg = CommonFileGen(self.string_helper_cpp_filename)
        self.vhg = CommonFileGen(self.validate_helper_filename)
        self.size_helper_filename = os.path.join(out_dir, self.api_prefix+"_struct_size_helper.h")
        self.size_helper_c_filename = os.path.join(out_dir, self.api_prefix+"_struct_size_helper.c")
        self.size_helper_gen = CommonFileGen(self.size_helper_filename)
        self.size_helper_c_gen = CommonFileGen(self.size_helper_c_filename)
        #print(self.header_filename)
        self.header_txt = ""
        self.definition_txt = ""

    def set_include_headers(self, include_headers):
        self.include_headers = include_headers

    def set_no_addr(self, no_addr):
        self.no_addr = no_addr
        if self.no_addr:
            self.shg = CommonFileGen(self.string_helper_no_addr_filename)
            self.shcppg = CommonFileGen(self.string_helper_no_addr_cpp_filename)
        else:
            self.shg = CommonFileGen(self.string_helper_filename)
            self.shcppg = CommonFileGen(self.string_helper_cpp_filename)

    # Return class name for given struct name
    def get_class_name(self, struct_name):
        class_name = struct_name.strip('_').lower() + "_struct_wrapper"
        return class_name

    def get_file_list(self):
        return [os.path.basename(self.header_filename), os.path.basename(self.class_filename), os.path.basename(self.string_helper_filename)]

    # Generate class header file
    def generateHeader(self):
        self.hfg.setCopyright(self._generateCopyright())
        self.hfg.setHeader(self._generateHeader())
        self.hfg.setBody(self._generateClassDeclaration())
        self.hfg.setFooter(self._generateFooter())
        self.hfg.generate()

    # Generate class definition
    def generateBody(self):
        self.cfg.setCopyright(self._generateCopyright())
        self.cfg.setHeader(self._generateCppHeader())
        self.cfg.setBody(self._generateClassDefinition())
        self.cfg.setFooter(self._generateFooter())
        self.cfg.generate()

    # Safe Structs are versions of vulkan structs with non-const safe ptrs
    #  that make shadowing structures and clean-up of shadowed structures very simple
    def generateSafeStructHeader(self):
        self.ssh.setCopyright(self._generateCopyright())
        self.ssh.setHeader(self._generateSafeStructHeader())
        self.ssh.setBody(self._generateSafeStructDecls())
        self.ssh.generate()

    def generateSafeStructs(self):
        self.sss.setCopyright(self._generateCopyright())
        self.sss.setHeader(self._generateSafeStructSourceHeader())
        self.sss.setBody(self._generateSafeStructSource())
        self.sss.generate()

    # Generate c-style .h file that contains functions for printing structs
    def generateStringHelper(self):
        print("Generating struct string helper")
        self.shg.setCopyright(self._generateCopyright())
        self.shg.setHeader(self._generateStringHelperHeader())
        self.shg.setBody(self._generateStringHelperFunctions())
        self.shg.generate()

    # Generate cpp-style .h file that contains functions for printing structs
    def generateStringHelperCpp(self):
        print("Generating struct string helper cpp")
        self.shcppg.setCopyright(self._generateCopyright())
        self.shcppg.setHeader(self._generateStringHelperHeaderCpp())
        self.shcppg.setBody(self._generateStringHelperFunctionsCpp())
        self.shcppg.generate()

    # Generate c-style .h file that contains functions for printing structs
    def generateValidateHelper(self):
        print("Generating struct validate helper")
        self.vhg.setCopyright(self._generateCopyright())
        self.vhg.setHeader(self._generateValidateHelperHeader())
        self.vhg.setBody(self._generateValidateHelperFunctions())
        self.vhg.generate()

    def generateSizeHelper(self):
        print("Generating struct size helper")
        self.size_helper_gen.setCopyright(self._generateCopyright())
        self.size_helper_gen.setHeader(self._generateSizeHelperHeader())
        self.size_helper_gen.setBody(self._generateSizeHelperFunctions())
        self.size_helper_gen.setFooter(self._generateSizeHelperFooter())
        self.size_helper_gen.generate()

    def generateSizeHelperC(self):
        print("Generating struct size helper c")
        self.size_helper_c_gen.setCopyright(self._generateCopyright())
        self.size_helper_c_gen.setHeader(self._generateSizeHelperHeaderC())
        self.size_helper_c_gen.setBody(self._generateSizeHelperFunctionsC())
        self.size_helper_c_gen.generate()

    def _generateCopyright(self):
        copyright = []
        copyright.append('/* THIS FILE IS GENERATED.  DO NOT EDIT. */');
        copyright.append('');
        copyright.append('/*');
        copyright.append(' * Vulkan');
        copyright.append(' *');
        copyright.append(' * Copyright (c) 2015-2016 The Khronos Group Inc.');
        copyright.append(' * Copyright (c) 2015-2016 Valve Corporation.');
        copyright.append(' * Copyright (c) 2015-2016 LunarG, Inc.');
        copyright.append(' * Copyright (c) 2015-2016 Google Inc.');
        copyright.append(' *');
        copyright.append(' * Licensed under the Apache License, Version 2.0 (the "License");');
        copyright.append(' * you may not use this file except in compliance with the License.');
        copyright.append(' * You may obtain a copy of the License at');
        copyright.append(' *');
        copyright.append(' *     http://www.apache.org/licenses/LICENSE-2.0');
        copyright.append(' *');
        copyright.append(' * Unless required by applicable law or agreed to in writing, software');
        copyright.append(' * distributed under the License is distributed on an "AS IS" BASIS,');
        copyright.append(' * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.');
        copyright.append(' * See the License for the specific language governing permissions and');
        copyright.append(' * limitations under the License.');
        copyright.append(' *');
        copyright.append(' * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>');
        copyright.append(' * Author: Tobin Ehlis <tobin@lunarg.com>');
        copyright.append(' */');
        copyright.append('');
        return "\n".join(copyright)

    def _generateCppHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append("#include <stdio.h>\n#include <%s>\n#include <%s_enum_string_helper.h>\n" % (os.path.basename(self.header_filename), self.api_prefix))
        return "".join(header)

    def _generateClassDefinition(self):
        class_def = []
        if 'vk' == self.api:
            class_def.append(self._generateDynamicPrintFunctions())
        for s in sorted(self.struct_dict):
            class_def.append("\n// %s class definition" % self.get_class_name(s))
            class_def.append(self._generateConstructorDefinitions(s))
            class_def.append(self._generateDestructorDefinitions(s))
            class_def.append(self._generateDisplayDefinitions(s))
        return "\n".join(class_def)

    def _generateConstructorDefinitions(self, s):
        con_defs = []
        con_defs.append("%s::%s() : m_struct(), m_indent(0), m_dummy_prefix('\\0'), m_origStructAddr(NULL) {}" % (self.get_class_name(s), self.get_class_name(s)))
        # TODO : This is a shallow copy of ptrs
        con_defs.append("%s::%s(%s* pInStruct) : m_indent(0), m_dummy_prefix('\\0')\n{\n    m_struct = *pInStruct;\n    m_origStructAddr = pInStruct;\n}" % (self.get_class_name(s), self.get_class_name(s), typedef_fwd_dict[s]))
        con_defs.append("%s::%s(const %s* pInStruct) : m_indent(0), m_dummy_prefix('\\0')\n{\n    m_struct = *pInStruct;\n    m_origStructAddr = pInStruct;\n}" % (self.get_class_name(s), self.get_class_name(s), typedef_fwd_dict[s]))
        return "\n".join(con_defs)

    def _generateDestructorDefinitions(self, s):
        return "%s::~%s() {}" % (self.get_class_name(s), self.get_class_name(s))

    def _generateDynamicPrintFunctions(self):
        dp_funcs = []
        dp_funcs.append("\nvoid dynamic_display_full_txt(const void* pStruct, uint32_t indent)\n{\n    // Cast to APP_INFO ptr initially just to pull sType off struct")
        dp_funcs.append("    VkStructureType sType = ((VkApplicationInfo*)pStruct)->sType;\n")
        dp_funcs.append("    switch (sType)\n    {")
        for e in enum_type_dict:
            class_num = 0
            if "StructureType" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = get_struct_name_from_struct_type(v)
                    if struct_name not in self.struct_dict:
                        continue

                    class_name = self.get_class_name(struct_name)
                    instance_name = "swc%i" % class_num
                    dp_funcs.append("        case %s:\n        {" % (v))
                    dp_funcs.append("            %s %s((%s*)pStruct);" % (class_name, instance_name, struct_name))
                    dp_funcs.append("            %s.set_indent(indent);" % (instance_name))
                    dp_funcs.append("            %s.display_full_txt();" % (instance_name))
                    dp_funcs.append("        }")
                    dp_funcs.append("        break;")
                    class_num += 1
                dp_funcs.append("    }")
        dp_funcs.append("}\n")
        return "\n".join(dp_funcs)

    def _get_func_name(self, struct, mid_str):
        return "%s_%s_%s" % (self.api_prefix, mid_str, struct.lower().strip("_"))

    def _get_sh_func_name(self, struct):
        return self._get_func_name(struct, 'print')

    def _get_vh_func_name(self, struct):
        return self._get_func_name(struct, 'validate')

    def _get_size_helper_func_name(self, struct):
        return self._get_func_name(struct, 'size')

    # Return elements to create formatted string for given struct member
    def _get_struct_print_formatted(self, struct_member, pre_var_name="prefix", postfix = "\\n", struct_var_name="pStruct", struct_ptr=True, print_array=False):
        struct_op = "->"
        if not struct_ptr:
            struct_op = "."
        member_name = struct_member['name']
        print_type = "p"
        cast_type = ""
        member_post = ""
        array_index = ""
        member_print_post = ""
        print_delimiter = "%"
        if struct_member['array'] and 'char' in struct_member['type'].lower(): # just print char array as string
            if member_name.startswith('pp'): # TODO : Only printing first element of dynam array of char* for now
                member_post = "[0]"
            print_type = "s"
            print_array = False
        elif struct_member['array'] and not print_array:
            # Just print base address of array when not full print_array
            print_delimiter = "0x%"
            cast_type = "(void*)"
        elif is_type(struct_member['type'], 'enum'):
            cast_type = "string_%s" % struct_member['type']
            if struct_member['ptr']:
                struct_var_name = "*" + struct_var_name
                print_delimiter = "0x%"
            print_type = "s"
        elif is_type(struct_member['type'], 'struct'): # print struct address for now
            print_delimiter = "0x%"
            cast_type = "(void*)"
            if not struct_member['ptr']:
                cast_type = "(void*)&"
        elif 'bool' in struct_member['type'].lower():
            print_type = "s"
            member_post = ' ? "TRUE" : "FALSE"'
        elif 'float' in struct_member['type']:
            print_type = "f"
        elif 'uint64' in struct_member['type'] or 'gpusize' in struct_member['type'].lower():
            print_type = '" PRId64 "'
        elif 'uint8' in struct_member['type']:
            print_type = "hu"
        elif 'size' in struct_member['type'].lower():
            print_type = '" PRINTF_SIZE_T_SPECIFIER "'
            print_delimiter = ""
        elif True in [ui_str.lower() in struct_member['type'].lower() for ui_str in ['uint', 'flags', 'samplemask']]:
            print_type = "u"
        elif 'int' in struct_member['type']:
            print_type = "i"
        elif struct_member['ptr']:
            print_delimiter = "0x%"
            pass
        else:
            #print("Unhandled struct type: %s" % struct_member['type'])
            print_delimiter = "0x%"
            cast_type = "(void*)"
        if print_array and struct_member['array']:
            member_print_post = "[%u]"
            array_index = " i,"
            member_post = "[i]"
        print_out = "%%s%s%s = %s%s%s" % (member_name, member_print_post, print_delimiter, print_type, postfix) # section of print that goes inside of quotes
        print_arg = ", %s,%s %s(%s%s%s)%s" % (pre_var_name, array_index, cast_type, struct_var_name, struct_op, member_name, member_post) # section of print passed to portion in quotes
        if self.no_addr and "p" == print_type:
            print_out = "%%s%s%s = addr\\n" % (member_name, member_print_post) # section of print that goes inside of quotes
            print_arg = ", %s" % (pre_var_name)
        return (print_out, print_arg)

    def _generateStringHelperFunctions(self):
        sh_funcs = []
        # We do two passes, first pass just generates prototypes for all the functsions
        for s in sorted(self.struct_dict):
            sh_funcs.append('char* %s(const %s* pStruct, const char* prefix);' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
        sh_funcs.append('')
        sh_funcs.append('#if defined(_WIN32)')
        sh_funcs.append('// Microsoft did not implement C99 in Visual Studio; but started adding it with')
        sh_funcs.append('// VS2013.  However, VS2013 still did not have snprintf().  The following is a')
        sh_funcs.append('// work-around.')
        sh_funcs.append('#define snprintf _snprintf')
        sh_funcs.append('#endif // _WIN32\n')
        for s in sorted(self.struct_dict):
            p_out = ""
            p_args = ""
            stp_list = [] # stp == "struct to print" a list of structs for this API call that should be printed as structs
            # This pre-pass flags embedded structs and pNext
            for m in sorted(self.struct_dict[s]):
                if 'pNext' == self.struct_dict[s][m]['name'] or is_type(self.struct_dict[s][m]['type'], 'struct'):
                    stp_list.append(self.struct_dict[s][m])
            sh_funcs.append('char* %s(const %s* pStruct, const char* prefix)\n{\n    char* str;' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
            sh_funcs.append("    size_t len;")
            num_stps = len(stp_list);
            total_strlen_str = ''
            if 0 != num_stps:
                sh_funcs.append("    char* tmpStr;")
                sh_funcs.append('    char* extra_indent = (char*)malloc(strlen(prefix) + 3);')
                sh_funcs.append('    strcpy(extra_indent, "  ");')
                sh_funcs.append('    strncat(extra_indent, prefix, strlen(prefix));')
                sh_funcs.append('    char* stp_strs[%i];' % num_stps)
                for index in range(num_stps):
                    # If it's an array, print all of the elements
                    # If it's a ptr, print thing it's pointing to
                    # Non-ptr struct case. Print the struct using its address
                    struct_deref = '&'
                    if 1 < stp_list[index]['full_type'].count('*'):
                        struct_deref = ''
                    if (stp_list[index]['ptr']):
                        sh_funcs.append('    if (pStruct->%s) {' % stp_list[index]['name'])
                        if 'pNext' == stp_list[index]['name']:
                            sh_funcs.append('        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);')
                            sh_funcs.append('        len = 256+strlen(tmpStr);')
                            sh_funcs.append('        stp_strs[%i] = (char*)malloc(len);' % index)
                            if self.no_addr:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, " %%spNext (addr)\\n%%s", prefix, tmpStr);' % index)
                            else:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, " %%spNext (0x%%p)\\n%%s", prefix, (void*)pStruct->pNext, tmpStr);' % index)
                            sh_funcs.append('        free(tmpStr);')
                        else:
                            if stp_list[index]['name'] in ['pImageViews', 'pBufferViews']:
                                # TODO : This is a quick hack to handle these arrays of ptrs
                                sh_funcs.append('        tmpStr = %s(&pStruct->%s[0], extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            else:
                                sh_funcs.append('        tmpStr = %s(pStruct->%s, extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            sh_funcs.append('        len = 256+strlen(tmpStr)+strlen(prefix);')
                            sh_funcs.append('        stp_strs[%i] = (char*)malloc(len);' % (index))
                            if self.no_addr:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, " %%s%s (addr)\\n%%s", prefix, tmpStr);' % (index, stp_list[index]['name']))
                            else:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, " %%s%s (0x%%p)\\n%%s", prefix, (void*)pStruct->%s, tmpStr);' % (index, stp_list[index]['name'], stp_list[index]['name']))
                        sh_funcs.append('    }')
                        sh_funcs.append("    else\n        stp_strs[%i] = \"\";" % (index))
                    elif stp_list[index]['array']:
                        sh_funcs.append('    tmpStr = %s(&pStruct->%s[0], extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        sh_funcs.append('    len = 256+strlen(tmpStr);')
                        sh_funcs.append('    stp_strs[%i] = (char*)malloc(len);' % (index))
                        if self.no_addr:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s[0] (addr)\\n%%s", prefix, tmpStr);' % (index, stp_list[index]['name']))
                        else:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s[0] (0x%%p)\\n%%s", prefix, (void*)&pStruct->%s[0], tmpStr);' % (index, stp_list[index]['name'], stp_list[index]['name']))
                    else:
                        sh_funcs.append('    tmpStr = %s(&pStruct->%s, extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        sh_funcs.append('    len = 256+strlen(tmpStr);')
                        sh_funcs.append('    stp_strs[%i] = (char*)malloc(len);' % (index))
                        if self.no_addr:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s (addr)\\n%%s", prefix, tmpStr);' % (index, stp_list[index]['name']))
                        else:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s (0x%%p)\\n%%s", prefix, (void*)&pStruct->%s, tmpStr);' % (index, stp_list[index]['name'], stp_list[index]['name']))
                    total_strlen_str += 'strlen(stp_strs[%i]) + ' % index
            sh_funcs.append('    len = %ssizeof(char)*1024;' % (total_strlen_str))
            sh_funcs.append('    str = (char*)malloc(len);')
            sh_funcs.append('    snprintf(str, len, "')
            for m in sorted(self.struct_dict[s]):
                (p_out1, p_args1) = self._get_struct_print_formatted(self.struct_dict[s][m])
                p_out += p_out1
                p_args += p_args1
            p_out += '"'
            p_args += ");"
            sh_funcs[-1] = '%s%s%s' % (sh_funcs[-1], p_out, p_args)
            if 0 != num_stps:
                sh_funcs.append('    for (int32_t stp_index = %i; stp_index >= 0; stp_index--) {' % (num_stps-1))
                sh_funcs.append('        if (0 < strlen(stp_strs[stp_index])) {')
                sh_funcs.append('            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));')
                sh_funcs.append('            free(stp_strs[stp_index]);')
                sh_funcs.append('        }')
                sh_funcs.append('    }')
                sh_funcs.append('    free(extra_indent);')
            sh_funcs.append("    return str;\n}")
        # Add function to dynamically print out unknown struct
        sh_funcs.append("char* dynamic_display(const void* pStruct, const char* prefix)\n{")
        sh_funcs.append("    // Cast to APP_INFO ptr initially just to pull sType off struct")
        sh_funcs.append("    if (pStruct == NULL) {")
        sh_funcs.append("        return NULL;")
        sh_funcs.append("    }")
        sh_funcs.append("    VkStructureType sType = ((VkApplicationInfo*)pStruct)->sType;")
        sh_funcs.append('    char indent[100];\n    strcpy(indent, "    ");\n    strcat(indent, prefix);')
        sh_funcs.append("    switch (sType)\n    {")
        for e in enum_type_dict:
            if "StructureType" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = get_struct_name_from_struct_type(v)
                    if struct_name not in self.struct_dict:
                        continue
                    print_func_name = self._get_sh_func_name(struct_name)
                    sh_funcs.append('        case %s:\n        {' % (v))
                    sh_funcs.append('            return %s((%s*)pStruct, indent);' % (print_func_name, struct_name))
                    sh_funcs.append('        }')
                    sh_funcs.append('        break;')
                sh_funcs.append("        default:")
                sh_funcs.append("        return NULL;")
                sh_funcs.append("    }")
        sh_funcs.append("}")
        return "\n".join(sh_funcs)

    def _generateStringHelperFunctionsCpp(self):
        # declare str & tmp str
        # declare array of stringstreams for every struct ptr in current struct
        # declare array of stringstreams for every non-string element in current struct
        # For every struct ptr, if non-Null, then set its string, else set to NULL str
        # For every non-string element, set its string stream
        # create and return final string
        sh_funcs = []
        # First generate prototypes for every struct
        # XXX - REMOVE this comment
        lineinfo = sourcelineinfo()
        sh_funcs.append('%s' % lineinfo.get())
        for s in sorted(self.struct_dict):
            # Wrap this in platform check since it may contain undefined structs or functions
            add_platform_wrapper_entry(sh_funcs, typedef_fwd_dict[s])
            sh_funcs.append('std::string %s(const %s* pStruct, const std::string prefix);' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
            add_platform_wrapper_exit(sh_funcs, typedef_fwd_dict[s])

        sh_funcs.append('\n')
        sh_funcs.append('%s' % lineinfo.get())
        for s in sorted(self.struct_dict):
            num_non_enum_elems = [(is_type(self.struct_dict[s][elem]['type'], 'enum') and not self.struct_dict[s][elem]['ptr']) for elem in self.struct_dict[s]].count(False)
            stp_list = [] # stp == "struct to print" a list of structs for this API call that should be printed as structs
            # This pre-pass flags embedded structs and pNext
            for m in sorted(self.struct_dict[s]):
                if 'pNext' == self.struct_dict[s][m]['name'] or is_type(self.struct_dict[s][m]['type'], 'struct') or self.struct_dict[s][m]['array']:
                    # TODO: This is a tmp workaround
                    if 'ppActiveLayerNames' not in self.struct_dict[s][m]['name']:
                        stp_list.append(self.struct_dict[s][m])
            sh_funcs.append('%s' % lineinfo.get())

            # Wrap this in platform check since it may contain undefined structs or functions
            add_platform_wrapper_entry(sh_funcs, typedef_fwd_dict[s])

            sh_funcs.append('std::string %s(const %s* pStruct, const std::string prefix)\n{' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
            sh_funcs.append('%s' % lineinfo.get())
            indent = '    '
            sh_funcs.append('%susing namespace StreamControl;' % (indent))
            sh_funcs.append('%susing namespace std;' % (indent))
            sh_funcs.append('%sstring final_str;' % (indent))
            sh_funcs.append('%sstring tmp_str;' % (indent))
            sh_funcs.append('%sstring extra_indent = "  " + prefix;' % (indent))
            if (0 != num_non_enum_elems):
                sh_funcs.append('%sstringstream ss[%u];' % (indent, num_non_enum_elems))
            num_stps = len(stp_list)
            # First generate code for any embedded structs or arrays
            if 0 < num_stps:
                sh_funcs.append('%sstring stp_strs[%u];' % (indent, num_stps))
                idx_ss_decl = False # Make sure to only decl this once
                for index in range(num_stps):
                    addr_char = '&'
                    if 1 < stp_list[index]['full_type'].count('*'):
                        addr_char = ''
                    if stp_list[index]['array']:
                        sh_funcs.append('%s' % lineinfo.get())
                        if stp_list[index]['dyn_array']:
                            sh_funcs.append('%s' % lineinfo.get())
                            array_count = 'pStruct->%s' % (stp_list[index]['array_size'])
                        else:
                            sh_funcs.append('%s' % lineinfo.get())
                            array_count = '%s' % (stp_list[index]['array_size'])
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('%sstp_strs[%u] = "";' % (indent, index))
                        if not idx_ss_decl:
                            sh_funcs.append('%sstringstream index_ss;' % (indent))
                            idx_ss_decl = True
                        if (stp_list[index]['name'] == 'pQueueFamilyIndices'):
                            if (typedef_fwd_dict[s] == 'VkSwapchainCreateInfoKHR'):
                                sh_funcs.append('%sif (pStruct->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {' % (indent))
                            else:
                                sh_funcs.append('%sif (pStruct->sharingMode == VK_SHARING_MODE_CONCURRENT) {' % (indent))
                            indent += '    '
                        if (stp_list[index]['name'] == 'pImageInfo'):
                            sh_funcs.append('%sif ((pStruct->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)                ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)          ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE))           {' % (indent))
                            indent += '    '
                        elif (stp_list[index]['name'] == 'pBufferInfo'):
                            sh_funcs.append('%sif ((pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)         ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)         ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC))  {' % (indent))
                            indent += '    '
                        elif (stp_list[index]['name'] == 'pTexelBufferView'):
                            sh_funcs.append('%sif ((pStruct->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||' % (indent))
                            sh_funcs.append('%s    (pStruct->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER))  {' % (indent))
                            indent += '    '
                        if stp_list[index]['dyn_array']:
                            sh_funcs.append('%sif (pStruct->%s) {' % (indent, stp_list[index]['name']))
                            indent += '    '
                        sh_funcs.append('%sfor (uint32_t i = 0; i < %s; i++) {' % (indent, array_count))
                        indent += '    '
                        sh_funcs.append('%sindex_ss.str("");' % (indent))
                        sh_funcs.append('%sindex_ss << i;' % (indent))
                        if is_type(stp_list[index]['type'], 'enum'):
                            sh_funcs.append('%s' % lineinfo.get())
                            addr_char = ''
                            #value_print = 'string_%s(%spStruct->%s)' % (self.struct_dict[s][m]['type'], deref, self.struct_dict[s][m]['name'])
                            sh_funcs.append('%sss[%u] << string_%s(pStruct->%s[i]);' % (indent, index, stp_list[index]['type'], stp_list[index]['name']))
                            sh_funcs.append('%sstp_strs[%u] += " " + prefix + "%s[" + index_ss.str() + "] = " + ss[%u].str() + "\\n";' % (indent, index, stp_list[index]['name'], index))
                        elif is_type(stp_list[index]['type'], 'struct'):
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('%sss[%u] << "0x" << %spStruct->%s[i];' % (indent, index, addr_char, stp_list[index]['name']))
                            sh_funcs.append('%stmp_str = %s(%spStruct->%s[i], extra_indent);' % (indent, self._get_sh_func_name(stp_list[index]['type']), addr_char, stp_list[index]['name']))
                            if self.no_addr:
                                sh_funcs.append('%s' % lineinfo.get())
                                sh_funcs.append('%sstp_strs[%u] += " " + prefix + "%s[" + index_ss.str() + "] (addr)\\n" + tmp_str;' % (indent, index, stp_list[index]['name']))
                            else:
                                sh_funcs.append('%s' % lineinfo.get())
                                sh_funcs.append('%sstp_strs[%u] += " " + prefix + "%s[" + index_ss.str() + "] (" + ss[%u].str() + ")\\n" + tmp_str;' % (indent, index, stp_list[index]['name'], index))
                        else:
                            sh_funcs.append('%s' % lineinfo.get())
                            addr_char = ''
                            if stp_list[index]['ptr'] or 'UUID' in stp_list[index]['name']:
                                sh_funcs.append('%sss[%u] << "0x" << %spStruct->%s[i];' % (indent, index, addr_char, stp_list[index]['name']))
                            else:
                                sh_funcs.append('%sss[%u] << %spStruct->%s[i];' % (indent, index, addr_char, stp_list[index]['name']))
                            if stp_list[index]['type'] in vulkan.core.objects:
                                sh_funcs.append('%sstp_strs[%u] += " " + prefix + "%s[" + index_ss.str() + "].handle = " + ss[%u].str() + "\\n";' % (indent, index, stp_list[index]['name'], index))
                            else:
                                sh_funcs.append('%sstp_strs[%u] += " " + prefix + "%s[" + index_ss.str() + "] = " + ss[%u].str() + "\\n";' % (indent, index, stp_list[index]['name'], index))
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('%sss[%u].str("");' % (indent, index))
                        indent = indent[4:]
                        sh_funcs.append('%s}' % (indent))
                        if stp_list[index]['dyn_array']:
                            indent = indent[4:]
                            sh_funcs.append('%s}' % (indent))
                        #endif
                        if (stp_list[index]['name'] == 'pQueueFamilyIndices') or (stp_list[index]['name'] == 'pImageInfo') or (stp_list[index]['name'] == 'pBufferInfo') or (stp_list[index]['name'] == 'pTexelBufferView'):
                            indent = indent[4:]
                            sh_funcs.append('%s}' % (indent))
                    elif (stp_list[index]['ptr']):
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('%sif (pStruct->%s) {' % (indent, stp_list[index]['name']))
                        indent += '    '
                        if 'pNext' == stp_list[index]['name']:
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);')
                        else:
                            if stp_list[index]['name'] in ['pImageViews', 'pBufferViews']:
                                # TODO : This is a quick hack to handle these arrays of ptrs
                                sh_funcs.append('%s' % lineinfo.get())
                                sh_funcs.append('        tmp_str = %s(&pStruct->%s[0], extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            else:
                                sh_funcs.append('%s' % lineinfo.get())
                                sh_funcs.append('        tmp_str = %s(pStruct->%s, extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        sh_funcs.append('        ss[%u] << "0x" << %spStruct->%s;' % (index, addr_char, stp_list[index]['name']))
                        if self.no_addr:
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('        stp_strs[%u] = " " + prefix + "%s (addr)\\n" + tmp_str;' % (index, stp_list[index]['name']))
                        else:
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('        stp_strs[%u] = " " + prefix + "%s (" + ss[%u].str() + ")\\n" + tmp_str;' % (index, stp_list[index]['name'], index))
                        sh_funcs.append('        ss[%u].str("");' % (index))
                        sh_funcs.append('    }')
                        sh_funcs.append('    else')
                        sh_funcs.append('        stp_strs[%u] = "";' % index)
                    else:
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    tmp_str = %s(&pStruct->%s, extra_indent);' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        sh_funcs.append('    ss[%u] << "0x" << %spStruct->%s;' % (index, addr_char, stp_list[index]['name']))
                        if self.no_addr:
                            sh_funcs.append('    stp_strs[%u] = " " + prefix + "%s (addr)\\n" + tmp_str;' % (index, stp_list[index]['name']))
                            sh_funcs.append('%s' % lineinfo.get())
                        else:
                            sh_funcs.append('    stp_strs[%u] = " " + prefix + "%s (" + ss[%u].str() + ")\\n" + tmp_str;' % (index, stp_list[index]['name'], index))
                            sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    ss[%u].str("");' % index)
            # Now print one-line info for all data members
            index = 0
            final_str = []
            for m in sorted(self.struct_dict[s]):
                if not is_type(self.struct_dict[s][m]['type'], 'enum'):
                    if is_type(self.struct_dict[s][m]['type'], 'struct') and not self.struct_dict[s][m]['ptr']:
                        if self.no_addr:
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('    ss[%u].str("addr");' % (index))
                        else:
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('    ss[%u] << "0x" << &pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                    elif self.struct_dict[s][m]['array']:
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    ss[%u] << "0x" << (void*)pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                    elif 'bool' in self.struct_dict[s][m]['type'].lower():
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    ss[%u].str(pStruct->%s ? "TRUE" : "FALSE");' % (index, self.struct_dict[s][m]['name']))
                    elif 'uint8' in self.struct_dict[s][m]['type'].lower():
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    ss[%u] << pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                    elif 'void' in self.struct_dict[s][m]['type'].lower() and self.struct_dict[s][m]['ptr']:
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    if (StreamControl::writeAddress)')
                        sh_funcs.append('        ss[%u] << "0x" << pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                        sh_funcs.append('    else')
                        sh_funcs.append('        ss[%u].str("address");' % (index))
                    elif 'char' in self.struct_dict[s][m]['type'].lower() and self.struct_dict[s][m]['ptr']:
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    if (pStruct->%s != NULL) {' % self.struct_dict[s][m]['name'])
                        sh_funcs.append('        ss[%u] << pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                        sh_funcs.append('     } else {')
                        sh_funcs.append('        ss[%u] << "";' % index)
                        sh_funcs.append('     }')
                    else:
                        if self.struct_dict[s][m]['ptr'] or \
                           'Vk' in self.struct_dict[s][m]['full_type'] or \
                           'PFN_vk' in self.struct_dict[s][m]['full_type']:
                            sh_funcs.append('%s' % lineinfo.get())
                            sh_funcs.append('    ss[%u] << "0x" << pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                        elif any (x in self.struct_dict[s][m]['name'].lower() for x in ("flag", "bit", "offset", "handle", "buffer", "object", "mask")) or \
                             'ID' in self.struct_dict[s][m]['name']:
                            sh_funcs.append('%s: NB: Edit here to choose hex vs dec output by variable name' % lineinfo.get())
                            sh_funcs.append('    ss[%u] << "0x" << pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                        else:
                            sh_funcs.append('%s: NB Edit this section to choose hex vs dec output by variable name' % lineinfo.get())
                            sh_funcs.append('    ss[%u] << pStruct->%s;' % (index, self.struct_dict[s][m]['name']))
                    value_print = 'ss[%u].str()' % index
                    index += 1
                else:
                    # For an non-empty array of enums just print address w/ note that array will be displayed below
                    if self.struct_dict[s][m]['ptr']:
                        sh_funcs.append('%s' % lineinfo.get())
                        sh_funcs.append('    if (pStruct->%s)' % (self.struct_dict[s][m]['name']))
                        sh_funcs.append('        ss[%u] << "0x" << pStruct->%s << " (See individual array values below)";' % (index, self.struct_dict[s][m]['name']))
                        sh_funcs.append('    else')
                        sh_funcs.append('        ss[%u].str("NULL");' % (index))
                        value_print = 'ss[%u].str()' % index
                        index += 1
                    # For single enum just print the string representation
                    else:
                        value_print = 'string_%s(pStruct->%s)' % (self.struct_dict[s][m]['type'], self.struct_dict[s][m]['name'])
                final_str.append('+ prefix + "%s = " + %s + "\\n"' % (self.struct_dict[s][m]['name'], value_print))
            if 0 != num_stps: # Append data for any embedded structs
                final_str.append("+ %s" % " + ".join(['stp_strs[%u]' % n for n in reversed(range(num_stps))]))
            sh_funcs.append('%s' % lineinfo.get())
            for final_str_part in final_str:
                sh_funcs.append('    final_str = final_str %s;' % final_str_part)
            sh_funcs.append('    return final_str;\n}')

            # End of platform wrapped section
            add_platform_wrapper_exit(sh_funcs, typedef_fwd_dict[s])

        # Add function to return a string value for input void*
        sh_funcs.append('%s' % lineinfo.get())
        sh_funcs.append("std::string string_convert_helper(const void* toString, const std::string prefix)\n{")
        sh_funcs.append("    using namespace StreamControl;")
        sh_funcs.append("    using namespace std;")
        sh_funcs.append("    stringstream ss;")
        sh_funcs.append('    ss << toString;')
        sh_funcs.append('    string final_str = prefix + ss.str();')
        sh_funcs.append("    return final_str;")
        sh_funcs.append("}")
        sh_funcs.append('%s' % lineinfo.get())
        # Add function to return a string value for input uint64_t
        sh_funcs.append("std::string string_convert_helper(const uint64_t toString, const std::string prefix)\n{")
        sh_funcs.append("    using namespace StreamControl;")
        sh_funcs.append("    using namespace std;")
        sh_funcs.append("    stringstream ss;")
        sh_funcs.append('    ss << toString;')
        sh_funcs.append('    string final_str = prefix + ss.str();')
        sh_funcs.append("    return final_str;")
        sh_funcs.append("}")
        sh_funcs.append('%s' % lineinfo.get())
        # Add function to return a string value for input VkSurfaceFormatKHR*
        sh_funcs.append("std::string string_convert_helper(VkSurfaceFormatKHR toString, const std::string prefix)\n{")
        sh_funcs.append("    using namespace std;")
        sh_funcs.append('    string final_str = prefix + "format = " + string_VkFormat(toString.format) + "format = " + string_VkColorSpaceKHR(toString.colorSpace);')
        sh_funcs.append("    return final_str;")
        sh_funcs.append("}")
        sh_funcs.append('%s' % lineinfo.get())
        # Add function to dynamically print out unknown struct
        sh_funcs.append("std::string dynamic_display(const void* pStruct, const std::string prefix)\n{")
        sh_funcs.append("    using namespace std;")
        sh_funcs.append("    // Cast to APP_INFO ptr initially just to pull sType off struct")
        sh_funcs.append("    if (pStruct == NULL) {\n")
        sh_funcs.append("        return string();")
        sh_funcs.append("    }\n")
        sh_funcs.append("    VkStructureType sType = ((VkApplicationInfo*)pStruct)->sType;")
        sh_funcs.append('    string indent = "    ";')
        sh_funcs.append('    indent += prefix;')
        sh_funcs.append("    switch (sType)\n    {")
        for e in enum_type_dict:
            if "StructureType" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = get_struct_name_from_struct_type(v)
                    if struct_name not in self.struct_dict:
                        continue
                    print_func_name = self._get_sh_func_name(struct_name)
                    #sh_funcs.append('string %s(const %s* pStruct, const string prefix);' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
                    sh_funcs.append('        case %s:\n        {' % (v))
                    sh_funcs.append('            return %s((%s*)pStruct, indent);' % (print_func_name, struct_name))
                    sh_funcs.append('        }')
                    sh_funcs.append('        break;')
                sh_funcs.append("        default:")
                sh_funcs.append("        return string();")
        sh_funcs.append('%s' % lineinfo.get())
        sh_funcs.append("    }")
        sh_funcs.append("}")
        return "\n".join(sh_funcs)

    def _genStructMemberPrint(self, member, s, array, struct_array):
        (p_out, p_arg) = self._get_struct_print_formatted(self.struct_dict[s][member], pre_var_name="&m_dummy_prefix", struct_var_name="m_struct", struct_ptr=False, print_array=True)
        extra_indent = ""
        if array:
            extra_indent = "    "
        if is_type(self.struct_dict[s][member]['type'], 'struct'): # print struct address for now
            struct_array.insert(0, self.struct_dict[s][member])
        elif self.struct_dict[s][member]['ptr']:
            # Special case for void* named "pNext"
            if "void" in self.struct_dict[s][member]['type'] and "pNext" == self.struct_dict[s][member]['name']:
                struct_array.insert(0, self.struct_dict[s][member])
        return ('    %sprintf("%%*s    %s", m_indent, ""%s);' % (extra_indent, p_out, p_arg), struct_array)

    def _generateDisplayDefinitions(self, s):
        disp_def = []
        struct_array = []
        # Single-line struct print function
        disp_def.append("// Output 'structname = struct_address' on a single line")
        disp_def.append("void %s::display_single_txt()\n{" % self.get_class_name(s))
        disp_def.append('    printf(" %%*s%s = 0x%%p", m_indent, "", (void*)m_origStructAddr);' % typedef_fwd_dict[s])
        disp_def.append("}\n")
        # Private helper function to print struct members
        disp_def.append("// Private helper function that displays the members of the wrapped struct")
        disp_def.append("void %s::display_struct_members()\n{" % self.get_class_name(s))
        i_declared = False
        for member in sorted(self.struct_dict[s]):
            # TODO : Need to display each member based on its type
            # TODO : Need to handle pNext which are structs, but of void* type
            #   Can grab struct type off of header of struct pointed to
            # TODO : Handle Arrays
            if self.struct_dict[s][member]['array']:
                # Create for loop to print each element of array
                if not i_declared:
                    disp_def.append('    uint32_t i;')
                    i_declared = True
                disp_def.append('    for (i = 0; i<%s; i++) {' % self.struct_dict[s][member]['array_size'])
                (return_str, struct_array) = self._genStructMemberPrint(member, s, True, struct_array)
                disp_def.append(return_str)
                disp_def.append('    }')
            else:
                (return_str, struct_array) = self._genStructMemberPrint(member, s, False, struct_array)
                disp_def.append(return_str)
        disp_def.append("}\n")
        i_declared = False
        # Basic print function to display struct members
        disp_def.append("// Output all struct elements, each on their own line")
        disp_def.append("void %s::display_txt()\n{" % self.get_class_name(s))
        disp_def.append('    printf("%%*s%s struct contents at 0x%%p:\\n", m_indent, "", (void*)m_origStructAddr);' % typedef_fwd_dict[s])
        disp_def.append('    this->display_struct_members();')
        disp_def.append("}\n")
        # Advanced print function to display current struct and contents of any pointed-to structs
        disp_def.append("// Output all struct elements, and for any structs pointed to, print complete contents")
        disp_def.append("void %s::display_full_txt()\n{" % self.get_class_name(s))
        disp_def.append('    printf("%%*s%s struct contents at 0x%%p:\\n", m_indent, "", (void*)m_origStructAddr);' % typedef_fwd_dict[s])
        disp_def.append('    this->display_struct_members();')
        class_num = 0
        # TODO : Need to handle arrays of structs here
        for ms in struct_array:
            swc_name = "class%s" % str(class_num)
            if ms['array']:
                if not i_declared:
                    disp_def.append('    uint32_t i;')
                    i_declared = True
                disp_def.append('    for (i = 0; i<%s; i++) {' % ms['array_size'])
                #disp_def.append("        if (m_struct.%s[i]) {" % (ms['name']))
                disp_def.append("            %s %s(&(m_struct.%s[i]));" % (self.get_class_name(ms['type']), swc_name, ms['name']))
                disp_def.append("            %s.set_indent(m_indent + 4);" % (swc_name))
                disp_def.append("            %s.display_full_txt();" % (swc_name))
                #disp_def.append('        }')
                disp_def.append('    }')
            elif 'pNext' == ms['name']:
                # Need some code trickery here
                #  I'm thinking have a generated function that takes pNext ptr value
                #  then it checks sType and in large switch statement creates appropriate
                #  wrapper class type and then prints contents
                disp_def.append("    if (m_struct.%s) {" % (ms['name']))
                #disp_def.append('        printf("%*s    This is where we would call dynamic print function\\n", m_indent, "");')
                disp_def.append('        dynamic_display_full_txt(m_struct.%s, m_indent);' % (ms['name']))
                disp_def.append("    }")
            else:
                if ms['ptr']:
                    disp_def.append("    if (m_struct.%s) {" % (ms['name']))
                    disp_def.append("        %s %s(m_struct.%s);" % (self.get_class_name(ms['type']), swc_name, ms['name']))
                else:
                    disp_def.append("    if (&m_struct.%s) {" % (ms['name']))
                    disp_def.append("        %s %s(&m_struct.%s);" % (self.get_class_name(ms['type']), swc_name, ms['name']))
                disp_def.append("        %s.set_indent(m_indent + 4);" % (swc_name))
                disp_def.append("        %s.display_full_txt();\n    }" % (swc_name))
            class_num += 1
        disp_def.append("}\n")
        return "\n".join(disp_def)

    def _generateStringHelperHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            if 'vk_enum_string_helper' not in f:
                header.append("#include <%s>\n" % f)
        header.append('#include "vk_enum_string_helper.h"\n\n// Function Prototypes\n')
        header.append("char* dynamic_display(const void* pStruct, const char* prefix);\n")
        return "".join(header)

    def _generateStringHelperHeaderCpp(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            if 'vk_enum_string_helper' not in f:
                header.append("#include <%s>\n" % f)
        header.append('#include "vk_enum_string_helper.h"\n')
        header.append('namespace StreamControl\n')
        header.append('{\n')
        header.append('bool writeAddress = true;\n')
        header.append('template <typename T>\n')
        header.append('std::ostream& operator<< (std::ostream &out, T const* pointer)\n')
        header.append('{\n')
        header.append('    if(writeAddress)\n')
        header.append('    {\n')
        header.append('        out.operator<<(pointer);\n')
        header.append('    }\n')
        header.append('    else\n')
        header.append('    {\n')
        header.append('        std::operator<<(out, "address");\n')
        header.append('    }\n')
        header.append('    return out;\n')
        header.append('}\n')
        header.append('std::ostream& operator<<(std::ostream &out, char const*const s)\n')
        header.append('{\n')
        header.append('    return std::operator<<(out, s);\n')
        header.append('}\n')
        header.append('}\n')
        header.append('\n')
        header.append("std::string dynamic_display(const void* pStruct, const std::string prefix);\n")
        return "".join(header)

    def _generateValidateHelperFunctions(self):
        sh_funcs = []
        # We do two passes, first pass just generates prototypes for all the functsions
        for s in sorted(self.struct_dict):

            # Wrap this in platform check since it may contain undefined structs or functions
            add_platform_wrapper_entry(sh_funcs, typedef_fwd_dict[s])
            sh_funcs.append('uint32_t %s(const %s* pStruct);' % (self._get_vh_func_name(s), typedef_fwd_dict[s]))
            add_platform_wrapper_exit(sh_funcs, typedef_fwd_dict[s])

        sh_funcs.append('\n')
        for s in sorted(self.struct_dict):

            # Wrap this in platform check since it may contain undefined structs or functions
            add_platform_wrapper_entry(sh_funcs, typedef_fwd_dict[s])

            sh_funcs.append('uint32_t %s(const %s* pStruct)\n{' % (self._get_vh_func_name(s), typedef_fwd_dict[s]))
            for m in sorted(self.struct_dict[s]):
                # TODO : Need to handle arrays of enums like in VkRenderPassCreateInfo struct
                if is_type(self.struct_dict[s][m]['type'], 'enum') and not self.struct_dict[s][m]['ptr']:
                    sh_funcs.append('    if (!validate_%s(pStruct->%s))\n        return 0;' % (self.struct_dict[s][m]['type'], self.struct_dict[s][m]['name']))
                # TODO : Need a little refinement to this code to make sure type of struct matches expected input (ptr, const...)
                if is_type(self.struct_dict[s][m]['type'], 'struct'):
                    if (self.struct_dict[s][m]['ptr']):
                        sh_funcs.append('    if (pStruct->%s && !%s((const %s*)pStruct->%s))\n        return 0;' % (self.struct_dict[s][m]['name'], self._get_vh_func_name(self.struct_dict[s][m]['type']), self.struct_dict[s][m]['type'], self.struct_dict[s][m]['name']))
                    else:
                        sh_funcs.append('    if (!%s((const %s*)&pStruct->%s))\n        return 0;' % (self._get_vh_func_name(self.struct_dict[s][m]['type']), self.struct_dict[s][m]['type'], self.struct_dict[s][m]['name']))
            sh_funcs.append("    return 1;\n}")

            # End of platform wrapped section
            add_platform_wrapper_exit(sh_funcs, typedef_fwd_dict[s])

        return "\n".join(sh_funcs)

    def _generateValidateHelperHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            if 'vk_enum_validate_helper' not in f:
                header.append("#include <%s>\n" % f)
        header.append('#include "vk_enum_validate_helper.h"\n\n// Function Prototypes\n')
        #header.append("char* dynamic_display(const void* pStruct, const char* prefix);\n")
        return "".join(header)

    def _generateSizeHelperFunctions(self):
        sh_funcs = []
        # just generates prototypes for all the functions
        for s in sorted(self.struct_dict):

            # Wrap this in platform check since it may contain undefined structs or functions
            add_platform_wrapper_entry(sh_funcs, typedef_fwd_dict[s])
            sh_funcs.append('size_t %s(const %s* pStruct);' % (self._get_size_helper_func_name(s), typedef_fwd_dict[s]))
            add_platform_wrapper_exit(sh_funcs, typedef_fwd_dict[s])

        return "\n".join(sh_funcs)


    def _generateSizeHelperFunctionsC(self):
        sh_funcs = []
        # generate function definitions
        for s in sorted(self.struct_dict):

            # Wrap this in platform check since it may contain undefined structs or functions
            add_platform_wrapper_entry(sh_funcs, typedef_fwd_dict[s])

            skip_list = [] # Used when struct elements need to be skipped because size already accounted for
            sh_funcs.append('size_t %s(const %s* pStruct)\n{' % (self._get_size_helper_func_name(s), typedef_fwd_dict[s]))
            indent = '    '
            sh_funcs.append('%ssize_t structSize = 0;' % (indent))
            sh_funcs.append('%sif (pStruct) {' % (indent))
            indent = '        '
            sh_funcs.append('%sstructSize = sizeof(%s);' % (indent, typedef_fwd_dict[s]))
            i_decl = False
            for m in sorted(self.struct_dict[s]):
                if m in skip_list:
                    continue
                if self.struct_dict[s][m]['dyn_array']:
                    if self.struct_dict[s][m]['full_type'].count('*') > 1:
                        if not is_type(self.struct_dict[s][m]['type'], 'struct') and not 'char' in self.struct_dict[s][m]['type'].lower():
                            if 'ppMemoryBarriers' == self.struct_dict[s][m]['name']:
                                # TODO : For now be conservative and consider all memBarrier ptrs as largest possible struct
                                sh_funcs.append('%sstructSize += pStruct->%s*(sizeof(%s*) + sizeof(VkImageMemoryBarrier));' % (indent, self.struct_dict[s][m]['array_size'], self.struct_dict[s][m]['type']))
                            else:
                                sh_funcs.append('%sstructSize += pStruct->%s*(sizeof(%s*) + sizeof(%s));' % (indent, self.struct_dict[s][m]['array_size'], self.struct_dict[s][m]['type'], self.struct_dict[s][m]['type']))
                        else: # This is an array of char* or array of struct ptrs
                            if not i_decl:
                                sh_funcs.append('%suint32_t i = 0;' % (indent))
                                i_decl = True
                            sh_funcs.append('%sfor (i = 0; i < pStruct->%s; i++) {' % (indent, self.struct_dict[s][m]['array_size']))
                            indent = '            '
                            if is_type(self.struct_dict[s][m]['type'], 'struct'):
                                sh_funcs.append('%sstructSize += (sizeof(%s*) + %s(pStruct->%s[i]));' % (indent, self.struct_dict[s][m]['type'], self._get_size_helper_func_name(self.struct_dict[s][m]['type']), self.struct_dict[s][m]['name']))
                            else:
                                sh_funcs.append('%sstructSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->%s[i]))));' % (indent, self.struct_dict[s][m]['name']))
                            indent = '        '
                            sh_funcs.append('%s}' % (indent))
                    else:
                        if is_type(self.struct_dict[s][m]['type'], 'struct'):
                            if not i_decl:
                                sh_funcs.append('%suint32_t i = 0;' % (indent))
                                i_decl = True
                            sh_funcs.append('%sfor (i = 0; i < pStruct->%s; i++) {' % (indent, self.struct_dict[s][m]['array_size']))
                            indent = '            '
                            sh_funcs.append('%sstructSize += %s(&pStruct->%s[i]);' % (indent, self._get_size_helper_func_name(self.struct_dict[s][m]['type']), self.struct_dict[s][m]['name']))
                            indent = '        '
                            sh_funcs.append('%s}' % (indent))
                        else:
                            sh_funcs.append('%sstructSize += pStruct->%s*sizeof(%s);' % (indent, self.struct_dict[s][m]['array_size'], self.struct_dict[s][m]['type']))
                elif self.struct_dict[s][m]['ptr'] and 'pNext' != self.struct_dict[s][m]['name']:
                    if 'char' in self.struct_dict[s][m]['type'].lower():
                        sh_funcs.append('%sstructSize += (pStruct->%s != NULL) ? sizeof(%s)*(1+strlen(pStruct->%s)) : 0;' % (indent, self.struct_dict[s][m]['name'], self.struct_dict[s][m]['type'], self.struct_dict[s][m]['name']))
                    elif is_type(self.struct_dict[s][m]['type'], 'struct'):
                        sh_funcs.append('%sstructSize += %s(pStruct->%s);' % (indent, self._get_size_helper_func_name(self.struct_dict[s][m]['type']), self.struct_dict[s][m]['name']))
                    elif 'void' not in self.struct_dict[s][m]['type'].lower():
                        if (self.struct_dict[s][m]['type'] != 'xcb_connection_t'):
                            sh_funcs.append('%sstructSize += sizeof(%s);' % (indent, self.struct_dict[s][m]['type']))
                elif 'size_t' == self.struct_dict[s][m]['type'].lower():
                    sh_funcs.append('%sstructSize += pStruct->%s;' % (indent, self.struct_dict[s][m]['name']))
                    skip_list.append(m+1)
            indent = '    '
            sh_funcs.append('%s}' % (indent))
            sh_funcs.append("%sreturn structSize;\n}" % (indent))

            # End of platform wrapped section
            add_platform_wrapper_exit(sh_funcs, typedef_fwd_dict[s])

        # Now generate generic functions to loop over entire struct chain (or just handle single generic structs)
        if '_debug_' not in self.header_filename:
            for follow_chain in [True, False]:
                sh_funcs.append('%s' % self.lineinfo.get())
                if follow_chain:
                    sh_funcs.append('size_t get_struct_chain_size(const void* pStruct)\n{')
                else:
                    sh_funcs.append('size_t get_dynamic_struct_size(const void* pStruct)\n{')
                indent = '    '
                sh_funcs.append('%s// Just use VkApplicationInfo as struct until actual type is resolved' % (indent))
                sh_funcs.append('%sVkApplicationInfo* pNext = (VkApplicationInfo*)pStruct;' % (indent))
                sh_funcs.append('%ssize_t structSize = 0;' % (indent))
                if follow_chain:
                    sh_funcs.append('%swhile (pNext) {' % (indent))
                    indent = '        '
                sh_funcs.append('%sswitch (pNext->sType) {' % (indent))
                indent += '    '
                for e in enum_type_dict:
                    if 'StructureType' in e:
                        for v in sorted(enum_type_dict[e]):
                            struct_name = get_struct_name_from_struct_type(v)
                            if struct_name not in self.struct_dict:
                                continue

                            sh_funcs.append('%scase %s:' % (indent, v))
                            sh_funcs.append('%s{' % (indent))
                            indent += '    '
                            sh_funcs.append('%sstructSize += %s((%s*)pNext);' % (indent, self._get_size_helper_func_name(struct_name), struct_name))
                            sh_funcs.append('%sbreak;' % (indent))
                            indent = indent[:-4]
                            sh_funcs.append('%s}' % (indent))
                        sh_funcs.append('%sdefault:' % (indent))
                        indent += '    '
                        sh_funcs.append('%sassert(0);' % (indent))
                        sh_funcs.append('%sstructSize += 0;' % (indent))
                        indent = indent[:-4]
                indent = indent[:-4]
                sh_funcs.append('%s}' % (indent))
                if follow_chain:
                    sh_funcs.append('%spNext = (VkApplicationInfo*)pNext->pNext;' % (indent))
                    indent = indent[:-4]
                    sh_funcs.append('%s}' % (indent))
                sh_funcs.append('%sreturn structSize;\n}' % indent)
        return "\n".join(sh_funcs)

    def _generateSizeHelperHeader(self):
        header = []
        header.append('\n#ifdef __cplusplus\n')
        header.append('extern "C" {\n')
        header.append('#endif\n')
        header.append("\n")
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            header.append("#include <%s>\n" % f)
        header.append('\n// Function Prototypes\n')
        header.append("size_t get_struct_chain_size(const void* pStruct);\n")
        header.append("size_t get_dynamic_struct_size(const void* pStruct);\n")
        return "".join(header)

    def _generateSizeHelperHeaderC(self):
        header = []
        header.append('#include "vk_struct_size_helper.h"')
        header.append('#include <string.h>')
        header.append('#include <assert.h>')
        header.append('\n// Function definitions\n')
        return "\n".join(header)

    def _generateSizeHelperFooter(self):
        footer = []
        footer.append('\n\n#ifdef __cplusplus')
        footer.append('}')
        footer.append('#endif')
        return "\n".join(footer)

    def _generateHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            header.append("#include <%s>\n" % f)
        return "".join(header)

    # Declarations
    def _generateConstructorDeclarations(self, s):
        constructors = []
        constructors.append("    %s();\n" % self.get_class_name(s))
        constructors.append("    %s(%s* pInStruct);\n" % (self.get_class_name(s), typedef_fwd_dict[s]))
        constructors.append("    %s(const %s* pInStruct);\n" % (self.get_class_name(s), typedef_fwd_dict[s]))
        return "".join(constructors)

    def _generateDestructorDeclarations(self, s):
        return "    virtual ~%s();\n" % self.get_class_name(s)

    def _generateDisplayDeclarations(self, s):
        return "    void display_txt();\n    void display_single_txt();\n    void display_full_txt();\n"

    def _generateGetSetDeclarations(self, s):
        get_set = []
        get_set.append("    void set_indent(uint32_t indent) { m_indent = indent; }\n")
        for member in sorted(self.struct_dict[s]):
            # TODO : Skipping array set/get funcs for now
            if self.struct_dict[s][member]['array']:
                continue
            get_set.append("    %s get_%s() { return m_struct.%s; }\n" % (self.struct_dict[s][member]['full_type'], self.struct_dict[s][member]['name'], self.struct_dict[s][member]['name']))
            if not self.struct_dict[s][member]['const']:
                get_set.append("    void set_%s(%s inValue) { m_struct.%s = inValue; }\n" % (self.struct_dict[s][member]['name'], self.struct_dict[s][member]['full_type'], self.struct_dict[s][member]['name']))
        return "".join(get_set)

    def _generatePrivateMembers(self, s):
        priv = []
        priv.append("\nprivate:\n")
        priv.append("    %s m_struct;\n" % typedef_fwd_dict[s])
        priv.append("    const %s* m_origStructAddr;\n" % typedef_fwd_dict[s])
        priv.append("    uint32_t m_indent;\n")
        priv.append("    const char m_dummy_prefix;\n")
        priv.append("    void display_struct_members();\n")
        return "".join(priv)

    def _generateClassDeclaration(self):
        class_decl = []
        for s in sorted(self.struct_dict):
            class_decl.append("\n//class declaration")
            class_decl.append("class %s\n{\npublic:" % self.get_class_name(s))
            class_decl.append(self._generateConstructorDeclarations(s))
            class_decl.append(self._generateDestructorDeclarations(s))
            class_decl.append(self._generateDisplayDeclarations(s))
            class_decl.append(self._generateGetSetDeclarations(s))
            class_decl.append(self._generatePrivateMembers(s))
            class_decl.append("};\n")
        return "\n".join(class_decl)

    def _generateFooter(self):
        return "\n//any footer info for class\n"

    def _getSafeStructName(self, struct):
        return "safe_%s" % (struct)

    # If struct has sType or ptr members, generate safe type
    def _hasSafeStruct(self, s):
        exceptions = ['VkPhysicalDeviceFeatures']
        if s in exceptions:
            return False
        if 'sType' == self.struct_dict[s][0]['name']:
            return True
        for m in self.struct_dict[s]:
            if self.struct_dict[s][m]['ptr']:
                return True
        inclusions = ['VkDisplayPlanePropertiesKHR', 'VkDisplayModePropertiesKHR', 'VkDisplayPropertiesKHR']
        if s in inclusions:
            return True
        return False

    def _generateSafeStructHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append('#pragma once\n')
        header.append('#include "vulkan/vulkan.h"')
        return "".join(header)

    # If given ty is in obj list, or is a struct that contains anything in obj list, return True
    def _typeHasObject(self, ty, obj):
        if ty in obj:
            return True
        if is_type(ty, 'struct'):
            for m in self.struct_dict[ty]:
                if self.struct_dict[ty][m]['type'] in obj:
                    return True
        return False

    def _generateSafeStructDecls(self):
        ss_decls = []
        for s in struct_order_list:
            if not self._hasSafeStruct(s):
                continue
            if s in ifdef_dict:
                ss_decls.append('#ifdef %s' % ifdef_dict[s])
            ss_name = self._getSafeStructName(s)
            ss_decls.append("\nstruct %s {" % (ss_name))
            for m in sorted(self.struct_dict[s]):
                m_type = self.struct_dict[s][m]['type']
                if is_type(m_type, 'struct') and self._hasSafeStruct(m_type):
                    m_type = self._getSafeStructName(m_type)
                if self.struct_dict[s][m]['array_size'] != 0 and not self.struct_dict[s][m]['dyn_array']:
                    ss_decls.append("    %s %s[%s];" % (m_type, self.struct_dict[s][m]['name'], self.struct_dict[s][m]['array_size']))
                elif self.struct_dict[s][m]['ptr'] and 'safe_' not in m_type and not self._typeHasObject(m_type, vulkan.object_non_dispatch_list):#m_type in ['char', 'float', 'uint32_t', 'void', 'VkPhysicalDeviceFeatures']: # We'll never overwrite char* so it can remain const
                    ss_decls.append("    %s %s;" % (self.struct_dict[s][m]['full_type'], self.struct_dict[s][m]['name']))
                elif self.struct_dict[s][m]['array']:
                    ss_decls.append("    %s* %s;" % (m_type, self.struct_dict[s][m]['name']))
                elif self.struct_dict[s][m]['ptr']:
                    ss_decls.append("    %s* %s;" % (m_type, self.struct_dict[s][m]['name']))
                else:
                    ss_decls.append("    %s %s;" % (m_type, self.struct_dict[s][m]['name']))
            ss_decls.append("    %s(const %s* pInStruct);" % (ss_name, s))
            ss_decls.append("    %s(const %s& src);" % (ss_name, ss_name)) # Copy constructor
            ss_decls.append("    %s();" % (ss_name)) # Default constructor
            ss_decls.append("    ~%s();" % (ss_name))
            ss_decls.append("    void initialize(const %s* pInStruct);" % (s))
            ss_decls.append("    void initialize(const %s* src);" % (ss_name))
            ss_decls.append("    %s *ptr() { return reinterpret_cast<%s *>(this); }" % (s, s))
            ss_decls.append("    %s const *ptr() const { return reinterpret_cast<%s const *>(this); }" % (s, s))
            ss_decls.append("};")
            if s in ifdef_dict:
                ss_decls.append('#endif')
        return "\n".join(ss_decls)

    def _generateSafeStructSourceHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append('#include "vk_safe_struct.h"\n#include <string.h>\n\n')
        return "".join(header)

    def _generateSafeStructSource(self):
        ss_src = []
        for s in struct_order_list:
            if not self._hasSafeStruct(s):
                continue
            if s in ifdef_dict:
                ss_src.append('#ifdef %s' % ifdef_dict[s])
            ss_name = self._getSafeStructName(s)
            init_list = '' # list of members in struct constructor initializer
            default_init_list = '' # Default constructor just inits ptrs to nullptr in initializer
            init_func_txt = '' # Txt for initialize() function that takes struct ptr and inits members
            construct_txt = '' # Body of constuctor as well as body of initialize() func following init_func_txt
            destruct_txt = ''
            # VkWriteDescriptorSet is special case because pointers may be non-null but ignored
            # TODO : This is ugly, figure out better way to do this
            custom_construct_txt = {'VkWriteDescriptorSet' :
                                    '    switch (descriptorType) {\n'
                                    '        case VK_DESCRIPTOR_TYPE_SAMPLER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:\n'
                                    '        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:\n'
                                    '        if (descriptorCount && pInStruct->pImageInfo) {\n'
                                    '            pImageInfo = new VkDescriptorImageInfo[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pImageInfo[i] = pInStruct->pImageInfo[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:\n'
                                    '        if (descriptorCount && pInStruct->pBufferInfo) {\n'
                                    '            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pBufferInfo[i] = pInStruct->pBufferInfo[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:\n'
                                    '        if (descriptorCount && pInStruct->pTexelBufferView) {\n'
                                    '            pTexelBufferView = new VkBufferView[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pTexelBufferView[i] = pInStruct->pTexelBufferView[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        default:\n'
                                    '        break;\n'
                                    '    }\n'}
            for m in self.struct_dict[s]:
                m_name = self.struct_dict[s][m]['name']
                m_type = self.struct_dict[s][m]['type']
                if is_type(m_type, 'struct') and self._hasSafeStruct(m_type):
                    m_type = self._getSafeStructName(m_type)
                if self.struct_dict[s][m]['ptr'] and 'safe_' not in m_type and not self._typeHasObject(m_type, vulkan.object_non_dispatch_list):# in ['char', 'float', 'uint32_t', 'void', 'VkPhysicalDeviceFeatures']) or 'pp' == self.struct_dict[s][m]['name'][0:1]:
                    # Ptr types w/o a safe_struct, for non-null case need to allocate new ptr and copy data in
                    if 'KHR' in ss_name or m_type in ['void', 'char']:
                        # For these exceptions just copy initial value over for now
                        init_list += '\n\t%s(pInStruct->%s),' % (m_name, m_name)
                        init_func_txt += '    %s = pInStruct->%s;\n' % (m_name, m_name)
                    else:
                        default_init_list += '\n\t%s(nullptr),' % (m_name)
                        init_list += '\n\t%s(nullptr),' % (m_name)
                        init_func_txt += '    %s = nullptr;\n' % (m_name)
                        if 'pNext' != m_name and 'void' not in m_type:
                            if not self.struct_dict[s][m]['array']:
                                construct_txt += '    if (pInStruct->%s) {\n' % (m_name)
                                construct_txt += '        %s = new %s(*pInStruct->%s);\n' % (m_name, m_type, m_name)
                                construct_txt += '    }\n'
                                destruct_txt += '    if (%s)\n' % (m_name)
                                destruct_txt += '        delete %s;\n' % (m_name)
                            else: # new array and then init each element
                                construct_txt += '    if (pInStruct->%s) {\n' % (m_name)
                                construct_txt += '        %s = new %s[pInStruct->%s];\n' % (m_name, m_type, self.struct_dict[s][m]['array_size'])
                                #construct_txt += '        std::copy (pInStruct->%s, pInStruct->%s+pInStruct->%s, %s);\n' % (m_name, m_name, self.struct_dict[s][m]['array_size'], m_name)
                                construct_txt += '        memcpy ((void *)%s, (void *)pInStruct->%s, sizeof(%s)*pInStruct->%s);\n' % (m_name, m_name, m_type, self.struct_dict[s][m]['array_size'])
                                construct_txt += '    }\n'
                                destruct_txt += '    if (%s)\n' % (m_name)
                                destruct_txt += '        delete[] %s;\n' % (m_name)
                elif self.struct_dict[s][m]['array']:
                    if not self.struct_dict[s][m]['dyn_array']:
                        # Handle static array case
                        construct_txt += '    for (uint32_t i=0; i<%s; ++i) {\n' % (self.struct_dict[s][m]['array_size'])
                        construct_txt += '        %s[i] = pInStruct->%s[i];\n' % (m_name, m_name)
                        construct_txt += '    }\n'
                    else:
                        # Init array ptr to NULL
                        default_init_list += '\n\t%s(nullptr),' % (m_name)
                        init_list += '\n\t%s(nullptr),' % (m_name)
                        init_func_txt += '    %s = nullptr;\n' % (m_name)
                        array_element = 'pInStruct->%s[i]' % (m_name)
                        if is_type(self.struct_dict[s][m]['type'], 'struct') and self._hasSafeStruct(self.struct_dict[s][m]['type']):
                            array_element = '%s(&pInStruct->%s[i])' % (self._getSafeStructName(self.struct_dict[s][m]['type']), m_name)
                        construct_txt += '    if (%s && pInStruct->%s) {\n' % (self.struct_dict[s][m]['array_size'], m_name)
                        construct_txt += '        %s = new %s[%s];\n' % (m_name, m_type, self.struct_dict[s][m]['array_size'])
                        destruct_txt += '    if (%s)\n' % (m_name)
                        destruct_txt += '        delete[] %s;\n' % (m_name)
                        construct_txt += '        for (uint32_t i=0; i<%s; ++i) {\n' % (self.struct_dict[s][m]['array_size'])
                        if 'safe_' in m_type:
                            construct_txt += '            %s[i].initialize(&pInStruct->%s[i]);\n' % (m_name, m_name)
                        else:
                            construct_txt += '            %s[i] = %s;\n' % (m_name, array_element)
                        construct_txt += '        }\n'
                        construct_txt += '    }\n'
                elif self.struct_dict[s][m]['ptr']:
                    construct_txt += '    if (pInStruct->%s)\n' % (m_name)
                    construct_txt += '        %s = new %s(pInStruct->%s);\n' % (m_name, m_type, m_name)
                    construct_txt += '    else\n'
                    construct_txt += '        %s = NULL;\n' % (m_name)
                    destruct_txt += '    if (%s)\n' % (m_name)
                    destruct_txt += '        delete %s;\n' % (m_name)
                elif 'safe_' in m_type: # inline struct, need to pass in reference for constructor
                    init_list += '\n\t%s(&pInStruct->%s),' % (m_name, m_name)
                    init_func_txt += '        %s.initialize(&pInStruct->%s);\n' % (m_name, m_name)
                else:
                    init_list += '\n\t%s(pInStruct->%s),' % (m_name, m_name)
                    init_func_txt += '    %s = pInStruct->%s;\n' % (m_name, m_name)
            if '' != init_list:
                init_list = init_list[:-1] # hack off final comma
            if s in custom_construct_txt:
                construct_txt = custom_construct_txt[s]
            ss_src.append("\n%s::%s(const %s* pInStruct) : %s\n{\n%s}" % (ss_name, ss_name, s, init_list, construct_txt))
            if '' != default_init_list:
                default_init_list = " : %s" % (default_init_list[:-1])
            ss_src.append("\n%s::%s()%s\n{}" % (ss_name, ss_name, default_init_list))
            # Create slight variation of init and construct txt for copy constructor that takes a src object reference vs. struct ptr
            copy_construct_init = init_func_txt.replace('pInStruct->', 'src.')
            copy_construct_txt = construct_txt.replace(' (pInStruct->', ' (src.') # Exclude 'if' blocks from next line
            copy_construct_txt = copy_construct_txt.replace('(pInStruct->', '(*src.') # Pass object to copy constructors
            copy_construct_txt = copy_construct_txt.replace('pInStruct->', 'src.') # Modify remaining struct refs for src object
            ss_src.append("\n%s::%s(const %s& src)\n{\n%s%s}" % (ss_name, ss_name, ss_name, copy_construct_init, copy_construct_txt)) # Copy constructor
            ss_src.append("\n%s::~%s()\n{\n%s}" % (ss_name, ss_name, destruct_txt))
            ss_src.append("\nvoid %s::initialize(const %s* pInStruct)\n{\n%s%s}" % (ss_name, s, init_func_txt, construct_txt))
            # Copy initializer uses same txt as copy constructor but has a ptr and not a reference
            init_copy = copy_construct_init.replace('src.', 'src->')
            init_construct = copy_construct_txt.replace('src.', 'src->')
            ss_src.append("\nvoid %s::initialize(const %s* src)\n{\n%s%s}" % (ss_name, ss_name, init_copy, init_construct))
            if s in ifdef_dict:
                ss_src.append('#endif')
        return "\n".join(ss_src)

class EnumCodeGen:
    def __init__(self, enum_type_dict=None, enum_val_dict=None, typedef_fwd_dict=None, in_file=None, out_sh_file=None, out_vh_file=None):
        self.et_dict = enum_type_dict
        self.ev_dict = enum_val_dict
        self.tf_dict = typedef_fwd_dict
        self.in_file = in_file
        self.out_sh_file = out_sh_file
        self.eshfg = CommonFileGen(self.out_sh_file)
        self.out_vh_file = out_vh_file
        self.evhfg = CommonFileGen(self.out_vh_file)

    def generateStringHelper(self):
        self.eshfg.setHeader(self._generateSHHeader())
        self.eshfg.setBody(self._generateSHBody())
        self.eshfg.generate()

    def generateEnumValidate(self):
        self.evhfg.setHeader(self._generateSHHeader())
        self.evhfg.setBody(self._generateVHBody())
        self.evhfg.generate()

    def _generateVHBody(self):
        body = []
        for bet in sorted(self.et_dict):
            fet = self.tf_dict[bet]
            body.append("static inline uint32_t validate_%s(%s input_value)\n{" % (fet, fet))
            # TODO : This is not ideal, but allows for flag combinations. Need more rigorous validation of realistic flag combinations
            if 'flagbits' in bet.lower():
                body.append('    if (input_value > (%s))' % (' | '.join(self.et_dict[bet])))
                body.append('        return 0;')
                body.append('    return 1;')
                body.append('}\n\n')
            else:
                body.append('    switch ((%s)input_value)\n    {' % (fet))
                for e in sorted(self.et_dict[bet]):
                    if (self.ev_dict[e]['unique']):
                        body.append('        case %s:' % (e))
                body.append('            return 1;\n        default:\n            return 0;\n    }\n}\n\n')
        return "\n".join(body)

    def _generateSHBody(self):
        body = []
#        with open(self.out_file, "a") as hf:
            # bet == base_enum_type, fet == final_enum_type
        for bet in sorted(self.et_dict):
            fet = self.tf_dict[bet]
            body.append("static inline const char* string_%s(%s input_value)\n{\n    switch ((%s)input_value)\n    {" % (fet, fet, fet))
            for e in sorted(self.et_dict[bet]):
                if (self.ev_dict[e]['unique']):
                    body.append('        case %s:\n            return "%s";' % (e, e))
            body.append('        default:\n            return "Unhandled %s";\n    }\n}\n\n' % (fet))
        return "\n".join(body)

    def _generateSHHeader(self):
        header = []
        header.append('#pragma once\n')
        header.append('#ifdef _WIN32\n')
        header.append('#pragma warning( disable : 4065 )\n')
        header.append('#endif\n')
        header.append('#include <vulkan/%s>\n\n\n' % self.in_file)
        return "\n".join(header)


class CMakeGen:
    def __init__(self, struct_wrapper=None, out_dir=None):
        self.sw = struct_wrapper
        self.include_headers = []
        self.add_lib_file_list = self.sw.get_file_list()
        self.out_dir = out_dir
        self.out_file = os.path.join(self.out_dir, "CMakeLists.txt")
        self.cmg = CommonFileGen(self.out_file)

    def generate(self):
        self.cmg.setBody(self._generateBody())
        self.cmg.generate()

    def _generateBody(self):
        body = []
        body.append("project(%s)" % os.path.basename(self.out_dir))
        body.append("cmake_minimum_required(VERSION 2.8)\n")
        body.append("add_library(${PROJECT_NAME} %s)\n" % " ".join(self.add_lib_file_list))
        body.append('set(COMPILE_FLAGS "-fpermissive")')
        body.append('set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")\n')
        body.append("include_directories(${SRC_DIR}/thirdparty/${GEN_API}/inc/)\n")
        body.append("target_include_directories (%s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})\n" % os.path.basename(self.out_dir))
        return "\n".join(body)

class GraphVizGen:
    def __init__(self, struct_dict, prefix, out_dir):
        self.struct_dict = struct_dict
        self.api = prefix
        if prefix == "vulkan":
            self.api_prefix = "vk"
        else:
            self.api_prefix = prefix
        self.out_file = os.path.join(out_dir, self.api_prefix+"_struct_graphviz_helper.h")
        self.gvg = CommonFileGen(self.out_file)

    def generate(self):
        self.gvg.setCopyright("//This is the copyright\n")
        self.gvg.setHeader(self._generateHeader())
        self.gvg.setBody(self._generateBody())
        #self.gvg.setFooter('}')
        self.gvg.generate()

    def set_include_headers(self, include_headers):
        self.include_headers = include_headers

    def _generateHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            if 'vk_enum_string_helper' not in f:
                header.append("#include <%s>\n" % f)
        #header.append('#include "vk_enum_string_helper.h"\n\n// Function Prototypes\n')
        header.append("\nchar* dynamic_gv_display(const void* pStruct, const char* prefix);\n")
        return "".join(header)

    def _get_gv_func_name(self, struct):
        return "%s_gv_print_%s" % (self.api_prefix, struct.lower().strip("_"))

    # Return elements to create formatted string for given struct member
    def _get_struct_gv_print_formatted(self, struct_member, pre_var_name="", postfix = "\\n", struct_var_name="pStruct", struct_ptr=True, print_array=False, port_label=""):
        struct_op = "->"
        pre_var_name = '"%s "' % struct_member['full_type']
        if not struct_ptr:
            struct_op = "."
        member_name = struct_member['name']
        print_type = "p"
        cast_type = ""
        member_post = ""
        array_index = ""
        member_print_post = ""
        print_delimiter = "%"
        if struct_member['array'] and 'char' in struct_member['type'].lower(): # just print char array as string
            print_type = "p"
            print_array = False
        elif struct_member['array'] and not print_array:
            # Just print base address of array when not full print_array
            cast_type = "(void*)"
        elif is_type(struct_member['type'], 'enum'):
            if struct_member['ptr']:
                struct_var_name = "*" + struct_var_name
                print_delimiter = "0x%"
            cast_type = "string_%s" % struct_member['type']
            print_type = "s"
        elif is_type(struct_member['type'], 'struct'): # print struct address for now
            cast_type = "(void*)"
            print_delimiter = "0x%"
            if not struct_member['ptr']:
                cast_type = "(void*)&"
        elif 'bool' in struct_member['type'].lower():
            print_type = "s"
            member_post = ' ? "TRUE" : "FALSE"'
        elif 'float' in struct_member['type']:
            print_type = "f"
        elif 'uint64' in struct_member['type'] or 'gpusize' in struct_member['type'].lower():
            print_type = '" PRId64 "'
        elif 'uint8' in struct_member['type']:
            print_type = "hu"
        elif 'size' in struct_member['type'].lower():
            print_type = '" PRINTF_SIZE_T_SPECIFIER "'
            print_delimiter = ""
        elif True in [ui_str.lower() in struct_member['type'].lower() for ui_str in ['uint', 'flags', 'samplemask']]:
            print_type = "u"
        elif 'int' in struct_member['type']:
            print_type = "i"
        elif struct_member['ptr']:
            print_delimiter = "0x%"
            pass
        else:
            #print("Unhandled struct type: %s" % struct_member['type'])
            print_delimiter = "0x%"
            cast_type = "(void*)"
        if print_array and struct_member['array']:
            member_print_post = "[%u]"
            array_index = " i,"
            member_post = "[i]"
        print_out = "<TR><TD>%%s%s%s</TD><TD%s>%s%s%s</TD></TR>" % (member_name, member_print_post, port_label, print_delimiter, print_type, postfix) # section of print that goes inside of quotes
        print_arg = ", %s,%s %s(%s%s%s)%s\n" % (pre_var_name, array_index, cast_type, struct_var_name, struct_op, member_name, member_post) # section of print passed to portion in quotes
        return (print_out, print_arg)

    def _generateBody(self):
        gv_funcs = []
        array_func_list = [] # structs for which we'll generate an array version of their print function
        array_func_list.append('vkbufferviewattachinfo')
        array_func_list.append('vkimageviewattachinfo')
        array_func_list.append('vksamplerimageviewinfo')
        array_func_list.append('vkdescriptortypecount')
        # For first pass, generate prototype
        for s in sorted(self.struct_dict):
            gv_funcs.append('char* %s(const %s* pStruct, const char* myNodeName);\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
            if s.lower().strip("_") in array_func_list:
                if s.lower().strip("_") in ['vkbufferviewattachinfo', 'vkimageviewattachinfo']:
                    gv_funcs.append('char* %s_array(uint32_t count, const %s* const* pStruct, const char* myNodeName);\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
                else:
                    gv_funcs.append('char* %s_array(uint32_t count, const %s* pStruct, const char* myNodeName);\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
        gv_funcs.append('\n')
        for s in sorted(self.struct_dict):
            p_out = ""
            p_args = ""
            stp_list = [] # stp == "struct to print" a list of structs for this API call that should be printed as structs
            # the fields below are a super-hacky way for now to get port labels into GV output, TODO : Clean this up!            
            pl_dict = {}
            struct_num = 0
            # This isn't great but this pre-pass flags structs w/ pNext and other struct ptrs
            for m in sorted(self.struct_dict[s]):
                if 'pNext' == self.struct_dict[s][m]['name'] or is_type(self.struct_dict[s][m]['type'], 'struct'):
                    stp_list.append(self.struct_dict[s][m])
                    if 'pNext' == self.struct_dict[s][m]['name']:
                        pl_dict[m] = ' PORT=\\"pNext\\"'
                    else:
                        pl_dict[m] = ' PORT=\\"struct%i\\"' % struct_num
                    struct_num += 1
            gv_funcs.append('char* %s(const %s* pStruct, const char* myNodeName)\n{\n    char* str;\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
            num_stps = len(stp_list);
            total_strlen_str = ''
            if 0 != num_stps:
                gv_funcs.append("    char* tmpStr;\n")
                gv_funcs.append("    char nodeName[100];\n")
                gv_funcs.append('    char* stp_strs[%i];\n' % num_stps)
                for index in range(num_stps):
                    if (stp_list[index]['ptr']):
                        if 'pDescriptorInfo' == stp_list[index]['name']:
                            gv_funcs.append('    if (pStruct->pDescriptorInfo && (0 != pStruct->descriptorCount)) {\n')
                        else:
                            gv_funcs.append('    if (pStruct->%s) {\n' % stp_list[index]['name'])
                        if 'pNext' == stp_list[index]['name']:
                            gv_funcs.append('        sprintf(nodeName, "pNext_0x%p", (void*)pStruct->pNext);\n')
                            gv_funcs.append('        tmpStr = dynamic_gv_display((void*)pStruct->pNext, nodeName);\n')
                            gv_funcs.append('        stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % index)
                            gv_funcs.append('        sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":pNext -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % index)
                            gv_funcs.append('        free(tmpStr);\n')
                        else:
                            gv_funcs.append('        sprintf(nodeName, "%s_0x%%p", (void*)pStruct->%s);\n' % (stp_list[index]['name'], stp_list[index]['name']))
                            if stp_list[index]['name'] in ['pTypeCount', 'pSamplerImageViews']:
                                gv_funcs.append('        tmpStr = %s_array(pStruct->count, pStruct->%s, nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            else:
                                gv_funcs.append('        tmpStr = %s(pStruct->%s, nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            gv_funcs.append('        stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % (index))
                            gv_funcs.append('        sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":struct%i -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % (index, index))
                        gv_funcs.append('    }\n')
                        gv_funcs.append("    else\n        stp_strs[%i] = \"\";\n" % (index))
                    elif stp_list[index]['array']: # TODO : For now just printing first element of array
                        gv_funcs.append('    sprintf(nodeName, "%s_0x%%p", (void*)&pStruct->%s[0]);\n' % (stp_list[index]['name'], stp_list[index]['name']))
                        gv_funcs.append('    tmpStr = %s(&pStruct->%s[0], nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        gv_funcs.append('    stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % (index))
                        gv_funcs.append('    sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":struct%i -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % (index, index))
                    else:
                        gv_funcs.append('    sprintf(nodeName, "%s_0x%%p", (void*)&pStruct->%s);\n' % (stp_list[index]['name'], stp_list[index]['name']))
                        gv_funcs.append('    tmpStr = %s(&pStruct->%s, nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        gv_funcs.append('    stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % (index))
                        gv_funcs.append('    sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":struct%i -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % (index, index))
                    total_strlen_str += 'strlen(stp_strs[%i]) + ' % index
            gv_funcs.append('    str = (char*)malloc(%ssizeof(char)*2048);\n' % (total_strlen_str))
            gv_funcs.append('    sprintf(str, "\\"%s\\" [\\nlabel = <<TABLE BORDER=\\"0\\" CELLBORDER=\\"1\\" CELLSPACING=\\"0\\"><TR><TD COLSPAN=\\"2\\">%s (0x%p)</TD></TR>')
            p_args = ", myNodeName, myNodeName, pStruct"
            for m in sorted(self.struct_dict[s]):
                plabel = ""
                if m in pl_dict:
                    plabel = pl_dict[m]
                (p_out1, p_args1) = self._get_struct_gv_print_formatted(self.struct_dict[s][m], port_label=plabel)
                p_out += p_out1
                p_args += p_args1
            p_out += '</TABLE>>\\n];\\n\\n"'
            p_args += ");\n"
            gv_funcs.append(p_out)
            gv_funcs.append(p_args)
            if 0 != num_stps:
                gv_funcs.append('    for (int32_t stp_index = %i; stp_index >= 0; stp_index--) {\n' % (num_stps-1))
                gv_funcs.append('        if (0 < strlen(stp_strs[stp_index])) {\n')
                gv_funcs.append('            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));\n')
                gv_funcs.append('            free(stp_strs[stp_index]);\n')
                gv_funcs.append('        }\n')
                gv_funcs.append('    }\n')
            gv_funcs.append("    return str;\n}\n")
            if s.lower().strip("_") in array_func_list:
                ptr_array = False
                if s.lower().strip("_") in ['vkbufferviewattachinfo', 'vkimageviewattachinfo']:
                    ptr_array = True
                    gv_funcs.append('char* %s_array(uint32_t count, const %s* const* pStruct, const char* myNodeName)\n{\n    char* str;\n    char tmpStr[1024];\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
                else:
                    gv_funcs.append('char* %s_array(uint32_t count, const %s* pStruct, const char* myNodeName)\n{\n    char* str;\n    char tmpStr[1024];\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
                gv_funcs.append('    str = (char*)malloc(sizeof(char)*1024*count);\n')
                gv_funcs.append('    sprintf(str, "\\"%s\\" [\\nlabel = <<TABLE BORDER=\\"0\\" CELLBORDER=\\"1\\" CELLSPACING=\\"0\\"><TR><TD COLSPAN=\\"3\\">%s (0x%p)</TD></TR>", myNodeName, myNodeName, pStruct);\n')
                gv_funcs.append('    for (uint32_t i=0; i < count; i++) {\n')
                gv_funcs.append('        sprintf(tmpStr, "');
                p_args = ""
                p_out = ""
                for m in sorted(self.struct_dict[s]):
                    plabel = ""
                    (p_out1, p_args1) = self._get_struct_gv_print_formatted(self.struct_dict[s][m], port_label=plabel)
                    if 0 == m: # Add array index notation at end of first row
                        p_out1 = '%s<TD ROWSPAN=\\"%i\\" PORT=\\"slot%%u\\">%%u</TD></TR>' % (p_out1[:-5], len(self.struct_dict[s]))
                        p_args1 += ', i, i'
                    p_out += p_out1
                    p_args += p_args1
                p_out += '"'
                p_args += ");\n"
                if ptr_array:
                    p_args = p_args.replace('->', '[i]->')
                else:
                    p_args = p_args.replace('->', '[i].')
                gv_funcs.append(p_out);
                gv_funcs.append(p_args);
                gv_funcs.append('        strncat(str, tmpStr, strlen(tmpStr));\n')
                gv_funcs.append('    }\n')
                gv_funcs.append('    strncat(str, "</TABLE>>\\n];\\n\\n", 20);\n')
                gv_funcs.append('    return str;\n}\n')
        # Add function to dynamically print out unknown struct
        gv_funcs.append("char* dynamic_gv_display(const void* pStruct, const char* nodeName)\n{\n")
        gv_funcs.append("    // Cast to APP_INFO ptr initially just to pull sType off struct\n")
        gv_funcs.append("    VkStructureType sType = ((VkApplicationInfo*)pStruct)->sType;\n")
        gv_funcs.append("    switch (sType)\n    {\n")
        for e in enum_type_dict:
            if "StructureType" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = get_struct_name_from_struct_type(v)
                    if struct_name not in self.struct_dict:
                        continue

                    print_func_name = self._get_gv_func_name(struct_name)
                    # TODO : Hand-coded fixes for some exceptions
                    #if 'VkPipelineCbStateCreateInfo' in struct_name:
                    #    struct_name = 'VK_PIPELINE_CB_STATE'
                    if 'VkSemaphoreCreateInfo' in struct_name:
                        struct_name = 'VkSemaphoreCreateInfo'
                        print_func_name = self._get_gv_func_name(struct_name)
                    elif 'VkSemaphoreOpenInfo' in struct_name:
                        struct_name = 'VkSemaphoreOpenInfo'
                        print_func_name = self._get_gv_func_name(struct_name)
                    gv_funcs.append('        case %s:\n' % (v))
                    gv_funcs.append('            return %s((%s*)pStruct, nodeName);\n' % (print_func_name, struct_name))
                    #gv_funcs.append('        }\n')
                    #gv_funcs.append('        break;\n')
                gv_funcs.append("        default:\n")
                gv_funcs.append("        return NULL;\n")
                gv_funcs.append("    }\n")
        gv_funcs.append("}")
        return "".join(gv_funcs)





#    def _generateHeader(self):
#        hdr = []
#        hdr.append('digraph g {\ngraph [\nrankdir = "LR"\n];')
#        hdr.append('node [\nfontsize = "16"\nshape = "plaintext"\n];')
#        hdr.append('edge [\n];\n')
#        return "\n".join(hdr)
#
#    def _generateBody(self):
#        body = []
#        for s in sorted(self.struc_dict):
#            field_num = 1
#            body.append('"%s" [\nlabel = <<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0"> <TR><TD COLSPAN="2" PORT="f0">%s</TD></TR>' % (s, typedef_fwd_dict[s]))
#            for m in sorted(self.struc_dict[s]):
#                body.append('<TR><TD PORT="f%i">%s</TD><TD PORT="f%i">%s</TD></TR>' % (field_num, self.struc_dict[s][m]['full_type'], field_num+1, self.struc_dict[s][m]['name']))
#                field_num += 2
#            body.append('</TABLE>>\n];\n')
#        return "".join(body)

def main(argv=None):
    opts = handle_args()
    # Parse input file and fill out global dicts
    hfp = HeaderFileParser(opts.input_file)
    hfp.parse()
    # TODO : Don't want these to be global, see note at top about wrapper classes
    global enum_val_dict
    global enum_type_dict
    global struct_dict
    global typedef_fwd_dict
    global typedef_rev_dict
    global types_dict
    enum_val_dict = hfp.get_enum_val_dict()
    enum_type_dict = hfp.get_enum_type_dict()
    struct_dict = hfp.get_struct_dict()
    # TODO : Would like to validate struct data here to verify that all of the bools for struct members are correct at this point
    typedef_fwd_dict = hfp.get_typedef_fwd_dict()
    typedef_rev_dict = hfp.get_typedef_rev_dict()
    types_dict = hfp.get_types_dict()
    #print(enum_val_dict)
    #print(typedef_dict)
    #print(struct_dict)
    input_header = os.path.basename(opts.input_file)
    if 'vulkan.h' == input_header:
        input_header = "vulkan/vulkan.h"

    prefix = os.path.basename(opts.input_file).strip(".h")
    if prefix == "vulkan":
        prefix = "vk"
    if (opts.abs_out_dir is not None):
        enum_sh_filename = os.path.join(opts.abs_out_dir, prefix+"_enum_string_helper.h")
    else:
        enum_sh_filename = os.path.join(os.getcwd(), opts.rel_out_dir, prefix+"_enum_string_helper.h")
    enum_sh_filename = os.path.abspath(enum_sh_filename)
    if not os.path.exists(os.path.dirname(enum_sh_filename)):
        print("Creating output dir %s" % os.path.dirname(enum_sh_filename))
        os.mkdir(os.path.dirname(enum_sh_filename))
    if opts.gen_enum_string_helper:
        print("Generating enum string helper to %s" % enum_sh_filename)
        enum_vh_filename = os.path.join(os.path.dirname(enum_sh_filename), prefix+"_enum_validate_helper.h")
        print("Generating enum validate helper to %s" % enum_vh_filename)
        eg = EnumCodeGen(enum_type_dict, enum_val_dict, typedef_fwd_dict, os.path.basename(opts.input_file), enum_sh_filename, enum_vh_filename)
        eg.generateStringHelper()
        eg.generateEnumValidate()
    #for struct in struct_dict:
    #print(struct)
    if opts.gen_struct_wrappers:
        sw = StructWrapperGen(struct_dict, os.path.basename(opts.input_file).strip(".h"), os.path.dirname(enum_sh_filename))
        #print(sw.get_class_name(struct))
        sw.set_include_headers([input_header,os.path.basename(enum_sh_filename),"stdint.h","cinttypes", "stdio.h","stdlib.h"])
        print("Generating struct wrapper header to %s" % sw.header_filename)
        sw.generateHeader()
        print("Generating struct wrapper class to %s" % sw.class_filename)
        sw.generateBody()
        sw.generateStringHelper()
        sw.generateValidateHelper()
        # Generate a 2nd helper file that excludes addrs
        sw.set_no_addr(True)
        sw.generateStringHelper()
        sw.set_no_addr(False)
        sw.set_include_headers([input_header,os.path.basename(enum_sh_filename),"stdint.h","stdio.h","stdlib.h","iostream","sstream","string"])
        sw.set_no_addr(True)
        sw.generateStringHelperCpp()
        sw.set_no_addr(False)
        sw.generateStringHelperCpp()
        sw.set_include_headers(["stdio.h", "stdlib.h", input_header])
        sw.generateSizeHelper()
        sw.generateSizeHelperC()
        sw.generateSafeStructHeader()
        sw.generateSafeStructs()
    if opts.gen_struct_sizes:
        st = StructWrapperGen(struct_dict, os.path.basename(opts.input_file).strip(".h"), os.path.dirname(enum_sh_filename))
        st.set_include_headers(["stdio.h", "stdlib.h", input_header])
        st.generateSizeHelper()
        st.generateSizeHelperC()
    if opts.gen_cmake:
        cmg = CMakeGen(sw, os.path.dirname(enum_sh_filename))
        cmg.generate()
    if opts.gen_graphviz:
        gv = GraphVizGen(struct_dict, os.path.basename(opts.input_file).strip(".h"), os.path.dirname(enum_sh_filename))
        gv.set_include_headers([input_header,os.path.basename(enum_sh_filename),"stdint.h","stdio.h","stdlib.h", "cinttypes"])
        gv.generate()
    print("DONE!")
    #print(typedef_rev_dict)
    #print(types_dict)
    #recreate_structs()

if __name__ == "__main__":
    sys.exit(main())
