# Modularisation proposal

_Draft_

I propose that we modularise the build system for NaCl so that it has the
following properties, based on what I outlined in a [blog post]
(http://lackingrhoticity.blogspot.com/2009/12/modular-vs-monolithic-build-systems.html):

1.  The build consists of a set of modules.
2.  Each module can be built separately.
3.  Each module's build produces some output (a directory tree).
4.  A module may depend on the outputs of other modules, but it can't reach
    inside the others' build trees.
5.  There is a common interface that each module provides for building itself.
6.  The build tool can be replaced with another. The description of the module
    set is separate from the modules themselves.

In practice, this means that each module should: 1. Locate its dependencies
through environment variables and explicitly-passed arguments. Modules should
not reach outside their source/build directories to find dependencies using
relative pathnames. (However, for practical reasons, using some system-installed
dependencies, i.e. programs in /usr, is OK.) 1. Support `--prefix` (or an
equivalent). 1. Support build dirs that are separate from source dirs (not
essential, but preferable). 1. Support the `DESTDIR` argument to `make install`
or an equivalent. (If `DESTDIR` is missing, we can work around this using
`--prefix`, but it's a pain.)

We contrast modular build systems to monolithic build systems. The main
characteristic of a monolithic build is that any part of the build can reference
any other part via relative filenames.

## Applicability to Native Client

There are three areas where we might wish to apply this: * The toolchain:
binutils, gcc (pre-gcc and the full gcc), newlib, libnacl, pthread library, etc.
This is currently built by tools/Makefile, which invokes Scons. There is a
separate script for building the ARM toolchain. * The trusted codebase:
sel\_ldr, ncval, NPAPI plugin. Built by Scons. * Package collection
(native\_client\_sdk in SVN). Built by shell scripts. (The toolchain and package
collection are both sometimes referred to as "the SDK", but I'm using different
terms here in order to distinguish them.)

In the long term, the package collection is the most important part to
modularise, because we expect it to become large and take a long time to build.
However, the package collection is less of a priority in the short term because
fewer people are working on it. Modularising the NaCl Scons build is less of a
priority because it builds relatively quickly. Therefore I am focusing initially
on modularising the toolchain build.

The toolchain build takes a relatively long time, and covering multiple
architectures makes this worse. Currently our Buildbot-based continuous
integration system rebuilds the toolchain in a cycle that is separate from the
main build. The trybots don't rebuild the toolchain because that would take too
long for most changes; the trybots cannot tell what parts of the toolchain need
to be rebuilt. This means the trybots are not very useful for testing toolchain
changes. This situation could be improved if the toolchain could be rebuilt
incrementally.

## Prototype

I have written a prototype replacement for tools/Makefile.

*   I have put the code here for now:
    http://github.com/mseaborn/nacl-modular-build
*   To get the code you can do: `git clone
    git://github.com/mseaborn/nacl-modular-build.git`

This is now in the NaCl tree, under [tools/modular-build]
(http://code.google.com/p/nativeclient/source/browse/trunk/src/native_client/tools/modular-build).

## Pain points

*   nacl-newlib expects to get NaCl header files from its source tree. This
    means tools/Makefile must copy the header files into the newlib source tree.
    This violates the modular build model. It is better if we can separate
    source directories from build directories, so that source directories are
    not modified during the build.

    *   TODO: Add a `--with-headers=path` option similar to what glibc provides.

*   nacl-newlib installs 32-bit libraries into lib/32, and tools/Makefile works
    around this to make them visible in lib32.

*   NaCl's Scons build uses "naclsdk\_mode=custom:..." as both an input tree and
    an output tree for nacl\_extra\_sdk. build.py tries to use this argument to
    specify the output tree only (it passes the path of an empty directory). We
    could tell scons how to find nacl-gcc by setting PATH instead. Scons resets
    PATH by default, but we disable this by passing USE\_ENVIRON=1 (an option
    that was added for this purpose).

*   Is there a way to tell Scons to use a build directory other than
    `scons-out`? We need a way to ensure that Scons-built files are rebuilt
    whenever nacl-gcc is rebuilt.

*   The toolchain patches now require `-p2` to apply. (This is a minor point,
    but it is a divergence from the usual practice.)

## Wishlist

Missing features in the modular build widely varies by source. Some features can
be easily added by implementing Makefile-like functionality that is just not
there yet. Some other features might require an in-depth rewrite of important
modular-build concepts. The list is **not** sorted by importance.

*   The generic part of the build should be better documented. Take
    `AddAutoconfModule` as example. It accepts arguments through `**kwargs` that
    are not easy to follow by reading sources. Examples of parameters with
    somewhat cryptic names: `explicitly_passed_deps`, `use_install_root`. The
    directory structure of the build must be documented too. What is the concept
    of 'install-prefix' and how is it different from 'install'?

*   The build should support building from sources prepared by a human as
    opposed to be checking out sources from known HTTP locations automatically.
    I.e. it should be possible to fetch all sources somehow, then modular-build
    them. In the current implementation one has to make a full build first to
    fetch all sources, then remove everything except `out/source`, replace with
    my own sources, then rebuild. This could be solved by having all source
    targets grouped under a single target.

*   Document incremental rebuild options. Currently it is hard to see a result
    of changing something in gcc sources by rebuilding the whole toolchain
    incrementally. Should an `out/something` directory be removed or should
    `build.py` be invoked with some parameters to invoke an incremental rebuild?
    Ideally it should allow asking modules if they can incrementally rebuild by
    themselves. This does not always have to be clean in terms of the final
    correctness. Sometimes a developer wants to rebuild just one component (or
    only those components that changed) and install the results to the
    destination plus maybe rebuild (some) dependent components. In this
    situation a developer knowingly sacrifices correctness in favor of faster
    build.

*   The build should print all parameters of the scripts invoked by system()
    (make, configure). It is significantly harder to debug problems if all
    invocations are not logged properly.

*   The build should log file operations that it performs. Currently all ad hoc
    operations performed are not logged which makes it hard to understand
    sources of many problems. For example, `treemappers` that manipulate file
    trees should record their file operations in the log. Preferably into
    something like 'cp -r A/ B', not something like 'cp A/1 A/2 A/3 A/4/4 B'.
    Once run on buildbots this should make it easier to debug problems.

*   Parallel build option analogous to -j in GNU Make.

*   Reduce the amount of copies of the same file. This is probably unfixable.
    For example, there are 3 places where sys/nacl\_imc\_api.h is being copied
    during the build: `out/headers`, `out/headers/nacl/abi`,
    `out/input-prefix/glibc_toolchain/nacl/include` (the tools/Makefile build
    has only one replica). This makes it harder for a novice user to follow
    where this file comes from and whether it changes on the way.

*   The build should improve the readability of it's rules. It shold look rather
    as a static datafile description. Currently it is a complex program
    `build.py` that has loops, conditionals `os.path.join`, `os.path.abspath`
    trickery, etc.

## See also

*   [Releasing FLOSS Software](http://www.dwheeler.com/blog/2009/04/13/), David
    A. Wheeler
