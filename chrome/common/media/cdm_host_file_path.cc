// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media/cdm_host_file_path.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "chrome/common/chrome_version.h"

#if defined(OS_MACOSX)
#include "base/mac/bundle_locations.h"
#include "chrome/common/chrome_constants.h"
#endif

#if defined(GOOGLE_CHROME_BUILD)

namespace {

// TODO(xhwang): Move this to a common place if needed.
const base::FilePath::CharType kSignatureFileExtension[] =
    FILE_PATH_LITERAL(".sig");

// Returns the signature file path given the |file_path|. This function should
// only be used when the signature file and the file are located in the same
// directory.
base::FilePath GetSigFilePath(const base::FilePath& file_path) {
  return file_path.AddExtension(kSignatureFileExtension);
}

}  // namespace

void AddCdmHostFilePaths(
    std::vector<media::CdmHostFilePath>* cdm_host_file_paths) {
  DVLOG(1) << __func__;
  DCHECK(cdm_host_file_paths);
  DCHECK(cdm_host_file_paths->empty());

#if defined(OS_WIN)

  static const base::FilePath::CharType* const kUnversionedFiles[] = {
      FILE_PATH_LITERAL("chrome.exe")};
  static const base::FilePath::CharType* const kVersionedFiles[] = {
      FILE_PATH_LITERAL("chrome.dll"), FILE_PATH_LITERAL("chrome_child.dll")};

  // Find where chrome.exe is installed.
  base::FilePath chrome_exe_dir;
  if (!base::PathService::Get(base::DIR_EXE, &chrome_exe_dir))
    NOTREACHED();
  base::FilePath version_dir(chrome_exe_dir.AppendASCII(CHROME_VERSION_STRING));

  cdm_host_file_paths->reserve(arraysize(kUnversionedFiles) +
                               arraysize(kVersionedFiles));

  // Signature files are always in the version directory.
  for (size_t i = 0; i < arraysize(kUnversionedFiles); ++i) {
    base::FilePath file_path = chrome_exe_dir.Append(kUnversionedFiles[i]);
    base::FilePath sig_path =
        GetSigFilePath(version_dir.Append(kUnversionedFiles[i]));
    DVLOG(2) << __func__ << ": unversioned file " << i << " at "
             << file_path.value() << ", signature file " << sig_path.value();
    cdm_host_file_paths->emplace_back(file_path, sig_path);
  }

  for (size_t i = 0; i < arraysize(kVersionedFiles); ++i) {
    base::FilePath file_path = version_dir.Append(kVersionedFiles[i]);
    DVLOG(2) << __func__ << ": versioned file " << i << " at "
             << file_path.value();
    cdm_host_file_paths->emplace_back(file_path, GetSigFilePath(file_path));
  }

#elif defined(OS_MACOSX)

  base::FilePath chrome_framework_path =
      base::mac::FrameworkBundlePath().Append(chrome::kFrameworkExecutableName);

  // Framework signature is in the "Widevine Resources.bundle" next to the
  // framework directory, not next to the actual framework executable.
  static const base::FilePath::CharType kWidevineResourcesPath[] =
      FILE_PATH_LITERAL("Widevine Resources.bundle/Contents/Resources");
  base::FilePath widevine_signature_path =
      base::mac::FrameworkBundlePath().DirName().Append(kWidevineResourcesPath);
  base::FilePath chrome_framework_sig_path = GetSigFilePath(
      widevine_signature_path.Append(chrome::kFrameworkExecutableName));

  DVLOG(2) << __func__
           << ": chrome_framework_path=" << chrome_framework_path.value()
           << ", signature_path=" << chrome_framework_sig_path.value();
  cdm_host_file_paths->emplace_back(chrome_framework_path,
                                    chrome_framework_sig_path);

#elif defined(OS_LINUX)

  base::FilePath chrome_exe_dir;
  if (!base::PathService::Get(base::DIR_EXE, &chrome_exe_dir))
    NOTREACHED();

  base::FilePath chrome_path =
      chrome_exe_dir.Append(FILE_PATH_LITERAL("chrome"));
  DVLOG(2) << __func__ << ": chrome_path=" << chrome_path.value();
  cdm_host_file_paths->emplace_back(chrome_path, GetSigFilePath(chrome_path));

#endif  // defined(OS_WIN)
}

#else  // defined(GOOGLE_CHROME_BUILD)

void AddCdmHostFilePaths(
    std::vector<media::CdmHostFilePath>* cdm_host_file_paths) {
  NOTIMPLEMENTED() << "CDM host file paths need to be provided for the CDM to "
                      "verify the host.";
}

#endif  // defined(GOOGLE_CHROME_BUILD)

