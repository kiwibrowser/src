// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CONTENT_PREVIEWS_UI_SERVICE_H_
#define COMPONENTS_PREVIEWS_CONTENT_PREVIEWS_UI_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/previews/content/previews_io_data.h"
#include "components/previews/content/previews_optimization_guide.h"
#include "components/previews/core/previews_black_list.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_logger.h"
#include "components/previews/core/previews_opt_out_store.h"

class GURL;

namespace base {
class SingleThreadTaskRunner;
}

namespace previews {
class PreviewsIOData;

// A class to manage the UI portion of inter-thread communication between
// previews/ objects. Created and used on the UI thread.
class PreviewsUIService {
 public:
  PreviewsUIService(
      PreviewsIOData* previews_io_data,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      std::unique_ptr<PreviewsOptOutStore> previews_opt_out_store,
      std::unique_ptr<PreviewsOptimizationGuide> previews_opt_guide,
      const PreviewsIsEnabledCallback& is_enabled_callback,
      std::unique_ptr<PreviewsLogger> logger);
  virtual ~PreviewsUIService();

  // Sets |io_data_| to |io_data| to allow calls from the UI thread to the IO
  // thread. Virtualized in testing.
  virtual void SetIOData(base::WeakPtr<PreviewsIOData> io_data);

  // Adds a navigation to |url| to the black list with result |opt_out|.
  void AddPreviewNavigation(const GURL& url,
                            PreviewsType type,
                            bool opt_out,
                            uint64_t page_id);

  // Clears the history of the black list between |begin_time| and |end_time|.
  void ClearBlackList(base::Time begin_time, base::Time end_time);

  // Notifies |logger_| that |host| has been blacklisted at |time|. Virtualized
  // in testing.
  virtual void OnNewBlacklistedHost(const std::string& host, base::Time time);

  // Notifies |logger_| that the user blacklisted state has changed. Where
  // |blacklisted| is the new user blacklisted status. Virtualized in testing.
  virtual void OnUserBlacklistedStatusChange(bool blacklisted);

  // Notifies |logger_| that the blacklist is cleared at |time|. Virtualized in
  // testing.
  virtual void OnBlacklistCleared(base::Time time);

  // Change the status of whether to ignored or consider PreviewsBlackList
  // decisions in |io_data_|. This method is called when users interact with the
  // UI (i.e. click on the "Ignore Blacklist" button). Virtualized in testing.
  virtual void SetIgnorePreviewsBlacklistDecision(bool ignored);

  // Notifies |logger_| whether PreviewsBlackList decisions are ignored or not.
  // This method is listening for notification from PreviewsIOData for when the
  // blacklist ignore status is changed so that |logger_| can update all
  // PreviewsLoggerObservers so that multiple instances of the page have the
  // same status. Virtualized in testing.
  virtual void OnIgnoreBlacklistDecisionStatusChanged(bool ignored);

  // Log the navigation to PreviewsLogger. Virtualized in testing.
  virtual void LogPreviewNavigation(const GURL& url,
                                    PreviewsType type,
                                    bool opt_out,
                                    base::Time time,
                                    uint64_t page_id);

  // Log the determined previews eligibility decision |reason| to the
  // PreviewsLogger. |passed_reasons| is a collection of reason codes that
  // correspond to eligibility checks that were satisfied prior to determining
  // |reason| and so the opposite of these |passed_reasons| codes was true.
  // The method takes ownership of |passed_reasons|. |page_id| is generated
  // by PreviewsIOData, and used to group decisions into groups on the page,
  // messages that don't need to be grouped can pass in 0 as page_id.
  // Virtualized in testing.
  virtual void LogPreviewDecisionMade(
      PreviewsEligibilityReason reason,
      const GURL& url,
      base::Time time,
      PreviewsType type,
      std::vector<PreviewsEligibilityReason>&& passed_reasons,
      uint64_t page_id);

  // Expose the pointer to PreviewsLogger to extract logging messages. This
  // pointer's life time is the same as of |this|, and it is guaranteed to not
  // return null.
  PreviewsLogger* previews_logger() const;

 private:
  // The IO thread portion of the inter-thread communication for previews/.
  base::WeakPtr<previews::PreviewsIOData> io_data_;

  base::ThreadChecker thread_checker_;

  // The IO thread task runner. Used to post tasks to |io_data_|.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // A log object to keep track of events such as previews navigations,
  // blacklist actions, etc.
  std::unique_ptr<PreviewsLogger> logger_;
  base::WeakPtrFactory<PreviewsUIService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsUIService);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CONTENT_PREVIEWS_UI_SERVICE_H_
