// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CONTENT_BROWSER_DOWNLOAD_MHTML_GENERATION_MANAGER_H_
#define CONTENT_BROWSER_DOWNLOAD_MHTML_GENERATION_MANAGER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/process/process.h"
#include "content/common/download/mhtml_save_status.h"
#include "content/public/common/mhtml_generation_params.h"
#include "ipc/ipc_platform_file.h"

namespace base {
class FilePath;
}

namespace content {

class RenderFrameHostImpl;
class WebContents;

// The class and all of its members live on the UI thread.  Only static methods
// are executed on other threads.
//
// MHTMLGenerationManager is a singleton.  Each call to SaveMHTML method creates
// a new instance of MHTMLGenerationManager::Job that tracks generation of a
// single MHTML file.
class MHTMLGenerationManager {
 public:
  static MHTMLGenerationManager* GetInstance();

  // GenerateMHTMLCallback is called to report completion and status of MHTML
  // generation.  On success |file_size| indicates the size of the
  // generated file.  On failure |file_size| is -1.
  using GenerateMHTMLCallback = base::OnceCallback<void(int64_t file_size)>;

  // Instructs the RenderFrames in |web_contents| to generate a MHTML
  // representation of the current page.
  void SaveMHTML(WebContents* web_contents,
                 const MHTMLGenerationParams& params,
                 GenerateMHTMLCallback callback);

  // Handler for FrameHostMsg_SerializeAsMHTMLResponse (a notification from the
  // renderer that the MHTML generation finished for a single frame).
  void OnSerializeAsMHTMLResponse(
      RenderFrameHostImpl* sender,
      int job_id,
      MhtmlSaveStatus save_status,
      const std::set<std::string>& digests_of_uris_of_serialized_resources,
      base::TimeDelta renderer_main_thread_time);

 private:
  friend struct base::DefaultSingletonTraits<MHTMLGenerationManager>;
  class Job;

  MHTMLGenerationManager();
  virtual ~MHTMLGenerationManager();

  // Called on the file thread to create |file|.
  static base::File CreateFile(const base::FilePath& file_path);

  // Called on the UI thread when the file that should hold the MHTML data has
  // been created.
  void OnFileAvailable(int job_id, base::File browser_file);

  // Called on the UI thread when a job has been finished.
  void JobFinished(Job* job, MhtmlSaveStatus save_status);

  // Called on the UI thread after the file got finalized and we have its size.
  void OnFileClosed(
      int job_id,
      const std::tuple<MhtmlSaveStatus, int64_t>& save_status_size);

  // Creates and registers a new job.
  Job* NewJob(WebContents* web_contents,
              const MHTMLGenerationParams& params,
              GenerateMHTMLCallback callback);

  // Finds job by id.  Returns nullptr if no job with a given id was found.
  Job* FindJob(int job_id);

  // Called when the render process connected to a job exits.
  void RenderProcessExited(Job* job);

  std::map<int, std::unique_ptr<Job>> id_to_job_;

  int next_job_id_;

  DISALLOW_COPY_AND_ASSIGN(MHTMLGenerationManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_MHTML_GENERATION_MANAGER_H_
