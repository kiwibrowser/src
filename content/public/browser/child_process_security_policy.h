// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_CHILD_PROCESS_SECURITY_POLICY_H_
#define CONTENT_PUBLIC_BROWSER_CHILD_PROCESS_SECURITY_POLICY_H_

#include <string>

#include "content/common/content_export.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace base {
class FilePath;
}

namespace content {

// The ChildProcessSecurityPolicy class is used to grant and revoke security
// capabilities for child processes.  For example, it restricts whether a child
// process is permitted to load file:// URLs based on whether the process
// has ever been commanded to load file:// URLs by the browser.
//
// ChildProcessSecurityPolicy is a singleton that may be used on any thread.
//
class ChildProcessSecurityPolicy {
 public:
  virtual ~ChildProcessSecurityPolicy() {}

  // There is one global ChildProcessSecurityPolicy object for the entire
  // browser process.  The object returned by this method may be accessed on
  // any thread.
  static CONTENT_EXPORT ChildProcessSecurityPolicy* GetInstance();

  // Web-safe schemes can be requested by any child process.  Once a web-safe
  // scheme has been registered, any child process can request URLs whose
  // origins use that scheme. There is no mechanism for revoking web-safe
  // schemes.
  //
  // Only call this function if URLs of this scheme are okay to host in
  // any ordinary renderer process.
  //
  // Registering 'your-scheme' as web-safe also causes 'blob:your-scheme://'
  // and 'filesystem:your-scheme://' URLs to be considered web-safe.
  virtual void RegisterWebSafeScheme(const std::string& scheme) = 0;

  // More restrictive variant of RegisterWebSafeScheme; URLs with this scheme
  // may be requested by any child process, but navigations to this scheme may
  // only commit in child processes that have been explicitly granted
  // permission to do so.
  //
  // |always_allow_in_origin_headers| controls whether this scheme is allowed to
  // appear as the Origin HTTP header in outbound requests, even if the
  // originating process does not have permission to commit this scheme. This
  // may be necessary if the scheme is used in conjunction with blink's
  // IsolatedWorldSecurityOrigin mechanism, as for extension content scripts.
  virtual void RegisterWebSafeIsolatedScheme(
      const std::string& scheme,
      bool always_allow_in_origin_headers) = 0;

  // Returns true iff |scheme| has been registered as a web-safe scheme.
  // TODO(nick): https://crbug.com/651534 This function does not have enough
  // information to render an appropriate judgment for blob and filesystem URLs;
  // change it to accept an URL instead.
  virtual bool IsWebSafeScheme(const std::string& scheme) = 0;

  // This permission grants only read access to a file.
  // Whenever the user picks a file from a <input type="file"> element, the
  // browser should call this function to grant the child process the capability
  // to upload the file to the web. Grants FILE_PERMISSION_READ_ONLY.
  virtual void GrantReadFile(int child_id, const base::FilePath& file) = 0;

  // This permission grants creation, read, and full write access to a file,
  // including attributes.
  virtual void GrantCreateReadWriteFile(int child_id,
                                        const base::FilePath& file) = 0;

  // This permission grants copy-into permission for |dir|.
  virtual void GrantCopyInto(int child_id, const base::FilePath& dir) = 0;

  // This permission grants delete permission for |dir|.
  virtual void GrantDeleteFrom(int child_id, const base::FilePath& dir) = 0;

  // Determine whether the process has the capability to request the URL.
  // Before servicing a child process's request for a URL, the content layer
  // calls this method to determine whether it is safe.
  virtual bool CanRequestURL(int child_id, const GURL& url) = 0;

  // Whether the process is allowed to commit a document from the given URL.
  // This is more restrictive than CanRequestURL, since CanRequestURL allows
  // requests that might lead to cross-process navigations or external protocol
  // handlers.
  virtual bool CanCommitURL(int child_id, const GURL& url) = 0;

  // These methods verify whether or not the child process has been granted
  // permissions perform these functions on |file|.

  // Before servicing a child process's request to upload a file to the web, the
  // browser should call this method to determine whether the process has the
  // capability to upload the requested file.
  virtual bool CanReadFile(int child_id, const base::FilePath& file) = 0;
  virtual bool CanCreateReadWriteFile(int child_id,
                                      const base::FilePath& file) = 0;

  // Grants read access permission to the given isolated file system
  // identified by |filesystem_id|. An isolated file system can be
  // created for a set of native files/directories (like dropped files)
  // using storage::IsolatedContext. A child process needs to be granted
  // permission to the file system to access the files in it using
  // file system URL. You do NOT need to give direct permission to
  // individual file paths.
  //
  // Note: files/directories in the same file system share the same
  // permission as far as they are accessed via the file system, i.e.
  // using the file system URL (tip: you can create a new file system
  // to give different permission to part of files).
  virtual void GrantReadFileSystem(int child_id,
                                   const std::string& filesystem_id) = 0;

  // Grants write access permission to the given isolated file system
  // identified by |filesystem_id|.  See comments for GrantReadFileSystem
  // for more details.  You do NOT need to give direct permission to
  // individual file paths.
  //
  // This must be called with a great care as this gives write permission
  // to all files/directories included in the file system.
  virtual void GrantWriteFileSystem(int child_id,
                                    const std::string& filesystem_id) = 0;

  // Grants create file permission to the given isolated file system
  // identified by |filesystem_id|.  See comments for GrantReadFileSystem
  // for more details.  You do NOT need to give direct permission to
  // individual file paths.
  //
  // This must be called with a great care as this gives create permission
  // within all directories included in the file system.
  virtual void GrantCreateFileForFileSystem(
      int child_id,
      const std::string& filesystem_id) = 0;

  // Grants create, read and write access permissions to the given isolated
  // file system identified by |filesystem_id|.  See comments for
  // GrantReadFileSystem for more details.  You do NOT need to give direct
  // permission to individual file paths.
  //
  // This must be called with a great care as this gives create, read and write
  // permissions to all files/directories included in the file system.
  virtual void GrantCreateReadWriteFileSystem(
      int child_id,
      const std::string& filesystem_id) = 0;

  // Grants permission to copy-into filesystem |filesystem_id|. 'copy-into'
  // is used to allow copying files into the destination filesystem without
  // granting more general create and write permissions.
  virtual void GrantCopyIntoFileSystem(int child_id,
                                       const std::string& filesystem_id) = 0;

  // Grants permission to delete from filesystem |filesystem_id|. 'delete-from'
  // is used to allow deleting files into the destination filesystem without
  // granting more general create and write permissions.
  virtual void GrantDeleteFromFileSystem(int child_id,
                                         const std::string& filesystem_id) = 0;

  // Grants the child process the capability to access URLs with the provided
  // origin.
  virtual void GrantOrigin(int child_id, const url::Origin& origin) = 0;

  // Grants the child process the capability to access URLs of the provided
  // scheme.
  virtual void GrantScheme(int child_id, const std::string& scheme) = 0;

  // Returns true if read access has been granted to |filesystem_id|.
  virtual bool CanReadFileSystem(int child_id,
                                 const std::string& filesystem_id) = 0;

  // Returns true if read and write access has been granted to |filesystem_id|.
  virtual bool CanReadWriteFileSystem(int child_id,
                                      const std::string& filesystem_id) = 0;

  // Returns true if copy-into access has been granted to |filesystem_id|.
  virtual bool CanCopyIntoFileSystem(int child_id,
                                     const std::string& filesystem_id) = 0;

  // Returns true if delete-from access has been granted to |filesystem_id|.
  virtual bool CanDeleteFromFileSystem(int child_id,
                                       const std::string& filesystem_id) = 0;

  // Returns true if the specified child_id has been granted WebUI bindings.
  // The browser should check this property before assuming the child process
  // is allowed to use WebUI bindings.
  virtual bool HasWebUIBindings(int child_id) = 0;

  // Grants permission to send system exclusive message to any MIDI devices.
  virtual void GrantSendMidiSysExMessage(int child_id) = 0;

  // Returns true if the process is permitted to read and modify the data for
  // the origin of |url|. This is currently used to protect data such as
  // cookies, passwords, and local storage. Does not affect cookies attached to
  // or set by network requests.
  //
  // This can only return false for processes locked to a particular origin,
  // which can happen for any origin when the --site-per-process flag is used,
  // or for isolated origins that require a dedicated process (see
  // AddIsolatedOrigin).
  virtual bool CanAccessDataForOrigin(int child_id, const GURL& url) = 0;

  // Returns true if GrantOrigin was called earlier with the same parameters.
  //
  // TODO(alexmos): This currently exists to support checking whether a
  // <webview> guest process has permission to request blob URLs in its
  // embedder's origin on the IO thread.  This should be removed once that
  // check is superseded by a UI thread check.  See https://crbug.com/656752.
  virtual bool HasSpecificPermissionForOrigin(int child_id,
                                              const url::Origin& origin) = 0;

  // This function will check whether |origin| requires process isolation, and
  // if so, it will return true and put the most specific matching isolated
  // origin into |result|.
  //
  // Such origins may be registered with the --isolate-origins command-line
  // flag, via features::IsolateOrigins, via an IsolateOrigins enterprise
  // policy, or by a content/ embedder using
  // ContentBrowserClient::GetOriginsRequiringDedicatedProcess().
  //
  // If |origin| does not require process isolation, this function will return
  // false, and |result| will be a unique origin. This means that neither
  // |origin|, nor any origins for which |origin| is a subdomain, have been
  // registered as isolated origins.
  //
  // For example, if both https://isolated.com/ and
  // https://bar.foo.isolated.com/ are registered as isolated origins, then the
  // values returned in |result| are:
  //   https://isolated.com/             -->  https://isolated.com/
  //   https://foo.isolated.com/         -->  https://isolated.com/
  //   https://bar.foo.isolated.com/     -->  https://bar.foo.isolated.com/
  //   https://baz.bar.foo.isolated.com/ -->  https://bar.foo.isolated.com/
  //   https://unisolated.com/           -->  (unique origin)
  virtual bool GetMatchingIsolatedOrigin(const url::Origin& origin,
                                         url::Origin* result) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_CHILD_PROCESS_SECURITY_POLICY_H_
