// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LAUNCHER_SEARCH_PROVIDER_ERROR_REPORTER_H_
#define CHROME_BROWSER_CHROMEOS_LAUNCHER_SEARCH_PROVIDER_ERROR_REPORTER_H_

#include <memory>
#include <string>

#include "base/macros.h"

namespace content {
class RenderFrameHost;
}

namespace chromeos {
namespace launcher_search_provider {

// A utility class which sends error message to developer console.
class ErrorReporter {
 public:
  explicit ErrorReporter(content::RenderFrameHost* host);
  virtual ~ErrorReporter();

  // Shows |message| as warning in the developer console of the extension.
  virtual void Warn(const std::string& message);

  // Duplicate the instance. Since ErrorReporter is handled as scoped_ptr in the
  // code, we need this to duplicate error reporter to set it to each result.
  virtual std::unique_ptr<ErrorReporter> Duplicate();

 private:
  // Not owned.
  content::RenderFrameHost* host_;

  DISALLOW_COPY_AND_ASSIGN(ErrorReporter);
};

}  // namespace launcher_search_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LAUNCHER_SEARCH_PROVIDER_ERROR_REPORTER_H_
