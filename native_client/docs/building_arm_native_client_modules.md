# NOTE: THIS IS QUITE OBSOLETE

please go here instead:
http://dev.chromium.org/nativeclient/native-client-documentation-index/building-and-testing-portable-native-client

# Building Native Client Modules For ARM Plaforms

NOTE: ARM development is only supported for linux systems

## Configure Your System For Emulation Voa QEMU

**You only need to do this once per machine**

As root, edit /etc/sysctl.conf, e.g.

sudo vi /etc/sysctl.conf

Add a line to the end of the file reading

vm.mmap\_min\_addr = 16384

Alternatively run:

everytime you reboot:

echo "16384" > /proc/sys/vm/mmap\_min\_addr

## Obtaining Toolchains

**You need to do this once for every checkout of the tree**

You will need two toolchains: a trusted one and an untrusted one which live in

*   .../native\_client/toolchain/linux\_arm-trusted/
*   ../native\_client/toolchain/linux\_arm-untrusted/

The trusted toolchain builds browser plugins, sel\_ldr, etc. The untrusted
toolchain builds nacl modules.

You can download working toolchains using `./scons --download platform=arm
sdl=none
`

You can also build the toolchains yourself using:

```
cd .../native_client/
tools/llvm/trusted-toolchain-creator.sh  trusted_sdk
tools/llvm/untrusted-toolchain-creator.sh   untrusted_sdk
```

NOTE: * this requires network access * there will be an error messages about $1
being undefined - ignore it * the last step will take for ever

## Running Simple Tests

## Build an ARM Validator Running on X86

```
./scons targetplatform=arm sdl=none arm-ncval-core
```

### Exercise the trusted toolchain

```
cd .../native_client/
./scons MODE=nacl,opt-linux platform=arm sdl=none naclsdk_validate=0 sel_ldr
```

### Exercise the untrusted toolchain

```
cd .../native_client/
./scons MODE=nacl,opt-linux platform=arm sdl=none  naclsdk_validate=0 barebones_hello_world.nexe
```

### Running a Simple Test In the Harness

```
cd .../native_client/
./scons MODE=nacl,opt-linux platform=arm sdl=none  naclsdk_validate=0 run_barebones_hello_world_test
```

To see what goes on under the hood here add **--verbose** to the commandline
(and maybe **sysinfo=** to suppress some other messages) `./scons
MODE=nacl,opt-linux platform=arm sdl=none naclsdk_mode=manual naclsdk_validate=0
run_barebones_hello_world_test --verbose sysinfo=
`

You should see something like: `...
/usr/local/google/ArmHardwareImages/gclients/gclient-nacl1/native_client/compiler/linux_arm-trusted/qemu-arm
-cpu cortex-a8 -L
/usr/local/google/ArmHardwareImages/gclients/gclient-nacl1/native_client/compiler/linux_arm-trusted/arm-2009q3/arm-none-linux-gnueabi/libc
/usr/local/google/ArmHardwareImages/gclients/gclient-nacl1/native_client/scons-out/opt-linux-arm/staging/sel_ldr
-f
/usr/local/google/ArmHardwareImages/gclients/gclient-nacl1/native_client/scons-out/nacl-arm/obj/tests/sysbasic/barebones_hello_world.nexe
...
`

You should be able to run this very same command line from your shell. Notice,
that we are using qemu to run this test. If you want to run this test on real
hardware you need to copy **sel\_ldr** and **barebones\_hello\_world.nexe** to
the ARM machine and run `.../sel_ldr -f .../barebones_hello_world.nexe
`

If you do run inside QEMU, using the options: **-d cpu,in\_asm,exec** generates
a trace file into: /tmp/qemu.log

### Running Lots of Tests In the Harness

```
./scons MODE=nacl,opt-linux platform=arm sdl=none  naclsdk_validate=0 run_barebones_hello_world_test smoke_tests
```

## Advanced Topics

### Rebuilding Newlib

NOTE: this may or may not work, it is always safer to build the entire TC from
scratch `tools/llvm/untrusted-toolchain-creator.sh newlib
`

### Rebuilding Other Base Libraries

NOTE: this may or may not work, it is always safer to build the entire TC from
scratch `tools/llvm/untrusted-toolchain-creator.sh extrasdk
`
