// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPELLCHECKER_TEST_SPELLCHECK_MOCK_PANEL_HOST_H_
#define CHROME_BROWSER_SPELLCHECKER_TEST_SPELLCHECK_MOCK_PANEL_HOST_H_

#include "base/callback.h"
#include "components/spellcheck/common/spellcheck_panel.mojom.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace spellcheck {

class SpellCheckMockPanelHost : public spellcheck::mojom::SpellCheckPanelHost {
 public:
  explicit SpellCheckMockPanelHost(content::RenderProcessHost* process_host);
  ~SpellCheckMockPanelHost() override;

  content::RenderProcessHost* process_host() const { return process_host_; }

  bool SpellingPanelVisible();
  void SpellCheckPanelHostRequest(
      spellcheck::mojom::SpellCheckPanelHostRequest request);

 private:
  // spellcheck::mojom::SpellCheckPanelHost:
  void ShowSpellingPanel(bool show) override;
  void UpdateSpellingPanelWithMisspelledWord(
      const base::string16& word) override;

  mojo::BindingSet<spellcheck::mojom::SpellCheckPanelHost> bindings_;
  content::RenderProcessHost* process_host_;
  bool show_spelling_panel_called_ = false;
  bool spelling_panel_visible_ = false;
  base::OnceClosure quit_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckMockPanelHost);
};
}  // namespace spellcheck

#endif  // CHROME_BROWSER_SPELLCHECKER_TEST_SPELLCHECK_MOCK_PANEL_HOST_H_
