// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/cast_messages.h"

namespace IPC {

void ParamTraits<media::cast::RtpTimeTicks>::Write(base::Pickle* m,
                                                   const param_type& p) {
  ParamTraits<uint64_t>::Write(m, p.SerializeForIPC());
}

bool ParamTraits<media::cast::RtpTimeTicks>::Read(const base::Pickle* m,
                                                  base::PickleIterator* iter,
                                                  param_type* r) {
  uint64_t serialized = UINT64_C(0);
  if (ParamTraits<uint64_t>::Read(m, iter, &serialized)) {
    *r = param_type::DeserializeForIPC(serialized);
    return true;
  }
  return false;
}

void ParamTraits<media::cast::RtpTimeTicks>::Log(const param_type& p,
                                                 std::string* l) {
  std::ostringstream oss;
  oss << p;
  l->append(oss.str());
}

void ParamTraits<media::cast::FrameId>::Write(base::Pickle* m,
                                              const param_type& p) {
  ParamTraits<uint64_t>::Write(m, p.SerializeForIPC());
}

bool ParamTraits<media::cast::FrameId>::Read(const base::Pickle* m,
                                             base::PickleIterator* iter,
                                             param_type* r) {
  uint64_t serialized = UINT64_C(0);
  if (ParamTraits<uint64_t>::Read(m, iter, &serialized)) {
    *r = param_type::DeserializeForIPC(serialized);
    return true;
  }
  return false;
}

void ParamTraits<media::cast::FrameId>::Log(const param_type& p,
                                            std::string* l) {
  std::ostringstream oss;
  oss << p;
  l->append(oss.str());
}

}  // namespace IPC
