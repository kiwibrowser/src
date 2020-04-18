Build Validation Layers with Android CMake Plugin
=================================================
Gradle project in this directory builds layers into AAR.
The project could be directly added into application's gradle projects.
[Android Studio 3.0.0+](https://developer.android.com/studio/index.html)
IS required: earlier versions only publish release libs by default.

Pre-requirements
----------------
Build ShaderC binary
- Building from Github Repo source
1. cd android-build
2. ./update_external_sources_android.sh
3. ./android-generate.sh

Extra Steps if building from NDK's source tree
```
   cd ${your ANDROID_NDK_ROOT}/sources/third_party/shaderc
   ndk-build  APP_ABI=all APP_STL=c++_static NDK_TOOLCHAIN_VERSION=clang NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk libshaderc_combined
```

Adding layer module into Android Studio application project
--------------------------------------------------------
1. app's settings.gradle, add 
```
    include ':layerLib'
    project(':layerLib').projectDir = new File('/path/to/cmake/layerlib')
```
2. app's build.gradle:
```
dependencies {
    // Android Studio 3.0.0+ is required
    implementation project(':layerLib')
}
```
BUILD_IN_NDK variable in layerlib/CMakeLists.txt could detect whether the source is
from NDK or github repo clone, and would configure the file paths accordingly

Tested
-----
Build on Mac OS, tested on Google Pixel XL with Android Oreo.
Equvilaent build scripts for Windows OS are at the same directory.

