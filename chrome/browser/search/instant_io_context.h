// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_INSTANT_IO_CONTEXT_H_
#define CHROME_BROWSER_SEARCH_INSTANT_IO_CONTEXT_H_

#include <set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"

class GURL;

namespace content {
class ResourceContext;
}

// IO thread data held for Instant.  This reflects the data held in
// InstantService for use on the IO thread.  Owned by ResourceContext
// as user data.
class InstantIOContext : public base::RefCountedThreadSafe<InstantIOContext> {
 public:
  InstantIOContext();

  // Key name for context UserData.  UserData is created by InstantService
  // but accessed by InstantIOContext.
  static const char kInstantIOContextKeyName[];

  // Installs the |instant_io_context| into the UserData of the
  // |resource_context|.
  static void SetUserDataOnIO(
      content::ResourceContext* resource_context,
      scoped_refptr<InstantIOContext> instant_io_context);

  // Add and remove RenderProcessHost IDs that are associated with Instant
  // processes.  Used to keep process IDs in sync with InstantService.
  static void AddInstantProcessOnIO(
      scoped_refptr<InstantIOContext> instant_io_context,
      int process_id);
  static void RemoveInstantProcessOnIO(
      scoped_refptr<InstantIOContext> instant_io_context,
      int process_id);
  static void ClearInstantProcessesOnIO(
      scoped_refptr<InstantIOContext> instant_io_context);

  // Determine if this chrome-search: request is coming from an Instant render
  // process.
  static bool ShouldServiceRequest(const GURL& url,
                                   content::ResourceContext* resource_context,
                                   int render_process_id);

  // Returns true if the given |render_process_id| represents an Instant
  // renderer.
  static bool IsInstantProcess(content::ResourceContext* resource_context,
                               int render_process_id);

 protected:
   virtual ~InstantIOContext();

 private:
  friend class base::RefCountedThreadSafe<InstantIOContext>;

  // Check that |process_id| is in the known set of Instant processes, ie.
  // |process_ids_|.
  bool IsInstantProcess(int process_id) const;

  // The process IDs associated with Instant processes.  Mirror of the process
  // IDs in InstantService.  Duplicated here for synchronous access on the IO
  // thread.
  std::set<int> process_ids_;

  DISALLOW_COPY_AND_ASSIGN(InstantIOContext);
};

#endif  // CHROME_BROWSER_SEARCH_INSTANT_IO_CONTEXT_H_
