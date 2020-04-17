# Open Screen Library

The openscreen library implements the Open Screen Protocol.  Information about
the protocol and its specification can be found [on
GitHub](https://github.com/webscreens/openscreenprotocol).

## Getting the code

### Installing depot_tools

openscreen library dependencies are managed using `gclient`, from the
[depot_tools](https://www.chromium.org/developers/how-tos/depottools) repo.

To get gclient, run the following command in your terminal:
```bash
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Then add the `depot_tools` folder to your `PATH` environment variable.

Note that openscreen does not use other features of `depot_tools` like `repo` or
`drover`.  However, some `git-cl` functions *do* work, like `git cl try`, `git cl
lint` and `git cl upload.`

### Checking out code

From the parent directory of where you want the openscreen checkout, configure
`gclient` and check out openscreen with the following commands:

```bash
    gclient config https://chromium.googlesource.com/openscreen
    gclient sync
```

Now, you should have `openscreen/` repository checked out, with all dependencies
checked out to their appropriate revisions.

### Syncing your local checkout

To update your local checkout from the openscreen master repository, just run

```bash
   git pull
   gclient sync
```

This will rebase any local commits on the remote top-of-tree, and update any
dependencies that have changed.

## Building

### Installing build dependencies

Run `./tools/install-build-tools.sh` from the root source directory to obtain a
copy of the following build tools:

 - Build file generator: `gn`
 - Code formatter: `clang-format`
 - Builder: `ninja` ([GitHub releases](https://github.com/ninja-build/ninja/releases))

`gn`, `clang-format`, and `ninja` will be installed in the repository root,
which must be included in your `$PATH` or the build will fail.

You also need to ensure that you have the compiler toolchain dependencies.
Currently, both Linux and Mac OS X build configurations use clang by default.

### Linux

On Linux, the build will automatically download the Clang compiler from the
Google storage cache, the same way that Chromium does it.

Ensure that libstdc++ 8 is installed, as clang depends on the system
instance of it. On Debian flavors, you can run:

```bash
   sudo apt-get install libstdc++-8-dev
```

### Mac

On Mac OS X, the build will use the clang provided by XCode, which must be
installed.

### gcc support

Setting the `gn` argument "is_gcc=true" on Linux enables building using gcc
instead.  Note that g++ must be installed.

```bash
  gn gen out/Default --args="is_gcc=true"
```

### Debug build

Setting the `gn` argument "is_debug=true" enables debug build.

```bash
  gn gen out/Default --args="is_debug=true"
```

To install debug information for libstdc++ 8 on Debian flavors, you can run:

```bash
   sudo apt-get install libstdc++6-8-dbg
```

### gn configuration

Running `gn args` opens an editor that allows to create a list of arguments
passed to every invocation of `gn gen`.

```bash
  gn args out/Default
```

### Building the sample executable

The following commands will build a sample executable and run it.

``` bash
  mkdir out/Default
  gn gen out/Default          # Creates the build directory and necessary ninja files
  ninja -C out/Default demo   # Builds the executable with ninja
  ./out/Default/demo          # Runs the executable
```

The `-C` argument to `ninja` works just like it does for GNU Make: it specifies
the working directory for the build.  So the same could be done as follows:

``` bash
  ./gn gen out/Default
  cd out/Default
  ninja
  ./demo
```

After editing a file, only `ninja` needs to be rerun, not `gn`.  If you have
edited a `BUILD.gn` file, `ninja` will re-run `gn` for you.

For details on running `demo`, see its [README.md](demo/README.md).

### Building other targets

Running `ninja -C out/Default gn_all` will build all non-test targets in the
repository.

`gn ls --type=executable out/Default/` will list all of the executable targets
that can be built.

If you want to customize the build further, you can run `gn args out/Default` to
pull up an editor for build flags. `gn args --list out/Default` prints all of
the build flags available.

## Running unit tests

```bash
  ninja -C out/Default unittests
  ./out/Default/unittests
```

## Continuous build and try jobs

openscreen uses [LUCI builders](https://ci.chromium.org/p/openscreen/builders)
to monitor the build and test health of the library.  Current builders include:

| Name            | Arch   | OS                 | Toolchain | Build | Notes        |
|-----------------|--------|--------------------|-----------|-------|--------------|
| linux_debug     | x86-64 | Linux Ubuntu 16.04 | clang     | debug | ASAN enabled |
| linux_debug_gcc | x86-64 | Linux Ubuntu 16.04 | gcc       | debug | ASAN enabled |
| mac_debug       | x86-64 | Mac OS X/Xcode     | clang     | debug | |

You can run a patch through the try job queue (which tests it on all builders)
using `git cl try`, or through Gerrit (details below).

## Submitting changes

openscreen library code should follow the [Open Screen Library Style
Guide](docs/style_guide.md).

openscreen uses [Chromium Gerrit](https://chromium-review.googlesource.com/) for
patch management and code review (for better or worse).

The following sections contain some tips about dealing with Gerrit for code
reviews, specifically when pushing patches for review, getting patches reviewed,
and committing patches.

### Uploading a patch for review

The `git cl` tool handles details of interacting with Gerrit (the Chromium code
review tool) and is recommended for pushing patches for review.  Once you have
committed changes locally, simply run:

```bash
  git cl upload
```

This will run our `PRESUBMIT.sh` script to check style, and if it passes, a new
code review will be posted on `chromium-review.googlesource.com`.

If you make additional commits to your local branch, then running `git cl
upload` again in the same branch will merge those commits into the ongoing
review as a new patchset.

It's simplest to create a local git branch for each patch you want reviewed
separately.  `git cl` keeps track of review status separately for each local
branch.

### Addressing merge conflicts

If conflicting commits have been landed in the repository for a patch in review,
Gerrit will flag the patch as having a merge conflict.  In that case, use the
instructions above to rebase your commits on top-of-tree and upload a new
patchset with the merge conflicts resolved.

### Tryjobs

Clicking the `CQ DRY RUN` button (also, confusingly, labeled `COMMIT QUEUE +1`)
will run the current patchset through all LUCI builders and report the results.
It is always a good idea get a green tryjob on a patch before sending it for
review to avoid extra back-and-forth.

You can also run `git cl try` from the commandline to submit a tryjob.

### Code reviews

Send your patch to one or more committers in the
[COMMITTERS](https://chromium.googlesource.com/openscreen/+/refs/heads/master/COMMITTERS)
file for code review.  All patches must receive at least one LGTM by a committer
before it can be submitted.

### Submission

After your patch has received one or more LGTM commit it by clicking the
`SUBMIT` button (or, confusingly, `COMMIT QUEUE +2`) in Gerrit.  This will run
your patch through the builders again before committing to the main openscreen
repository.

## Chromium Build Differences

Currently, openscreen is also built in Chromium, with some build differences.
The files that are built are determined by the following build variables:
 - `build_with_chromium`: `true` when building as part of a Chromium checkout,
 `false` otherwise.  Set by `//build_overrides/build.gni`.
 - `use_mdns_responder`: `true` by default, `false` when `build_with_chromium`
 is `true`.  Controls whether the default mDNSResponder mDNS implementation is
 used.  Set by `//build/config/services.gni`.
 - `use_chromium_quic`: `true` by default, `false` when `build_with_chromium`
 is `true`.  Controls whether the Chromium QUIC implementation clone is used.
 Set by `//build/config/services.gni`.
