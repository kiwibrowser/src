// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/search_metadata.h"

#include <algorithm>
#include <queue>
#include <utility>

#include "base/bind.h"
#include "base/i18n/string_search.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system_core_util.h"
#include "net/base/escape.h"

namespace drive {
namespace internal {

namespace {

struct ResultCandidate {
  ResultCandidate(const std::string& local_id,
                  const ResourceEntry& entry,
                  const std::string& highlighted_base_name)
      : local_id(local_id),
        entry(entry),
        highlighted_base_name(highlighted_base_name) {
  }

  std::string local_id;
  ResourceEntry entry;
  std::string highlighted_base_name;
};

// Used to sort the result candidates per the last accessed/modified time. The
// recently accessed/modified files come first.
bool CompareByTimestamp(const ResourceEntry& a,
                        const ResourceEntry& b,
                        MetadataSearchOrder order) {
  const PlatformFileInfoProto& a_file_info = a.file_info();
  const PlatformFileInfoProto& b_file_info = b.file_info();

  switch (order) {
    case MetadataSearchOrder::LAST_ACCESSED:
      if (a_file_info.last_accessed() != b_file_info.last_accessed())
        return a_file_info.last_accessed() > b_file_info.last_accessed();

      // When the entries have the same last access time (which happens quite
      // often because Drive server doesn't set the field until an entry is
      /// viewed via drive.google.com), we use last modified time as the tie
      // breaker.
      return a_file_info.last_modified() > b_file_info.last_modified();
    case MetadataSearchOrder::LAST_MODIFIED:
      return a_file_info.last_modified() > b_file_info.last_modified();
  }
}

struct ResultCandidateComparator {
  explicit ResultCandidateComparator(MetadataSearchOrder order)
      : order_(order) {}
  bool operator()(const std::unique_ptr<ResultCandidate>& a,
                  const std::unique_ptr<ResultCandidate>& b) const {
    return CompareByTimestamp(a->entry, b->entry, order_);
  }

 private:
  const MetadataSearchOrder order_;
};

typedef std::priority_queue<std::unique_ptr<ResultCandidate>,
                            std::vector<std::unique_ptr<ResultCandidate>>,
                            ResultCandidateComparator>
    ResultCandidateQueue;

// Classifies the given entry as hidden if it's not under specific directories.
class HiddenEntryClassifier {
 public:
  HiddenEntryClassifier(ResourceMetadata* metadata,
                        const std::string& mydrive_local_id)
      : metadata_(metadata) {
    // Only things under My Drive and drive/other are not hidden.
    is_hiding_child_[mydrive_local_id] = false;
    is_hiding_child_[util::kDriveOtherDirLocalId] = false;

    // Everything else is hidden, including the directories mentioned above
    // themselves.
    is_hiding_child_[""] = true;
  }

  // |result| is set to true if |entry| is hidden.
  FileError IsHidden(const ResourceEntry& entry, bool* result) {
    // Look up for parents recursively.
    std::vector<std::string> undetermined_ids;
    undetermined_ids.push_back(entry.parent_local_id());

    std::map<std::string, bool>::iterator it =
        is_hiding_child_.find(undetermined_ids.back());
    for (; it == is_hiding_child_.end();
         it = is_hiding_child_.find(undetermined_ids.back())) {
      ResourceEntry parent;
      FileError error =
          metadata_->GetResourceEntryById(undetermined_ids.back(), &parent);
      if (error != FILE_ERROR_OK)
        return error;
      undetermined_ids.push_back(parent.parent_local_id());
    }

    // Cache the result.
    undetermined_ids.pop_back();  // The last one is already in the map.
    for (size_t i = 0; i < undetermined_ids.size(); ++i)
      is_hiding_child_[undetermined_ids[i]] = it->second;

    *result = it->second;
    return FILE_ERROR_OK;
  }

 private:
  ResourceMetadata* metadata_;

  // local ID to is_hidden map.
  std::map<std::string, bool> is_hiding_child_;
};

// Used to implement SearchMetadata.
// Adds entry to the result when appropriate.
// In particular, if size of |queries| is larger than 0, only adds files with
// the name matching the query.
FileError MaybeAddEntryToResult(
    ResourceMetadata* resource_metadata,
    ResourceMetadata::Iterator* it,
    const std::vector<std::unique_ptr<
        base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>& queries,
    const SearchMetadataPredicate& predicate,
    size_t at_most_num_matches,
    MetadataSearchOrder order,
    HiddenEntryClassifier* hidden_entry_classifier,
    ResultCandidateQueue* result_candidates) {
  DCHECK_GE(at_most_num_matches, result_candidates->size());

  const ResourceEntry& entry = it->GetValue();

  // If the candidate set is already full, and this |entry| is old, do nothing.
  // We perform this check first in order to avoid the costly find-and-highlight
  // or FilePath lookup as much as possible.
  if (result_candidates->size() == at_most_num_matches &&
      !CompareByTimestamp(entry, result_candidates->top()->entry, order))
    return FILE_ERROR_OK;

  // Add |entry| to the result if the entry is eligible for the given
  // |options| and matches the query. The base name of the entry must
  // contain |query| to match the query.
  std::string highlighted;
  if (!predicate.Run(entry) ||
      !FindAndHighlight(entry.base_name(), queries, &highlighted))
    return FILE_ERROR_OK;

  // Hidden entry should not be returned.
  bool hidden = false;
  FileError error = hidden_entry_classifier->IsHidden(entry, &hidden);
  if (error != FILE_ERROR_OK || hidden)
    return error;

  // Make space for |entry| when appropriate.
  if (result_candidates->size() == at_most_num_matches)
    result_candidates->pop();
  result_candidates->push(
      std::make_unique<ResultCandidate>(it->GetID(), entry, highlighted));
  return FILE_ERROR_OK;
}

// Implements SearchMetadata().
FileError SearchMetadataOnBlockingPool(ResourceMetadata* resource_metadata,
                                       const std::string& query_text,
                                       const SearchMetadataPredicate& predicate,
                                       int at_most_num_matches,
                                       MetadataSearchOrder order,
                                       MetadataSearchResultVector* results) {
  ResultCandidateQueue result_candidates((ResultCandidateComparator(order)));

  // Prepare data structure for searching.
  std::vector<base::string16> keywords =
      base::SplitString(base::UTF8ToUTF16(query_text),
                        base::StringPiece16(base::kWhitespaceUTF16),
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<std::unique_ptr<
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>
      queries;
  for (const auto& keyword : keywords) {
    queries.push_back(
        std::make_unique<
            base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
            keyword));
  }

  // Prepare an object to filter out hidden entries.
  ResourceEntry mydrive;
  FileError error = resource_metadata->GetResourceEntryByPath(
      util::GetDriveMyDriveRootPath(), &mydrive);
  if (error != FILE_ERROR_OK)
    return error;
  HiddenEntryClassifier hidden_entry_classifier(resource_metadata,
                                                mydrive.local_id());

  // Iterate over entries.
  std::unique_ptr<ResourceMetadata::Iterator> it =
      resource_metadata->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    FileError error = MaybeAddEntryToResult(
        resource_metadata, it.get(), queries, predicate, at_most_num_matches,
        order, &hidden_entry_classifier, &result_candidates);
    if (error != FILE_ERROR_OK)
      return error;
  }

  // Prepare the result.
  for (; !result_candidates.empty(); result_candidates.pop()) {
    const ResultCandidate& candidate = *result_candidates.top();
    // The path field of entries in result_candidates are empty at this point,
    // because we don't want to run the expensive metadata DB look up except for
    // the final results. Hence, here we fill the part.
    base::FilePath path;
    error = resource_metadata->GetFilePath(candidate.local_id, &path);
    if (error != FILE_ERROR_OK)
      return error;
    bool is_directory = candidate.entry.file_info().is_directory();
    results->push_back(MetadataSearchResult(
        path, is_directory, candidate.highlighted_base_name,
        candidate.entry.file_specific_info().md5()));
  }

  // Reverse the order here because |result_candidates| puts the most
  // uninteresting candidate at the top.
  std::reverse(results->begin(), results->end());

  return FILE_ERROR_OK;
}

// Runs the SearchMetadataCallback and updates the histogram.
void RunSearchMetadataCallback(
    const SearchMetadataCallback& callback,
    const base::TimeTicks& start_time,
    std::unique_ptr<MetadataSearchResultVector> results,
    FileError error) {
  if (error != FILE_ERROR_OK)
    results.reset();
  callback.Run(error, std::move(results));

  UMA_HISTOGRAM_TIMES("Drive.SearchMetadataTime",
                      base::TimeTicks::Now() - start_time);
}

// Appends substring of |original_text| to |highlighted_text| with highlight.
void AppendStringWithHighlight(const base::string16& original_text,
                               size_t start,
                               size_t length,
                               bool highlight,
                               std::string* highlighted_text) {
  if (highlight)
    highlighted_text->append("<b>");

  highlighted_text->append(net::EscapeForHTML(
      base::UTF16ToUTF8(original_text.substr(start, length))));

  if (highlight)
    highlighted_text->append("</b>");
}

}  // namespace

void SearchMetadata(
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner,
    ResourceMetadata* resource_metadata,
    const std::string& query,
    const SearchMetadataPredicate& predicate,
    size_t at_most_num_matches,
    MetadataSearchOrder order,
    const SearchMetadataCallback& callback) {
  DCHECK(callback);

  const base::TimeTicks start_time = base::TimeTicks::Now();

  std::unique_ptr<MetadataSearchResultVector> results(
      new MetadataSearchResultVector);
  MetadataSearchResultVector* results_ptr = results.get();
  base::PostTaskAndReplyWithResult(
      blocking_task_runner.get(), FROM_HERE,
      base::BindOnce(&SearchMetadataOnBlockingPool, resource_metadata, query,
                     predicate, at_most_num_matches, order, results_ptr),
      base::BindOnce(&RunSearchMetadataCallback, callback, start_time,
                     std::move(results)));
}

bool MatchesType(int options, const ResourceEntry& entry) {
  if ((options & SEARCH_METADATA_EXCLUDE_HOSTED_DOCUMENTS) &&
      entry.file_specific_info().is_hosted_document())
    return false;

  if ((options & SEARCH_METADATA_EXCLUDE_DIRECTORIES) &&
      entry.file_info().is_directory())
    return false;

  if (options & SEARCH_METADATA_SHARED_WITH_ME)
    return entry.shared_with_me();

  if (options & SEARCH_METADATA_OFFLINE) {
    if (entry.file_specific_info().is_hosted_document()) {
      // Not all hosted documents are cached by Drive offline app.
      // https://support.google.com/drive/answer/2375012
      std::string mime_type = entry.file_specific_info().content_mime_type();
      return mime_type == drive::util::kGoogleDocumentMimeType ||
             mime_type == drive::util::kGoogleSpreadsheetMimeType ||
             mime_type == drive::util::kGooglePresentationMimeType ||
             mime_type == drive::util::kGoogleDrawingMimeType;
    }
    return entry.file_specific_info().cache_state().is_present();
  }

  return true;
}

bool FindAndHighlight(
    const std::string& text,
    const std::vector<std::unique_ptr<
        base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>& queries,
    std::string* highlighted_text) {
  DCHECK(highlighted_text);
  highlighted_text->clear();

  // Check text matches with all queries.
  size_t match_start = 0;
  size_t match_length = 0;

  base::string16 text16 = base::UTF8ToUTF16(text);
  std::vector<bool> highlights(text16.size(), false);
  for (const auto& query : queries) {
    if (!query->Search(text16, &match_start, &match_length))
      return false;

    std::fill(highlights.begin() + match_start,
              highlights.begin() + match_start + match_length, true);
  }

  // Generate highlighted text.
  size_t start_current_segment = 0;

  for (size_t i = 0; i < text16.size(); ++i) {
    if (highlights[start_current_segment] == highlights[i])
      continue;

    AppendStringWithHighlight(
        text16, start_current_segment, i - start_current_segment,
        highlights[start_current_segment], highlighted_text);

    start_current_segment = i;
  }

  DCHECK_GE(text16.size(), start_current_segment);
  AppendStringWithHighlight(
      text16, start_current_segment, text16.size() - start_current_segment,
      highlights[start_current_segment], highlighted_text);

  return true;
}

}  // namespace internal
}  // namespace drive
