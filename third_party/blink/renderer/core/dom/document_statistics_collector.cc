// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/document_statistics_collector.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_distillability.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/html_meta_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/histogram.h"

namespace blink {

using namespace HTMLNames;

namespace {

// Saturate the length of a paragraph to save time.
const int kTextContentLengthSaturation = 1000;

// Filter out short P elements. The threshold is set to around 2 English
// sentences.
const unsigned kParagraphLengthThreshold = 140;

// Saturate the scores to save time. The max is the score of 6 long paragraphs.
// 6 * sqrt(kTextContentLengthSaturation - kParagraphLengthThreshold)
const double kMozScoreSaturation = 175.954539583;
// 6 * sqrt(kTextContentLengthSaturation);
const double kMozScoreAllSqrtSaturation = 189.73665961;
const double kMozScoreAllLinearSaturation = 6 * kTextContentLengthSaturation;

unsigned TextContentLengthSaturated(const Element& root) {
  unsigned length = 0;
  // This skips shadow DOM intentionally, to match the JavaScript
  // implementation.  We would like to use the same statistics extracted by the
  // JavaScript implementation on iOS, and JavaScript cannot peek deeply into
  // shadow DOM except on modern Chrome versions.
  // Given shadow DOM rarely appears in <P> elements in long-form articles, the
  // overall accuracy should not be largely affected.
  for (Node& node : NodeTraversal::InclusiveDescendantsOf(root)) {
    if (!node.IsTextNode()) {
      continue;
    }
    length += ToText(node).length();
    if (length > kTextContentLengthSaturation) {
      return kTextContentLengthSaturation;
    }
  }
  return length;
}

bool IsVisible(const Element& element) {
  const ComputedStyle* style = element.GetComputedStyle();
  if (!style)
    return false;
  return (style->Display() != EDisplay::kNone &&
          style->Visibility() != EVisibility::kHidden && style->Opacity() != 0);
}

bool MatchAttributes(const Element& element, const Vector<String>& words) {
  const String& classes = element.GetClassAttribute();
  const String& id = element.GetIdAttribute();
  for (const String& word : words) {
    if (classes.FindIgnoringCase(word) != WTF::kNotFound ||
        id.FindIgnoringCase(word) != WTF::kNotFound) {
      return true;
    }
  }
  return false;
}

bool IsGoodForScoring(const WebDistillabilityFeatures& features,
                      const Element& element) {
  DEFINE_STATIC_LOCAL(Vector<String>, unlikely_candidates, ());
  if (unlikely_candidates.IsEmpty()) {
    auto words = {
        "banner",  "combx",      "comment", "community",  "disqus",  "extra",
        "foot",    "header",     "menu",    "related",    "remark",  "rss",
        "share",   "shoutbox",   "sidebar", "skyscraper", "sponsor", "ad-break",
        "agegate", "pagination", "pager",   "popup"};
    for (auto* word : words) {
      unlikely_candidates.push_back(word);
    }
  }
  DEFINE_STATIC_LOCAL(Vector<String>, highly_likely_candidates, ());
  if (highly_likely_candidates.IsEmpty()) {
    auto words = {"and", "article", "body", "column", "main", "shadow"};
    for (auto* word : words) {
      highly_likely_candidates.push_back(word);
    }
  }

  if (!IsVisible(element))
    return false;
  if (features.moz_score >= kMozScoreSaturation &&
      features.moz_score_all_sqrt >= kMozScoreAllSqrtSaturation &&
      features.moz_score_all_linear >= kMozScoreAllLinearSaturation)
    return false;
  if (MatchAttributes(element, unlikely_candidates) &&
      !MatchAttributes(element, highly_likely_candidates))
    return false;
  return true;
}

// underListItem denotes that at least one of the ancesters is <li> element.
void CollectFeatures(Element& root,
                     WebDistillabilityFeatures& features,
                     bool under_list_item = false) {
  for (Node& node : NodeTraversal::ChildrenOf(root)) {
    bool is_list_item = false;
    if (!node.IsElementNode()) {
      continue;
    }

    features.element_count++;
    Element& element = ToElement(node);
    if (element.HasTagName(aTag)) {
      features.anchor_count++;
    } else if (element.HasTagName(formTag)) {
      features.form_count++;
    } else if (element.HasTagName(inputTag)) {
      const HTMLInputElement& input = ToHTMLInputElement(element);
      if (input.type() == InputTypeNames::text) {
        features.text_input_count++;
      } else if (input.type() == InputTypeNames::password) {
        features.password_input_count++;
      }
    } else if (element.HasTagName(pTag) || element.HasTagName(preTag)) {
      if (element.HasTagName(pTag)) {
        features.p_count++;
      } else {
        features.pre_count++;
      }
      if (!under_list_item && IsGoodForScoring(features, element)) {
        unsigned length = TextContentLengthSaturated(element);
        if (length >= kParagraphLengthThreshold) {
          features.moz_score += sqrt(length - kParagraphLengthThreshold);
          features.moz_score =
              std::min(features.moz_score, kMozScoreSaturation);
        }
        features.moz_score_all_sqrt += sqrt(length);
        features.moz_score_all_sqrt =
            std::min(features.moz_score_all_sqrt, kMozScoreAllSqrtSaturation);

        features.moz_score_all_linear += length;
        features.moz_score_all_linear = std::min(features.moz_score_all_linear,
                                                 kMozScoreAllLinearSaturation);
      }
    } else if (element.HasTagName(liTag)) {
      is_list_item = true;
    }
    CollectFeatures(element, features, under_list_item || is_list_item);
  }
}

bool HasOpenGraphArticle(const Element& head) {
  DEFINE_STATIC_LOCAL(AtomicString, og_type, ("og:type"));
  DEFINE_STATIC_LOCAL(AtomicString, property_attr, ("property"));
  for (const Element* child = ElementTraversal::FirstChild(head); child;
       child = ElementTraversal::NextSibling(*child)) {
    if (!IsHTMLMetaElement(*child))
      continue;
    const HTMLMetaElement& meta = ToHTMLMetaElement(*child);

    if (meta.GetName() == og_type ||
        meta.getAttribute(property_attr) == og_type) {
      if (DeprecatedEqualIgnoringCase(meta.Content(), "article")) {
        return true;
      }
    }
  }
  return false;
}

bool IsMobileFriendly(Document& document) {
  if (Page* page = document.GetPage())
    return page->GetVisualViewport().ShouldDisableDesktopWorkarounds();
  return false;
}

}  // namespace

WebDistillabilityFeatures DocumentStatisticsCollector::CollectStatistics(
    Document& document) {
  TRACE_EVENT0("blink", "DocumentStatisticsCollector::collectStatistics");

  WebDistillabilityFeatures features = WebDistillabilityFeatures();

  if (!document.GetFrame() || !document.GetFrame()->IsMainFrame())
    return features;

  DCHECK(document.HasFinishedParsing());

  HTMLElement* body = document.body();
  HTMLElement* head = document.head();

  if (!body || !head)
    return features;

  features.is_mobile_friendly = IsMobileFriendly(document);

  double start_time = CurrentTimeTicksInSeconds();

  // This should be cheap since collectStatistics is only called right after
  // layout.
  document.UpdateStyleAndLayoutTree();

  // Traverse the DOM tree and collect statistics.
  CollectFeatures(*body, features);
  features.open_graph = HasOpenGraphArticle(*head);

  double elapsed_time = CurrentTimeTicksInSeconds() - start_time;

  DEFINE_STATIC_LOCAL(CustomCountHistogram, distillability_histogram,
                      ("WebCore.DistillabilityUs", 1, 1000000, 50));
  distillability_histogram.Count(static_cast<int>(1e6 * elapsed_time));

  return features;
}

}  // namespace blink
