# Welcome to Kiwi Browser

Kiwi Browser is a fully open-source web browser for Android.

Kiwi is based on Chromium, the engine that powers the most popular browser in the world so you won't loose your habits.

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


### How to build

The reference build machine is using Ubuntu 19.04

You can use a virtual machine, or a Google Cloud VM.

To build Kiwi Browser you can directly clone the repository, as we have packed all dependencies already:

1. Install depot_tools in your home directory using git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git and add it to the PATH: export PATH=/path/to/depot_tools:$PATH - This will give you access to one utility called gclient (as in "Google client")

2. In a folder called ~/chromium/, fetch the dependencies (git LFS repository): git clone https://github.com/kiwibrowser/dependencies.git .cipd (do not forget the .cipd) and then copy ~/chromium/.cipd/.gclient and ~/chromium/.cipd/.gclient_entries to ~/chromium/

3. From ~/chromium/ folder, fetch the main source-code: git clone https://github.com/kiwibrowser/src.git

In ~/chromium/ you will have the .cipd folder, and a folder with the Kiwi Browser source-code called src.

4. In the Kiwi Browser source-code directory, run install-build-deps.sh using: bash install-build-deps.sh

5. In the Kiwi Browser source-code directory, create a android_arm folder and create a folder called android_arm/args.gn with this content:

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
    android_keystore_path = "keystore.jks"
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


Replace Android keystore password and Android keystore keypath with the data for your Android keystore (or you can generate a new key)

To prepare initial setup run:

    gclient runhooks


then generate the build files:

    ~/chromium/opensourcebuild$ gn gen out/android_arm (or gen args out/android_arm)
    Writing build/secondary/third_party/android_tools/google_play_services_basement_java.info
    Writing build/secondary/third_party/android_tools/google_play_services_tasks_java.info
    Writing third_party/android_support_test_runner/rules_java.info
    Writing build/secondary/third_party/android_tools/google_play_services_base_java.info
    Writing     build/secondary/third_party/android_tools/google_play_services_auth_base_java.info
    [...]
    Done. Made 19910 targets from 1626 files in 21974ms


===

To compile, use the command:
ninja -C out/android_arm chrome_public_apk

you'll have the output APK in ./out/android_arm/apks/ChromePublic.apk

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

and

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

