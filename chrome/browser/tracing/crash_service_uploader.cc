// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tracing/crash_service_uploader.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/tracing/common/tracing_switches.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/base/network_delegate.h"
#include "net/http/http_status_code.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/zlib/zlib.h"
#include "url/gurl.h"

using std::string;

namespace {

const char kUploadURL[] = "https://crash2.browser.mises.site/cr/report";
const char kCrashUploadContentType[] = "multipart/form-data";
const char kCrashMultipartBoundary[] =
    "----**--yradnuoBgoLtrapitluMklaTelgooG--**----";

// Allow up to 10MB for trace upload
const size_t kMaxUploadBytes = 10000000;

}  // namespace

TraceCrashServiceUploader::TraceCrashServiceUploader(
    net::URLRequestContextGetter* request_context)
    : request_context_(request_context), max_upload_bytes_(kMaxUploadBytes) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string upload_url = kUploadURL;
  if (command_line.HasSwitch(switches::kTraceUploadURL)) {
    upload_url = command_line.GetSwitchValueASCII(switches::kTraceUploadURL);
  }
  SetUploadURL(upload_url);
}

TraceCrashServiceUploader::~TraceCrashServiceUploader() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void TraceCrashServiceUploader::SetUploadURL(const std::string& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  upload_url_ = url;

  if (!GURL(upload_url_).is_valid())
    upload_url_.clear();
}

void TraceCrashServiceUploader::SetMaxUploadBytes(size_t max_upload_bytes) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  max_upload_bytes_ = max_upload_bytes;
}

void TraceCrashServiceUploader::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_EQ(source, url_fetcher_.get());
  int response_code = source->GetResponseCode();
  string feedback;
  bool success = (response_code == net::HTTP_OK);
  if (success) {
    source->GetResponseAsString(&feedback);
  } else {
    feedback =
        "Uploading failed, response code: " + base::IntToString(response_code);
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(done_callback_), success, feedback));
  url_fetcher_.reset();
}

void TraceCrashServiceUploader::OnURLFetchUploadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total) {
  DCHECK(url_fetcher_.get());
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LOG(WARNING) << "Upload progress: " << current << " of " << total;

  if (progress_callback_.is_null())
    return;
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(progress_callback_, current, total));
}

void TraceCrashServiceUploader::DoUpload(
    const std::string& file_contents,
    UploadMode upload_mode,
    std::unique_ptr<const base::DictionaryValue> metadata,
    const UploadProgressCallback& progress_callback,
    UploadDoneCallback done_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  progress_callback_ = progress_callback;
  done_callback_ = std::move(done_callback);

  base::PostTaskWithTraits(
      FROM_HERE, {base::TaskPriority::BACKGROUND},
      base::Bind(&TraceCrashServiceUploader::DoCompressOnBackgroundThread,
                 base::Unretained(this), file_contents, upload_mode,
                 upload_url_, base::Passed(std::move(metadata))));
}

void TraceCrashServiceUploader::DoCompressOnBackgroundThread(
    const std::string& file_contents,
    UploadMode upload_mode,
    const std::string& upload_url,
    std::unique_ptr<const base::DictionaryValue> metadata) {
  DCHECK(!url_fetcher_.get());

  if (upload_url.empty()) {
    OnUploadError("Upload URL empty or invalid");
    return;
  }

#if defined(OS_WIN)
  const char product[] = "Chrome";
#elif defined(OS_MACOSX)
  const char product[] = "Chrome_Mac";
#elif defined(OS_CHROMEOS)
  // On ChromeOS, defined(OS_LINUX) also evalutes to true, so the
  // defined(OS_CHROMEOS) block must come first.
  const char product[] = "Chrome_ChromeOS";
#elif defined(OS_LINUX)
  const char product[] = "Chrome_Linux";
#elif defined(OS_ANDROID)
  const char product[] = "Chrome_Android";
#else
#error Platform not supported.
#endif

  // version_info::GetProductNameAndVersionForUserAgent() returns a string like
  // "Chrome/aa.bb.cc.dd", split out the part before the "/".
  std::vector<std::string> product_components = base::SplitString(
      version_info::GetProductNameAndVersionForUserAgent(), "/",
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  DCHECK_EQ(2U, product_components.size());
  std::string version;
  if (product_components.size() == 2U) {
    version = product_components[1];
  } else {
    version = "unknown";
  }

  if (url_fetcher_.get()) {
    OnUploadError("Already uploading.");
    return;
  }

  std::string compressed_contents;
  if (upload_mode == COMPRESSED_UPLOAD) {
    std::unique_ptr<char[]> compressed_buffer(new char[max_upload_bytes_]);
    int compressed_bytes;
    if (!Compress(file_contents, max_upload_bytes_, compressed_buffer.get(),
                  &compressed_bytes)) {
      OnUploadError("Compressing file failed.");
      return;
    }
    compressed_contents =
        std::string(compressed_buffer.get(), compressed_bytes);
  } else {
    compressed_contents = file_contents;
  }
  if (compressed_contents.size() > max_upload_bytes_) {
    OnUploadError("File is too large to upload.");
    return;
  }

  std::string post_data;
  SetupMultipart(product, version, std::move(metadata), "trace.json.gz",
                 compressed_contents, &post_data);

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&TraceCrashServiceUploader::CreateAndStartURLFetcher,
                 base::Unretained(this), upload_url, post_data));
}

void TraceCrashServiceUploader::OnUploadError(
    const std::string& error_message) {
  LOG(ERROR) << error_message;
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(done_callback_), false, error_message));
}

void TraceCrashServiceUploader::SetupMultipart(
    const std::string& product,
    const std::string& version,
    std::unique_ptr<const base::DictionaryValue> metadata,
    const std::string& trace_filename,
    const std::string& trace_contents,
    std::string* post_data) {
  net::AddMultipartValueForUpload("prod", product, kCrashMultipartBoundary, "",
                                  post_data);
  net::AddMultipartValueForUpload("ver", version + "-trace",
                                  kCrashMultipartBoundary, "", post_data);
  net::AddMultipartValueForUpload("guid", "0", kCrashMultipartBoundary, "",
                                  post_data);
  net::AddMultipartValueForUpload("type", "trace", kCrashMultipartBoundary, "",
                                  post_data);
  // No minidump means no need for crash to process the report.
  net::AddMultipartValueForUpload("should_process", "false",
                                  kCrashMultipartBoundary, "", post_data);
  if (metadata) {
    for (base::DictionaryValue::Iterator it(*metadata); !it.IsAtEnd();
         it.Advance()) {
      std::string value;
      if (!it.value().GetAsString(&value)) {
        if (!base::JSONWriter::Write(it.value(), &value))
          continue;
      }

      net::AddMultipartValueForUpload(it.key(), value, kCrashMultipartBoundary,
                                      "", post_data);
    }
  }

  AddTraceFile(trace_filename, trace_contents, post_data);

  net::AddMultipartFinalDelimiterForUpload(kCrashMultipartBoundary, post_data);
}

void TraceCrashServiceUploader::AddTraceFile(const std::string& trace_filename,
                                             const std::string& trace_contents,
                                             std::string* post_data) {
  post_data->append("--");
  post_data->append(kCrashMultipartBoundary);
  post_data->append("\r\n");
  post_data->append("Content-Disposition: form-data; name=\"trace\"");
  post_data->append("; filename=\"");
  post_data->append(trace_filename);
  post_data->append("\"\r\n");
  post_data->append("Content-Type: application/gzip\r\n\r\n");
  post_data->append(trace_contents);
  post_data->append("\r\n");
}

bool TraceCrashServiceUploader::Compress(std::string input,
                                         int max_compressed_bytes,
                                         char* compressed,
                                         int* compressed_bytes) {
  DCHECK(compressed);
  DCHECK(compressed_bytes);
  z_stream stream = {0};
  int result = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                            // 16 is added to produce a gzip header + trailer.
                            MAX_WBITS + 16,
                            8,  // memLevel = 8 is default.
                            Z_DEFAULT_STRATEGY);
  DCHECK_EQ(Z_OK, result);
  stream.next_in = reinterpret_cast<uint8_t*>(&input[0]);
  stream.avail_in = input.size();
  stream.next_out = reinterpret_cast<uint8_t*>(compressed);
  stream.avail_out = max_compressed_bytes;
  // Do a one-shot compression. This will return Z_STREAM_END only if |output|
  // is large enough to hold all compressed data.
  result = deflate(&stream, Z_FINISH);
  bool success = (result == Z_STREAM_END);
  result = deflateEnd(&stream);
  DCHECK(result == Z_OK || result == Z_DATA_ERROR);

  if (success)
    *compressed_bytes = max_compressed_bytes - stream.avail_out;

  LOG(WARNING) << "input size: " << input.size()
               << ", output size: " << *compressed_bytes;
  return success;
}

void TraceCrashServiceUploader::CreateAndStartURLFetcher(
    const std::string& upload_url,
    const std::string& post_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!url_fetcher_.get());

  std::string content_type = kCrashUploadContentType;
  content_type.append("; boundary=");
  content_type.append(kCrashMultipartBoundary);

  // Create traffic annotation tag.
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("background_performance_tracer", R"(
        semantics {
          sender: "Background Performance Traces"
          description:
            "Under certain conditions, Chromium will send anonymized "
            "performance timeline data to Google for the purposes of improving "
            "Chromium performance. We can set up a percentage of the "
            "population to send back trace reports when a certain UMA "
            "histogram bucket is incremented, for example, 'For 1% of the Beta "
            "population, send us a trace if it ever takes more than 1 seconds "
            "for the Omnibox to respond to a typed character'. The possible "
            "types of triggers right now are UMA histograms, and manually "
            "triggered events from code (think of them like asserts, that'll "
            "cause a report to be sent if enabled for that population)."
          trigger:
            "Google-controlled triggering conditions, usually when a bad "
            "performance situation occurs."
          data: "An anonymized Chromium trace (see about://tracing)."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "You can enable or disable this feature via 'Automatically send "
            "usage statistics and crash reports to Google' in Chromium's "
            "settings under Advanced, Privacy. This feature is enabled by "
            "default."
          chrome_policy {
            MetricsReportingEnabled {
              policy_options {mode: MANDATORY}
              MetricsReportingEnabled: false
            }
          }
        })");

  url_fetcher_ = net::URLFetcher::Create(
      GURL(upload_url), net::URLFetcher::POST, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher_.get(),
      data_use_measurement::DataUseUserData::TRACING_UPLOADER);
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES);
  url_fetcher_->SetRequestContext(request_context_);
  url_fetcher_->SetUploadData(content_type, post_data);
  url_fetcher_->Start();
}
