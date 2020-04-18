This directory is for features that are intended for reuse. For example, code
that is shared between multiple embedders of ios/web like ios/web_view and
ios/chrome. This means that components must not depend on either ios/web_view or
ios/chrome.

Code which can be shared across Chrome on all platforms should go in
//components (which also supports iOS specific sources).

Code in a component should be placed in a namespace corresponding to
the name of the component; e.g. for a component living in
//components/foo, code in that component should be in the foo::
namespace.
