// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/compiler_specific.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "remoting/client/plugin/chromoting_instance.h"

namespace remoting {

class ChromotingModule : public pp::Module {
 protected:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    pp::Instance* result = new ChromotingInstance(instance);
    return result;
  }
 private:
  base::AtExitManager at_exit_manager_;
};

}  // namespace remoting

namespace pp {

// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new remoting::ChromotingModule();
}

}  // namespace pp
