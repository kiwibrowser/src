# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""crosfw - Chrome OS Firmware build/flash script.

Builds a firmware image for any board and writes it to the board. The image
can be pure upstream or include Chrome OS components (-V). Some device
tree parameters can be provided, including silent console (-C) and secure
boot (-S). Use -i for a faster incremental build. The image is written to
the board by default using USB/em100 (or sdcard with -x). Use -b to specify
the board to build. Options can be added to ~/.crosfwrc - see the script for
details.

It can also flash SPI by writing a 'magic flasher' U-Boot with a payload
to the board.

The script is normally run from within the U-Boot directory which is
.../src/third_party/u-boot/files

Example 1: Build upstream image for coreboot and write to a 'link':

 crosfw -b link

Example 2: Build verified boot image (V) for daisy/snow and boot in secure
 mode (S) so that breaking in on boot is not possible.

 crosfw -b daisy -VS
 crosfw -b daisy -VSC         (no console output)

Example 3: Build a magic flasher (F) with full verified boot for peach_pit,
 but with console enabled, write to SD card (x)

 crosfw -b peach_pit -VSFx

This sript does not use an ebuild. It does a similar thing to the
chromeos-u-boot ebuild, and runs cros_bundle_firmware to produce various
types of image, a little like the chromeos-bootimage ebuild.

The purpose of this script is to make it easier and faster to perform
common firmware build tasks without changing boards, manually updating
device tree files or lots of USE flags and complexity in the ebuilds.

This script has been tested with snow, link and peach_pit. It builds for
peach_pit by default. Note that it will also build any upstream ARM
board - e.g. "-b snapper9260" will build an image for that board.

Mostly you can use the script inside and outside the chroot. The main
limitation is that dut-control doesn't really work outside the chroot,
so writing the image to the board over USB is not possible, nor can the
board be automatically reset on x86 platforms.

For an incremental build (faster), run with -i

To get faster clean builds, install ccache, and create ~/.crosfwrc with
this line:

 USE_CCACHE = True

(make sure ~/.ccache is not on NFS, or set CCACHE_DIR)

Other options are the default board to build, and verbosity (0-4), e.g.:

 DEFAULT_BOARD = 'daisy'
 VERBOSE = 1

It is possible to use multiple servo boards, each on its own port. Add
these lines to your ~/.crosfwrc to set the servo port to use for each
board:

 SERVO_PORT['link'] = 8888
 SERVO_PORT['daisy'] = 9999
 SERVO_PORT['peach_pit'] = 7777

All builds appear in the <outdir>/<board> subdirectory and images are written
to <outdir>/<uboard>/out, where <uboard> is the U-Boot name for the board (in
the U-Boot boards.cfg file)

The value for <outdir> defaults to /tmp/crosfw but can be configured in your
~/.crosfwrc file, e.g.:"

 OUT_DIR = '/tmp/u-boot'

For the -a option here are some useful options:

--add-blob cros-splash /dev/null
--gbb-flags -force-dev-switch-on
--add-node-enable /spi@131b0000/cros-ecp@0 1
--verify --full-erase
--bootcmd "cros_test sha"
--gbb-flags -force-dev-switch-on
--bmpblk ~/trunk/src/third_party/u-boot/bmp.bin

For example: -a "--gbb-flags -force-dev-switch-on"

Note the standard bmpblk is at:
  /home/$USER/trunk/src/third_party/chromiumos-overlay/sys-boot/
      chromeos-bootimage/files/bmpblk.bin"
"""

from __future__ import print_function

import glob
import multiprocessing
import os
import re
import sys

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import parallel


arch = None
board = None
compiler = None
default_board = None
family = None
in_chroot = True

logging.basicConfig(format='%(message)s')
kwargs = {'print_cmd': False, 'error_code_ok': True,
          'debug_level': logging.getLogger().getEffectiveLevel()}

outdir = ''

 # If you have multiple boards connected on different servo ports, put lines
# like 'SERVO_PORT{"peach_pit"} = 7777' in your ~/.crosfwrc
SERVO_PORT = {}

smdk = None
src_root = os.path.join(constants.SOURCE_ROOT, 'src')
in_chroot = cros_build_lib.IsInsideChroot()

uboard = ''

default_board = 'peach_pit'
use_ccache = False
vendor = None
verbose = False

# Special cases for the U-Boot board config, the SOCs and default device tree
# since the naming is not always consistent.
# x86 has a lot of boards, but to U-Boot they are all the same
UBOARDS = {
    'daisy': 'smdk5250',
    'peach': 'smdk5420',
}
for b in ['alex', 'butterfly', 'emeraldlake2', 'link', 'lumpy', 'parrot',
          'stout', 'stumpy']:
  UBOARDS[b] = 'coreboot-x86'
  UBOARDS['chromeos_%s' % b] = 'chromeos_coreboot'

SOCS = {
    'coreboot-x86': '',
    'chromeos_coreboot': '',
    'daisy': 'exynos5250-',
    'peach': 'exynos5420-',
}

DEFAULT_DTS = {
    'daisy': 'snow',
    'daisy_spring': 'spring',
    'peach_pit': 'peach-pit',
}

OUT_DIR = '/tmp/crosfw'

rc_file = os.path.expanduser('~/.crosfwrc')
if os.path.exists(rc_file):
  execfile(rc_file)


def Log(msg):
  """Print out a message if we are in verbose mode.

  Args:
    msg: Message to print
  """
  if verbose:
    logging.info(msg)


def Dumper(flag, infile, outfile):
  """Run objdump on an input file.

  Args:
    flag: Flag to pass objdump (e.g. '-d').
    infile: Input file to process.
    outfile: Output file to write to.
  """
  result = cros_build_lib.RunCommand(
      [CompilerTool('objdump'), flag, infile],
      log_stdout_to_file=outfile, **kwargs)
  if result.returncode:
    sys.exit()


def CompilerTool(tool):
  """Returns the cross-compiler tool filename.

  Args:
    tool: Tool name to return, e.g. 'size'.

  Returns:
    Filename of requested tool.
  """
  return '%s%s' % (compiler, tool)


def ParseCmdline(argv):
  """Parse all command line options.

  Args:
    argv: Arguments to parse.

  Returns:
    The parsed options object
  """
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-a', '--cbfargs', action='append',
                      help='Pass extra arguments to cros_bundle_firmware')
  parser.add_argument('-b', '--board', type=str, default=default_board,
                      help='Select board to build (daisy/peach_pit/link)')
  parser.add_argument('-B', '--build', action='store_false', default=True,
                      help="Don't build U-Boot, just configure device tree")
  parser.add_argument('-C', '--console', action='store_false', default=True,
                      help='Permit console output')
  parser.add_argument('-d', '--dt', default='seaboard',
                      help='Select name of device tree file to use')
  parser.add_argument('-D', '--nodefaults', dest='use_defaults',
                      action='store_false', default=True,
                      help="Don't select default filenames for those not given")
  parser.add_argument('-F', '--flash', action='store_true', default=False,
                      help='Create magic flasher for SPI flash')
  parser.add_argument('-M', '--mmc', action='store_true', default=False,
                      help='Create magic flasher for eMMC')
  parser.add_argument('-i', '--incremental', action='store_true', default=False,
                      help="Don't reconfigure and clean")
  parser.add_argument('-k', '--kernel', action='store_true', default=False,
                      help='Send kernel to board also')
  parser.add_argument('-O', '--objdump', action='store_true', default=False,
                      help='Write disassembly output')
  parser.add_argument('-r', '--run', action='store_false', default=True,
                      help='Run the boot command')
  parser.add_argument('--ro', action='store_true', default=False,
                      help='Create Chrome OS read-only image')
  parser.add_argument('--rw', action='store_true', default=False,
                      help='Create Chrome OS read-write image')
  parser.add_argument('-s', '--separate', action='store_false', default=True,
                      help='Link device tree into U-Boot, instead of separate')
  parser.add_argument('-S', '--secure', action='store_true', default=False,
                      help='Use vboot_twostop secure boot')
  parser.add_argument('--small', action='store_true', default=False,
                      help='Create Chrome OS small image')
  parser.add_argument('-t', '--trace', action='store_true', default=False,
                      help='Enable trace support')
  parser.add_argument('-v', '--verbose', type=int, default=0,
                      help='Make cros_bundle_firmware verbose')
  parser.add_argument('-V', '--verified', action='store_true', default=False,
                      help='Include Chrome OS verified boot components')
  parser.add_argument('-w', '--write', action='store_false', default=True,
                      help="Don't write image to board using usb/em100")
  parser.add_argument('-x', '--sdcard', action='store_true', default=False,
                      help='Write to SD card instead of USB/em100')
  parser.add_argument('-z', '--size', action='store_true', default=False,
                      help='Display U-Boot image size')
  parser.add_argument('target', nargs='?', default='all',
                      help='The target to work on')
  return parser.parse_args(argv)


def FindCompiler(gcc, cros_prefix):
  """Look up the compiler for an architecture.

  Args:
    gcc: GCC architecture, either 'arm' or 'aarch64'
    cros_prefix: Full Chromium OS toolchain prefix
  """
  if in_chroot:
    # Use the Chromium OS toolchain.
    prefix = cros_prefix
  else:
    prefix = glob.glob('/opt/linaro/gcc-linaro-%s-linux-*/bin/*gcc' % gcc)
    if not prefix:
      cros_build_lib.Die("""Please install an %s toolchain for your machine.
Install a Linaro toolchain from:
https://launchpad.net/linaro-toolchain-binaries
or see cros/commands/cros_chrome_sdk.py.""" % gcc)
    prefix = re.sub('gcc$', '', prefix[0])
  return prefix


def SetupBuild(options):
  """Set up parameters needed for the build.

  This checks the current environment and options and sets up various things
  needed for the build, including 'base' which holds the base flags for
  passing to the U-Boot Makefile.

  Args:
    options: Command line options

  Returns:
    Base flags to use for U-Boot, as a list.
  """
  # pylint: disable=W0603
  global arch, board, compiler, family, outdir, smdk, uboard, vendor, verbose

  if not verbose:
    verbose = options.verbose != 0

  logging.getLogger().setLevel(options.verbose)

  Log('Building for %s' % options.board)

  # Separate out board_variant string: "peach_pit" becomes "peach", "pit".
  # But don't mess up upstream boards which use _ in their name.
  parts = options.board.split('_')
  if parts[0] in ['daisy', 'peach']:
    board = parts[0]
  else:
    board = options.board

  # To allow this to be run from 'cros_sdk'
  if in_chroot:
    os.chdir(os.path.join(src_root, 'third_party', 'u-boot', 'files'))

  base_board = board

  if options.verified:
    base_board = 'chromeos_%s' % base_board

  uboard = UBOARDS.get(base_board, base_board)
  Log('U-Boot board is %s' % uboard)

  # Pull out some information from the U-Boot boards config file
  family = None
  (PRE_KBUILD, PRE_KCONFIG, KCONFIG) = range(3)
  if os.path.exists('MAINTAINERS'):
    board_format = PRE_KBUILD
  else:
    board_format = PRE_KCONFIG
  with open('boards.cfg') as f:
    for line in f:
      if 'genboardscfg' in line:
        board_format = KCONFIG
      if uboard in line:
        if line[0] == '#':
          continue
        fields = line.split()
        if not fields:
          continue
        arch = fields[1]
        fields += [None, None, None]
        if board_format == PRE_KBUILD:
          smdk = fields[3]
          vendor = fields[4]
          family = fields[5]
          target = fields[6]
        elif board_format in (PRE_KCONFIG, KCONFIG):
          smdk = fields[5]
          vendor = fields[4]
          family = fields[3]
          target = fields[0]

        # Make sure this is the right target.
        if target == uboard:
          break
  if not arch:
    cros_build_lib.Die("Selected board '%s' not found in boards.cfg." % board)

  vboot = os.path.join('build', board, 'usr')
  if arch == 'x86':
    family = 'em100'
    if in_chroot:
      compiler = 'i686-pc-linux-gnu-'
    else:
      compiler = '/opt/i686/bin/i686-unknown-elf-'
  elif arch == 'arm':
    compiler = FindCompiler(arch, 'armv7a-cros-linux-gnueabi-')
  elif arch == 'aarch64':
    compiler = FindCompiler(arch, 'aarch64-cros-linux-gnu-')
    # U-Boot builds both arm and aarch64 with the 'arm' architecture.
    arch = 'arm'
  elif arch == 'sandbox':
    compiler = ''
  else:
    cros_build_lib.Die("Selected arch '%s' not supported." % arch)

  if not options.build:
    options.incremental = True

  cpus = multiprocessing.cpu_count()

  outdir = os.path.join(OUT_DIR, uboard)
  base = [
      'make',
      '-j%d' % cpus,
      'O=%s' % outdir,
      'ARCH=%s' % arch,
      'CROSS_COMPILE=%s' % compiler,
      '--no-print-directory',
      'HOSTSTRIP=true',
      'DEV_TREE_SRC=%s-%s' % (family, options.dt),
      'QEMU_ARCH=']

  if options.verbose < 2:
    base.append('-s')
  elif options.verbose > 2:
    base.append('V=1')

  if options.ro and options.rw:
    cros_build_lib.Die('Cannot specify both --ro and --rw options')
  if options.ro:
    base.append('CROS_RO=1')
    options.small = True

  if options.rw:
    base.append('CROS_RW=1')
    options.small = True

  if options.small:
    base.append('CROS_SMALL=1')
  else:
    base.append('CROS_FULL=1')

  if options.verified:
    base += [
        'VBOOT=%s' % vboot,
        'MAKEFLAGS_VBOOT=DEBUG=1',
        'QUIET=1',
        'CFLAGS_EXTRA_VBOOT=-DUNROLL_LOOPS',
        'VBOOT_SOURCE=%s/platform/vboot_reference' % src_root]
    base.append('VBOOT_DEBUG=1')

  # Handle the Chrome OS USE_STDINT workaround. Vboot needs <stdint.h> due
  # to a recent change, the need for which I didn't fully understand. But
  # U-Boot doesn't normally use this. We have added an option to U-Boot to
  # enable use of <stdint.h> and without it vboot will fail to build. So we
  # need to enable it where ww can. We can't just enable it always since
  # that would prevent this script from building other non-Chrome OS boards
  # with a different (older) toolchain, or Chrome OS boards without vboot.
  # So use USE_STDINT if the toolchain supports it, and not if not. This
  # file was originally part of glibc but has recently migrated to the
  # compiler so it is reasonable to use it with a stand-alone program like
  # U-Boot. At this point the comment has got long enough that we may as
  # well include some poetry which seems to be sorely lacking the code base,
  # so this is from Ogden Nash:
  #    To keep your marriage brimming
  #    With love in the loving cup,
  #    Whenever you're wrong, admit it;
  #    Whenever you're right, shut up.
  cmd = [CompilerTool('gcc'), '-ffreestanding', '-x', 'c', '-c', '-']
  result = cros_build_lib.RunCommand(cmd,
                                     input='#include <stdint.h>',
                                     capture_output=True,
                                     **kwargs)
  if result.returncode == 0:
    base.append('USE_STDINT=1')

  base.append('BUILD_ROM=1')
  if options.trace:
    base.append('FTRACE=1')
  if options.separate:
    base.append('DEV_TREE_SEPARATE=1')

  if options.incremental:
    # Get the correct board for cros_write_firmware
    config_mk = '%s/include/autoconf.mk' % outdir
    if not os.path.exists(config_mk):
      logging.warning('No build found for %s - dropping -i' % board)
      options.incremental = False

  config_mk = 'include/autoconf.mk'
  if os.path.exists(config_mk):
    logging.warning("Warning: '%s' exists, try 'make distclean'" % config_mk)

  # For when U-Boot supports ccache
  # See http://patchwork.ozlabs.org/patch/245079/
  if use_ccache:
    os.environ['CCACHE'] = 'ccache'

  return base


def RunBuild(options, base, target, queue):
  """Run the U-Boot build.

  Args:
    options: Command line options.
    base: Base U-Boot flags.
    target: Target to build.
    queue: A parallel queue to add jobs to.
  """
  Log('U-Boot build flags: %s' % ' '.join(base))

  # Reconfigure U-Boot.
  if not options.incremental:
    # Ignore any error from this, some older U-Boots fail on this.
    cros_build_lib.RunCommand(base + ['distclean'], **kwargs)
    if os.path.exists('tools/genboardscfg.py'):
      mtarget = 'defconfig'
    else:
      mtarget = 'config'
    cmd = base + ['%s_%s' % (uboard, mtarget)]
    result = cros_build_lib.RunCommand(cmd, capture_output=True,
                                       combine_stdout_stderr=True, **kwargs)
    if result.returncode:
      print("cmd: '%s', output: '%s'" % (result.cmdstr, result.output))
      sys.exit(result.returncode)

  # Do the actual build.
  if options.build:
    result = cros_build_lib.RunCommand(base + [target], capture_output=True,
                                       combine_stdout_stderr=True, **kwargs)
    if result.returncode:
      # The build failed, so output the results to stderr.
      print("cmd: '%s', output: '%s'" % (result.cmdstr, result.output),
            file=sys.stderr)
      sys.exit(result.returncode)

  files = ['%s/u-boot' % outdir]
  spl = glob.glob('%s/spl/u-boot-spl' % outdir)
  if spl:
    files += spl
  if options.size:
    result = cros_build_lib.RunCommand([CompilerTool('size')] + files,
                                       **kwargs)
    if result.returncode:
      sys.exit()

  # Create disassembly files .dis and .Dis (full dump)
  for f in files:
    base = os.path.splitext(f)[0]
    if options.objdump:
      queue.put(('-d', f, base + '.dis'))
      queue.put(('-D', f, base + '.Dis'))
    else:
      # Remove old files which otherwise might be confusing
      osutils.SafeUnlink(base + '.dis')
      osutils.SafeUnlink(base + '.Dis')

  Log('Output directory %s' % outdir)


def WriteFirmware(options):
  """Write firmware to the board.

  This uses cros_bundle_firmware to create a firmware image and write it to
  the board.

  Args:
    options: Command line options
  """
  flash = []
  kernel = []
  run = []
  secure = []
  servo = []
  silent = []
  verbose_arg = []
  ro_uboot = []

  bl2 = ['--bl2', '%s/spl/%s-spl.bin' % (outdir, smdk)]

  if options.use_defaults:
    bl1 = []
    bmpblk = []
    ecro = []
    ecrw = []
    defaults = []
  else:
    bl1 = ['--bl1', '##/build/%s/firmware/u-boot.bl1.bin' % options.board]
    bmpblk = ['--bmpblk', '##/build/%s/firmware/bmpblk.bin' % options.board]
    ecro = ['--ecro', '##/build/%s/firmware/ec.RO.bin' % options.board]
    ecrw = ['--ec', '##/build/%s/firmware/ec.RW.bin' % options.board]
    defaults = ['-D']

  if arch == 'x86':
    seabios = ['--seabios',
               '##/build/%s/firmware/seabios.cbfs' % options.board]
  else:
    seabios = []

  if options.sdcard:
    dest = 'sd:.'
  elif arch == 'x86':
    dest = 'em100'
  elif arch == 'sandbox':
    dest = ''
  else:
    dest = 'usb'

  port = SERVO_PORT.get(options.board, '')
  if port:
    servo = ['--servo', '%d' % port]

  if options.flash:
    flash = ['-F', 'spi']

    # The small builds don't have the command line interpreter so cannot
    # run the magic flasher script. So use the standard U-Boot in this
    # case.
    if options.small:
      logging.warning('Using standard U-Boot as flasher')
      flash += ['-U', '##/build/%s/firmware/u-boot.bin' % options.board]

  if options.mmc:
    flash = ['-F', 'sdmmc']

  if options.verbose:
    verbose_arg = ['-v', '%s' % options.verbose]

  if options.secure:
    secure += ['--bootsecure', '--bootcmd', 'vboot_twostop']

  if not options.verified:
    # Make a small image, without GBB, etc.
    secure.append('-s')

  if options.kernel:
    kernel = ['--kernel', '##/build/%s/boot/vmlinux.uimg' % options.board]

  if not options.console:
    silent = ['--add-config-int', 'silent-console', '1']

  if not options.run:
    run = ['--bootcmd', 'none']

  if arch != 'sandbox' and not in_chroot and servo:
    if dest == 'usb':
      logging.warning('Image cannot be written to board')
      dest = ''
      servo = []
    elif dest == 'em100':
      logging.warning('Please reset the board manually to boot firmware')
      servo = []

    if not servo:
      logging.warning('(sadly dut-control does not work outside chroot)')

  if dest:
    dest = ['-w', dest]
  else:
    dest = []

  soc = SOCS.get(board)
  if not soc:
    soc = SOCS.get(uboard, '')
  dt_name = DEFAULT_DTS.get(options.board, options.board)
  dts_file = 'board/%s/dts/%s%s.dts' % (vendor, soc, dt_name)
  Log('Device tree: %s' % dts_file)

  if arch == 'sandbox':
    uboot_fname = '%s/u-boot' % outdir
  else:
    uboot_fname = '%s/u-boot.bin' % outdir

  if options.ro:
    # RO U-Boot is passed through as blob 'ro-boot'. We use the standard
    # ebuild one as RW.
    # TODO(sjg@chromium.org): Option to build U-Boot a second time to get
    # a fresh RW U-Boot.
    logging.warning('Using standard U-Boot for RW')
    ro_uboot = ['--add-blob', 'ro-boot', uboot_fname]
    uboot_fname = '##/build/%s/firmware/u-boot.bin' % options.board
  cbf = ['%s/platform/dev/host/cros_bundle_firmware' % src_root,
         '-b', options.board,
         '-d', dts_file,
         '-I', 'arch/%s/dts' % arch, '-I', 'cros/dts',
         '-u', uboot_fname,
         '-O', '%s/out' % outdir,
         '-M', family]

  for other in [bl1, bl2, bmpblk, defaults, dest, ecro, ecrw, flash, kernel,
                run, seabios, secure, servo, silent, verbose_arg, ro_uboot]:
    if other:
      cbf += other
  if options.cbfargs:
    for item in options.cbfargs:
      cbf += item.split(' ')
  os.environ['PYTHONPATH'] = ('%s/platform/dev/host/lib:%s/..' %
                              (src_root, src_root))
  Log(' '.join(cbf))
  result = cros_build_lib.RunCommand(cbf, **kwargs)
  if result.returncode:
    cros_build_lib.Die('cros_bundle_firmware failed')

  if not dest or not result.returncode:
    logging.info('Image is available at %s/out/image.bin' % outdir)
  else:
    if result.returncode:
      cros_build_lib.Die('Failed to write image to board')
    else:
      logging.info('Image written to board with %s' % ' '.join(dest + servo))


def main(argv):
  """Main function for script to build/write firmware.

  Args:
    argv: Program arguments.
  """
  options = ParseCmdline(argv)
  base = SetupBuild(options)

  with parallel.BackgroundTaskRunner(Dumper) as queue:
    RunBuild(options, base, options.target, queue)

    if options.write:
      WriteFirmware(options)

    if options.objdump:
      Log('Writing diasssembly files')
