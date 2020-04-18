// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_BROWSER_IMPL_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_BROWSER_IMPL_H_

#include "headless/public/headless_browser.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/web_contents.h"
#include "headless/lib/browser/headless_devtools_manager_delegate.h"
#include "headless/lib/browser/headless_web_contents_impl.h"
#include "headless/public/headless_export.h"
#include "headless/public/util/moveable_auto_lock.h"

namespace net {
class NetLog;
}  // namespace net

namespace ui {
class Compositor;
}  // namespace ui

namespace headless {

class HeadlessBrowserContextImpl;
class HeadlessBrowserMainParts;

extern const base::FilePath::CharType kDefaultProfileName[];

// Exported for tests.
class HEADLESS_EXPORT HeadlessBrowserImpl : public HeadlessBrowser,
                                            public HeadlessDevToolsTarget {
 public:
  HeadlessBrowserImpl(
      base::OnceCallback<void(HeadlessBrowser*)> on_start_callback,
      HeadlessBrowser::Options options);
  ~HeadlessBrowserImpl() override;

  // HeadlessBrowser implementation:
  HeadlessBrowserContext::Builder CreateBrowserContextBuilder() override;
  scoped_refptr<base::SingleThreadTaskRunner> BrowserIOThread() const override;
  scoped_refptr<base::SingleThreadTaskRunner> BrowserMainThread()
      const override;

  void Shutdown() override;

  std::vector<HeadlessBrowserContext*> GetAllBrowserContexts() override;
  HeadlessWebContents* GetWebContentsForDevToolsAgentHostId(
      const std::string& devtools_agent_host_id) override;
  HeadlessBrowserContext* GetBrowserContextForId(
      const std::string& id) override;
  void SetDefaultBrowserContext(
      HeadlessBrowserContext* browser_context) override;
  HeadlessBrowserContext* GetDefaultBrowserContext() override;
  HeadlessDevToolsTarget* GetDevToolsTarget() override;

  // HeadlessDevToolsTarget implementation:
  void AttachClient(HeadlessDevToolsClient* client) override;
  void DetachClient(HeadlessDevToolsClient* client) override;
  bool IsAttached() override;

  void set_browser_main_parts(HeadlessBrowserMainParts* browser_main_parts);
  HeadlessBrowserMainParts* browser_main_parts() const;

  void PreMainMessageLoopRun();
  void RunOnStartCallback();

  HeadlessBrowser::Options* options() { return &options_; }
  net::NetLog* net_log() const { return net_log_.get(); }

  HeadlessBrowserContext* CreateBrowserContext(
      HeadlessBrowserContext::Builder* builder);
  // Close given |browser_context| and delete it
  // (all web contents associated with it go away too).
  void DestroyBrowserContext(HeadlessBrowserContextImpl* browser_context);

  HeadlessWebContentsImpl* GetWebContentsForWindowId(const int window_id);

  base::WeakPtr<HeadlessBrowserImpl> GetWeakPtr();

  // Returns the corresponding HeadlessBrowserContextImpl or null if one can't
  // be found. Can be called on any thread.
  LockedPtr<HeadlessBrowserContextImpl> GetBrowserContextForRenderFrame(
      int render_process_id,
      int render_frame_id) const;

  // All the methods that begin with Platform need to be implemented by the
  // platform specific headless implementation.
  // Helper for one time initialization of application
  void PlatformInitialize();
  void PlatformStart();
  void PlatformInitializeWebContents(HeadlessWebContentsImpl* web_contents);
  void PlatformSetWebContentsBounds(HeadlessWebContentsImpl* web_contents,
                                    const gfx::Rect& bounds);
  ui::Compositor* PlatformGetCompositor(HeadlessWebContentsImpl* web_contents);

 protected:
  base::OnceCallback<void(HeadlessBrowser*)> on_start_callback_;
  HeadlessBrowser::Options options_;
  std::unique_ptr<net::NetLog> net_log_;
  HeadlessBrowserMainParts* browser_main_parts_;  // Not owned.

  mutable base::Lock browser_contexts_lock_;  // Protects |browser_contexts_|
  base::flat_map<std::string, std::unique_ptr<HeadlessBrowserContextImpl>>
      browser_contexts_;
  HeadlessBrowserContext* default_browser_context_;  // Not owned.

  scoped_refptr<content::DevToolsAgentHost> agent_host_;

  base::WeakPtrFactory<HeadlessBrowserImpl> weak_ptr_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(HeadlessBrowserImpl);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_BROWSER_IMPL_H_
