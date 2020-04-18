## Feature Policy Guide
### How to add a new feature to feature policy

Feature policy (see [spec](https://wicg.github.io/feature-policy/)) is a
mechanism that allows developers to selectively enable and disable various
[browser features and
APIs](https://cs.chromium.org/chromium/src/third_party/blink/public/mojom/feature_policy/feature_policy.mojom)
(e.g, "vibrate", "fullscreen", "usb", etc.). A feature policy can be defined
via a HTTP header and/or an iframe "allow" attribute.

Below is an example of a header policy (note that the header should be kept in
one line, split into multiple for clarity reasons):

    Feature-Policy: vibrate 'none'; geolocation 'self' https://example.com; camera *


- `vibrate` is disabled for all browsing contexts;
- `geolocation` is disabled for all browsing contexts except for its own
  origin and those whose origin is "https://example.com";
- `camera` is enabled for all browsing contexts.

Below is an example of a container policy:

    <iframe allowpaymentrequest allow='vibrate; fullscreen'></iframe>

OR

    <iframe allowpaymentrequest allow="vibrate 'src'; fullscreen 'src'"></iframe>


- `payment` is enabled (via `allowpaymentrequest`) on all browsing contexts
 within the iframe;
- `vibrate` and `fullscreen` are enabled on the origin of the URL of the
  iframe's `src` attribute.

Combined with a header policy and a container policy, [inherited
policy](https://wicg.github.io/feature-policy/#inherited-policy) defines the
availability of a feature.
See more details for how to [define an inherited policy for
feature](https://wicg.github.io/feature-policy/#define-inherited-policy)

#### Adding a new feature to feature policy
A step-to-step guide with examples.

##### Shipping features behind a flag
There are currently two runtime-enabled flags: `FeaturePolicy` (status:
stable) and `FeaturePolicyExperimentalFeatures` (status: experimental).
If the additional feature is unshipped, or if the correct behaviour with feature
policy is undetermined, consider shipping the feature behind a flag (i.e.,
`FeaturePolicyExperimentalFeatures`).

##### Define new feature
1. Feature policy features are defined in
`third_party/blink/public/common/feature_policy/feature_policy_feature.h`. Add the new feature
enum with a brief decription about what the feature does in the comment, right
above `LAST_FEATURE`

2. Append the new feature enum with a brief description as well in
`third_party/blink/public/mojom/feature_policy/feature_policy.mojom`

3. Update `third_party/blink/public/mojom/feature_policy/feature_policy.mojom_traits.h`
to include the new feature

4. Update `third_party/blink/renderer/platform/feature_policy/feature_policy.cc`:
Add your `("feature-name", FeatureEnumValue)` mapping to
  `GetDefaultFeatureNameMap()` (note: "feature-name" is the string web
  developers will be using to define the policy in the HTTP header and iframe
  "allow" attribute).
+ If shipping behind the flag (`FeaturePolicyExperimentalFeatures`):
Add the mapping inside the `if
(RuntimeEnabledFeatures::FeaturePolicyExperimentalFeaturesEnabled())`
stament;
+ Otherwise:
Add the mapping above the if statment.

##### Integrate the feature behaviour with feature policy
1. Add a case for the feature in `IsSupportedInFeaturePolicy()` (which checks
if feature policy is enabled and the feature is supported in feature policy):
- If shipping behind the flag (`FeaturePolicyExperimentalFeatures`):

    return RuntimeEnabledFeatures::FeaturePolicyExperimentalFeaturesEnabled();

- Otherwise:

    return true;


2. Implement the behaviour of the new feature for the 3 cases:
- Default behaviour without feature policy
i.e,

    if (!IsSupportedInFeaturePolicy(...)) {
      ...
    }

- When feature policy is enabled and feature is enabled by feature policy;
i.e,

    if (!IsSupportedInFeaturePolicy(...)) {
      if (frame->IsFeatureEnabled(...)) {
        ...
      }
    }

- When feature policy is enabled and feature is disabled by feature policy.
i.e,

    if (IsSupportedInFeaturePolicy(...)) {
      if (!frame->IsFeatureEnabled(...)) {
        ...
      }
    }


3. Examples:
- `vibrate`: `NavigatorVibration::vibrate()`
- `payment`: `AllowedToUsePaymentRequest()`
- `usb`: `USB::getDevices()`

##### Write web-platform-tests
To test the new feature with feature policy, refer to
`third_party/WebKit/LayoutTests/external/wpt/feature-policy/README.md` for
instructions on how to use the feature policy test framework.

#### Contacts
For more questions, please feel free to reach out to:
loonybear@chromium.org
iclelland@chromium.org
