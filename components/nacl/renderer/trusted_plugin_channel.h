// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_RENDERER_TRUSTED_PLUGIN_CHANNEL_H_
#define COMPONENTS_NACL_RENDERER_TRUSTED_PLUGIN_CHANNEL_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "components/nacl/common/nacl.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "ppapi/c/pp_instance.h"

namespace nacl {
class NexeLoadManager;

class TrustedPluginChannel : public mojom::NaClRendererHost {
 public:
  TrustedPluginChannel(NexeLoadManager* nexe_load_manager,
                       mojom::NaClRendererHostRequest request,
                       bool is_helper_nexe);
  ~TrustedPluginChannel() override;

 private:
  void OnChannelError();

  // mojom::NaClRendererHost overrides.
  void ReportExitStatus(int exit_status,
                        ReportExitStatusCallback callback) override;
  void ReportLoadStatus(NaClErrorCode load_status,
                        ReportLoadStatusCallback callback) override;
  void ProvideExitControl(mojom::NaClExitControlPtr exit_control) override;

  // Non-owning pointer. This is safe because the TrustedPluginChannel is owned
  // by the NexeLoadManager pointed to here.
  NexeLoadManager* nexe_load_manager_;
  mojo::Binding<mojom::NaClRendererHost> binding_;
  mojom::NaClExitControlPtr exit_control_;
  const bool is_helper_nexe_;

  DISALLOW_COPY_AND_ASSIGN(TrustedPluginChannel);
};

}  // namespace nacl

#endif  // COMPONENTS_NACL_RENDERER_TRUSTED_PLUGIN_CHANNEL_H_
