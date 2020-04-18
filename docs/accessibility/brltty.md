# BRLTTY in Chrome OS

Chrome OS uses the open-source [BRLTTY](http://mielke.cc/brltty/)
library to provide support for refreshable braille displays.

We typically ship with a stable release build of BRLTTY plus some
cherry-picked patches.

## Updating BRLTTY or adding a patch

First, follow the public
[Chromium OS Developer Guide](http://www.chromium.org/chromium-os/developer-guide) to check out the source.
At a minimum you'll need to create a chroot.
You do not need to build everything from source.
You do need to start the devserver.

Next, flash your device to a very recent test build. Internally at Google
you can do this with the following command when the dev server is running,
where CHROMEBOOK_IP_ADDRESS is the IP address of your Chromebook already
in developer mode, and $BOARD is your Chromebook's board name.

```cros flash ssh://CHROMEBOOK_IP_ADDRESS xbuddy://remote/$BOARD/latest-dev/test```

The BRLTTY files can be found in this directory:

```third_party/chromiumos-overlay/app-accessibility/brltty```

The first thing you'll need to do is edit the ebuild symlink to change the
revision number. The real file is something like brltty-5.4.ebuild,
but the revision will be something like brltty-5.4-r5.ebuild. You'll need
to increment it.

To increment it from r5 to r6, you'd do something like this:

```
rm brltty-5.4-r5.ebuild
ln -s brltty-5.4.ebuild brltty-5.4-r6.ebuild
git add brltty-5.4-r6.ebuild
```

The changes we make are all patches against a stable release of brltty.
To add a new patch, put it in the files/ directory and reference it in
brltty.bashrc

Once you're done adding patches or making other changes, flash it to your
device like this:

```
emerge-$BOARD brltty
cros deploy CHROMEBOOK_IP_ADDRESS brltty
```

After that, reboot your Chromebook and verify that brltty works.

To upload a change, use repo, something like this:

```
repo start <branch_name> .
git commit -a
  BUG=chromium:12345
  TEST=Write what you tested here
repo upload .
```

Note that you shouldn't need to run cros_workon.
