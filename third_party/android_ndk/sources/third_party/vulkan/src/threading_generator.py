#!/usr/bin/python3 -i
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
# Author: Mike Stroyan <stroyan@google.com>

import os,re,sys
from generator import *

# ThreadGeneratorOptions - subclass of GeneratorOptions.
#
# Adds options used by ThreadOutputGenerator objects during threading
# layer generation.
#
# Additional members
#   prefixText - list of strings to prefix generated header with
#     (usually a copyright statement + calling convention macros).
#   protectFile - True if multiple inclusion protection should be
#     generated (based on the filename) around the entire header.
#   protectFeature - True if #ifndef..#endif protection should be
#     generated around a feature interface in the header file.
#   genFuncPointers - True if function pointer typedefs should be
#     generated
#   protectProto - If conditional protection should be generated
#     around prototype declarations, set to either '#ifdef'
#     to require opt-in (#ifdef protectProtoStr) or '#ifndef'
#     to require opt-out (#ifndef protectProtoStr). Otherwise
#     set to None.
#   protectProtoStr - #ifdef/#ifndef symbol to use around prototype
#     declarations, if protectProto is set
#   apicall - string to use for the function declaration prefix,
#     such as APICALL on Windows.
#   apientry - string to use for the calling convention macro,
#     in typedefs, such as APIENTRY.
#   apientryp - string to use for the calling convention macro
#     in function pointer typedefs, such as APIENTRYP.
#   indentFuncProto - True if prototype declarations should put each
#     parameter on a separate line
#   indentFuncPointer - True if typedefed function pointers should put each
#     parameter on a separate line
#   alignFuncParam - if nonzero and parameters are being put on a
#     separate line, align parameter names at the specified column
class ThreadGeneratorOptions(GeneratorOptions):
    def __init__(self,
                 filename = None,
                 directory = '.',
                 apiname = None,
                 profile = None,
                 versions = '.*',
                 emitversions = '.*',
                 defaultExtensions = None,
                 addExtensions = None,
                 removeExtensions = None,
                 sortProcedure = regSortFeatures,
                 prefixText = "",
                 genFuncPointers = True,
                 protectFile = True,
                 protectFeature = True,
                 protectProto = None,
                 protectProtoStr = None,
                 apicall = '',
                 apientry = '',
                 apientryp = '',
                 indentFuncProto = True,
                 indentFuncPointer = False,
                 alignFuncParam = 0):
        GeneratorOptions.__init__(self, filename, directory, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.prefixText      = prefixText
        self.genFuncPointers = genFuncPointers
        self.protectFile     = protectFile
        self.protectFeature  = protectFeature
        self.protectProto    = protectProto
        self.protectProtoStr = protectProtoStr
        self.apicall         = apicall
        self.apientry        = apientry
        self.apientryp       = apientryp
        self.indentFuncProto = indentFuncProto
        self.indentFuncPointer = indentFuncPointer
        self.alignFuncParam  = alignFuncParam

# ThreadOutputGenerator - subclass of OutputGenerator.
# Generates Thread checking framework
#
# ---- methods ----
# ThreadOutputGenerator(errFile, warnFile, diagFile) - args as for
#   OutputGenerator. Defines additional internal state.
# ---- methods overriding base class ----
# beginFile(genOpts)
# endFile()
# beginFeature(interface, emit)
# endFeature()
# genType(typeinfo,name)
# genStruct(typeinfo,name)
# genGroup(groupinfo,name)
# genEnum(enuminfo, name)
# genCmd(cmdinfo)
class ThreadOutputGenerator(OutputGenerator):
    """Generate specified API interfaces in a specific style, such as a C header"""
    # This is an ordered list of sections in the header file.
    TYPE_SECTIONS = ['include', 'define', 'basetype', 'handle', 'enum',
                     'group', 'bitmask', 'funcpointer', 'struct']
    ALL_SECTIONS = TYPE_SECTIONS + ['command']
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        # Internal state - accumulators for different inner block text
        self.sections = dict([(section, []) for section in self.ALL_SECTIONS])
        self.intercepts = []

    # Check if the parameter passed in is a pointer to an array
    def paramIsArray(self, param):
        return param.attrib.get('len') is not None

    # Check if the parameter passed in is a pointer
    def paramIsPointer(self, param):
        ispointer = False
        for elem in param:
            #write('paramIsPointer '+elem.text, file=sys.stderr)
            #write('elem.tag '+elem.tag, file=sys.stderr)
            #if (elem.tail is None):
            #    write('elem.tail is None', file=sys.stderr)
            #else:
            #    write('elem.tail '+elem.tail, file=sys.stderr)
            if ((elem.tag is not 'type') and (elem.tail is not None)) and '*' in elem.tail:
                ispointer = True
            #    write('is pointer', file=sys.stderr)
        return ispointer
    def makeThreadUseBlock(self, cmd, functionprefix):
        """Generate C function pointer typedef for <command> Element"""
        paramdecl = ''
        thread_check_dispatchable_objects = [
            "VkCommandBuffer",
            "VkDevice",
            "VkInstance",
            "VkQueue",
        ]
        thread_check_nondispatchable_objects = [
            "VkBuffer",
            "VkBufferView",
            "VkCommandPool",
            "VkDescriptorPool",
            "VkDescriptorSetLayout",
            "VkDeviceMemory",
            "VkEvent",
            "VkFence",
            "VkFramebuffer",
            "VkImage",
            "VkImageView",
            "VkPipeline",
            "VkPipelineCache",
            "VkPipelineLayout",
            "VkQueryPool",
            "VkRenderPass",
            "VkSampler",
            "VkSemaphore",
            "VkShaderModule",
        ]

        # Find and add any parameters that are thread unsafe
        params = cmd.findall('param')
        for param in params:
            paramname = param.find('name')
            if False: # self.paramIsPointer(param):
                paramdecl += '    // not watching use of pointer ' + paramname.text + '\n'
            else:
                externsync = param.attrib.get('externsync')
                if externsync == 'true':
                    if self.paramIsArray(param):
                        paramdecl += '    for (uint32_t index=0;index<' + param.attrib.get('len') + ';index++) {\n'
                        paramdecl += '        ' + functionprefix + 'WriteObject(my_data, ' + paramname.text + '[index]);\n'
                        paramdecl += '    }\n'
                    else:
                        paramdecl += '    ' + functionprefix + 'WriteObject(my_data, ' + paramname.text + ');\n'
                elif (param.attrib.get('externsync')):
                    if self.paramIsArray(param):
                        # Externsync can list pointers to arrays of members to synchronize
                        paramdecl += '    for (uint32_t index=0;index<' + param.attrib.get('len') + ';index++) {\n'
                        for member in externsync.split(","):
                            # Replace first empty [] in member name with index
                            element = member.replace('[]','[index]',1)
                            if '[]' in element:
                                # Replace any second empty [] in element name with
                                # inner array index based on mapping array names like
                                # "pSomeThings[]" to "someThingCount" array size.
                                # This could be more robust by mapping a param member
                                # name to a struct type and "len" attribute.
                                limit = element[0:element.find('s[]')] + 'Count'
                                dotp = limit.rfind('.p')
                                limit = limit[0:dotp+1] + limit[dotp+2:dotp+3].lower() + limit[dotp+3:]
                                paramdecl += '        for(uint32_t index2=0;index2<'+limit+';index2++)\n'
                                element = element.replace('[]','[index2]')
                            paramdecl += '            ' + functionprefix + 'WriteObject(my_data, ' + element + ');\n'
                        paramdecl += '    }\n'
                    else:
                        # externsync can list members to synchronize
                        for member in externsync.split(","):
                            member = str(member).replace("::", "->")
                            paramdecl += '    ' + functionprefix + 'WriteObject(my_data, ' + member + ');\n'
                else:
                    paramtype = param.find('type')
                    if paramtype is not None:
                        paramtype = paramtype.text
                    else:
                        paramtype = 'None'
                    if paramtype in thread_check_dispatchable_objects or paramtype in thread_check_nondispatchable_objects:
                        if self.paramIsArray(param) and ('pPipelines' != paramname.text):
                            paramdecl += '    for (uint32_t index=0;index<' + param.attrib.get('len') + ';index++) {\n'
                            paramdecl += '        ' + functionprefix + 'ReadObject(my_data, ' + paramname.text + '[index]);\n'
                            paramdecl += '    }\n'
                        elif not self.paramIsPointer(param):
                            # Pointer params are often being created.
                            # They are not being read from.
                            paramdecl += '    ' + functionprefix + 'ReadObject(my_data, ' + paramname.text + ');\n'
        explicitexternsyncparams = cmd.findall("param[@externsync]")
        if (explicitexternsyncparams is not None):
            for param in explicitexternsyncparams:
                externsyncattrib = param.attrib.get('externsync')
                paramname = param.find('name')
                paramdecl += '    // Host access to '
                if externsyncattrib == 'true':
                    if self.paramIsArray(param):
                        paramdecl += 'each member of ' + paramname.text
                    elif self.paramIsPointer(param):
                        paramdecl += 'the object referenced by ' + paramname.text
                    else:
                        paramdecl += paramname.text
                else:
                    paramdecl += externsyncattrib
                paramdecl += ' must be externally synchronized\n'

        # Find and add any "implicit" parameters that are thread unsafe
        implicitexternsyncparams = cmd.find('implicitexternsyncparams')
        if (implicitexternsyncparams is not None):
            for elem in implicitexternsyncparams:
                paramdecl += '    // '
                paramdecl += elem.text
                paramdecl += ' must be externally synchronized between host accesses\n'

        if (paramdecl == ''):
            return None
        else:
            return paramdecl
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        # C-specific
        #
        # Multiple inclusion protection & C++ namespace.
        if (genOpts.protectFile and self.genOpts.filename):
            headerSym = '__' + re.sub('\.h', '_h_', os.path.basename(self.genOpts.filename))
            write('#ifndef', headerSym, file=self.outFile)
            write('#define', headerSym, '1', file=self.outFile)
            self.newline()
        write('namespace threading {', file=self.outFile)
        self.newline()
        #
        # User-supplied prefix text, if any (list of strings)
        if (genOpts.prefixText):
            for s in genOpts.prefixText:
                write(s, file=self.outFile)
    def endFile(self):
        # C-specific
        # Finish C++ namespace and multiple inclusion protection
        self.newline()
        # record intercepted procedures
        write('// intercepts', file=self.outFile)
        write('struct { const char* name; PFN_vkVoidFunction pFunc;} procmap[] = {', file=self.outFile)
        write('\n'.join(self.intercepts), file=self.outFile)
        write('};\n', file=self.outFile)
        self.newline()
        write('} // namespace threading', file=self.outFile)
        if (self.genOpts.protectFile and self.genOpts.filename):
            self.newline()
            write('#endif', file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFile(self)
    def beginFeature(self, interface, emit):
        #write('// starting beginFeature', file=self.outFile)
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)
        # C-specific
        # Accumulate includes, defines, types, enums, function pointer typedefs,
        # end function prototypes separately for this feature. They're only
        # printed in endFeature().
        self.sections = dict([(section, []) for section in self.ALL_SECTIONS])
        #write('// ending beginFeature', file=self.outFile)
    def endFeature(self):
        # C-specific
        # Actually write the interface to the output file.
        #write('// starting endFeature', file=self.outFile)
        if (self.emit):
            self.newline()
            if (self.genOpts.protectFeature):
                write('#ifndef', self.featureName, file=self.outFile)
            # If type declarations are needed by other features based on
            # this one, it may be necessary to suppress the ExtraProtect,
            # or move it below the 'for section...' loop.
            #write('// endFeature looking at self.featureExtraProtect', file=self.outFile)
            if (self.featureExtraProtect != None):
                write('#ifdef', self.featureExtraProtect, file=self.outFile)
            #write('#define', self.featureName, '1', file=self.outFile)
            for section in self.TYPE_SECTIONS:
                #write('// endFeature writing section'+section, file=self.outFile)
                contents = self.sections[section]
                if contents:
                    write('\n'.join(contents), file=self.outFile)
                    self.newline()
            #write('// endFeature looking at self.sections[command]', file=self.outFile)
            if (self.sections['command']):
                write('\n'.join(self.sections['command']), end='', file=self.outFile)
                self.newline()
            if (self.featureExtraProtect != None):
                write('#endif /*', self.featureExtraProtect, '*/', file=self.outFile)
            if (self.genOpts.protectFeature):
                write('#endif /*', self.featureName, '*/', file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFeature(self)
        #write('// ending endFeature', file=self.outFile)
    #
    # Append a definition to the specified section
    def appendSection(self, section, text):
        # self.sections[section].append('SECTION: ' + section + '\n')
        self.sections[section].append(text)
    #
    # Type generation
    def genType(self, typeinfo, name):
        pass
    #
    # Struct (e.g. C "struct" type) generation.
    # This is a special case of the <type> tag where the contents are
    # interpreted as a set of <member> tags instead of freeform C
    # C type declarations. The <member> tags are just like <param>
    # tags - they are a declaration of a struct or union member.
    # Only simple member declarations are supported (no nested
    # structs etc.)
    def genStruct(self, typeinfo, typeName):
        OutputGenerator.genStruct(self, typeinfo, typeName)
        body = 'typedef ' + typeinfo.elem.get('category') + ' ' + typeName + ' {\n'
        # paramdecl = self.makeCParamDecl(typeinfo.elem, self.genOpts.alignFuncParam)
        for member in typeinfo.elem.findall('.//member'):
            body += self.makeCParamDecl(member, self.genOpts.alignFuncParam)
            body += ';\n'
        body += '} ' + typeName + ';\n'
        self.appendSection('struct', body)
    #
    # Group (e.g. C "enum" type) generation.
    # These are concatenated together with other types.
    def genGroup(self, groupinfo, groupName):
        pass
    # Enumerant generation
    # <enum> tags may specify their values in several ways, but are usually
    # just integers.
    def genEnum(self, enuminfo, name):
        pass
    #
    # Command generation
    def genCmd(self, cmdinfo, name):
        # Commands shadowed by interface functions and are not implemented
        interface_functions = [
            'vkEnumerateInstanceLayerProperties',
            'vkEnumerateInstanceExtensionProperties',
            'vkEnumerateDeviceLayerProperties',
        ]
        if name in interface_functions:
            return
        special_functions = [
            'vkGetDeviceProcAddr',
            'vkGetInstanceProcAddr',
            'vkCreateDevice',
            'vkDestroyDevice',
            'vkCreateInstance',
            'vkDestroyInstance',
            'vkAllocateCommandBuffers',
            'vkFreeCommandBuffers',
            'vkCreateDebugReportCallbackEXT',
            'vkDestroyDebugReportCallbackEXT',
        ]
        if name in special_functions:
            decls = self.makeCDecls(cmdinfo.elem)
            self.appendSection('command', '')
            self.appendSection('command', '// declare only')
            self.appendSection('command', decls[0])
            self.intercepts += [ '    {"%s", reinterpret_cast<PFN_vkVoidFunction>(%s)},' % (name,name[2:]) ]
            return
        if "KHR" in name:
            self.appendSection('command', '// TODO - not wrapping KHR function ' + name)
            return
        if ("DebugMarker" in name) and ("EXT" in name):
            self.appendSection('command', '// TODO - not wrapping EXT function ' + name)
            return
        # Determine first if this function needs to be intercepted
        startthreadsafety = self.makeThreadUseBlock(cmdinfo.elem, 'start')
        if startthreadsafety is None:
            return
        finishthreadsafety = self.makeThreadUseBlock(cmdinfo.elem, 'finish')
        # record that the function will be intercepted
        if (self.featureExtraProtect != None):
            self.intercepts += [ '#ifdef %s' % self.featureExtraProtect ]
        self.intercepts += [ '    {"%s", reinterpret_cast<PFN_vkVoidFunction>(%s)},' % (name,name[2:]) ]
        if (self.featureExtraProtect != None):
            self.intercepts += [ '#endif' ]

        OutputGenerator.genCmd(self, cmdinfo, name)
        #
        decls = self.makeCDecls(cmdinfo.elem)
        self.appendSection('command', '')
        self.appendSection('command', decls[0][:-1])
        self.appendSection('command', '{')
        # setup common to call wrappers
        # first parameter is always dispatchable
        dispatchable_type = cmdinfo.elem.find('param/type').text
        dispatchable_name = cmdinfo.elem.find('param/name').text
        self.appendSection('command', '    dispatch_key key = get_dispatch_key('+dispatchable_name+');')
        self.appendSection('command', '    layer_data *my_data = get_my_data_ptr(key, layer_data_map);')
        if dispatchable_type in ["VkPhysicalDevice", "VkInstance"]:
            self.appendSection('command', '    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;')
        else:
            self.appendSection('command', '    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;')
        # Declare result variable, if any.
        resulttype = cmdinfo.elem.find('proto/type')
        if (resulttype != None and resulttype.text == 'void'):
          resulttype = None
        if (resulttype != None):
            self.appendSection('command', '    ' + resulttype.text + ' result;')
            assignresult = 'result = '
        else:
            assignresult = ''

        self.appendSection('command', '    bool threadChecks = startMultiThread();')
        self.appendSection('command', '    if (threadChecks) {')
        self.appendSection('command', "    "+"\n    ".join(str(startthreadsafety).rstrip().split("\n")))
        self.appendSection('command', '    }')
        params = cmdinfo.elem.findall('param/name')
        paramstext = ','.join([str(param.text) for param in params])
        API = cmdinfo.elem.attrib.get('name').replace('vk','pTable->',1)
        self.appendSection('command', '    ' + assignresult + API + '(' + paramstext + ');')
        self.appendSection('command', '    if (threadChecks) {')
        self.appendSection('command', "    "+"\n    ".join(str(finishthreadsafety).rstrip().split("\n")))
        self.appendSection('command', '    } else {')
        self.appendSection('command', '        finishMultiThread();')
        self.appendSection('command', '    }')
        # Return result variable, if any.
        if (resulttype != None):
            self.appendSection('command', '    return result;')
        self.appendSection('command', '}')
    #
    # override makeProtoName to drop the "vk" prefix
    def makeProtoName(self, name, tail):
        return self.genOpts.apientry + name[2:] + tail
