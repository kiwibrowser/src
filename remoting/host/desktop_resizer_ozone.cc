// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/desktop_resizer.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"

namespace remoting {

class DesktopResizerOzone : public DesktopResizer {
 public:
  DesktopResizerOzone();
  ~DesktopResizerOzone() override;

  // DesktopResizer:
  ScreenResolution GetCurrentResolution() override;
  std::list<ScreenResolution> GetSupportedResolutions(
      const ScreenResolution& preferred) override;
  void SetResolution(const ScreenResolution& resolution) override;
  void RestoreResolution(const ScreenResolution& original) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DesktopResizerOzone);
};

DesktopResizerOzone::DesktopResizerOzone() {
}

DesktopResizerOzone::~DesktopResizerOzone() {
}

ScreenResolution DesktopResizerOzone::GetCurrentResolution() {
  NOTIMPLEMENTED();
  return ScreenResolution();
}

std::list<ScreenResolution> DesktopResizerOzone::GetSupportedResolutions(
    const ScreenResolution& preferred) {
  NOTIMPLEMENTED();
  return std::list<ScreenResolution>();
}

void DesktopResizerOzone::SetResolution(const ScreenResolution& resolution) {
  NOTIMPLEMENTED();
}

void DesktopResizerOzone::RestoreResolution(const ScreenResolution& original) {
}

std::unique_ptr<DesktopResizer> DesktopResizer::Create() {
  return base::WrapUnique(new DesktopResizerOzone);
}

}  // namespace remoting
