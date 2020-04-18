// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_UTILS_H_
#define COMPONENTS_UPDATE_CLIENT_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "components/update_client/update_client.h"

class GURL;

namespace base {
class DictionaryValue;
class FilePath;
}

namespace net {
class URLFetcher;
class URLFetcherDelegate;
class URLRequestContextGetter;
}

namespace update_client {

class Component;
struct CrxComponent;

// Defines a name-value pair that represents an installer attribute.
// Installer attributes are component-specific metadata, which may be serialized
// in an update check request.
using InstallerAttribute = std::pair<std::string, std::string>;

// Sends a protocol request to the the service endpoint specified by |url|.
// The body of the request is provided by |protocol_request| and it is
// expected to contain XML data. The caller owns the returned object.
std::unique_ptr<net::URLFetcher> SendProtocolRequest(
    const GURL& url,
    const std::map<std::string, std::string>& protocol_request_extra_headers,
    const std::string& protocol_request,
    net::URLFetcherDelegate* url_fetcher_delegate,
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter);

// Returns true if the url request of |fetcher| was succesful.
bool FetchSuccess(const net::URLFetcher& fetcher);

// Returns the error code which occured during the fetch. The function returns 0
// if the fetch was successful. If errors happen, the function could return a
// network error, an http response code, or the status of the fetch, if the
// fetch is pending or canceled.
int GetFetchError(const net::URLFetcher& fetcher);

// Returns true if the |component| contains a valid differential update url.
bool HasDiffUpdate(const Component& component);

// Returns true if the |status_code| represents a server error 5xx.
bool IsHttpServerError(int status_code);

// Deletes the file and its directory, if the directory is empty. If the
// parent directory is not empty, the function ignores deleting the directory.
// Returns true if the file and the empty directory are deleted.
bool DeleteFileAndEmptyParentDirectory(const base::FilePath& filepath);

// Returns the component id of the |component|. The component id is in a
// format similar with the format of an extension id.
std::string GetCrxComponentID(const CrxComponent& component);

// Returns true if the actual SHA-256 hash of the |filepath| matches the
// |expected_hash|.
bool VerifyFileHash256(const base::FilePath& filepath,
                       const std::string& expected_hash);

// Returns true if the |brand| parameter matches ^[a-zA-Z]{4}?$ .
bool IsValidBrand(const std::string& brand);

// Returns true if the name part of the |attr| parameter matches
// ^[-_a-zA-Z0-9]{1,256}$ and the value part of the |attr| parameter
// matches ^[-.,;+_=a-zA-Z0-9]{0,256}$ .
bool IsValidInstallerAttribute(const InstallerAttribute& attr);

// Removes the unsecure urls in the |urls| parameter.
void RemoveUnsecureUrls(std::vector<GURL>* urls);

// Adapter function for the old definitions of CrxInstaller::Install until the
// component installer code is migrated to use a Result instead of bool.
CrxInstaller::Result InstallFunctionWrapper(
    base::OnceCallback<bool()> callback);

// Deserializes the CRX manifest. The top level must be a dictionary.
std::unique_ptr<base::DictionaryValue> ReadManifest(
    const base::FilePath& unpack_path);

// Converts a custom, specific installer error (and optionally extended error)
// to an installer result.
template <typename T>
CrxInstaller::Result ToInstallerResult(const T& error, int extended_error = 0) {
  static_assert(std::is_enum<T>::value,
                "Use an enum class to define custom installer errors");
  return CrxInstaller::Result(
      static_cast<int>(update_client::InstallError::CUSTOM_ERROR_BASE) +
          static_cast<int>(error),
      extended_error);
}

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_UTILS_H_
