// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/safe_browsing/mac/dmg_analyzer.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/safe_browsing/archive_analyzer_results.h"
#include "chrome/common/safe_browsing/binary_feature_extractor.h"
#include "chrome/common/safe_browsing/mach_o_image_reader_mac.h"
#include "chrome/utility/safe_browsing/mac/dmg_iterator.h"
#include "chrome/utility/safe_browsing/mac/read_stream.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"

namespace safe_browsing {
namespace dmg {

namespace {

// MachOFeatureExtractor examines files to determine if they are Mach-O, and,
// if so, it uses the BinaryFeatureExtractor to obtain information about the
// image. In addition, this class will compute the SHA256 hash of the file.
class MachOFeatureExtractor {
 public:
  MachOFeatureExtractor();
  ~MachOFeatureExtractor();

  // Tests if the stream references a Mach-O image by examinig its magic
  // number.
  bool IsMachO(ReadStream* stream);

  // Computes the hash of the data in |stream| and extracts the Mach-O
  // features from the data. Returns true if successful, or false on error or
  // if the file was not Mach-O.
  bool ExtractFeatures(ReadStream* stream,
                       ClientDownloadRequest_ArchivedBinary* result);

 private:
  // Reads the entire stream and updates the hash.
  bool HashAndCopyStream(ReadStream* stream,
                         uint8_t digest[crypto::kSHA256Length]);

  scoped_refptr<BinaryFeatureExtractor> bfe_;
  std::vector<uint8_t> buffer_;  // Buffer that contains read stream data.

  DISALLOW_COPY_AND_ASSIGN(MachOFeatureExtractor);
};

MachOFeatureExtractor::MachOFeatureExtractor()
    : bfe_(new BinaryFeatureExtractor()),
      buffer_() {
  buffer_.reserve(1024 * 1024);
}

MachOFeatureExtractor::~MachOFeatureExtractor() {}

bool MachOFeatureExtractor::IsMachO(ReadStream* stream) {
  uint32_t magic = 0;
  return stream->ReadType<uint32_t>(&magic) &&
         MachOImageReader::IsMachOMagicValue(magic);
}

bool MachOFeatureExtractor::ExtractFeatures(
    ReadStream* stream,
    ClientDownloadRequest_ArchivedBinary* result) {
  uint8_t digest[crypto::kSHA256Length];
  if (!HashAndCopyStream(stream, digest))
    return false;

  if (!bfe_->ExtractImageFeaturesFromData(
          &buffer_[0], buffer_.size(), 0,
          result->mutable_image_headers(),
          result->mutable_signature()->mutable_signed_data())) {
    return false;
  }

  result->set_length(buffer_.size());
  result->mutable_digests()->set_sha256(digest, sizeof(digest));

  return true;
}

bool MachOFeatureExtractor::HashAndCopyStream(
    ReadStream* stream, uint8_t digest[crypto::kSHA256Length]) {
  if (stream->Seek(0, SEEK_SET) != 0)
    return false;

  buffer_.clear();
  std::unique_ptr<crypto::SecureHash> sha256(
      crypto::SecureHash::Create(crypto::SecureHash::SHA256));

  size_t bytes_read;
  const size_t kBufferSize = 2048;
  do {
    size_t buffer_offset = buffer_.size();

    buffer_.resize(buffer_.size() + kBufferSize);
    if (!stream->Read(&buffer_[buffer_offset], kBufferSize, &bytes_read))
      return false;

    buffer_.resize(buffer_offset + bytes_read);
    sha256->Update(&buffer_[buffer_offset], bytes_read);
  } while (bytes_read > 0);

  sha256->Finish(digest, crypto::kSHA256Length);

  return true;
}

}  // namespace

void AnalyzeDMGFile(base::File dmg_file, ArchiveAnalyzerResults* results) {
  MachOFeatureExtractor feature_extractor;
  results->success = false;

  FileReadStream read_stream(dmg_file.GetPlatformFile());
  DMGIterator iterator(&read_stream);
  if (!iterator.Open())
    return;

  results->signature_blob = iterator.GetCodeSignature();

  while (iterator.Next()) {
    std::unique_ptr<ReadStream> stream = iterator.GetReadStream();
    if (!stream || !feature_extractor.IsMachO(stream.get()))
      continue;

    ClientDownloadRequest_ArchivedBinary* binary =
        results->archived_binary.Add();
    binary->set_file_basename(base::UTF16ToUTF8(iterator.GetPath()));

    if (feature_extractor.ExtractFeatures(stream.get(), binary)) {
      binary->set_download_type(
          ClientDownloadRequest_DownloadType_MAC_EXECUTABLE);
      results->has_executable = true;
    } else {
      results->archived_binary.RemoveLast();
    }
  }

  results->success = true;
}

}  // namespace dmg
}  // namespace safe_browsing
