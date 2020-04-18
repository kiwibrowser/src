// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_REMOVER_H_
#define CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_REMOVER_H_

#include "chrome/browser/chromeos/crostini/crostini_manager.h"

namespace crostini {

class CrostiniRemover : public crostini::CrostiniManager::RestartObserver,
                        public base::RefCountedThreadSafe<CrostiniRemover> {
 public:
  CrostiniRemover(Profile* profile,
                  std::string vm_name,
                  std::string container_name,
                  CrostiniManager::RemoveCrostiniCallback callback);

  void RemoveCrostini();

  // crostini::CrostiniManager::RestartObserver
  void OnComponentLoaded(crostini::ConciergeClientResult result) override {}
  void OnConciergeStarted(crostini::ConciergeClientResult result) override;
  void OnDiskImageCreated(crostini::ConciergeClientResult result) override {}
  void OnVmStarted(crostini::ConciergeClientResult result) override {}

 private:
  friend class base::RefCountedThreadSafe<CrostiniRemover>;
  ~CrostiniRemover() override;
  void OnRestartCrostini(crostini::ConciergeClientResult result);
  void StopVmFinished(crostini::ConciergeClientResult result);
  void DestroyDiskImageFinished(crostini::ConciergeClientResult result);
  void StopConciergeFinished(bool success);

  crostini::CrostiniManager::RestartId restart_id_ =
      crostini::CrostiniManager::kUninitializedRestartId;
  Profile* profile_;
  std::string vm_name_;
  std::string container_name_;
  CrostiniManager::RemoveCrostiniCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(CrostiniRemover);
};

}  // namespace crostini

#endif  // CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_REMOVER_H_
