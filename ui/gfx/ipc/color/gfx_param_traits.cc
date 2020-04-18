// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/ipc/color/gfx_param_traits.h"

#include "ui/gfx/color_space.h"
#include "ui/gfx/ipc/color/gfx_param_traits_macros.h"

namespace IPC {

void ParamTraits<gfx::ColorSpace>::Write(base::Pickle* m,
                                         const gfx::ColorSpace& p) {
  WriteParam(m, p.primaries_);
  WriteParam(m, p.transfer_);
  WriteParam(m, p.matrix_);
  WriteParam(m, p.range_);
  WriteParam(m, p.icc_profile_id_);
  if (p.primaries_ == gfx::ColorSpace::PrimaryID::CUSTOM) {
    m->WriteBytes(reinterpret_cast<const char*>(p.custom_primary_matrix_),
                  sizeof(p.custom_primary_matrix_));
  }
  if (p.transfer_ == gfx::ColorSpace::TransferID::CUSTOM) {
    m->WriteBytes(reinterpret_cast<const char*>(p.custom_transfer_params_),
                  sizeof(p.custom_transfer_params_));
  }
}

bool ParamTraits<gfx::ColorSpace>::Read(const base::Pickle* m,
                                        base::PickleIterator* iter,
                                        gfx::ColorSpace* r) {
  if (!ReadParam(m, iter, &r->primaries_))
    return false;
  if (!ReadParam(m, iter, &r->transfer_))
    return false;
  if (!ReadParam(m, iter, &r->matrix_))
    return false;
  if (!ReadParam(m, iter, &r->range_))
    return false;
  if (!ReadParam(m, iter, &r->icc_profile_id_))
    return false;
  if (r->primaries_ == gfx::ColorSpace::PrimaryID::CUSTOM) {
    const char* data = nullptr;
    if (!iter->ReadBytes(&data, sizeof(r->custom_primary_matrix_)))
      return false;
    memcpy(r->custom_primary_matrix_, data, sizeof(r->custom_primary_matrix_));
  }
  if (r->transfer_ == gfx::ColorSpace::TransferID::CUSTOM) {
    const char* data = nullptr;
    if (!iter->ReadBytes(&data, sizeof(r->custom_transfer_params_)))
      return false;
    memcpy(r->custom_transfer_params_, data,
           sizeof(r->custom_transfer_params_));
  }
  return true;
}

void ParamTraits<gfx::ColorSpace>::Log(const gfx::ColorSpace& p,
                                       std::string* l) {
  l->append("<gfx::ColorSpace>");
}

void ParamTraits<gfx::ICCProfile>::Write(base::Pickle* m,
                                         const gfx::ICCProfile& p) {
  if (p.internals_) {
    WriteParam(m, p.internals_->id_);
    WriteParam(m, p.internals_->data_);
  } else {
    uint64_t id = 0;
    std::vector<char> data;
    WriteParam(m, id);
    WriteParam(m, data);
  }
}

bool ParamTraits<gfx::ICCProfile>::Read(const base::Pickle* m,
                                        base::PickleIterator* iter,
                                        gfx::ICCProfile* r) {
  uint64_t id;
  std::vector<char> data;
  if (!ReadParam(m, iter, &id))
    return false;
  if (!ReadParam(m, iter, &data))
    return false;
  *r = gfx::ICCProfile::FromDataWithId(data.data(), data.size(), id);
  return true;
}

void ParamTraits<gfx::ICCProfile>::Log(const gfx::ICCProfile& p,
                                       std::string* l) {
  l->append("<gfx::ICCProfile>");
}

}  // namespace IPC

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef UI_GFX_IPC_COLOR_GFX_PARAM_TRAITS_MACROS_H_
#include "ui/gfx/ipc/color/gfx_param_traits_macros.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef UI_GFX_IPC_COLOR_GFX_PARAM_TRAITS_MACROS_H_
#include "ui/gfx/ipc/color/gfx_param_traits_macros.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef UI_GFX_IPC_COLOR_GFX_PARAM_TRAITS_MACROS_H_
#include "ui/gfx/ipc/color/gfx_param_traits_macros.h"
}  // namespace IPC
