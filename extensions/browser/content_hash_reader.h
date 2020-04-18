// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_CONTENT_HASH_READER_H_
#define EXTENSIONS_BROWSER_CONTENT_HASH_READER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/version.h"
#include "extensions/browser/content_verifier/content_verifier_key.h"
#include "extensions/common/extension_id.h"

namespace extensions {

class ContentHash;

// This class creates an object that will read expected hashes that may have
// been fetched/calculated by the ContentHashFetcher, and vends them out for
// use in ContentVerifyJob's.
class ContentHashReader {
 public:
  ~ContentHashReader();

  // Factory to create ContentHashReader to get expected hashes for the file at
  // |relative_path| within an extension.
  // Must be called on a thread that is allowed to do file I/O. Returns an
  // instance whose succees/failure can be determined by calling succeeded()
  // method. On failure, this object should likely be discarded.
  static std::unique_ptr<const ContentHashReader> Create(
      const base::FilePath& relative_path,
      const scoped_refptr<const ContentHash>& content_hash);

  bool succeeded() const { return status_ == SUCCESS; }

  // Returns true if we found valid verified_contents.json and
  // computed_hashes.json files. Note that this can be true even if we didn't
  // find an entry for |relative_path| in them.
  bool has_content_hashes() const { return has_content_hashes_; }
  // Returns whether or not this resource's entry exists in
  // verified_contents.json (given that |has_content_hashes_| is true.
  bool file_missing_from_verified_contents() const {
    return file_missing_from_verified_contents_;
  }

  // Return the number of blocks and block size, respectively. Only valid after
  // calling Init().
  int block_count() const;
  int block_size() const;

  // Returns a pointer to the expected sha256 hash value for the block at the
  // given index. Only valid after calling Init().
  bool GetHashForBlock(int block_index, const std::string** result) const;

 private:
  enum InitStatus { SUCCESS, FAILURE };

  ContentHashReader();

  InitStatus status_ = FAILURE;

  bool has_content_hashes_ = false;
  bool file_missing_from_verified_contents_ = false;

  // The blocksize used for generating the hashes.
  int block_size_ = 0;

  std::vector<std::string> hashes_;

  DISALLOW_COPY_AND_ASSIGN(ContentHashReader);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_CONTENT_HASH_READER_H_
