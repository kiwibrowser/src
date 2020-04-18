# Policy indicators

Settings that can't be controlled by the current user often show an icon and a
tooltip explaining why. This happens when a setting is:

* enforced by user policy, or different from a policy's "recommended" value
* overridden by an extension
* or (on Chrome OS):
    * enforced/recommended by device policy (for enrolled devices)
    * set by the device owner (for non-enrolled devices)
    * controlled by the primary user (for multiple profile sessions)

## Indicator UI

The badge icons are sourced from [cr_elements/icons.html] by default.

Indicators show a tooltip with explanatory text on hover if `CrPolicyStrings`
is set; see [settings_ui.js] for an example from MD Settings.

## Using an indicator

Elements like `<cr-policy-indicator>` and `<cr-policy-pref-indicator>` are
provided to be reused in WebUI pages:

    <cr-policy-indicator indicator-type="userPolicy"></cr-policy-indicator>

Example: [settings-checkbox].

For one-off or composed elements, `CrPolicyIndicatorBehavior` provides some
configurable properties and calculates dependent properties, such as the
tooltip, icon, and visibility of the indicator.

Example: [cr_policy_pref_indicator.js] overrides `indicatorType` and
`indicatorTooltip`. [cr_policy_pref_indicator.html] displays the computed
properties from `CrPolicyIndicatorBehavior`.


[cr_elements/icons.html]: ../icons.html
[settings_ui.js]: /chrome/browser/resources/settings/settings_ui/settings_ui.js
[settings-checkbox]: /chrome/browser/resources/settings/controls/settings_checkbox.html
[cr_policy_pref_indicator.js]: cr_policy_pref_indicator.js
[cr_policy_pref_indicator.html]: cr_policy_pref_indicator.html
