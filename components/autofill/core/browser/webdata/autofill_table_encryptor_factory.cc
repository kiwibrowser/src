// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/webdata/autofill_table_encryptor_factory.h"

#include <memory>

#include "base/memory/singleton.h"
#include "components/autofill/core/browser/webdata/system_encryptor.h"

namespace autofill {

AutofillTableEncryptorFactory::AutofillTableEncryptorFactory() = default;

AutofillTableEncryptorFactory::~AutofillTableEncryptorFactory() = default;

AutofillTableEncryptorFactory* AutofillTableEncryptorFactory::GetInstance() {
  return base::Singleton<AutofillTableEncryptorFactory>::get();
}

std::unique_ptr<AutofillTableEncryptor>
AutofillTableEncryptorFactory::Create() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  return delegate_ ? delegate_->Create() : std::make_unique<SystemEncryptor>();
}

void AutofillTableEncryptorFactory::SetDelegate(
    std::unique_ptr<Delegate> delegate) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  delegate_ = std::move(delegate);
}

}  // namespace autofill
