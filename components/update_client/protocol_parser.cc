// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/protocol_parser.h"

#include <stddef.h>

#include <algorithm>
#include <memory>

#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/version.h"
#include "libxml/tree.h"
#include "third_party/libxml/chromium/libxml_utils.h"

namespace update_client {

const char ProtocolParser::Result::kCohort[] = "cohort";
const char ProtocolParser::Result::kCohortHint[] = "cohorthint";
const char ProtocolParser::Result::kCohortName[] = "cohortname";

ProtocolParser::ProtocolParser() = default;
ProtocolParser::~ProtocolParser() = default;

ProtocolParser::Results::Results() = default;
ProtocolParser::Results::Results(const Results& other) = default;
ProtocolParser::Results::~Results() = default;

ProtocolParser::Result::Result() = default;
ProtocolParser::Result::Result(const Result& other) = default;
ProtocolParser::Result::~Result() = default;

ProtocolParser::Result::Manifest::Manifest() = default;
ProtocolParser::Result::Manifest::Manifest(const Manifest& other) = default;
ProtocolParser::Result::Manifest::~Manifest() = default;

ProtocolParser::Result::Manifest::Package::Package() = default;
ProtocolParser::Result::Manifest::Package::Package(const Package& other) =
    default;
ProtocolParser::Result::Manifest::Package::~Package() = default;

void ProtocolParser::ParseError(const char* details, ...) {
  va_list args;
  va_start(args, details);

  if (!errors_.empty()) {
    errors_ += "\r\n";
  }

  base::StringAppendV(&errors_, details, args);
  va_end(args);
}

// Checks whether a given node's name matches |expected_name|.
static bool TagNameEquals(const xmlNode* node, const char* expected_name) {
  return 0 == strcmp(expected_name, reinterpret_cast<const char*>(node->name));
}

// Returns child nodes of |root| with name |name|.
static std::vector<xmlNode*> GetChildren(xmlNode* root, const char* name) {
  std::vector<xmlNode*> result;
  for (xmlNode* child = root->children; child != nullptr; child = child->next) {
    if (!TagNameEquals(child, name)) {
      continue;
    }
    result.push_back(child);
  }
  return result;
}

// Returns the value of a named attribute, or the empty string.
static std::string GetAttribute(xmlNode* node, const char* attribute_name) {
  const xmlChar* name = reinterpret_cast<const xmlChar*>(attribute_name);
  for (xmlAttr* attr = node->properties; attr != nullptr; attr = attr->next) {
    if (!xmlStrcmp(attr->name, name) && attr->children &&
        attr->children->content) {
      return std::string(
          reinterpret_cast<const char*>(attr->children->content));
    }
  }
  return std::string();
}

// Returns the value of a named attribute, or nullptr .
static std::unique_ptr<std::string> GetAttributePtr(
    xmlNode* node,
    const char* attribute_name) {
  const xmlChar* name = reinterpret_cast<const xmlChar*>(attribute_name);
  for (xmlAttr* attr = node->properties; attr != nullptr; attr = attr->next) {
    if (!xmlStrcmp(attr->name, name) && attr->children &&
        attr->children->content) {
      return std::make_unique<std::string>(
          reinterpret_cast<const char*>(attr->children->content));
    }
  }
  return nullptr;
}

// This is used for the xml parser to report errors. This assumes the context
// is a pointer to a std::string where the error message should be appended.
static void XmlErrorFunc(void* context, const char* message, ...) {
  va_list args;
  va_start(args, message);
  std::string* error = static_cast<std::string*>(context);
  base::StringAppendV(error, message, args);
  va_end(args);
}

// Utility class for cleaning up the xml document when leaving a scope.
class ScopedXmlDocument {
 public:
  explicit ScopedXmlDocument(xmlDocPtr document) : document_(document) {}
  ~ScopedXmlDocument() {
    if (document_)
      xmlFreeDoc(document_);
  }

  xmlDocPtr get() { return document_; }

 private:
  xmlDocPtr document_;
};

// Parses the <package> tag.
bool ParsePackageTag(xmlNode* package,
                     ProtocolParser::Result* result,
                     std::string* error) {
  ProtocolParser::Result::Manifest::Package p;
  p.name = GetAttribute(package, "name");
  if (p.name.empty()) {
    *error = "Missing name for package.";
    return false;
  }

  p.namediff = GetAttribute(package, "namediff");

  // package_fingerprint is optional. It identifies the package, preferably
  // with a modified sha256 hash of the package in hex format.
  p.fingerprint = GetAttribute(package, "fp");

  p.hash_sha256 = GetAttribute(package, "hash_sha256");
  int size = 0;
  if (base::StringToInt(GetAttribute(package, "size"), &size)) {
    p.size = size;
  }

  p.hashdiff_sha256 = GetAttribute(package, "hashdiff_sha256");
  int sizediff = 0;
  if (base::StringToInt(GetAttribute(package, "sizediff"), &sizediff)) {
    p.sizediff = sizediff;
  }

  result->manifest.packages.push_back(p);

  return true;
}

// Parses the <manifest> tag.
bool ParseManifestTag(xmlNode* manifest,
                      ProtocolParser::Result* result,
                      std::string* error) {
  // Get the version.
  result->manifest.version = GetAttribute(manifest, "version");
  if (result->manifest.version.empty()) {
    *error = "Missing version for manifest.";
    return false;
  }
  base::Version version(result->manifest.version);
  if (!version.IsValid()) {
    *error = "Invalid version: '";
    *error += result->manifest.version;
    *error += "'.";
    return false;
  }

  // Get the minimum browser version (not required).
  result->manifest.browser_min_version =
      GetAttribute(manifest, "prodversionmin");
  if (result->manifest.browser_min_version.length()) {
    base::Version browser_min_version(result->manifest.browser_min_version);
    if (!browser_min_version.IsValid()) {
      *error = "Invalid prodversionmin: '";
      *error += result->manifest.browser_min_version;
      *error += "'.";
      return false;
    }
  }

  // Get the <packages> node.
  std::vector<xmlNode*> packages = GetChildren(manifest, "packages");
  if (packages.empty()) {
    *error = "Missing packages tag on manifest.";
    return false;
  }

  // Parse each of the <package> tags.
  std::vector<xmlNode*> package = GetChildren(packages[0], "package");
  for (size_t i = 0; i != package.size(); ++i) {
    if (!ParsePackageTag(package[i], result, error))
      return false;
  }

  return true;
}

// Parses the <urls> tag and its children in the <updatecheck>.
bool ParseUrlsTag(xmlNode* urls,
                  ProtocolParser::Result* result,
                  std::string* error) {
  // Get the url nodes.
  std::vector<xmlNode*> url = GetChildren(urls, "url");
  if (url.empty()) {
    *error = "Missing url tags on urls.";
    return false;
  }

  // Get the list of urls for full and optionally, for diff updates.
  // There can only be either a codebase or a codebasediff attribute in a tag.
  for (size_t i = 0; i != url.size(); ++i) {
    // Find the url to the crx file.
    const GURL crx_url(GetAttribute(url[i], "codebase"));
    if (crx_url.is_valid()) {
      result->crx_urls.push_back(crx_url);
      continue;
    }
    const GURL crx_diffurl(GetAttribute(url[i], "codebasediff"));
    if (crx_diffurl.is_valid()) {
      result->crx_diffurls.push_back(crx_diffurl);
      continue;
    }
  }

  // Expect at least one url for full update.
  if (result->crx_urls.empty()) {
    *error = "Missing valid url for full update.";
    return false;
  }

  return true;
}

// Parses the <actions> tag. It picks up the "run" attribute of the first
// "action" element in "actions".
void ParseActionsTag(xmlNode* updatecheck, ProtocolParser::Result* result) {
  std::vector<xmlNode*> actions = GetChildren(updatecheck, "actions");
  if (actions.empty())
    return;

  std::vector<xmlNode*> action = GetChildren(actions.front(), "action");
  if (action.empty())
    return;

  result->action_run = GetAttribute(action.front(), "run");
}

// Parses the <updatecheck> tag.
bool ParseUpdateCheckTag(xmlNode* updatecheck,
                         ProtocolParser::Result* result,
                         std::string* error) {
  // Read the |status| attribute.
  result->status = GetAttribute(updatecheck, "status");
  if (result->status.empty()) {
    *error = "Missing status on updatecheck node";
    return false;
  }

  if (result->status == "noupdate") {
    ParseActionsTag(updatecheck, result);
    return true;
  }

  if (result->status == "ok") {
    std::vector<xmlNode*> urls = GetChildren(updatecheck, "urls");
    if (urls.empty()) {
      *error = "Missing urls on updatecheck.";
      return false;
    }

    if (!ParseUrlsTag(urls[0], result, error)) {
      return false;
    }

    std::vector<xmlNode*> manifests = GetChildren(updatecheck, "manifest");
    if (manifests.empty()) {
      *error = "Missing manifest on updatecheck.";
      return false;
    }

    ParseActionsTag(updatecheck, result);
    return ParseManifestTag(manifests[0], result, error);
  }

  // Return the |updatecheck| element status as a parsing error.
  *error = result->status;
  return false;
}

// Parses a single <app> tag.
bool ParseAppTag(xmlNode* app,
                 ProtocolParser::Result* result,
                 std::string* error) {
  // Read cohort information.
  auto cohort = GetAttributePtr(app, "cohort");
  static const char* attrs[] = {ProtocolParser::Result::kCohort,
                                ProtocolParser::Result::kCohortHint,
                                ProtocolParser::Result::kCohortName};
  for (auto* attr : attrs) {
    auto value = GetAttributePtr(app, attr);
    if (value)
      result->cohort_attrs.insert({attr, *value});
  }

  // Read the crx id.
  result->extension_id = GetAttribute(app, "appid");
  if (result->extension_id.empty()) {
    *error = "Missing appid on app node";
    return false;
  }

  // Get the <updatecheck> tag.
  std::vector<xmlNode*> updates = GetChildren(app, "updatecheck");
  if (updates.empty()) {
    *error = "Missing updatecheck on app.";
    return false;
  }

  return ParseUpdateCheckTag(updates[0], result, error);
}

bool ProtocolParser::Parse(const std::string& response_xml) {
  results_.daystart_elapsed_seconds = kNoDaystart;
  results_.daystart_elapsed_days = kNoDaystart;
  results_.list.clear();
  errors_.clear();

  if (response_xml.empty()) {
    ParseError("Empty xml");
    return false;
  }

  std::string xml_errors;
  ScopedXmlErrorFunc error_func(&xml_errors, &XmlErrorFunc);

  // Start up the xml parser with the manifest_xml contents.
  ScopedXmlDocument document(
      xmlParseDoc(reinterpret_cast<const xmlChar*>(response_xml.c_str())));
  if (!document.get()) {
    ParseError("%s", xml_errors.c_str());
    return false;
  }

  xmlNode* root = xmlDocGetRootElement(document.get());
  if (!root) {
    ParseError("Missing root node");
    return false;
  }

  if (!TagNameEquals(root, "response")) {
    ParseError("Missing response tag");
    return false;
  }

  // Check for the response "protocol" attribute.
  const auto protocol = GetAttribute(root, "protocol");
  if (protocol != kProtocolVersion) {
    ParseError(
        "Missing/incorrect protocol on response tag "
        "(expected '%s', found '%s')",
        kProtocolVersion, protocol.c_str());
    return false;
  }

  // Parse the first <daystart> if it is present.
  std::vector<xmlNode*> daystarts = GetChildren(root, "daystart");
  if (!daystarts.empty()) {
    xmlNode* first = daystarts[0];
    std::string elapsed_seconds = GetAttribute(first, "elapsed_seconds");
    int parsed_elapsed = kNoDaystart;
    if (base::StringToInt(elapsed_seconds, &parsed_elapsed)) {
      results_.daystart_elapsed_seconds = parsed_elapsed;
    }
    std::string elapsed_days = GetAttribute(first, "elapsed_days");
    parsed_elapsed = kNoDaystart;
    if (base::StringToInt(elapsed_days, &parsed_elapsed)) {
      results_.daystart_elapsed_days = parsed_elapsed;
    }
  }

  // Parse each of the <app> tags.
  std::vector<xmlNode*> apps = GetChildren(root, "app");
  for (size_t i = 0; i != apps.size(); ++i) {
    Result result;
    std::string error;
    if (ParseAppTag(apps[i], &result, &error)) {
      results_.list.push_back(result);
    } else {
      ParseError("%s", error.c_str());
    }
  }

  return true;
}

}  // namespace update_client
