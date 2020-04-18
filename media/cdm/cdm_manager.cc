// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/cdm_manager.h"

#include <utility>

#include "base/logging.h"
#include "media/base/content_decryption_module.h"

namespace media {

CdmManager::CdmManager() = default;

CdmManager::~CdmManager() = default;

// static
CdmManager* CdmManager::GetInstance() {
  static CdmManager* manager = new CdmManager();
  return manager;
}

scoped_refptr<ContentDecryptionModule> CdmManager::GetCdm(int cdm_id) {
  base::AutoLock lock(lock_);
  auto iter = cdm_map_.find(cdm_id);
  return iter == cdm_map_.end() ? nullptr : iter->second;
}

void CdmManager::RegisterCdm(int cdm_id,
                             scoped_refptr<ContentDecryptionModule> cdm) {
  base::AutoLock lock(lock_);
  DCHECK(!cdm_map_.count(cdm_id));
  cdm_map_[cdm_id] = cdm;
}

void CdmManager::UnregisterCdm(int cdm_id) {
  base::AutoLock lock(lock_);
  DCHECK(cdm_map_.count(cdm_id));
  cdm_map_.erase(cdm_id);
}

}  // namespace media
