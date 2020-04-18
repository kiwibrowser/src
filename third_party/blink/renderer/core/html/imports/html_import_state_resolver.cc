/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/imports/html_import_state_resolver.h"

#include "third_party/blink/renderer/core/html/imports/html_import.h"
#include "third_party/blink/renderer/core/html/imports/html_import_child.h"
#include "third_party/blink/renderer/core/html/imports/html_import_loader.h"

namespace blink {

inline bool HTMLImportStateResolver::IsBlockingFollowers(HTMLImport* import) {
  if (!import->IsSync())
    return false;
  HTMLImportChild* child = ToHTMLImportChild(import);
  if (!child->Loader()->IsFirstImport(child))
    return false;
  return !import->GetState().IsReady();
}

inline bool HTMLImportStateResolver::ShouldBlockScriptExecution() const {
  // FIXME: Memoize to make this faster.
  for (HTMLImport* ancestor = import_; ancestor;
       ancestor = ancestor->Parent()) {
    for (HTMLImport* predecessor = ancestor->Previous(); predecessor;
         predecessor = predecessor->Previous()) {
      if (IsBlockingFollowers(predecessor))
        return true;
    }
  }

  for (HTMLImport* child = import_->FirstChild(); child;
       child = child->Next()) {
    if (IsBlockingFollowers(child))
      return true;
  }

  return false;
}

inline bool HTMLImportStateResolver::IsActive() const {
  return !import_->HasFinishedLoading();
}

HTMLImportState HTMLImportStateResolver::Resolve() const {
  if (ShouldBlockScriptExecution())
    return HTMLImportState(HTMLImportState::kBlockingScriptExecution);
  if (IsActive())
    return HTMLImportState(HTMLImportState::kActive);
  return HTMLImportState(HTMLImportState::kReady);
}

}  // namespace blink
