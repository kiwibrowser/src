// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_RESOURCE_H_
#define CC_RESOURCES_RESOURCE_H_

#include "base/logging.h"
#include "base/macros.h"
#include "cc/cc_export.h"
#include "cc/resources/resource_provider.h"
#include "cc/resources/resource_util.h"
#include "ui/gfx/geometry/size.h"

namespace cc {

class CC_EXPORT Resource {
 public:
  Resource() : id_(0), format_(viz::RGBA_8888) {}
  Resource(unsigned id,
           const gfx::Size& size,
           viz::ResourceFormat format,
           const gfx::ColorSpace& color_space)
      : id_(id), size_(size), format_(format), color_space_(color_space) {}

  viz::ResourceId id() const { return id_; }
  const gfx::Size& size() const { return size_; }
  viz::ResourceFormat format() const { return format_; }
  const gfx::ColorSpace& color_space() const { return color_space_; }

 protected:
  void set_id(viz::ResourceId id) { id_ = id; }
  void set_dimensions(const gfx::Size& size, viz::ResourceFormat format) {
    size_ = size;
    format_ = format;
  }
  void set_color_space(const gfx::ColorSpace& color_space) {
    color_space_ = color_space;
  }

 private:
  viz::ResourceId id_;
  gfx::Size size_;
  viz::ResourceFormat format_;
  gfx::ColorSpace color_space_;

  DISALLOW_COPY_AND_ASSIGN(Resource);
};

}  // namespace cc

#endif  // CC_RESOURCES_RESOURCE_H_
