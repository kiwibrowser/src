// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/local_files_ntp_source.h"

#include <memory>

#if !defined(GOOGLE_CHROME_BUILD)

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/url_data_source.h"
#include "third_party/re2/src/re2/re2.h"
#include "third_party/re2/src/re2/stringpiece.h"

namespace {

const char kBasePath[] = "chrome/browser/resources/local_ntp";

// Matches lines of form '<include src="foo">' and captures 'foo'.
// TODO(treib): None of the local NTP files use this. Remove it?
const char kInlineResourceRegex[] = "<include.*?src\\=[\"'](.+?)[\"'].*?>";

// TODO(treib): local_ntp.css contains url(...) references to images, which get
// inlined by grit's "flattenhtml" feature during regular builds. Find some way
// to make that work with local files.

void CallbackWithLoadedResource(
    const std::string& origin,
    const content::URLDataSource::GotDataCallback& callback,
    const std::string& content) {
  std::string output = content;
  if (!origin.empty())
    base::ReplaceFirstSubstringAfterOffset(&output, 0, "{{ORIGIN}}", origin);

  // Strip out the integrity placeholders. CSP is disabled in local-files mode,
  // so the integrity values aren't required.
  base::ReplaceFirstSubstringAfterOffset(&output, 0, "{{CONFIG_INTEGRITY}}",
                                         std::string());
  base::ReplaceFirstSubstringAfterOffset(&output, 0, "{{LOCAL_NTP_INTEGRITY}}",
                                         std::string());

  callback.Run(base::RefCountedString::TakeString(&output));
}

// Read a file to a string and return.
std::string ReadFileAndReturn(const base::FilePath& path) {
  std::string data;
  // This call can fail, but it doesn't matter for our purposes. If it fails,
  // we simply return an empty string.
  base::ReadFileToString(path, &data);
  return data;
}

}  // namespace

namespace local_ntp {

void FlattenLocalInclude(
    const content::URLDataSource::GotDataCallback& callback,
    std::string topLevelResource,
    scoped_refptr<base::RefCountedMemory> inlineResource);

// Helper method invoked by both CheckLocalIncludes and FlattenLocalInclude.
// Checks for any <include> directives; if any are found, loads the associated
// file and calls FlattenLocalInclude with the result. Otherwise, processing
// is done, and so the original callback is invoked.
void CheckLocalIncludesHelper(
    const content::URLDataSource::GotDataCallback& callback,
    std::string& resource) {
  std::string filename;
  re2::StringPiece resourceWrapper(resource);
  if (re2::RE2::FindAndConsume(&resourceWrapper, kInlineResourceRegex,
                               &filename)) {
    content::URLDataSource::GotDataCallback wrapper =
        base::Bind(&FlattenLocalInclude, callback, resource);
    SendLocalFileResource(filename, wrapper);
  } else {
    callback.Run(base::RefCountedString::TakeString(&resource));
  }
}

// Wrapper around the above helper function for use as a callback. Processes
// local files to inline any files indicated by an <include> directive.
void CheckLocalIncludes(const content::URLDataSource::GotDataCallback& callback,
                        scoped_refptr<base::RefCountedMemory> resource) {
  std::string resourceAsStr(resource->front_as<char>(), resource->size());
  CheckLocalIncludesHelper(callback, resourceAsStr);
}

// Replaces the first <include> directive found with the given file contents.
// Afterwards, re-invokes CheckLocalIncludesHelper to handle any subsequent
// <include>s, including those which may have been added by the newly-inlined
// resource.
void FlattenLocalInclude(
    const content::URLDataSource::GotDataCallback& callback,
    std::string topLevelResource,
    scoped_refptr<base::RefCountedMemory> inlineResource) {
  std::string inlineAsStr(inlineResource->front_as<char>(),
                          inlineResource->size());
  re2::RE2::Replace(&topLevelResource, kInlineResourceRegex, inlineAsStr);
  CheckLocalIncludesHelper(callback, topLevelResource);
}

void SendLocalFileResource(
    const std::string& path,
    const content::URLDataSource::GotDataCallback& callback) {
  SendLocalFileResourceWithOrigin(path, std::string(), callback);
}

void SendLocalFileResourceWithOrigin(
    const std::string& path,
    const std::string& origin,
    const content::URLDataSource::GotDataCallback& callback) {
  base::FilePath fullpath;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &fullpath);
  fullpath = fullpath.AppendASCII(kBasePath).AppendASCII(path);
  content::URLDataSource::GotDataCallback wrapper =
      base::Bind(&CheckLocalIncludes, callback);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&ReadFileAndReturn, fullpath),
      base::Bind(&CallbackWithLoadedResource, origin, wrapper));
}

}  // namespace local_ntp

#endif  //  !defined(GOOGLE_CHROME_BUILD)
