// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_BOOKMARK_HTML_WRITER_H_
#define CHROME_BROWSER_BOOKMARKS_BOOKMARK_HTML_WRITER_H_

#include <list>
#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task_scheduler/post_task.h"
#include "components/favicon_base/favicon_types.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

namespace bookmarks {
class BookmarkNode;
}

namespace chrome {
struct FaviconRawBitmapResult;
}

// Observer for bookmark html output. Used only in tests.
class BookmarksExportObserver {
 public:
  // Is invoked on the IO thread.
  virtual void OnExportFinished() = 0;

 protected:
  virtual ~BookmarksExportObserver() {}
};

// Class that fetches favicons for list of bookmarks and
// then starts Writer which outputs bookmarks and favicons to html file.
// Should be used only by WriteBookmarks function.
class BookmarkFaviconFetcher: public content::NotificationObserver {
 public:
  // Map of URL and corresponding favicons.
  typedef std::map<std::string, scoped_refptr<base::RefCountedMemory> >
      URLFaviconMap;

  BookmarkFaviconFetcher(Profile* profile,
                         const base::FilePath& path,
                         BookmarksExportObserver* observer);
  ~BookmarkFaviconFetcher() override;

  // Executes bookmark export process.
  void ExportBookmarks();

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  // Recursively extracts URLs from bookmarks.
  void ExtractUrls(const bookmarks::BookmarkNode* node);

  // Executes Writer task that writes bookmarks data to html file.
  void ExecuteWriter();

  // Starts async fetch for the next bookmark favicon.
  // Takes single url from bookmark_urls_ and removes it from the list.
  // Returns true if there are more favicons to extract.
  bool FetchNextFavicon();

  // Favicon fetch callback. After all favicons are fetched executes
  // html output with |background_io_task_runner_|.
  void OnFaviconDataAvailable(
      const favicon_base::FaviconRawBitmapResult& bitmap_result);

  // The Profile object used for accessing FaviconService, bookmarks model.
  Profile* profile_;

  // All URLs that are extracted from bookmarks. Used to fetch favicons
  // for each of them. After favicon is fetched top url is removed from list.
  std::list<std::string> bookmark_urls_;

  // Tracks favicon tasks.
  base::CancelableTaskTracker cancelable_task_tracker_;

  // Map that stores favicon per URL.
  std::unique_ptr<URLFaviconMap> favicons_map_;

  // Path where html output is stored.
  base::FilePath path_;

  BookmarksExportObserver* observer_;

  content::NotificationRegistrar registrar_;

  scoped_refptr<base::SequencedTaskRunner> background_io_task_runner_ =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});

  DISALLOW_COPY_AND_ASSIGN(BookmarkFaviconFetcher);
};

namespace bookmark_html_writer {

// Writes the bookmarks out in the 'bookmarks.html' format understood by
// Firefox and IE. The results are written asynchronously to the file at |path|.
// Before writing to the file favicons are fetched on the main thread.
// TODO(sky): need a callback on failure.
void WriteBookmarks(Profile* profile,
                    const base::FilePath& path,
                    BookmarksExportObserver* observer);

}  // namespace bookmark_html_writer

#endif  // CHROME_BROWSER_BOOKMARKS_BOOKMARK_HTML_WRITER_H_
