
## Windows Runtime Installer

This directory contains the files required for building the Windows Vulkan Runtime Installer package.
The runtime installer is a method of delivering a Vulkan loader to system.
The runtime installer is used by the SDK installer.
It is also used by some drivers to ensure that an adequate Vulkan loader is installed on a system.
Additionally, applications may install a runtime to ensure that a minimum loader version is present.

To build a runtime installer:
1. Get a copy of the Nullsoft Install System (NSIS) version 3.0b3.
   Other versions may work, but the patch included in this directory is built against version 3.0b3.
   Apply the `NSIS_Security.patch` file provided in this directory to the NSIS source code.
   This security patch adds the /DYNAMICBASE /GS, and /guard:cf options to the build.
   In addition, it will be necessary to specify NSIS_CONFIG_LOG=yes and NSIS_MAX_STRLEN=8192 when compiling.
   Once you have applied the patch, compile NSIS with the command:
    ```
    scons SKIPUTILS="NSIS Menu","MakeLangId" UNICODE=yes \
        ZLIB_W32=<path_to_zlib>\zlib-1.2.7-win32-x86 NSIS_MAX_STRLEN=8192 \
        NSIS_CONFIG_LOG=yes NSIS_CONFIG_LOG_TIMESTAMP=yes \
        APPEND_CCFLAGS="/DYNAMICBASE /Zi" APPEND_LINKFLAGS="/DYNAMICBASE \
        /DEBUG /OPT:REF /OPT:ICF" SKIPDOC=all dist-zip
    ```

2. The compilation should have created a zip file containing the new NSIS build.
   Unzip this file to the location of your choosing.
   Download the NSIS Access Control plugin in copy it into the plugin directory in the build you just unzipped.
   It may be useful to prepend this NSIS binary directory to your system's path, so that this NSIS will be run when you type "makensis".
   Otherwise, you will just have to specify the full path to makensis.exe in the following steps.
   It may be useful to verify that all shared libraries in the build have the DYNAMIC_BASE and NX_COMPAT flags set.

3. Build the Vulkan-Loader repository and this one.

4. Build the runtime installer from this directory with the command:
   ```
   makensis InstallerRT.nsi -DLOADER64="?" -DLOADER32="?" -DVULKANINFO64="?" -DVULKANINFO32="?"
   ```
   where the question marks are replaced with the 64 and 32 bit versions of the loader and vulkaninfo builds.
