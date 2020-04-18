// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/renderer/distillability_agent.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "components/dom_distiller/content/common/distillability_service.mojom.h"
#include "components/dom_distiller/core/distillable_page_detector.h"
#include "components/dom_distiller/core/experiments.h"
#include "components/dom_distiller/core/page_features.h"
#include "components/dom_distiller/core/url_utils.h"
#include "content/public/renderer/render_frame.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/web_distillability.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace dom_distiller {

using namespace blink;

namespace {

const char* const kBlacklist[] = {"www.reddit.com", "tools.usps.com"};

enum RejectionBuckets {
  NOT_ARTICLE = 0,
  MOBILE_FRIENDLY,
  BLACKLISTED,
  TOO_SHORT,
  NOT_REJECTED,
  REJECTION_BUCKET_BOUNDARY
};

// Returns whether it is necessary to send updates back to the browser.
// The number of updates can be from 0 to 2. See the tests in
// "distillable_page_utils_browsertest.cc".
// Most heuristics types only require one update after parsing.
// Adaboost-based heuristics are the only ones doing the second update,
// which is after loading.
bool NeedToUpdate(bool is_loaded) {
  switch (GetDistillerHeuristicsType()) {
    case DistillerHeuristicsType::ALWAYS_TRUE:
      return !is_loaded;
    case DistillerHeuristicsType::OG_ARTICLE:
      return !is_loaded;
    case DistillerHeuristicsType::ADABOOST_MODEL:
    case DistillerHeuristicsType::ALL_ARTICLES:
      return true;
    case DistillerHeuristicsType::NONE:
    default:
      return false;
  }
}

// Returns whether this update is the last one for the page.
bool IsLast(bool is_loaded) {
  if (GetDistillerHeuristicsType() == DistillerHeuristicsType::ADABOOST_MODEL ||
      GetDistillerHeuristicsType() == DistillerHeuristicsType::ALL_ARTICLES)
    return is_loaded;

  return true;
}

bool IsBlacklisted(const GURL& url) {
  for (size_t i = 0; i < arraysize(kBlacklist); ++i) {
    if (base::LowerCaseEqualsASCII(url.host(), kBlacklist[i])) {
      return true;
    }
  }
  return false;
}

bool IsDistillablePageAdaboost(WebDocument& doc,
                               const DistillablePageDetector* detector,
                               const DistillablePageDetector* long_page,
                               bool is_last,
                               bool& is_mobile_friendly) {
  GURL parsed_url(doc.Url());
  if (!parsed_url.is_valid()) {
    return false;
  }
  WebDistillabilityFeatures features = doc.DistillabilityFeatures();
  is_mobile_friendly = features.is_mobile_friendly;
  std::vector<double> derived = CalculateDerivedFeatures(
      features.open_graph, parsed_url, features.element_count,
      features.anchor_count, features.form_count, features.moz_score,
      features.moz_score_all_sqrt, features.moz_score_all_linear);
  double score = detector->Score(derived) - detector->GetThreshold();
  double long_score = long_page->Score(derived) - long_page->GetThreshold();
  bool distillable = score > 0;
  bool long_article = long_score > 0;
  bool blacklisted = IsBlacklisted(parsed_url);

  if (!features.is_mobile_friendly) {
    int score_int = std::round(score * 100);
    if (score > 0) {
      UMA_HISTOGRAM_COUNTS_1000("DomDistiller.DistillabilityScoreNMF.Positive",
          score_int);
    } else {
      UMA_HISTOGRAM_COUNTS_1000("DomDistiller.DistillabilityScoreNMF.Negative",
          -score_int);
    }
    if (distillable) {
      // The long-article model is trained with pages that are
      // non-mobile-friendly, and distillable (deemed by the first model), so
      // only record on that type of pages.
      int long_score_int = std::round(long_score * 100);
      if (long_score > 0) {
        UMA_HISTOGRAM_COUNTS_1000("DomDistiller.LongArticleScoreNMF.Positive",
            long_score_int);
      } else {
        UMA_HISTOGRAM_COUNTS_1000("DomDistiller.LongArticleScoreNMF.Negative",
            -long_score_int);
      }
    }
  }

  int bucket = static_cast<unsigned>(features.is_mobile_friendly) |
               (static_cast<unsigned>(distillable) << 1);
  if (is_last) {
    UMA_HISTOGRAM_ENUMERATION("DomDistiller.PageDistillableAfterLoading",
        bucket, 4);
  } else {
    UMA_HISTOGRAM_ENUMERATION("DomDistiller.PageDistillableAfterParsing",
        bucket, 4);
    if (!distillable) {
      UMA_HISTOGRAM_ENUMERATION("DomDistiller.DistillabilityRejection",
          NOT_ARTICLE, REJECTION_BUCKET_BOUNDARY);
    } else if (features.is_mobile_friendly) {
      UMA_HISTOGRAM_ENUMERATION("DomDistiller.DistillabilityRejection",
          MOBILE_FRIENDLY, REJECTION_BUCKET_BOUNDARY);
    } else if (blacklisted) {
      UMA_HISTOGRAM_ENUMERATION("DomDistiller.DistillabilityRejection",
          BLACKLISTED, REJECTION_BUCKET_BOUNDARY);
    } else if (!long_article) {
      UMA_HISTOGRAM_ENUMERATION("DomDistiller.DistillabilityRejection",
          TOO_SHORT, REJECTION_BUCKET_BOUNDARY);
    } else {
      UMA_HISTOGRAM_ENUMERATION("DomDistiller.DistillabilityRejection",
          NOT_REJECTED, REJECTION_BUCKET_BOUNDARY);
    }
  }

  if (blacklisted) {
    return false;
  }
  return distillable && long_article;
}

bool IsDistillablePage(WebDocument& doc,
                       bool is_last,
                       bool& is_mobile_friendly) {
  switch (GetDistillerHeuristicsType()) {
    case DistillerHeuristicsType::ALWAYS_TRUE:
      return true;
    case DistillerHeuristicsType::OG_ARTICLE:
      return doc.DistillabilityFeatures().open_graph;
    case DistillerHeuristicsType::ADABOOST_MODEL:
    case DistillerHeuristicsType::ALL_ARTICLES:
      return IsDistillablePageAdaboost(
          doc, DistillablePageDetector::GetNewModel(),
          DistillablePageDetector::GetLongPageModel(), is_last,
          is_mobile_friendly);
    case DistillerHeuristicsType::NONE:
    default:
      return false;
  }
}

}  // namespace

DistillabilityAgent::DistillabilityAgent(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
}

void DistillabilityAgent::DidMeaningfulLayout(
    WebMeaningfulLayout layout_type) {
  if (layout_type != WebMeaningfulLayout::kFinishedParsing &&
      layout_type != WebMeaningfulLayout::kFinishedLoading) {
    return;
  }

  DCHECK(render_frame());
  if (!render_frame()->IsMainFrame()) return;
  DCHECK(render_frame()->GetWebFrame());
  WebDocument doc = render_frame()->GetWebFrame()->GetDocument();
  if (doc.IsNull() || doc.Body().IsNull())
    return;
  if (!url_utils::IsUrlDistillable(doc.Url()))
    return;

  bool is_loaded = layout_type == WebMeaningfulLayout::kFinishedLoading;
  if (!NeedToUpdate(is_loaded)) return;

  bool is_last = IsLast(is_loaded);
  // Connect to Mojo service on browser to notify page distillability.
  mojom::DistillabilityServicePtr distillability_service;
  render_frame()->GetRemoteInterfaces()->GetInterface(
      &distillability_service);
  DCHECK(distillability_service);
  if (!distillability_service.is_bound()) return;
  bool is_mobile_friendly = false;
  bool is_distillable = IsDistillablePage(doc, is_last, is_mobile_friendly);
  distillability_service->NotifyIsDistillable(is_distillable, is_last,
                                              is_mobile_friendly);
}

DistillabilityAgent::~DistillabilityAgent() {}

void DistillabilityAgent::OnDestruct() {
  delete this;
}

}  // namespace dom_distiller
