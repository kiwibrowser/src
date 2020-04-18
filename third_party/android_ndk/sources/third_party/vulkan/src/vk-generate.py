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
# Author: Chia-I Wu <olv@lunarg.com>
# Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
# Author: Jon Ashburn <jon@lunarg.com>
# Author: Gwan-gyeong Mun <kk.moon@samsung.com>

import sys

import vulkan

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'v' || %s[1] != 'k')\n" \
           "        return NULL;" % ((name,) * 3)

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.headers = vulkan.headers
        self.protos = vulkan.protos
        self.outfile = None

    def run(self):
        if self.outfile:
            with open(self.outfile, "w") as outfile:
                outfile.write(self.generate())
        else:
            print(self.generate())

    def generate(self):
        copyright = self.generate_copyright()
        header = self.generate_header()
        body = self.generate_body()
        footer = self.generate_footer()

        contents = []
        if copyright:
            contents.append(copyright)
        if header:
            contents.append(header)
        if body:
            contents.append(body)
        if footer:
            contents.append(footer)

        return "\n\n".join(contents)

    def generate_copyright(self):
        return """/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 */"""

    def generate_header(self):
        return "\n".join(["#include <" + h + ">" for h in self.headers])

    def generate_body(self):
        pass

    def generate_footer(self):
        pass

class DispatchTableOpsSubcommand(Subcommand):
    def __init__(self, argv):
        self.argv = argv
        self.headers = vulkan.headers_all
        self.protos = vulkan.protos_all
        self.outfile = None

    def run(self):
        if len(self.argv) < 1:
            print("DispatchTableOpsSubcommand: <prefix> unspecified")
            return

        self.prefix = self.argv[0]

        if len(self.argv) > 2:
            print("DispatchTableOpsSubcommand: <prefix> [outfile]")
            return

        if len(self.argv) == 2:
            self.outfile = self.argv[1]

        super(DispatchTableOpsSubcommand, self).run()

    def generate_header(self):
        return "\n".join(["#include <vulkan/vulkan.h>",
                          "#include <vulkan/vk_layer.h>",
                          "#include <string.h>"])

    def _generate_init_dispatch(self, type):
        stmts = []
        func = []
        if type == "device":
            # GPA has to be first one and uses wrapped object
            stmts.append("    memset(table, 0, sizeof(*table));")
            stmts.append("    // Core device function pointers")
            stmts.append("    table->GetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) gpa(device, \"vkGetDeviceProcAddr\");")

            for proto in self.protos:
                if proto.name == "CreateInstance" or proto.name == "EnumerateInstanceExtensionProperties" or \
                  proto.name == "EnumerateInstanceLayerProperties" or proto.params[0].ty == "VkInstance" or \
                  proto.params[0].ty == "VkPhysicalDevice" or proto.name == "GetDeviceProcAddr":
                    continue
                if proto.name == "GetMemoryWin32HandleNV":
                    stmts.append("#ifdef VK_USE_PLATFORM_WIN32_KHR")
                    stmts.append("    table->%s = (PFN_vk%s) gpa(device, \"vk%s\");" %
                            (proto.name, proto.name, proto.name))
                    stmts.append("#endif // VK_USE_PLATFORM_WIN32_KHR")
                else:
                    stmts.append("    table->%s = (PFN_vk%s) gpa(device, \"vk%s\");" %
                            (proto.name, proto.name, proto.name))
            func.append("static inline void %s_init_device_dispatch_table(VkDevice device,"
                % self.prefix)
            func.append("%s                                               VkLayerDispatchTable *table,"
                % (" " * len(self.prefix)))
            func.append("%s                                               PFN_vkGetDeviceProcAddr gpa)"
                % (" " * len(self.prefix)))
        else:
            stmts.append("    memset(table, 0, sizeof(*table));")
            stmts.append("    // Core instance function pointers")
            stmts.append("    table->GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) gpa(instance, \"vkGetInstanceProcAddr\");")

            for proto in self.protos:
                if proto.params[0].ty != "VkInstance" and proto.params[0].ty != "VkPhysicalDevice" or \
                  proto.name == "CreateDevice" or proto.name == "GetInstanceProcAddr":
                    continue
                # Protect platform-dependent APIs with #ifdef
                if 'KHR' in proto.name and 'Win32' in proto.name:
                    stmts.append("#ifdef VK_USE_PLATFORM_WIN32_KHR")
                if 'KHR' in proto.name and 'Xlib' in proto.name:
                    stmts.append("#ifdef VK_USE_PLATFORM_XLIB_KHR")
                if 'KHR' in proto.name and 'Xcb' in proto.name:
                    stmts.append("#ifdef VK_USE_PLATFORM_XCB_KHR")
                if 'KHR' in proto.name and 'Mir' in proto.name:
                    stmts.append("#ifdef VK_USE_PLATFORM_MIR_KHR")
                if 'KHR' in proto.name and 'Wayland' in proto.name:
                    stmts.append("#ifdef VK_USE_PLATFORM_WAYLAND_KHR")
                if 'KHR' in proto.name and 'Android' in proto.name:
                    stmts.append("#ifdef VK_USE_PLATFORM_ANDROID_KHR")
                # Output dispatch table entry
                stmts.append("    table->%s = (PFN_vk%s) gpa(instance, \"vk%s\");" %
                      (proto.name, proto.name, proto.name))
                # If entry was protected by an #ifdef, close with a #endif
                if 'KHR' in proto.name and 'Win32' in proto.name:
                    stmts.append("#endif // VK_USE_PLATFORM_WIN32_KHR")
                if 'KHR' in proto.name and 'Xlib' in proto.name:
                    stmts.append("#endif // VK_USE_PLATFORM_XLIB_KHR")
                if 'KHR' in proto.name and 'Xcb' in proto.name:
                    stmts.append("#endif // VK_USE_PLATFORM_XCB_KHR")
                if 'KHR' in proto.name and 'Mir' in proto.name:
                    stmts.append("#endif // VK_USE_PLATFORM_MIR_KHR")
                if 'KHR' in proto.name and 'Wayland' in proto.name:
                    stmts.append("#endif // VK_USE_PLATFORM_WAYLAND_KHR")
                if 'KHR' in proto.name and 'Android' in proto.name:
                    stmts.append("#endif // VK_USE_PLATFORM_ANDROID_KHR")
            func.append("static inline void %s_init_instance_dispatch_table(" % self.prefix)
            func.append("%s        VkInstance instance," % (" " * len(self.prefix)))
            func.append("%s        VkLayerInstanceDispatchTable *table," % (" " * len(self.prefix)))
            func.append("%s        PFN_vkGetInstanceProcAddr gpa)" % (" " * len(self.prefix)))
        func.append("{")
        func.append("%s" % "\n".join(stmts))
        func.append("}")

        return "\n".join(func)

    def generate_body(self):
        body = [self._generate_init_dispatch("device"),
                self._generate_init_dispatch("instance")]

        return "\n\n".join(body)

class WinDefFileSubcommand(Subcommand):
    def run(self):
        library_exports = {
                "all": [],
                "icd": [
                    "vk_icdGetInstanceProcAddr",
                ],
                "layer": [
                    "vkGetInstanceProcAddr",
                    "vkGetDeviceProcAddr",
                    "vkEnumerateInstanceLayerProperties",
                    "vkEnumerateInstanceExtensionProperties"
                ],
                "layer_multi": [
                    "multi2GetInstanceProcAddr",
                    "multi1GetDeviceProcAddr"
                ]
        }

        if len(self.argv) < 2 or len(self.argv) > 3 or self.argv[1] not in library_exports:
            print("WinDefFileSubcommand: <library-name> {%s} [outfile]" %
                    "|".join(library_exports.keys()))
            return

        self.library = self.argv[0]
        if self.library == "VkLayer_multi":
            self.exports = library_exports["layer_multi"]
        else:
            self.exports = library_exports[self.argv[1]]

        if len(self.argv) == 3:
            self.outfile = self.argv[2]

        super(WinDefFileSubcommand, self).run()

    def generate_copyright(self):
        return """; THIS FILE IS GENERATED.  DO NOT EDIT.

;;;; Begin Copyright Notice ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Vulkan
;
; Copyright (c) 2015-2016 The Khronos Group Inc.
; Copyright (c) 2015-2016 Valve Corporation
; Copyright (c) 2015-2016 LunarG, Inc.
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;
;  Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
;;;;  End Copyright Notice ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"""

    def generate_header(self):
        return "; The following is required on Windows, for exporting symbols from the DLL"

    def generate_body(self):
        body = []

        body.append("LIBRARY " + self.library)
        body.append("EXPORTS")

        for proto in self.exports:
            if self.library != "VkLayerSwapchain" or proto != "vkEnumerateInstanceExtensionProperties" and proto != "vkEnumerateInstanceLayerProperties":
                body.append( proto)

        return "\n".join(body)

def main():
    wsi = {
            "Win32",
            "Android",
            "Xcb",
            "Xlib",
            "Wayland",
            "Mir",
            "Display",
            "AllPlatforms"
    }
    subcommands = {
            "dispatch-table-ops": DispatchTableOpsSubcommand,
            "win-def-file": WinDefFileSubcommand,
    }

    if len(sys.argv) < 3 or sys.argv[1] not in wsi or sys.argv[2] not in subcommands:
        print("Usage: %s <wsi> <subcommand> [options]" % sys.argv[0])
        print
        print("Available sucommands are: %s" % " ".join(subcommands))
        exit(1)

    subcmd = subcommands[sys.argv[2]](sys.argv[3:])
    subcmd.run()

if __name__ == "__main__":
    main()
