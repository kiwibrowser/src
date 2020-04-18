// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Singly or multiply-included shared traits file depending on circumstances.
// This allows the use of printing IPC serialization macros in more than one IPC
// message file.
#ifndef COMPONENTS_PRINTING_COMMON_PRINTING_PARAM_TRAITS_MACROS_H_
#define COMPONENTS_PRINTING_COMMON_PRINTING_PARAM_TRAITS_MACROS_H_

#include "ipc/ipc_message_macros.h"
#include "printing/print_job_constants.h"

// TODO(dgn) move all those macros back to
// components/printing/common/print_messages.h when they can be handled by a
// single generator. (main tracking bug: crbug.com/450822)
IPC_ENUM_TRAITS_MAX_VALUE(printing::MarginType,
                          printing::MARGIN_TYPE_LAST)
IPC_ENUM_TRAITS_MIN_MAX_VALUE(printing::DuplexMode,
                              printing::UNKNOWN_DUPLEX_MODE,
                              printing::SHORT_EDGE)

#endif  // COMPONENTS_PRINTING_COMMON_PRINTING_PARAM_TRAITS_MACROS_H_
