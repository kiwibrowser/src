// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/savable_resources.h"

#include <set>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/common/url_utils.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/public/web/web_view.h"

using blink::WebDocument;
using blink::WebElement;
using blink::WebElementCollection;
using blink::WebFrame;
using blink::WebInputElement;
using blink::WebLocalFrame;
using blink::WebNode;
using blink::WebString;
using blink::WebVector;
using blink::WebView;

namespace content {
namespace {

// Returns |true| if |web_frame| contains (or should be assumed to contain)
// a html document.
bool DoesFrameContainHtmlDocument(WebFrame* web_frame,
                                  const WebElement& element) {
  if (web_frame->IsWebLocalFrame()) {
    WebDocument doc = web_frame->ToWebLocalFrame()->GetDocument();
    return doc.IsHTMLDocument() || doc.IsXHTMLDocument();
  }

  // Cannot inspect contents of a remote frame, so we use a heuristic:
  // Assume that <iframe> and <frame> elements contain a html document,
  // and other elements (i.e. <object>) contain plugins or other resources.
  // If the heuristic is wrong (i.e. the remote frame in <object> does
  // contain an html document), then things will still work, but with the
  // following caveats: 1) original frame content will be saved and 2) links
  // in frame's html doc will not be rewritten to point to locally saved
  // files.
  return element.HasHTMLTagName("iframe") || element.HasHTMLTagName("frame");
}

// If present and valid, then push the link associated with |element|
// into either SavableResourcesResult::subframes or
// SavableResourcesResult::resources_list.
void GetSavableResourceLinkForElement(
    const WebElement& element,
    const WebDocument& current_doc,
    SavableResourcesResult* result) {
  // Get absolute URL.
  WebString link_attribute_value = GetSubResourceLinkFromElement(element);
  GURL element_url = current_doc.CompleteURL(link_attribute_value);

  // See whether to report this element as a subframe.
  WebFrame* web_frame = WebFrame::FromFrameOwnerElement(element);
  if (web_frame && DoesFrameContainHtmlDocument(web_frame, element)) {
    SavableSubframe subframe;
    subframe.original_url = element_url;
    subframe.routing_id = RenderFrame::GetRoutingIdForWebFrame(web_frame);
    result->subframes->push_back(subframe);
    return;
  }

  // Check whether the node has sub resource URL or not.
  if (link_attribute_value.IsNull())
    return;

  // Ignore invalid URL.
  if (!element_url.is_valid())
    return;

  // Ignore those URLs which are not standard protocols. Because FTP
  // protocol does no have cache mechanism, we will skip all
  // sub-resources if they use FTP protocol.
  if (!element_url.SchemeIsHTTPOrHTTPS() &&
      !element_url.SchemeIs(url::kFileScheme))
    return;

  result->resources_list->push_back(element_url);
}

}  // namespace

bool GetSavableResourceLinksForFrame(WebLocalFrame* current_frame,
                                     SavableResourcesResult* result) {
  // Get current frame's URL.
  GURL current_frame_url = current_frame->GetDocument().Url();

  // If url of current frame is invalid, ignore it.
  if (!current_frame_url.is_valid())
    return false;

  // If url of current frame is not a savable protocol, ignore it.
  if (!IsSavableURL(current_frame_url))
    return false;

  // Get current using document.
  WebDocument current_doc = current_frame->GetDocument();
  // Go through all descent nodes.
  WebElementCollection all = current_doc.All();
  // Go through all elements in this frame.
  for (WebElement element = all.FirstItem(); !element.IsNull();
       element = all.NextItem()) {
    GetSavableResourceLinkForElement(element,
                                     current_doc,
                                     result);
  }

  return true;
}

WebString GetSubResourceLinkFromElement(const WebElement& element) {
  const char* attribute_name = nullptr;
  if (element.HasHTMLTagName("img") || element.HasHTMLTagName("frame") ||
      element.HasHTMLTagName("iframe") || element.HasHTMLTagName("script")) {
    attribute_name = "src";
  } else if (element.HasHTMLTagName("input")) {
    const WebInputElement input = element.ToConst<WebInputElement>();
    if (input.IsImageButton()) {
      attribute_name = "src";
    }
  } else if (element.HasHTMLTagName("body") ||
             element.HasHTMLTagName("table") || element.HasHTMLTagName("tr") ||
             element.HasHTMLTagName("td")) {
    attribute_name = "background";
  } else if (element.HasHTMLTagName("blockquote") ||
             element.HasHTMLTagName("q") || element.HasHTMLTagName("del") ||
             element.HasHTMLTagName("ins")) {
    attribute_name = "cite";
  } else if (element.HasHTMLTagName("object")) {
    attribute_name = "data";
  } else if (element.HasHTMLTagName("link")) {
    // If the link element is not linked to css, ignore it.
    WebString type = element.GetAttribute("type");
    WebString rel = element.GetAttribute("rel");
    if ((type.ContainsOnlyASCII() &&
         base::LowerCaseEqualsASCII(type.Ascii(), "text/css")) ||
        (rel.ContainsOnlyASCII() &&
         base::LowerCaseEqualsASCII(rel.Ascii(), "stylesheet"))) {
      // TODO(jnd): Add support for extracting links of sub-resources which
      // are inside style-sheet such as @import, url(), etc.
      // See bug: http://b/issue?id=1111667.
      attribute_name = "href";
    }
  }
  if (!attribute_name)
    return WebString();
  WebString value = element.GetAttribute(WebString::FromUTF8(attribute_name));
  // If value has content and not start with "javascript:" then return it,
  // otherwise return NULL.
  if (!value.IsNull() && !value.IsEmpty() &&
      !base::StartsWith(value.Utf8(),
                        "javascript:", base::CompareCase::INSENSITIVE_ASCII))
    return value;

  return WebString();
}

}  // namespace content
