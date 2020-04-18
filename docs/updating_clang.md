# Updating clang

1.  Sync your Chromium tree to the latest revision to pick up any plugin
    changes
1.  Run `python tools/clang/scripts/upload_revision.py NNNN`
    with the target LLVM SVN revision number. This creates a roll CL on a new
    branch, uploads it and starts tryjobs that build the compiler binaries into
    a staging bucket on Google Cloud Storage (GCS).
1.  If the clang upload try bots succeed, copy the binaries from the staging
    bucket to the production one. For example:

    ```shell
    $ export rev=123456-1
    $ for x in Linux_x64 Mac Win ; do \
        gsutil.py cp -n -a public-read gs://chromium-browser-clang-staging/$x/clang-$rev.tgz \
            gs://chromium-browser-clang/$x/clang-$rev.tgz ; \
        gsutil.py cp -n -a public-read gs://chromium-browser-clang-staging/$x/llvmobjdump-$rev.tgz \
            gs://chromium-browser-clang/$x/llvmobjdump-$rev.tgz ; \
        gsutil.py cp -n -a public-read gs://chromium-browser-clang-staging/$x/llvmcfiverify-$rev.tgz \
            gs://chromium-browser-clang/$x/llvmcfiverify-$rev.tgz ; \
        gsutil.py cp -n -a public-read gs://chromium-browser-clang-staging/$x/translation_unit-$rev.tgz \
            gs://chromium-browser-clang/$x/translation_unit-$rev.tgz ; \
        gsutil.py cp -n -a public-read gs://chromium-browser-clang-staging/$x/llvm-code-coverage-$rev.tgz \
            gs://chromium-browser-clang/$x/llvm-code-coverage-$rev.tgz ; \
        done
    $ gsutil.py cp -n -a public-read gs://chromium-browser-clang-staging/Mac/lld-$rev.tgz \
          gs://chromium-browser-clang/Mac/lld-$rev.tgz
    ```

1.  Run the goma package update script to push these packages to goma. If you do
    not have the necessary credentials to do the upload, ask clang@chromium.org
    to find someone who does
1.  Run an exhaustive set of try jobs to test the new compiler:

    ```shell
    git cl try &&
    git cl try -m tryserver.chromium.mac -b mac_chromium_asan_rel_ng &&
    git cl try -B luci.chromium.try -b ios-device &&
    git cl try -m tryserver.chromium.linux \
      -b linux_chromium_chromeos_asan_rel_ng -b linux_chromium_msan_rel_ng \
      -b linux_chromium_cfi_rel_ng &&
    git cl try -m tryserver.blink -b linux_trusty_blink_rel &&
    git cl try -m tryserver.chromium.chromiumos -b linux-chromeos-dbg
    ```

1.  Commit roll CL from the first step
1.  The bots will now pull the prebuilt binary, and goma will have a matching
    binary, too.
