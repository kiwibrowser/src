// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_FETCHER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_FETCHER_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/loader/modulescript/module_script_creation_params.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"

namespace blink {

class ConsoleMessage;

// ModuleScriptFetcher is an abstract class to fetch module scripts. Derived
// classes are expected to fetch a module script for the given FetchParameters
// and return its client a fetched resource as ModuleScriptCreationParams.
class CORE_EXPORT ModuleScriptFetcher
    : public GarbageCollectedFinalized<ModuleScriptFetcher> {
 public:
  class CORE_EXPORT Client : public GarbageCollectedMixin {
   public:
    virtual void NotifyFetchFinished(
        const base::Optional<ModuleScriptCreationParams>&,
        const HeapVector<Member<ConsoleMessage>>& error_messages) = 0;
  };

  ModuleScriptFetcher() = default;
  virtual ~ModuleScriptFetcher() = default;

  // Takes a non-const reference to FetchParameters because
  // ScriptResource::Fetch() requires it.
  virtual void Fetch(FetchParameters&, Client*) = 0;

  virtual void Trace(blink::Visitor*);

 protected:
  void NotifyFetchFinished(const base::Optional<ModuleScriptCreationParams>&,
                           const HeapVector<Member<ConsoleMessage>>&);

  void SetClient(Client*);

 private:
  Member<Client> client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_MODULESCRIPT_MODULE_SCRIPT_FETCHER_H_
