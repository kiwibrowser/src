//ash/components
----------------
This directory is for medium-sized UI "components" or "modules" or "mini-apps"
that run on Chrome OS. In the long term, some of these mini-apps may become
their own mojo applications and run independently, instead of as part of the
browser or as part of ash.

Generally these mini-apps depend on //base, //chromeos, //ui and low-level
components like //components/prefs.

Code here should not depend on //ash, except for the public mojo IPC interfaces
in //ash/public. Likewise, //ash should not depend on these components, except
for possibly for a header that allows launching the mini-app.

If the mini-app contains webui it might depend on //content, but in general
//content dependencies should be avoided.

Code in //ash/components/foo should be in "namespace foo" not "namespace ash".

//ash is only used on Chrome OS. If a component is used on other platforms the
code should move to //components.
