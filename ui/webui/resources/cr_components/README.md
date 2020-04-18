This directory contains complex Polymer web components for Web UI. They may be
shared between Settings, login, stand alone dialogs, etc.

These components are allowed to use I18nBehavior. The Web UI hosting these
components is expected to provide loadTimeData with any necessary strings.
TODO(stevenjb/dschuyler): Add support for i18n{} substitution.

These components may also use chrome and extension APIs, e.g. chrome.send
(through a browser proxy) or chrome.settingsPrivate. The C++ code hosting the
component is expected to handle these calls.

For simpler components with no I18n or chrome dependencies, see cr_elements.
