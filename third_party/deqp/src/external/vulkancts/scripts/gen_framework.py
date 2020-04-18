# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------
# Vulkan CTS
# ----------
#
# Copyright (c) 2015 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#-------------------------------------------------------------------------

import os
import re
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "..", "scripts"))

from build.common import DEQP_DIR
from khr_util.format import indentLines, writeInlFile

VULKAN_H	= os.path.join(os.path.dirname(__file__), "src", "vulkan.h.in")
VULKAN_DIR	= os.path.join(os.path.dirname(__file__), "..", "framework", "vulkan")

INL_HEADER = """\
/* WARNING: This is auto-generated file. Do not modify, since changes will
 * be lost! Modify the generating script instead.
 */\
"""

DEFINITIONS			= [
	("VK_API_VERSION",						"deUint32"),
	("VK_MAX_PHYSICAL_DEVICE_NAME_SIZE",	"size_t"),
	("VK_MAX_EXTENSION_NAME_SIZE",			"size_t"),
	("VK_UUID_SIZE",						"size_t"),
	("VK_LUID_SIZE_KHR",					"size_t"),
	("VK_MAX_MEMORY_TYPES",					"size_t"),
	("VK_MAX_MEMORY_HEAPS",					"size_t"),
	("VK_MAX_DESCRIPTION_SIZE",				"size_t"),
	("VK_ATTACHMENT_UNUSED",				"deUint32"),
	("VK_SUBPASS_EXTERNAL",					"deUint32"),
	("VK_QUEUE_FAMILY_IGNORED",				"deUint32"),
	("VK_QUEUE_FAMILY_EXTERNAL_KHR",		"deUint32"),
	("VK_REMAINING_MIP_LEVELS",				"deUint32"),
	("VK_REMAINING_ARRAY_LAYERS",			"deUint32"),
	("VK_WHOLE_SIZE",						"vk::VkDeviceSize"),
	("VK_TRUE",								"vk::VkBool32"),
	("VK_FALSE",							"vk::VkBool32"),
]

PLATFORM_TYPES		= [
	# VK_KHR_xlib_surface
	("Display*",					"XlibDisplayPtr",				"void*"),
	("Window",						"XlibWindow",					"deUintptr",),
	("VisualID",					"XlibVisualID",					"deUint32"),

	# VK_KHR_xcb_surface
	("xcb_connection_t*",			"XcbConnectionPtr",				"void*"),
	("xcb_window_t",				"XcbWindow",					"deUintptr"),
	("xcb_visualid_t",				"XcbVisualid",					"deUint32"),

	# VK_KHR_wayland_surface
	("struct wl_display*",			"WaylandDisplayPtr",			"void*"),
	("struct wl_surface*",			"WaylandSurfacePtr",			"void*"),

	# VK_KHR_mir_surface
	("MirConnection*",				"MirConnectionPtr",				"void*"),
	("MirSurface*",					"MirSurfacePtr",				"void*"),

	# VK_KHR_android_surface
	("ANativeWindow*",				"AndroidNativeWindowPtr",		"void*"),

	# VK_KHR_win32_surface
	("HINSTANCE",					"Win32InstanceHandle",			"void*"),
	("HWND",						"Win32WindowHandle",			"void*"),
	("HANDLE",						"Win32Handle",					"void*"),
	("const SECURITY_ATTRIBUTES*",	"Win32SecurityAttributesPtr",	"const void*"),
]
PLATFORM_TYPE_NAMESPACE	= "pt"
TYPE_SUBSTITUTIONS		= [
	("uint8_t",		"deUint8"),
	("uint16_t",	"deUint16"),
	("uint32_t",	"deUint32"),
	("uint64_t",	"deUint64"),
	("int8_t",		"deInt8"),
	("int16_t",		"deInt16"),
	("int32_t",		"deInt32"),
	("int64_t",		"deInt64"),
	("bool32_t",	"deUint32"),
	("size_t",		"deUintptr"),

	# Platform-specific
	("DWORD",		"deUint32"),
	("HANDLE*",		PLATFORM_TYPE_NAMESPACE + "::" + "Win32Handle*"),
	("LPCWSTR",		"char*"),
]

EXTENSION_POSTFIXES		= ["KHR", "EXT", "NV", "NVX", "KHX"]

def typeNameToEnumValue (name):
	name = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', name[2:])
	name = re.sub(r'([a-zA-Z])([0-9])', r'\1_\2', name)
	name = name.upper()

	# Patch irregularities
	name = name.replace("YCB_CR_", "YCBCR_")
	name = name.replace("WIN_32_", "WIN32_")
	name = name.replace("16_BIT_", "16BIT_")
	name = name.replace("D_3_D_12_", "D3D12_")
	name = name.replace("_IDPROPERTIES_", "_ID_PROPERTIES_")

	return name

class Handle:
	TYPE_DISP		= 0
	TYPE_NONDISP	= 1

	def __init__ (self, type, name):
		self.type	= type
		self.name	= name

	def getHandleType (self):
		return "HANDLE_TYPE_" + typeNameToEnumValue(self.name)

class Enum:
	def __init__ (self, name, values):
		self.name	= name
		self.values	= values

class Bitfield:
	def __init__ (self, name, values):
		self.name	= name
		self.values	= values

class Variable:
	def __init__ (self, type, name, arraySize = None):
		self.type		= type
		self.name		= name
		self.arraySize	= arraySize

class CompositeType:
	CLASS_STRUCT	= 0
	CLASS_UNION		= 1

	def __init__ (self, typeClass, name, members):
		self.typeClass	= typeClass
		self.name		= name
		self.members	= members

	def getClassName (self):
		names = {CompositeType.CLASS_STRUCT: 'struct', CompositeType.CLASS_UNION: 'union'}
		return names[self.typeClass]

class Function:
	TYPE_PLATFORM		= 0 # Not bound to anything
	TYPE_INSTANCE		= 1 # Bound to VkInstance
	TYPE_DEVICE			= 2 # Bound to VkDevice

	def __init__ (self, name, returnType, arguments):
		self.name		= name
		self.returnType	= returnType
		self.arguments	= arguments

	def getType (self):
		# Special functions
		if self.name == "vkGetInstanceProcAddr":
			return Function.TYPE_PLATFORM
		elif self.name == "vkGetDeviceProcAddr":
			return Function.TYPE_INSTANCE

		assert len(self.arguments) > 0
		firstArgType = self.arguments[0].type

		if firstArgType in ["VkInstance", "VkPhysicalDevice"]:
			return Function.TYPE_INSTANCE
		elif firstArgType in ["VkDevice", "VkCommandBuffer", "VkQueue"]:
			return Function.TYPE_DEVICE
		else:
			return Function.TYPE_PLATFORM

class API:
	def __init__ (self, definitions, handles, enums, bitfields, compositeTypes, functions):
		self.definitions	= definitions
		self.handles		= handles
		self.enums			= enums
		self.bitfields		= bitfields
		self.compositeTypes	= compositeTypes
		self.functions		= functions

def readFile (filename):
	with open(filename, 'rb') as f:
		return f.read()

IDENT_PTRN	= r'[a-zA-Z_][a-zA-Z0-9_]*'
TYPE_PTRN	= r'[a-zA-Z_][a-zA-Z0-9_ \t*]*'

def fixupEnumValues (values):
	fixed = []
	for name, value in values:
		if "_BEGIN_RANGE" in name or "_END_RANGE" in name:
			continue
		fixed.append((name, value))
	return fixed

def fixupType (type):
	for platformType, substitute, compat in PLATFORM_TYPES:
		if type == platformType:
			return PLATFORM_TYPE_NAMESPACE + "::" + substitute

	for src, dst in TYPE_SUBSTITUTIONS:
		type = type.replace(src, dst)

	return type

def fixupFunction (function):
	fixedArgs		= [Variable(fixupType(a.type), a.name, a.arraySize) for a in function.arguments]
	fixedReturnType	= fixupType(function.returnType)

	return Function(function.name, fixedReturnType, fixedArgs)

def getInterfaceName (function):
	assert function.name[:2] == "vk"
	return function.name[2].lower() + function.name[3:]

def getFunctionTypeName (function):
	assert function.name[:2] == "vk"
	return function.name[2:] + "Func"

def endsWith (str, postfix):
	return str[-len(postfix):] == postfix

def splitNameExtPostfix (name):
	knownExtPostfixes = EXTENSION_POSTFIXES
	for postfix in knownExtPostfixes:
		if endsWith(name, postfix):
			return (name[:-len(postfix)], postfix)
	return (name, "")

def getBitEnumNameForBitfield (bitfieldName):
	bitfieldName, postfix = splitNameExtPostfix(bitfieldName)

	assert bitfieldName[-1] == "s"
	return bitfieldName[:-1] + "Bits" + postfix

def getBitfieldNameForBitEnum (bitEnumName):
	bitEnumName, postfix = splitNameExtPostfix(bitEnumName)

	assert bitEnumName[-4:] == "Bits"
	return bitEnumName[:-4] + "s" + postfix

def parsePreprocDefinedValue (src, name):
	definition = re.search(r'#\s*define\s+' + name + r'\s+([^\n]+)\n', src)
	if definition is None:
		raise Exception("No such definition: %s" % name)
	value = definition.group(1).strip()

	if value == "UINT32_MAX":
		value = "(~0u)"

	return value

def parseEnum (name, src):
	keyValuePtrn	= '(' + IDENT_PTRN + r')\s*=\s*([^\s,}]+)\s*[,}]'
	matches			= re.findall(keyValuePtrn, src)

	return Enum(name, fixupEnumValues(matches))

# \note Parses raw enums, some are mapped to bitfields later
def parseEnums (src):
	matches	= re.findall(r'typedef enum(\s*' + IDENT_PTRN + r')?\s*{([^}]*)}\s*(' + IDENT_PTRN + r')\s*;', src)
	enums	= []

	for enumname, contents, typename in matches:
		enums.append(parseEnum(typename, contents))

	return enums

def parseCompositeType (type, name, src):
	# \todo [pyry] Array support is currently a hack (size coupled with name)
	typeNamePtrn	= r'(' + TYPE_PTRN + ')(\s' + IDENT_PTRN + r'(\[[^\]]+\])*)\s*;'
	matches			= re.findall(typeNamePtrn, src)
	members			= [Variable(fixupType(t.strip()), n.strip()) for t, n, a in matches]

	return CompositeType(type, name, members)

def parseCompositeTypes (src):
	typeMap	= { 'struct': CompositeType.CLASS_STRUCT, 'union': CompositeType.CLASS_UNION }
	matches	= re.findall(r'typedef (struct|union)(\s*' + IDENT_PTRN + r')?\s*{([^}]*)}\s*(' + IDENT_PTRN + r')\s*;', src)
	types	= []

	for type, structname, contents, typename in matches:
		types.append(parseCompositeType(typeMap[type], typename, contents))

	return types

def parseHandles (src):
	matches	= re.findall(r'VK_DEFINE(_NON_DISPATCHABLE|)_HANDLE\((' + IDENT_PTRN + r')\)[ \t]*[\n\r]', src)
	handles	= []
	typeMap	= {'': Handle.TYPE_DISP, '_NON_DISPATCHABLE': Handle.TYPE_NONDISP}

	for type, name in matches:
		handle = Handle(typeMap[type], name)
		handles.append(handle)

	return handles

def parseArgList (src):
	typeNamePtrn	= r'(' + TYPE_PTRN + ')(\s' + IDENT_PTRN + r')(\[[^\]]+\])?'
	args			= []

	for rawArg in src.split(','):
		m = re.search(typeNamePtrn, rawArg)
		args.append(Variable(m.group(1).strip(), m.group(2).strip(), m.group(3)))

	return args

def parseFunctions (src):
	ptrn		= r'VKAPI_ATTR\s+(' + TYPE_PTRN + ')VKAPI_CALL\s+(' + IDENT_PTRN + r')\s*\(([^)]*)\)\s*;'
	matches		= re.findall(ptrn, src)
	functions	= []

	for returnType, name, argList in matches:
		functions.append(Function(name.strip(), returnType.strip(), parseArgList(argList)))

	return [fixupFunction(f) for f in functions]

def parseBitfieldNames (src):
	ptrn		= r'typedef\s+VkFlags\s(' + IDENT_PTRN + r')\s*;'
	matches		= re.findall(ptrn, src)

	return matches

def parseAPI (src):
	definitions		= [(name, type, parsePreprocDefinedValue(src, name)) for name, type in DEFINITIONS]
	rawEnums		= parseEnums(src)
	bitfieldNames	= parseBitfieldNames(src)
	enums			= []
	bitfields		= []
	bitfieldEnums	= set([getBitEnumNameForBitfield(n) for n in bitfieldNames])

	for enum in rawEnums:
		if enum.name in bitfieldEnums:
			bitfields.append(Bitfield(getBitfieldNameForBitEnum(enum.name), enum.values))
		else:
			enums.append(enum)

	for bitfieldName in bitfieldNames:
		if not bitfieldName in [bitfield.name for bitfield in bitfields]:
			# Add empty bitfield
			bitfields.append(Bitfield(bitfieldName, []))

	return API(
		definitions		= definitions,
		handles			= parseHandles(src),
		enums			= enums,
		bitfields		= bitfields,
		compositeTypes	= parseCompositeTypes(src),
		functions		= parseFunctions(src))

def writeHandleType (api, filename):
	def gen ():
		yield "enum HandleType"
		yield "{"
		yield "\t%s = 0," % api.handles[0].getHandleType()
		for handle in api.handles[1:]:
			yield "\t%s," % handle.getHandleType()
		yield "\tHANDLE_TYPE_LAST"
		yield "};"
		yield ""

	writeInlFile(filename, INL_HEADER, gen())

def getEnumValuePrefix (enum):
	prefix = enum.name[0]
	for i in range(1, len(enum.name)):
		if enum.name[i].isupper() and not enum.name[i-1].isupper():
			prefix += "_"
		prefix += enum.name[i].upper()
	return prefix

def parseInt (value):
	if value[:2] == "0x":
		return int(value, 16)
	else:
		return int(value, 10)

def areEnumValuesLinear (enum):
	curIndex = 0
	for name, value in enum.values:
		if parseInt(value) != curIndex:
			return False
		curIndex += 1
	return True

def genEnumSrc (enum):
	yield "enum %s" % enum.name
	yield "{"

	for line in indentLines(["\t%s\t= %s," % v for v in enum.values]):
		yield line

	if areEnumValuesLinear(enum):
		yield ""
		yield "\t%s_LAST" % getEnumValuePrefix(enum)

	yield "};"

def genBitfieldSrc (bitfield):
	if len(bitfield.values) > 0:
		yield "enum %s" % getBitEnumNameForBitfield(bitfield.name)
		yield "{"
		for line in indentLines(["\t%s\t= %s," % v for v in bitfield.values]):
			yield line
		yield "};"

	yield "typedef deUint32 %s;" % bitfield.name

def genCompositeTypeSrc (type):
	yield "%s %s" % (type.getClassName(), type.name)
	yield "{"
	for line in indentLines(["\t%s\t%s;" % (m.type, m.name) for m in type.members]):
		yield line
	yield "};"

def genHandlesSrc (handles):
	def genLines (handles):
		for handle in handles:
			if handle.type == Handle.TYPE_DISP:
				yield "VK_DEFINE_HANDLE\t(%s,\t%s);" % (handle.name, handle.getHandleType())
			elif handle.type == Handle.TYPE_NONDISP:
				yield "VK_DEFINE_NON_DISPATCHABLE_HANDLE\t(%s,\t%s);" % (handle.name, handle.getHandleType())

	for line in indentLines(genLines(handles)):
		yield line

def writeBasicTypes (api, filename):
	def gen ():
		for line in indentLines(["#define %s\t(static_cast<%s>\t(%s))" % (name, type, value) for name, type, value in api.definitions]):
			yield line
		yield ""
		for line in genHandlesSrc(api.handles):
			yield line
		yield ""
		for enum in api.enums:
			for line in genEnumSrc(enum):
				yield line
			yield ""
		for bitfield in api.bitfields:
			for line in genBitfieldSrc(bitfield):
				yield line
			yield ""
		for line in indentLines(["VK_DEFINE_PLATFORM_TYPE(%s,\t%s);" % (s, c) for n, s, c in PLATFORM_TYPES]):
			yield line

	writeInlFile(filename, INL_HEADER, gen())

def writeCompositeTypes (api, filename):
	def gen ():
		for type in api.compositeTypes:
			for line in genCompositeTypeSrc(type):
				yield line
			yield ""

	writeInlFile(filename, INL_HEADER, gen())

def argListToStr (args):
	return ", ".join("%s %s%s" % (v.type, v.name, v.arraySize if v.arraySize != None else "") for v in args)

def writeInterfaceDecl (api, filename, functionTypes, concrete):
	def genProtos ():
		postfix = "" if concrete else " = 0"
		for function in api.functions:
			if function.getType() in functionTypes:
				yield "virtual %s\t%s\t(%s) const%s;" % (function.returnType, getInterfaceName(function), argListToStr(function.arguments), postfix)

	writeInlFile(filename, INL_HEADER, indentLines(genProtos()))

def writeFunctionPtrTypes (api, filename):
	def genTypes ():
		for function in api.functions:
			yield "typedef VKAPI_ATTR %s\t(VKAPI_CALL* %s)\t(%s);" % (function.returnType, getFunctionTypeName(function), argListToStr(function.arguments))

	writeInlFile(filename, INL_HEADER, indentLines(genTypes()))

def writeFunctionPointers (api, filename, functionTypes):
	writeInlFile(filename, INL_HEADER, indentLines(["%s\t%s;" % (getFunctionTypeName(function), getInterfaceName(function)) for function in api.functions if function.getType() in functionTypes]))

def writeInitFunctionPointers (api, filename, functionTypes, cond = None):
	def makeInitFunctionPointers ():
		for function in api.functions:
			if function.getType() in functionTypes and (cond == None or cond(function)):
				yield "m_vk.%s\t= (%s)\tGET_PROC_ADDR(\"%s\");" % (getInterfaceName(function), getFunctionTypeName(function), function.name)

	writeInlFile(filename, INL_HEADER, indentLines(makeInitFunctionPointers()))

def writeFuncPtrInterfaceImpl (api, filename, functionTypes, className):
	def makeFuncPtrInterfaceImpl ():
		for function in api.functions:
			if function.getType() in functionTypes:
				yield ""
				yield "%s %s::%s (%s) const" % (function.returnType, className, getInterfaceName(function), argListToStr(function.arguments))
				yield "{"
				yield "	%sm_vk.%s(%s);" % ("return " if function.returnType != "void" else "", getInterfaceName(function), ", ".join(a.name for a in function.arguments))
				yield "}"

	writeInlFile(filename, INL_HEADER, makeFuncPtrInterfaceImpl())

def writeStrUtilProto (api, filename):
	def makeStrUtilProto ():
		for line in indentLines(["const char*\tget%sName\t(%s value);" % (enum.name[2:], enum.name) for enum in api.enums]):
			yield line
		yield ""
		for line in indentLines(["inline tcu::Format::Enum<%s>\tget%sStr\t(%s value)\t{ return tcu::Format::Enum<%s>(get%sName, value);\t}" % (e.name, e.name[2:], e.name, e.name, e.name[2:]) for e in api.enums]):
			yield line
		yield ""
		for line in indentLines(["inline std::ostream&\toperator<<\t(std::ostream& s, %s value)\t{ return s << get%sStr(value);\t}" % (e.name, e.name[2:]) for e in api.enums]):
			yield line
		yield ""
		for line in indentLines(["tcu::Format::Bitfield<32>\tget%sStr\t(%s value);" % (bitfield.name[2:], bitfield.name) for bitfield in api.bitfields]):
			yield line
		yield ""
		for line in indentLines(["std::ostream&\toperator<<\t(std::ostream& s, const %s& value);" % (s.name) for s in api.compositeTypes]):
			yield line

	writeInlFile(filename, INL_HEADER, makeStrUtilProto())

def writeStrUtilImpl (api, filename):
	def makeStrUtilImpl ():
		for line in indentLines(["template<> const char*\tgetTypeName<%s>\t(void) { return \"%s\";\t}" % (handle.name, handle.name) for handle in api.handles]):
			yield line

		yield ""
		yield "namespace %s" % PLATFORM_TYPE_NAMESPACE
		yield "{"

		for line in indentLines("std::ostream& operator<< (std::ostream& s, %s\tv) { return s << tcu::toHex(v.internal); }" % s for n, s, c in PLATFORM_TYPES):
			yield line

		yield "}"

		for enum in api.enums:
			yield ""
			yield "const char* get%sName (%s value)" % (enum.name[2:], enum.name)
			yield "{"
			yield "\tswitch (value)"
			yield "\t{"
			for line in indentLines(["\t\tcase %s:\treturn \"%s\";" % (n, n) for n, v in enum.values] + ["\t\tdefault:\treturn DE_NULL;"]):
				yield line
			yield "\t}"
			yield "}"

		for bitfield in api.bitfields:
			yield ""
			yield "tcu::Format::Bitfield<32> get%sStr (%s value)" % (bitfield.name[2:], bitfield.name)
			yield "{"

			if len(bitfield.values) > 0:
				yield "\tstatic const tcu::Format::BitDesc s_desc[] ="
				yield "\t{"
				for line in indentLines(["\t\ttcu::Format::BitDesc(%s,\t\"%s\")," % (n, n) for n, v in bitfield.values]):
					yield line
				yield "\t};"
				yield "\treturn tcu::Format::Bitfield<32>(value, DE_ARRAY_BEGIN(s_desc), DE_ARRAY_END(s_desc));"
			else:
				yield "\treturn tcu::Format::Bitfield<32>(value, DE_NULL, DE_NULL);"

			yield "}"

		bitfieldTypeNames = set([bitfield.name for bitfield in api.bitfields])

		for type in api.compositeTypes:
			yield ""
			yield "std::ostream& operator<< (std::ostream& s, const %s& value)" % type.name
			yield "{"
			yield "\ts << \"%s = {\\n\";" % type.name
			for member in type.members:
				memberName	= member.name
				valFmt		= None
				newLine		= ""
				if member.type in bitfieldTypeNames:
					valFmt = "get%sStr(value.%s)" % (member.type[2:], member.name)
				elif member.type == "const char*" or member.type == "char*":
					valFmt = "getCharPtrStr(value.%s)" % member.name
				elif '[' in member.name:
					baseName = member.name[:member.name.find('[')]
					if baseName in ["extensionName", "deviceName", "layerName", "description"]:
						valFmt = "(const char*)value.%s" % baseName
					elif member.type == 'char' or member.type == 'deUint8':
						newLine = "'\\n' << "
						valFmt = "tcu::formatArray(tcu::Format::HexIterator<%s>(DE_ARRAY_BEGIN(value.%s)), tcu::Format::HexIterator<%s>(DE_ARRAY_END(value.%s)))" % (member.type, baseName, member.type, baseName)
					else:
						if baseName == "memoryTypes" or baseName == "memoryHeaps":
							endIter = "DE_ARRAY_BEGIN(value.%s) + value.%sCount" % (baseName, baseName[:-1])
						else:
							endIter = "DE_ARRAY_END(value.%s)" % baseName
						newLine = "'\\n' << "
						valFmt = "tcu::formatArray(DE_ARRAY_BEGIN(value.%s), %s)" % (baseName, endIter)
					memberName = baseName
				else:
					valFmt = "value.%s" % member.name
				yield ("\ts << \"\\t%s = \" << " % memberName) + newLine + valFmt + " << '\\n';"
			yield "\ts << '}';"
			yield "\treturn s;"
			yield "}"


	writeInlFile(filename, INL_HEADER, makeStrUtilImpl())

class ConstructorFunction:
	def __init__ (self, type, name, objectType, iface, arguments):
		self.type		= type
		self.name		= name
		self.objectType	= objectType
		self.iface		= iface
		self.arguments	= arguments

def getConstructorFunctions (api):
	funcs = []
	for function in api.functions:
		if (function.name[:8] == "vkCreate" or function.name == "vkAllocateMemory") and not "createInfoCount" in [a.name for a in function.arguments]:
			if function.name == "vkCreateDisplayModeKHR":
				continue # No way to delete display modes (bug?)

			# \todo [pyry] Rather hacky
			iface = None
			if function.getType() == Function.TYPE_PLATFORM:
				iface = Variable("const PlatformInterface&", "vk")
			elif function.getType() == Function.TYPE_INSTANCE:
				iface = Variable("const InstanceInterface&", "vk")
			else:
				iface = Variable("const DeviceInterface&", "vk")

			assert function.arguments[-2].type == "const VkAllocationCallbacks*"

			objectType	= function.arguments[-1].type.replace("*", "").strip()
			arguments	= function.arguments[:-1]
			funcs.append(ConstructorFunction(function.getType(), getInterfaceName(function), objectType, iface, arguments))
	return funcs

def writeRefUtilProto (api, filename):
	functions	= getConstructorFunctions(api)

	def makeRefUtilProto ():
		unindented = []
		for line in indentLines(["Move<%s>\t%s\t(%s = DE_NULL);" % (function.objectType, function.name, argListToStr([function.iface] + function.arguments)) for function in functions]):
			yield line

	writeInlFile(filename, INL_HEADER, makeRefUtilProto())

def writeRefUtilImpl (api, filename):
	functions = getConstructorFunctions(api)

	def makeRefUtilImpl ():
		yield "namespace refdetails"
		yield "{"
		yield ""

		for function in api.functions:
			if function.getType() == Function.TYPE_DEVICE \
			   and (function.name[:9] == "vkDestroy" or function.name == "vkFreeMemory") \
			   and not function.name == "vkDestroyDevice":
				objectType = function.arguments[-2].type
				yield "template<>"
				yield "void Deleter<%s>::operator() (%s obj) const" % (objectType, objectType)
				yield "{"
				yield "\tm_deviceIface->%s(m_device, obj, m_allocator);" % (getInterfaceName(function))
				yield "}"
				yield ""

		yield "} // refdetails"
		yield ""

		for function in functions:
			if function.type == Function.TYPE_DEVICE:
				dtorObj = "device"
			elif function.type == Function.TYPE_INSTANCE:
				if function.name == "createDevice":
					dtorObj = "object"
				else:
					dtorObj = "instance"
			else:
				dtorObj = "object"

			yield "Move<%s> %s (%s)" % (function.objectType, function.name, argListToStr([function.iface] + function.arguments))
			yield "{"
			yield "\t%s object = 0;" % function.objectType
			yield "\tVK_CHECK(vk.%s(%s));" % (function.name, ", ".join([a.name for a in function.arguments] + ["&object"]))
			yield "\treturn Move<%s>(check<%s>(object), Deleter<%s>(%s));" % (function.objectType, function.objectType, function.objectType, ", ".join(["vk", dtorObj, function.arguments[-1].name]))
			yield "}"
			yield ""

	writeInlFile(filename, INL_HEADER, makeRefUtilImpl())

def writeStructTraitsImpl (api, filename):
	def gen ():
		for type in api.compositeTypes:
			if type.getClassName() == "struct" and type.members[0].name == "sType":
				yield "template<> VkStructureType getStructureType<%s> (void)" % type.name
				yield "{"
				yield "\treturn VK_STRUCTURE_TYPE_%s;" % typeNameToEnumValue(type.name)
				yield "}"
				yield ""

	writeInlFile(filename, INL_HEADER, gen())

def writeNullDriverImpl (api, filename):
	def genNullDriverImpl ():
		specialFuncNames	= [
				"vkCreateGraphicsPipelines",
				"vkCreateComputePipelines",
				"vkGetInstanceProcAddr",
				"vkGetDeviceProcAddr",
				"vkEnumeratePhysicalDevices",
				"vkEnumerateInstanceExtensionProperties",
				"vkEnumerateDeviceExtensionProperties",
				"vkGetPhysicalDeviceFeatures",
				"vkGetPhysicalDeviceFeatures2KHR",
				"vkGetPhysicalDeviceProperties",
				"vkGetPhysicalDeviceProperties2KHR",
				"vkGetPhysicalDeviceQueueFamilyProperties",
				"vkGetPhysicalDeviceMemoryProperties",
				"vkGetPhysicalDeviceFormatProperties",
				"vkGetPhysicalDeviceImageFormatProperties",
				"vkGetDeviceQueue",
				"vkGetBufferMemoryRequirements",
				"vkGetBufferMemoryRequirements2KHR",
				"vkGetImageMemoryRequirements",
				"vkGetImageMemoryRequirements2KHR",
				"vkMapMemory",
				"vkAllocateDescriptorSets",
				"vkFreeDescriptorSets",
				"vkResetDescriptorPool",
				"vkAllocateCommandBuffers",
				"vkFreeCommandBuffers",
				"vkCreateDisplayModeKHR",
				"vkCreateSharedSwapchainsKHR",
			]
		specialFuncs		= [f for f in api.functions if f.name in specialFuncNames]
		createFuncs			= [f for f in api.functions if (f.name[:8] == "vkCreate" or f.name == "vkAllocateMemory") and not f in specialFuncs]
		destroyFuncs		= [f for f in api.functions if (f.name[:9] == "vkDestroy" or f.name == "vkFreeMemory") and not f in specialFuncs]
		dummyFuncs			= [f for f in api.functions if f not in specialFuncs + createFuncs + destroyFuncs]

		def getHandle (name):
			for handle in api.handles:
				if handle.name == name:
					return handle
			raise Exception("No such handle: %s" % name)

		for function in createFuncs:
			objectType	= function.arguments[-1].type.replace("*", "").strip()
			argsStr		= ", ".join([a.name for a in function.arguments[:-1]])

			yield "VKAPI_ATTR %s VKAPI_CALL %s (%s)" % (function.returnType, getInterfaceName(function), argListToStr(function.arguments))
			yield "{"
			yield "\tDE_UNREF(%s);" % function.arguments[-2].name

			if getHandle(objectType).type == Handle.TYPE_NONDISP:
				yield "\tVK_NULL_RETURN((*%s = allocateNonDispHandle<%s, %s>(%s)));" % (function.arguments[-1].name, objectType[2:], objectType, argsStr)
			else:
				yield "\tVK_NULL_RETURN((*%s = allocateHandle<%s, %s>(%s)));" % (function.arguments[-1].name, objectType[2:], objectType, argsStr)

			yield "}"
			yield ""

		for function in destroyFuncs:
			objectArg	= function.arguments[-2]

			yield "VKAPI_ATTR %s VKAPI_CALL %s (%s)" % (function.returnType, getInterfaceName(function), argListToStr(function.arguments))
			yield "{"
			for arg in function.arguments[:-2]:
				yield "\tDE_UNREF(%s);" % arg.name

			if getHandle(objectArg.type).type == Handle.TYPE_NONDISP:
				yield "\tfreeNonDispHandle<%s, %s>(%s, %s);" % (objectArg.type[2:], objectArg.type, objectArg.name, function.arguments[-1].name)
			else:
				yield "\tfreeHandle<%s, %s>(%s, %s);" % (objectArg.type[2:], objectArg.type, objectArg.name, function.arguments[-1].name)

			yield "}"
			yield ""

		for function in dummyFuncs:
			yield "VKAPI_ATTR %s VKAPI_CALL %s (%s)" % (function.returnType, getInterfaceName(function), argListToStr(function.arguments))
			yield "{"
			for arg in function.arguments:
				yield "\tDE_UNREF(%s);" % arg.name
			if function.returnType != "void":
				yield "\treturn VK_SUCCESS;"
			yield "}"
			yield ""

		def genFuncEntryTable (type, name):
			funcs = [f for f in api.functions if f.getType() == type]

			yield "static const tcu::StaticFunctionLibrary::Entry %s[] =" % name
			yield "{"
			for line in indentLines(["\tVK_NULL_FUNC_ENTRY(%s,\t%s)," % (function.name, getInterfaceName(function)) for function in funcs]):
				yield line
			yield "};"
			yield ""

		# Func tables
		for line in genFuncEntryTable(Function.TYPE_PLATFORM, "s_platformFunctions"):
			yield line

		for line in genFuncEntryTable(Function.TYPE_INSTANCE, "s_instanceFunctions"):
			yield line

		for line in genFuncEntryTable(Function.TYPE_DEVICE, "s_deviceFunctions"):
			yield line


	writeInlFile(filename, INL_HEADER, genNullDriverImpl())

def writeTypeUtil (api, filename):
	# Structs filled by API queries are not often used in test code
	QUERY_RESULT_TYPES = set([
			"VkPhysicalDeviceFeatures",
			"VkPhysicalDeviceLimits",
			"VkFormatProperties",
			"VkImageFormatProperties",
			"VkPhysicalDeviceSparseProperties",
			"VkQueueFamilyProperties",
			"VkMemoryType",
			"VkMemoryHeap",
		])
	COMPOSITE_TYPES = set([t.name for t in api.compositeTypes])

	def isSimpleStruct (type):
		def hasArrayMember (type):
			for member in type.members:
				if "[" in member.name:
					return True
			return False

		def hasCompositeMember (type):
			for member in type.members:
				if member.type in COMPOSITE_TYPES:
					return True
			return False

		return type.typeClass == CompositeType.CLASS_STRUCT and \
			   type.members[0].type != "VkStructureType" and \
			   not type.name in QUERY_RESULT_TYPES and \
			   not hasArrayMember(type) and \
			   not hasCompositeMember(type)

	def gen ():
		for type in api.compositeTypes:
			if not isSimpleStruct(type):
				continue

			yield ""
			yield "inline %s make%s (%s)" % (type.name, type.name[2:], argListToStr(type.members))
			yield "{"
			yield "\t%s res;" % type.name
			for line in indentLines(["\tres.%s\t= %s;" % (m.name, m.name) for m in type.members]):
				yield line
			yield "\treturn res;"
			yield "}"

	writeInlFile(filename, INL_HEADER, gen())

if __name__ == "__main__":
	src				= readFile(VULKAN_H)
	api				= parseAPI(src)
	platformFuncs	= set([Function.TYPE_PLATFORM])
	instanceFuncs	= set([Function.TYPE_INSTANCE])
	deviceFuncs		= set([Function.TYPE_DEVICE])

	writeHandleType				(api, os.path.join(VULKAN_DIR, "vkHandleType.inl"))
	writeBasicTypes				(api, os.path.join(VULKAN_DIR, "vkBasicTypes.inl"))
	writeCompositeTypes			(api, os.path.join(VULKAN_DIR, "vkStructTypes.inl"))
	writeInterfaceDecl			(api, os.path.join(VULKAN_DIR, "vkVirtualPlatformInterface.inl"),		functionTypes = platformFuncs,	concrete = False)
	writeInterfaceDecl			(api, os.path.join(VULKAN_DIR, "vkVirtualInstanceInterface.inl"),		functionTypes = instanceFuncs,	concrete = False)
	writeInterfaceDecl			(api, os.path.join(VULKAN_DIR, "vkVirtualDeviceInterface.inl"),			functionTypes = deviceFuncs,	concrete = False)
	writeInterfaceDecl			(api, os.path.join(VULKAN_DIR, "vkConcretePlatformInterface.inl"),		functionTypes = platformFuncs,	concrete = True)
	writeInterfaceDecl			(api, os.path.join(VULKAN_DIR, "vkConcreteInstanceInterface.inl"),		functionTypes = instanceFuncs,	concrete = True)
	writeInterfaceDecl			(api, os.path.join(VULKAN_DIR, "vkConcreteDeviceInterface.inl"),		functionTypes = deviceFuncs,	concrete = True)
	writeFunctionPtrTypes		(api, os.path.join(VULKAN_DIR, "vkFunctionPointerTypes.inl"))
	writeFunctionPointers		(api, os.path.join(VULKAN_DIR, "vkPlatformFunctionPointers.inl"),		functionTypes = platformFuncs)
	writeFunctionPointers		(api, os.path.join(VULKAN_DIR, "vkInstanceFunctionPointers.inl"),		functionTypes = instanceFuncs)
	writeFunctionPointers		(api, os.path.join(VULKAN_DIR, "vkDeviceFunctionPointers.inl"),			functionTypes = deviceFuncs)
	writeInitFunctionPointers	(api, os.path.join(VULKAN_DIR, "vkInitPlatformFunctionPointers.inl"),	functionTypes = platformFuncs,	cond = lambda f: f.name != "vkGetInstanceProcAddr")
	writeInitFunctionPointers	(api, os.path.join(VULKAN_DIR, "vkInitInstanceFunctionPointers.inl"),	functionTypes = instanceFuncs)
	writeInitFunctionPointers	(api, os.path.join(VULKAN_DIR, "vkInitDeviceFunctionPointers.inl"),		functionTypes = deviceFuncs)
	writeFuncPtrInterfaceImpl	(api, os.path.join(VULKAN_DIR, "vkPlatformDriverImpl.inl"),				functionTypes = platformFuncs,	className = "PlatformDriver")
	writeFuncPtrInterfaceImpl	(api, os.path.join(VULKAN_DIR, "vkInstanceDriverImpl.inl"),				functionTypes = instanceFuncs,	className = "InstanceDriver")
	writeFuncPtrInterfaceImpl	(api, os.path.join(VULKAN_DIR, "vkDeviceDriverImpl.inl"),				functionTypes = deviceFuncs,	className = "DeviceDriver")
	writeStrUtilProto			(api, os.path.join(VULKAN_DIR, "vkStrUtil.inl"))
	writeStrUtilImpl			(api, os.path.join(VULKAN_DIR, "vkStrUtilImpl.inl"))
	writeRefUtilProto			(api, os.path.join(VULKAN_DIR, "vkRefUtil.inl"))
	writeRefUtilImpl			(api, os.path.join(VULKAN_DIR, "vkRefUtilImpl.inl"))
	writeStructTraitsImpl		(api, os.path.join(VULKAN_DIR, "vkGetStructureTypeImpl.inl"))
	writeNullDriverImpl			(api, os.path.join(VULKAN_DIR, "vkNullDriverImpl.inl"))
	writeTypeUtil				(api, os.path.join(VULKAN_DIR, "vkTypeUtil.inl"))
