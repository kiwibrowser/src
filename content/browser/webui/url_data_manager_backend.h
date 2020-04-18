// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBUI_URL_DATA_MANAGER_BACKEND_H_
#define CONTENT_BROWSER_WEBUI_URL_DATA_MANAGER_BACKEND_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/supports_user_data.h"
#include "base/values.h"
#include "content/browser/webui/url_data_manager.h"
#include "content/public/browser/url_data_source.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_job_factory.h"

class GURL;

namespace base {
class RefCountedMemory;
}

namespace content {

class ChromeBlobStorageContext;
class ResourceContext;
class URLDataManagerBackend;
class URLDataSourceImpl;
class URLRequestChromeJob;

// URLDataManagerBackend is used internally by ChromeURLDataManager on the IO
// thread. In most cases you can use the API in ChromeURLDataManager and ignore
// this class. URLDataManagerBackend is owned by ResourceContext.
class URLDataManagerBackend : public base::SupportsUserData::Data {
 public:
  typedef int RequestID;

  URLDataManagerBackend();
  ~URLDataManagerBackend() override;

  // Invoked to create the protocol handler for chrome://. Called on the UI
  // thread.
  CONTENT_EXPORT static std::unique_ptr<
      net::URLRequestJobFactory::ProtocolHandler>
  CreateProtocolHandler(ResourceContext* resource_context,
                        ChromeBlobStorageContext* blob_storage_context);

  // Adds a DataSource to the collection of data sources.
  void AddDataSource(URLDataSourceImpl* source);

  void UpdateWebUIDataSource(const std::string& source_name,
                             const base::DictionaryValue& update);

  // DataSource invokes this. Sends the data to the URLRequest. |bytes| may be
  // null, which signals an error handling the request.
  void DataAvailable(RequestID request_id, base::RefCountedMemory* bytes);

  // Look up the data source for the request. Returns the source if it is found,
  // else NULL.
  URLDataSourceImpl* GetDataSourceFromURL(const GURL& url);

  // Creates and sets the response headers for the given request.
  static scoped_refptr<net::HttpResponseHeaders> GetHeaders(
      URLDataSourceImpl* source,
      const std::string& path,
      const std::string& origin);

  // Returns whether |url| passes some sanity checks and is a valid GURL.
  static bool CheckURLIsValid(const GURL& url);

  // Parse |url| to get the path which will be used to resolve the request. The
  // path is the remaining portion after the scheme and hostname.
  static void URLToRequestPath(const GURL& url, std::string* path);

  // Check if the given integer is a valid network error code.
  static bool IsValidNetworkErrorCode(int error_code);

  // Returns the schemes that are used by WebUI (i.e. the set from content and
  // its embedder).
  static std::vector<std::string> GetWebUISchemes();

 private:
  friend class URLRequestChromeJob;

  typedef std::map<std::string,
      scoped_refptr<URLDataSourceImpl> > DataSourceMap;
  typedef std::map<RequestID, URLRequestChromeJob*> PendingRequestMap;

  // Called by the job when it's starting up.
  // Returns false if |url| is not a URL managed by this object.
  bool StartRequest(const net::URLRequest* request, URLRequestChromeJob* job);

  // Helper function to call StartDataRequest on |source|'s delegate. This is
  // needed because while we want to call URLDataSourceDelegate's method, we
  // need to add a refcount on the source.
  static void CallStartRequest(
      scoped_refptr<URLDataSourceImpl> source,
      const std::string& path,
      const ResourceRequestInfo::WebContentsGetter& wc_getter,
      int request_id);

  // Remove a request from the list of pending requests.
  void RemoveRequest(URLRequestChromeJob* job);

  // Returns true if the job exists in |pending_requests_|. False otherwise.
  // Called by ~URLRequestChromeJob to verify that |pending_requests_| is kept
  // up to date.
  bool HasPendingJob(URLRequestChromeJob* job) const;

  // Custom sources of data, keyed by source path (e.g. "favicon").
  DataSourceMap data_sources_;

  // All pending URLRequestChromeJobs, keyed by ID of the request.
  // URLRequestChromeJob calls into this object when it's constructed and
  // destructed to ensure that the pointers in this map remain valid.
  PendingRequestMap pending_requests_;

  // The ID we'll use for the next request we receive.
  RequestID next_request_id_;

  DISALLOW_COPY_AND_ASSIGN(URLDataManagerBackend);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBUI_URL_DATA_MANAGER_BACKEND_H_
