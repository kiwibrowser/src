// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPELLCHECKER_TEST_SPELLCHECK_CONTENT_BROWSER_CLIENT_H_
#define CHROME_BROWSER_SPELLCHECKER_TEST_SPELLCHECK_CONTENT_BROWSER_CLIENT_H_

#include <vector>

#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/spellchecker/test/spellcheck_mock_panel_host.h"

namespace spellcheck {
class SpellCheckContentBrowserClient : public ChromeContentBrowserClient {
 public:
  SpellCheckContentBrowserClient();
  ~SpellCheckContentBrowserClient() override;

  // ContentBrowserClient overrides.
  void OverrideOnBindInterface(
      const service_manager::BindSourceInfo& remote_info,
      const std::string& name,
      mojo::ScopedMessagePipeHandle* handle) override;

  SpellCheckMockPanelHost* GetSpellCheckMockPanelHostForProcess(
      content::RenderProcessHost* render_process_host) const;

  void RunUntilBind();

 private:
  void BindSpellCheckPanelHostRequest(
      spellcheck::mojom::SpellCheckPanelHostRequest request,
      const service_manager::BindSourceInfo& source_info);

  base::OnceClosure quit_on_bind_closure_;
  std::vector<std::unique_ptr<SpellCheckMockPanelHost>> hosts_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckContentBrowserClient);
};
}  // namespace spellcheck

#endif  // CHROME_BROWSER_SPELLCHECKER_TEST_SPELLCHECK_CONTENT_BROWSER_CLIENT_H_
