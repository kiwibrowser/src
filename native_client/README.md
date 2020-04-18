#  Native Client

Welcome to Native Client.  For the latest information about Native Client, see
the [Native Client project page](http://code.google.com/p/nativeclient).

##  Documentation

Most of the Native Client project documentation is available online:

 * Documentation for [Native Client users](http://gonacl.com)
 * Documentation for [contributors to Native
   Client](http://www.chromium.org/nativeclient)
 * [Research
   papers](http://www.chromium.org/nativeclient/reference/research-papers)

##  Directory structure

The following list describes major files and directories that you'll see in
your working copy of the repository, including some directories that don't
exist until you've built Native Client. Paths are relative to the
`native_client` directory.

 * `COPYING NOTICE README.md RELEASE_NOTES documentation/`: Documentation,
   release, and license information.

 * `SConstruct scons.bat scons scons-out/ site_scons/`: Build-related files.
   The `scons.bat` and `scons` files, with data from `SConstruct`, let you
   build Native Client and its tests. The `scons-out` and `site-scons`
   directories don't exist in the git repository; they're created when Native
   Client is built. The `scons-out/*/staging` directories contain files, such
   as the Native Client plug-in and compiled examples, that let you use and
   test Native Client.

 * `src/`: Core source code for Native Client.

 * `src/include/`: Header files that are missing from some platforms and are
   used by more than one major part of Native Client

 * `src/shared/`: Source code that's used by both trusted code (such as the
   service runtime) and untrusted code (such as Native Client modules)

 * `src/third_party`: Other people's source code

 * `src/trusted/`: Source code that's used only by trusted code

 * `src/untrusted/`: Source code that's used only by untrusted code

 * `tests/common/`: Source code for examples and tests.

 * `../third_party/`: Third-party source code and binaries that aren't part of
   the service runtime.  When built, the Native Client toolchain is in
   `src/third_party/nacl_sdk/`.

 * `tools/`: Utilities such as the plug-in installer.
