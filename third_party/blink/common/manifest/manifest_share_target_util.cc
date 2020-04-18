// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/manifest/manifest_share_target_util.h"

#include <map>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "net/base/escape.h"
#include "url/gurl.h"

namespace blink {
namespace {

// Determines whether a character is allowed in a URL template placeholder.
bool IsIdentifier(char c) {
  return base::IsAsciiAlpha(c) || base::IsAsciiDigit(c) || c == '-' || c == '_';
}

// Returns to |out| the result of running the "replace placeholders"
// algorithm on |url_template|. The algorithm is specified at
// https://wicg.github.io/web-share-target/#dfn-replace-placeholders
// Does not copy any string data. The resulting vector can be concatenated
// together with base::StrCat to produce the final string.
bool ReplacePlaceholders(base::StringPiece template_string,
                         const std::map<base::StringPiece, std::string>& data,
                         std::vector<base::StringPiece>* out) {
  DCHECK(out);

  bool last_saw_open = false;
  size_t start_index_to_copy = 0;
  for (size_t i = 0; i < template_string.size(); ++i) {
    if (last_saw_open) {
      if (template_string[i] == '}') {
        base::StringPiece placeholder = template_string.substr(
            start_index_to_copy + 1, i - 1 - start_index_to_copy);
        auto it = data.find(placeholder);
        if (it != data.end()) {
          // Replace the placeholder text with the parameter value.
          out->push_back(it->second);
        }

        last_saw_open = false;
        start_index_to_copy = i + 1;
      } else if (!IsIdentifier(template_string[i])) {
        // Error: Non-identifier character seen after open.
        return false;
      }
    } else {
      if (template_string[i] == '}') {
        // Error: Saw close, with no corresponding open.
        return false;
      }
      if (template_string[i] == '{') {
        out->push_back(template_string.substr(start_index_to_copy,
                                              i - start_index_to_copy));

        last_saw_open = true;
        start_index_to_copy = i;
      }
    }
  }
  if (last_saw_open) {
    // Error: Saw open that was never closed.
    return false;
  }
  out->push_back(template_string.substr(
      start_index_to_copy, template_string.size() - start_index_to_copy));
  return true;
}

}  // namespace

bool ValidateWebShareUrlTemplate(const GURL& url_template) {
  return ReplaceWebShareUrlPlaceholders(url_template, /*title=*/"", /*text=*/"",
                                        /*share_url=*/GURL(),
                                        /*url_template_filled=*/nullptr);
}

bool ReplaceWebShareUrlPlaceholders(const GURL& url_template,
                                    base::StringPiece title,
                                    base::StringPiece text,
                                    const GURL& share_url,
                                    GURL* url_template_filled) {
  constexpr char kTitlePlaceholder[] = "title";
  constexpr char kTextPlaceholder[] = "text";
  constexpr char kUrlPlaceholder[] = "url";
  std::map<base::StringPiece, std::string> replacements;
  replacements[kTitlePlaceholder] = net::EscapeQueryParamValue(title, false);
  replacements[kTextPlaceholder] = net::EscapeQueryParamValue(text, false);
  replacements[kUrlPlaceholder] =
      net::EscapeQueryParamValue(share_url.spec(), false);

  std::vector<base::StringPiece> new_query_split;
  std::vector<base::StringPiece> new_ref_split;
  if (!ReplacePlaceholders(url_template.query_piece(), replacements,
                           &new_query_split) ||
      !ReplacePlaceholders(url_template.ref_piece(), replacements,
                           &new_ref_split)) {
    return false;
  }

  // Early-return optimization, since ReplaceWebShareUrlPlaceholders() is called
  // at manifest parse time just to get the success boolean, ignoring the
  // result.
  if (!url_template_filled)
    return true;

  // Ensure that the replacements are not deleted prior to
  // GURL::ReplaceComponents() being called. GURL::Replacements::SetQueryStr()
  // does not make a copy.
  std::string new_query = base::StrCat(new_query_split);
  std::string new_ref = base::StrCat(new_ref_split);

  // Check whether |url_template| has a query in order to preserve the '?' in
  // a URL with an empty query. e.g. http://www.google.com/?
  GURL::Replacements url_replacements;
  if (url_template.has_query())
    url_replacements.SetQueryStr(new_query);
  if (url_template.has_ref())
    url_replacements.SetRefStr(new_ref);
  *url_template_filled = url_template.ReplaceComponents(url_replacements);
  return true;
}

}  // namespace blink
