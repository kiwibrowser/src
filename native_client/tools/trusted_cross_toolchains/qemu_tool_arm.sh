#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

#@ Various commands to emulate arm code using qemu
#@
#@ Note: this script is not meant to be run as
#@     tools/trusted_cross_toolchains/qemu_tool_arm.sh
#@ but rather as:
#@     toolchain/linux_x86/arm_trusted/qemu_tool_arm.sh

readonly SDK_ROOT=$(dirname $0)
readonly QEMU=${SDK_ROOT}/qemu-arm
readonly QEMU_STOCK=/usr/bin/qemu-arm
readonly QEMU_JAIL=${SDK_ROOT}

# Hook for adding stuff like timeout wrappers
readonly QEMU_PREFIX_HOOK=${QEMU_PREFIX_HOOK:-}

# NOTE: some useful debugging options for qemu:
#       env vars:
#                  QEMU_STRACE=1
#       args:
#                  -strace
#                  -d out_asm,in_asm,op,int,exec,cpu
#       c.f.  cpu_log_items in qemu-XXX/exec.c
readonly QEMU_ARGS="-cpu cortex-a9"
readonly QEMU_ARGS_DEBUG="-d in_asm,int,exec,cpu"
readonly QEMU_ARGS_DEBUG_SR="-d in_asm,int,exec,cpu,service_runtime"

######################################################################
# Helpers
######################################################################

Banner() {
  echo "######################################################################"
  echo $*
  echo "######################################################################"
}

Usage() {
  egrep "^#@" $0 | cut --bytes=3-
}


CheckPrerequisites () {
  if [[ ! -d ${QEMU_JAIL} ]] ; then
    echo "ERROR:  no proper root-jail directory found"
    exit -1
  fi
}


Hints() {
  echo
  echo "traces can be found in /tmp/qemu.log"
  echo "for faster execution disable sel_ldr validation"
  echo
}

######################################################################

#@
#@ help
#@
#@   print help for all modes
help () {
  Usage
}

#@
#@ run
#@
#@   run emulation using a locally patched qemu
run() {
  CheckPrerequisites
  exec ${QEMU_PREFIX_HOOK} ${QEMU} -L ${QEMU_JAIL} ${QEMU_ARGS} "$@"
}

#@
#@ run_stock
#@
#@   run emulation using the stock qemu
run_stock() {
  exec ${QEMU_PREFIX_HOOK} ${QEMU_STOCK} -L ${QEMU_JAIL} ${QEMU_ARGS} "$@"
}

#@
#@ run_debug
#@
#@   run emulation but also generate trace in /tmp
run_debug() {
  Hints
  CheckPrerequisites
  exec ${QEMU} -L ${QEMU_JAIL} ${QEMU_ARGS} ${QEMU_ARGS_DEBUG} "$@"
}

#@
#@ run_debug_service_runtime
#@
#@   run emulation but also generate trace in /tmp even for service_runtime
run_debug_service_runtime() {
  Hints
  CheckPrerequisites
  exec ${QEMU} -L ${QEMU_JAIL} ${QEMU_ARGS} ${QEMU_ARGS_DEBUG_SR} "$@"
}

#@
#@ install_stock
#@
#@   install stock qemu emulator (for user mode)
install_stock_qemu() {
    sudo apt-get install qemu-user
}

######################################################################
if [[ "$0" == *run_under_qemu_arm ]] ; then
  run "$@"
elif [[ $# -eq 0 ]] ; then
  echo "you must specify a mode on the commandline:"
  echo
  Usage
  exit -1
elif [[ "$(type -t $1)" != "function" ]]; then
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
else
  "$@"
fi
