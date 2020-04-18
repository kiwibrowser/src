/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/arch/x86_32/nacl_switch_all_regs_32.h"

#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


void NaClSwitchRemainingRegsSetup(struct NaClSwitchRemainingRegsState *state,
                                  struct NaClAppThread *natp,
                                  const struct NaClSignalContext *regs) {
  natp->user.gs_segment.new_prog_ctr = regs->prog_ctr;
  natp->user.gs_segment.new_ecx = regs->ecx;

  state->stack_ptr = regs->stack_ptr;
  state->ss = natp->user.ss;
  state->spring_addr = natp->nap->all_regs_springboard.start_addr;
  state->cs = natp->user.cs;
  state->ds = natp->user.ds;
  state->es = natp->user.es;
  state->fs = natp->user.fs;
  state->gs = natp->user.gs;
}
