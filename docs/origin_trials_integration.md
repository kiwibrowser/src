# Integrating a feature with the Origin Trials framework

To expose your feature via the origin trials framework, there are a few code
changes required.

[TOC]

## Code Changes

### Runtime Enabled Features

First, you’ll need to configure [runtime\_enabled\_features.json5]. This is
explained in the file, but you use `origin_trial_feature_name` to associate your
runtime feature flag with a name for your origin trial.  The name can be the
same as your runtime feature flag, or different.  Eventually, this configured
name will be used in the Origin Trials developer console (still under
development). You can have both `status: experimental` and
`origin_trial_feature_name` if you want your feature to be enabled either by
using the `--enable-experimental-web-platform-features` flag **or** the origin
trial.

You may have a feature that is not available on all platforms, or need to limit
the trial to specific platforms. Use `origin_trial_os: [list]` to specify which
platforms will allow the trial to be enabled. The list values are case-
insensitive, but must match one of the defined `OS_<platform>` macros (see
[build_config.h]).

#### Examples

Flag name and trial name are the same:
```
{
  name: "MyFeature",
  origin_trial_feature_name: "MyFeature",
  status: "experimental",
},
```
Flag name and trial name are different:
```
{
  name: "MyFeature",
  origin_trial_feature_name: "MyFeatureTrial",
  status: "experimental",
},
```
Trial limited to specific platform:
``` json
{
  name: "MyFeature",
  origin_trial_feature_name: "MyFeature",
  origin_trial_os: ["android"],
  status: "experimental",
},
```

### Gating Access

Once configured, there are two mechanisms to gate access to your feature behind
an origin trial. You can use either mechanism, or both, as appropriate to your
feature implementation.

1. A native C++ method that you can call in Blink code at runtime to expose your
    feature: `bool OriginTrials::myFeatureEnabled()`
2. An IDL attribute \[[OriginTrialEnabled]\] that you can use to automatically
    generate code to expose and hide JavaScript methods/attributes/objects. This
    attribute works very similarly to \[RuntimeEnabled\].
```
[OriginTrialEnabled=MyFeature]
partial interface Navigator {
     readonly attribute MyFeatureManager myFeature;
}
```

**NOTE:** Your feature implementation must not persist the result of the enabled
check. Your code should simply call `OriginTrials::myFeatureEnabled()` as often
as necessary to gate access to your feature.

## Limitations

What you can't do, because of the nature of these Origin Trials, is know at
either browser or renderer startup time whether your feature is going to be used
in the current page/context. This means that if you require lots of expensive
processing to begin (say you index the user's hard drive, or scan an entire city
for interesting weather patterns,) that you will have to either do it on browser
startup for *all* users, just in case it's used, or do it on first access. (If
you go with first access, then only people trying the experiment will notice the
delay, and hopefully only the first time they use it.). We are investigating
providing a method like `OriginTrials::myFeatureShouldInitialize()` that will
hint if you should do startup initialization.  For example, this could include
checks for trials that have been revoked (or throttled) due to usage, if the
entire origin trials framework has been disabled, etc.  The method would be
conservative and assume initialization is required, but it could avoid expensive
startup in some known scenarios.

Similarly, if you need to know in the browser process whether a feature should
be enabled, then you will have to either have the renderer inform it at runtime,
or else just assume that it's always enabled, and gate access to the feature
from the renderer.

## Testing

If you want to test your code's interactions with the framework, you'll need to
generate some tokens of your own. To generate your own tokens, use
[generate_token.py]. You can generate signed tokens for localhost, or for
127.0.0.1, or for any origin that you need to help you test. For example:

```
tools/origin_trials/generate_token.py http://localhost:8000 MyFeature
```

The file `tools/origin_trials/eftest.key` is used by default as the private key
for the test keypair used by Origin Trials unit tests and layout tests (i.e. in
content shell). Tokens generated with this key will **not** work in the browser
by default (see the [Developer Guide] for instructions on creating real tokens).
To use a test token with the browser, run Chrome with the command-line flag:

```
--origin-trial-public-key=dRCs+TocuKkocNKa0AtZ4awrt9XKH2SQCI6o4FY6BNA=
```

This is the base64 encoding of the public key associated with `eftest.key`. If
it doesn't work, see [trial_token_unittest.cc]. If you cannot set command-line
switches (e.g., on Chrome OS), you can also directly modify
[chrome_origin_trial_policy.cc].

### Layout Tests
When using the \[OriginTrialEnabled\] IDL attribute, you should add layout tests
to verify that the V8 bindings code is working as expected. Depending on how
your feature is exposed, you'll want tests for the exposed interfaces, as well
as tests for script-added tokens. For examples, refer to the existing tests in
[origin_trials/webexposed].

[build_config.h]: /build/build_config.h
[chrome_origin_trial_policy.cc]: /chrome/common/origin_trials/chrome_origin_trial_policy.cc
[generate_token.py]: /tools/origin_trials/generate_token.py
[Developer Guide]: https://github.com/jpchase/OriginTrials/blob/gh-pages/developer-guide.md
[OriginTrialEnabled]: /third_party/blink/renderer/bindings/IDLExtendedAttributes.md#_OriginTrialEnabled_i_m_a_c_
[origin_trials/webexposed]: /third_party/WebKit/LayoutTests/http/tests/origin_trials/webexposed/
[runtime\_enabled\_features.json5]: /third_party/blink/renderer/platform/runtime_enabled_features.json5
[trial_token_unittest.cc]: /third_party/blink/common/origin_trials/trial_token_unittest.cc
