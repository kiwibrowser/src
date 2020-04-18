// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FILE_SELECT_HELPER_H_
#define CHROME_BROWSER_FILE_SELECT_HELPER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_widget_host_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/file_chooser_params.h"
#include "net/base/directory_lister.h"
#include "ui/shell_dialogs/select_file_dialog.h"

class Profile;

namespace content {
struct FileChooserFileInfo;
class RenderViewHost;
class WebContents;
}

namespace ui {
struct SelectedFileInfo;
}

// This class handles file-selection requests coming from renderer processes.
// It implements both the initialisation and listener functions for
// file-selection dialogs.
//
// Since FileSelectHelper listens to observations of a widget, it needs to live
// on and be destroyed on the UI thread. References to FileSelectHelper may be
// passed on to other threads.
class FileSelectHelper : public base::RefCountedThreadSafe<
                             FileSelectHelper,
                             content::BrowserThread::DeleteOnUIThread>,
                         public ui::SelectFileDialog::Listener,
                         public content::WebContentsObserver,
                         public content::RenderWidgetHostObserver {
 public:
  // Show the file chooser dialog.
  static void RunFileChooser(content::RenderFrameHost* render_frame_host,
                             const content::FileChooserParams& params);

  // Enumerates all the files in directory.
  static void EnumerateDirectory(content::WebContents* tab,
                                 int request_id,
                                 const base::FilePath& path);

 private:
  friend class base::RefCountedThreadSafe<FileSelectHelper>;
  friend class base::DeleteHelper<FileSelectHelper>;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;

  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, IsAcceptTypeValid);
  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, ZipPackage);
  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, GetSanitizedFileName);
  FRIEND_TEST_ALL_PREFIXES(FileSelectHelperTest, LastSelectedDirectory);
  explicit FileSelectHelper(Profile* profile);
  ~FileSelectHelper() override;

  // Utility class which can listen for directory lister events and relay
  // them to the main object with the correct tracking id.
  class DirectoryListerDispatchDelegate
      : public net::DirectoryLister::DirectoryListerDelegate {
   public:
    DirectoryListerDispatchDelegate(FileSelectHelper* parent, int id)
        : parent_(parent),
          id_(id) {}
    ~DirectoryListerDispatchDelegate() override {}
    void OnListFile(
        const net::DirectoryLister::DirectoryListerData& data) override;
    void OnListDone(int error) override;

   private:
    // This FileSelectHelper owns this object.
    FileSelectHelper* parent_;
    int id_;

    DISALLOW_COPY_AND_ASSIGN(DirectoryListerDispatchDelegate);
  };

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      std::unique_ptr<content::FileChooserParams> params);
  void GetFileTypesInThreadPool(
      std::unique_ptr<content::FileChooserParams> params);
  void GetSanitizedFilenameOnUIThread(
      std::unique_ptr<content::FileChooserParams> params);
#if defined(FULL_SAFE_BROWSING)
  void CheckDownloadRequestWithSafeBrowsing(
      const base::FilePath& default_path,
      std::unique_ptr<content::FileChooserParams> params);
  void ProceedWithSafeBrowsingVerdict(
      const base::FilePath& default_path,
      std::unique_ptr<content::FileChooserParams> params,
      bool allowed_by_safe_browsing);
#endif
  void RunFileChooserOnUIThread(
      const base::FilePath& default_path,
      std::unique_ptr<content::FileChooserParams> params);

  // Cleans up and releases this instance. This must be called after the last
  // callback is received from the file chooser dialog.
  void RunFileChooserEnd();

  // SelectFileDialog::Listener overrides.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;
  void FileSelectedWithExtraInfo(const ui::SelectedFileInfo& file,
                                 int index,
                                 void* params) override;
  void MultiFilesSelected(const std::vector<base::FilePath>& files,
                          void* params) override;
  void MultiFilesSelectedWithExtraInfo(
      const std::vector<ui::SelectedFileInfo>& files,
      void* params) override;
  void FileSelectionCanceled(void* params) override;

  // content::RenderWidgetHostObserver overrides.
  void RenderWidgetHostDestroyed(
      content::RenderWidgetHost* widget_host) override;

  // content::WebContentsObserver overrides.
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void WebContentsDestroyed() override;

  void EnumerateDirectory(int request_id,
                          content::RenderViewHost* render_view_host,
                          const base::FilePath& path);

  // Kicks off a new directory enumeration.
  void StartNewEnumeration(const base::FilePath& path,
                           int request_id,
                           content::RenderViewHost* render_view_host);

  // Callbacks from directory enumeration.
  virtual void OnListFile(
      int id,
      const net::DirectoryLister::DirectoryListerData& data);
  virtual void OnListDone(int id, int error);

  void LaunchConfirmationDialog(
      const base::FilePath& path,
      std::vector<ui::SelectedFileInfo> selected_files);

  // Cleans up and releases this instance. This must be called after the last
  // callback is received from the enumeration code.
  void EnumerateDirectoryEnd();

#if defined(OS_MACOSX)
  // Must be called from a MayBlock() task. Each selected file that is a package
  // will be zipped, and the zip will be passed to the render view host in place
  // of the package.
  void ProcessSelectedFilesMac(const std::vector<ui::SelectedFileInfo>& files);

  // Saves the paths of |zipped_files| for later deletion. Passes |files| to the
  // render view host.
  void ProcessSelectedFilesMacOnUIThread(
      const std::vector<ui::SelectedFileInfo>& files,
      const std::vector<base::FilePath>& zipped_files);

  // Zips the package at |path| into a temporary destination. Returns the
  // temporary destination, if the zip was successful. Otherwise returns an
  // empty path.
  static base::FilePath ZipPackage(const base::FilePath& path);
#endif  // defined(OS_MACOSX)

  // Utility method that passes |files| to the RenderFrameHost, and ends the
  // file chooser.
  void NotifyRenderFrameHostAndEnd(
      const std::vector<ui::SelectedFileInfo>& files);

  // Sends the result to the render process, and call |RunFileChooserEnd|.
  void NotifyRenderFrameHostAndEndAfterConversion(
      const std::vector<content::FileChooserFileInfo>& list);

  // Schedules the deletion of the files in |temporary_files_| and clears the
  // vector.
  void DeleteTemporaryFiles();

  // Cleans up when the initiator of the file chooser is no longer valid.
  void CleanUp();

  // Calls RunFileChooserEnd() if the webcontents was destroyed. Returns true
  // if the file chooser operation shouldn't proceed.
  bool AbortIfWebContentsDestroyed();

  // Helper method to get allowed extensions for select file dialog from
  // the specified accept types as defined in the spec:
  //   http://whatwg.org/html/number-state.html#attr-input-accept
  // |accept_types| contains only valid lowercased MIME types or file extensions
  // beginning with a period (.).
  static std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
  GetFileTypesFromAcceptType(const std::vector<base::string16>& accept_types);

  // Check the accept type is valid. It is expected to be all lower case with
  // no whitespace.
  static bool IsAcceptTypeValid(const std::string& accept_type);

  // Get a sanitized filename suitable for use as a default filename. The
  // suggested filename coming over the IPC may contain invalid characters or
  // may result in a filename that's reserved on the current platform.
  //
  // If |suggested_path| is empty, the return value is also empty.
  //
  // If |suggested_path| is non-empty, but can't be safely converted to UTF-8,
  // or is entirely lost during the sanitization process (e.g. because it
  // consists entirely of invalid characters), it's replaced with a default
  // filename.
  //
  // Otherwise, returns |suggested_path| with any invalid characters will be
  // replaced with a suitable replacement character.
  static base::FilePath GetSanitizedFileName(
      const base::FilePath& suggested_path);

  // Profile used to set/retrieve the last used directory.
  Profile* profile_;

  // The RenderFrameHost and WebContents for the page showing a file dialog
  // (may only be one such dialog).
  content::RenderFrameHost* render_frame_host_;
  content::WebContents* web_contents_;

  // Dialog box used for choosing files to upload from file form fields.
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> select_file_types_;

  // The type of file dialog last shown.
  ui::SelectFileDialog::Type dialog_type_;

  // The mode of file dialog last shown.
  content::FileChooserParams::Mode dialog_mode_;

  // Maintain a list of active directory enumerations.  These could come from
  // the file select dialog or from drag-and-drop of directories, so there could
  // be more than one going on at a time.
  struct ActiveDirectoryEnumeration;
  std::map<int, ActiveDirectoryEnumeration*> directory_enumerations_;

  ScopedObserver<content::RenderWidgetHost, content::RenderWidgetHostObserver>
      observer_;

  // Temporary files only used on OSX. This class is responsible for deleting
  // these files when they are no longer needed.
  std::vector<base::FilePath> temporary_files_;

  DISALLOW_COPY_AND_ASSIGN(FileSelectHelper);
};

#endif  // CHROME_BROWSER_FILE_SELECT_HELPER_H_
