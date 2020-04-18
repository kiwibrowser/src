Ash
---
Ash is the "Aura Shell", the window manager and system UI for Chrome OS.
Ash uses the views UI toolkit (e.g. views::View, views::Widget, etc.) backed
by the aura native widget and layer implementations.

Ash sits below chrome in the dependency graph (i.e. it cannot depend on code
in //chrome). It has a few dependencies on //content, but these are isolated
in their own module in //ash/content. This allows targets like ash_unittests
to build more quickly.

Tests
-----
Most tests should be added to the ash_unittests target. Tests that rely on
//content should be added to ash_content_unittests, but these should be rare.

Tests can bring up most of the ash UI and simulate a login session by deriving
from AshTestBase. This is often needed to test code that depends on ash::Shell
and the controllers it owns.

Test support code (TestFooDelegate, FooControllerTestApi, etc.) lives in the
same directory as the class under test (e.g. //ash/foo rather than //ash/test).
Test code uses namespace ash; there is no special "test" namespace.

Mustash
----------
Ash is transitioning to run as a mojo service in its own process. This change
means that code in chrome cannot call into ash directly, but must use the mojo
interfaces in //ash/public/interfaces.

Out-of-process Ash is referred to as "mash" (mojo ash). In-process ash is
referred to as "classic ash". Ash can run in either mode depending on the
--enable-features=Mash command line flag.

In the few cases where chrome code is allowed to call into ash (e.g. code that
will only ever run in classic ash) the #include lines have "// mash-ok"
appended. This makes it easier to use grep to determine which parts of chrome
have not yet been adapted to mash.

Ash used to support a "mus" mode that ran the mojo window service from
//services/ui on a background thread in the browser process. This configuration
was deprecated in April 2018.

Mustash Tests
-----
ash_unittests --enable-features=Mash runs in mash mode. Some tests will fail
because the underlying code has not yet been ported to work with mash. We use
filter files to skip these tests, because it makes it easier to run the entire
suite without the filter to see what passes.

To simulate what the bots run (e.g. to check if you broke an existing test that
works under mash) you can run:

`ash_unittests --enable-features=Mash --test-launcher-filter-file=testing/buildbot/filters/mash.ash_unittests.filter`

There is a similar filter file for browser_tests --enable-features=Mash.

Any new feature you add (and its tests) should work under mash. If your test
cannot pass under mash due to some dependency being broken you may add the test
to the filter file. Make sure there is a bug for the underlying issue and cite
it in the filter file.

Prefs
-----
Ash supports both per-user prefs and device-wide prefs. These are called
"profile prefs" and "local state" to match the naming conventions in chrome. Ash
also supports "signin screen" prefs, bound to a special profile that allows
users to toggle features like spoken feedback at the login screen.

Local state prefs are loaded asynchronously during startup. User prefs are
loaded asynchronously after login, and after adding a multiprofile user. Code
that wants to observe prefs must wait until they are loaded. See
ShellObserver::OnLocalStatePrefServiceInitialized() and
SessionObserver::OnActiveUserPrefServiceChanged(). All PrefService objects exist
for the lifetime of the login session, including the signin prefs.

Pref names are in //ash/public/cpp so that code in chrome can also use the
names. Prefs are registered in the classes that use them because those classes
have the best knowledge of default values.

All PrefService instances in ash are backed by the mojo preferences service.
This means an update to a pref is asynchronous between code in ash and code in
chrome. For example, if code in chrome changes a pref value then immediately
calls a C++ function in ash, that ash function may not see the new value yet.
(This pattern only happens in the classic ash configuration; code in chrome
cannot call directly into the ash process in the mash config.)

Prefs are either "owned" by ash or by chrome browser. New prefs used by ash
should be owned by ash. See NightLightController and LogoutButtonTray for
examples of ash-owned prefs. See //services/preferences/README.md for details of
pref ownership and "foreign" prefs.

Historical notes
----------------
Ash shipped on Windows for a couple years to support Windows 8 Metro mode.
Windows support was removed in 2016.
