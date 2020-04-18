# Using moblabvm for autotest / devserver / moblab development

moblabvm is a virtual machine (VM) setup that allows launching two VMs -- one
moblab VM, running an image for the moblab-generic-vm board, and another
device-under-test (DUT) VM.  The DUT VM can run an image for board that supports
running as a VM. At present this includes only moblab-generic-vm and betty.

A moblabvm setup launches these two VMs and creates a private network bridge
between them.  Additionally, it sets up the moblab VM such that the DUT VM is
connected to it on the private network.

Here is a typical flow for using the setup.

- Use an existing workspace uploaded by the [moblab-generic-vm-paladin
  builder](https://uberchromegw.corp.google.com/i/chromiumos/builders/moblab-generic-vm-paladin).
  Simply download the workspace and untar it:
  ```
  pprabhu@pprabhu:scratch$ gsutil -m cp gs://chromeos-image-archive/moblab-generic-vm-paladin/R67-10469.0.0-rc1/workspace.tar.bz2 .
  Copying gs://chromeos-image-archive/moblab-generic-vm-paladin/R67-10469.0.0-rc1/workspace.tar.bz2...
  \ [1/1 files][  5.4 GiB/  5.4 GiB] 100% Done  93.6 MiB/s ETA 00:00:00
  Operation completed over 1 objects/5.4 GiB.
  pprabhu@pprabhu:scratch$ pbzip2 -d workspace.tar.bz2
  pprabhu@pprabhu:scratch$ tar -C moblabvm/ -xvf workspace.tar
  ./
  ./dut_image/
  ./dut_image/esp/
  ./dut_image/pack_partitions.sh
  ./dut_image/chromiumos_qemu_image.bin
  ./dut_image/config.txt
  ./dut_image/license_credits.html
  ./dut_image/boot.desc
  ./dut_image/id_rsa.pub
  ./dut_image/vmlinuz.bin
  ./dut_image/id_rsa
  ./dut_image/boot.config
  ./dut_image/umount_image.sh
  ./dut_image/partition_script.sh
  ./dut_image/au-generator.zip
  ./dut_image/mount_image.sh
  ./dut_image/unpack_partitions.sh
  ./moblab_image/
  ./moblab_image/esp/
  ./moblab_image/pack_partitions.sh
  ./moblab_image/chromiumos_qemu_image.bin
  ./moblab_image/config.txt
  ./moblab_image/license_credits.html
  ./moblab_image/boot.desc
  ./moblab_image/id_rsa.pub
  ./moblab_image/vmlinuz.bin
  ./moblab_image/id_rsa
  ./moblab_image/boot.config
  ./moblab_image/umount_image.sh
  ./moblab_image/partition_script.sh
  ./moblab_image/au-generator.zip
  ./moblab_image/moblab_disk
  ./moblab_image/mount_image.sh
  ./moblab_image/unpack_partitions.sh
  ./moblabvm.json
  ```
  Now jump to the instructions below for starting the VMs from this pre-created
  workspace.

- Obtain and unzip a moblab image. You can start with artifacts from a [recent
  moblab-generic-vm-paladin run](https://uberchromegw.corp.google.com/i/chromiumos/builders/moblab-generic-vm-paladin).
  - It is faster to download the image via gsutil.
    ```
    pprabhu@pprabhu:moblab_image$ gsutil -m cp
    gs://chromeos-image-archive/moblab-generic-vm-paladin/R66-10406.0.0-rc2/image.zip .
    Copying
    gs://chromeos-image-archive/moblab-generic-vm-paladin/R66-10406.0.0-rc2/image.zip...
    \ [1/1 files][  1.9 GiB/  1.9 GiB] 100% Done  83.3 MiB/s ETA 00:00:00
    Operation completed over 1 objects/1.9 GiB.
    ```
  - unzip into *moblab_image* folder. Note that we need the image as well
    as some of the scripts bundled with it, and that the unzipped contents take
    up about 13 GB.
    ```
    pprabhu@pprabhu:moblab_image$ unzip image.zip
    Archive:  image.zip
      inflating: config.txt
      creating: esp/
      inflating: chromiumos_base_image.bin
      inflating: umount_image.sh
      inflating: chromiumos_test_image.bin
      inflating: boot.config
      inflating: license_credits.html
      inflating: id_rsa.pub
      inflating: id_rsa
      inflating: boot.desc
      inflating: partition_script.sh
      inflating: unpack_partitions.sh
    extracting: au-generator.zip
      inflating: vmlinuz.bin
      inflating: mount_image.sh
      inflating: pack_partitions.sh
    ```
- Obtain and unzip our DUT image. You could use the same moblab image that you
  just downloaded (just provided the same path for *dut_image* below).
  We'll use a recent [betty paladin
  image](https://uberchromegw.corp.google.com/i/chromeos/builders/betty-paladin).
  ```
  pprabhu@pprabhu:dut_image$ gsutil -m cp gs://chromeos-image-archive/betty-paladin/R66-10406.0.0-rc2/image.zip .
  Copying gs://chromeos-image-archive/betty-paladin/R66-10406.0.0-rc2/image.zip...
  / [1/1 files][  4.0 GiB/  4.0 GiB] 100% Done  75.7 MiB/s ETA 00:00:00
  Operation completed over 1 objects/4.0 GiB.
  pprabhu@pprabhu:dut_image$ unzip image.zip
  Archive:  image.zip
    inflating: chromiumos_test_image.bin
    inflating: partition_script.sh
  extracting: au-generator.zip
    inflating: umount_image.sh
    inflating: license_credits.html
    inflating: mount_image.sh
    creating: esp/
    inflating: cheets-fingerprint.txt
    inflating: chromiumos_qemu_image.bin
    inflating: vmlinuz.bin
    inflating: unpack_partitions.sh
    inflating: boot.config
    inflating: config.txt
    inflating: chromiumos_base_image.bin
    inflating: pack_partitions.sh
    inflating: id_rsa
    inflating: boot.desc
    inflating: id_rsa.pub
  ```
- Create a new moblabvm using these images. You need to be under a full
  chromiumos checkout for the following to work smoothly. All _cros_ commands
  are run outside the chroot. Note that this assumes that /work/scratch, or
  whatever you use in its place, is a sane path and has ~15 GB of space free.
  ```
  pprabhu@pprabhu:chromiumos$ cros moblabvm --workspace /work/scratch/moblabvm create --dut-image-dir /work/scratch/dut_image /work/scratch/moblab_image
  11:00:48: NOTICE: Initializing workspace in /work/scratch/moblabvm
  11:00:48: NOTICE: This involves creating some VM images. May take a few minutes.
  11:00:48: NOTICE: Preparing moblab image...
  11:05:16: NOTICE: Generating moblab external disk...
  11:05:17: NOTICE: Preparing dut image...
  11:11:33: NOTICE: All Done!
  ```
  Workspace is where the prepared images and other metadata for the setup will
  be stored. Pick any writeable dir.
- Launch the VM setup.
  ```
  pprabhu@pprabhu:chromiumos$ cros moblabvm --workspace /work/scratch/moblabvm start
  11:14:42: NOTICE: MoblabVm is running.
  11:14:42: NOTICE: Moblab VM information:
  11:14:42: NOTICE:   SSH Port to connect from host: 16482
  11:14:42: NOTICE:   MAC address of moblab-internal network in the VM: 02:00:00:99:99:01
  11:14:42: NOTICE: sub-DUT information:
  11:14:42: NOTICE:   SSH Port to connect from host: 16493
  11:14:42: NOTICE:   MAC address of moblab-internal network in the VM: 02:00:00:99:99:51
  ```

The VM setup is now up. You can SSH into the moblab VM or the DUT VM like any other Chrome OS device.
```
pprabhu@pprabhu:chromiumos$ ssh -o StrictHostKeyChecking=no -i ~/.ssh/testing_rsa root@localhost -p 16482
Warning: Permanently added '[localhost.corp.google.com]:16482' (ED25519) to the list of known hosts.
localhost ~ # cat /etc/lsb-release
CHROMEOS_RELEASE_BUILDER_PATH=moblab-generic-vm-paladin/R66-10406.0.0-rc2
GOOGLE_RELEASE=10406.0.0-rc2
CHROMEOS_DEVSERVER=http://build112-m2.golo.chromium.org:8080
CHROMEOS_RELEASE_BOARD=moblab-generic-vm
CHROMEOS_RELEASE_BUILD_NUMBER=10406
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=66
CHROMEOS_RELEASE_PATCH_NUMBER=0-rc2
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=10406.0.0-rc2 (Continuous Builder - Builder: N/A) moblab-generic-vm
CHROMEOS_RELEASE_NAME=Chromium OS
CHROMEOS_RELEASE_BUILD_TYPE=Continuous Builder - Builder: N/A
CHROMEOS_RELEASE_VERSION=10406.0.0-rc2
CHROMEOS_AUSERVER=http://build112-m2.golo.chromium.org:8080/update
```

## Bootstrapping

The setup so far is sufficient if you want to run
[moblab_RunSuite](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/site_tests/moblab_RunSuite/)
against it. This test bootstraps the moblab setup (independent of whether it's a
VM or a real moblab device connected to other real DUTs).

If you want to use this moblab VM setup for local development, you need to
bootstrap it yourself. One way to do this is to simply run a moblab_RunSuite
test against the setup before you begin.
```
cros_sdk -- test_that --no-quickmerge -b moblab-generic-vm localhost:16482 \
    moblab_DummyServerNoSspSuite --args 'services_init_timeout_m=10 \
    target_build="betty-paladin/R66-10406.0.0-rc2" \
    test_timeout_hint_m=101'
```
This test takes a while to run (~30 minutes) because it stages the requested DUT
VM image from Google Storage onto the moblab's devserver cache, and provisions
the DUT VM with that image.

If you do not want to run this test, you need to at a minimum:
- copy your $HOME/.boto file to /home/moblab/.boto (chown moblab:moblab user)
  ```
  pprabhu@pprabhu:~$ scp -o StrictHostKeyChecking=no -i ~/.ssh/testing_rsa -P 16482 ~/.boto root@localhost:/home/moblab/.boto
  ```
- Find and add the DUT to the moblab's autotest instance.  You can get the IP of
  the DUT VM on the private network by SSHing into the DUT. The DUT has two
  network interfaces:
  ```
  pprabhu@pprabhu:~$ ssh -o StrictHostKeyChecking=no -i ~/.ssh/testing_rsa root@localhost -p 16493

  localhost ~ # ip addr show eth0
  2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
      link/ether 52:54:00:12:34:56 brd ff:ff:ff:ff:ff:ff
      inet 10.0.2.15/24 brd 10.0.2.255 scope global eth0
        valid_lft forever preferred_lft forever
      inet6 fec0::8582:d2dc:ddb7:397b/64 scope site temporary dynamic
        valid_lft 86352sec preferred_lft 14352sec
      inet6 fec0::5054:ff:fe12:3456/64 scope site mngtmpaddr dynamic
        valid_lft 86352sec preferred_lft 14352sec
      inet6 fe80::5054:ff:fe12:3456/64 scope link
        valid_lft forever preferred_lft forever
  localhost ~ # ip addr show eth1
  4: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
      link/ether 02:00:00:99:99:51 brd ff:ff:ff:ff:ff:ff
      inet 192.168.231.100/24 brd 192.168.231.255 scope global eth1
        valid_lft forever preferred_lft forever
      inet6 fe80::ff:fe99:9951/64 scope link
        valid_lft forever preferred_lft forever
  ```
  The 192.XXX is the one on the internal network. So, add that on the moblab VM.
  ```
  moblab@localhost ~ $ /usr/local/autotest/cli/atest host create 192.168.231.100
  ...
  Added host:
          192.168.231.100

  moblab@localhost ~ $ /usr/local/autotest/cli/atest host list
  Host             Status  Shard  Locked  Lock Reason  Locked by  Platform  Labels
  192.168.231.100  Ready   None   False                None       (error)   sparse_coverage_5, ... REDACTED
  ```
  This will spew out a bunch of stuff as autotest gets the label information for
  the DUT.

## Using the setup

At this point, you have a running moblabvm setup. You can use it like any other
moblab setup. For example, you can ssh into the moblab VM, and see the DUT VM
listed as an autotest host.

You can also schedule a simple suite against the DUT in the moblab VM.
```
moblab@localhost ~ $ /usr/local/autotest/site_utils/run_suite.py -b betty -i
betty-paladin/R66-10406.0.0-rc2 --pool '' -s dummy_server_nossp
Autotest instance created: localhost
02-15-2018 [11:30:54] Submitted create_suite_job rpc
02-15-2018 [11:30:54] Created suite job:
http://localhost/afe/#tab_id=view_job&object_id=1
@@@STEP_LINK@Link to suite@http://localhost/afe/#tab_id=view_job&object_id=1@@@
...
```
Here, we're installing the same betty image on the DUT VM that it alread has,
but the provision flow still will be run (the DUT VM will be provisioned)
because the autotest host doesn't have the cros-version label (see above).
This will take a while the first time as the image to be provisioned on the DUT
VM is staged inside moblab from Google Storage.

## Iterating on infrastructure code

This setup is especially useful for iterating on most pieces of the autotest
infrastructure code, instead of using
[local_dev_autotest](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/site_utils/setup_dev_autotest.sh)
to install the full autotest stack on your workstation, as we've done in the
past.

For this, instead of downloading a moblab image from a builder, build the image
yourself.
```
pprabhu@pprabhu:chromiumos$ cros_sdk
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ ./setup_board --board moblab-generic-vm
...
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ ./build_packages --board moblab-generic-vm
...
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ ./build_image --board moblab-generic-vm --noenable_rootfs_verification test
...
```

Then use the image directory
(~/chromiumos/src/build/images/moblab-generic-vm/latest) for creating the moblab
VM. To iterate, simply rebuild the package your changing, and cros deploy it to
the moblab VM.

Continuing the example above, if you were working on autotest infrastructure
code:
```
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ cros_workon --board moblab-generic-vm start chromeos-base/autotest-server
11:13:26: INFO: Started working on 'chromeos-base/autotest-server' for 'moblab-generic-vm'
```
Make changes to autotest as needed, then deploy the changes to moblab.
```
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ emerge-moblab-generic-vm chromeos-base/autotest-server
pprabhu@pprabhu:~$ cros deploy localhost:16482 chromeos-base/autotest-server
```
Finally, restart any services on moblab that may have been affected. You can
find relevant services by running `ls /etc/init | grep moblab`.
```
moblab@localhost ~ $ sudo initctl restart moblab-scheduler-init
```

Your changes are now live!

If you're working on devserver instead, you want:
```
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ cros_workon --board moblab-generic-vm start chromeos-base/devserver
(cr) ((8e3381b52...)) pprabhu@pprabhu ~/trunk/src/scripts $ emerge-moblab-generic-vm chromeos-base/devserver
pprabhu@pprabhu:~$ cros deploy localhost:16482 chromeos-base/devserver
```
# Troubleshooting

For most errors, the first thing to do is to rerun with `--debug`. This may make
the path forward clear on its own, and if not will get a better message to seek
help from other developers with.

## Out of Disk Space

Images are large, and so if their location is chosen poorly you may run out of
disk space while unzipping the pre-existing images or creating the new ones.
Development machines generally have a extra partition with >60G of
space, which may be named `/work` or `/dev`, where you can place them.

## qemu-ifup fails

If you see:
```
network script /etc/qemu-ifup failed with status 256
```
You don't have qemu-kvm installed.

Run:
```
sudo apt-get install qemu-kvm
```
