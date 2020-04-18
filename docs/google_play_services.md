# Google Play services in Chrome for Android

[TOC]

Google employee? See [go/chrome-gms](https://goto.google.com/chrome-gms) for
more info.

## General setup

The Google Play services are a combination of [services][play_store] exposed on
Android devices and [libraries][dev_doc] to interact with them. Chrome relies
on them for critical features like Sign in, Feedback or Cast.

The standard way of adding the Google Play services as a dependency to a project
is to import it through the Android SDK manager as a Maven repository. That
repository contains multiple versions of the library split into separate APIs
(for example Cast, GCM, Android Pay, etc). To avoid downloading a lot of data we
don't need to build Chrome, android checkouts of Chromium download an archive
containing only the APIs we currently need in Chrome, and in a single version.

The up to date list of clients and version used can be seen in
[//build/android/play_services/config.json][config_json_rel_path].

**Note**: If you are working on a feature that requires different or more recent
APIs, you will need to locally download the Google Play services SDK repository.

The simplest way to download the latest SDK is to run:

```
$CHROMIUM_SRC/build/android/play_services/update.py sdk
```

Check out the help of that script for more info.

[play_store]: https://play.google.com/store/apps/details?id=com.google.android.gms
[dev_doc]: https://developers.google.com/android/guides/overview
[config_json_rel_path]: ../build/android/play_services/config.json

## Adding a dependency on new APIs

As explained above, the default checkout has access to only a specific set of
APIs during builds. If your CL depends on some APIs that are not included in the
build, you will need [file an issue][bug_link] to request an update of our
dependencies.

Not doing so could make the CL fail on the trybots and commit queue. Even if it
passes, it might fail on the internal bots and result in the CL getting
reverted, so please make sure the APIs are available to the bots before
submitting.

[bug_link]:https://bugs.chromium.org/p/chromium/issues/entry?labels=Restrict-View-Google,pri-1,Hotlist-GooglePlayServices&owner=agrieve@chromium.org&os=Android
