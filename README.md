# Welcome to Kiwi Browser

Kiwi Browser is a fully open-source web browser for Android.

Kiwi is based on Chromium. Easily switch to Kiwi without having to painstakingly learn a new interface or break your existing browsing habits.

Among other functionalities, Kiwi Browser supports:

 - Night Mode (another implementation than Chromium)
 - Support for Chrome Extensions
 - Bottom address bar
It also includes performance improvements (partial rasterisation of tiles, etc)

The browser is licensed under the same licence as Chromium, which means that you are allowed to create derivatives of the browser.

### Timeline
15 April 2018 - First Kiwi Browser release.

15 April 2019 - Kiwi Browser gets support for Chrome Extensions.

17 April 2020 - Kiwi Browser goes fully open-source.


This code is up-to-date and is matching the build on the Play Store.

The new builds are done from the open-source edition directly to the Play Store.

There are thousands of hours of work in this repository and thousands of files changed.

### Contributors

Contributions are welcome and encouraged.

If you want your code to be integrated into Kiwi, open a merge request, I (and/or a member of the community) can review the code with you and push it to the Play Store.

### Modifications

If you create your own browser or a mod, make sure to change the browser name and icon in chrome/android/java/res_chromium/values/channel_constants.xml and translation strings (search and replace Kiwi in all *.xtb, all *.grd and all *.grdp files)


## How to build

The reference build machine is using Ubuntu 19.04 (tested also using Ubuntu 19.10).

Minimum system requirements is 2 vCPUs, 7.5 GB Memory.

You can use a virtual machine, or a Google Cloud VM.

### Getting the source-code and environment

To build Kiwi Browser you can directly clone the repository, as we have packed all dependencies already:

In ~ (your home directory) run:

    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

and edit the file ~/.bashrc to add at the very end

    export PATH=$HOME/depot_tools:$PATH
    
Validate the changes by running:

    source ~/.bashrc

This will give you access to one utility called gclient (as in "Google client")

Create a directory called ~/chromium/, and in this newly created directory run:

    git clone https://github.com/kiwibrowser/dependencies.git .cipd
    cp ~/chromium/.cipd/.gclient ~/chromium/
    cp ~/chromium/.cipd/.gclient_entries ~/chromium/

Enter ~/chromium/ and run:

    git clone https://github.com/kiwibrowser/src.git

At this stage, in ~/chromium/ you will have the .cipd folder, and a folder with the Kiwi Browser source-code called src.

### Setting-up dependencies

To be able to build Kiwi Browser, you need python and OpenJDK (OpenJDK to create Java bindings for Android):

    sudo apt-get install python openjdk-8-jdk-headless

then run the following commands:

    bash install-build-deps.sh --no-chromeos-fonts
    build/linux/sysroot_scripts/install-sysroot.py --arch=i386
    build/linux/sysroot_scripts/install-sysroot.py --arch=amd64

These commands will install all necessary system packages using apt-get and gather a minimal build filesystem.

### Preparing a signing key

APKs (application packages) on Android need to be signed by developers in order to be distributed.

To generate a key:

    keytool -genkey -v -keystore ~/chromium/keystore.jks -alias production -keyalg RSA -keysize 2048 -validity 10000 -keypass HERE_YOUR_ANDROID_KEYSTORE_PASSWORD

### Configuring the build type and platform

In ~/chromium/src/, create a folder named "android_arm" and in this folder create a file called args.gn with this content:

args.gn:

    target_os = "android"
    target_cpu = "arm" # <---- can be arm, arm64, x86 or x64
    is_debug = false
    is_java_debug = false
    
    android_channel = "stable"
    is_official_build = true
    is_component_build = false
    is_chrome_branded = false
    is_clang = true
    symbol_level = 1
    use_unofficial_version_number = false
    android_default_version_code = "158"
    android_keystore_name = "production"
    android_keystore_password = "HERE_YOUR_ANDROID_KEYSTORE_PASSWORD"
    android_keystore_path = "../../../keystore.jks"
    android_default_version_name = "Quadea"
    fieldtrial_testing_like_official_build = true
    icu_use_data_file = false
    enable_iterator_debugging = false

    google_api_key = "KIWIBROWSER"
    google_default_client_id = "42.apps.kiwibrowser.com"
    google_default_client_secret = "KIWIBROWSER_NOT_SO_SECRET"
    use_official_google_api_keys = true

    ffmpeg_branding = "Chrome"
    proprietary_codecs = true
    enable_hevc_demuxing = true
    enable_nacl = false
    enable_wifi_display = false
    enable_widevine = false
    enable_google_now = true
    enable_ac3_eac3_audio_demuxing = true
    enable_iterator_debugging = false
    enable_mse_mpeg2ts_stream_parser = true
    enable_remoting = false
    rtc_use_h264 = false
    rtc_use_lto = false
    use_openh264 = false
    
    v8_use_external_startup_data = true
    update_android_aar_prebuilts = true
    
    use_thin_lto = true
    
    enable_extensions = true
    enable_plugins = true


You can replace Android keystore password and Android keystore keypath with the data for your Android keystore (or you can generate a new key)

### Prepare the first build

To prepare initial setup run:

    gclient runhooks


then generate the build files:

    ~/chromium/src$ gn gen out/android_arm # (you can also use 'gn args out/android_arm')
    Writing build/secondary/third_party/android_tools/google_play_services_basement_java.info
    Writing build/secondary/third_party/android_tools/google_play_services_tasks_java.info
    Writing third_party/android_support_test_runner/rules_java.info
    Writing build/secondary/third_party/android_tools/google_play_services_base_java.info
    Writing     build/secondary/third_party/android_tools/google_play_services_auth_base_java.info
    [...]
    Done. Made 19910 targets from 1626 files in 21974ms


### Compiling Kiwi Browser

To compile, use the command:

    ninja -C out/android_arm chrome_public_apk

you'll have the output APK in ~/chromium/src/out/android_arm/apks/ChromePublic.apk

then you can run the APK on your phone.

### To investigate crashes
     components/crash/content/tools/generate_breakpad_symbols.py --build-dir=out/lnx64 --symbols-dir=/tmp/my_symbols/ --binary=out/android_arm/lib.unstripped/libchrome.so --clear --verbose

or from a tombstone:

     ~/chromium/src$ ./third_party/android_ndk/ndk-stack -sym out/android_x86/lib.unstripped -dump /home/raven/tombstone_06
==

### Remote debugging

You can use Google Chrome to debug using the devtools console.

In case the devtools console doesn't work (error 404),  the solution is to use chrome://inspect (Inspect fallback)
or change SHA1 in build/util/LASTCHANGE

    LASTCHANGE=8920e690dd011895672947112477d10d5c8afb09-refs/branch-heads/3497@{#948}

and confirm the change using:

     rm out/android_arm/gen/components/version_info/version_info_values.h out/android_x86/gen/components/version_info/version_info_values.h out/android_arm/gen/build/util/webkit_version.h out/android_x86/gen/build/util/webkit_version.h out/android_arm/gen/chrome/common/chrome_version.h out/android_x86/gen/chrome/common/chrome_version.h

### Binary size optimization

If you want to optimize of the final APK, you can look at the size of each individual component using command:

    ./tools/binary_size/supersize archive chrome.size --apk-file out/android_arm/apks/ChromePublic.apk -v
    ./tools/binary_size/supersize html_report chrome.size --report-dir size-report -v
==

If you need any assistance with building Kiwi, feel free to ask.
To do so, please open a GitHub issue so answers will benefit to everyone.

You can also reach out on Discord https://discord.gg/XyMppQq

Have fun with Kiwi!

Arnaud.

