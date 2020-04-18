// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRERENDER_PRERENDER_CONTENTS_H_
#define CHROME_BROWSER_PRERENDER_PRERENDER_CONTENTS_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/prerender/prerender_final_status.h"
#include "chrome/browser/prerender/prerender_origin.h"
#include "chrome/common/prerender.mojom.h"
#include "chrome/common/prerender_types.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/referrer.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "ui/gfx/geometry/rect.h"

class Profile;

namespace base {
class ProcessMetrics;
}

namespace content {
class RenderViewHost;
class SessionStorageNamespace;
class WebContents;
}

namespace history {
struct HistoryAddPageArgs;
}

namespace memory_instrumentation {
class GlobalMemoryDump;
}

namespace prerender {

class PrerenderManager;
class PrerenderResourceThrottle;

class PrerenderContents : public content::NotificationObserver,
                          public content::WebContentsObserver,
                          public chrome::mojom::PrerenderCanceler {
 public:
  // PrerenderContents::Create uses the currently registered Factory to create
  // the PrerenderContents. Factory is intended for testing.
  class Factory {
   public:
    Factory() {}
    virtual ~Factory() {}

    // Ownership is not transfered through this interface as prerender_manager
    // and profile are stored as weak pointers.
    virtual PrerenderContents* CreatePrerenderContents(
        PrerenderManager* prerender_manager,
        Profile* profile,
        const GURL& url,
        const content::Referrer& referrer,
        Origin origin) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Factory);
  };

  class Observer {
   public:
    // Signals that the prerender has started running.
    virtual void OnPrerenderStart(PrerenderContents* contents) {}

    // Signals that the prerender has had its load event.
    virtual void OnPrerenderStopLoading(PrerenderContents* contents) {}

    // Signals that the prerender has had its 'DOMContentLoaded' event.
    virtual void OnPrerenderDomContentLoaded(PrerenderContents* contents) {}

    // Signals that the prerender has stopped running. A PrerenderContents with
    // an unset final status will always call OnPrerenderStop before being
    // destroyed.
    virtual void OnPrerenderStop(PrerenderContents* contents) {}

    // Signals that a resource finished loading and altered the running byte
    // count.
    virtual void OnPrerenderNetworkBytesChanged(PrerenderContents* contents) {}

   protected:
    Observer() {}
    virtual ~Observer() = 0;
  };

  ~PrerenderContents() override;

  // All observers of a PrerenderContents are removed after the OnPrerenderStop
  // event is sent, so there is no need to call RemoveObserver() in the normal
  // use case.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool Init();

  // Set the mode of this contents. This must be called before prerender has
  // started.
  void SetPrerenderMode(PrerenderMode mode);
  PrerenderMode prerender_mode() const { return prerender_mode_; }

  static Factory* CreateFactory();

  // Returns a PrerenderContents from the given web_contents, if it's used for
  // prerendering. Otherwise returns NULL. Handles a NULL input for
  // convenience.
  static PrerenderContents* FromWebContents(content::WebContents* web_contents);

  // Start rendering the contents in the prerendered state. If
  // |is_control_group| is true, this will go through some of the mechanics of
  // starting a prerender, without actually creating the RenderView. |bounds|
  // indicates the rectangle that the prerendered page should be in.
  // |session_storage_namespace| indicates the namespace that the prerendered
  // page should be part of.
  virtual void StartPrerendering(
      const gfx::Rect& bounds,
      content::SessionStorageNamespace* session_storage_namespace);

  // Verifies that the prerendering is not using too many resources, and kills
  // it if not.
  void DestroyWhenUsingTooManyResources();

  content::RenderViewHost* GetRenderViewHostMutable();
  const content::RenderViewHost* GetRenderViewHost() const;

  PrerenderManager* prerender_manager() { return prerender_manager_; }

  base::string16 title() const { return title_; }
  const GURL& prerender_url() const { return prerender_url_; }
  const content::Referrer& referrer() const { return referrer_; }
  bool has_stopped_loading() const { return has_stopped_loading_; }
  bool has_finished_loading() const { return has_finished_loading_; }
  bool prerendering_has_started() const { return prerendering_has_started_; }

  // Sets the parameter to the value of the associated RenderViewHost's child id
  // and returns a boolean indicating the validity of that id.
  virtual bool GetChildId(int* child_id) const;

  // Sets the parameter to the value of the associated RenderViewHost's route id
  // and returns a boolean indicating the validity of that id.
  virtual bool GetRouteId(int* route_id) const;

  FinalStatus final_status() const { return final_status_; }

  Origin origin() const { return origin_; }

  base::TimeTicks load_start_time() const { return load_start_time_; }

  // Indicates whether this prerendered page can be used for the provided
  // |url| and |session_storage_namespace|.
  bool Matches(
      const GURL& url,
      const content::SessionStorageNamespace* session_storage_namespace) const;

  // content::WebContentsObserver implementation.
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void DidStopLoading() override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void RenderProcessGone(base::TerminationStatus status) override;
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

  // content::NotificationObserver
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Checks that a URL may be prerendered, for one of the many redirections. If
  // the URL can not be prerendered - for example, it's an ftp URL - |this| will
  // be destroyed and false is returned. Otherwise, true is returned.
  virtual bool CheckURL(const GURL& url);

  // Adds an alias URL. If the URL can not be prerendered, |this| will be
  // destroyed and false is returned.
  bool AddAliasURL(const GURL& url);

  // The prerender WebContents (may be NULL).
  content::WebContents* prerender_contents() const {
    return prerender_contents_.get();
  }

  std::unique_ptr<content::WebContents> ReleasePrerenderContents();

  // Sets the final status, calls OnDestroy and adds |this| to the
  // PrerenderManager's pending deletes list.
  void Destroy(FinalStatus reason);

  // Called by the history tab helper with the information that it woudl have
  // added to the history service had this web contents not been used for
  // prerendering.
  void DidNavigate(const history::HistoryAddPageArgs& add_page_args);

  // Applies all the URL history encountered during prerendering to the
  // new tab.
  void CommitHistory(content::WebContents* tab);

  std::unique_ptr<base::DictionaryValue> GetAsValue() const;

  // Marks prerender as used and releases any throttled resource requests.
  void PrepareForUse();

  // Called when a PrerenderResourceThrottle defers a request. If the prerender
  // is used it'll be resumed on the IO thread, otherwise they will get
  // cancelled automatically if prerendering is cancelled.
  void AddResourceThrottle(
      const base::WeakPtr<PrerenderResourceThrottle>& throttle);

  // Called when a PrerenderResourceThrottle changes a resource priority to
  // net::IDLE. The resources are reset back to their original priorities when
  // the prerender contents is swapped in.
  void AddIdleResource(
      const base::WeakPtr<PrerenderResourceThrottle>& throttle);

  // Increments the number of bytes fetched over the network for this prerender.
  void AddNetworkBytes(int64_t bytes);

  bool prerendering_has_been_cancelled() const {
    return prerendering_has_been_cancelled_;
  }

  // Running byte count. Increased when each resource completes loading.
  int64_t network_bytes() { return network_bytes_; }

 protected:
  PrerenderContents(PrerenderManager* prerender_manager,
                    Profile* profile,
                    const GURL& url,
                    const content::Referrer& referrer,
                    Origin origin);

  // Set the final status for how the PrerenderContents was used. This
  // should only be called once, and should be called before the prerender
  // contents are destroyed.
  void SetFinalStatus(FinalStatus final_status);

  // These call out to methods on our Observers, using our observer_list_. Note
  // that NotifyPrerenderStop() also clears the observer list.
  void NotifyPrerenderStart();
  void NotifyPrerenderStopLoading();
  void NotifyPrerenderDomContentLoaded();
  void NotifyPrerenderStop();

  // Called whenever a RenderViewHost is created for prerendering.  Only called
  // once the RenderViewHost has a RenderView and RenderWidgetHostView.
  virtual void OnRenderViewHostCreated(
      content::RenderViewHost* new_render_view_host);

  content::NotificationRegistrar& notification_registrar() {
    return notification_registrar_;
  }

  std::unique_ptr<content::WebContents> CreateWebContents(
      content::SessionStorageNamespace* session_storage_namespace);

  PrerenderMode prerender_mode_;
  bool prerendering_has_started_;

  // Time at which we started to load the URL.  This is used to compute
  // the time elapsed from initiating a prerender until the time the
  // (potentially only partially) prerendered page is shown to the user.
  base::TimeTicks load_start_time_;

  // The prerendered WebContents; may be null.
  std::unique_ptr<content::WebContents> prerender_contents_;

  // The session storage namespace id for use in matching. We must save it
  // rather than get it from the RenderViewHost since in the control group
  // we won't have a RenderViewHost.
  std::string session_storage_namespace_id_;

 private:
  class WebContentsDelegateImpl;

  // Needs to be able to call the constructor.
  friend class PrerenderContentsFactoryImpl;

  // Returns the ProcessMetrics for the render process, if it exists.
  void DidGetMemoryUsage(
      bool success,
      std::unique_ptr<memory_instrumentation::GlobalMemoryDump> dump);

  // chrome::mojom::PrerenderCanceler:
  void CancelPrerenderForPrinting() override;
  void CancelPrerenderForUnsupportedMethod() override;
  void CancelPrerenderForUnsupportedScheme(const GURL& url) override;
  void CancelPrerenderForSyncDeferredRedirect() override;

  void OnPrerenderCancelerRequest(
      chrome::mojom::PrerenderCancelerRequest request);

  mojo::Binding<chrome::mojom::PrerenderCanceler> prerender_canceler_binding_;

  base::ObserverList<Observer> observer_list_;

  // The prerender manager owning this object.
  PrerenderManager* prerender_manager_;

  // The URL being prerendered.
  GURL prerender_url_;

  // The referrer.
  content::Referrer referrer_;

  // The profile being used
  Profile* profile_;

  // Information about the title and URL of the page that this class as a
  // RenderViewHostDelegate has received from the RenderView.
  // Used to apply to the new RenderViewHost delegate that might eventually
  // own the contained RenderViewHost when the prerendered page is shown
  // in a WebContents.
  base::string16 title_;
  GURL url_;
  content::NotificationRegistrar notification_registrar_;

  // A vector of URLs that this prerendered page matches against.
  // This array can contain more than element as a result of redirects,
  // such as HTTP redirects or javascript redirects.
  std::vector<GURL> alias_urls_;

  bool has_stopped_loading_;

  // True when the main frame has finished loading.
  bool has_finished_loading_;

  FinalStatus final_status_;

  // Tracks whether or not prerendering has been cancelled by calling Destroy.
  // Used solely to prevent double deletion.
  bool prerendering_has_been_cancelled_;

  // Pid of the render process associated with the RenderViewHost for this
  // object.
  base::ProcessId process_pid_;

  std::unique_ptr<WebContentsDelegateImpl> web_contents_delegate_;

  // These are -1 before a RenderView is created.
  int child_id_;
  int route_id_;

  // Origin for this prerender.
  Origin origin_;

  // The bounds of the WebView from the launching page.
  gfx::Rect bounds_;

  typedef std::vector<history::HistoryAddPageArgs> AddPageVector;

  // Caches pages to be added to the history.
  AddPageVector add_page_vector_;

  // Resources that are throttled, pending a prerender use. Can only access a
  // throttle on the IO thread.
  std::vector<base::WeakPtr<PrerenderResourceThrottle> > resource_throttles_;
  // Resources for which the priority was lowered to net::IDLE.
  std::vector<base::WeakPtr<PrerenderResourceThrottle>> idle_resources_;

  // A running tally of the number of bytes this prerender has caused to be
  // transferred over the network for resources.  Updated with AddNetworkBytes.
  int64_t network_bytes_;

  service_manager::BinderRegistry registry_;

  base::WeakPtrFactory<PrerenderContents> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrerenderContents);
};

}  // namespace prerender

#endif  // CHROME_BROWSER_PRERENDER_PRERENDER_CONTENTS_H_
