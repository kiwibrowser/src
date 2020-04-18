// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A UrlInfo object is used to store prediction related information about a host
// port and scheme triplet.  When performing DNS pre-resolution of the host/port
// pair, its state is monitored as it is resolved.
// It includes progress, from placement in the Predictor's queue, to resolution
// by the DNS service as either FOUND or NO_SUCH_NAME.  Each instance may also
// hold records of previous resolution times, which might later be shown to be
// savings relative to resolution time during a navigation.
// UrlInfo objects are also used to describe frames, and additional instances
// may describe associated subresources, for future speculative connections to
// those expected subresources.

#ifndef CHROME_BROWSER_NET_URL_INFO_H_
#define CHROME_BROWSER_NET_URL_INFO_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "net/base/host_port_pair.h"
#include "url/gurl.h"

namespace chrome_browser_net {

// Use command line switch to enable detailed logging.
void EnablePredictorDetailedLog(bool enable);

class UrlInfo {
 public:
  // Reasons for a domain to be resolved.
  enum ResolutionMotivation {
    MOUSE_OVER_MOTIVATED,  // Mouse-over link induced resolution.
    PAGE_SCAN_MOTIVATED,   // Scan of rendered page induced resolution.
    UNIT_TEST_MOTIVATED,
    LINKED_MAX_MOTIVATED,    // enum demarkation above motivation from links.
    OMNIBOX_MOTIVATED,       // Omni-box suggested resolving this.
    STARTUP_LIST_MOTIVATED,  // Startup list caused this resolution.
    EARLY_LOAD_MOTIVATED,    // In some cases we use the prefetcher to warm up
                             // the connection in advance of issuing the real
                             // request.

    NO_PREFETCH_MOTIVATION,  // Browser navigation info (not prefetch related).

    // The following involve predictive prefetching, triggered by a navigation.
    // TODO(jar): Support STATIC_REFERAL_MOTIVATED API and integration.
    STATIC_REFERAL_MOTIVATED,  // External database suggested this resolution.
    LEARNED_REFERAL_MOTIVATED,  // Prior navigation taught us this resolution.
    SELF_REFERAL_MOTIVATED,  // Guess about need for a second connection.

    MAX_MOTIVATED  // Beyond all enums, for use in histogram bounding.
  };

  enum DnsProcessingState {
      // When processed by our prefetching system, the states are:
      PENDING,       // Constructor has completed.
      QUEUED,        // In name queue but not yet being resolved.
      ASSIGNED,      // Being resolved (or being reset to earlier state)
      ASSIGNED_BUT_MARKED,  // Needs to be deleted as soon as it's resolved.
      FOUND,         // DNS resolution completed.
      NO_SUCH_NAME,  // DNS resolution completed.
  };

  typedef std::vector<UrlInfo> UrlInfoTable;

  static base::TimeDelta NullDuration() {
    return base::TimeDelta::FromMilliseconds(-1);
  }

  // UrlInfo are usually made by the default constructor during
  // initializing of the Predictor's map (of info for Hostnames).
  UrlInfo();

  UrlInfo(const UrlInfo& other);

  ~UrlInfo();

  // NeedDnsUpdate decides, based on our internal info,
  // if it would be valuable to attempt to update (prefectch)
  // DNS data for hostname.  This decision is based
  // on how recently we've done DNS prefetching for hostname.
  bool NeedsDnsUpdate();

  // FOR TEST ONLY: The following access the otherwise constant values.
  static void set_cache_expiration(base::TimeDelta time);
  static base::TimeDelta get_cache_expiration();

  // The prefetching lifecycle.
  void SetQueuedState(ResolutionMotivation motivation);
  void SetAssignedState();
  void RemoveFromQueue();
  void SetPendingDeleteState();
  void SetFoundState();
  void SetNoSuchNameState();

  // Finish initialization. Must only be called once.
  void SetUrl(const GURL& url);

  bool was_linked() const { return was_linked_; }

  bool was_found() const { return FOUND == state_; }
  bool was_nonexistent() const { return NO_SUCH_NAME == state_; }
  bool is_assigned() const {
    return ASSIGNED == state_ || ASSIGNED_BUT_MARKED == state_;
  }
  bool is_marked_to_delete() const { return ASSIGNED_BUT_MARKED == state_; }

  bool HasUrl(const GURL& url) const {
    return url_ == url;
  }

  base::TimeDelta resolve_duration() const { return resolve_duration_;}
  base::TimeDelta queue_duration() const { return queue_duration_;}

  void DLogResultsStats(const char* message) const;

  static void GetHtmlTable(const UrlInfoTable& host_infos,
                           const char* description,
                           bool brief,
                           std::string* output);

  // For testing, and use in printing tables of info, we sometimes need to
  // adjust the time manually.  Usually, this value is maintained by state
  // transition, and this call is not made.
  void set_time(const base::TimeTicks& time) { time_ = time; }

 private:
  base::TimeDelta GetDuration() {
    base::TimeTicks old_time = time_;
    time_ = base::TimeTicks::Now();
    return time_ - old_time;
  }

  // IsStillCached() guesses if the DNS cache still has IP data.
  bool IsStillCached() const;

  // Record why we created, or have updated (reqested pre-resolution) of this
  // instance.
  void SetMotivation(ResolutionMotivation motivation);

  // Helper function for about:dns printing.
  std::string GetAsciiMotivation() const;

  // The current state of this instance.
  DnsProcessingState state_;

  // Record the state prior to going to a queued state, in case we have to back
  // out of the queue.
  DnsProcessingState old_prequeue_state_;

  GURL url_;  // Host, port and scheme for this info.

  // When was last state changed (usually lookup completed).
  base::TimeTicks time_;
  // Time needed for DNS to resolve.
  base::TimeDelta resolve_duration_;
  // Time spent in queue.
  base::TimeDelta queue_duration_;

  int sequence_number_;  // Used to calculate potential of cache eviction.
  static int sequence_counter;  // Used to allocate sequence_number_'s.

  // Motivation for creation of this instance.
  ResolutionMotivation motivation_;

  // Record if the motivation for prefetching was ever a page-link-scan.
  bool was_linked_;

  // We put these objects into a std::map, and hence we
  // need some "evil" constructors.
  // DISALLOW_COPY_AND_ASSIGN(UrlInfo);
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_URL_INFO_H_
