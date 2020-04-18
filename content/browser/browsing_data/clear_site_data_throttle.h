// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSING_DATA_CLEAR_SITE_DATA_THROTTLE_H_
#define CONTENT_BROWSER_BROWSING_DATA_CLEAR_SITE_DATA_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/console_message_level.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace net {
class HttpResponseHeaders;
struct RedirectInfo;
class URLRequest;
}

namespace url {
class Origin;
}

namespace content {

class WebContents;

// This throttle parses the Clear-Site-Data header and executes the clearing
// of browsing data. The resource load is delayed until the header is parsed
// and, if valid, until the browsing data are deleted. See the W3C working draft
// at https://w3c.github.io/webappsec-clear-site-data/.
class CONTENT_EXPORT ClearSiteDataThrottle : public ResourceThrottle {
 public:
  // Stores and outputs console messages.
  class CONTENT_EXPORT ConsoleMessagesDelegate {
   public:
    struct Message {
      GURL url;
      std::string text;
      ConsoleMessageLevel level;
    };

    typedef base::Callback<
        void(WebContents*, ConsoleMessageLevel, const std::string&)>
        OutputFormattedMessageFunction;

    ConsoleMessagesDelegate();
    virtual ~ConsoleMessagesDelegate();

    // Logs a |text| message from |url| with |level|.
    virtual void AddMessage(const GURL& url,
                            const std::string& text,
                            ConsoleMessageLevel level);

    // Outputs stored messages to the console of WebContents identified by
    // |web_contents_getter|.
    virtual void OutputMessages(
        const ResourceRequestInfo::WebContentsGetter& web_contents_getter);

    const std::vector<Message>& messages() const { return messages_; }

   protected:
    void SetOutputFormattedMessageFunctionForTesting(
        const OutputFormattedMessageFunction& function);

   private:
    std::vector<Message> messages_;
    OutputFormattedMessageFunction output_formatted_message_function_;
  };

  // Instantiates a throttle for the given if it's supported for the given
  // |request|. The caller must guarantee that |request| outlives the throttle.
  static std::unique_ptr<ResourceThrottle> MaybeCreateThrottleForRequest(
      net::URLRequest* request);

  ~ClearSiteDataThrottle() override;

  // ResourceThrottle implementation:
  const char* GetNameForLogging() const override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  void WillProcessResponse(bool* defer) override;

  // Exposes ParseHeader() publicly for testing.
  static bool ParseHeaderForTesting(const std::string& header,
                                    bool* clear_cookies,
                                    bool* clear_storage,
                                    bool* clear_cache,
                                    ConsoleMessagesDelegate* delegate,
                                    const GURL& current_url);

 protected:
  ClearSiteDataThrottle(net::URLRequest* request,
                        std::unique_ptr<ConsoleMessagesDelegate> delegate);

  virtual const GURL& GetCurrentURL() const;

 private:
  // Returns HTTP response headers of the underlying URLRequest.
  // Can be overriden for testing.
  virtual const net::HttpResponseHeaders* GetResponseHeaders() const;

  // Scans for the first occurrence of the 'Clear-Site-Data' header, calls
  // ParseHeader() to parse it, and then ExecuteClearingTask() if applicable.
  // This is the common logic of WillRedirectRequest()
  // and WillProcessResponse(). Returns true if a valid header was found and
  // the clearing was executed.
  bool HandleHeader();

  // Parses the value of the 'Clear-Site-Data' header and outputs whether
  // the header requests to |clear_cookies|, |clear_storage|, and |clear_cache|.
  // The |delegate| will be filled with messages to be output in the console,
  // prepended by the |current_url|. Returns true if parsing was successful.
  static bool ParseHeader(const std::string& header,
                          bool* clear_cookies,
                          bool* clear_storage,
                          bool* clear_cache,
                          ConsoleMessagesDelegate* delegate,
                          const GURL& current_url);

  // Executes the clearing task. Can be overriden for testing.
  virtual void ExecuteClearingTask(const url::Origin& origin,
                                   bool clear_cookies,
                                   bool clear_storage,
                                   bool clear_cache,
                                   base::OnceClosure callback);

  // Signals that a parsing and deletion task was finished.
  void TaskFinished();

  // Outputs the console messages in the |delegate_|.
  void OutputConsoleMessages();

  // The request this throttle is observing.
  net::URLRequest* request_;

  // The delegate that stores and outputs console messages.
  std::unique_ptr<ConsoleMessagesDelegate> delegate_;

  // The time when the last clearing operation started. Used when clearing
  // finishes to compute the duration.
  base::TimeTicks clearing_started_;

  // Needed for asynchronous parsing and deletion tasks.
  base::WeakPtrFactory<ClearSiteDataThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ClearSiteDataThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSING_DATA_CLEAR_SITE_DATA_THROTTLE_H_
