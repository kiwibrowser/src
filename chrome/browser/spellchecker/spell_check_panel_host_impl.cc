// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spell_check_panel_host_impl.h"

#include "base/bind.h"
#include "components/spellcheck/browser/spellcheck_platform.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

SpellCheckPanelHostImpl::SpellCheckPanelHostImpl() = default;

SpellCheckPanelHostImpl::~SpellCheckPanelHostImpl() = default;

// static
void SpellCheckPanelHostImpl::Create(
    spellcheck::mojom::SpellCheckPanelHostRequest request) {
  mojo::MakeStrongBinding(std::make_unique<SpellCheckPanelHostImpl>(),
                          std::move(request));
}

void SpellCheckPanelHostImpl::ShowSpellingPanel(bool show) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  spellcheck_platform::ShowSpellingPanel(show);
}

void SpellCheckPanelHostImpl::UpdateSpellingPanelWithMisspelledWord(
    const base::string16& word) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  spellcheck_platform::UpdateSpellingPanelWithMisspelledWord(word);
}
