// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_SMS_SERVICE_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_SMS_SERVICE_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}

class OAuth2TokenService;
class SigninManagerBase;

// Provides an API for querying a logged in users's verified phone number,
// and sending a predetermined promotional SMS to that number.  This class is
// based heavily on WebHistoryService's implementation to query Google services.
class SMSService : public KeyedService {
 public:
  class Request {
   public:
    virtual ~Request();

    virtual bool IsPending() = 0;

    // Returns the response code received from the server, which will only be
    // valid if the request succeeded.
    virtual int GetResponseCode() = 0;

    // Returns the contents of the response body received from the server.
    virtual const std::string& GetResponseBody() = 0;

    virtual void SetPostData(const std::string& post_data) = 0;

    virtual void SetPostDataAndType(const std::string& post_data,
                                    const std::string& mime_type) = 0;

    // Tells the request to begin.
    virtual void Start() = 0;

   protected:
    Request();
  };

  typedef base::Callback<
      void(Request*, bool success, const std::string& number)>
      PhoneNumberCallback;
  typedef base::Callback<void(Request*, bool success)> CompletionCallback;

  SMSService(OAuth2TokenService* token_service,
             SigninManagerBase* signin_manager,
             scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SMSService() override;

  // Query the logged in user's verified phone number.
  virtual void QueryPhoneNumber(const PhoneNumberCallback& callback);

  // Send an SMS to the logged in user's verified phone number.  The text of
  // the SMS is determined by |promo_id|.
  virtual void SendSMS(const std::string& promo_id,
                       const SMSService::PhoneNumberCallback& callback);

 protected:
  void QueryPhoneNumberCompletionCallback(
      const SMSService::PhoneNumberCallback& callback,
      SMSService::Request* request,
      bool success);

  void SendSMSCallback(const SMSService::PhoneNumberCallback& callback,
                       SMSService::Request* request,
                       bool success);

 private:
  virtual Request* CreateRequest(const GURL& url,
                                 const CompletionCallback& callback);

  // Stores pointer to OAuth2TokenService and SigninManagerBase instance. They
  // must outlive the SMSService and can be null during
  // tests.
  OAuth2TokenService* token_service_;
  SigninManagerBase* signin_manager_;

  // Request context getter to use.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Pending expiration requests to be canceled if not complete by profile
  // shutdown.
  std::map<Request*, std::unique_ptr<Request>> pending_requests_;

  base::WeakPtrFactory<SMSService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SMSService);
};

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_SMS_SERVICE_H_
