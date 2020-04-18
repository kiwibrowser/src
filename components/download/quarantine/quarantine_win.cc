// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/quarantine/quarantine.h"

#include <windows.h>
#include <wrl/client.h>

#include <cguid.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wininet.h>

#include <vector>

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/scoped_handle.h"
#include "url/gurl.h"

namespace download {
namespace {

// [MS-FSCC] Section 5.6.1
const base::FilePath::CharType kZoneIdentifierStreamSuffix[] =
    FILE_PATH_LITERAL(":Zone.Identifier");

// UMA enumeration for recording Download.AttachmentServicesResult.
enum class AttachmentServicesResult : int {
  SUCCESS_WITH_MOTW = 0,
  SUCCESS_WITHOUT_MOTW = 1,
  SUCCESS_WITHOUT_FILE = 2,
  NO_ATTACHMENT_SERVICES = 3,
  FAILED_TO_SET_PARAMETER = 4,
  BLOCKED_WITH_FILE = 5,
  BLOCKED_WITHOUT_FILE = 6,
  INFECTED_WITH_FILE = 7,
  INFECTED_WITHOUT_FILE = 8,
  ACCESS_DENIED_WITH_FILE = 9,
  ACCESS_DENIED_WITHOUT_FILE = 10,
  OTHER_WITH_FILE = 11,
  OTHER_WITHOUT_FILE = 12,
};

void RecordAttachmentServicesResult(AttachmentServicesResult type) {
  base::UmaHistogramSparse("Download.AttachmentServices.Result",
                           static_cast<int>(type));
}

bool ZoneIdentifierPresentForFile(const base::FilePath& path) {
  const DWORD kShare = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  base::FilePath::StringType zone_identifier_path =
      path.value() + kZoneIdentifierStreamSuffix;
  base::win::ScopedHandle file(
      CreateFile(zone_identifier_path.c_str(), GENERIC_READ, kShare, nullptr,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (!file.IsValid())
    return false;

  // The zone identifier contents is expected to be:
  // "[ZoneTransfer]\r\nZoneId=3\r\n". The actual ZoneId can be different. A
  // buffer of 32 bytes is sufficient for verifying the contents.
  std::vector<char> zone_identifier_contents_buffer(32);
  DWORD actual_length = 0;
  if (!ReadFile(file.Get(), &zone_identifier_contents_buffer.front(),
                zone_identifier_contents_buffer.size(), &actual_length,
                nullptr))
    return false;
  zone_identifier_contents_buffer.resize(actual_length);

  std::string zone_identifier_contents(zone_identifier_contents_buffer.begin(),
                                       zone_identifier_contents_buffer.end());

  std::vector<base::StringPiece> lines =
      base::SplitStringPiece(zone_identifier_contents, "\n",
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return lines.size() > 1 && lines[0] == "[ZoneTransfer]" &&
         lines[1].find("ZoneId=") == 0;
}

void RecordAttachmentServicesSaveResult(const base::FilePath& file,
                                        HRESULT hr) {
  bool file_exists = base::PathExists(file);
  switch (hr) {
    case INET_E_SECURITY_PROBLEM:
      RecordAttachmentServicesResult(
          file_exists ? AttachmentServicesResult::BLOCKED_WITH_FILE
                      : AttachmentServicesResult::BLOCKED_WITHOUT_FILE);
      break;

    case E_FAIL:
      RecordAttachmentServicesResult(
          file_exists ? AttachmentServicesResult::INFECTED_WITH_FILE
                      : AttachmentServicesResult::INFECTED_WITHOUT_FILE);
      break;

    case E_ACCESSDENIED:
    case ERROR_ACCESS_DENIED:
      // ERROR_ACCESS_DENIED is not a valid HRESULT. However,
      // IAttachmentExecute::Save() is known to return it and other system error
      // codes in practice.
      RecordAttachmentServicesResult(
          file_exists ? AttachmentServicesResult::ACCESS_DENIED_WITH_FILE
                      : AttachmentServicesResult::ACCESS_DENIED_WITHOUT_FILE);
      break;

    default:
      if (SUCCEEDED(hr)) {
        bool motw_exists = file_exists && ZoneIdentifierPresentForFile(file);
        RecordAttachmentServicesResult(
            file_exists ? motw_exists
                              ? AttachmentServicesResult::SUCCESS_WITH_MOTW
                              : AttachmentServicesResult::SUCCESS_WITHOUT_MOTW
                        : AttachmentServicesResult::SUCCESS_WITHOUT_FILE);
        return;
      }

      // Failure codes.
      RecordAttachmentServicesResult(
          file_exists ? AttachmentServicesResult::OTHER_WITH_FILE
                      : AttachmentServicesResult::OTHER_WITHOUT_FILE);
  }
}

// Sets the Zone Identifier on the file to "Internet" (3). Returns true if the
// function succeeds, false otherwise. A failure is expected if alternate
// streams are not supported, like a file on a FAT32 filesystem.  This function
// does not invoke Windows Attachment Execution Services.
//
// |full_path| is the path to the downloaded file.
QuarantineFileResult SetInternetZoneIdentifierDirectly(
    const base::FilePath& full_path) {
  const DWORD kShare = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  std::wstring path = full_path.value() + kZoneIdentifierStreamSuffix;
  HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, kShare, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (INVALID_HANDLE_VALUE == file)
    return QuarantineFileResult::ANNOTATION_FAILED;

  static const char kIdentifier[] = "[ZoneTransfer]\r\nZoneId=3\r\n";
  // Don't include trailing null in data written.
  static const DWORD kIdentifierSize = arraysize(kIdentifier) - 1;
  DWORD written = 0;
  BOOL write_result =
      WriteFile(file, kIdentifier, kIdentifierSize, &written, nullptr);
  BOOL flush_result = FlushFileBuffers(file);
  CloseHandle(file);

  return write_result && flush_result && written == kIdentifierSize
             ? QuarantineFileResult::OK
             : QuarantineFileResult::ANNOTATION_FAILED;
}

// Invokes IAttachmentExecute::Save on CLSID_AttachmentServices to validate the
// downloaded file. The call may scan the file for viruses and if necessary,
// annotate it with evidence.  As a result of the validation, the file may be
// deleted. See: http://msdn.microsoft.com/en-us/bb776299
//
// IAE::Save() will delete the file if it was found to be blocked by local
// security policy or if it was found to be infected. The call may also delete
// the file due to other failures (http://crbug.com/153212). A failure code will
// be returned in these cases.
//
// The return value is |false| iff the function fails to invoke
// IAttachmentExecute::Save(). If the function returns |true|, then the result
// of invoking IAttachmentExecute::Save() is stored in |save_result|.
//
// Typical |save_result| values:
//   S_OK   : The file was okay. If any viruses were found, they were cleaned.
//   E_FAIL : Virus infected.
//   INET_E_SECURITY_PROBLEM : The file was blocked due to security policy.
//
// Any other return value indicates an unexpected error during the scan.
//
// |full_path| : is the path to the downloaded file. This should be the final
//               path of the download. Must be present.
// |source_url|: the source URL for the download. If empty, the source will
//               be set to 'about:internet'.
// |referrer_url|: the referrer URL for the download. If empty, the referrer
//               will not be set.
// |client_guid|: the GUID to be set in the IAttachmentExecute client slot.
//                Used to identify the app to the system AV function.
// |save_result|: Receives the result of invoking IAttachmentExecute::Save().
bool InvokeAttachmentServices(const base::FilePath& full_path,
                              const std::string& source_url,
                              const std::string& referrer_url,
                              const GUID& client_guid,
                              HRESULT* save_result) {
  Microsoft::WRL::ComPtr<IAttachmentExecute> attachment_services;
  HRESULT hr = ::CoCreateInstance(CLSID_AttachmentServices, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&attachment_services));
  *save_result = S_OK;

  if (FAILED(hr)) {
    // The thread must have COM initialized.
    DCHECK_NE(CO_E_NOTINITIALIZED, hr);
    RecordAttachmentServicesResult(
        AttachmentServicesResult::NO_ATTACHMENT_SERVICES);
    return false;
  }

  // Note that it is mandatory to check the return values from here on out. If
  // setting one of the parameters fails, it could leave the object in a state
  // where the final Save() call will also fail.

  hr = attachment_services->SetClientGuid(client_guid);
  if (FAILED(hr)) {
    RecordAttachmentServicesResult(
        AttachmentServicesResult::FAILED_TO_SET_PARAMETER);
    return false;
  }

  hr = attachment_services->SetLocalPath(full_path.value().c_str());
  if (FAILED(hr)) {
    RecordAttachmentServicesResult(
        AttachmentServicesResult::FAILED_TO_SET_PARAMETER);
    return false;
  }

  // The source URL could be empty if it was not a valid URL, or was not HTTP/S,
  // or the download was off-the-record. If so, user "about:internet" as a
  // fallback URL. The latter is known to reliably map to the Internet zone.
  //
  // In addition, URLs that are longer than INTERNET_MAX_URL_LENGTH are also
  // known to cause problems for URLMon. Hence also use "about:internet" in
  // these cases. See http://crbug.com/601538.
  hr = attachment_services->SetSource(
      source_url.empty() || source_url.size() > INTERNET_MAX_URL_LENGTH
          ? L"about:internet"
          : base::UTF8ToWide(source_url).c_str());
  if (FAILED(hr)) {
    RecordAttachmentServicesResult(
        AttachmentServicesResult::FAILED_TO_SET_PARAMETER);
    return false;
  }

  // Only set referrer if one is present and shorter than
  // INTERNET_MAX_URL_LENGTH. Also, the source_url is authoritative for
  // determining the relative danger of |full_path| so we don't consider it an
  // error if we have to skip the |referrer_url|.
  if (!referrer_url.empty() && referrer_url.size() < INTERNET_MAX_URL_LENGTH) {
    hr = attachment_services->SetReferrer(
        base::UTF8ToWide(referrer_url).c_str());
    if (FAILED(hr)) {
      RecordAttachmentServicesResult(
          AttachmentServicesResult::FAILED_TO_SET_PARAMETER);
      return false;
    }
  }

  {
    // This method has been known to take longer than 10 seconds in some
    // instances.
    SCOPED_UMA_HISTOGRAM_LONG_TIMER("Download.AttachmentServices.Duration");
    *save_result = attachment_services->Save();
  }
  RecordAttachmentServicesSaveResult(full_path, *save_result);
  return true;
}

// Maps a return code from an unsuccessful IAttachmentExecute::Save() call to a
// QuarantineFileResult.
QuarantineFileResult FailedSaveResultToQuarantineResult(HRESULT result) {
  switch (result) {
    case INET_E_SECURITY_PROBLEM:  // 0x800c000e
      // This is returned if the download was blocked due to security
      // restrictions. E.g. if the source URL was in the Restricted Sites zone
      // and downloads are blocked on that zone, then the download would be
      // deleted and this error code is returned.
      return QuarantineFileResult::BLOCKED_BY_POLICY;

    case E_FAIL:  // 0x80004005
      // Returned if an anti-virus product reports an infection in the
      // downloaded file during IAE::Save().
      return QuarantineFileResult::VIRUS_INFECTED;

    default:
      // Any other error that occurs during IAttachmentExecute::Save() likely
      // indicates a problem with the security check, but not necessarily the
      // download. This also includes cases where SUCCEEDED(result) is true. In
      // the latter case we are likely dealing with a situation where the file
      // is missing after a successful scan. See http://crbug.com/153212.
      return QuarantineFileResult::SECURITY_CHECK_FAILED;
  }
}

}  // namespace

QuarantineFileResult QuarantineFile(const base::FilePath& file,
                                    const GURL& source_url,
                                    const GURL& referrer_url,
                                    const std::string& client_guid) {
  base::AssertBlockingAllowed();

  int64_t file_size = 0;
  if (!base::PathExists(file) || !base::GetFileSize(file, &file_size))
    return QuarantineFileResult::FILE_MISSING;

  std::string braces_guid = "{" + client_guid + "}";
  GUID guid = GUID_NULL;
  if (base::IsValidGUID(client_guid)) {
    HRESULT hr = CLSIDFromString(base::UTF8ToUTF16(braces_guid).c_str(), &guid);
    if (FAILED(hr))
      guid = GUID_NULL;
  }

  if (file_size == 0 || IsEqualGUID(guid, GUID_NULL)) {
    // Calling InvokeAttachmentServices on an empty file can result in the file
    // being deleted.  Also an anti-virus scan doesn't make a lot of sense to
    // perform on an empty file.
    return SetInternetZoneIdentifierDirectly(file);
  }

  HRESULT save_result = S_OK;
  bool attachment_services_available = InvokeAttachmentServices(
      file, source_url.spec(), referrer_url.spec(), guid, &save_result);
  if (!attachment_services_available)
    return SetInternetZoneIdentifierDirectly(file);

  // If the download file is missing after the call, then treat this as an
  // interrupted download.
  //
  // If InvokeAttachmentServices() failed, but the downloaded file is still
  // around, then don't interrupt the download. Attachment Execution Services
  // deletes the submitted file if the downloaded file is blocked by policy or
  // if it was found to be infected.
  //
  // If the file is still there, then the error could be due to Windows
  // Attachment Services not being available or some other error during the AES
  // invocation. In either case, we don't surface the error to the user.
  if (!base::PathExists(file))
    return FailedSaveResultToQuarantineResult(save_result);
  return QuarantineFileResult::OK;
}

bool IsFileQuarantined(const base::FilePath& file,
                       const GURL& source_url,
                       const GURL& referrer_url) {
  return ZoneIdentifierPresentForFile(file);
}

}  // namespace download
