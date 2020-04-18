// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profile_resetter/brandcode_config_fetcher.h"

#include <stddef.h>
#include <vector>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profile_resetter/brandcoded_default_settings.h"
#include "libxml/parser.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace {

const int kDownloadTimeoutSec = 10;
const char kPostXml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<request"
    "    version=\"chromeprofilereset-1.1\""
    "    protocol=\"3.0\""
    "    installsource=\"profilereset\">"
    "  <app appid=\"{8A69D345-D564-463C-AFF1-A69D9E530F96}\">"
    "    <data name=\"install\" index=\"__BRANDCODE_PLACEHOLDER__\"/>"
    "  </app>"
    "</request>";

// Returns the query to the server which can be used to retrieve the config.
// |brand| is a brand code, it mustn't be empty.
std::string GetUploadData(const std::string& brand) {
  DCHECK(!brand.empty());
  std::string data(kPostXml);
  const std::string placeholder("__BRANDCODE_PLACEHOLDER__");
  size_t placeholder_pos = data.find(placeholder);
  DCHECK(placeholder_pos != std::string::npos);
  data.replace(placeholder_pos, placeholder.size(), brand);
  return data;
}

// Extracts json master prefs from xml.
class XmlConfigParser {
 public:
  XmlConfigParser();
  ~XmlConfigParser();

  // Returns the content of /response/app/data tag.
  static void Parse(const std::string& input_buffer,
                    std::string* output_buffer);

 private:
  static XmlConfigParser* FromContext(void* ctx);
  static std::string XMLCharToString(const xmlChar* value);
  static void StartElementImpl(void* ctx,
                               const xmlChar* name,
                               const xmlChar** atts);
  static void EndElementImpl(void* ctx, const xmlChar* name);
  static void CharactersImpl(void* ctx, const xmlChar* ch, int len);

  bool IsParsingData() const;

  // Extracted json file.
  std::string master_prefs_;

  // Current stack of the elements being parsed.
  std::vector<std::string> elements_;

  DISALLOW_COPY_AND_ASSIGN(XmlConfigParser);
};

XmlConfigParser::XmlConfigParser() {}

XmlConfigParser::~XmlConfigParser() {}

void XmlConfigParser::Parse(const std::string& input_buffer,
                            std::string* output_buffer) {
  using logging::LOG_WARNING;

  DCHECK(output_buffer);
  xmlSAXHandler sax_handler = {};
  sax_handler.startElement = &XmlConfigParser::StartElementImpl;
  sax_handler.endElement = &XmlConfigParser::EndElementImpl;
  sax_handler.characters = &XmlConfigParser::CharactersImpl;
  XmlConfigParser parser;
  int error = xmlSAXUserParseMemory(&sax_handler,
                                    &parser,
                                    input_buffer.c_str(),
                                    input_buffer.size());
  if (error) {
    VLOG(LOG_WARNING) << "Error parsing brandcoded master prefs, err=" << error;
  } else {
    output_buffer->swap(parser.master_prefs_);
  }
}

XmlConfigParser* XmlConfigParser::FromContext(void* ctx) {
  return static_cast<XmlConfigParser*>(ctx);
}

std::string XmlConfigParser::XMLCharToString(const xmlChar* value) {
  return std::string(reinterpret_cast<const char*>(value));
}

void XmlConfigParser::StartElementImpl(void* ctx,
                                       const xmlChar* name,
                                       const xmlChar** atts) {
  std::string node_name(XMLCharToString(name));
  XmlConfigParser* context = FromContext(ctx);
  context->elements_.push_back(node_name);
  if (context->IsParsingData())
    context->master_prefs_.clear();
}

void XmlConfigParser::EndElementImpl(void* ctx, const xmlChar* name) {
  XmlConfigParser* context = FromContext(ctx);
  context->elements_.pop_back();
}

void XmlConfigParser::CharactersImpl(void* ctx, const xmlChar* ch, int len) {
  XmlConfigParser* context = FromContext(ctx);
  if (context->IsParsingData()) {
    context->master_prefs_ +=
        std::string(reinterpret_cast<const char*>(ch), len);
  }
}

bool XmlConfigParser::IsParsingData() const {
  const std::string data_path[] = {"response", "app", "data"};
  return elements_.size() == arraysize(data_path) &&
         std::equal(elements_.begin(), elements_.end(), data_path);
}

}  // namespace

BrandcodeConfigFetcher::BrandcodeConfigFetcher(
    network::mojom::URLLoaderFactory* url_loader_factory,
    const FetchCallback& callback,
    const GURL& url,
    const std::string& brandcode)
    : fetch_callback_(callback) {
  DCHECK(!brandcode.empty());
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brandcode_config", R"(
        semantics {
          sender: "Brandcode Configuration Fetcher"
          description:
            "Chrome installation can be non-organic. That means that Chrome "
            "is distributed by partners and it has a brand code associated "
            "with that partner. For the settings reset operation, Chrome needs "
            "to know the default settings which are partner specific."
          trigger: "'Reset Settings' invocation from Chrome settings."
          data: "Brandcode."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled and is only invoked by user "
            "request."
          policy_exception_justification:
            "Not implemented, considered not useful as enterprises don't need "
            "to install Chrome in a non-organic fashion."
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->load_flags = net::LOAD_DO_NOT_SEND_COOKIES |
                                 net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_DISABLE_CACHE;
  resource_request->method = "POST";
  resource_request->headers.SetHeader("Accept", "text/xml");
  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->AttachStringForUpload(GetUploadData(brandcode),
                                            "text/xml");
  simple_url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
      base::BindOnce(&BrandcodeConfigFetcher::OnSimpleLoaderComplete,
                     base::Unretained(this)));
  // Abort the download attempt if it takes too long.
  download_timer_.Start(FROM_HERE,
                        base::TimeDelta::FromSeconds(kDownloadTimeoutSec),
                        this,
                        &BrandcodeConfigFetcher::OnDownloadTimeout);
}

BrandcodeConfigFetcher::~BrandcodeConfigFetcher() {}

void BrandcodeConfigFetcher::SetCallback(const FetchCallback& callback) {
  fetch_callback_ = callback;
}

void BrandcodeConfigFetcher::OnSimpleLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  if (response_body && simple_url_loader_->ResponseInfo() &&
      simple_url_loader_->ResponseInfo()->mime_type == "text/xml") {
    std::string master_prefs;
    XmlConfigParser::Parse(*response_body, &master_prefs);
    default_settings_.reset(new BrandcodedDefaultSettings(master_prefs));
  }
  simple_url_loader_.reset();
  download_timer_.Stop();
  base::ResetAndReturn(&fetch_callback_).Run();
}

void BrandcodeConfigFetcher::OnDownloadTimeout() {
  if (simple_url_loader_) {
    simple_url_loader_.reset();
    base::ResetAndReturn(&fetch_callback_).Run();
  }
}
