## Browsing the source code

If you just want to look at the source code, you can either
[browse](https://chromium.googlesource.com/native_client/src/native_client/) it
online or download it with git. Here's how to download the latest version of the
source code:

    git clone https://chromium.googlesource.com/native_client/src/native_client

If you use `git clone`, the source code appears under a directory named
`native_client` in the current directory. You can update it using `git pull`.

## Getting buildable source code

If you want to build the latest version of Native Client, follow these steps:

1. If you don't already have `gclient`, get it by downloading the Chromium
   [depot tools](http://dev.chromium.org/developers/how-tos/install-depot-tools).
2. If you don't already have [git](http://git-scm.com/downloads), download it.
3. Create a directory to hold the Native Client source code. We'll
   call it `$NACL_ROOT`.
4. **Important:** Make sure the path to the directory has no spaces. Examples of
   good `$NACL_ROOT` values:
   - `/home/me/nativeclient` *(Linux)*
   - `/Users/me/nativeclient` *(Mac)*
   - `C:\nativeclient` *(Windows)*
5. In a shell window, execute the following commands:

        cd $NACL_ROOT
        gclient config https://chromium.googlesource.com/native_client/src/native_client
        cd $NACL_ROOT
        gclient sync

Here's what the `gclient` steps do:

1. `gclient config` sets up your working directory, creating a `.gclient` file
   that identifies the structure to pull from the repository.
2. `gclient sync` creates several subdirectories and downloads the latest Native
    Client files.

To update the Native Client source code, run `gclient sync` from `$NACL_ROOT` or
any of its subdirectories.  Once you've downloaded the source code, you can
build Native Client. To do so, you need Python and a platform-specific
development environment. Details on prerequisites and how to build are in
[Building Native
Client](href='http://www.chromium.org/nativeclient/how-tos/build-tcb).
