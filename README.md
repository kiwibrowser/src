# Kiwi Browser

![automatic build of apk](https://github.com/kiwibrowser/src/workflows/automatic%20build%20of%20apk/badge.svg)

## Overview

[Kiwi Browser](https://kiwibrowser.com/) is a fully open-source web browser for Android.

Kiwi is based on Chromium. Easily switch to Kiwi without having to painstakingly learn a new interface or break your existing browsing habits.

Among other functionalities, Kiwi Browser supports:

 - Night Mode (another implementation than Chromium)
 - Support for Chrome Extensions
 - Bottom address bar
It also includes performance improvements (partial rasterization of tiles, etc)

The browser is licensed under the same license as Chromium, which means that you are allowed to create derivatives of the browser.

Make sure to properly attribute the code to this repository (don't just replace with your name)

## Table of contents

- [Timeline](#timeline)
- [Contributing](#contributing)
- [Modifying](#modifying)
- [Building](#building)
  - [Getting the source-code and environment](#getting-the-source-code-and-environment)
  - [Setting-up dependencies](#setting-up-dependencies)
  - [Preparing a signing key](#preparing-a-signing-key)
  - [Configuring the build type and platform](#configuring-the-build-type-and-platform)
  - [Preparing the first build](#preparing-the-first-build)
  - [Compiling Kiwi Browser](#compiling-kiwi-browser)
  - [Investigating crashes](#investigating-crashes)
  - [Remote debugging](#remote-debugging)
  - [Optimizing binary size](#optimizing-binary-size)
- [Roadmap](#roadmap)
- [Additional help](#additional-help)

## Timeline

- 15 April 2018 - First Kiwi Browser release.

- 15 April 2019 - Kiwi Browser gets support for Chrome Extensions.

- 17 April 2020 - Kiwi Browser goes fully open-source.


This code is up-to-date and is matching the build on the Play Store.

The new builds are done from the open-source edition directly to the [Play Store](https://play.google.com/store/apps/details?id=com.kiwibrowser.browser).

There are thousands of hours of work in this repository and thousands of files changed.

## Contributing

Contributions are welcome and encouraged.

If you want your code to be integrated into Kiwi, open a merge request, I (and/or a member of the community) can review the code with you and push it to the Play Store.

## Modifying

If you create your own browser or a mod, make sure to change the browser name and icon in `chrome/android/java/res_chromium/values/channel_constants.xml` and translation strings (search and replace Kiwi in all `*.xtb`, all `*.grd` and all `*.grdp` files).
When replacing the app icon, make sure to add the new icon files in their respective `chrome/android/java/res/mipmap` folders(mdpi, hdpi etc) and also update the AndroidManifest.xml.

## Building

The reference build machine is using Ubuntu 19.04 (also tested using Ubuntu 18.04 and Ubuntu 19.10).

The minimum system requirements are 2 vCPUs, 7.5 GB Memory.

You can use a virtual machine, an AWS VM, or a Google Cloud VM.

### Getting the source-code and environment

To build Kiwi Browser you can directly clone the repository, as we have packed all dependencies already:

In ~ (your home directory) run:

```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

and edit the file ~/.bashrc to add at the very end

```bash
export PATH=$HOME/depot_tools:$PATH
```

Validate the changes by running:

```bash
source ~/.bashrc
```

This will give you access to one utility called gclient (as in "Google client")

Create a directory called ~/chromium/, and in ~/chromium/ run:

```bash
git clone https://github.com/kiwibrowser/dependencies.git .cipd
cp ~/chromium/.cipd/.gclient ~/chromium/
cp ~/chromium/.cipd/.gclient_entries ~/chromium/
git clone https://github.com/kiwibrowser/src.git
```

At this stage, in ~/chromium/ you will have the .cipd folder, and a folder with the Kiwi Browser source-code called src.

### Setting up dependencies

To be able to build Kiwi Browser, you need python and OpenJDK (OpenJDK to create Java bindings for Android):

```bash
sudo apt-get update
sudo apt-get install python openjdk-8-jdk-headless libncurses5
```

We want to be sure to use Java 1.8 in order to not get compilation errors (lint and errorprone):

```bash
sudo update-java-alternatives --set java-1.8.0-openjdk-amd64
```

then run the following commands in ~/chromium/src:

```bash
bash install-build-deps.sh --no-chromeos-fonts
build/linux/sysroot_scripts/install-sysroot.py --arch=i386
build/linux/sysroot_scripts/install-sysroot.py --arch=amd64
```

These commands will install all necessary system packages using apt-get and gather a minimal build filesystem.

### Preparing a signing key

APKs (application packages) on Android need to be signed by developers in order to be distributed.

To generate a key:

```bash
keytool -genkey -v -keystore ~/chromium/keystore.jks -alias production -keyalg RSA -keysize 2048 -validity 10000 -storepass HERE_YOUR_ANDROID_KEYSTORE_PASSWORD -keypass HERE_YOUR_ANDROID_KEYSTORE_PASSWORD
```

### Configuring the build type and platform

Run:

```bash
mkdir -p ~/chromium/src/out/android_arm
```

Create a file called args.gn in ~/chromium/src/out/android_arm/ with this content:

```bash
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
```

You can replace Android keystore password and Android keystore keypath with the data for your Android keystore (or you can generate a new key).

### Preparing the first build

To prepare initial setup run from ~/chromium/src:

```
gclient runhooks
```

then generate the build files in ~/chromium/src:

```
gn gen out/android_arm
```

Alternatively you can use: gn args out/android_arm

### Compiling Kiwi Browser

To compile, use the command:

```
ninja -C out/android_arm chrome_public_apk
```

you'll have the output APK in ~/chromium/src/out/android_arm/apks/ChromePublic.apk

then you can run the APK on your phone.

### Investigating crashes

You need to have the symbols for the version that crashed, the symbols can be generated using:
```
components/crash/content/tools/generate_breakpad_symbols.py --build-dir=out/lnx64 --symbols-dir=/tmp/my_symbols/ --binary=out/android_arm/lib.unstripped/libchrome.so --clear --verbose
```

If you have the crash information from logcat:
```
out/lnx64/microdump_stackwalk -s /tmp/dump.dmp /tmp/my_symbols/
```

If you have the crash information in a tombstone:
```
./third_party/android_ndk/ndk-stack -sym out/android_x86/lib.unstripped -dump tombstone
```

### Remote debugging

You can use Google Chrome to debug using the devtools console.

In case the devtools console doesn't work (error 404),  the solution is to use chrome://inspect (Inspect fallback)
or change SHA1 in build/util/LASTCHANGE

```
LASTCHANGE=8920e690dd011895672947112477d10d5c8afb09-refs/branch-heads/3497@{#948}
```

and confirm the change using:

```
rm out/android_arm/gen/components/version_info/version_info_values.h out/android_x86/gen/components/version_info/version_info_values.h out/android_arm/gen/build/util/webkit_version.h out/android_x86/gen/build/util/webkit_version.h out/android_arm/gen/chrome/common/chrome_version.h out/android_x86/gen/chrome/common/chrome_version.h
```

### Optimizing binary size

If you want to optimize of the final APK, you can look at the size of each individual component using command:

```
./tools/binary_size/supersize archive chrome.size --apk-file out/android_arm/apks/ChromePublic.apk -v
./tools/binary_size/supersize html_report chrome.size --report-dir size-report -v
```

## Precompiled binaries

<a href="https://play.google.com/store/apps/details?id=com.kiwibrowser.browser"> <img src="https://camo.githubusercontent.com/59c5c810fc8363f8488c3a36fc78f89990d13e99/68747470733a2f2f706c61792e676f6f676c652e636f6d2f696e746c2f656e5f75732f6261646765732f696d616765732f67656e657269632f656e5f62616467655f7765625f67656e657269632e706e67" height="55">

## Business model

The browser is getting paid by search engines for every search done using Kiwi Browser.

Depending on the search engine choice, requests may go via Kiwibrowser / Kiwisearchservices servers.
This is for invoicing our search partners and provide alternative search results (e.g. bangs aka "shortcuts").

In some countries, the browser displays sponsored tiles or news on the homepage.

User data (browsing, navigation, passwords, accounts) is not collected because we have no interest to know what you do in the browser. Our main goal is to convince you to use a search engine partner, and this search engine makes money / new partnerships and shares revenue with us.

## Roadmap

* During year 2020, the goal of the project is to make maintenance fixes and security updates.

If there is an issue or bug that you want to be included to Kiwi, please open an issue ticket pointing to the related Chromium bug or commit. Be precise, there are dozen of thousands of changes in Chromium.

* During 2021, Kiwi Browser will switch to a new branch called Kiwi Browser Next with a quite automated Chromium rebasing system.

## Additional help

You can ask for extra help in our Discord server, or by [filing an issue](https://github.com/kiwibrowser/src/issues).

<a href="https://discord.gg/XyMppQq"> <img src="https://discordapp.com/assets/e4923594e694a21542a489471ecffa50.svg" height="50"></a>

Have fun with Kiwi!

Arnaud.
