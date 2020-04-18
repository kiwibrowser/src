// Copyright (c) 2006-2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/win/sandbox_poc/pocdll/exports.h"
#include "sandbox/win/sandbox_poc/pocdll/ntundoc.h"
#include "sandbox/win/sandbox_poc/pocdll/utils.h"

// This file contains the tests used to verify the security of handles in
// the process

NTQUERYOBJECT NtQueryObject;
NTQUERYINFORMATIONFILE NtQueryInformationFile;
NTQUERYSYSTEMINFORMATION NtQuerySystemInformation;

void POCDLL_API TestGetHandle(HANDLE log) {
  HandleToFile handle2file;
  FILE *output = handle2file.Translate(log, "w");

  // Initialize the NTAPI functions we need
  HMODULE ntdll_handle = ::GetModuleHandle(L"ntdll.dll");
  if (!ntdll_handle) {
    fprintf(output, "[ERROR] Cannot load ntdll.dll. Error %ld\r\n",
            ::GetLastError());
    return;
  }

  NtQueryObject = reinterpret_cast<NTQUERYOBJECT>(
                      GetProcAddress(ntdll_handle, "NtQueryObject"));
  NtQueryInformationFile = reinterpret_cast<NTQUERYINFORMATIONFILE>(
                      GetProcAddress(ntdll_handle, "NtQueryInformationFile"));
  NtQuerySystemInformation = reinterpret_cast<NTQUERYSYSTEMINFORMATION>(
                      GetProcAddress(ntdll_handle, "NtQuerySystemInformation"));

  if (!NtQueryObject || !NtQueryInformationFile || !NtQuerySystemInformation) {
    fprintf(output, "[ERROR] Cannot load all NT functions. Error %ld\r\n",
                    ::GetLastError());
    return;
  }

  // Get the number of handles on the system
  DWORD buffer_size = 0;
  SYSTEM_HANDLE_INFORMATION_EX temp_info;
  NTSTATUS status = NtQuerySystemInformation(
      SystemHandleInformation, &temp_info, sizeof(temp_info),
      &buffer_size);
  if (!buffer_size) {
    fprintf(output, "[ERROR] Get the number of handles. Error 0x%lX\r\n",
                    status);
    return;
  }

  SYSTEM_HANDLE_INFORMATION_EX *system_handles =
      reinterpret_cast<SYSTEM_HANDLE_INFORMATION_EX*>(new BYTE[buffer_size]);

  status = NtQuerySystemInformation(SystemHandleInformation, system_handles,
                                    buffer_size, &buffer_size);
  if (STATUS_SUCCESS != status) {
    fprintf(output, "[ERROR] Failed to get the handle list. Error 0x%lX\r\n",
                    status);
    delete [] system_handles;
    return;
  }

  for (ULONG i = 0; i < system_handles->NumberOfHandles; ++i) {
    USHORT h = system_handles->Information[i].Handle;
    if (system_handles->Information[i].ProcessId != ::GetCurrentProcessId())
      continue;

    OBJECT_NAME_INFORMATION *name = NULL;
    ULONG name_size = 0;
    // Query the name information a first time to get the size of the name.
    status = NtQueryObject(reinterpret_cast<HANDLE>(h),
                           ObjectNameInformation,
                           name,
                           name_size,
                           &name_size);

    if (name_size) {
      name = reinterpret_cast<OBJECT_NAME_INFORMATION *>(new BYTE[name_size]);

      // Query the name information a second time to get the name of the
      // object referenced by the handle.
      status = NtQueryObject(reinterpret_cast<HANDLE>(h),
                             ObjectNameInformation,
                             name,
                             name_size,
                             &name_size);
    }

    PUBLIC_OBJECT_TYPE_INFORMATION *type = NULL;
    ULONG type_size = 0;

    // Query the object to get the size of the object type name.
    status = NtQueryObject(reinterpret_cast<HANDLE>(h),
                           ObjectTypeInformation,
                           type,
                           type_size,
                           &type_size);
    if (type_size) {
      type = reinterpret_cast<PUBLIC_OBJECT_TYPE_INFORMATION *>(
          new BYTE[type_size]);

      // Query the type information a second time to get the object type
      // name.
      status = NtQueryObject(reinterpret_cast<HANDLE>(h),
                             ObjectTypeInformation,
                             type,
                             type_size,
                             &type_size);
    }

    // NtQueryObject cannot return the name for a file. In this case we
    // need to ask NtQueryInformationFile
    FILE_NAME_INFORMATION *file_name = NULL;
    if (type && wcsncmp(L"File", type->TypeName.Buffer,
                        (type->TypeName.Length /
                        sizeof(type->TypeName.Buffer[0]))) == 0)  {
      // This function does not return the size of the buffer. We need to
      // iterate and always increase the buffer size until the function
      // succeeds. (Or at least does not fail with STATUS_BUFFER_OVERFLOW)
      ULONG size_file = MAX_PATH;
      IO_STATUS_BLOCK status_block = {};
      do {
        // Delete the previous buffer create. The buffer was too small
        if (file_name) {
          delete[] reinterpret_cast<BYTE*>(file_name);
          file_name = NULL;
        }

        // Increase the buffer and do the call agan
        size_file += MAX_PATH;
        file_name = reinterpret_cast<FILE_NAME_INFORMATION *>(
            new BYTE[size_file]);
        status = NtQueryInformationFile(reinterpret_cast<HANDLE>(h),
                                        &status_block,
                                        file_name,
                                        size_file,
                                        FileNameInformation);
      } while (status == STATUS_BUFFER_OVERFLOW);

      if (STATUS_SUCCESS != status) {
        if (file_name) {
          delete[] file_name;
          file_name = NULL;
        }
      }
    }

    if (file_name) {
      UNICODE_STRING file_name_string;
      file_name_string.Buffer = file_name->FileName;
      file_name_string.Length = (USHORT)file_name->FileNameLength;
      file_name_string.MaximumLength = (USHORT)file_name->FileNameLength;
      fprintf(output, "[GRANTED] Handle 0x%4.4X Access: 0x%8.8lX "
                      "Type: %-13wZ Path: %wZ\r\n",
                      h,
                      system_handles->Information[i].GrantedAccess,
                      type ? &type->TypeName : NULL,
                      &file_name_string);
    } else {
      fprintf(output, "[GRANTED] Handle 0x%4.4X Access: 0x%8.8lX "
                      "Type: %-13wZ Path: %wZ\r\n",
                      h,
                      system_handles->Information[i].GrantedAccess,
                      type ? &type->TypeName : NULL,
                      name ? &name->ObjectName : NULL);
    }

    if (type) {
      delete[] type;
    }

    if (file_name) {
      delete[] file_name;
    }

    if (name) {
      delete [] name;
    }
  }

  if (system_handles) {
    delete [] system_handles;
  }
}
