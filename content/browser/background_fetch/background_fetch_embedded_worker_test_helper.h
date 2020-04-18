// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_EMBEDDED_WORKER_TEST_HELPER_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_EMBEDDED_WORKER_TEST_HELPER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"

namespace content {

struct BackgroundFetchSettledFetch;

namespace mojom {
enum class BackgroundFetchState;
}

// Extension of the EmbeddedWorkerTestHelper that enables instrumentation of the
// events related to the Background Fetch API. Storage for these tests will
// always be kept in memory, as data persistence is tested elsewhere.
class BackgroundFetchEmbeddedWorkerTestHelper
    : public EmbeddedWorkerTestHelper {
 public:
  BackgroundFetchEmbeddedWorkerTestHelper();
  ~BackgroundFetchEmbeddedWorkerTestHelper() override;

  // Toggles whether the named Service Worker event should fail.
  void set_fail_abort_event(bool fail) { fail_abort_event_ = fail; }
  void set_fail_click_event(bool fail) { fail_click_event_ = fail; }
  void set_fail_fetch_fail_event(bool fail) { fail_fetch_fail_event_ = fail; }
  void set_fail_fetched_event(bool fail) { fail_fetched_event_ = fail; }

  // Sets a base::Callback that should be executed when the named event is ran.
  void set_abort_event_closure(const base::Closure& closure) {
    abort_event_closure_ = closure;
  }
  void set_click_event_closure(const base::Closure& closure) {
    click_event_closure_ = closure;
  }
  void set_fetch_fail_event_closure(const base::Closure& closure) {
    fetch_fail_event_closure_ = closure;
  }
  void set_fetched_event_closure(const base::Closure& closure) {
    fetched_event_closure_ = closure;
  }

  const base::Optional<std::string>& last_developer_id() const {
    return last_developer_id_;
  }
  const base::Optional<std::string>& last_unique_id() const {
    return last_unique_id_;
  }
  const base::Optional<mojom::BackgroundFetchState>& last_state() const {
    return last_state_;
  }
  const base::Optional<std::vector<BackgroundFetchSettledFetch>> last_fetches()
      const {
    return last_fetches_;
  }

 protected:
  // EmbeddedWorkerTestHelper overrides:
  void OnBackgroundFetchAbortEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchAbortEventCallback callback) override;
  void OnBackgroundFetchClickEvent(
      const std::string& developer_id,
      mojom::BackgroundFetchState state,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchClickEventCallback callback) override;
  void OnBackgroundFetchFailEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchFailEventCallback callback) override;
  void OnBackgroundFetchedEvent(
      const std::string& developer_id,
      const std::string& unique_id,
      const std::vector<BackgroundFetchSettledFetch>& fetches,
      mojom::ServiceWorkerEventDispatcher::
          DispatchBackgroundFetchedEventCallback callback) override;

 private:
  bool fail_abort_event_ = false;
  bool fail_click_event_ = false;
  bool fail_fetch_fail_event_ = false;
  bool fail_fetched_event_ = false;

  base::Closure abort_event_closure_;
  base::Closure click_event_closure_;
  base::Closure fetch_fail_event_closure_;
  base::Closure fetched_event_closure_;

  base::Optional<std::string> last_developer_id_;
  base::Optional<std::string> last_unique_id_;
  base::Optional<mojom::BackgroundFetchState> last_state_;
  base::Optional<std::vector<BackgroundFetchSettledFetch>> last_fetches_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchEmbeddedWorkerTestHelper);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_EMBEDDED_WORKER_TEST_HELPER_H_
