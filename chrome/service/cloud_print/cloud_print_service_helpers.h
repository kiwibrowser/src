// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_SERVICE_HELPERS_H_
#define CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_SERVICE_HELPERS_H_

#include <string>
#include <vector>

#include "chrome/service/cloud_print/print_system.h"
#include "url/gurl.h"

namespace cloud_print {

// Helper methods for the cloud print proxy code.
GURL GetUrlForJobStatusUpdate(const GURL& cloud_print_server_url,
                              const std::string& job_id,
                              PrintJobStatus status,
                              int connector_code);

GURL GetUrlForJobStatusUpdate(const GURL& cloud_print_server_url,
                              const std::string& job_id,
                              const PrintJobDetails& details);

// Returns an MD5 hash for printer tags in the given |printer_info|.
std::string GetHashOfPrinterInfo(
    const printing::PrinterBasicInfo& printer_info);

// Returns any post data for printer tags in the given |printer_info|.
std::string GetPostDataForPrinterInfo(
    const printing::PrinterBasicInfo& printer_info,
    const std::string& mime_boundary);

// Returns true if tags indicate a dry run (test) job.
bool IsDryRunJob(const std::vector<std::string>& tags);

// Created cloud print auth header from the auth token stored in the store.
std::string GetCloudPrintAuthHeaderFromStore();

}  // namespace cloud_print

#endif  // CHROME_SERVICE_CLOUD_PRINT_CLOUD_PRINT_SERVICE_HELPERS_H_
