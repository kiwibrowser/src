// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_UTIL_WIN_SHELL_UTIL_WIN_IMPL_H_
#define CHROME_SERVICES_UTIL_WIN_SHELL_UTIL_WIN_IMPL_H_

#include "base/macros.h"
#include "chrome/services/util_win/public/mojom/shell_util_win.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

class ShellUtilWinImpl : public chrome::mojom::ShellUtilWin {
 public:
  explicit ShellUtilWinImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~ShellUtilWinImpl() override;

 private:
  // chrome::mojom::ShellUtilWin:
  void IsPinnedToTaskbar(IsPinnedToTaskbarCallback callback) override;

  void CallGetOpenFileName(
      uint32_t owner,
      uint32_t flags,
      const std::vector<std::tuple<base::string16, base::string16>>& filters,
      const base::FilePath& initial_directory,
      const base::FilePath& initial_filename,
      CallGetOpenFileNameCallback callback) override;

  void CallGetSaveFileName(
      uint32_t owner,
      uint32_t flags,
      const std::vector<std::tuple<base::string16, base::string16>>& filters,
      uint32_t one_based_filter_index,
      const base::FilePath& initial_directory,
      const base::FilePath& suggested_filename,
      const base::string16& default_extension,
      CallGetSaveFileNameCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(ShellUtilWinImpl);
};

#endif  // CHROME_SERVICES_UTIL_WIN_SHELL_UTIL_WIN_IMPL_H_
