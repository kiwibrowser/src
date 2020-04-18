// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/mhtml_generation_manager.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/containers/queue.h"
#include "base/files/file.h"
#include "base/guid.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/scoped_observer.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/download/public/common/download_task_runner.h"
#include "content/browser/bad_message.h"
#include "content/browser/download/mhtml_extra_parts_impl.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/mhtml_extra_parts.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/mhtml_generation_params.h"
#include "net/base/mime_util.h"

namespace {
const char kContentLocation[] = "Content-Location: ";
const char kContentType[] = "Content-Type: ";
int kInvalidFileSize = -1;
}  // namespace

namespace content {

// The class and all of its members live on the UI thread.  Only static methods
// are executed on other threads.
class MHTMLGenerationManager::Job : public RenderProcessHostObserver {
 public:
  Job(int job_id,
      WebContents* web_contents,
      const MHTMLGenerationParams& params,
      GenerateMHTMLCallback callback);
  ~Job() override;

  int id() const { return job_id_; }
  void set_browser_file(base::File file) { browser_file_ = std::move(file); }
  base::TimeTicks creation_time() const { return creation_time_; }

  GenerateMHTMLCallback callback() { return std::move(callback_); }

  // Indicates whether we expect a message from the |sender| at this time.
  // We expect only one message per frame - therefore calling this method
  // will always clear |frame_tree_node_id_of_busy_frame_|.
  bool IsMessageFromFrameExpected(RenderFrameHostImpl* sender);

  // Handler for FrameHostMsg_SerializeAsMHTMLResponse (a notification from the
  // renderer that the MHTML generation for previous frame has finished).
  // Returns MhtmlSaveStatus::SUCCESS or a specific error status.
  MhtmlSaveStatus OnSerializeAsMHTMLResponse(
      const std::set<std::string>& digests_of_uris_of_serialized_resources);

  // Sends IPC to the renderer, asking for MHTML generation of the next frame.
  // Returns MhtmlSaveStatus::SUCCESS or a specific error status.
  MhtmlSaveStatus SendToNextRenderFrame();

  // Indicates if more calls to SendToNextRenderFrame are needed.
  bool IsDone() const {
    bool waiting_for_response_from_renderer =
        frame_tree_node_id_of_busy_frame_ !=
        FrameTreeNode::kFrameTreeNodeInvalidId;
    bool no_more_requests_to_send = pending_frame_tree_node_ids_.empty();
    return !waiting_for_response_from_renderer && no_more_requests_to_send;
  }

  // Write the MHTML footer and close the file on the file thread and respond
  // back on the UI thread with the updated status and file size (which will be
  // negative in case of errors).
  void CloseFile(
      base::OnceCallback<void(const std::tuple<MhtmlSaveStatus, int64_t>&)>
          callback,
      MhtmlSaveStatus save_status);

  // RenderProcessHostObserver:
  void RenderProcessExited(RenderProcessHost* host,
                           const ChildProcessTerminationInfo& info) override;
  void RenderProcessHostDestroyed(RenderProcessHost* host) override;

  void MarkAsFinished();

  void ReportRendererMainThreadTime(base::TimeDelta renderer_main_thread_time);

 private:
  // Writes the MHTML footer to the file and closes it.
  //
  // Note: The same |boundary| marker must be used for all "boundaries" -- in
  // the header, parts and footer -- that belong to the same MHTML document (see
  // also rfc1341, section 7.2.1, "boundary" description).
  static std::tuple<MhtmlSaveStatus, int64_t> FinalizeAndCloseFileOnFileThread(
      MhtmlSaveStatus save_status,
      const std::string& boundary,
      base::File file,
      const std::vector<MHTMLExtraDataPart>& extra_data_parts);
  void AddFrame(RenderFrameHost* render_frame_host);

  // If we have any extra MHTML parts to write out, write them into the file
  // while on the file thread.  Returns true for success, or if there is no data
  // to write.
  static bool WriteExtraDataParts(
      const std::string& boundary,
      base::File& file,
      const std::vector<MHTMLExtraDataPart>& extra_data_parts);

  // Writes the footer into the MHTML file.  Returns false for faiulre.
  static bool WriteFooter(const std::string& boundary, base::File& file);

  // Close the MHTML file if it looks good, setting the size param.  Returns
  // false for failure.
  static bool CloseFileIfValid(base::File& file, int64_t* file_size);

  // Id used to map renderer responses to jobs.
  // See also MHTMLGenerationManager::id_to_job_ map.
  const int job_id_;

  // Time tracking for performance metrics reporting.
  const base::TimeTicks creation_time_;
  base::TimeTicks wait_on_renderer_start_time_;
  base::TimeDelta all_renderers_wait_time_;
  base::TimeDelta all_renderers_main_thread_time_;
  base::TimeDelta longest_renderer_main_thread_time_;

  // User-configurable parameters. Includes the file location, binary encoding
  // choices, and whether to skip storing resources marked
  // Cache-Control: no-store.
  MHTMLGenerationParams params_;

  // The IDs of frames that still need to be processed.
  base::queue<int> pending_frame_tree_node_ids_;

  // Identifies a frame to which we've sent FrameMsg_SerializeAsMHTML but for
  // which we didn't yet process FrameHostMsg_SerializeAsMHTMLResponse via
  // OnSerializeAsMHTMLResponse.
  int frame_tree_node_id_of_busy_frame_;

  // The handle to the file the MHTML is saved to for the browser process.
  base::File browser_file_;

  // MIME multipart boundary to use in the MHTML doc.
  const std::string mhtml_boundary_marker_;

  // Digests of URIs of already generated MHTML parts.
  std::set<std::string> digests_of_already_serialized_uris_;
  std::string salt_;

  // The callback to call once generation is complete.
  GenerateMHTMLCallback callback_;

  // Whether the job is finished (set to true only for the short duration of
  // time between MHTMLGenerationManager::JobFinished is called and the job is
  // destroyed by MHTMLGenerationManager::OnFileClosed).
  bool is_finished_;

  // Any extra data parts that should be emitted into the output MHTML.
  std::vector<MHTMLExtraDataPart> extra_data_parts_;

  // RAII helper for registering this Job as a RenderProcessHost observer.
  ScopedObserver<RenderProcessHost, MHTMLGenerationManager::Job>
      observed_renderer_process_host_;

  DISALLOW_COPY_AND_ASSIGN(Job);
};

MHTMLGenerationManager::Job::Job(int job_id,
                                 WebContents* web_contents,
                                 const MHTMLGenerationParams& params,
                                 GenerateMHTMLCallback callback)
    : job_id_(job_id),
      creation_time_(base::TimeTicks::Now()),
      params_(params),
      frame_tree_node_id_of_busy_frame_(FrameTreeNode::kFrameTreeNodeInvalidId),
      mhtml_boundary_marker_(net::GenerateMimeMultipartBoundary()),
      salt_(base::GenerateGUID()),
      callback_(std::move(callback)),
      is_finished_(false),
      observed_renderer_process_host_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  web_contents->ForEachFrame(base::BindRepeating(
      &MHTMLGenerationManager::Job::AddFrame,
      base::Unretained(this)));  // Safe because ForEachFrame() is synchronous.

  // Main frame needs to be processed first.
  DCHECK(!pending_frame_tree_node_ids_.empty());
  DCHECK(FrameTreeNode::GloballyFindByID(pending_frame_tree_node_ids_.front())
             ->parent() == nullptr);

  // Save off any extra data.
  auto* extra_parts = static_cast<MHTMLExtraPartsImpl*>(
      MHTMLExtraParts::FromWebContents(web_contents));
  if (extra_parts)
    extra_data_parts_ = extra_parts->parts();
}

MHTMLGenerationManager::Job::~Job() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

MhtmlSaveStatus MHTMLGenerationManager::Job::SendToNextRenderFrame() {
  DCHECK(browser_file_.IsValid());
  DCHECK(!pending_frame_tree_node_ids_.empty());

  FrameMsg_SerializeAsMHTML_Params ipc_params;
  ipc_params.job_id = job_id_;
  ipc_params.mhtml_boundary_marker = mhtml_boundary_marker_;
  ipc_params.mhtml_binary_encoding = params_.use_binary_encoding;
  ipc_params.mhtml_cache_control_policy = params_.cache_control_policy;
  ipc_params.mhtml_popup_overlay_removal = params_.remove_popup_overlay;
  ipc_params.mhtml_problem_detection = params_.use_page_problem_detectors;

  int frame_tree_node_id = pending_frame_tree_node_ids_.front();
  pending_frame_tree_node_ids_.pop();

  FrameTreeNode* ftn = FrameTreeNode::GloballyFindByID(frame_tree_node_id);
  if (!ftn)  // The contents went away.
    return MhtmlSaveStatus::FRAME_NO_LONGER_EXISTS;
  RenderFrameHost* rfh = ftn->current_frame_host();

  // Get notified if the target of the IPC message dies between responding.
  observed_renderer_process_host_.RemoveAll();
  observed_renderer_process_host_.Add(rfh->GetProcess());

  // Tell the renderer to skip (= deduplicate) already covered MHTML parts.
  ipc_params.salt = salt_;
  ipc_params.digests_of_uris_to_skip = digests_of_already_serialized_uris_;

  ipc_params.destination_file = IPC::GetPlatformFileForTransit(
      browser_file_.GetPlatformFile(), false);  // |close_source_handle|.

  // Send the IPC asking the renderer to serialize the frame.
  DCHECK_EQ(FrameTreeNode::kFrameTreeNodeInvalidId,
            frame_tree_node_id_of_busy_frame_);
  frame_tree_node_id_of_busy_frame_ = frame_tree_node_id;
  rfh->Send(new FrameMsg_SerializeAsMHTML(rfh->GetRoutingID(), ipc_params));
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("page-serialization", "WaitingOnRenderer",
                                    this, "frame tree node id",
                                    frame_tree_node_id);
  DCHECK(wait_on_renderer_start_time_.is_null());
  wait_on_renderer_start_time_ = base::TimeTicks::Now();
  return MhtmlSaveStatus::SUCCESS;
}

void MHTMLGenerationManager::Job::RenderProcessExited(
    RenderProcessHost* host,
    const ChildProcessTerminationInfo& info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  MHTMLGenerationManager::GetInstance()->RenderProcessExited(this);
}

void MHTMLGenerationManager::Job::MarkAsFinished() {
  DCHECK(!is_finished_);
  if (is_finished_)
    return;

  is_finished_ = true;

  // Stopping RenderProcessExited notifications is needed to avoid calling
  // JobFinished twice.  See also https://crbug.com/612098.
  observed_renderer_process_host_.RemoveAll();

  TRACE_EVENT_NESTABLE_ASYNC_INSTANT0("page-serialization", "JobFinished",
                                      this);

  // End of job timing reports.
  if (!wait_on_renderer_start_time_.is_null()) {
    base::TimeDelta renderer_wait_time =
        base::TimeTicks::Now() - wait_on_renderer_start_time_;
    UMA_HISTOGRAM_TIMES(
        "PageSerialization.MhtmlGeneration.BrowserWaitForRendererTime."
        "SingleFrame",
        renderer_wait_time);
    all_renderers_wait_time_ += renderer_wait_time;
  }
  if (!all_renderers_wait_time_.is_zero()) {
    UMA_HISTOGRAM_TIMES(
        "PageSerialization.MhtmlGeneration.BrowserWaitForRendererTime."
        "FrameTree",
        all_renderers_wait_time_);
  }
  if (!all_renderers_main_thread_time_.is_zero()) {
    UMA_HISTOGRAM_TIMES(
        "PageSerialization.MhtmlGeneration.RendererMainThreadTime.FrameTree",
        all_renderers_main_thread_time_);
  }
  if (!longest_renderer_main_thread_time_.is_zero()) {
    UMA_HISTOGRAM_TIMES(
        "PageSerialization.MhtmlGeneration.RendererMainThreadTime.SlowestFrame",
        longest_renderer_main_thread_time_);
  }
}

void MHTMLGenerationManager::Job::ReportRendererMainThreadTime(
    base::TimeDelta renderer_main_thread_time) {
  DCHECK(renderer_main_thread_time > base::TimeDelta());
  if (renderer_main_thread_time > base::TimeDelta())
    all_renderers_main_thread_time_ += renderer_main_thread_time;
  if (renderer_main_thread_time > longest_renderer_main_thread_time_)
    longest_renderer_main_thread_time_ = renderer_main_thread_time;
}

void MHTMLGenerationManager::Job::AddFrame(RenderFrameHost* render_frame_host) {
  auto* rfhi = static_cast<RenderFrameHostImpl*>(render_frame_host);
  int frame_tree_node_id = rfhi->frame_tree_node()->frame_tree_node_id();
  pending_frame_tree_node_ids_.push(frame_tree_node_id);
}

void MHTMLGenerationManager::Job::RenderProcessHostDestroyed(
    RenderProcessHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observed_renderer_process_host_.Remove(host);
}

void MHTMLGenerationManager::Job::CloseFile(
    base::OnceCallback<void(const std::tuple<MhtmlSaveStatus, int64_t>&)>
        callback,
    MhtmlSaveStatus save_status) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!mhtml_boundary_marker_.empty());

  if (!browser_file_.IsValid()) {
    // Only update the status if that won't hide an earlier error.
    if (save_status == MhtmlSaveStatus::SUCCESS)
      save_status = MhtmlSaveStatus::FILE_WRITTING_ERROR;
    std::move(callback).Run(std::make_tuple(save_status, -1));
    return;
  }

  // If no previous error occurred the boundary should be sent.
  base::PostTaskAndReplyWithResult(
      download::GetDownloadTaskRunner().get(), FROM_HERE,
      base::BindOnce(
          &MHTMLGenerationManager::Job::FinalizeAndCloseFileOnFileThread,
          save_status,
          (save_status == MhtmlSaveStatus::SUCCESS ? mhtml_boundary_marker_
                                                   : std::string()),
          std::move(browser_file_), std::move(extra_data_parts_)),
      std::move(callback));
}

bool MHTMLGenerationManager::Job::IsMessageFromFrameExpected(
    RenderFrameHostImpl* sender) {
  int sender_id = sender->frame_tree_node()->frame_tree_node_id();
  if (sender_id != frame_tree_node_id_of_busy_frame_)
    return false;

  // We only expect one message per frame - let's make sure subsequent messages
  // from the same |sender| will be rejected.
  frame_tree_node_id_of_busy_frame_ = FrameTreeNode::kFrameTreeNodeInvalidId;

  return true;
}

MhtmlSaveStatus MHTMLGenerationManager::Job::OnSerializeAsMHTMLResponse(
    const std::set<std::string>& digests_of_uris_of_serialized_resources) {
  DCHECK(!wait_on_renderer_start_time_.is_null());
  base::TimeDelta renderer_wait_time =
      base::TimeTicks::Now() - wait_on_renderer_start_time_;
  UMA_HISTOGRAM_TIMES(
      "PageSerialization.MhtmlGeneration.BrowserWaitForRendererTime."
      "SingleFrame",
      renderer_wait_time);
  all_renderers_wait_time_ += renderer_wait_time;
  wait_on_renderer_start_time_ = base::TimeTicks();

  // Renderer should be deduping resources with the same uris.
  DCHECK_EQ(0u, base::STLSetIntersection<std::set<std::string>>(
                    digests_of_already_serialized_uris_,
                    digests_of_uris_of_serialized_resources).size());
  digests_of_already_serialized_uris_.insert(
      digests_of_uris_of_serialized_resources.begin(),
      digests_of_uris_of_serialized_resources.end());

  // Report success if all frames have been processed.
  if (pending_frame_tree_node_ids_.empty())
    return MhtmlSaveStatus::SUCCESS;

  return SendToNextRenderFrame();
}

// static
std::tuple<MhtmlSaveStatus, int64_t>
MHTMLGenerationManager::Job::FinalizeAndCloseFileOnFileThread(
    MhtmlSaveStatus save_status,
    const std::string& boundary,
    base::File file,
    const std::vector<MHTMLExtraDataPart>& extra_data_parts) {
  DCHECK(download::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());

  // If no previous error occurred the boundary should have been provided.
  if (save_status == MhtmlSaveStatus::SUCCESS) {
    TRACE_EVENT0("page-serialization",
                 "MHTMLGenerationManager::Job MHTML footer writing");
    DCHECK(!boundary.empty());

    // Write the extra data into a part of its own, if we have any.
    if (!WriteExtraDataParts(boundary, file, extra_data_parts)) {
      save_status = MhtmlSaveStatus::FILE_WRITTING_ERROR;
    }

    // Write out the footer at the bottom of the file.
    if (save_status == MhtmlSaveStatus::SUCCESS &&
        !WriteFooter(boundary, file)) {
      save_status = MhtmlSaveStatus::FILE_WRITTING_ERROR;
    }
  }

  // If the file is still valid try to close it. Only update the status if that
  // won't hide an earlier error.
  int64_t file_size = kInvalidFileSize;
  if (!CloseFileIfValid(file, &file_size) &&
      save_status == MhtmlSaveStatus::SUCCESS) {
    save_status = MhtmlSaveStatus::FILE_CLOSING_ERROR;
  }

  return std::make_tuple(save_status, file_size);
}

// static
bool MHTMLGenerationManager::Job::WriteExtraDataParts(
    const std::string& boundary,
    base::File& file,
    const std::vector<MHTMLExtraDataPart>& extra_data_parts) {
  DCHECK(download::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
  // Don't write an extra data part if there is none.
  if (extra_data_parts.empty())
    return true;

  std::string serialized_extra_data_parts;

  // For each extra part, serialize that part and add to our accumulator
  // string.
  for (const auto& part : extra_data_parts) {
    // Write a newline, then a boundary, a newline, then the content
    // location, a newline, the content type, a newline, extra_headers,
    // two newlines, the body, and end with a newline.
    std::string serialized_extra_data_part = base::StringPrintf(
        "\r\n--%s\r\n%s%s\r\n%s%s\r\n%s\r\n\r\n%s\r\n", boundary.c_str(),
        kContentLocation, part.content_location.c_str(), kContentType,
        part.content_type.c_str(), part.extra_headers.c_str(),
        part.body.c_str());
    DCHECK(base::IsStringASCII(serialized_extra_data_part));

    serialized_extra_data_parts += serialized_extra_data_part;
  }

  // Write the string into the file.  Returns false if we failed the write.
  return (file.WriteAtCurrentPos(serialized_extra_data_parts.data(),
                                 serialized_extra_data_parts.size()) >= 0);
}

// static
bool MHTMLGenerationManager::Job::WriteFooter(const std::string& boundary,
                                              base::File& file) {
  DCHECK(download::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
  // Per the spec, the boundary must occur at the beginning of a line.
  std::string footer = base::StringPrintf("\r\n--%s--\r\n", boundary.c_str());
  DCHECK(base::IsStringASCII(footer));
  return (file.WriteAtCurrentPos(footer.data(), footer.size()) >= 0);
}

// static
bool MHTMLGenerationManager::Job::CloseFileIfValid(base::File& file,
                                                   int64_t* file_size) {
  DCHECK(download::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
  DCHECK(file_size);
  if (file.IsValid()) {
    *file_size = file.GetLength();
    file.Close();
    return true;
  }

  return false;
}

MHTMLGenerationManager* MHTMLGenerationManager::GetInstance() {
  return base::Singleton<MHTMLGenerationManager>::get();
}

MHTMLGenerationManager::MHTMLGenerationManager() : next_job_id_(0) {}

MHTMLGenerationManager::~MHTMLGenerationManager() {
}

void MHTMLGenerationManager::SaveMHTML(WebContents* web_contents,
                                       const MHTMLGenerationParams& params,
                                       GenerateMHTMLCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Job* job = NewJob(web_contents, params, std::move(callback));
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN2(
      "page-serialization", "SavingMhtmlJob", job, "url",
      web_contents->GetLastCommittedURL().possibly_invalid_spec(),
      "file", params.file_path.AsUTF8Unsafe());

  base::PostTaskAndReplyWithResult(
      download::GetDownloadTaskRunner().get(), FROM_HERE,
      base::Bind(&MHTMLGenerationManager::CreateFile, params.file_path),
      base::Bind(&MHTMLGenerationManager::OnFileAvailable,
                 base::Unretained(this),  // Safe b/c |this| is a singleton.
                 job->id()));
}

void MHTMLGenerationManager::OnSerializeAsMHTMLResponse(
    RenderFrameHostImpl* sender,
    int job_id,
    MhtmlSaveStatus save_status,
    const std::set<std::string>& digests_of_uris_of_serialized_resources,
    base::TimeDelta renderer_main_thread_time) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Job* job = FindJob(job_id);
  if (!job || !job->IsMessageFromFrameExpected(sender)) {
    NOTREACHED();
    ReceivedBadMessage(sender->GetProcess(),
                       bad_message::DWNLD_INVALID_SERIALIZE_AS_MHTML_RESPONSE);
    return;
  }

  TRACE_EVENT_NESTABLE_ASYNC_END0("page-serialization", "WaitingOnRenderer",
                                  job);
  job->ReportRendererMainThreadTime(renderer_main_thread_time);

  // If the renderer succeeded notify the Job and update the status.
  if (save_status == MhtmlSaveStatus::SUCCESS) {
    save_status = job->OnSerializeAsMHTMLResponse(
        digests_of_uris_of_serialized_resources);
  }

  // If there was a failure (either from the renderer or from the job) then
  // terminate the job and return.
  if (save_status != MhtmlSaveStatus::SUCCESS) {
    JobFinished(job, save_status);
    return;
  }

  // Otherwise report completion if the job is done.
  if (job->IsDone())
    JobFinished(job, MhtmlSaveStatus::SUCCESS);
}

// static
base::File MHTMLGenerationManager::CreateFile(const base::FilePath& file_path) {
  DCHECK(download::GetDownloadTaskRunner()->RunsTasksInCurrentSequence());

  // SECURITY NOTE: A file descriptor to the file created below will be passed
  // to multiple renderer processes which (in out-of-process iframes mode) can
  // act on behalf of separate web principals.  Therefore it is important to
  // only allow writing to the file and forbid reading from the file (as this
  // would allow reading content generated by other renderers / other web
  // principals).
  uint32_t file_flags = base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE;

  base::File browser_file(file_path, file_flags);
  if (!browser_file.IsValid()) {
    LOG(ERROR) << "Failed to create file to save MHTML at: "
               << file_path.value();
  }
  return browser_file;
}

void MHTMLGenerationManager::OnFileAvailable(int job_id,
                                             base::File browser_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Job* job = FindJob(job_id);
  DCHECK(job);

  if (!browser_file.IsValid()) {
    LOG(ERROR) << "Failed to create file";
    JobFinished(job, MhtmlSaveStatus::FILE_CREATION_ERROR);
    return;
  }

  job->set_browser_file(std::move(browser_file));

  MhtmlSaveStatus save_status = job->SendToNextRenderFrame();
  if (save_status != MhtmlSaveStatus::SUCCESS) {
    JobFinished(job, save_status);
  }
}

void MHTMLGenerationManager::JobFinished(Job* job,
                                         MhtmlSaveStatus save_status) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(job);
  job->MarkAsFinished();
  job->CloseFile(
      base::BindOnce(&MHTMLGenerationManager::OnFileClosed,
                     base::Unretained(this),  // Safe b/c |this| is a singleton.
                     job->id()),
      save_status);
}

void MHTMLGenerationManager::OnFileClosed(
    int job_id,
    const std::tuple<MhtmlSaveStatus, int64_t>& save_status_size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  MhtmlSaveStatus save_status = std::get<0>(save_status_size);
  int64_t file_size = std::get<1>(save_status_size);

  Job* job = FindJob(job_id);
  DCHECK(job);
  TRACE_EVENT_NESTABLE_ASYNC_END2(
      "page-serialization", "SavingMhtmlJob", job, "job save status",
      GetMhtmlSaveStatusLabel(save_status), "file size", file_size);
  UMA_HISTOGRAM_TIMES("PageSerialization.MhtmlGeneration.FullPageSavingTime",
                      base::TimeTicks::Now() - job->creation_time());
  UMA_HISTOGRAM_ENUMERATION("PageSerialization.MhtmlGeneration.FinalSaveStatus",
                            static_cast<int>(save_status),
                            static_cast<int>(MhtmlSaveStatus::LAST));
  std::move(job->callback())
      .Run(save_status == MhtmlSaveStatus::SUCCESS ? file_size : -1);
  id_to_job_.erase(job_id);
}

MHTMLGenerationManager::Job* MHTMLGenerationManager::NewJob(
    WebContents* web_contents,
    const MHTMLGenerationParams& params,
    GenerateMHTMLCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Job* job = new Job(++next_job_id_, web_contents, params, std::move(callback));
  id_to_job_[job->id()] = base::WrapUnique(job);
  return job;
}

MHTMLGenerationManager::Job* MHTMLGenerationManager::FindJob(int job_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto iter = id_to_job_.find(job_id);
  if (iter == id_to_job_.end()) {
    NOTREACHED();
    return nullptr;
  }
  return iter->second.get();
}

void MHTMLGenerationManager::RenderProcessExited(Job* job) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(job);
  JobFinished(job, MhtmlSaveStatus::RENDER_PROCESS_EXITED);
}

}  // namespace content
