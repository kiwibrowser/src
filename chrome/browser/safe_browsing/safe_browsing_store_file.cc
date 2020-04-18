// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_store_file.h"

#include <stddef.h>

#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/md5.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "components/safe_browsing/db/prefix_set.h"

namespace safe_browsing {

namespace {

// NOTE(shess): kFileMagic should not be a byte-wise palindrome, so
// that byte-order changes force corruption.
const int32_t kFileMagic = 0x600D71FE;

// Version history:
// Version 6: aad08754/r2814 by erikkay@google.com on 2008-10-02 (sqlite)
// Version 7: 6afe28a5/r37435 by shess@chromium.org on 2010-01-28
// Version 8: d3dd0715/r259791 by shess@chromium.org on 2014-03-27
const int32_t kFileVersion = 8;

// ReadAndVerifyHeader() returns this in case of error.
const int32_t kInvalidVersion = -1;

// Starting with version 8, the storage is sorted and can be sharded to allow
// updates to be done with lower memory requirements.  Newly written files will
// be sharded to need less than this amount of memory during update.  Larger
// values are preferred to minimize looping overhead during processing.
const int64_t kUpdateStorageBytes = 100 * 1024;

// Prevent excessive sharding by setting a lower limit on the shard stride.
// Smaller values should work fine, but very small values will probably lead to
// poor performance.  Shard stride is indirectly related to
// |kUpdateStorageBytes|, setting that very small will bump against this.
const uint32_t kMinShardStride = 1 << 24;

// Strides over the entire SBPrefix space.
const uint64_t kMaxShardStride = 1ULL << 32;

// Maximum SBPrefix value.
const SBPrefix kMaxSBPrefix = 0xFFFFFFFF;

// Header at the front of the main database file.
struct FileHeader {
  int32_t magic, version;
  uint32_t add_chunk_count, sub_chunk_count;
  uint32_t shard_stride;
  // TODO(shess): Is this where 64-bit will bite me?  Perhaps write a
  // specialized read/write?
};

// Header for each chunk in the chunk-accumulation file.
struct ChunkHeader {
  uint32_t add_prefix_count, sub_prefix_count;
  uint32_t add_hash_count, sub_hash_count;
};

// Header for each shard of data in the main database file.
struct ShardHeader {
  uint32_t add_prefix_count, sub_prefix_count;
  uint32_t add_hash_count, sub_hash_count;
};

// Enumerate different format-change events for histogramming
// purposes.  DO NOT CHANGE THE ORDERING OF THESE VALUES.
enum FormatEventType {
  // Corruption detected, broken down by file format.
  FORMAT_EVENT_FILE_CORRUPT,
  FORMAT_EVENT_SQLITE_CORRUPT,  // Obsolete

  // The type of format found in the file.  The expected case (new
  // file format) is intentionally not covered.
  FORMAT_EVENT_FOUND_SQLITE,  // Obsolete
  FORMAT_EVENT_FOUND_UNKNOWN,  // magic does not match.

  // The number of SQLite-format files deleted should be the same as
  // FORMAT_EVENT_FOUND_SQLITE.  It can differ if the delete fails,
  // or if a failure prevents the update from succeeding.
  FORMAT_EVENT_SQLITE_DELETED,  // Obsolete
  FORMAT_EVENT_SQLITE_DELETE_FAILED,  // Obsolete

  // Found and deleted (or failed to delete) the ancient "Safe
  // Browsing" file.
  FORMAT_EVENT_DELETED_ORIGINAL,  // Obsolete
  FORMAT_EVENT_DELETED_ORIGINAL_FAILED,  // Obsolete

  // The checksum did not check out in CheckValidity() or in
  // FinishUpdate().  This most likely indicates that the machine
  // crashed before the file was fully sync'ed to disk.
  FORMAT_EVENT_VALIDITY_CHECKSUM_FAILURE,
  FORMAT_EVENT_UPDATE_CHECKSUM_FAILURE,

  // The header checksum was incorrect in ReadAndVerifyHeader().  Likely
  // indicates that the system crashed while writing an update.
  FORMAT_EVENT_HEADER_CHECKSUM_FAILURE,

  FORMAT_EVENT_FOUND_DEPRECATED,  // version too old.

  // Memory space for histograms is determined by the max.  ALWAYS
  // ADD NEW VALUES BEFORE THIS ONE.
  FORMAT_EVENT_MAX
};

void RecordFormatEvent(FormatEventType event_type) {
  UMA_HISTOGRAM_ENUMERATION("SB2.FormatEvent", event_type, FORMAT_EVENT_MAX);
}

// Rewind the file.  Using fseek(2) because rewind(3) errors are
// weird.
bool FileRewind(FILE* fp) {
  int rv = fseek(fp, 0, SEEK_SET);
  DCHECK_EQ(rv, 0);
  return rv == 0;
}

// Read from |fp| into |item|, and fold the input data into the
// checksum in |context|, if non-NULL.  Return true on success.
template <class T>
bool ReadItem(T* item, FILE* fp, base::MD5Context* context) {
  const size_t ret = fread(item, sizeof(T), 1, fp);
  if (ret != 1)
    return false;

  if (context) {
    base::MD5Update(context,
                    base::StringPiece(reinterpret_cast<char*>(item),
                                      sizeof(T)));
  }
  return true;
}

// Write |item| to |fp|, and fold the output data into the checksum in
// |context|, if non-NULL.  Return true on success.
template <class T>
bool WriteItem(const T& item, FILE* fp, base::MD5Context* context) {
  const size_t ret = fwrite(&item, sizeof(T), 1, fp);
  if (ret != 1)
    return false;

  if (context) {
    base::MD5Update(context,
                    base::StringPiece(reinterpret_cast<const char*>(&item),
                                      sizeof(T)));
  }

  return true;
}

// Read |count| items into |values| from |fp|, and fold them into the
// checksum in |context|.  Returns true on success.
template <typename CT>
bool ReadToContainer(CT* values, size_t count, FILE* fp,
                     base::MD5Context* context) {
  if (!count)
    return true;

  for (size_t i = 0; i < count; ++i) {
    typename CT::value_type value;
    if (!ReadItem(&value, fp, context))
      return false;

    // push_back() is more obvious, but coded this way std::set can
    // also be read.
    values->insert(values->end(), value);
  }

  return true;
}

// Write values between |beg| and |end| to |fp|, and fold the data into the
// checksum in |context|, if non-NULL.  Returns true if all items successful.
template <typename CTI>
bool WriteRange(const CTI& beg, const CTI& end,
                FILE* fp, base::MD5Context* context) {
  for (CTI iter = beg; iter != end; ++iter) {
    if (!WriteItem(*iter, fp, context))
      return false;
  }
  return true;
}

// Write all of |values| to |fp|, and fold the data into the checksum
// in |context|, if non-NULL.  Returns true if all items successful.
template <typename CT>
bool WriteContainer(const CT& values, FILE* fp,
                    base::MD5Context* context) {
  return WriteRange(values.begin(), values.end(), fp, context);
}

// Delete the chunks in |deleted| from |chunks|.
void DeleteChunksFromSet(const base::hash_set<int32_t>& deleted,
                         std::set<int32_t>* chunks) {
  for (std::set<int32_t>::iterator iter = chunks->begin();
       iter != chunks->end();) {
    std::set<int32_t>::iterator prev = iter++;
    if (deleted.count(*prev) > 0)
      chunks->erase(prev);
  }
}

bool ReadAndVerifyChecksum(FILE* fp, base::MD5Context* context) {
  base::MD5Digest calculated_digest;
  base::MD5IntermediateFinal(&calculated_digest, context);

  base::MD5Digest file_digest;
  if (!ReadItem(&file_digest, fp, context))
    return false;

  return memcmp(&file_digest, &calculated_digest, sizeof(file_digest)) == 0;
}

// Helper function to read the file header and chunk TOC.  Rewinds |fp| and
// initializes |context|.  The header is left in |header|, with the version
// returned.  kInvalidVersion is returned for sanity check or checksum failure.
int ReadAndVerifyHeader(const base::FilePath& filename,
                        FileHeader* header,
                        std::set<int32_t>* add_chunks,
                        std::set<int32_t>* sub_chunks,
                        FILE* fp,
                        base::MD5Context* context) {
  DCHECK(header);
  DCHECK(add_chunks);
  DCHECK(sub_chunks);
  DCHECK(fp);
  DCHECK(context);

  base::MD5Init(context);
  if (!FileRewind(fp))
    return kInvalidVersion;
  if (!ReadItem(header, fp, context))
    return kInvalidVersion;
  if (header->magic != kFileMagic)
    return kInvalidVersion;

  // Track version read to inform removal of support for older versions.
  base::UmaHistogramSparse("SB2.StoreVersionRead", header->version);

  if (header->version != kFileVersion)
    return kInvalidVersion;

  if (!ReadToContainer(add_chunks, header->add_chunk_count, fp, context) ||
      !ReadToContainer(sub_chunks, header->sub_chunk_count, fp, context)) {
    return kInvalidVersion;
  }

  // Verify that the data read thus far is valid.
  if (!ReadAndVerifyChecksum(fp, context)) {
    RecordFormatEvent(FORMAT_EVENT_HEADER_CHECKSUM_FAILURE);
    return kInvalidVersion;
  }

  return kFileVersion;
}

// Helper function to write out the initial header and chunks-contained data.
// Rewinds |fp|, initializes |context|, then writes a file header and
// |add_chunks| and |sub_chunks|.
bool WriteHeader(uint32_t out_stride,
                 const std::set<int32_t>& add_chunks,
                 const std::set<int32_t>& sub_chunks,
                 FILE* fp,
                 base::MD5Context* context) {
  if (!FileRewind(fp))
    return false;

  base::MD5Init(context);
  FileHeader header;
  header.magic = kFileMagic;
  header.version = kFileVersion;
  header.add_chunk_count = add_chunks.size();
  header.sub_chunk_count = sub_chunks.size();
  header.shard_stride = out_stride;
  if (!WriteItem(header, fp, context))
    return false;

  if (!WriteContainer(add_chunks, fp, context) ||
      !WriteContainer(sub_chunks, fp, context))
    return false;

  // Write out the header digest.
  base::MD5Digest header_digest;
  base::MD5IntermediateFinal(&header_digest, context);
  if (!WriteItem(header_digest, fp, context))
    return false;

  return true;
}

// Return |true| if the range is sorted by the given comparator.
template <typename CTI, typename LESS>
bool sorted(CTI beg, CTI end, LESS less) {
  while ((end - beg) > 2) {
    CTI n = beg++;
    DCHECK(!less(*beg, *n));
    if (less(*beg, *n))
      return false;
  }
  return true;
}

// Merge |beg|..|end| into |container|.  Both should be sorted by the given
// comparator, and the range iterators should not be derived from |container|.
// Differs from std::inplace_merge() in that additional memory is not required
// for linear performance.
template <typename CT, typename CTI, typename COMP>
void container_merge(CT* container, CTI beg, CTI end, const COMP& less) {
  DCHECK(sorted(container->begin(), container->end(), less));
  DCHECK(sorted(beg, end, less));

  // Size the container to fit the results.
  const size_t c_size = container->size();
  container->resize(c_size + (end - beg));

  // |c_end| points to the original endpoint, while |c_out| points to the
  // endpoint that will scan from end to beginning while merging.
  typename CT::iterator c_end = container->begin() + c_size;
  typename CT::iterator c_out = container->end();

  // While both inputs have data, move the greater to |c_out|.
  while (c_end != container->begin() && end != beg) {
    if (less(*(c_end - 1), *(end - 1))) {
      *(--c_out) = *(--end);
    } else {
      *(--c_out) = *(--c_end);
    }
  }

  // Copy any data remaining in the new range.
  if (end != beg) {
    // The original container data has been fully shifted.
    DCHECK(c_end == container->begin());

    // There is exactly the correct amount of space left.
    DCHECK_EQ(c_out - c_end, end - beg);

    std::copy(beg, end, container->begin());
  }

  DCHECK(sorted(container->begin(), container->end(), less));
}

// Collection of iterators used while stepping through StateInternal (see
// below).
class StateInternalPos {
 public:
  StateInternalPos(SBAddPrefixes::iterator add_prefixes_iter,
                   SBSubPrefixes::iterator sub_prefixes_iter,
                   std::vector<SBAddFullHash>::iterator add_hashes_iter,
                   std::vector<SBSubFullHash>::iterator sub_hashes_iter)
      : add_prefixes_iter_(add_prefixes_iter),
        sub_prefixes_iter_(sub_prefixes_iter),
        add_hashes_iter_(add_hashes_iter),
        sub_hashes_iter_(sub_hashes_iter) {
  }

  SBAddPrefixes::iterator add_prefixes_iter_;
  SBSubPrefixes::iterator sub_prefixes_iter_;
  std::vector<SBAddFullHash>::iterator add_hashes_iter_;
  std::vector<SBSubFullHash>::iterator sub_hashes_iter_;
};

// Helper to find the next shard boundary.
template <class T>
bool prefix_bounder(SBPrefix val, const T& elt) {
  return val < elt.GetAddPrefix();
}

// Container for partial database state.  Includes add/sub prefixes/hashes, plus
// aggregate operations on same.
class StateInternal {
 public:
  // Append indicated amount of data from |fp|.
  bool AppendData(size_t add_prefix_count, size_t sub_prefix_count,
                  size_t add_hash_count, size_t sub_hash_count,
                  FILE* fp, base::MD5Context* context) {
    return
        ReadToContainer(&add_prefixes_, add_prefix_count, fp, context) &&
        ReadToContainer(&sub_prefixes_, sub_prefix_count, fp, context) &&
        ReadToContainer(&add_full_hashes_, add_hash_count, fp, context) &&
        ReadToContainer(&sub_full_hashes_, sub_hash_count, fp, context);
  }

  void ClearData() {
    add_prefixes_.clear();
    sub_prefixes_.clear();
    add_full_hashes_.clear();
    sub_full_hashes_.clear();
  }

  // Merge data from |beg|..|end| into receiver's state, then process the state.
  // The current state and the range given should corrospond to the same sorted
  // shard of data from different sources.  |add_del_cache| and |sub_del_cache|
  // indicate the chunk ids which should be deleted during processing (see
  // SBProcessSubs).
  void MergeDataAndProcess(const StateInternalPos& beg,
                           const StateInternalPos& end,
                           const base::hash_set<int32_t>& add_del_cache,
                           const base::hash_set<int32_t>& sub_del_cache) {
    container_merge(&add_prefixes_,
                    beg.add_prefixes_iter_,
                    end.add_prefixes_iter_,
                    SBAddPrefixLess<SBAddPrefix, SBAddPrefix>);

    container_merge(&sub_prefixes_,
                    beg.sub_prefixes_iter_,
                    end.sub_prefixes_iter_,
                    SBAddPrefixLess<SBSubPrefix, SBSubPrefix>);

    container_merge(&add_full_hashes_,
                    beg.add_hashes_iter_,
                    end.add_hashes_iter_,
                    SBAddPrefixHashLess<SBAddFullHash, SBAddFullHash>);

    container_merge(&sub_full_hashes_,
                    beg.sub_hashes_iter_,
                    end.sub_hashes_iter_,
                    SBAddPrefixHashLess<SBSubFullHash, SBSubFullHash>);

    SBProcessSubs(&add_prefixes_, &sub_prefixes_,
                  &add_full_hashes_, &sub_full_hashes_,
                  add_del_cache, sub_del_cache);
  }

  // Sort the data appropriately for the sharding, merging, and processing
  // operations.
  void SortData() {
    std::sort(add_prefixes_.begin(), add_prefixes_.end(),
              SBAddPrefixLess<SBAddPrefix, SBAddPrefix>);
    std::sort(sub_prefixes_.begin(), sub_prefixes_.end(),
              SBAddPrefixLess<SBSubPrefix, SBSubPrefix>);
    std::sort(add_full_hashes_.begin(), add_full_hashes_.end(),
              SBAddPrefixHashLess<SBAddFullHash, SBAddFullHash>);
    std::sort(sub_full_hashes_.begin(), sub_full_hashes_.end(),
              SBAddPrefixHashLess<SBSubFullHash, SBSubFullHash>);
  }

  // Iterator from the beginning of the state's data.
  StateInternalPos StateBegin() {
    return StateInternalPos(add_prefixes_.begin(),
                            sub_prefixes_.begin(),
                            add_full_hashes_.begin(),
                            sub_full_hashes_.begin());
  }

  // An iterator pointing just after the last possible element of the shard
  // indicated by |shard_max|.  Used to step through the state by shard.
  // TODO(shess): Verify whether binary search really improves over linear.
  // Merging or writing will immediately touch all of these elements.
  StateInternalPos ShardEnd(const StateInternalPos& beg, SBPrefix shard_max) {
    return StateInternalPos(
        std::upper_bound(beg.add_prefixes_iter_, add_prefixes_.end(),
                         shard_max, prefix_bounder<SBAddPrefix>),
        std::upper_bound(beg.sub_prefixes_iter_, sub_prefixes_.end(),
                         shard_max, prefix_bounder<SBSubPrefix>),
        std::upper_bound(beg.add_hashes_iter_, add_full_hashes_.end(),
                         shard_max, prefix_bounder<SBAddFullHash>),
        std::upper_bound(beg.sub_hashes_iter_, sub_full_hashes_.end(),
                         shard_max, prefix_bounder<SBSubFullHash>));
  }

  // Write a shard header and data for the shard starting at |beg| and ending at
  // the element before |end|.
  bool WriteShard(const StateInternalPos& beg, const StateInternalPos& end,
                  FILE* fp, base::MD5Context* context) {
    ShardHeader shard_header;
    shard_header.add_prefix_count =
        end.add_prefixes_iter_ - beg.add_prefixes_iter_;
    shard_header.sub_prefix_count =
        end.sub_prefixes_iter_ - beg.sub_prefixes_iter_;
    shard_header.add_hash_count =
        end.add_hashes_iter_ - beg.add_hashes_iter_;
    shard_header.sub_hash_count =
        end.sub_hashes_iter_ - beg.sub_hashes_iter_;

    return
        WriteItem(shard_header, fp, context) &&
        WriteRange(beg.add_prefixes_iter_, end.add_prefixes_iter_,
                   fp, context) &&
        WriteRange(beg.sub_prefixes_iter_, end.sub_prefixes_iter_,
                   fp, context) &&
        WriteRange(beg.add_hashes_iter_, end.add_hashes_iter_,
                   fp, context) &&
        WriteRange(beg.sub_hashes_iter_, end.sub_hashes_iter_,
                   fp, context);
  }

  SBAddPrefixes add_prefixes_;
  SBSubPrefixes sub_prefixes_;
  std::vector<SBAddFullHash> add_full_hashes_;
  std::vector<SBSubFullHash> sub_full_hashes_;
};

// True if |val| is an even power of two.
template <typename T>
bool IsPowerOfTwo(const T& val) {
  return val && (val & (val - 1)) == 0;
}

// Helper to read the entire database state, used by GetAddPrefixes() and
// GetAddFullHashes().  Those functions are generally used only for smaller
// files.  Returns false in case of errors reading the data.
bool ReadDbStateHelper(const base::FilePath& filename,
                       StateInternal* db_state) {
  base::ScopedFILE file(base::OpenFile(filename, "rb"));
  if (file.get() == NULL)
    return false;

  std::set<int32_t> add_chunks;
  std::set<int32_t> sub_chunks;

  base::MD5Context context;
  FileHeader header;
  const int version =
      ReadAndVerifyHeader(filename, &header, &add_chunks, &sub_chunks,
                          file.get(), &context);
  if (version == kInvalidVersion)
    return false;

  uint64_t in_min = 0;
  uint64_t in_stride = header.shard_stride;
  if (!in_stride)
    in_stride = kMaxShardStride;
  if (!IsPowerOfTwo(in_stride))
    return false;

  do {
    ShardHeader shard_header;
    if (!ReadItem(&shard_header, file.get(), &context))
      return false;

    if (!db_state->AppendData(shard_header.add_prefix_count,
                              shard_header.sub_prefix_count,
                              shard_header.add_hash_count,
                              shard_header.sub_hash_count,
                              file.get(), &context)) {
      return false;
    }

    in_min += in_stride;
  } while (in_min <= kMaxSBPrefix);

  if (!ReadAndVerifyChecksum(file.get(), &context))
    return false;

  int64_t size = 0;
  if (!base::GetFileSize(filename, &size))
    return false;

  return static_cast<int64_t>(ftell(file.get())) == size;
}

}  // namespace

SafeBrowsingStoreFile::SafeBrowsingStoreFile(
    const scoped_refptr<const base::SequencedTaskRunner>& task_runner)
    : task_runner_(task_runner),
      chunks_written_(0),
      empty_(false),
      corruption_seen_(false) {
}

SafeBrowsingStoreFile::~SafeBrowsingStoreFile() {
  // Thread-checking is disabled in the destructor due to crbug.com/338486.
  task_runner_ = nullptr;

  Close();
}

bool SafeBrowsingStoreFile::CalledOnValidThread() {
  return !task_runner_ || task_runner_->RunsTasksInCurrentSequence();
}

bool SafeBrowsingStoreFile::Delete() {
  DCHECK(CalledOnValidThread());

  // The database should not be open at this point.  But, just in
  // case, close everything before deleting.
  if (!Close()) {
    NOTREACHED();
    return false;
  }

  return DeleteStore(filename_);
}

bool SafeBrowsingStoreFile::CheckValidity() {
  DCHECK(CalledOnValidThread());

  // The file was either empty or never opened.  The empty case is
  // presumed not to be invalid.  The never-opened case can happen if
  // BeginUpdate() fails for any databases, and should already have
  // caused the corruption callback to fire.
  if (!file_.get())
    return true;

  if (!FileRewind(file_.get()))
    return OnCorruptDatabase();

  int64_t size = 0;
  if (!base::GetFileSize(filename_, &size))
    return OnCorruptDatabase();

  base::MD5Context context;
  base::MD5Init(&context);

  // Read everything except the final digest.
  size_t bytes_left = static_cast<size_t>(size);
  CHECK(size == static_cast<int64_t>(bytes_left));
  if (bytes_left < sizeof(base::MD5Digest))
    return OnCorruptDatabase();
  bytes_left -= sizeof(base::MD5Digest);

  // Fold the contents of the file into the checksum.
  while (bytes_left > 0) {
    char buf[4096];
    const size_t c = std::min(sizeof(buf), bytes_left);
    const size_t ret = fread(buf, 1, c, file_.get());

    // The file's size changed while reading, give up.
    if (ret != c)
      return OnCorruptDatabase();
    base::MD5Update(&context, base::StringPiece(buf, c));
    bytes_left -= c;
  }

  if (!ReadAndVerifyChecksum(file_.get(), &context)) {
    RecordFormatEvent(FORMAT_EVENT_VALIDITY_CHECKSUM_FAILURE);
    return OnCorruptDatabase();
  }

  return true;
}

void SafeBrowsingStoreFile::Init(const base::FilePath& filename,
                                 const base::Closure& corruption_callback) {
  DCHECK(CalledOnValidThread());
  filename_ = filename;
  corruption_callback_ = corruption_callback;
}

bool SafeBrowsingStoreFile::BeginChunk() {
  DCHECK(CalledOnValidThread());
  return ClearChunkBuffers();
}

bool SafeBrowsingStoreFile::WriteAddPrefix(int32_t chunk_id, SBPrefix prefix) {
  DCHECK(CalledOnValidThread());
  add_prefixes_.push_back(SBAddPrefix(chunk_id, prefix));
  return true;
}

bool SafeBrowsingStoreFile::GetAddPrefixes(SBAddPrefixes* add_prefixes) {
  DCHECK(CalledOnValidThread());

  add_prefixes->clear();
  if (!base::PathExists(filename_))
    return true;

  StateInternal db_state;
  if (!ReadDbStateHelper(filename_, &db_state))
    return OnCorruptDatabase();

  add_prefixes->swap(db_state.add_prefixes_);
  return true;
}

bool SafeBrowsingStoreFile::GetAddFullHashes(
    std::vector<SBAddFullHash>* add_full_hashes) {
  DCHECK(CalledOnValidThread());

  add_full_hashes->clear();
  if (!base::PathExists(filename_))
    return true;

  StateInternal db_state;
  if (!ReadDbStateHelper(filename_, &db_state))
    return OnCorruptDatabase();

  add_full_hashes->swap(db_state.add_full_hashes_);
  return true;
}

bool SafeBrowsingStoreFile::WriteAddHash(int32_t chunk_id,
                                         const SBFullHash& full_hash) {
  DCHECK(CalledOnValidThread());
  add_hashes_.push_back(SBAddFullHash(chunk_id, full_hash));
  return true;
}

bool SafeBrowsingStoreFile::WriteSubPrefix(int32_t chunk_id,
                                           int32_t add_chunk_id,
                                           SBPrefix prefix) {
  DCHECK(CalledOnValidThread());
  sub_prefixes_.push_back(SBSubPrefix(chunk_id, add_chunk_id, prefix));
  return true;
}

bool SafeBrowsingStoreFile::WriteSubHash(int32_t chunk_id,
                                         int32_t add_chunk_id,
                                         const SBFullHash& full_hash) {
  DCHECK(CalledOnValidThread());
  sub_hashes_.push_back(SBSubFullHash(chunk_id, add_chunk_id, full_hash));
  return true;
}

bool SafeBrowsingStoreFile::OnCorruptDatabase() {
  DCHECK(CalledOnValidThread());

  if (!corruption_seen_)
    RecordFormatEvent(FORMAT_EVENT_FILE_CORRUPT);
  corruption_seen_ = true;

  corruption_callback_.Run();

  // Return false as a convenience to callers.
  return false;
}

bool SafeBrowsingStoreFile::Close() {
  DCHECK(CalledOnValidThread());

  ClearUpdateBuffers();

  // Make sure the files are closed.
  file_.reset();
  new_file_.reset();
  return true;
}

bool SafeBrowsingStoreFile::BeginUpdate() {
  DCHECK(CalledOnValidThread());
  DCHECK(!file_.get() && !new_file_.get());

  // Structures should all be clear unless something bad happened.
  DCHECK(add_chunks_cache_.empty());
  DCHECK(sub_chunks_cache_.empty());
  DCHECK(add_del_cache_.empty());
  DCHECK(sub_del_cache_.empty());
  DCHECK(add_prefixes_.empty());
  DCHECK(sub_prefixes_.empty());
  DCHECK(add_hashes_.empty());
  DCHECK(sub_hashes_.empty());
  DCHECK_EQ(chunks_written_, 0);

  corruption_seen_ = false;

  const base::FilePath new_filename = TemporaryFileForFilename(filename_);
  base::ScopedFILE new_file(base::OpenFile(new_filename, "wb+"));
  if (new_file.get() == NULL)
    return false;

  base::ScopedFILE file(base::OpenFile(filename_, "rb"));
  empty_ = (file.get() == NULL);
  if (empty_) {
    // If the file exists but cannot be opened, try to delete it (not
    // deleting directly, the bloom filter needs to be deleted, too).
    if (base::PathExists(filename_))
      return OnCorruptDatabase();

    new_file_.swap(new_file);
    return true;
  }

  base::MD5Context context;
  FileHeader header;
  const int version =
      ReadAndVerifyHeader(filename_, &header,
                          &add_chunks_cache_, &sub_chunks_cache_,
                          file.get(), &context);
  if (version == kInvalidVersion) {
    FileHeader retry_header;
    if (FileRewind(file.get()) && ReadItem(&retry_header, file.get(), NULL)) {
      if (retry_header.magic == kFileMagic &&
          retry_header.version < kFileVersion) {
        RecordFormatEvent(FORMAT_EVENT_FOUND_DEPRECATED);
      } else {
        RecordFormatEvent(FORMAT_EVENT_FOUND_UNKNOWN);
      }
    }

    // Close the file so that it can be deleted.
    file.reset();

    return OnCorruptDatabase();
  }

  file_.swap(file);
  new_file_.swap(new_file);
  return true;
}

bool SafeBrowsingStoreFile::FinishChunk() {
  DCHECK(CalledOnValidThread());

  if (add_prefixes_.empty() && sub_prefixes_.empty() &&
      add_hashes_.empty() && sub_hashes_.empty())
    return true;

  ChunkHeader header;
  header.add_prefix_count = add_prefixes_.size();
  header.sub_prefix_count = sub_prefixes_.size();
  header.add_hash_count = add_hashes_.size();
  header.sub_hash_count = sub_hashes_.size();
  if (!WriteItem(header, new_file_.get(), NULL))
    return false;

  if (!WriteContainer(add_prefixes_, new_file_.get(), NULL) ||
      !WriteContainer(sub_prefixes_, new_file_.get(), NULL) ||
      !WriteContainer(add_hashes_, new_file_.get(), NULL) ||
      !WriteContainer(sub_hashes_, new_file_.get(), NULL))
    return false;

  ++chunks_written_;

  // Clear everything to save memory.
  return ClearChunkBuffers();
}

bool SafeBrowsingStoreFile::DoUpdate(
    PrefixSetBuilder* builder,
    std::vector<SBAddFullHash>* add_full_hashes_result) {
  DCHECK(CalledOnValidThread());
  DCHECK(file_.get() || empty_);
  DCHECK(new_file_.get());
  CHECK(builder);
  CHECK(add_full_hashes_result);

  // Rewind the temporary storage.
  if (!FileRewind(new_file_.get()))
    return false;

  // Get chunk file's size for validating counts.
  int64_t update_size = 0;
  if (!base::GetFileSize(TemporaryFileForFilename(filename_), &update_size))
    return OnCorruptDatabase();

  // Track update size to answer questions at http://crbug.com/72216 .
  // Log small updates as 1k so that the 0 (underflow) bucket can be
  // used for "empty" in SafeBrowsingDatabase.
  UMA_HISTOGRAM_COUNTS("SB2.DatabaseUpdateKilobytes",
                       std::max(static_cast<int>(update_size / 1024), 1));

  // Chunk updates to integrate.
  StateInternal new_state;

  // Read update chunks.
  for (int i = 0; i < chunks_written_; ++i) {
    ChunkHeader header;

    int64_t ofs = ftell(new_file_.get());
    if (ofs == -1)
      return false;

    if (!ReadItem(&header, new_file_.get(), NULL))
      return false;

    // As a safety measure, make sure that the header describes a sane
    // chunk, given the remaining file size.
    int64_t expected_size = ofs + sizeof(ChunkHeader);
    expected_size += header.add_prefix_count * sizeof(SBAddPrefix);
    expected_size += header.sub_prefix_count * sizeof(SBSubPrefix);
    expected_size += header.add_hash_count * sizeof(SBAddFullHash);
    expected_size += header.sub_hash_count * sizeof(SBSubFullHash);
    if (expected_size > update_size)
      return false;

    if (!new_state.AppendData(header.add_prefix_count, header.sub_prefix_count,
                              header.add_hash_count, header.sub_hash_count,
                              new_file_.get(), NULL)) {
      return false;
    }
  }

  // The state was accumulated by chunk, sort by prefix.
  new_state.SortData();

  // These strides control how much data is loaded into memory per pass.
  // Strides must be an even power of two.  |in_stride| will be derived from the
  // input file.  |out_stride| will be derived from an estimate of the resulting
  // file's size.  |process_stride| will be the max of both.
  uint64_t in_stride = kMaxShardStride;
  uint64_t out_stride = kMaxShardStride;
  uint64_t process_stride = 0;

  // Used to verify the input's checksum if |!empty_|.
  base::MD5Context in_context;

  if (!empty_) {
    DCHECK(file_.get());

    FileHeader header = {0};
    int version = ReadAndVerifyHeader(filename_, &header,
                                      &add_chunks_cache_, &sub_chunks_cache_,
                                      file_.get(), &in_context);
    if (version == kInvalidVersion)
      return OnCorruptDatabase();

    if (header.shard_stride)
      in_stride = header.shard_stride;

    // The header checksum should have prevented this case, but the code will be
    // broken if this is not correct.
    if (!IsPowerOfTwo(in_stride))
      return OnCorruptDatabase();
  }

  // We no longer need to track deleted chunks.
  DeleteChunksFromSet(add_del_cache_, &add_chunks_cache_);
  DeleteChunksFromSet(sub_del_cache_, &sub_chunks_cache_);

  // Calculate |out_stride| to break the file down into reasonable shards.
  {
    int64_t original_size = 0;
    if (!empty_ && !base::GetFileSize(filename_, &original_size))
      return OnCorruptDatabase();

    // Approximate the final size as everything.  Subs and deletes will reduce
    // the size, but modest over-sharding won't hurt much.
    int64_t shard_size = original_size + update_size;

    // Keep splitting until a single stride of data fits the target.
    size_t shifts = 0;
    while (out_stride > kMinShardStride && shard_size > kUpdateStorageBytes) {
      out_stride >>= 1;
      shard_size >>= 1;
      ++shifts;
    }
    UMA_HISTOGRAM_COUNTS("SB2.OutShardShifts", shifts);

    DCHECK(IsPowerOfTwo(out_stride));
  }

  // Outer loop strides by the max of the input stride (to read integral shards)
  // and the output stride (to write integral shards).
  process_stride = std::max(in_stride, out_stride);
  DCHECK(IsPowerOfTwo(process_stride));
  DCHECK_EQ(0u, process_stride % in_stride);
  DCHECK_EQ(0u, process_stride % out_stride);

  // Start writing the new data to |new_file_|.
  base::MD5Context out_context;
  if (!WriteHeader(out_stride, add_chunks_cache_, sub_chunks_cache_,
                   new_file_.get(), &out_context)) {
    return false;
  }

  // Start at the beginning of the SBPrefix space.
  uint64_t in_min = 0;
  uint64_t out_min = 0;
  uint64_t process_min = 0;

  // Start at the beginning of the updates.
  StateInternalPos new_pos = new_state.StateBegin();

  // Re-usable container for shard processing.
  StateInternal db_state;

  // Track aggregate counts for histograms.
  size_t add_prefix_count = 0;
  size_t sub_prefix_count = 0;

  do {
    // Maximum element in the current shard.
    SBPrefix process_max =
        static_cast<SBPrefix>(process_min + process_stride - 1);
    DCHECK_GT(process_max, process_min);

    // Drop the data from previous pass.
    db_state.ClearData();

    // Fill the processing shard with one or more input shards.
    if (!empty_) {
      do {
        ShardHeader shard_header;
        if (!ReadItem(&shard_header, file_.get(), &in_context))
          return OnCorruptDatabase();

        if (!db_state.AppendData(shard_header.add_prefix_count,
                                 shard_header.sub_prefix_count,
                                 shard_header.add_hash_count,
                                 shard_header.sub_hash_count,
                                 file_.get(), &in_context))
          return OnCorruptDatabase();

        in_min += in_stride;
      } while (in_min <= kMaxSBPrefix && in_min < process_max);
    }

    // Shard the update data to match the database data, then merge the update
    // data and process the results.
    {
      StateInternalPos new_end = new_state.ShardEnd(new_pos, process_max);
      db_state.MergeDataAndProcess(new_pos, new_end,
                                   add_del_cache_, sub_del_cache_);
      new_pos = new_end;
    }

    // Collect the processed data for return to caller.
    for (size_t i = 0; i < db_state.add_prefixes_.size(); ++i) {
      builder->AddPrefix(db_state.add_prefixes_[i].prefix);
    }
    add_full_hashes_result->insert(add_full_hashes_result->end(),
                                   db_state.add_full_hashes_.begin(),
                                   db_state.add_full_hashes_.end());
    add_prefix_count += db_state.add_prefixes_.size();
    sub_prefix_count += db_state.sub_prefixes_.size();

    // Write one or more shards of processed output.
    StateInternalPos out_pos = db_state.StateBegin();
    do {
      SBPrefix out_max = static_cast<SBPrefix>(out_min + out_stride - 1);
      DCHECK_GT(out_max, out_min);

      StateInternalPos out_end = db_state.ShardEnd(out_pos, out_max);
      if (!db_state.WriteShard(out_pos, out_end, new_file_.get(), &out_context))
        return false;
      out_pos = out_end;

      out_min += out_stride;
    } while (out_min == static_cast<SBPrefix>(out_min) &&
             out_min < process_max);

    process_min += process_stride;
  } while (process_min <= kMaxSBPrefix);

  // Verify the overall checksum.
  if (!empty_) {
    if (!ReadAndVerifyChecksum(file_.get(), &in_context)) {
      RecordFormatEvent(FORMAT_EVENT_UPDATE_CHECKSUM_FAILURE);
      return OnCorruptDatabase();
    }

    // TODO(shess): Verify EOF?

    // Close the input file so the new file can be renamed over it.
    file_.reset();
  }
  DCHECK(!file_.get());

  // Write the overall checksum.
  base::MD5Digest out_digest;
  base::MD5Final(&out_digest, &out_context);
  if (!WriteItem(out_digest, new_file_.get(), NULL))
    return false;

  // Trim any excess left over from the temporary chunk data.
  if (!base::TruncateFile(new_file_.get()))
    return false;

  // Close the file handle and swizzle the file into place.
  new_file_.reset();
  if (!base::DeleteFile(filename_, false) &&
      base::PathExists(filename_))
    return false;

  const base::FilePath new_filename = TemporaryFileForFilename(filename_);
  if (!base::Move(new_filename, filename_))
    return false;

  // Record counts before swapping to caller.
  UMA_HISTOGRAM_COUNTS("SB2.AddPrefixes", add_prefix_count);
  UMA_HISTOGRAM_COUNTS("SB2.SubPrefixes", sub_prefix_count);

  return true;
}

bool SafeBrowsingStoreFile::FinishUpdate(
    PrefixSetBuilder* builder,
    std::vector<SBAddFullHash>* add_full_hashes_result) {
  DCHECK(CalledOnValidThread());
  DCHECK(builder);
  DCHECK(add_full_hashes_result);

  if (!DoUpdate(builder, add_full_hashes_result)) {
    CancelUpdate();
    return false;
  }

  DCHECK(!new_file_.get());
  DCHECK(!file_.get());

  return Close();
}

bool SafeBrowsingStoreFile::CancelUpdate() {
  DCHECK(CalledOnValidThread());
  bool ret = Close();

  // Delete stale staging file.
  const base::FilePath new_filename = TemporaryFileForFilename(filename_);
  base::DeleteFile(new_filename, false);

  return ret;
}

void SafeBrowsingStoreFile::SetAddChunk(int32_t chunk_id) {
  DCHECK(CalledOnValidThread());
  add_chunks_cache_.insert(chunk_id);
}

bool SafeBrowsingStoreFile::CheckAddChunk(int32_t chunk_id) {
  DCHECK(CalledOnValidThread());
  return add_chunks_cache_.count(chunk_id) > 0;
}

void SafeBrowsingStoreFile::GetAddChunks(std::vector<int32_t>* out) {
  DCHECK(CalledOnValidThread());
  out->clear();
  out->insert(out->end(), add_chunks_cache_.begin(), add_chunks_cache_.end());
}

void SafeBrowsingStoreFile::SetSubChunk(int32_t chunk_id) {
  DCHECK(CalledOnValidThread());
  sub_chunks_cache_.insert(chunk_id);
}

bool SafeBrowsingStoreFile::CheckSubChunk(int32_t chunk_id) {
  DCHECK(CalledOnValidThread());
  return sub_chunks_cache_.count(chunk_id) > 0;
}

void SafeBrowsingStoreFile::GetSubChunks(std::vector<int32_t>* out) {
  DCHECK(CalledOnValidThread());
  out->clear();
  out->insert(out->end(), sub_chunks_cache_.begin(), sub_chunks_cache_.end());
}

void SafeBrowsingStoreFile::DeleteAddChunk(int32_t chunk_id) {
  DCHECK(CalledOnValidThread());
  add_del_cache_.insert(chunk_id);
}

void SafeBrowsingStoreFile::DeleteSubChunk(int32_t chunk_id) {
  DCHECK(CalledOnValidThread());
  sub_del_cache_.insert(chunk_id);
}

// static
bool SafeBrowsingStoreFile::DeleteStore(const base::FilePath& basename) {
  if (!base::DeleteFile(basename, false) &&
      base::PathExists(basename)) {
    NOTREACHED();
    return false;
  }

  const base::FilePath new_filename = TemporaryFileForFilename(basename);
  if (!base::DeleteFile(new_filename, false) &&
      base::PathExists(new_filename)) {
    NOTREACHED();
    return false;
  }

  // With SQLite support gone, one way to get to this code is if the
  // existing file is a SQLite file.  Make sure the journal file is
  // also removed.
  const base::FilePath journal_filename(
      basename.value() + FILE_PATH_LITERAL("-journal"));
  if (base::PathExists(journal_filename))
    base::DeleteFile(journal_filename, false);

  return true;
}

}  // namespace safe_browsing
