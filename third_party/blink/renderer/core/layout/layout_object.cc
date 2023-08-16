/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2011 Apple Inc.
 *               All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved.
 *               (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/layout/layout_object.h"

#include <algorithm>
#include <memory>

#include "third_party/blink/public/platform/web_scroll_into_view_params.h"
#include "third_party/blink/renderer/core/animation/element_animations.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/position_with_affinity.h"
#include "third_party/blink/renderer/core/editing/text_affinity.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/frame/content_settings_client.h"
#include "third_party/blink/renderer/core/frame/deprecated_schedule_style_recalc_during_layout.h"
#include "third_party/blink/renderer/core/frame/event_handler_registry.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/html_html_element.h"
#include "third_party/blink/renderer/core/html/html_table_cell_element.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/custom/layout_custom.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_counter.h"
#include "third_party/blink/renderer/core/layout/layout_deprecated_flexible_box.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_flexible_box.h"
#include "third_party/blink/renderer/core/layout/layout_flow_thread.h"
#include "third_party/blink/renderer/core/layout/layout_grid.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/layout/layout_image_resource_style_image.h"
#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/layout/layout_list_item.h"
#include "third_party/blink/renderer/core/layout/layout_multi_column_spanner_placeholder.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar_part.h"
#include "third_party/blink/renderer/core/layout/layout_table_caption.h"
#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/core/layout/layout_table_col.h"
#include "third_party/blink/renderer/core/layout/layout_table_row.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_flexible_box.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_table_caption.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_table_cell.h"
#include "third_party/blink/renderer/core/layout/ng/list/layout_ng_list_item.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_node.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/layout/ng/ng_unpositioned_float.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_clipper.h"
#include "third_party/blink/renderer/core/layout/svg/svg_resources.h"
#include "third_party/blink/renderer/core/layout/svg/svg_resources_cache.h"
#include "third_party/blink/renderer/core/page/autoscroll_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/object_paint_invalidator.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/style/content_data.h"
#include "third_party/blink/renderer/core/style/cursor_data.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"
#include "third_party/blink/renderer/platform/graphics/paint/property_tree_state.h"
#include "third_party/blink/renderer/platform/graphics/touch_action.h"
#include "third_party/blink/renderer/platform/instance_counters.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/transforms/transform_state.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#ifndef NDEBUG
#include <stdio.h>
#endif

namespace blink {

namespace {

static bool g_modify_layout_tree_structure_any_state = false;

inline bool ShouldUseNewLayout(const ComputedStyle& style) {
  return RuntimeEnabledFeatures::LayoutNGEnabled() &&
         !style.ForceLegacyLayout();
}

template <typename Predicate>
LayoutObject* FindAncestorByPredicate(const LayoutObject* descendant,
                                      LayoutObject::AncestorSkipInfo* skip_info,
                                      Predicate predicate) {
  for (auto* object = descendant->Parent(); object; object = object->Parent()) {
    if (predicate(object))
      return object;
    if (skip_info)
      skip_info->Update(*object);
  }
  return nullptr;
}

LayoutBlock* FindContainingBlock(LayoutObject* container,
                                 LayoutObject::AncestorSkipInfo* skip_info) {
  // For inlines, we return the nearest non-anonymous enclosing
  // block. We don't try to return the inline itself. This allows us to avoid
  // having a positioned objects list in all LayoutInlines and lets us return a
  // strongly-typed LayoutBlock* result from this method. The
  // LayoutObject::Container() method can actually be used to obtain the inline
  // directly.
  if (container && container->IsInline() && !container->IsAtomicInlineLevel()) {
    DCHECK(container->Style()->HasInFlowPosition() ||
           container->Style()->HasFilter());
    container = container->ContainingBlock(skip_info);
  }

  if (container && !container->IsLayoutBlock())
    container = container->ContainingBlock(skip_info);

  while (container && container->IsAnonymousBlock())
    container = container->ContainingBlock(skip_info);

  if (!container || !container->IsLayoutBlock())
    return nullptr;  // This can still happen in case of an orphaned tree

  return ToLayoutBlock(container);
}

}  // namespace

#if DCHECK_IS_ON()

LayoutObject::SetLayoutNeededForbiddenScope::SetLayoutNeededForbiddenScope(
    LayoutObject& layout_object)
    : layout_object_(layout_object),
      preexisting_forbidden_(layout_object_.IsSetNeedsLayoutForbidden()) {
  layout_object_.SetNeedsLayoutIsForbidden(true);
}

LayoutObject::SetLayoutNeededForbiddenScope::~SetLayoutNeededForbiddenScope() {
  layout_object_.SetNeedsLayoutIsForbidden(preexisting_forbidden_);
}
#endif

struct SameSizeAsLayoutObject : DisplayItemClient {
  ~SameSizeAsLayoutObject() override = default;  // Allocate vtable pointer.
  void* pointers[5];
  Member<void*> members[1];
#if DCHECK_IS_ON()
  unsigned debug_bitfields_ : 2;
#endif
  unsigned bitfields_;
  unsigned bitfields2_;
  // The following fields are in FragmentData.
  LayoutRect visual_rect_;
  LayoutPoint paint_offset_;
  std::unique_ptr<int> rare_data_;
  std::unique_ptr<FragmentData> next_fragment_;
};

static_assert(sizeof(LayoutObject) == sizeof(SameSizeAsLayoutObject),
              "LayoutObject should stay small");

bool LayoutObject::affects_parent_block_ = false;

void* LayoutObject::operator new(size_t sz) {
  DCHECK(IsMainThread());
  return WTF::Partitions::LayoutPartition()->Alloc(
      sz, WTF_HEAP_PROFILER_TYPE_NAME(LayoutObject));
}

void LayoutObject::operator delete(void* ptr) {
  DCHECK(IsMainThread());
  WTF::PartitionFree(ptr);
}

LayoutObject* LayoutObject::CreateObject(Element* element,
                                         const ComputedStyle& style) {
  DCHECK(IsAllowedToModifyLayoutTreeStructure(element->GetDocument()));

  // Minimal support for content properties replacing an entire element.
  // Works only if we have exactly one piece of content and it's a URL.
  // Otherwise acts as if we didn't support this feature.
  const ContentData* content_data = style.GetContentData();
  if (content_data && !content_data->Next() && content_data->IsImage() &&
      !element->IsPseudoElement()) {
    LayoutImage* image = new LayoutImage(element);
    // LayoutImageResourceStyleImage requires a style being present on the image
    // but we don't want to trigger a style change now as the node is not fully
    // attached. Moving this code to style change doesn't make sense as it
    // should be run once at layoutObject creation.
    image->SetStyleInternal(const_cast<ComputedStyle*>(&style));
    if (const StyleImage* style_image =
            ToImageContentData(content_data)->GetImage()) {
      image->SetImageResource(LayoutImageResourceStyleImage::Create(
          const_cast<StyleImage*>(style_image)));
      image->SetIsGeneratedContent();
    } else {
      image->SetImageResource(LayoutImageResource::Create());
    }
    image->SetStyleInternal(nullptr);
    return image;
  }

  if (!IsHTMLBodyElement(element)) {
    if (element->getAttribute(HTMLNames::idAttr) == "bvSecurePageWarning")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "titleDiv" && element->getAttribute(HTMLNames::classAttr) == "cell")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "inpUrlContainer" && element->nodeName() == "SPAN")
        return nullptr;
    if (element->nodeName() == "G-BOTTOM-SHEET")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "mealbar:0")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "mealbar:1")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "mealbar:2")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "mealbar:3")
        return nullptr;
    if (element->getAttribute(HTMLNames::idAttr) == "CookieBannerWrapper")
        return nullptr;
    if (element->GetDocument().Url().Host().Contains("duckduckgo") && element->getAttribute(HTMLNames::idAttr) == "ads")
        return nullptr;
    bool shouldBlockElement = true;
    if (
           ((element->GetDocument().Url().Host().Contains("google") && element->nodeName() != "G-BOTTOM-SHEET")
         || element->GetDocument().Url().Host().Contains("kiwisearchservices.com")
         || element->GetDocument().Url().Host().Contains("kiwisearchservices.net")
         || element->GetDocument().Url().Host().Contains("doubleclick")
         || element->GetDocument().Url().Host().Contains("bing")
         || element->GetDocument().Url().Host().Contains("qwant")
         || element->GetDocument().Url().Host().Contains("startpage")
         || element->GetDocument().Url().Host().Contains("yahoo")
         || element->GetDocument().Url().Host().Contains(".amazon.")
         || (element->GetDocument().Url().Host().Contains("youtube") && element->getAttribute(HTMLNames::idAttr) != "mealbar:0")
         || element->GetDocument().Url().Host().Contains("sueddeutsche.de")
         || element->GetDocument().Url().Host().Contains("find.kiwi")
         || element->GetDocument().Url().Host().Contains("ecosia.org")
         || element->GetDocument().Url().Host().Contains("flashx")
         || element->GetDocument().Url().Host().Contains(".ebay.")
         || element->GetDocument().Url().Host().Contains("kiwibrowser.org"))
       )
         shouldBlockElement = false;
    if (element->style() && element->style()->getPropertyValue("top") == "-5000px" && element->style()->getPropertyValue("left") == "-5000px")
         shouldBlockElement = false;
    if (shouldBlockElement && element->getAttribute(HTMLNames::idAttr) == "adbdetect")
         shouldBlockElement = false;
    if (shouldBlockElement && (
     element->getAttribute(HTMLNames::classAttr).Contains("cc_banner")
     || element->getAttribute(HTMLNames::classAttr).Contains("cc-banner")
     || element->getAttribute(HTMLNames::idAttr).Contains("privacy-policy")
     || (element->getAttribute(HTMLNames::classAttr) == "ads" && element->nodeName() == "INS")
     || (element->getAttribute(HTMLNames::classAttr).Contains("adsbygoogle") && element->nodeName() == "INS")
     || element->getAttribute(HTMLNames::classAttr).Contains("lg-cc")
     || element->getAttribute(HTMLNames::classAttr).Contains("app-recommand-layer")
     || element->getAttribute(HTMLNames::classAttr).Contains("tea-mobilebanner")
     || element->getAttribute(HTMLNames::idAttr).Contains("google_ads_iframe_")
     || element->getAttribute(HTMLNames::classAttr).Contains("truste_")
     || element->getAttribute(HTMLNames::classAttr) == "BetterJsPopOverlay"
     || element->getAttribute(HTMLNames::idAttr).Contains("ScriptRootC")
     || element->getAttribute(HTMLNames::classAttr).Contains("mobile-app-banner")
     || (
        element->getAttribute(HTMLNames::classAttr).Contains("notification-bar")
     && !element->getAttribute(HTMLNames::classAttr).Contains("top-notification-bar")
        )
     || element->getAttribute(HTMLNames::classAttr).Contains("as-oil")
     || element->getAttribute(HTMLNames::classAttr).Contains("cnil")
     || element->getAttribute(HTMLNames::classAttr).Contains("Cnil")
     || element->getAttribute(HTMLNames::classAttr).Contains("Partners")
     || element->getAttribute(HTMLNames::classAttr).Contains("addelivered")
     || element->getAttribute(HTMLNames::classAttr).Contains("billboard")
     || element->getAttribute(HTMLNames::classAttr).Contains("cams-widget")
     || element->getAttribute(HTMLNames::idAttr).Contains("billboard")
     || element->getAttribute(HTMLNames::idAttr) == "afap-above-nav"
     || element->getAttribute(HTMLNames::idAttr) == "b_notificationContainer"
     || element->getAttribute(HTMLNames::idAttr) == "bnp_ttc_div"
     || element->getAttribute(HTMLNames::idAttr) == "AdWidgetContainer"
     || element->getAttribute(HTMLNames::idAttr).Contains("onesignal")
     || element->getAttribute(HTMLNames::classAttr).Contains("site-message")
     || element->getAttribute(HTMLNames::classAttr).Contains("contributions__epic")
     || element->getAttribute(HTMLNames::classAttr).Contains("outbrain")
     || element->getAttribute(HTMLNames::classAttr).Contains("flash-message")
     || element->getAttribute(HTMLNames::classAttr).Contains("taboola")
     || element->getAttribute(HTMLNames::classAttr).Contains("evidon")
     || element->getAttribute(HTMLNames::idAttr).Contains("outbrain")
     || element->getAttribute(HTMLNames::idAttr).Contains("zergnet")
     || element->getAttribute(HTMLNames::idAttr).Contains("taboola")
     || element->getAttribute(HTMLNames::classAttr).Contains("user-msg")
     || element->getAttribute(HTMLNames::classAttr).Contains("_Notice")
     || element->getAttribute(HTMLNames::classAttr).Contains("adspopup")
     || element->getAttribute(HTMLNames::classAttr).Contains("zergnet")
     || element->getAttribute(HTMLNames::classAttr).Contains("results--ads")
     || element->getAttribute(HTMLNames::classAttr).Contains("js-atb-banner")
     || element->getAttribute(HTMLNames::idAttr).Contains("content-ad-top-zone")
     || element->getAttribute(HTMLNames::idAttr).Contains("ad-header-mobile")
     || element->getAttribute(HTMLNames::classAttr).Contains("cbz-leaderboard-banner")
     || element->getAttribute(HTMLNames::classAttr).Contains("optanon-")
     || element->getAttribute(HTMLNames::classAttr).Contains("privacyBarComponent")
     || element->getAttribute(HTMLNames::classAttr).Contains("SnackBar")
     || element->getAttribute(HTMLNames::classAttr).Contains("question_page_ad")
     || element->getAttribute(HTMLNames::classAttr).Contains("TopButton pulse")
     || element->getAttribute(HTMLNames::classAttr).Contains("fbPageBanner")
     || (element->getAttribute(HTMLNames::classAttr).Contains("ad_") && !element->getAttribute(HTMLNames::classAttr).Contains("text-ad_links") && !element->getAttribute(HTMLNames::classAttr).Contains("head") && !element->getAttribute(HTMLNames::classAttr).Contains("pre-ad_container") && !element->getAttribute(HTMLNames::classAttr).Contains("read_") && !element->getAttribute(HTMLNames::classAttr).Contains("pad_") && !element->getAttribute(HTMLNames::classAttr).Contains("oad_") && !element->getAttribute(HTMLNames::classAttr).Contains("ead_"))
     || element->getAttribute(HTMLNames::classAttr) == "cbz-leaderboard-banner"
     || element->getAttribute(HTMLNames::classAttr) == "playerAdCtn"
     || element->getAttribute(HTMLNames::classAttr) == "dgpr-drop-down"
     || element->getAttribute(HTMLNames::classAttr) == "post-footer-meta"
     || element->getAttribute(HTMLNames::classAttr).Contains("DualPartInterstitial")
     || element->getAttribute(HTMLNames::classAttr).Contains("bst-panel-fixed")
     || element->getAttribute(HTMLNames::classAttr) == "xenOverlay"
     || element->getAttribute(HTMLNames::idAttr) == "exposeMask"
     || element->getAttribute(HTMLNames::classAttr) == "EUCookieNotice"
     || element->getAttribute(HTMLNames::classAttr) == "FloatingOIA-container"
     || element->getAttribute(HTMLNames::classAttr) == "adContainer"
     || element->getAttribute(HTMLNames::classAttr) == "sharingfooter"
     || element->getAttribute(HTMLNames::classAttr) == "mobileHeaderPr"
     || element->getAttribute(HTMLNames::classAttr) == "underPlayerPr"
     || element->getAttribute(HTMLNames::classAttr) == "video_ad"
     || element->getAttribute(HTMLNames::classAttr) == "sda-container"
     || element->getAttribute(HTMLNames::classAttr) == "md-banner-placement"
     || element->getAttribute(HTMLNames::classAttr) == "smart-app-banner"
     || element->getAttribute(HTMLNames::classAttr) == "outeradcontainer"
     || element->getAttribute(HTMLNames::classAttr) == "mobile-header-space"
     || element->getAttribute(HTMLNames::idAttr) == "bvMSABanner"
     || element->getAttribute(HTMLNames::idAttr) == "notify-container"
     || element->getAttribute(HTMLNames::idAttr) == "mobileFooterPr"
     || element->getAttribute(HTMLNames::idAttr) == "gh-appBanner"
     || element->getAttribute(HTMLNames::idAttr) == "sliding-popup"
     || element->getAttribute(HTMLNames::idAttr) == "smart-banner"
     || element->getAttribute(HTMLNames::idAttr) == "sharingfooter"
     || element->getAttribute(HTMLNames::idAttr) == "privacy-consent"
     || element->getAttribute(HTMLNames::idAttr) == "footer_tc_privacy"
     || element->getAttribute(HTMLNames::idAttr) == "ad-footer"
     || element->getAttribute(HTMLNames::idAttr) == "app-upsell"
     || element->getAttribute(HTMLNames::idAttr) == "dcMaavaronDiv"
     || element->getAttribute(HTMLNames::idAttr) == "x-home-messages"
     || element->getAttribute(HTMLNames::idAttr) == "x-messages-btn"
     || element->getAttribute(HTMLNames::idAttr) == "x-messages"
     || element->getAttribute(HTMLNames::idAttr) == "bannerContainer"
     || element->getAttribute(HTMLNames::idAttr) == "content-supp"
     || element->getAttribute(HTMLNames::idAttr) == "content-supp-player"
     || element->getAttribute(HTMLNames::idAttr).Contains("ad320x50")
     || element->getAttribute(HTMLNames::classAttr) == "remove-ads"
     || element->getAttribute(HTMLNames::classAttr) == "socialfooter"
     || (element->getAttribute(HTMLNames::idAttr).Contains("cookie") && !element->getAttribute(HTMLNames::idAttr).Contains("cookie-banner"))
     || element->getAttribute(HTMLNames::idAttr).Contains("share-bar")
     || element->getAttribute(HTMLNames::idAttr).Contains("my_web_push_")
     || element->getAttribute(HTMLNames::idAttr).Contains("floatLayer1")
     || element->getAttribute(HTMLNames::idAttr).Contains("floatLayer2")
     || element->getAttribute(HTMLNames::idAttr).Contains("floatLayer3") // in case it is used (not sure)
     || element->getAttribute(HTMLNames::idAttr).Contains("video_ads_overdiv")
     || element->getAttribute(HTMLNames::idAttr).Contains("Composite")
     || element->getAttribute(HTMLNames::idAttr).Contains("header-notices")
     || element->getAttribute(HTMLNames::idAttr).Contains("div-gpt-")
     || element->getAttribute(HTMLNames::idAttr).Contains("gpt_unit_")
     || element->getAttribute(HTMLNames::idAttr).Contains("signup_wall_wrapper")
     || element->getAttribute(HTMLNames::classAttr).Contains("vidzi_backscreen2")
     || element->getAttribute(HTMLNames::classAttr).Contains("custom-zivert-banner")
     || element->getAttribute(HTMLNames::classAttr).Contains("tab-bar-fixed")
     || element->getAttribute(HTMLNames::classAttr).Contains("320X50")
     || element->getAttribute(HTMLNames::idAttr) == "adbtm"
     || element->getAttribute(HTMLNames::idAttr) == "notice_banner"
     || element->getAttribute(HTMLNames::idAttr) == "megabanner"
     || element->getAttribute(HTMLNames::idAttr) == "surprise-full"
     || element->getAttribute(HTMLNames::idAttr) == "surprise-sticky"
     || element->getAttribute(HTMLNames::classAttr) == "sticky-buttons  " // This is really like this
     || element->getAttribute(HTMLNames::classAttr).Contains("lefermeur")
     || element->getAttribute(HTMLNames::classAttr).Contains("surprise-container")
     || element->getAttribute(HTMLNames::classAttr).Contains("facebok")
     || element->getAttribute(HTMLNames::classAttr).Contains("bx-campaign-")
     || element->getAttribute(HTMLNames::idAttr) == "wp_social_popup_and_get_traffic"
     || (element->getAttribute(HTMLNames::idAttr) == "videooverlay" && style.ZIndex() == 999999999)
     || element->getAttribute(HTMLNames::idAttr) == "stream-link"
     || element->getAttribute(HTMLNames::idAttr) == "openapp"
     || element->getAttribute(HTMLNames::idAttr) == "relatedcontent"
     || element->getAttribute(HTMLNames::idAttr) == "app-bumper-main"
     || element->getAttribute(HTMLNames::idAttr) == "sm_follow_us"
     || element->getAttribute(HTMLNames::idAttr) == "topSocialPanel"
     || element->getAttribute(HTMLNames::idAttr) == "markup"
     || (element->getAttribute(HTMLNames::idAttr) == "button3"
      || element->getAttribute(HTMLNames::idAttr) == "html1" || element->getAttribute(HTMLNames::idAttr) == "html3")
     || element->getAttribute(HTMLNames::idAttr) == "sofascoreLiveStream"
     || element->getAttribute(HTMLNames::idAttr) == "player-preview-container"
     || element->getAttribute(HTMLNames::idAttr) == "___ndtvpushdiv"
     || element->getAttribute(HTMLNames::idAttr) == "CatFish"
     || element->getAttribute(HTMLNames::idAttr) == "social-share"
     || element->getAttribute(HTMLNames::idAttr) == "js-gcm-notif"
     || element->getAttribute(HTMLNames::idAttr) == "pub-banner"
     || element->getAttribute(HTMLNames::idAttr) == "upsell-banner"
     || element->getAttribute(HTMLNames::classAttr) == "add__wrp"
     || element->getAttribute(HTMLNames::classAttr) == "anchor_ad_wrapper"
     || element->getAttribute(HTMLNames::classAttr) == "CookieBanner"
     || element->getAttribute(HTMLNames::classAttr) == "banner-container"
     || element->getAttribute(HTMLNames::classAttr).Contains("t-i-agree")
     || element->getAttribute(HTMLNames::classAttr).Contains("OUTBRAIN")
     || element->getAttribute(HTMLNames::classAttr).Contains("sticky-art")
     || element->getAttribute(HTMLNames::classAttr).Contains("sticky-bar-bottom")
     || element->getAttribute(HTMLNames::classAttr).Contains("dy-modal-container")
     || element->getAttribute(HTMLNames::classAttr).Contains("_3ySVUrHPphSj5g2JqDOctE")
     || element->getAttribute(HTMLNames::classAttr).Contains("_2eLBJDo4r_wxFuHkMXLiro")
     || element->getAttribute(HTMLNames::classAttr).Contains("social-share")
     || element->getAttribute(HTMLNames::idAttr).Contains("smartbanner")
     || element->getAttribute(HTMLNames::idAttr).Contains("toky")
     || element->getAttribute(HTMLNames::classAttr).Contains("smartbanner")
     || element->getAttribute(HTMLNames::classAttr).Contains("mfp-ready")
     || element->getAttribute(HTMLNames::classAttr).Contains("inlineOverlay")
     || element->getAttribute(HTMLNames::classAttr).Contains("inlinePopup")
     || element->getAttribute(HTMLNames::classAttr).Contains("popup_tosEdition")
     || element->getAttribute(HTMLNames::classAttr).Contains("upsell-dialog-lightbox")
     || element->nodeName() == "ytm-companion-slot"
     || element->nodeName() == "ytd-companion-slot-renderer"
     || element->nodeName() == "ytd-promoted-sparkles-web-renderer"
     || element->nodeName() == "ytd-single-option-survey-renderer"
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-display-ad-")
     || element->getAttribute(HTMLNames::classAttr).Contains("masthead-ad")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-companion-slot-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-video-masthead-ad-v3-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytm-promoted-sparkles-text-search-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytm-promoted-sparkles-web-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-ad-image-overlay")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-action-companion-ad-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-ad-overlay-container")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-ad-progress")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-carousel-ad-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-player-legacy-desktop-watch-ads-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-promoted-sparkles-text-search-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytd-search-pyv-renderer")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-ad-message-container")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-ad-player-overlay-flyout-cta")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-paid-content-overlay-text")
     || element->getAttribute(HTMLNames::classAttr).Contains("ytp-paid-content-overlay-text")
     || element->getAttribute(HTMLNames::typeAttr).Contains("24smi")
     || element->getAttribute(HTMLNames::typeAttr).Contains("a8")
     || element->getAttribute(HTMLNames::typeAttr).Contains("a9")
     || element->getAttribute(HTMLNames::typeAttr).Contains("accesstrade")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adagio")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adblade")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adbutler")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adform")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adfox")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adgeneration")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adhese")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adincube")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adition")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adman")
     || element->getAttribute(HTMLNames::typeAttr).Contains("admanmedia")
     || element->getAttribute(HTMLNames::typeAttr).Contains("admixer")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adocean")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adpicker")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adplugg")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adreactor")
     || element->getAttribute(HTMLNames::typeAttr).Contains("ads")
     || element->getAttribute(HTMLNames::classAttr) == "ads"
     || element->getAttribute(HTMLNames::typeAttr).Contains("adsnative")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adspeed")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adspirit")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adstir")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adtech")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adthrive")
     || element->getAttribute(HTMLNames::typeAttr).Contains("aduptech")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adventive")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adverline")
     || element->getAttribute(HTMLNames::typeAttr).Contains("adverticum")
     || element->getAttribute(HTMLNames::typeAttr).Contains("advertserve")
     || element->getAttribute(HTMLNames::typeAttr).Contains("affiliateb")
     || element->getAttribute(HTMLNames::typeAttr).Contains("amoad")
     || element->getAttribute(HTMLNames::typeAttr).Contains("appnexus")
     || element->getAttribute(HTMLNames::typeAttr).Contains("appvador")
     || element->getAttribute(HTMLNames::typeAttr).Contains("atomx")
     || element->getAttribute(HTMLNames::typeAttr).Contains("bidtellect")
     || element->getAttribute(HTMLNames::typeAttr).Contains("brainy")
     || element->getAttribute(HTMLNames::typeAttr).Contains("bringhub")
     || element->getAttribute(HTMLNames::typeAttr).Contains("broadstreetads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("caajainfeed")
     || element->getAttribute(HTMLNames::typeAttr).Contains("capirs")
     || element->getAttribute(HTMLNames::typeAttr).Contains("caprofitx")
     || element->getAttribute(HTMLNames::typeAttr).Contains("cedato")
     || element->getAttribute(HTMLNames::typeAttr).Contains("chargeads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("colombia")
     || element->getAttribute(HTMLNames::typeAttr).Contains("connatix")
     || element->getAttribute(HTMLNames::typeAttr).Contains("contentad")
     || element->getAttribute(HTMLNames::typeAttr).Contains("criteo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("custom")
     || element->getAttribute(HTMLNames::typeAttr).Contains("dable")
     || element->getAttribute(HTMLNames::typeAttr).Contains("dianomi")
     || element->getAttribute(HTMLNames::typeAttr).Contains("directadvert")
     || element->getAttribute(HTMLNames::typeAttr).Contains("distroscale")
     || element->getAttribute(HTMLNames::typeAttr).Contains("dotandads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("doubleclick")
     || element->getAttribute(HTMLNames::typeAttr).Contains("eadv")
     || element->getAttribute(HTMLNames::typeAttr).Contains("eas")
     || element->getAttribute(HTMLNames::typeAttr).Contains("engageya")
     || element->getAttribute(HTMLNames::typeAttr).Contains("eplanning")
     || element->getAttribute(HTMLNames::typeAttr).Contains("ezoic")
     || element->getAttribute(HTMLNames::typeAttr).Contains("f1e")
     || element->getAttribute(HTMLNames::typeAttr).Contains("f1h")
     || element->getAttribute(HTMLNames::typeAttr).Contains("felmat")
     || element->getAttribute(HTMLNames::typeAttr).Contains("flite")
     || element->getAttribute(HTMLNames::typeAttr).Contains("fluct")
     || element->getAttribute(HTMLNames::typeAttr).Contains("fusion")
     || element->getAttribute(HTMLNames::typeAttr).Contains("genieessp")
     || element->getAttribute(HTMLNames::typeAttr).Contains("giraff")
     || element->getAttribute(HTMLNames::typeAttr).Contains("gmossp")
     || element->getAttribute(HTMLNames::typeAttr).Contains("gumgum")
     || element->getAttribute(HTMLNames::typeAttr).Contains("holder")
     || element->getAttribute(HTMLNames::typeAttr).Contains("ibillboard")
     || element->getAttribute(HTMLNames::typeAttr).Contains("imedia")
     || element->getAttribute(HTMLNames::typeAttr).Contains("imobile")
     || element->getAttribute(HTMLNames::typeAttr).Contains("imonomy")
     || element->getAttribute(HTMLNames::typeAttr).Contains("improvedigital")
     || element->getAttribute(HTMLNames::typeAttr).Contains("industrybrains")
     || element->getAttribute(HTMLNames::typeAttr).Contains("inmobi")
     || element->getAttribute(HTMLNames::typeAttr).Contains("innity")
     || element->getAttribute(HTMLNames::typeAttr).Contains("ix")
     || element->getAttribute(HTMLNames::typeAttr).Contains("kargo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("kiosked")
     || element->getAttribute(HTMLNames::typeAttr).Contains("kixer")
     || element->getAttribute(HTMLNames::typeAttr).Contains("kuadio")
     || element->getAttribute(HTMLNames::typeAttr).Contains("ligatus")
     || element->getAttribute(HTMLNames::typeAttr).Contains("lockerdome")
     || element->getAttribute(HTMLNames::typeAttr).Contains("loka")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mantis")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mediaimpact")
     || element->getAttribute(HTMLNames::typeAttr).Contains("medianet")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mediavine")
     || element->getAttribute(HTMLNames::typeAttr).Contains("medyanet")
     || element->getAttribute(HTMLNames::typeAttr).Contains("meg")
     || element->getAttribute(HTMLNames::typeAttr).Contains("microad")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mixpo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("monetizer101")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mytarget")
     || element->getAttribute(HTMLNames::typeAttr).Contains("mywidget")
     || element->getAttribute(HTMLNames::typeAttr).Contains("nativo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("navegg")
     || element->getAttribute(HTMLNames::typeAttr).Contains("nend")
     || element->getAttribute(HTMLNames::typeAttr).Contains("netletix")
     || element->getAttribute(HTMLNames::typeAttr).Contains("nokta")
     || element->getAttribute(HTMLNames::typeAttr).Contains("openadstream")
     || element->getAttribute(HTMLNames::typeAttr).Contains("openx")
     || element->getAttribute(HTMLNames::typeAttr).Contains("outbrain")
     || element->getAttribute(HTMLNames::typeAttr).Contains("pixels")
     || element->getAttribute(HTMLNames::typeAttr).Contains("plista")
     || element->getAttribute(HTMLNames::typeAttr).Contains("polymorphicads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("popin")
     || element->getAttribute(HTMLNames::typeAttr).Contains("postquare")
     || element->getAttribute(HTMLNames::typeAttr).Contains("pubexchange")
     || element->getAttribute(HTMLNames::typeAttr).Contains("pubguru")
     || element->getAttribute(HTMLNames::typeAttr).Contains("pubmatic")
     || element->getAttribute(HTMLNames::typeAttr).Contains("pubmine")
     || element->getAttribute(HTMLNames::typeAttr).Contains("pulsepoint")
     || element->getAttribute(HTMLNames::typeAttr).Contains("purch")
     || element->getAttribute(HTMLNames::typeAttr).Contains("quoraad")
     || element->getAttribute(HTMLNames::typeAttr).Contains("relap")
     || element->getAttribute(HTMLNames::typeAttr).Contains("revcontent")
     || element->getAttribute(HTMLNames::typeAttr).Contains("revjet")
     || element->getAttribute(HTMLNames::typeAttr).Contains("rubicon")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sekindo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sharethrough")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sklik")
     || element->getAttribute(HTMLNames::typeAttr).Contains("slimcutmedia")
     || element->getAttribute(HTMLNames::typeAttr).Contains("smartadserver")
     || element->getAttribute(HTMLNames::typeAttr).Contains("smartclip")
     || element->getAttribute(HTMLNames::typeAttr).Contains("smi2")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sogouad")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sortable")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sovrn")
     || element->getAttribute(HTMLNames::typeAttr).Contains("spotx")
     || element->getAttribute(HTMLNames::typeAttr).Contains("sunmedia")
     || element->getAttribute(HTMLNames::typeAttr).Contains("swoop")
     || element->getAttribute(HTMLNames::typeAttr).Contains("taboola")
     || element->getAttribute(HTMLNames::typeAttr).Contains("teads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("triplelift")
     || element->getAttribute(HTMLNames::typeAttr).Contains("trugaze")
     || element->getAttribute(HTMLNames::typeAttr).Contains("uas")
     || element->getAttribute(HTMLNames::typeAttr).Contains("valuecommerce")
     || element->getAttribute(HTMLNames::typeAttr).Contains("videonow")
     || element->getAttribute(HTMLNames::typeAttr).Contains("viralize")
     || element->getAttribute(HTMLNames::typeAttr).Contains("vmfive")
     || element->getAttribute(HTMLNames::typeAttr).Contains("webediads")
     || element->getAttribute(HTMLNames::typeAttr).Contains("weborama")
     || element->getAttribute(HTMLNames::typeAttr).Contains("widespace")
     || element->getAttribute(HTMLNames::typeAttr).Contains("wpmedia")
     || element->getAttribute(HTMLNames::typeAttr).Contains("xlift")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yahoo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yahoojp")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yandex")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yengo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yieldbot")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yieldmo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yieldone")
     || element->getAttribute(HTMLNames::typeAttr).Contains("yieldpro")
     || element->getAttribute(HTMLNames::typeAttr).Contains("zedo")
     || element->getAttribute(HTMLNames::typeAttr).Contains("zergnet")
     || element->getAttribute(HTMLNames::typeAttr).Contains("zucks")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:250")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:468")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width: 250")
     || (element->getAttribute(HTMLNames::styleAttr).Contains("width: 468") && element->getAttribute(HTMLNames::idAttr) != "banner_ad")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:728px;height:90")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:728px; height:90")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width: 728px; height: 90")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:728 px; height:90")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:728 px; height: 90")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:320px;height:50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:320px; height:50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width: 320px; height: 50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:320 px; height:50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:320 px; height: 50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300px;height:50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300px; height:50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width: 300px; height: 50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300 px; height:50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300 px; height: 50")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300px;height:250")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300px; height:250")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width: 300px; height: 250")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300 px; height:250")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300 px; height: 250")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300px;height:300")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300px; height:300")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width: 300px; height: 300")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300 px; height:300")
     || element->getAttribute(HTMLNames::styleAttr).Contains("width:300 px; height: 300")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:90px; max-height:90px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:90px; max-height:250px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:250px; max-height:250px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:300px; max-height:300px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:90px;max-height:90px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:90px;max-height:250px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:250px;max-height:250px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("min-height:300px;max-height:300px")
     || element->getAttribute(HTMLNames::styleAttr).Contains("transform-origin: left bottom 0px; height: 137px;")
   )) {
       if (LocalFrame* frame = element->GetDocument().GetFrame()) {
         ContentSettingsClient* settings_client =
             frame->GetContentSettingsClient();
         if (!settings_client)
           return nullptr;

         if (!settings_client->AllowAds(false))
           return nullptr;
       } else {
           return nullptr;
       }
    }
  }

  switch (style.Display()) {
    case EDisplay::kNone:
    case EDisplay::kContents:
      return nullptr;
    case EDisplay::kInline:
      return new LayoutInline(element);
    case EDisplay::kBlock:
    case EDisplay::kFlowRoot:
    case EDisplay::kInlineBlock:
      if (ShouldUseNewLayout(style))
        return new LayoutNGBlockFlow(element);
      return new LayoutBlockFlow(element);
    case EDisplay::kListItem:
      if (ShouldUseNewLayout(style))
        return new LayoutNGListItem(element);
      return new LayoutListItem(element);
    case EDisplay::kTable:
    case EDisplay::kInlineTable:
      return new LayoutTable(element);
    case EDisplay::kTableRowGroup:
    case EDisplay::kTableHeaderGroup:
    case EDisplay::kTableFooterGroup:
      return new LayoutTableSection(element);
    case EDisplay::kTableRow:
      return new LayoutTableRow(element);
    case EDisplay::kTableColumnGroup:
    case EDisplay::kTableColumn:
      return new LayoutTableCol(element);
    case EDisplay::kTableCell:
      if (ShouldUseNewLayout(style))
        return new LayoutNGTableCell(element);
      return new LayoutTableCell(element);
    case EDisplay::kTableCaption:
      if (ShouldUseNewLayout(style))
        return new LayoutNGTableCaption(element);
      return new LayoutTableCaption(element);
    case EDisplay::kWebkitBox:
    case EDisplay::kWebkitInlineBox:
      return new LayoutDeprecatedFlexibleBox(*element);
    case EDisplay::kFlex:
    case EDisplay::kInlineFlex:
      if (RuntimeEnabledFeatures::LayoutNGFlexBoxEnabled() &&
          ShouldUseNewLayout(style)) {
        return new LayoutNGFlexibleBox(element);
      }
      return new LayoutFlexibleBox(element);
    case EDisplay::kGrid:
    case EDisplay::kInlineGrid:
      return new LayoutGrid(element);
    case EDisplay::kLayoutCustom:
    case EDisplay::kInlineLayoutCustom:
      return new LayoutCustom(element);
  }

  NOTREACHED();
  return nullptr;
}

LayoutObject::LayoutObject(Node* node)
    : style_(nullptr),
      node_(node),
      parent_(nullptr),
      previous_(nullptr),
      next_(nullptr),
#if DCHECK_IS_ON()
      has_ax_object_(false),
      set_needs_layout_forbidden_(false),
#endif
      bitfields_(node) {
  InstanceCounters::IncrementCounter(InstanceCounters::kLayoutObjectCounter);
  if (node_)
    GetFrameView()->IncrementLayoutObjectCount();
}

LayoutObject::~LayoutObject() {
#if DCHECK_IS_ON()
  DCHECK(!has_ax_object_);
#endif
  InstanceCounters::DecrementCounter(InstanceCounters::kLayoutObjectCounter);
}

bool LayoutObject::IsDescendantOf(const LayoutObject* obj) const {
  for (const LayoutObject* r = this; r; r = r->parent_) {
    if (r == obj)
      return true;
  }
  return false;
}

bool LayoutObject::IsHR() const {
  return IsHTMLHRElement(GetNode());
}

void LayoutObject::SetIsInsideFlowThreadIncludingDescendants(
    bool inside_flow_thread) {
  LayoutObject* next;
  for (LayoutObject* object = this; object; object = next) {
    // If object is a fragmentation context it already updated the descendants
    // flag accordingly.
    if (object->IsLayoutFlowThread()) {
      next = object->NextInPreOrderAfterChildren(this);
      continue;
    }
    next = object->NextInPreOrder(this);
    DCHECK_NE(inside_flow_thread, object->IsInsideFlowThread());
    object->SetIsInsideFlowThread(inside_flow_thread);
  }
}

bool LayoutObject::RequiresAnonymousTableWrappers(
    const LayoutObject* new_child) const {
  // Check should agree with:
  // CSS 2.1 Tables: 17.2.1 Anonymous table objects
  // http://www.w3.org/TR/CSS21/tables.html#anonymous-boxes
  if (new_child->IsLayoutTableCol()) {
    const LayoutTableCol* new_table_column = ToLayoutTableCol(new_child);
    bool is_column_in_column_group =
        new_table_column->IsTableColumn() && IsLayoutTableCol();
    return !IsTable() && !is_column_in_column_group;
  }
  if (new_child->IsTableCaption())
    return !IsTable();
  if (new_child->IsTableSection())
    return !IsTable();
  if (new_child->IsTableRow())
    return !IsTableSection();
  if (new_child->IsTableCell())
    return !IsTableRow();
  return false;
}

DISABLE_CFI_PERF
void LayoutObject::AddChild(LayoutObject* new_child,
                            LayoutObject* before_child) {
  DCHECK(IsAllowedToModifyLayoutTreeStructure(GetDocument()));

  LayoutObjectChildList* children = VirtualChildren();
  DCHECK(children);
  if (!children)
    return;

  if (RequiresAnonymousTableWrappers(new_child)) {
    // Generate an anonymous table or reuse existing one from previous child
    // Per: 17.2.1 Anonymous table objects 3. Generate missing parents
    // http://www.w3.org/TR/CSS21/tables.html#anonymous-boxes
    LayoutTable* table;
    LayoutObject* after_child =
        before_child ? before_child->PreviousSibling() : children->LastChild();
    if (after_child && after_child->IsAnonymous() && after_child->IsTable() &&
        !after_child->IsBeforeContent()) {
      table = ToLayoutTable(after_child);
    } else {
      table = LayoutTable::CreateAnonymousWithParent(this);
      children->InsertChildNode(this, table, before_child);
    }
    table->AddChild(new_child);
  } else {
    children->InsertChildNode(this, new_child, before_child);
  }

  if (new_child->IsText() &&
      new_child->Style()->TextTransform() == ETextTransform::kCapitalize)
    ToLayoutText(new_child)->TransformText();
}

void LayoutObject::RemoveChild(LayoutObject* old_child) {
  DCHECK(IsAllowedToModifyLayoutTreeStructure(GetDocument()));

  LayoutObjectChildList* children = VirtualChildren();
  DCHECK(children);
  if (!children)
    return;

  children->RemoveChildNode(this, old_child);
}

void LayoutObject::SetDangerousOneWayParent(LayoutObject* parent) {
  DCHECK(!PreviousSibling());
  DCHECK(!NextSibling());
  DCHECK(!parent || !parent_);
  SetParent(parent);
}

void LayoutObject::RegisterSubtreeChangeListenerOnDescendants(bool value) {
  // If we're set to the same value then we're done as that means it's
  // set down the tree that way already.
  if (bitfields_.SubtreeChangeListenerRegistered() == value)
    return;

  bitfields_.SetSubtreeChangeListenerRegistered(value);

  for (LayoutObject* curr = SlowFirstChild(); curr; curr = curr->NextSibling())
    curr->RegisterSubtreeChangeListenerOnDescendants(value);
}

void LayoutObject::NotifyAncestorsOfSubtreeChange() {
  if (bitfields_.NotifiedOfSubtreeChange())
    return;

  bitfields_.SetNotifiedOfSubtreeChange(true);
  if (Parent())
    Parent()->NotifyAncestorsOfSubtreeChange();
}

void LayoutObject::NotifyOfSubtreeChange() {
  if (!bitfields_.SubtreeChangeListenerRegistered())
    return;
  if (bitfields_.NotifiedOfSubtreeChange())
    return;

  NotifyAncestorsOfSubtreeChange();

  // We can modify the layout tree during layout which means that we may
  // try to schedule this during performLayout. This should no longer
  // happen when crbug.com/370457 is fixed.
  DeprecatedScheduleStyleRecalcDuringLayout marker(GetDocument().Lifecycle());
  GetDocument().ScheduleLayoutTreeUpdateIfNeeded();
}

void LayoutObject::HandleSubtreeModifications() {
  DCHECK(WasNotifiedOfSubtreeChange());
  DCHECK(GetDocument().Lifecycle().StateAllowsLayoutTreeNotifications());

  if (ConsumesSubtreeChangeNotification())
    SubtreeDidChange();

  bitfields_.SetNotifiedOfSubtreeChange(false);

  for (LayoutObject* object = SlowFirstChild(); object;
       object = object->NextSibling()) {
    if (!object->WasNotifiedOfSubtreeChange())
      continue;
    object->HandleSubtreeModifications();
  }
}

LayoutObject* LayoutObject::NextInPreOrder() const {
  if (LayoutObject* o = SlowFirstChild())
    return o;

  return NextInPreOrderAfterChildren();
}

bool LayoutObject::HasClipRelatedProperty() const {
  // TODO(trchen): Refactor / remove this function.
  // This function detects a bunch of properties that can potentially affect
  // clip inheritance chain. However such generalization is practially useless
  // because these properties change clip inheritance in different way that
  // needs to be handled explicitly.
  // CSS clip applies clip to the current element and all descendants.
  // CSS overflow clip applies only to containg-block descendants.
  // CSS contain:paint applies to all descendants by making itself a containing
  // block for all descendants.
  // CSS clip-path/mask/filter induces a stacking context and applies inherited
  // clip to that stacking context, while resetting clip for descendants. This
  // special behavior is already handled elsewhere.
  if (HasClip() || HasOverflowClip() || Style()->ContainsPaint())
    return true;
  if (IsBox() && ToLayoutBox(this)->HasControlClip())
    return true;
  return false;
}

LayoutObject* LayoutObject::NextInPreOrderAfterChildren() const {
  LayoutObject* o = NextSibling();
  if (!o) {
    o = Parent();
    while (o && !o->NextSibling())
      o = o->Parent();
    if (o)
      o = o->NextSibling();
  }

  return o;
}

LayoutObject* LayoutObject::NextInPreOrder(
    const LayoutObject* stay_within) const {
  if (LayoutObject* o = SlowFirstChild())
    return o;

  return NextInPreOrderAfterChildren(stay_within);
}

LayoutObject* LayoutObject::NextInPreOrderAfterChildren(
    const LayoutObject* stay_within) const {
  if (this == stay_within)
    return nullptr;

  const LayoutObject* current = this;
  LayoutObject* next = current->NextSibling();
  for (; !next; next = current->NextSibling()) {
    current = current->Parent();
    if (!current || current == stay_within)
      return nullptr;
  }
  return next;
}

LayoutObject* LayoutObject::PreviousInPreOrder() const {
  if (LayoutObject* o = PreviousSibling()) {
    while (LayoutObject* last_child = o->SlowLastChild())
      o = last_child;
    return o;
  }

  return Parent();
}

LayoutObject* LayoutObject::PreviousInPreOrder(
    const LayoutObject* stay_within) const {
  if (this == stay_within)
    return nullptr;

  return PreviousInPreOrder();
}

LayoutObject* LayoutObject::ChildAt(unsigned index) const {
  LayoutObject* child = SlowFirstChild();
  for (unsigned i = 0; child && i < index; i++)
    child = child->NextSibling();
  return child;
}

LayoutObject* LayoutObject::LastLeafChild() const {
  LayoutObject* r = SlowLastChild();
  while (r) {
    LayoutObject* n = nullptr;
    n = r->SlowLastChild();
    if (!n)
      break;
    r = n;
  }
  return r;
}

static void AddLayers(LayoutObject* obj,
                      PaintLayer* parent_layer,
                      LayoutObject*& new_object,
                      PaintLayer*& before_child) {
  if (obj->HasLayer()) {
    if (!before_child && new_object) {
      // We need to figure out the layer that follows newObject. We only do
      // this the first time we find a child layer, and then we update the
      // pointer values for newObject and beforeChild used by everyone else.
      before_child =
          new_object->Parent()->FindNextLayer(parent_layer, new_object);
      new_object = nullptr;
    }
    parent_layer->AddChild(ToLayoutBoxModelObject(obj)->Layer(), before_child);
    return;
  }

  for (LayoutObject* curr = obj->SlowFirstChild(); curr;
       curr = curr->NextSibling())
    AddLayers(curr, parent_layer, new_object, before_child);
}

void LayoutObject::AddLayers(PaintLayer* parent_layer) {
  if (!parent_layer)
    return;

  LayoutObject* object = this;
  PaintLayer* before_child = nullptr;
  blink::AddLayers(this, parent_layer, object, before_child);
}

void LayoutObject::RemoveLayers(PaintLayer* parent_layer) {
  if (!parent_layer)
    return;

  if (HasLayer()) {
    parent_layer->RemoveChild(ToLayoutBoxModelObject(this)->Layer());
    return;
  }

  for (LayoutObject* curr = SlowFirstChild(); curr; curr = curr->NextSibling())
    curr->RemoveLayers(parent_layer);
}

void LayoutObject::MoveLayers(PaintLayer* old_parent, PaintLayer* new_parent) {
  if (!new_parent)
    return;

  if (HasLayer()) {
    PaintLayer* layer = ToLayoutBoxModelObject(this)->Layer();
    DCHECK_EQ(old_parent, layer->Parent());
    if (old_parent)
      old_parent->RemoveChild(layer);
    new_parent->AddChild(layer);
    return;
  }

  for (LayoutObject* curr = SlowFirstChild(); curr; curr = curr->NextSibling())
    curr->MoveLayers(old_parent, new_parent);
}

PaintLayer* LayoutObject::FindNextLayer(PaintLayer* parent_layer,
                                        LayoutObject* start_point,
                                        bool check_parent) {
  // Error check the parent layer passed in. If it's null, we can't find
  // anything.
  if (!parent_layer)
    return nullptr;

  // Step 1: If our layer is a child of the desired parent, then return our
  // layer.
  PaintLayer* our_layer =
      HasLayer() ? ToLayoutBoxModelObject(this)->Layer() : nullptr;
  if (our_layer && our_layer->Parent() == parent_layer)
    return our_layer;

  // Step 2: If we don't have a layer, or our layer is the desired parent, then
  // descend into our siblings trying to find the next layer whose parent is the
  // desired parent.
  if (!our_layer || our_layer == parent_layer) {
    for (LayoutObject* curr = start_point ? start_point->NextSibling()
                                          : SlowFirstChild();
         curr; curr = curr->NextSibling()) {
      PaintLayer* next_layer =
          curr->FindNextLayer(parent_layer, nullptr, false);
      if (next_layer)
        return next_layer;
    }
  }

  // Step 3: If our layer is the desired parent layer, then we're finished. We
  // didn't find anything.
  if (parent_layer == our_layer)
    return nullptr;

  // Step 4: If |checkParent| is set, climb up to our parent and check its
  // siblings that follow us to see if we can locate a layer.
  if (check_parent && Parent())
    return Parent()->FindNextLayer(parent_layer, this, true);

  return nullptr;
}

PaintLayer* LayoutObject::EnclosingLayer() const {
  for (const LayoutObject* current = this; current;
       current = current->Parent()) {
    if (current->HasLayer())
      return ToLayoutBoxModelObject(current)->Layer();
  }
  // TODO(crbug.com/365897): we should get rid of detached layout subtrees, at
  // which point this code should not be reached.
  return nullptr;
}

PaintLayer* LayoutObject::PaintingLayer() const {
  for (const LayoutObject* current = this; current;
       // Use containingBlock instead of parentCrossingFrames for floating
       // objects to omit any self-painting layers of inline objects that don't
       // paint the floating object.
       current = current->IsFloating() ? current->ContainingBlock()
                                       : current->ParentCrossingFrames()) {
    if (current->HasLayer() &&
        ToLayoutBoxModelObject(current)->Layer()->IsSelfPaintingLayer()) {
      return ToLayoutBoxModelObject(current)->Layer();
    } else if (current->IsColumnSpanAll()) {
      // Column spanners paint through their multicolumn containers which can
      // be accessed through the associated out-of-flow placeholder's parent.
      current = current->SpannerPlaceholder();
    }
  }
  // TODO(crbug.com/365897): we should get rid of detached layout subtrees, at
  // which point this code should not be reached.
  return nullptr;
}

bool LayoutObject::IsFixedPositionObjectInPagedMedia() const {
  if (StyleRef().GetPosition() != EPosition::kFixed)
    return false;
  LayoutView* view = View();
  return Container() == view && view->PageLogicalHeight() &&
         // TODO(crbug.com/619094): Figure out the correct behaviour for fixed
         // position objects in paged media with vertical writing modes.
         view->IsHorizontalWritingMode();
}

LayoutRect LayoutObject::ScrollRectToVisible(
    const LayoutRect& rect,
    const WebScrollIntoViewParams& params) {
  LayoutBox* enclosing_box = EnclosingBox();
  if (!enclosing_box)
    return rect;

  GetDocument().GetPage()->GetSmoothScrollSequencer()->AbortAnimations();
  WebScrollIntoViewParams new_params(params);
  new_params.is_for_scroll_sequence |=
      params.GetScrollType() == kProgrammaticScroll;
  LayoutRect new_location =
      enclosing_box->ScrollRectToVisibleRecursive(rect, new_params);
  GetDocument().GetPage()->GetSmoothScrollSequencer()->RunQueuedAnimations();

  return new_location;
}

LayoutBox* LayoutObject::EnclosingBox() const {
  LayoutObject* curr = const_cast<LayoutObject*>(this);
  while (curr) {
    if (curr->IsBox())
      return ToLayoutBox(curr);
    curr = curr->Parent();
  }

  NOTREACHED();
  return nullptr;
}

LayoutBoxModelObject* LayoutObject::EnclosingBoxModelObject() const {
  LayoutObject* curr = const_cast<LayoutObject*>(this);
  while (curr) {
    if (curr->IsBoxModelObject())
      return ToLayoutBoxModelObject(curr);
    curr = curr->Parent();
  }

  NOTREACHED();
  return nullptr;
}

LayoutBlockFlow* LayoutObject::EnclosingNGBlockFlow() const {
  if (!RuntimeEnabledFeatures::LayoutNGEnabled())
    return nullptr;
  LayoutBox* box = EnclosingBox();
  DCHECK(box);
  return NGBlockNode::CanUseNewLayout(*box) ? ToLayoutBlockFlow(box) : nullptr;
}

const NGPhysicalBoxFragment* LayoutObject::EnclosingBlockFlowFragment() const {
  DCHECK(IsInline() || IsText());
  LayoutBlockFlow* const block_flow = EnclosingNGBlockFlow();
  if (!block_flow || !block_flow->ChildrenInline())
    return nullptr;
  // TODO(kojii): CurrentFragment isn't always available after layout clean.
  // Investigate why.
  return block_flow->CurrentFragment();
}

LayoutBox* LayoutObject::EnclosingScrollableBox() const {
  for (LayoutObject* ancestor = Parent(); ancestor;
       ancestor = ancestor->Parent()) {
    if (!ancestor->IsBox())
      continue;

    LayoutBox* ancestor_box = ToLayoutBox(ancestor);
    if (ancestor_box->CanBeScrolledAndHasScrollableArea())
      return ancestor_box;
  }

  return nullptr;
}

LayoutFlowThread* LayoutObject::LocateFlowThreadContainingBlock() const {
  DCHECK(IsInsideFlowThread());

  // See if we have the thread cached because we're in the middle of layout.
  if (LayoutView* view = View()) {
    if (LayoutState* layout_state = view->GetLayoutState()) {
      // TODO(mstensho): We should really just return whatever
      // layoutState->flowThread() returns here, also if the value is nullptr.
      if (LayoutFlowThread* flow_thread = layout_state->FlowThread())
        return flow_thread;
    }
  }

  // Not in the middle of layout so have to find the thread the slow way.
  return LayoutFlowThread::LocateFlowThreadContainingBlockOf(
      *this, LayoutFlowThread::kAnyAncestor);
}

static inline bool ObjectIsRelayoutBoundary(const LayoutObject* object) {
  // FIXME: In future it may be possible to broaden these conditions in order to
  // improve performance.
  if (object->IsTextControl())
    return true;

  if (object->IsSVGRoot())
    return true;

  // LayoutInline can't be relayout roots since LayoutBlockFlow is responsible
  // for layouting them.
  if (object->IsLayoutInline())
    return false;

  // Table parts can't be relayout roots since the table is responsible for
  // layouting all the parts.
  if (object->IsTablePart())
    return false;

  const ComputedStyle* style = object->Style();
  if (style->ContainsLayout() && style->ContainsSize())
    return true;

  if (!object->HasOverflowClip())
    return false;

  // If either dimension is percent-based, intrinsic, or anything but fixed,
  // this object cannot form a re-layout boundary. A non-fixed computed logical
  // height will allow the object to grow and shrink based on the content
  // inside. The same goes for for logical width, if this objects is inside a
  // shrink-to-fit container, for instance.
  if (!style->Width().IsFixed() || !style->Height().IsFixed())
    return false;

  // Scrollbar parts can be removed during layout. Avoid the complexity of
  // having to deal with that.
  if (object->IsLayoutScrollbarPart())
    return false;

  // In general we can't relayout a flex item independently of its container;
  // not only is the result incorrect due to the override size that's set, it
  // also messes with the cached main size on the flexbox.
  if (object->IsBox() && ToLayoutBox(object)->IsFlexItem())
    return false;

  // Inside multicol it's generally problematic to allow relayout roots. The
  // multicol container itself may be scheduled for relayout as well (due to
  // other changes that may have happened since the previous layout pass),
  // which might affect the column heights, which may affect how this object
  // breaks across columns). Spanners may also have been added or removed since
  // the previous layout pass, which is just another way of affecting the column
  // heights (and the number of rows). Instead of identifying cases where it's
  // safe to allow relayout roots, just disallow them inside multicol.
  if (object->IsInsideFlowThread())
    return false;

  return true;
}

// NGInlineNode::ColectInlines() collects inline children into NGInlineItem.
// This function marks NeedsCollectInlines() to let it re-collect.
void LayoutObject::MarkContainerNeedsCollectInlines() {
  if (!RuntimeEnabledFeatures::LayoutNGEnabled())
    return;

  // Mark only if this is a LayoutObject collected by CollectInlines().
  if (!IsInline() && !IsFloatingOrOutOfFlowPositioned()) {
    // If this is the container box of inline children, mark it.
    if (IsLayoutBlockFlow())
      SetNeedsCollectInlines(true);
    return;
  }

  for (LayoutObject* object = this; !object->NeedsCollectInlines();) {
    object->SetNeedsCollectInlines(true);
    object = object->Parent();
    if (!object || object->IsLayoutBlockFlow())
      break;
  }
}

void LayoutObject::MarkContainerChainForLayout(bool schedule_relayout,
                                               SubtreeLayoutScope* layouter) {
#if DCHECK_IS_ON()
  DCHECK(!IsSetNeedsLayoutForbidden());
#endif
  DCHECK(!layouter || this != layouter->Root());
  // When we're in layout, we're marking a descendant as needing layout with
  // the intention of visiting it during this layout. We shouldn't be
  // scheduling it to be laid out later. Also, scheduleRelayout() must not be
  // called while iterating LocalFrameView::layout_subtree_root_list_.
  schedule_relayout &= !GetFrameView()->IsInPerformLayout();

  LayoutObject* object = Container();
  LayoutObject* last = this;

  bool simplified_normal_flow_layout = NeedsSimplifiedNormalFlowLayout() &&
                                       !SelfNeedsLayout() &&
                                       !NormalChildNeedsLayout();

  // We need to set NeedsCollectInlines() only if LayoutNGEnabled, but setting a
  // flag in non-LayoutNG is harmless.
  // When we set a flag, setting another flag should be zero-cost.
  if (object)
    object->SetNeedsCollectInlines(true);

  while (object) {
    if (object->SelfNeedsLayout())
      return;

    // Don't mark the outermost object of an unrooted subtree. That object will
    // be marked when the subtree is added to the document.
    LayoutObject* container = object->Container();
    if (!container && !object->IsLayoutView())
      return;
    if (!last->IsTextOrSVGChild() && last->Style()->HasOutOfFlowPosition()) {
      object = last->ContainingBlock();
      if (object->PosChildNeedsLayout())
        return;
      container = object->Container();
      object->SetPosChildNeedsLayout(true);
      object->SetNeedsCollectInlines(true);
      simplified_normal_flow_layout = true;
    } else if (simplified_normal_flow_layout) {
      if (object->NeedsSimplifiedNormalFlowLayout())
        return;
      object->SetNeedsSimplifiedNormalFlowLayout(true);
      object->SetNeedsCollectInlines(true);
    } else {
      if (object->NormalChildNeedsLayout())
        return;
      object->SetNormalChildNeedsLayout(true);
      object->SetNeedsCollectInlines(true);
    }
#if DCHECK_IS_ON()
    DCHECK(!object->IsSetNeedsLayoutForbidden());
#endif

    if (layouter) {
      layouter->RecordObjectMarkedForLayout(object);
      if (object == layouter->Root())
        return;
    }

    last = object;
    if (schedule_relayout && ObjectIsRelayoutBoundary(last))
      break;
    object = container;
  }

  if (schedule_relayout)
    last->ScheduleRelayout();
}

#if DCHECK_IS_ON()
void LayoutObject::CheckBlockPositionedObjectsNeedLayout() {
  DCHECK(!NeedsLayout());

  if (IsLayoutBlock())
    ToLayoutBlock(this)->CheckPositionedObjectsNeedLayout();
}
#endif

void LayoutObject::SetPreferredLogicalWidthsDirty(
    MarkingBehavior mark_parents) {
  bitfields_.SetPreferredLogicalWidthsDirty(true);
  if (mark_parents == kMarkContainerChain &&
      (IsText() || !Style()->HasOutOfFlowPosition()))
    InvalidateContainerPreferredLogicalWidths();
}

void LayoutObject::ClearPreferredLogicalWidthsDirty() {
  bitfields_.SetPreferredLogicalWidthsDirty(false);
}

inline void LayoutObject::InvalidateContainerPreferredLogicalWidths() {
  // In order to avoid pathological behavior when inlines are deeply nested, we
  // do include them in the chain that we mark dirty (even though they're kind
  // of irrelevant).
  LayoutObject* o = IsTableCell() ? ContainingBlock() : Container();
  while (o && !o->PreferredLogicalWidthsDirty()) {
    // Don't invalidate the outermost object of an unrooted subtree. That object
    // will be invalidated when the subtree is added to the document.
    LayoutObject* container =
        o->IsTableCell() ? o->ContainingBlock() : o->Container();
    if (!container && !o->IsLayoutView())
      break;

    o->bitfields_.SetPreferredLogicalWidthsDirty(true);
    // A positioned object has no effect on the min/max width of its containing
    // block ever. We can optimize this case and not go up any further.
    if (o->Style()->HasOutOfFlowPosition())
      break;
    o = container;
  }
}

LayoutObject* LayoutObject::ContainerForAbsolutePosition(
    AncestorSkipInfo* skip_info) const {
  return FindAncestorByPredicate(this, skip_info, [](LayoutObject* candidate) {
    if (!candidate->CanContainAbsolutePositionObjects() &&
        candidate->StyleRef().ContainsLayout()) {
      UseCounter::Count(candidate->GetDocument(),
                        WebFeature::kCSSContainLayoutPositionedDescendants);
    }
    return candidate->CanContainAbsolutePositionObjects();
  });
}

LayoutObject* LayoutObject::ContainerForFixedPosition(
    AncestorSkipInfo* skip_info) const {
  DCHECK(!IsText());
  return FindAncestorByPredicate(this, skip_info, [](LayoutObject* candidate) {
    if (!candidate->CanContainFixedPositionObjects() &&
        candidate->StyleRef().ContainsLayout()) {
      UseCounter::Count(candidate->GetDocument(),
                        WebFeature::kCSSContainLayoutPositionedDescendants);
    }
    return candidate->CanContainFixedPositionObjects();
  });
}

LayoutBlock* LayoutObject::ContainingBlockForAbsolutePosition(
    AncestorSkipInfo* skip_info) const {
  auto* container = ContainerForAbsolutePosition(skip_info);
  return FindContainingBlock(container, skip_info);
}

LayoutBlock* LayoutObject::ContainingBlockForFixedPosition(
    AncestorSkipInfo* skip_info) const {
  auto* container = ContainerForFixedPosition(skip_info);
  return FindContainingBlock(container, skip_info);
}

const LayoutBlock* LayoutObject::InclusiveContainingBlock() const {
  if (IsLayoutBlock())
    return ToLayoutBlock(this);
  return ContainingBlock();
}

LayoutBlock* LayoutObject::ContainingBlock(AncestorSkipInfo* skip_info) const {
  LayoutObject* object = Parent();
  if (!object && IsLayoutScrollbarPart())
    object = ToLayoutScrollbarPart(this)->ScrollbarStyleSource();
  if (!IsTextOrSVGChild()) {
    if (style_->GetPosition() == EPosition::kFixed)
      return ContainingBlockForFixedPosition(skip_info);
    if (style_->GetPosition() == EPosition::kAbsolute)
      return ContainingBlockForAbsolutePosition(skip_info);
  }
  if (IsColumnSpanAll()) {
    object = SpannerPlaceholder()->ContainingBlock();
  } else {
    while (object && ((object->IsInline() && !object->IsAtomicInlineLevel()) ||
                      !object->IsLayoutBlock())) {
      if (skip_info)
        skip_info->Update(*object);
      object = object->Parent();
    }
  }

  if (!object || !object->IsLayoutBlock())
    return nullptr;  // This can still happen in case of an orphaned tree

  return ToLayoutBlock(object);
}

FloatRect LayoutObject::AbsoluteBoundingBoxFloatRect() const {
  Vector<FloatQuad> quads;
  AbsoluteQuads(quads);

  size_t n = quads.size();
  if (n == 0)
    return FloatRect();

  FloatRect result = quads[0].BoundingBox();
  for (size_t i = 1; i < n; ++i)
    result.Unite(quads[i].BoundingBox());
  return result;
}

IntRect LayoutObject::AbsoluteBoundingBoxRect(MapCoordinatesFlags flags) const {
  Vector<FloatQuad> quads;
  AbsoluteQuads(quads, flags);

  size_t n = quads.size();
  if (!n)
    return IntRect();

  IntRect result = quads[0].EnclosingBoundingBox();
  for (size_t i = 1; i < n; ++i)
    result.Unite(quads[i].EnclosingBoundingBox());
  return result;
}

IntRect LayoutObject::AbsoluteBoundingBoxRectIgnoringTransforms() const {
  FloatPoint abs_pos = LocalToAbsolute();
  Vector<IntRect> rects;
  AbsoluteRects(rects, FlooredLayoutPoint(abs_pos));

  size_t n = rects.size();
  if (!n)
    return IntRect();

  IntRect result = rects[0];
  for (size_t i = 1; i < n; ++i)
    result.Unite(rects[i]);
  return result;
}

LayoutRect LayoutObject::AbsoluteBoundingBoxRectHandlingEmptyAnchor() const {
  return AbsoluteBoundingBoxRectHelper(ExpandScrollMargin::kIgnore);
}

LayoutRect LayoutObject::AbsoluteBoundingBoxRectForScrollIntoView() const {
  return AbsoluteBoundingBoxRectHelper(ExpandScrollMargin::kExpand);
}

LayoutRect LayoutObject::AbsoluteBoundingBoxRectHelper(
    ExpandScrollMargin expand) const {
  FloatPoint upper_left, lower_right;
  bool found_upper_left = GetUpperLeftCorner(expand, upper_left);
  bool found_lower_right = GetLowerRightCorner(expand, lower_right);

  // If we've found one corner, but not the other,
  // then we should just return a point at the corner that we did find.
  if (found_upper_left != found_lower_right) {
    if (found_upper_left)
      lower_right = upper_left;
    else
      upper_left = lower_right;
  }

  FloatSize size = lower_right.ExpandedTo(upper_left) - upper_left;
  if (std::isnan(size.Width()) || std::isnan(size.Height()))
    return LayoutRect();

  return EnclosingLayoutRect(FloatRect(upper_left, size));
}

namespace {

enum class MarginCorner { kTopLeft, kBottomRight };

void MovePointByScrollMargin(const LayoutObject* layout_object,
                             MarginCorner corner,
                             FloatPoint& point) {
  FloatSize offset;
  const ComputedStyle* style = layout_object->Style();

  if (corner == MarginCorner::kTopLeft)
    offset = FloatSize(-style->ScrollMarginLeft(), -style->ScrollMarginTop());
  else
    offset = FloatSize(style->ScrollMarginRight(), style->ScrollMarginBottom());

  point.Move(offset);
}

inline const LayoutObject* EndOfContinuations(
    const LayoutObject* layout_object) {
  const LayoutObject* prev = nullptr;
  const LayoutObject* cur = layout_object;

  if (!cur->IsLayoutInline() && !cur->IsLayoutBlockFlow())
    return nullptr;

  while (cur) {
    prev = cur;
    if (cur->IsLayoutInline())
      cur = ToLayoutInline(cur)->Continuation();
    else
      cur = ToLayoutBlockFlow(cur)->Continuation();
  }

  return prev;
}

}  // namespace

bool LayoutObject::GetUpperLeftCorner(ExpandScrollMargin expand,
                                      FloatPoint& point) const {
  if (!IsInline() || IsAtomicInlineLevel()) {
    point = LocalToAbsolute(FloatPoint(), kUseTransforms);
    if (expand == ExpandScrollMargin::kExpand)
      MovePointByScrollMargin(this, MarginCorner::kTopLeft, point);
    return true;
  }

  // Find the next text/image child, to get a position.
  const LayoutObject* runner = this;
  while (runner) {
    const LayoutObject* const previous = runner;
    if (LayoutObject* runner_first_child = runner->SlowFirstChild()) {
      runner = runner_first_child;
    } else if (runner->NextSibling()) {
      runner = runner->NextSibling();
    } else {
      LayoutObject* next = nullptr;
      while (!next && runner->Parent()) {
        runner = runner->Parent();
        next = runner->NextSibling();
      }
      runner = next;

      if (!runner)
        break;
    }
    DCHECK(runner);

    if (!runner->IsInline() || runner->IsAtomicInlineLevel()) {
      point = runner->LocalToAbsolute(FloatPoint(), kUseTransforms);
      if (expand == ExpandScrollMargin::kExpand)
        MovePointByScrollMargin(runner, MarginCorner::kTopLeft, point);
      return true;
    }

    if (runner->IsText() && !runner->IsBR()) {
      const base::Optional<FloatPoint> maybe_point =
          ToLayoutText(runner)->GetUpperLeftCorner();
      if (maybe_point.has_value()) {
        point = runner->LocalToAbsolute(maybe_point.value(), kUseTransforms);
        return true;
      }
      if (previous->GetNode() == GetNode()) {
        // Do nothing - skip unrendered whitespace that is a child or next
        // sibling of the anchor.
        // FIXME: This fails to skip a whitespace sibling when there was also a
        // whitespace child (because |previous| has moved).
        continue;
      }
      point = runner->LocalToAbsolute(FloatPoint(), kUseTransforms);
      if (expand == ExpandScrollMargin::kExpand)
        MovePointByScrollMargin(runner, MarginCorner::kTopLeft, point);
      return true;
    }

    if (runner->IsAtomicInlineLevel()) {
      DCHECK(runner->IsBox());
      const LayoutBox* box = ToLayoutBox(runner);
      point = FloatPoint(box->Location());
      point = runner->Container()->LocalToAbsolute(point, kUseTransforms);
      if (expand == ExpandScrollMargin::kExpand)
        MovePointByScrollMargin(box, MarginCorner::kTopLeft, point);
      return true;
    }
  }

  // If the target doesn't have any children or siblings that could be used to
  // calculate the scroll position, we must be at the end of the
  // document. Scroll to the bottom.
  // FIXME: who said anything about scrolling?
  if (!runner && GetDocument().View()) {
    point = FloatPoint(0, GetDocument()
                              .View()
                              ->LayoutViewportScrollableArea()
                              ->ContentsSize()
                              .Height());
    return true;
  }
  return false;
}

bool LayoutObject::GetLowerRightCorner(ExpandScrollMargin expand,
                                       FloatPoint& point) const {
  if (!IsInline() || IsAtomicInlineLevel()) {
    const LayoutBox* box = ToLayoutBox(this);
    point = LocalToAbsolute(FloatPoint(box->Size()), kUseTransforms);
    if (expand == ExpandScrollMargin::kExpand)
      MovePointByScrollMargin(this, MarginCorner::kBottomRight, point);
    return true;
  }

  const LayoutObject* runner = this;
  const LayoutObject* start_continuation = nullptr;
  // Find the last text/image child, to get a position.
  while (runner) {
    if (LayoutObject* runner_last_child = runner->SlowLastChild()) {
      runner = runner_last_child;
    } else if (runner != this && runner->PreviousSibling()) {
      runner = runner->PreviousSibling();
    } else {
      const LayoutObject* prev = nullptr;
      while (!prev) {
        // Check if the current layoutObject has contiunation and move the
        // location for finding the layoutObject to the end of continuations if
        // there is the continuation.  Skip to check the contiunation on
        // contiunations section
        if (start_continuation == runner) {
          start_continuation = nullptr;
        } else if (!start_continuation) {
          if (const LayoutObject* continuation = EndOfContinuations(runner)) {
            start_continuation = runner;
            prev = continuation;
            break;
          }
        }
        // Prevent to overrun out of own layout tree
        if (runner == this) {
          return false;
        }
        runner = runner->Parent();
        if (!runner)
          return false;
        prev = runner->PreviousSibling();
      }
      runner = prev;
    }
    DCHECK(runner);
    if (runner->IsText() || runner->IsAtomicInlineLevel()) {
      point = FloatPoint();
      if (runner->IsText()) {
        const LayoutText* text = ToLayoutText(runner);
        IntRect lines_box = EnclosingIntRect(text->LinesBoundingBox());
        if (!lines_box.MaxX() && !lines_box.MaxY())
          continue;
        point.MoveBy(lines_box.MaxXMaxYCorner());
        point = runner->LocalToAbsolute(point, kUseTransforms);
      } else {
        const LayoutBox* box = ToLayoutBox(runner);
        point.MoveBy(box->FrameRect().MaxXMaxYCorner());
        point = runner->Container()->LocalToAbsolute(point, kUseTransforms);
        if (expand == ExpandScrollMargin::kExpand)
          MovePointByScrollMargin(box, MarginCorner::kBottomRight, point);
      }
      return true;
    }
  }
  return true;
}

IntRect LayoutObject::AbsoluteElementBoundingBoxRect() const {
  Vector<LayoutRect> rects;
  const LayoutBoxModelObject& container = EnclosingLayer()->GetLayoutObject();
  AddElementVisualOverflowRects(
      rects, LayoutPoint(LocalToAncestorPoint(FloatPoint(), &container)));
  return container.LocalToAbsoluteQuad(FloatQuad(FloatRect(UnionRect(rects))))
      .EnclosingBoundingBox();
}

FloatRect LayoutObject::AbsoluteBoundingBoxRectForRange(
    const EphemeralRange& range) {
  if (range.IsNull() || !range.StartPosition().ComputeContainerNode())
    return FloatRect();

  range.GetDocument().UpdateStyleAndLayout();

  return ComputeTextFloatRect(range);
}

void LayoutObject::AddAbsoluteRectForLayer(IntRect& result) {
  if (HasLayer())
    result.Unite(AbsoluteBoundingBoxRect());
  for (LayoutObject* current = SlowFirstChild(); current;
       current = current->NextSibling())
    current->AddAbsoluteRectForLayer(result);
}

IntRect LayoutObject::AbsoluteBoundingBoxRectIncludingDescendants() const {
  IntRect result = AbsoluteBoundingBoxRect();
  for (LayoutObject* current = SlowFirstChild(); current;
       current = current->NextSibling())
    current->AddAbsoluteRectForLayer(result);
  return result;
}

void LayoutObject::Paint(const PaintInfo&, const LayoutPoint&) const {}

const LayoutBoxModelObject& LayoutObject::ContainerForPaintInvalidation()
    const {
  CHECK(IsRooted());

  if (const LayoutBoxModelObject* paint_invalidation_container =
          EnclosingCompositedContainer())
    return *paint_invalidation_container;

  // If the current frame is not composited, we send just return the main
  // frame's LayoutView so that we generate invalidations on the window.
  const LayoutView* layout_view = View();
  while (const LayoutObject* owner_object =
             layout_view->GetFrame()->OwnerLayoutObject())
    layout_view = owner_object->View();

  DCHECK(layout_view);
  return *layout_view;
}

bool LayoutObject::RecalcOverflowAfterStyleChange() {
  if (!ChildNeedsOverflowRecalcAfterStyleChange())
    return false;
  bool children_overflow_changed = false;
  for (LayoutObject* current = SlowFirstChild(); current;
       current = current->NextSibling()) {
    if (current->RecalcOverflowAfterStyleChange())
      children_overflow_changed = true;
  }
  return children_overflow_changed;
}

const LayoutBoxModelObject* LayoutObject::EnclosingCompositedContainer() const {
  LayoutBoxModelObject* container = nullptr;
  // FIXME: CompositingState is not necessarily up to date for many callers of
  // this function.
  DisableCompositingQueryAsserts disabler;

  if (PaintLayer* painting_layer = PaintingLayer()) {
    if (PaintLayer* compositing_layer =
            painting_layer
                ->EnclosingLayerForPaintInvalidationCrossingFrameBoundaries())
      container = &compositing_layer->GetLayoutObject();
  }
  return container;
}

String LayoutObject::DecoratedName() const {
  StringBuilder name;
  name.Append(GetName());

  if (IsAnonymous())
    name.Append(" (anonymous)");
  // FIXME: Remove the special case for LayoutView here (requires rebaseline of
  // all tests).
  if (IsOutOfFlowPositioned() && !IsLayoutView())
    name.Append(" (positioned)");
  if (IsRelPositioned())
    name.Append(" (relative positioned)");
  if (IsStickyPositioned())
    name.Append(" (sticky positioned)");
  if (IsFloating())
    name.Append(" (floating)");
  if (SpannerPlaceholder())
    name.Append(" (column spanner)");

  return name.ToString();
}

String LayoutObject::DebugName() const {
  StringBuilder name;
  name.Append(DecoratedName());

  if (const Node* node = GetNode()) {
    name.Append(' ');
    name.Append(node->DebugName());
  }
  return name.ToString();
}

LayoutRect LayoutObject::FragmentsVisualRectBoundingBox() const {
  if (!fragment_.NextFragment())
    return fragment_.VisualRect();
  LayoutRect visual_rect;
  for (auto* fragment = &fragment_; fragment;
       fragment = fragment->NextFragment())
    visual_rect.Unite(fragment->VisualRect());
  return visual_rect;
}

LayoutRect LayoutObject::VisualRect() const {
  return FragmentsVisualRectBoundingBox();
}

bool LayoutObject::IsPaintInvalidationContainer() const {
  return HasLayer() &&
         ToLayoutBoxModelObject(this)->Layer()->IsPaintInvalidationContainer();
}

void LayoutObject::InvalidateDisplayItemClients(
    PaintInvalidationReason reason) const {
  // This default implementation invalidates only the object itself as a
  // DisplayItemClient.
  ObjectPaintInvalidator(*this).InvalidateDisplayItemClient(*this, reason);
}

bool LayoutObject::CompositedScrollsWithRespectTo(
    const LayoutBoxModelObject& paint_invalidation_container) const {
  return paint_invalidation_container.UsesCompositedScrolling() &&
         this != &paint_invalidation_container;
}

IntSize LayoutObject::ScrollAdjustmentForPaintInvalidation(
    const LayoutBoxModelObject& paint_invalidation_container) const {
  // Non-composited scrolling should be included in the bounds of scrolled
  // items. Since mapToVisualRectInAncestorSpace does not include scrolling of
  // the ancestor, we need to add it back in after.
  if (paint_invalidation_container.IsBox() &&
      !paint_invalidation_container.UsesCompositedScrolling() &&
      this != &paint_invalidation_container) {
    const LayoutBox* box = ToLayoutBox(&paint_invalidation_container);
    if (box->HasOverflowClip())
      return -box->ScrolledContentOffset();
  }
  return IntSize();
}

void LayoutObject::InvalidatePaintRectangle(const LayoutRect& dirty_rect) {
  if (dirty_rect.IsEmpty())
    return;

  SetPartialInvalidationRect(UnionRect(dirty_rect, PartialInvalidationRect()));

  // Not using the WithoutGeometryChange version because we need to map the
  // partial invalidated rect to visual rect in backing or the containing
  // transform node.
  SetMayNeedPaintInvalidation();
}

LayoutRect LayoutObject::AbsoluteSelectionRect() const {
  LayoutRect selection_rect = LocalSelectionRect();
  if (!selection_rect.IsEmpty())
    MapToVisualRectInAncestorSpace(View(), selection_rect);

  if (LocalFrameView* frame_view = GetFrameView())
    selection_rect = frame_view->DocumentToAbsolute(selection_rect);

  return selection_rect;
}

DISABLE_CFI_PERF
PaintInvalidationReason LayoutObject::InvalidatePaint(
    const PaintInvalidatorContext& context) const {
  return ObjectPaintInvalidatorWithContext(*this, context).InvalidatePaint();
}

void LayoutObject::AdjustVisualRectForCompositedScrolling(
    LayoutRect& rect,
    const LayoutBoxModelObject& paint_invalidation_container) const {
  if (CompositedScrollsWithRespectTo(paint_invalidation_container)) {
    LayoutSize offset(
        -ToLayoutBox(&paint_invalidation_container)->ScrolledContentOffset());
    rect.Move(offset);
  }
}

LayoutRect LayoutObject::VisualRectIncludingCompositedScrolling(
    const LayoutBoxModelObject& paint_invalidation_container) const {
  LayoutRect rect = VisualRect();
  AdjustVisualRectForCompositedScrolling(rect, paint_invalidation_container);
  return rect;
}

void LayoutObject::ClearPreviousVisualRects() {
  fragment_.SetVisualRect(LayoutRect());
  fragment_.SetLocationInBacking(LayoutPoint());
  fragment_.SetSelectionVisualRect(LayoutRect());

  // Ensure check paint invalidation of subtree that would be triggered by
  // location change if we had valid previous location.
  SetMayNeedPaintInvalidationSubtree();
  // After clearing ("invalidating") the visual rects, mark this object as
  // needing to re-compute them.
  SetShouldDoFullPaintInvalidation();
}

LayoutRect LayoutObject::AbsoluteVisualRect() const {
  LayoutRect rect = LocalVisualRect();
  MapToVisualRectInAncestorSpace(View(), rect);
  return rect;
}

LayoutRect LayoutObject::LocalVisualRectIgnoringVisibility() const {
  NOTREACHED();
  return LayoutRect();
}

bool LayoutObject::MapToVisualRectInAncestorSpaceInternalFastPath(
    const LayoutBoxModelObject* ancestor,
    LayoutRect& rect,
    VisualRectFlags visual_rect_flags,
    bool& intersects) const {
  if (!(visual_rect_flags & kUseGeometryMapper) ||
      !RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
      !FirstFragment().HasLocalBorderBoxProperties() || !ancestor ||
      !ancestor->FirstFragment().HasLocalBorderBoxProperties()) {
    intersects = true;
    return false;
  }

  if (ancestor == this) {
    intersects = true;
    return true;
  }

  rect.MoveBy(FirstFragment().PaintOffset());
  FloatClipRect clip_rect((FloatRect(rect)));
  intersects = GeometryMapper::LocalToAncestorVisualRect(
      FirstFragment().LocalBorderBoxProperties(),
      ancestor->FirstFragment().ContentsProperties(), clip_rect,
      kIgnorePlatformOverlayScrollbarSize,
      (visual_rect_flags & kEdgeInclusive) ? kInclusiveIntersect
                                           : kNonInclusiveIntersect);

  rect = LayoutRect(clip_rect.Rect());
  rect.MoveBy(-ancestor->FirstFragment().PaintOffset());

  return true;
}

bool LayoutObject::MapToVisualRectInAncestorSpace(
    const LayoutBoxModelObject* ancestor,
    LayoutRect& rect,
    VisualRectFlags visual_rect_flags) const {
  bool intersects = true;
  if (MapToVisualRectInAncestorSpaceInternalFastPath(
          ancestor, rect, visual_rect_flags, intersects))
    return intersects;

  TransformState transform_state(TransformState::kApplyTransformDirection,
                                 FloatQuad(FloatRect(rect)));
  intersects = MapToVisualRectInAncestorSpaceInternal(ancestor, transform_state,
                                                      visual_rect_flags);
  transform_state.Flatten();
  rect = LayoutRect(transform_state.LastPlanarQuad().BoundingBox());
  return intersects;
}

bool LayoutObject::MapToVisualRectInAncestorSpaceInternal(
    const LayoutBoxModelObject* ancestor,
    TransformState& transform_state,
    VisualRectFlags visual_rect_flags) const {
  // For any layout object that doesn't override this method (the main example
  // is LayoutText), the rect is assumed to be in the parent's coordinate space,
  // except for container flip.

  if (ancestor == this)
    return true;

  if (LayoutObject* parent = Parent()) {
    if (parent->IsBox()) {
      LayoutBox* parent_box = ToLayoutBox(parent);

      // Never flip for SVG as it handles writing modes itself.
      if (!IsSVG()) {
        transform_state.Flatten();
        LayoutRect rect(transform_state.LastPlanarQuad().BoundingBox());
        parent_box->FlipForWritingMode(rect);
        transform_state.SetQuad(FloatQuad(FloatRect(rect)));
      }

      bool preserve3d = parent->Style()->Preserves3D() && !parent->IsText();

      TransformState::TransformAccumulation accumulation =
          preserve3d ? TransformState::kAccumulateTransform
                     : TransformState::kFlattenTransform;

      if (parent != ancestor &&
          !parent_box->MapContentsRectToBoxSpace(transform_state, accumulation,
                                                 *this, visual_rect_flags))
        return false;
    }
    return parent->MapToVisualRectInAncestorSpaceInternal(
        ancestor, transform_state, visual_rect_flags);
  }
  return true;
}

void LayoutObject::DirtyLinesFromChangedChild(LayoutObject*, MarkingBehavior) {}

std::ostream& operator<<(std::ostream& out, const LayoutObject& object) {
  StringBuilder string_builder;
  object.DumpLayoutObject(string_builder, false, 0);
  return out << string_builder.ToString().Utf8().data();
}

std::ostream& operator<<(std::ostream& out, const LayoutObject* object) {
  if (!object)
    return out << "<null>";
  return out << *object;
}

#ifndef NDEBUG

void LayoutObject::ShowTreeForThis() const {
  if (GetNode())
    ::showTree(GetNode());
}

void LayoutObject::ShowLayoutTreeForThis() const {
  showLayoutTree(this, nullptr);
}

void LayoutObject::ShowLineTreeForThis() const {
  if (const LayoutBlock* cb = InclusiveContainingBlock()) {
    if (cb->IsLayoutBlockFlow())
      ToLayoutBlockFlow(cb)->ShowLineTreeAndMark(nullptr, nullptr, nullptr,
                                                 nullptr, this);
  }
}

void LayoutObject::ShowLayoutObject() const {
  StringBuilder string_builder;
  DumpLayoutObject(string_builder, true, kShowTreeCharacterOffset);
  DLOG(INFO) << "\n" << string_builder.ToString().Utf8().data();
}

#endif  // NDEBUG

void LayoutObject::DumpLayoutObject(StringBuilder& string_builder,
                                    bool dump_address,
                                    unsigned show_tree_character_offset) const {
  string_builder.Append(DecoratedName());

  if (dump_address)
    string_builder.Append(String::Format(" %p", this));

  if (IsText() && ToLayoutText(this)->IsTextFragment())
    string_builder.Append(String::Format(
        " \"%s\" ", ToLayoutText(this)->GetText().Ascii().data()));

  if (VirtualContinuation())
    string_builder.Append(
        String::Format(" continuation=%p", VirtualContinuation()));

  if (GetNode()) {
    while (string_builder.length() < show_tree_character_offset)
      string_builder.Append(' ');
    string_builder.Append('\t');
    string_builder.Append(GetNode()->ToString().Utf8().data());
  }
}

#ifndef NDEBUG

void LayoutObject::DumpLayoutTreeAndMark(StringBuilder& string_builder,
                                         const LayoutObject* marked_object1,
                                         const char* marked_label1,
                                         const LayoutObject* marked_object2,
                                         const char* marked_label2,
                                         unsigned depth) const {
  StringBuilder object_info;
  if (marked_object1 == this && marked_label1)
    object_info.Append(marked_label1);
  if (marked_object2 == this && marked_label2)
    object_info.Append(marked_label2);
  while (object_info.length() < depth * 2)
    object_info.Append(' ');

  DumpLayoutObject(object_info, true, kShowTreeCharacterOffset);
  string_builder.Append(object_info);

  for (const LayoutObject* child = SlowFirstChild(); child;
       child = child->NextSibling()) {
    string_builder.Append('\n');
    child->DumpLayoutTreeAndMark(string_builder, marked_object1, marked_label1,
                                 marked_object2, marked_label2, depth + 1);
  }
}

#endif  // NDEBUG

bool LayoutObject::IsSelectable() const {
  return !IsInert() && !(Style()->UserSelect() == EUserSelect::kNone &&
                         Style()->UserModify() == EUserModify::kReadOnly);
}

// Called when an object that was floating or positioned becomes a normal flow
// object again. We have to make sure the layout tree updates as needed to
// accommodate the new normal flow object.
static inline void HandleDynamicFloatPositionChange(LayoutObject* object) {
  // We have gone from not affecting the inline status of the parent flow to
  // suddenly having an impact.  See if there is a mismatch between the parent
  // flow's childrenInline() state and our state.
  object->SetInline(object->Style()->IsDisplayInlineType());
  if (object->IsInline() != object->Parent()->ChildrenInline()) {
    if (!object->IsInline()) {
      ToLayoutBoxModelObject(object->Parent())->ChildBecameNonInline(object);
    } else {
      // An anonymous block must be made to wrap this inline.
      LayoutBlock* block =
          ToLayoutBlock(object->Parent())->CreateAnonymousBlock();
      LayoutObjectChildList* childlist = object->Parent()->VirtualChildren();
      childlist->InsertChildNode(object->Parent(), block, object);
      block->Children()->AppendChildNode(
          block, childlist->RemoveChildNode(object->Parent(), object));
    }
  }
}

StyleDifference LayoutObject::AdjustStyleDifference(
    StyleDifference diff) const {
  if (diff.TransformChanged() && IsSVG()) {
    // Skip a full layout for transforms at the html/svg boundary which do not
    // affect sizes inside SVG.
    if (!IsSVGRoot())
      diff.SetNeedsFullLayout();
  }

  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    // If transform changed, and the layer does not paint into its own separate
    // backing, then we need to invalidate paints.
    if (diff.TransformChanged()) {
      // Text nodes share style with their parents but transforms don't apply to
      // them, hence the !isText() check.
      if (!IsText() &&
          (!HasLayer() || !ToLayoutBoxModelObject(this)
                               ->Layer()
                               ->HasStyleDeterminedDirectCompositingReasons()))
        diff.SetNeedsPaintInvalidationSubtree();
    }

    // If opacity or zIndex changed, and the layer does not paint into its own
    // separate backing, then we need to invalidate paints (also
    // ignoring text nodes).
    if (diff.OpacityChanged() || diff.ZIndexChanged()) {
      if (!IsText() &&
          (!HasLayer() || !ToLayoutBoxModelObject(this)
                               ->Layer()
                               ->HasStyleDeterminedDirectCompositingReasons()))
        diff.SetNeedsPaintInvalidationSubtree();
    }

    // If filter changed, and the layer does not paint into its own separate
    // backing or it paints with filters, then we need to invalidate paints.
    if (diff.FilterChanged() && HasLayer()) {
      PaintLayer* layer = ToLayoutBoxModelObject(this)->Layer();
      if (!layer->HasStyleDeterminedDirectCompositingReasons() ||
          layer->PaintsWithFilters())
        diff.SetNeedsPaintInvalidationSubtree();
    }

    // If backdrop filter changed, and the layer does not paint into its own
    // separate backing or it paints with filters, then we need to invalidate
    // paints.
    if (diff.BackdropFilterChanged() && HasLayer()) {
      PaintLayer* layer = ToLayoutBoxModelObject(this)->Layer();
      if (!layer->HasStyleDeterminedDirectCompositingReasons() ||
          layer->PaintsWithBackdropFilters())
        diff.SetNeedsPaintInvalidationSubtree();
    }
  }

  // TODO(wangxianzhu): We may avoid subtree paint invalidation on CSS clip
  // change for SPv2.
  if (diff.CssClipChanged())
    diff.SetNeedsPaintInvalidationSubtree();

  // Optimization: for decoration/color property changes, invalidation is only
  // needed if we have style or text affected by these properties.
  if (diff.TextDecorationOrColorChanged() &&
      !diff.NeedsFullPaintInvalidation()) {
    if (Style()->HasBorderColorReferencingCurrentColor() ||
        Style()->HasOutlineWithCurrentColor() ||
        Style()->HasBackgroundRelatedColorReferencingCurrentColor() ||
        // Skip any text nodes that do not contain text boxes. Whitespace cannot
        // be skipped or we will miss invalidating decorations (e.g.,
        // underlines).
        (IsText() && !IsBR() && ToLayoutText(this)->HasTextBoxes()) ||
        (IsSVG() && Style()->SvgStyle().IsFillColorCurrentColor()) ||
        (IsSVG() && Style()->SvgStyle().IsStrokeColorCurrentColor()) ||
        IsListMarker())
      diff.SetNeedsPaintInvalidationObject();
  }

  // The answer to layerTypeRequired() for plugins, iframes, and canvas can
  // change without the actual style changing, since it depends on whether we
  // decide to composite these elements. When the/ layer status of one of these
  // elements changes, we need to force a layout.
  if (!diff.NeedsFullLayout() && Style() && IsBoxModelObject()) {
    bool requires_layer =
        ToLayoutBoxModelObject(this)->LayerTypeRequired() != kNoPaintLayer;
    if (HasLayer() != requires_layer)
      diff.SetNeedsFullLayout();
  }

  return diff;
}

void LayoutObject::SetPseudoStyle(scoped_refptr<ComputedStyle> pseudo_style) {
  DCHECK(pseudo_style->StyleType() == kPseudoIdBefore ||
         pseudo_style->StyleType() == kPseudoIdAfter ||
         pseudo_style->StyleType() == kPseudoIdFirstLetter);

  // FIXME: We should consider just making all pseudo items use an inherited
  // style.

  // Images are special and must inherit the pseudoStyle so the width and height
  // of the pseudo element doesn't change the size of the image. In all other
  // cases we can just share the style.
  //
  // Quotes are also LayoutInline, so we need to create an inherited style to
  // avoid getting an inline with positioning or an invalid display.
  //
  if (IsImage() || IsQuote()) {
    scoped_refptr<ComputedStyle> style = ComputedStyle::Create();
    style->InheritFrom(*pseudo_style);
    SetStyle(std::move(style));
    return;
  }

  SetStyle(std::move(pseudo_style));
}

void LayoutObject::FirstLineStyleDidChange(const ComputedStyle& old_style,
                                           const ComputedStyle& new_style) {
  StyleDifference diff =
      old_style.VisualInvalidationDiff(GetDocument(), new_style);

  if (diff.NeedsFullPaintInvalidation() ||
      diff.TextDecorationOrColorChanged()) {
    // We need to invalidate all inline boxes in the first line, because they
    // need to be repainted with the new style, e.g. background, font style,
    // etc.
    LayoutBlockFlow* first_line_container = nullptr;
    if (BehavesLikeBlockContainer()) {
      // This object is a LayoutBlock having PseudoIdFirstLine pseudo style
      // changed.
      first_line_container =
          ToLayoutBlock(this)->NearestInnerBlockWithFirstLine();
    } else if (IsLayoutInline()) {
      // This object is a LayoutInline having FIRST_LINE_INHERITED pesudo style
      // changed. This method can be called even if the LayoutInline doesn't
      // intersect the first line, but we only need to invalidate if it does.
      if (InlineBox* first_line_box =
              ToLayoutInline(this)->FirstLineBoxIncludingCulling()) {
        if (first_line_box->IsFirstLineStyle())
          first_line_container = ToLayoutBlockFlow(ContainingBlock());
      }
    }
    if (first_line_container)
      first_line_container->SetShouldDoFullPaintInvalidationForFirstLine();
  }
  if (diff.NeedsLayout())
    SetNeedsLayoutAndPrefWidthsRecalc(LayoutInvalidationReason::kStyleChange);
}

void LayoutObject::MarkContainerChainForOverflowRecalcIfNeeded() {
  LayoutObject* object = this;
  do {
    // Cell and row need to propagate the flag to their containing section and
    // row as their containing block is the table wrapper.
    // This enables us to only recompute overflow the modified sections / rows.
    object = object->IsTableCell() || object->IsTableRow()
                 ? object->Parent()
                 : object->Container();
    if (object)
      object->SetChildNeedsOverflowRecalcAfterStyleChange();
  } while (object);
}

void LayoutObject::SetNeedsOverflowRecalcAfterStyleChange() {
  bool needed_recalc = NeedsOverflowRecalcAfterStyleChange();
  SetSelfNeedsOverflowRecalcAfterStyleChange();
  SetMayNeedPaintInvalidation();
  if (!needed_recalc)
    MarkContainerChainForOverflowRecalcIfNeeded();
}

DISABLE_CFI_PERF
void LayoutObject::SetStyle(scoped_refptr<ComputedStyle> style) {
  DCHECK(style);
  if (style_ == style)
    return;

  StyleDifference diff;
  if (style_) {
    diff = style_->VisualInvalidationDiff(GetDocument(), *style);
  } else {
    // If there was no previous style, set the object as at least needing
    // paint invalidation, to prevent diff.HasDifference() from returning
    // false.
    // TODO(chrishtr): shouldn't this set all of the bits? crbug.com/817610.
    diff.SetNeedsPaintInvalidationObject();
  }

  diff = AdjustStyleDifference(diff);

  StyleWillChange(diff, *style);

  scoped_refptr<ComputedStyle> old_style = std::move(style_);
  SetStyleInternal(std::move(style));

  UpdateFillImages(old_style ? &old_style->BackgroundLayers() : nullptr,
                   style_->BackgroundLayers());
  UpdateFillImages(old_style ? &old_style->MaskLayers() : nullptr,
                   style_->MaskLayers());

  UpdateImage(old_style ? old_style->BorderImage().GetImage() : nullptr,
              style_->BorderImage().GetImage());
  UpdateImage(old_style ? old_style->MaskBoxImage().GetImage() : nullptr,
              style_->MaskBoxImage().GetImage());

  StyleImage* new_content_image =
      style_->GetContentData() && style_->GetContentData()->IsImage()
          ? ToImageContentData(style_->GetContentData())->GetImage()
          : nullptr;
  StyleImage* old_content_image =
      old_style && old_style->GetContentData() &&
              old_style->GetContentData()->IsImage()
          ? ToImageContentData(old_style->GetContentData())->GetImage()
          : nullptr;
  UpdateImage(old_content_image, new_content_image);

  StyleImage* new_box_reflect_mask_image =
      style_->BoxReflect() ? style_->BoxReflect()->Mask().GetImage() : nullptr;
  StyleImage* old_box_reflect_mask_image =
      old_style && old_style->BoxReflect()
          ? old_style->BoxReflect()->Mask().GetImage()
          : nullptr;
  UpdateImage(old_box_reflect_mask_image, new_box_reflect_mask_image);

  UpdateShapeImage(old_style ? old_style->ShapeOutside() : nullptr,
                   style_->ShapeOutside());
  UpdateCursorImages(old_style ? old_style->Cursors() : nullptr,
                     style_->Cursors());

  CheckCounterChanges(old_style.get(), style_.get());

  bool does_not_need_layout_or_paint_invalidation = !parent_;

  StyleDidChange(diff, old_style.get());

  // FIXME: |this| might be destroyed here. This can currently happen for a
  // LayoutTextFragment when its first-letter block gets an update in
  // LayoutTextFragment::styleDidChange. For LayoutTextFragment(s),
  // we will safely bail out with the doesNotNeedLayoutOrPaintInvalidation flag.
  // We might want to broaden this condition in the future as we move
  // layoutObject changes out of layout and into style changes.
  if (does_not_need_layout_or_paint_invalidation)
    return;

  // Now that the layer (if any) has been updated, we need to adjust the diff
  // again, check whether we should layout now, and decide if we need to
  // invalidate paints.
  StyleDifference updated_diff = AdjustStyleDifference(diff);

  if (!diff.NeedsFullLayout()) {
    if (updated_diff.NeedsFullLayout())
      SetNeedsLayoutAndPrefWidthsRecalc(LayoutInvalidationReason::kStyleChange);
    else if (updated_diff.NeedsPositionedMovementLayout())
      SetNeedsPositionedMovementLayout();
  }

  if (diff.TransformChanged() && !NeedsLayout()) {
    if (LayoutBlock* container = ContainingBlock())
      container->SetNeedsOverflowRecalcAfterStyleChange();
  }

  if (diff.NeedsRecomputeOverflow() && !NeedsLayout()) {
    // TODO(rhogan): Make inlines capable of recomputing overflow too.
    if (IsLayoutBlock())
      SetNeedsOverflowRecalcAfterStyleChange();
    else
      SetNeedsLayoutAndPrefWidthsRecalc(LayoutInvalidationReason::kStyleChange);
  }

  if (diff.NeedsPaintInvalidationSubtree() ||
      updated_diff.NeedsPaintInvalidationSubtree()) {
    SetShouldDoFullPaintInvalidationIncludingNonCompositingDescendants();
  } else if (diff.NeedsPaintInvalidationObject() ||
             updated_diff.NeedsPaintInvalidationObject()) {
    // TODO(wangxianzhu): For now LayoutSVGRoot::localVisualRect() depends on
    // several styles. Refactor to avoid this special case.
    if (IsSVGRoot())
      SetShouldDoFullPaintInvalidation();
    else
      SetShouldDoFullPaintInvalidationWithoutGeometryChange();
  }

  if ((diff.NeedsPaintInvalidationObject() ||
       diff.NeedsPaintInvalidationSubtree()) &&
      old_style && !old_style->ClipPathDataEquivalent(*style_))
    InvalidateClipPathCache();

  if (diff.NeedsVisualRectUpdate())
    SetMayNeedPaintInvalidation();

  // Text nodes share style with their parents but the paint properties don't
  // apply to them, hence the !isText() check.
  // In SPv175 mode, if property nodes are added or removed as a result of these
  // style changes, PaintPropertyTreeBuilder will call SetNeedsRepaint
  // to cause re-generation of PaintChunks.
  if (!IsText() && (diff.TransformChanged() || diff.OpacityChanged() ||
                    diff.ZIndexChanged() || diff.FilterChanged() ||
                    diff.BackdropFilterChanged() || diff.CssClipChanged()))
    SetNeedsPaintPropertyUpdate();
}

void LayoutObject::StyleWillChange(StyleDifference diff,
                                   const ComputedStyle& new_style) {
  if (style_) {
    bool visibility_changed = style_->Visibility() != new_style.Visibility();
    // If our z-index changes value or our visibility changes,
    // we need to dirty our stacking context's z-order list.
    if (visibility_changed || style_->ZIndex() != new_style.ZIndex() ||
        style_->IsStackingContext() != new_style.IsStackingContext()) {
      GetDocument().SetAnnotatedRegionsDirty(true);
      if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache())
        cache->ChildrenChanged(Parent());
    }

    // Keep layer hierarchy visibility bits up to date if visibility changes.
    if (visibility_changed) {
      // We might not have an enclosing layer yet because we might not be in the
      // tree.
      if (PaintLayer* layer = EnclosingLayer())
        layer->DirtyVisibleContentStatus();
    }

    if (IsFloating() && (style_->Floating() != new_style.Floating())) {
      // For changes in float styles, we need to conceivably remove ourselves
      // from the floating objects list.
      ToLayoutBox(this)->RemoveFloatingOrPositionedChildFromBlockLists();
    } else if (IsOutOfFlowPositioned() &&
               (style_->GetPosition() != new_style.GetPosition())) {
      // For changes in positioning styles, we need to conceivably remove
      // ourselves from the positioned objects list.
      ToLayoutBox(this)->RemoveFloatingOrPositionedChildFromBlockLists();
    }

    affects_parent_block_ =
        IsFloatingOrOutOfFlowPositioned() &&
        (!new_style.IsFloating() && !new_style.HasOutOfFlowPosition()) &&
        Parent() &&
        (Parent()->IsLayoutBlockFlow() || Parent()->IsLayoutInline());

    // Clearing these bits is required to avoid leaving stale layoutObjects.
    // FIXME: We shouldn't need that hack if our logic was totally correct.
    if (diff.NeedsLayout()) {
      SetFloating(false);
      ClearPositionedState();
    }
  } else {
    affects_parent_block_ = false;
  }

  // Elements with non-auto touch-action will send a SetTouchAction message
  // on touchstart in EventHandler::handleTouchEvent, and so effectively have
  // a touchstart handler that must be reported.
  //
  // Since a CSS property cannot be applied directly to a text node, a
  // handler will have already been added for its parent so ignore it.
  //
  // Elements may inherit touch action from parent frame, so we need to report
  // touchstart handler if the root layout object has non-auto effective touch
  // action.
  TouchAction old_touch_action = TouchAction::kTouchActionAuto;
  bool is_document_element = GetNode() && IsDocumentElement();
  if (style_) {
    old_touch_action = is_document_element ? style_->GetEffectiveTouchAction()
                                           : style_->GetTouchAction();
  }
  TouchAction new_touch_action = is_document_element
                                     ? new_style.GetEffectiveTouchAction()
                                     : new_style.GetTouchAction();
  if (GetNode() && !GetNode()->IsTextNode() &&
      (old_touch_action == TouchAction::kTouchActionAuto) !=
          (new_touch_action == TouchAction::kTouchActionAuto)) {
    EventHandlerRegistry& registry =
        GetDocument().GetFrame()->GetEventHandlerRegistry();
    if (new_touch_action != TouchAction::kTouchActionAuto) {
      registry.DidAddEventHandler(*GetNode(),
                                  EventHandlerRegistry::kTouchAction);
    } else {
      registry.DidRemoveEventHandler(*GetNode(),
                                     EventHandlerRegistry::kTouchAction);
    }
  }
}

void LayoutObject::ClearBaseComputedStyle() {
  if (!GetNode())
    return;
  if (!GetNode()->IsElementNode())
    return;
  if (ElementAnimations* animations =
          ToElement(GetNode())->GetElementAnimations())
    animations->ClearBaseComputedStyle();
}

static bool AreNonIdenticalCursorListsEqual(const ComputedStyle* a,
                                            const ComputedStyle* b) {
  DCHECK_NE(a->Cursors(), b->Cursors());
  return a->Cursors() && b->Cursors() && *a->Cursors() == *b->Cursors();
}

static inline bool AreCursorsEqual(const ComputedStyle* a,
                                   const ComputedStyle* b) {
  return a->Cursor() == b->Cursor() && (a->Cursors() == b->Cursors() ||
                                        AreNonIdenticalCursorListsEqual(a, b));
}

void LayoutObject::SetScrollAnchorDisablingStyleChangedOnAncestor() {
  // Walk up the parent chain and find the first scrolling block to disable
  // scroll anchoring on.
  LayoutObject* object = Parent();
  Element* viewport_defining_element = GetDocument().ViewportDefiningElement();
  while (object) {
    if (object->IsLayoutBlock()) {
      LayoutBlock* block = ToLayoutBlock(object);
      if (block->HasOverflowClip() ||
          block->GetNode() == viewport_defining_element) {
        block->SetScrollAnchorDisablingStyleChanged(true);
        return;
      }
    }
    object = object->Parent();
  }
}

void LayoutObject::StyleDidChange(StyleDifference diff,
                                  const ComputedStyle* old_style) {
  // First assume the outline will be affected. It may be updated when we know
  // it's not affected.
  bool has_outline = style_->HasOutline();
  SetOutlineMayBeAffectedByDescendants(has_outline);
  if (!has_outline)
    SetPreviousOutlineMayBeAffectedByDescendants(false);

  if (affects_parent_block_)
    HandleDynamicFloatPositionChange(this);

  if (diff.NeedsFullLayout()) {
    // If the in-flow state of an element is changed, disable scroll
    // anchoring on the containing scroller.
    if (old_style->HasOutOfFlowPosition() != style_->HasOutOfFlowPosition())
      SetScrollAnchorDisablingStyleChangedOnAncestor();

    // If the object already needs layout, then setNeedsLayout won't do
    // any work. But if the containing block has changed, then we may need
    // to mark the new containing blocks for layout. The change that can
    // directly affect the containing block of this object is a change to
    // the position style.
    if (NeedsLayout() && old_style->GetPosition() != style_->GetPosition())
      MarkContainerChainForLayout();

    // Ditto.
    if (NeedsOverflowRecalcAfterStyleChange() &&
        old_style->GetPosition() != style_->GetPosition())
      MarkContainerChainForOverflowRecalcIfNeeded();

    SetNeedsLayoutAndPrefWidthsRecalc(LayoutInvalidationReason::kStyleChange);
  } else if (diff.NeedsPositionedMovementLayout()) {
    SetNeedsPositionedMovementLayout();
  }

  if (diff.ScrollAnchorDisablingPropertyChanged())
    SetScrollAnchorDisablingStyleChanged(true);

  // Don't check for paint invalidation here; we need to wait until the layer
  // has been updated by subclasses before we know if we have to invalidate
  // paints (in setStyle()).

  if (old_style && !AreCursorsEqual(old_style, Style())) {
    if (LocalFrame* frame = GetFrame()) {
      // Cursor update scheduling is done by the local root, which is the main
      // frame if there are no RemoteFrame ancestors in the frame tree. Use of
      // localFrameRoot() is discouraged but will change when cursor update
      // scheduling is moved from EventHandler to PageEventHandler.
      frame->LocalFrameRoot().GetEventHandler().ScheduleCursorUpdate();
    }
  }

  if (diff.NeedsFullPaintInvalidation() && old_style) {
    if (ResolveColor(*old_style, GetCSSPropertyBackgroundColor()) !=
            ResolveColor(GetCSSPropertyBackgroundColor()) ||
        old_style->BackgroundLayers() != StyleRef().BackgroundLayers())
      SetBackgroundChangedSinceLastPaintInvalidation();
  }

  if (old_style && old_style->StyleType() == kPseudoIdNone)
    ApplyPseudoStyleChanges(*old_style);

  if (old_style &&
      old_style->UsedTransformStyle3D() != StyleRef().UsedTransformStyle3D()) {
    // Change of transform-style may affect descendant transform property nodes.
    SetSubtreeNeedsPaintPropertyUpdate();
  }
}

void LayoutObject::ApplyPseudoStyleChanges(const ComputedStyle& old_style) {
  if (old_style.HasPseudoStyle(kPseudoIdFirstLine) ||
      StyleRef().HasPseudoStyle(kPseudoIdFirstLine))
    ApplyFirstLineChanges(old_style);

  if (old_style.HasPseudoStyle(kPseudoIdSelection) ||
      StyleRef().HasPseudoStyle(kPseudoIdSelection))
    InvalidateSelectionOfSelectedChildren();
}

void LayoutObject::ApplyFirstLineChanges(const ComputedStyle& old_style) {
  if (old_style.HasPseudoStyle(kPseudoIdFirstLine)) {
    scoped_refptr<ComputedStyle> old_pseudo_style =
        old_style.GetCachedPseudoStyle(kPseudoIdFirstLine);
    if (StyleRef().HasPseudoStyle(kPseudoIdFirstLine) && old_pseudo_style) {
      scoped_refptr<ComputedStyle> new_pseudo_style = UncachedFirstLineStyle();
      if (new_pseudo_style) {
        FirstLineStyleDidChange(*old_pseudo_style, *new_pseudo_style);
        return;
      }
    }
  }
  SetNeedsLayoutAndPrefWidthsRecalc(LayoutInvalidationReason::kStyleChange);
}

void LayoutObject::PropagateStyleToAnonymousChildren() {
  // FIXME: We could save this call when the change only affected non-inherited
  // properties.
  for (LayoutObject* child = SlowFirstChild(); child;
       child = child->NextSibling()) {
    if (!child->IsAnonymous() || child->Style()->StyleType() != kPseudoIdNone)
      continue;

    if (child->AnonymousHasStylePropagationOverride())
      continue;

    scoped_refptr<ComputedStyle> new_style =
        ComputedStyle::CreateAnonymousStyleWithDisplay(
            StyleRef(), child->Style()->Display());

    // Preserve the position style of anonymous block continuations as they can
    // have relative position when they contain block descendants of relative
    // positioned inlines.
    if (child->IsInFlowPositioned() && child->IsLayoutBlockFlow() &&
        ToLayoutBlockFlow(child)->IsAnonymousBlockContinuation())
      new_style->SetPosition(child->Style()->GetPosition());

    UpdateAnonymousChildStyle(*child, *new_style);

    child->SetStyle(std::move(new_style));
  }
}

void LayoutObject::SetStyleWithWritingModeOf(scoped_refptr<ComputedStyle> style,
                                             LayoutObject* parent) {
  if (parent)
    style->SetWritingMode(parent->StyleRef().GetWritingMode());
  SetStyle(std::move(style));
}

void LayoutObject::SetStyleWithWritingModeOfParent(
    scoped_refptr<ComputedStyle> style) {
  SetStyleWithWritingModeOf(std::move(style), Parent());
}

void LayoutObject::UpdateFillImages(const FillLayer* old_layers,
                                    const FillLayer& new_layers) {
  // Optimize the common case
  if (FillLayer::ImagesIdentical(old_layers, &new_layers))
    return;

  // Go through the new layers and addClients first, to avoid removing all
  // clients of an image.
  for (const FillLayer* curr_new = &new_layers; curr_new;
       curr_new = curr_new->Next()) {
    if (curr_new->GetImage())
      curr_new->GetImage()->AddClient(this);
  }

  for (const FillLayer* curr_old = old_layers; curr_old;
       curr_old = curr_old->Next()) {
    if (curr_old->GetImage())
      curr_old->GetImage()->RemoveClient(this);
  }
}

void LayoutObject::UpdateCursorImages(const CursorList* old_cursors,
                                      const CursorList* new_cursors) {
  if (old_cursors && new_cursors && *old_cursors == *new_cursors)
    return;

  if (new_cursors) {
    for (const CursorData& cursor_new : *new_cursors) {
      if (cursor_new.GetImage())
        cursor_new.GetImage()->AddClient(this);
    }
  }
  RemoveCursorImageClient(old_cursors);
}

void LayoutObject::UpdateImage(StyleImage* old_image, StyleImage* new_image) {
  if (old_image != new_image) {
    if (old_image)
      old_image->RemoveClient(this);
    if (new_image)
      new_image->AddClient(this);
  }
}

void LayoutObject::UpdateShapeImage(const ShapeValue* old_shape_value,
                                    const ShapeValue* new_shape_value) {
  if (old_shape_value || new_shape_value) {
    UpdateImage(old_shape_value ? old_shape_value->GetImage() : nullptr,
                new_shape_value ? new_shape_value->GetImage() : nullptr);
  }
}

void LayoutObject::CheckCounterChanges(const ComputedStyle* old_style,
                                       const ComputedStyle* new_style) {
  DCHECK(new_style);
  if (old_style) {
    if (old_style->CounterDirectivesEqual(*new_style))
      return;
  } else {
    if (!new_style->GetCounterDirectives())
      return;
  }
  LayoutCounter::LayoutObjectStyleChanged(*this, old_style, *new_style);
  View()->SetNeedsCounterUpdate();
}

LayoutRect LayoutObject::ViewRect() const {
  return View()->ViewRect();
}

FloatPoint LayoutObject::LocalToAbsolute(const FloatPoint& local_point,
                                         MapCoordinatesFlags mode) const {
  TransformState transform_state(TransformState::kApplyTransformDirection,
                                 local_point);
  MapLocalToAncestor(nullptr, transform_state, mode | kApplyContainerFlip);
  transform_state.Flatten();

  return transform_state.LastPlanarPoint();
}

FloatPoint LayoutObject::AncestorToLocal(LayoutBoxModelObject* ancestor,
                                         const FloatPoint& container_point,
                                         MapCoordinatesFlags mode) const {
  TransformState transform_state(
      TransformState::kUnapplyInverseTransformDirection, container_point);
  MapAncestorToLocal(ancestor, transform_state, mode);
  transform_state.Flatten();

  return transform_state.LastPlanarPoint();
}

FloatQuad LayoutObject::AncestorToLocalQuad(LayoutBoxModelObject* ancestor,
                                            const FloatQuad& quad,
                                            MapCoordinatesFlags mode) const {
  TransformState transform_state(
      TransformState::kUnapplyInverseTransformDirection,
      quad.BoundingBox().Center(), quad);
  MapAncestorToLocal(ancestor, transform_state, mode);
  transform_state.Flatten();
  return transform_state.LastPlanarQuad();
}

void LayoutObject::MapLocalToAncestor(const LayoutBoxModelObject* ancestor,
                                      TransformState& transform_state,
                                      MapCoordinatesFlags mode) const {
  if (ancestor == this)
    return;

  AncestorSkipInfo skip_info(ancestor);
  const LayoutObject* container = Container(&skip_info);
  if (!container)
    return;

  if (mode & kApplyContainerFlip) {
    if (IsBox()) {
      mode &= ~kApplyContainerFlip;
    } else if (container->IsBox()) {
      if (container->Style()->IsFlippedBlocksWritingMode()) {
        IntPoint center_point = RoundedIntPoint(transform_state.MappedPoint());
        transform_state.Move(ToLayoutBox(container)->FlipForWritingMode(
                                 LayoutPoint(center_point)) -
                             center_point);
      }
      mode &= ~kApplyContainerFlip;
    }
  }

  LayoutSize container_offset =
      OffsetFromContainer(container, mode & kIgnoreScrollOffset);
  // TODO(smcgruer): This is inefficient. Instead we should avoid including
  // offsetForInFlowPosition in offsetFromContainer when ignoring sticky.
  if (mode & kIgnoreStickyOffset && IsStickyPositioned()) {
    container_offset -= ToLayoutBoxModelObject(this)->OffsetForInFlowPosition();
  }

  if (IsLayoutFlowThread()) {
    // So far the point has been in flow thread coordinates (i.e. as if
    // everything in the fragmentation context lived in one tall single column).
    // Convert it to a visual point now, since we're about to escape the flow
    // thread.
    container_offset +=
        ColumnOffset(LayoutPoint(transform_state.MappedPoint()));
  }

  // Text objects just copy their parent's computed style, so we need to ignore
  // them.
  bool preserve3d =
      mode & kUseTransforms &&
      ((container->Style()->Preserves3D() && !container->IsText()) ||
       (Style()->Preserves3D() && !IsText()));
  if (mode & kUseTransforms && ShouldUseTransformFromContainer(container)) {
    TransformationMatrix t;
    GetTransformFromContainer(container, container_offset, t);
    transform_state.ApplyTransform(t, preserve3d
                                          ? TransformState::kAccumulateTransform
                                          : TransformState::kFlattenTransform);
  } else {
    transform_state.Move(container_offset.Width(), container_offset.Height(),
                         preserve3d ? TransformState::kAccumulateTransform
                                    : TransformState::kFlattenTransform);
  }

  if (skip_info.AncestorSkipped()) {
    // There can't be a transform between |ancestor| and |o|, because transforms
    // create containers, so it should be safe to just subtract the delta
    // between the ancestor and |o|.
    LayoutSize container_offset =
        ancestor->OffsetFromAncestorContainer(container);
    transform_state.Move(-container_offset.Width(), -container_offset.Height(),
                         preserve3d ? TransformState::kAccumulateTransform
                                    : TransformState::kFlattenTransform);
    // If the ancestor is fixed, then the rect is already in its coordinates so
    // doesn't need viewport-adjusting.
    if (ancestor->Style()->GetPosition() != EPosition::kFixed &&
        container->IsLayoutView() &&
        StyleRef().GetPosition() == EPosition::kFixed) {
      LayoutSize adjustment = ToLayoutView(container)->OffsetForFixedPosition();
      transform_state.Move(adjustment.Width(), adjustment.Height());
    }
    return;
  }

  container->MapLocalToAncestor(ancestor, transform_state, mode);
}

const LayoutObject* LayoutObject::PushMappingToContainer(
    const LayoutBoxModelObject* ancestor_to_stop_at,
    LayoutGeometryMap& geometry_map) const {
  NOTREACHED();
  return nullptr;
}

void LayoutObject::MapAncestorToLocal(const LayoutBoxModelObject* ancestor,
                                      TransformState& transform_state,
                                      MapCoordinatesFlags mode) const {
  if (this == ancestor)
    return;

  AncestorSkipInfo skip_info(ancestor);
  LayoutObject* container = Container(&skip_info);
  if (!container)
    return;

  bool apply_container_flip = false;
  if (mode & kApplyContainerFlip) {
    if (IsBox()) {
      mode &= ~kApplyContainerFlip;
    } else if (container->IsBox()) {
      apply_container_flip = container->Style()->IsFlippedBlocksWritingMode();
      mode &= ~kApplyContainerFlip;
    }
  }

  if (!skip_info.AncestorSkipped())
    container->MapAncestorToLocal(ancestor, transform_state, mode);

  LayoutSize container_offset = OffsetFromContainer(container);
  bool preserve3d =
      mode & kUseTransforms &&
      (container->Style()->Preserves3D() || Style()->Preserves3D());
  if (mode & kUseTransforms && ShouldUseTransformFromContainer(container)) {
    TransformationMatrix t;
    GetTransformFromContainer(container, container_offset, t);
    transform_state.ApplyTransform(t, preserve3d
                                          ? TransformState::kAccumulateTransform
                                          : TransformState::kFlattenTransform);
  } else {
    transform_state.Move(container_offset.Width(), container_offset.Height(),
                         preserve3d ? TransformState::kAccumulateTransform
                                    : TransformState::kFlattenTransform);
  }

  if (IsLayoutFlowThread()) {
    // Descending into a flow thread. Convert to the local coordinate space,
    // i.e. flow thread coordinates.
    LayoutPoint visual_point = LayoutPoint(transform_state.MappedPoint());
    transform_state.Move(
        visual_point -
        ToLayoutFlowThread(this)->VisualPointToFlowThreadPoint(visual_point));
  }

  if (apply_container_flip) {
    IntPoint center_point = RoundedIntPoint(transform_state.MappedPoint());
    transform_state.Move(
        center_point -
        ToLayoutBox(container)->FlipForWritingMode(LayoutPoint(center_point)));
  }

  if (skip_info.AncestorSkipped()) {
    container_offset = ancestor->OffsetFromAncestorContainer(container);
    transform_state.Move(-container_offset.Width(), -container_offset.Height());
    // If the ancestor is fixed, then the rect is already in its coordinates so
    // doesn't need viewport-adjusting.
    if (ancestor->Style()->GetPosition() != EPosition::kFixed &&
        container->IsLayoutView() &&
        StyleRef().GetPosition() == EPosition::kFixed) {
      LayoutSize adjustment = ToLayoutView(container)->OffsetForFixedPosition();
      transform_state.Move(adjustment.Width(), adjustment.Height());
    }
  }
}

bool LayoutObject::ShouldUseTransformFromContainer(
    const LayoutObject* container_object) const {
  // hasTransform() indicates whether the object has transform, transform-style
  // or perspective. We just care about transform, so check the layer's
  // transform directly.
  return (HasLayer() && ToLayoutBoxModelObject(this)->Layer()->Transform()) ||
         (container_object && container_object->Style()->HasPerspective());
}

void LayoutObject::GetTransformFromContainer(
    const LayoutObject* container_object,
    const LayoutSize& offset_in_container,
    TransformationMatrix& transform) const {
  transform.MakeIdentity();
  PaintLayer* layer =
      HasLayer() ? ToLayoutBoxModelObject(this)->Layer() : nullptr;
  if (layer && layer->Transform())
    transform.Multiply(layer->CurrentTransform());

  transform.PostTranslate(offset_in_container.Width().ToFloat(),
                          offset_in_container.Height().ToFloat());

  if (container_object && container_object->HasLayer() &&
      container_object->Style()->HasPerspective()) {
    // Perspective on the container affects us, so we have to factor it in here.
    DCHECK(container_object->HasLayer());
    FloatPoint perspective_origin =
        ToLayoutBoxModelObject(container_object)->Layer()->PerspectiveOrigin();

    TransformationMatrix perspective_matrix;
    perspective_matrix.ApplyPerspective(
        container_object->Style()->Perspective());
    perspective_matrix.ApplyTransformOrigin(perspective_origin.X(),
                                            perspective_origin.Y(), 0);

    transform = perspective_matrix * transform;
  }
}

FloatQuad LayoutObject::LocalToAncestorQuad(
    const FloatQuad& local_quad,
    const LayoutBoxModelObject* ancestor,
    MapCoordinatesFlags mode) const {
  return LocalToAncestorQuadInternal(local_quad, ancestor,
                                     mode | kUseTransforms);
}

FloatQuad LayoutObject::LocalToAncestorQuadWithoutTransforms(
    const FloatQuad& local_quad,
    const LayoutBoxModelObject* ancestor,
    MapCoordinatesFlags mode) const {
  DCHECK(!(mode & kUseTransforms));
  return LocalToAncestorQuadInternal(local_quad, ancestor, mode);
}

FloatQuad LayoutObject::LocalToAncestorQuadInternal(
    const FloatQuad& local_quad,
    const LayoutBoxModelObject* ancestor,
    MapCoordinatesFlags mode) const {
  // Track the point at the center of the quad's bounding box. As
  // mapLocalToAncestor() calls offsetFromContainer(), it will use that point
  // as the reference point to decide which column's transform to apply in
  // multiple-column blocks.
  // TODO(chrishtr): the second argument to this constructor is unnecessary,
  // since we never call lastPlanarPoint().
  TransformState transform_state(TransformState::kApplyTransformDirection,
                                 local_quad.BoundingBox().Center(), local_quad);
  MapLocalToAncestor(ancestor, transform_state, mode | kApplyContainerFlip);
  transform_state.Flatten();

  return transform_state.LastPlanarQuad();
}

FloatPoint LayoutObject::LocalToAncestorPoint(
    const FloatPoint& local_point,
    const LayoutBoxModelObject* ancestor,
    MapCoordinatesFlags mode) const {
  TransformState transform_state(TransformState::kApplyTransformDirection,
                                 local_point);
  MapLocalToAncestor(ancestor, transform_state,
                     mode | kApplyContainerFlip | kUseTransforms);
  transform_state.Flatten();

  return transform_state.LastPlanarPoint();
}

void LayoutObject::LocalToAncestorRects(Vector<LayoutRect>& rects,
                                        const LayoutBoxModelObject* ancestor,
                                        const LayoutPoint& pre_offset,
                                        const LayoutPoint& post_offset) const {
  for (size_t i = 0; i < rects.size(); ++i) {
    LayoutRect& rect = rects[i];
    rect.MoveBy(pre_offset);
    FloatQuad container_quad =
        LocalToAncestorQuad(FloatQuad(FloatRect(rect)), ancestor);
    LayoutRect container_rect = LayoutRect(container_quad.BoundingBox());
    if (container_rect.IsEmpty()) {
      rects.EraseAt(i--);
      continue;
    }
    container_rect.MoveBy(post_offset);
    rects[i] = container_rect;
  }
}

TransformationMatrix LayoutObject::LocalToAncestorTransform(
    const LayoutBoxModelObject* ancestor,
    MapCoordinatesFlags mode) const {
  TransformState transform_state(TransformState::kApplyTransformDirection);
  MapLocalToAncestor(ancestor, transform_state,
                     mode | kApplyContainerFlip | kUseTransforms);
  return transform_state.AccumulatedTransform();
}

LayoutSize LayoutObject::OffsetFromContainer(const LayoutObject* o,
                                             bool ignore_scroll_offset) const {
  return OffsetFromContainerInternal(o, ignore_scroll_offset);
}

LayoutSize LayoutObject::OffsetFromContainerInternal(
    const LayoutObject* o,
    bool ignore_scroll_offset) const {
  DCHECK_EQ(o, Container());
  return o->HasOverflowClip()
             ? OffsetFromScrollableContainer(o, ignore_scroll_offset)
             : LayoutSize();
}

LayoutSize LayoutObject::OffsetFromScrollableContainer(
    const LayoutObject* container,
    bool ignore_scroll_offset) const {
  DCHECK(container->HasOverflowClip());
  const LayoutBox* box = ToLayoutBox(container);
  if (!ignore_scroll_offset)
    return -LayoutSize(box->ScrolledContentOffset());

  // ScrollOrigin accounts for other writing modes whose content's origin is not
  // at the top-left.
  return LayoutSize(ToIntSize(box->GetScrollableArea()->ScrollOrigin()) -
                    box->OriginAdjustmentForScrollbars());
}

LayoutSize LayoutObject::OffsetFromAncestorContainer(
    const LayoutObject* ancestor_container) const {
  if (ancestor_container == this)
    return LayoutSize();

  LayoutSize offset;
  LayoutPoint reference_point;
  const LayoutObject* curr_container = this;
  do {
    const LayoutObject* next_container = curr_container->Container();
    // This means we reached the top without finding container.
    DCHECK(next_container);
    if (!next_container)
      break;
    DCHECK(!curr_container->HasTransformRelatedProperty());
    LayoutSize current_offset =
        curr_container->OffsetFromContainer(next_container);
    offset += current_offset;
    reference_point.Move(current_offset);
    curr_container = next_container;
  } while (curr_container != ancestor_container);

  return offset;
}

LayoutRect LayoutObject::LocalCaretRect(
    const InlineBox*,
    int,
    LayoutUnit* extra_width_to_end_of_line) const {
  if (extra_width_to_end_of_line)
    *extra_width_to_end_of_line = LayoutUnit();

  return LayoutRect();
}

void LayoutObject::ComputeLayerHitTestRects(
    LayerHitTestRects& layer_rects,
    TouchAction supported_fast_actions) const {
  // Figure out what layer our container is in. Any offset (or new layer) for
  // this layoutObject within it's container will be applied in
  // addLayerHitTestRects.
  LayoutPoint layer_offset;
  const PaintLayer* current_layer = nullptr;

  if (!HasLayer()) {
    LayoutObject* container = Container();
    if (container) {
      current_layer = container->EnclosingLayer();
      if (current_layer->GetLayoutObject() != container) {
        layer_offset.Move(container->OffsetFromAncestorContainer(
            &current_layer->GetLayoutObject()));
        // If the layer itself is scrolled, we have to undo the subtraction of
        // its scroll offset since we want the offset relative to the scrolling
        // content, not the element itself.
        if (current_layer->GetLayoutObject().HasOverflowClip()) {
          layer_offset.Move(
              current_layer->GetLayoutBox()->ScrolledContentOffset());
        }
      }
    }
  }

  AddLayerHitTestRects(layer_rects, current_layer, layer_offset,
                       supported_fast_actions, LayoutRect(),
                       TouchAction::kTouchActionAuto);
}

void LayoutObject::AddLayerHitTestRects(
    LayerHitTestRects& layer_rects,
    const PaintLayer* current_layer,
    const LayoutPoint& layer_offset,
    TouchAction supported_fast_actions,
    const LayoutRect& container_rect,
    TouchAction container_whitelisted_touch_action) const {
  DCHECK(current_layer);
  DCHECK_EQ(current_layer, EnclosingLayer());

  // Compute the rects for this layoutObject only and add them to the results.
  // Note that we could avoid passing the offset and instead adjust each result,
  // but this seems slightly simpler.
  Vector<LayoutRect> own_rects;
  LayoutRect new_container_rect;
  TouchAction new_container_whitelisted_touch_action =
      TouchAction::kTouchActionAuto;
  ComputeSelfHitTestRects(own_rects, layer_offset);

  // When we get to have a lot of rects on a layer, the performance cost of
  // tracking those rects outweighs the benefit of doing compositor thread hit
  // testing.
  // FIXME: This limit needs to be low due to the O(n^2) algorithm in
  // ScrollingCoordinator::SetTouchEventTargetRects() - crbug.com/300282.
  const size_t kMaxRectsPerLayer = 100;

  LayerHitTestRects::iterator iter = layer_rects.find(current_layer);
  Vector<TouchActionRect>* iter_value;
  if (iter == layer_rects.end()) {
    iter_value = &layer_rects.insert(current_layer, Vector<TouchActionRect>())
                      .stored_value->value;
  } else {
    iter_value = &iter->value;
  }
  TouchAction whitelisted_touch_action =
      Style()->GetEffectiveTouchAction() & supported_fast_actions;
  for (size_t i = 0; i < own_rects.size(); i++) {
    // If we have a different touch action than the container the rect needs to
    // be reported even if it is contained.
    if (whitelisted_touch_action != container_whitelisted_touch_action ||
        !container_rect.Contains(own_rects[i])) {
      iter_value->push_back(
          TouchActionRect(own_rects[i], whitelisted_touch_action));
      if (iter_value->size() > kMaxRectsPerLayer) {
        // Just mark the entire layer instead, and switch to walking the layer
        // tree instead of the layout tree.
        layer_rects.erase(current_layer);
        current_layer->AddLayerHitTestRects(layer_rects,
                                            supported_fast_actions);
        return;
      }
      if (new_container_rect.IsEmpty()) {
        new_container_whitelisted_touch_action = whitelisted_touch_action;
        new_container_rect = own_rects[i];
      }
    }
  }
  if (new_container_rect.IsEmpty()) {
    new_container_whitelisted_touch_action = container_whitelisted_touch_action;
    new_container_rect = container_rect;
  }

  // If it's possible for children to have rects outside our bounds, then we
  // need to descend into the children and compute them.
  // Ideally there would be other cases where we could detect that children
  // couldn't have rects outside our bounds and prune the tree walk.
  // Note that we don't use Region here because Union is O(N) - better to just
  // keep a list of partially redundant rectangles. If we find examples where
  // this is expensive, then we could rewrite Region to be more efficient. See
  // https://bugs.webkit.org/show_bug.cgi?id=100814.
  if (!IsLayoutView()) {
    for (LayoutObject* curr = SlowFirstChild(); curr;
         curr = curr->NextSibling()) {
      curr->AddLayerHitTestRects(layer_rects, current_layer, layer_offset,
                                 supported_fast_actions, new_container_rect,
                                 new_container_whitelisted_touch_action);
    }
  }
}

bool LayoutObject::IsRooted() const {
  const LayoutObject* object = this;
  while (object->Parent() && !object->HasLayer())
    object = object->Parent();
  if (object->HasLayer())
    return ToLayoutBoxModelObject(object)->Layer()->Root()->IsRootLayer();
  return false;
}

RespectImageOrientationEnum LayoutObject::ShouldRespectImageOrientation(
    const LayoutObject* layout_object) {
  if (!layout_object)
    return kDoNotRespectImageOrientation;

  // Respect the image's orientation if it's being used as a full-page image or
  // it's an <img> and the setting to respect it everywhere is set or the <img>
  // has image-orientation: from-image style. FIXME: crbug.com/498233
  if (layout_object->GetDocument().IsImageDocument())
    return kRespectImageOrientation;

  if (!IsHTMLImageElement(layout_object->GetNode()))
    return kDoNotRespectImageOrientation;

  if (layout_object->GetDocument().GetSettings() &&
      layout_object->GetDocument()
          .GetSettings()
          ->GetShouldRespectImageOrientation())
    return kRespectImageOrientation;

  if (layout_object->Style() &&
      layout_object->Style()->RespectImageOrientation() ==
          kRespectImageOrientation)
    return kRespectImageOrientation;

  return kDoNotRespectImageOrientation;
}

LayoutObject* LayoutObject::Container(AncestorSkipInfo* skip_info) const {
  // TODO(mstensho): Get rid of this. Nobody should call this method with those
  // flags already set.
  if (skip_info)
    skip_info->ResetOutput();

  if (IsTextOrSVGChild())
    return Parent();

  EPosition pos = style_->GetPosition();
  if (pos == EPosition::kFixed)
    return ContainerForFixedPosition(skip_info);

  if (pos == EPosition::kAbsolute) {
    return ContainerForAbsolutePosition(skip_info);
  }

  if (IsColumnSpanAll()) {
    LayoutObject* multicol_container = SpannerPlaceholder()->Container();
    if (skip_info) {
      // We jumped directly from the spanner to the multicol container. Need to
      // check if we skipped |ancestor| or filter/reflection on the way.
      for (LayoutObject* walker = Parent();
           walker && walker != multicol_container; walker = walker->Parent())
        skip_info->Update(*walker);
    }
    return multicol_container;
  }

  if (IsFloating())
    return ContainingBlock(skip_info);

  return Parent();
}

inline LayoutObject* LayoutObject::ParentCrossingFrames() const {
  if (IsLayoutView())
    return GetFrame()->OwnerLayoutObject();
  return Parent();
}

bool LayoutObject::IsSelectionBorder() const {
  SelectionState st = GetSelectionState();
  return st == SelectionState::kStart || st == SelectionState::kEnd ||
         st == SelectionState::kStartAndEnd;
}

inline void LayoutObject::ClearLayoutRootIfNeeded() const {
  if (LocalFrameView* view = GetFrameView()) {
    if (!DocumentBeingDestroyed())
      view->ClearLayoutSubtreeRoot(*this);
  }
}

void LayoutObject::WillBeDestroyed() {
  // Destroy any leftover anonymous children.
  LayoutObjectChildList* children = VirtualChildren();
  if (children)
    children->DestroyLeftoverChildren();

  if (LocalFrame* frame = GetFrame()) {
    // If this layoutObject is being autoscrolled, stop the autoscrolling.
    if (frame->GetPage())
      frame->GetPage()->GetAutoscrollController().StopAutoscrollIfNeeded(this);
  }

  // For accessibility management, notify the parent of the imminent change to
  // its child set.
  // We do it now, before remove(), while the parent pointer is still available.
  if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache())
    cache->ChildrenChanged(Parent());

  Remove();

  // The remove() call above may invoke axObjectCache()->childrenChanged() on
  // the parent, which may require the AX layout object for this layoutObject.
  // So we remove the AX layout object now, after the layoutObject is removed.
  if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache())
    cache->Remove(this);

  // If this layoutObject had a parent, remove should have destroyed any
  // counters attached to this layoutObject and marked the affected other
  // counters for reevaluation. This apparently redundant check is here for the
  // case when this layoutObject had no parent at the time remove() was called.

  if (HasCounterNodeMap())
    LayoutCounter::DestroyCounterNodes(*this);

  // Remove the handler if node had touch-action set. Handlers are not added
  // for text nodes so don't try removing for one too. Need to check if
  // m_style is null in cases of partial construction. Any handler we added
  // previously may have already been removed by the Document independently.
  if (GetNode() && !GetNode()->IsTextNode() && style_ &&
      style_->GetTouchAction() != TouchAction::kTouchActionAuto) {
    EventHandlerRegistry& registry =
        GetDocument().GetFrame()->GetEventHandlerRegistry();
    if (registry.EventHandlerTargets(EventHandlerRegistry::kTouchAction)
            ->Contains(GetNode())) {
      registry.DidRemoveEventHandler(*GetNode(),
                                     EventHandlerRegistry::kTouchAction);
    }
  }

  SetAncestorLineBoxDirty(false);

  ClearLayoutRootIfNeeded();

  if (style_) {
    for (const FillLayer* bg_layer = &style_->BackgroundLayers(); bg_layer;
         bg_layer = bg_layer->Next()) {
      if (StyleImage* background_image = bg_layer->GetImage())
        background_image->RemoveClient(this);
    }

    for (const FillLayer* mask_layer = &style_->MaskLayers(); mask_layer;
         mask_layer = mask_layer->Next()) {
      if (StyleImage* mask_image = mask_layer->GetImage())
        mask_image->RemoveClient(this);
    }

    if (StyleImage* border_image = style_->BorderImage().GetImage())
      border_image->RemoveClient(this);

    if (StyleImage* mask_box_image = style_->MaskBoxImage().GetImage())
      mask_box_image->RemoveClient(this);

    if (style_->GetContentData() && style_->GetContentData()->IsImage())
      ToImageContentData(style_->GetContentData())
          ->GetImage()
          ->RemoveClient(this);

    if (style_->BoxReflect() && style_->BoxReflect()->Mask().GetImage())
      style_->BoxReflect()->Mask().GetImage()->RemoveClient(this);

    RemoveShapeImageClient(style_->ShapeOutside());
    RemoveCursorImageClient(style_->Cursors());
  }

  if (GetFrameView())
    SetIsBackgroundAttachmentFixedObject(false);
}

DISABLE_CFI_PERF
void LayoutObject::InsertedIntoTree() {
  // FIXME: We should DCHECK(isRooted()) here but generated content makes some
  // out-of-order insertion.

  // Keep our layer hierarchy updated. Optimize for the common case where we
  // don't have any children and don't have a layer attached to ourselves.
  PaintLayer* layer = nullptr;
  if (SlowFirstChild() || HasLayer()) {
    layer = Parent()->EnclosingLayer();
    AddLayers(layer);
  }

  // If |this| is visible but this object was not, tell the layer it has some
  // visible content that needs to be drawn and layer visibility optimization
  // can't be used
  if (Parent()->Style()->Visibility() != EVisibility::kVisible &&
      Style()->Visibility() == EVisibility::kVisible && !HasLayer()) {
    if (!layer)
      layer = Parent()->EnclosingLayer();
    if (layer)
      layer->DirtyVisibleContentStatus();
  }

  if (Parent()->ChildrenInline())
    Parent()->DirtyLinesFromChangedChild(this);

  if (LayoutFlowThread* flow_thread = FlowThreadContainingBlock())
    flow_thread->FlowThreadDescendantWasInserted(this);
}

enum FindReferencingScrollAnchorsBehavior { kDontClear, kClear };

static bool FindReferencingScrollAnchors(
    LayoutObject* layout_object,
    FindReferencingScrollAnchorsBehavior behavior) {
  PaintLayer* layer = nullptr;
  if (LayoutObject* parent = layout_object->Parent())
    layer = parent->EnclosingLayer();
  bool found = false;

  // Walk up the layer tree to clear any scroll anchors that reference us.
  while (layer) {
    if (PaintLayerScrollableArea* scrollable_area =
            layer->GetScrollableArea()) {
      ScrollAnchor* anchor = scrollable_area->GetScrollAnchor();
      DCHECK(anchor);
      if (anchor->RefersTo(layout_object)) {
        found = true;
        if (behavior == kClear)
          anchor->NotifyRemoved(layout_object);
        else
          return true;
      }
    }
    layer = layer->Parent();
  }
  if (LocalFrameView* view = layout_object->GetFrameView()) {
    ScrollAnchor* anchor = view->GetScrollAnchor();
    DCHECK(anchor);
    if (anchor->RefersTo(layout_object)) {
      found = true;
      if (behavior == kClear)
        anchor->NotifyRemoved(layout_object);
    }
  }
  return found;
}

void LayoutObject::WillBeRemovedFromTree() {
  // FIXME: We should DCHECK(isRooted()) but we have some out-of-order removals
  // which would need to be fixed first.

  // If we remove a visible child from an invisible parent, we don't know the
  // layer visibility any more.
  PaintLayer* layer = nullptr;
  if (Parent()->Style()->Visibility() != EVisibility::kVisible &&
      Style()->Visibility() == EVisibility::kVisible && !HasLayer()) {
    layer = Parent()->EnclosingLayer();
    if (layer)
      layer->DirtyVisibleContentStatus();
  }

  // Keep our layer hierarchy updated.
  if (SlowFirstChild() || HasLayer()) {
    if (!layer)
      layer = Parent()->EnclosingLayer();
    RemoveLayers(layer);
  }

  if (IsOutOfFlowPositioned() && Parent()->ChildrenInline())
    Parent()->DirtyLinesFromChangedChild(this);

  RemoveFromLayoutFlowThread();

  // Update cached boundaries in SVG layoutObjects if a child is removed.
  if (Parent()->IsSVG())
    Parent()->SetNeedsBoundariesUpdate();

  if (bitfields_.IsScrollAnchorObject()) {
    // Clear the bit first so that anchor.clear() doesn't recurse into
    // findReferencingScrollAnchors.
    bitfields_.SetIsScrollAnchorObject(false);
    FindReferencingScrollAnchors(this, kClear);
  }
}

void LayoutObject::SetNeedsPaintPropertyUpdate() {
  bitfields_.SetNeedsPaintPropertyUpdate(true);

  LayoutObject* ancestor = ParentCrossingFrames();
  while (ancestor && !ancestor->DescendantNeedsPaintPropertyUpdate()) {
    ancestor->bitfields_.SetDescendantNeedsPaintPropertyUpdate(true);
    ancestor = ancestor->ParentCrossingFrames();
  }
}

void LayoutObject::SetAncestorsNeedPaintPropertyUpdateForMainThreadScrolling() {
  LayoutObject* ancestor = ParentCrossingFrames();
  while (ancestor) {
    ancestor->SetNeedsPaintPropertyUpdate();
    ancestor = ancestor->ParentCrossingFrames();
  }
}

void LayoutObject::MaybeClearIsScrollAnchorObject() {
  if (!bitfields_.IsScrollAnchorObject())
    return;
  bitfields_.SetIsScrollAnchorObject(
      FindReferencingScrollAnchors(this, kDontClear));
}

void LayoutObject::RemoveFromLayoutFlowThread() {
  if (!IsInsideFlowThread())
    return;

  // Sometimes we remove the element from the flow, but it's not destroyed at
  // that time.
  // It's only until later when we actually destroy it and remove all the
  // children from it.
  // Currently, that happens for firstLetter elements and list markers.
  // Pass in the flow thread so that we don't have to look it up for all the
  // children.
  // If we're a column spanner, we need to use our parent to find the flow
  // thread, since a spanner doesn't have the flow thread in its containing
  // block chain. We still need to notify the flow thread when the layoutObject
  // removed happens to be a spanner, so that we get rid of the spanner
  // placeholder, and column sets around the placeholder get merged.
  LayoutFlowThread* flow_thread = IsColumnSpanAll()
                                      ? Parent()->FlowThreadContainingBlock()
                                      : FlowThreadContainingBlock();
  RemoveFromLayoutFlowThreadRecursive(flow_thread);
}

void LayoutObject::RemoveFromLayoutFlowThreadRecursive(
    LayoutFlowThread* layout_flow_thread) {
  if (const LayoutObjectChildList* children = VirtualChildren()) {
    for (LayoutObject* child = children->FirstChild(); child;
         child = child->NextSibling()) {
      if (child->IsLayoutFlowThread())
        continue;  // Don't descend into inner fragmentation contexts.
      child->RemoveFromLayoutFlowThreadRecursive(child->IsLayoutFlowThread()
                                                     ? ToLayoutFlowThread(child)
                                                     : layout_flow_thread);
    }
  }

  if (layout_flow_thread && layout_flow_thread != this)
    layout_flow_thread->FlowThreadDescendantWillBeRemoved(this);
  SetIsInsideFlowThread(false);
  CHECK(!SpannerPlaceholder());
}

void LayoutObject::DestroyAndCleanupAnonymousWrappers() {
  // If the tree is destroyed, there is no need for a clean-up phase.
  if (DocumentBeingDestroyed()) {
    Destroy();
    return;
  }

  LayoutObject* destroy_root = this;
  for (LayoutObject *destroy_root_parent = destroy_root->Parent();
       destroy_root_parent && destroy_root_parent->IsAnonymous();
       destroy_root = destroy_root_parent,
                    destroy_root_parent = destroy_root_parent->Parent()) {
    // Anonymous block continuations are tracked and destroyed elsewhere (see
    // the bottom of LayoutBlockFlow::RemoveChild)
    if (destroy_root_parent->IsLayoutBlockFlow() &&
        ToLayoutBlockFlow(destroy_root_parent)->IsAnonymousBlockContinuation())
      break;
    // A flow thread is tracked by its containing block. Whether its children
    // are removed or not is irrelevant.
    if (destroy_root_parent->IsLayoutFlowThread())
      break;

    if (destroy_root->PreviousSibling() || destroy_root->NextSibling())
      break;  // Need to keep the anonymous parent, since it won't become empty
              // by the removal of this LayoutObject.
  }

  destroy_root->Destroy();

  // WARNING: |this| is deleted here.
}

void LayoutObject::Destroy() {
  WillBeDestroyed();
  delete this;
}

DISABLE_CFI_PERF
void LayoutObject::RemoveShapeImageClient(ShapeValue* shape_value) {
  if (!shape_value)
    return;
  if (StyleImage* shape_image = shape_value->GetImage())
    shape_image->RemoveClient(this);
}

void LayoutObject::RemoveCursorImageClient(const CursorList* cursor_list) {
  if (!cursor_list)
    return;
  for (const CursorData& cursor : *cursor_list) {
    if (cursor.GetImage())
      cursor.GetImage()->RemoveClient(this);
  }
}

PositionWithAffinity LayoutObject::PositionForPoint(const LayoutPoint&) const {
  return CreatePositionWithAffinity(CaretMinOffset());
}

CompositingState LayoutObject::GetCompositingState() const {
  return HasLayer()
             ? ToLayoutBoxModelObject(this)->Layer()->GetCompositingState()
             : kNotComposited;
}

CompositingReasons LayoutObject::AdditionalCompositingReasons() const {
  return CompositingReason::kNone;
}

bool LayoutObject::HitTestAllPhases(
    HitTestResult& result,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    HitTestFilter hit_test_filter) {
  bool inside = false;
  if (hit_test_filter != kHitTestSelf) {
    // First test the foreground layer (lines and inlines).
    inside = NodeAtPoint(result, location_in_container, accumulated_offset,
                         kHitTestForeground);

    // Test floats next.
    if (!inside)
      inside = NodeAtPoint(result, location_in_container, accumulated_offset,
                           kHitTestFloat);

    // Finally test to see if the mouse is in the background (within a child
    // block's background).
    if (!inside)
      inside = NodeAtPoint(result, location_in_container, accumulated_offset,
                           kHitTestChildBlockBackgrounds);
  }

  // See if the mouse is inside us but not any of our descendants
  if (hit_test_filter != kHitTestDescendants && !inside)
    inside = NodeAtPoint(result, location_in_container, accumulated_offset,
                         kHitTestBlockBackground);

  return inside;
}

Node* LayoutObject::NodeForHitTest() const {
  Node* node = GetNode();

  // If we hit the anonymous layoutObjects inside generated content we should
  // actually hit the generated content so walk up to the PseudoElement.
  if (!node && Parent() && Parent()->IsBeforeOrAfterContent()) {
    for (LayoutObject* layout_object = Parent(); layout_object && !node;
         layout_object = layout_object->Parent())
      node = layout_object->GetNode();
  }

  return node;
}

void LayoutObject::UpdateHitTestResult(HitTestResult& result,
                                       const LayoutPoint& point) {
  if (result.InnerNode())
    return;

  if (Node* n = NodeForHitTest())
    result.SetNodeAndPosition(n, point);
}

bool LayoutObject::NodeAtPoint(HitTestResult&,
                               const HitTestLocation& /*locationInContainer*/,
                               const LayoutPoint& /*accumulatedOffset*/,
                               HitTestAction) {
  return false;
}

void LayoutObject::ScheduleRelayout() {
  if (IsLayoutView()) {
    LocalFrameView* view = ToLayoutView(this)->GetFrameView();
    if (view)
      view->ScheduleRelayout();
  } else {
    if (IsRooted()) {
      if (LayoutView* layout_view = View()) {
        if (LocalFrameView* frame_view = layout_view->GetFrameView())
          frame_view->ScheduleRelayoutOfSubtree(this);
      }
    }
  }
}

void LayoutObject::ForceLayout() {
  SetSelfNeedsLayout(true);
  MarkContainerNeedsCollectInlines();
  SetShouldDoFullPaintInvalidation();
  UpdateLayout();
}

// FIXME: Does this do anything different than forceLayout given that we don't
// walk the containing block chain. If not, we should change all callers to use
// forceLayout.
void LayoutObject::ForceChildLayout() {
  SetNormalChildNeedsLayout(true);
  MarkContainerNeedsCollectInlines();
  UpdateLayout();
}

enum StyleCacheState { kCached, kUncached };

static scoped_refptr<ComputedStyle> FirstLineStyleForCachedUncachedType(
    StyleCacheState type,
    const LayoutObject* layout_object,
    ComputedStyle* style) {
  const LayoutObject* layout_object_for_first_line_style = layout_object;
  if (layout_object->IsBeforeOrAfterContent())
    layout_object_for_first_line_style = layout_object->Parent();

  if (layout_object_for_first_line_style->BehavesLikeBlockContainer()) {
    if (const LayoutBlock* first_line_block =
            ToLayoutBlock(layout_object_for_first_line_style)
                ->EnclosingFirstLineStyleBlock()) {
      if (type == kCached)
        return first_line_block->GetCachedPseudoStyle(kPseudoIdFirstLine,
                                                      style);
      return first_line_block->GetUncachedPseudoStyle(
          PseudoStyleRequest(kPseudoIdFirstLine), style);
    }
  } else if (!layout_object_for_first_line_style->IsAnonymous() &&
             layout_object_for_first_line_style->IsLayoutInline() &&
             !layout_object_for_first_line_style->GetNode()
                  ->IsFirstLetterPseudoElement()) {
    const ComputedStyle* parent_style =
        layout_object_for_first_line_style->Parent()->FirstLineStyle();
    if (parent_style != layout_object_for_first_line_style->Parent()->Style()) {
      if (type == kCached) {
        // A first-line style is in effect. Cache a first-line style for
        // ourselves.
        return layout_object_for_first_line_style->GetCachedPseudoStyle(
            kPseudoIdFirstLineInherited, parent_style);
      }
      return layout_object_for_first_line_style->GetUncachedPseudoStyle(
          PseudoStyleRequest(kPseudoIdFirstLineInherited), parent_style);
    }
  }
  return nullptr;
}

scoped_refptr<ComputedStyle> LayoutObject::UncachedFirstLineStyle() const {
  if (!GetDocument().GetStyleEngine().UsesFirstLineRules())
    return nullptr;

  DCHECK(!IsText());

  return FirstLineStyleForCachedUncachedType(kUncached, this, style_.get());
}

ComputedStyle* LayoutObject::CachedFirstLineStyle() const {
  DCHECK(GetDocument().GetStyleEngine().UsesFirstLineRules());

  if (scoped_refptr<ComputedStyle> style = FirstLineStyleForCachedUncachedType(
          kCached, IsText() ? Parent() : this, style_.get()))
    return style.get();

  return style_.get();
}

ComputedStyle* LayoutObject::GetCachedPseudoStyle(
    PseudoId pseudo,
    const ComputedStyle* parent_style) const {
  DCHECK_NE(pseudo, kPseudoIdBefore);
  DCHECK_NE(pseudo, kPseudoIdAfter);
  if (!GetNode())
    return nullptr;

  Element* element = Traversal<Element>::FirstAncestorOrSelf(*GetNode());
  if (!element)
    return nullptr;

  return element->PseudoStyle(PseudoStyleRequest(pseudo), parent_style);
}

scoped_refptr<ComputedStyle> LayoutObject::GetUncachedPseudoStyle(
    const PseudoStyleRequest& request,
    const ComputedStyle* parent_style) const {
  DCHECK_NE(request.pseudo_id, kPseudoIdBefore);
  DCHECK_NE(request.pseudo_id, kPseudoIdAfter);
  if (!GetNode())
    return nullptr;

  Element* element = Traversal<Element>::FirstAncestorOrSelf(*GetNode());
  if (!element)
    return nullptr;
  if (element->IsPseudoElement())
    return nullptr;

  return element->GetUncachedPseudoStyle(request, parent_style);
}

void LayoutObject::AddAnnotatedRegions(Vector<AnnotatedRegionValue>& regions) {
  // Convert the style regions to absolute coordinates.
  if (Style()->Visibility() != EVisibility::kVisible || !IsBox())
    return;

  if (Style()->DraggableRegionMode() == EDraggableRegionMode::kNone)
    return;

  LayoutBox* box = ToLayoutBox(this);
  FloatRect local_bounds(FloatPoint(), FloatSize(box->Size()));
  FloatRect abs_bounds = LocalToAbsoluteQuad(local_bounds).BoundingBox();

  AnnotatedRegionValue region;
  region.draggable =
      Style()->DraggableRegionMode() == EDraggableRegionMode::kDrag;
  region.bounds = LayoutRect(abs_bounds);
  regions.push_back(region);
}

bool LayoutObject::WillRenderImage() {
  // Without visibility we won't render (and therefore don't care about
  // animation).
  if (Style()->Visibility() != EVisibility::kVisible)
    return false;

  // We will not render a new image when PausableObjects is paused
  if (GetDocument().IsContextPaused())
    return false;

  // If we're not in a window (i.e., we're dormant from being in a background
  // tab) then we don't want to render either.
  return GetDocument().View()->IsVisible();
}

bool LayoutObject::GetImageAnimationPolicy(ImageAnimationPolicy& policy) {
  if (!GetDocument().GetSettings())
    return false;
  policy = GetDocument().GetSettings()->GetImageAnimationPolicy();
  return true;
}

int LayoutObject::CaretMinOffset() const {
  return 0;
}

int LayoutObject::CaretMaxOffset() const {
  if (IsAtomicInlineLevel())
    return GetNode() ? std::max(1U, GetNode()->CountChildren()) : 1;
  if (IsHR())
    return 1;
  return 0;
}

bool LayoutObject::IsInert() const {
  const LayoutObject* layout_object = this;
  while (!layout_object->GetNode())
    layout_object = layout_object->Parent();
  return layout_object->GetNode()->IsInert();
}

void LayoutObject::ImageChanged(ImageResourceContent* image,
                                CanDeferInvalidation defer,
                                const IntRect* rect) {
  DCHECK(node_);

  // Image change notifications should not be received during paint because
  // the resulting invalidations will be cleared following paint. This can also
  // lead to modifying the tree out from under paint(), see: crbug.com/616700.
  DCHECK_NE(GetDocument().Lifecycle().GetState(),
            DocumentLifecycle::LifecycleState::kInPaint);

  ImageChanged(static_cast<WrappedImagePtr>(image), defer, rect);
}

Element* LayoutObject::OffsetParent(const Element* base) const {
  if (IsDocumentElement() || IsBody())
    return nullptr;

  if (IsFixedPositioned())
    return nullptr;

  float effective_zoom = Style()->EffectiveZoom();
  Node* node = nullptr;
  for (LayoutObject* ancestor = Parent(); ancestor;
       ancestor = ancestor->Parent()) {
    // Spec: http://www.w3.org/TR/cssom-view/#offset-attributes

    node = ancestor->GetNode();

    if (!node)
      continue;

    // TODO(kochi): If |base| or |node| is nested deep in shadow roots, this
    // loop may get expensive, as isUnclosedNodeOf() can take up to O(N+M) time
    // (N and M are depths).
    if (base && (node->IsClosedShadowHiddenFrom(*base) ||
                 (node->IsInShadowTree() &&
                  node->ContainingShadowRoot()->IsUserAgent()))) {
      // If 'position: fixed' node is found while traversing up, terminate the
      // loop and return null.
      if (ancestor->IsFixedPositioned())
        return nullptr;
      continue;
    }

    if (ancestor->CanContainAbsolutePositionObjects())
      break;

    if (IsHTMLBodyElement(*node))
      break;

    if (!IsPositioned() &&
        (IsHTMLTableElement(*node) || IsHTMLTableCellElement(*node)))
      break;

    // Webkit specific extension where offsetParent stops at zoom level changes.
    if (effective_zoom != ancestor->Style()->EffectiveZoom())
      break;
  }

  return node && node->IsElementNode() ? ToElement(node) : nullptr;
}

PositionWithAffinity LayoutObject::CreatePositionWithAffinity(
    int offset,
    TextAffinity affinity) const {
  // If this is a non-anonymous layoutObject in an editable area, then it's
  // simple.
  if (Node* node = NonPseudoNode()) {
    if (!HasEditableStyle(*node)) {
      // If it can be found, we prefer a visually equivalent position that is
      // editable.
      // TODO(layout-dev): Once we fix callers of |CreatePositionWithAffinity()|
      // we should use |Position| constructor. See http://crbug.com/827923
      const Position position =
          Position::CreateWithoutValidationDeprecated(*node, offset);
      Position candidate =
          MostForwardCaretPosition(position, kCanCrossEditingBoundary);
      if (HasEditableStyle(*candidate.AnchorNode()))
        return PositionWithAffinity(candidate, affinity);
      candidate = MostBackwardCaretPosition(position, kCanCrossEditingBoundary);
      if (HasEditableStyle(*candidate.AnchorNode()))
        return PositionWithAffinity(candidate, affinity);
    }
    // FIXME: Eliminate legacy editing positions
    return PositionWithAffinity(Position::EditingPositionOf(node, offset),
                                affinity);
  }

  // We don't want to cross the boundary between editable and non-editable
  // regions of the document, but that is either impossible or at least
  // extremely unlikely in any normal case because we stop as soon as we
  // find a single non-anonymous layoutObject.

  // Find a nearby non-anonymous layoutObject.
  const LayoutObject* child = this;
  while (const LayoutObject* parent = child->Parent()) {
    // Find non-anonymous content after.
    for (const LayoutObject* layout_object = child->NextInPreOrder(parent);
         layout_object; layout_object = layout_object->NextInPreOrder(parent)) {
      if (const Node* node = layout_object->NonPseudoNode()) {
        return PositionWithAffinity(FirstPositionInOrBeforeNode(*node));
      }
    }

    // Find non-anonymous content before.
    for (const LayoutObject* layout_object = child->PreviousInPreOrder();
         layout_object; layout_object = layout_object->PreviousInPreOrder()) {
      if (layout_object == parent)
        break;
      if (const Node* node = layout_object->NonPseudoNode())
        return PositionWithAffinity(LastPositionInOrAfterNode(*node));
    }

    // Use the parent itself unless it too is anonymous.
    if (const Node* node = parent->NonPseudoNode())
      return PositionWithAffinity(FirstPositionInOrBeforeNode(*node));

    // Repeat at the next level up.
    child = parent;
  }

  // Everything was anonymous. Give up.
  return PositionWithAffinity();
}

PositionWithAffinity LayoutObject::CreatePositionWithAffinity(
    int offset) const {
  return CreatePositionWithAffinity(offset, TextAffinity::kDownstream);
}

PositionWithAffinity LayoutObject::CreatePositionWithAffinity(
    const Position& position) const {
  if (position.IsNotNull())
    return PositionWithAffinity(position);

  DCHECK(!GetNode());
  return CreatePositionWithAffinity(0);
}

CursorDirective LayoutObject::GetCursor(const LayoutPoint&, Cursor&) const {
  return kSetCursorBasedOnStyle;
}

bool LayoutObject::CanUpdateSelectionOnRootLineBoxes() const {
  if (NeedsLayout())
    return false;

  const LayoutBlock* containing_block = ContainingBlock();
  return containing_block ? !containing_block->NeedsLayout() : false;
}

void LayoutObject::SetNeedsBoundariesUpdate() {
  if (IsSVGChild()) {
    // The boundaries affect mask clip.
    auto* resources = SVGResourcesCache::CachedResourcesForLayoutObject(*this);
    if (resources && resources->Masker())
      SetNeedsPaintPropertyUpdate();
    if (resources && resources->Clipper())
      InvalidateClipPathCache();
  }
  if (LayoutObject* layout_object = Parent())
    layout_object->SetNeedsBoundariesUpdate();
}

FloatRect LayoutObject::ObjectBoundingBox() const {
  NOTREACHED();
  return FloatRect();
}

FloatRect LayoutObject::StrokeBoundingBox() const {
  NOTREACHED();
  return FloatRect();
}

FloatRect LayoutObject::VisualRectInLocalSVGCoordinates() const {
  NOTREACHED();
  return FloatRect();
}

AffineTransform LayoutObject::LocalSVGTransform() const {
  return AffineTransform();
}

bool LayoutObject::NodeAtFloatPoint(HitTestResult&,
                                    const FloatPoint&,
                                    HitTestAction) {
  NOTREACHED();
  return false;
}

bool LayoutObject::IsRelayoutBoundaryForInspector() const {
  return ObjectIsRelayoutBoundary(this);
}

static PaintInvalidationReason DocumentLifecycleBasedPaintInvalidationReason(
    const DocumentLifecycle& document_lifecycle) {
  switch (document_lifecycle.GetState()) {
    case DocumentLifecycle::kInStyleRecalc:
      return PaintInvalidationReason::kStyle;
    case DocumentLifecycle::kInPreLayout:
    case DocumentLifecycle::kInPerformLayout:
    case DocumentLifecycle::kAfterPerformLayout:
      return PaintInvalidationReason::kGeometry;
    case DocumentLifecycle::kInCompositingUpdate:
      return PaintInvalidationReason::kCompositing;
    default:
      return PaintInvalidationReason::kFull;
  }
}

inline void LayoutObject::MarkAncestorsForPaintInvalidation() {
  for (LayoutObject* parent = ParentCrossingFrames();
       parent && !parent->ShouldCheckForPaintInvalidation();
       parent = parent->ParentCrossingFrames())
    parent->bitfields_.SetMayNeedPaintInvalidation(true);
}

inline void LayoutObject::SetNeedsPaintOffsetAndVisualRectUpdate() {
  for (auto* object = this;
       object && !object->NeedsPaintOffsetAndVisualRectUpdate();
       object = object->ParentCrossingFrames()) {
    object->bitfields_.SetNeedsPaintOffsetAndVisualRectUpdate(true);
  }
}

void LayoutObject::SetShouldInvalidateSelection() {
  if (!CanUpdateSelectionOnRootLineBoxes())
    return;
  bitfields_.SetShouldInvalidateSelection(true);
  SetMayNeedPaintInvalidation();
  GetFrameView()->ScheduleVisualUpdateForPaintInvalidationIfNeeded();
}

void LayoutObject::SetShouldDoFullPaintInvalidation(
    PaintInvalidationReason reason) {
  SetNeedsPaintOffsetAndVisualRectUpdate();
  SetShouldDoFullPaintInvalidationWithoutGeometryChange(reason);
}

void LayoutObject::SetShouldDoFullPaintInvalidationWithoutGeometryChange(
    PaintInvalidationReason reason) {
  // Only full invalidation reasons are allowed.
  DCHECK(IsFullPaintInvalidationReason(reason));

  bool is_upgrading_delayed_full_to_full =
      bitfields_.FullPaintInvalidationReason() ==
          PaintInvalidationReason::kDelayedFull &&
      reason != PaintInvalidationReason::kDelayedFull;

  if (bitfields_.FullPaintInvalidationReason() ==
          PaintInvalidationReason::kNone ||
      is_upgrading_delayed_full_to_full) {
    if (reason == PaintInvalidationReason::kFull) {
      reason = DocumentLifecycleBasedPaintInvalidationReason(
          GetDocument().Lifecycle());
    }
    bitfields_.SetFullPaintInvalidationReason(reason);
    if (!is_upgrading_delayed_full_to_full)
      MarkAncestorsForPaintInvalidation();
  }

  GetFrameView()->ScheduleVisualUpdateForPaintInvalidationIfNeeded();
}

void LayoutObject::SetMayNeedPaintInvalidation() {
  SetNeedsPaintOffsetAndVisualRectUpdate();
  SetMayNeedPaintInvalidationWithoutGeometryChange();
}

void LayoutObject::SetMayNeedPaintInvalidationWithoutGeometryChange() {
  if (MayNeedPaintInvalidation())
    return;
  bitfields_.SetMayNeedPaintInvalidation(true);
  MarkAncestorsForPaintInvalidation();
  GetFrameView()->ScheduleVisualUpdateForPaintInvalidationIfNeeded();
}

void LayoutObject::SetMayNeedPaintInvalidationSubtree() {
  if (MayNeedPaintInvalidationSubtree())
    return;
  bitfields_.SetMayNeedPaintInvalidationSubtree(true);
  SetMayNeedPaintInvalidation();
}

void LayoutObject::SetMayNeedPaintInvalidationAnimatedBackgroundImage() {
  if (MayNeedPaintInvalidationAnimatedBackgroundImage())
    return;
  bitfields_.SetMayNeedPaintInvalidationAnimatedBackgroundImage(true);
  SetMayNeedPaintInvalidationWithoutGeometryChange();
}

void LayoutObject::ClearPaintInvalidationFlags() {
// PaintInvalidationStateIsDirty should be kept in sync with the
// booleans that are cleared below.
#if DCHECK_IS_ON()
  DCHECK(!ShouldCheckForPaintInvalidation() || PaintInvalidationStateIsDirty());
#endif
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
      !RuntimeEnabledFeatures::PartialRasterInvalidationEnabled())
    fragment_.SetPartialInvalidationRect(LayoutRect());

  ClearShouldDoFullPaintInvalidation();
  bitfields_.SetMayNeedPaintInvalidation(false);
  bitfields_.SetMayNeedPaintInvalidationSubtree(false);
  bitfields_.SetMayNeedPaintInvalidationAnimatedBackgroundImage(false);
  bitfields_.SetNeedsPaintOffsetAndVisualRectUpdate(false);
  bitfields_.SetShouldInvalidateSelection(false);
  bitfields_.SetBackgroundChangedSinceLastPaintInvalidation(false);
}

bool LayoutObject::IsAllowedToModifyLayoutTreeStructure(Document& document) {
  return DeprecatedDisableModifyLayoutTreeStructureAsserts::
             CanModifyLayoutTreeStateInAnyState() ||
         document.Lifecycle().StateAllowsLayoutTreeMutations();
}

DeprecatedDisableModifyLayoutTreeStructureAsserts::
    DeprecatedDisableModifyLayoutTreeStructureAsserts()
    : disabler_(&g_modify_layout_tree_structure_any_state, true) {}

bool DeprecatedDisableModifyLayoutTreeStructureAsserts::
    CanModifyLayoutTreeStateInAnyState() {
  return g_modify_layout_tree_structure_any_state;
}

void LayoutObject::
    SetShouldDoFullPaintInvalidationIncludingNonCompositingDescendants() {
  // Clear first because PaintInvalidationSubtree overrides other full paint
  // invalidation reasons.
  ClearShouldDoFullPaintInvalidation();
  SetShouldDoFullPaintInvalidation(PaintInvalidationReason::kSubtree);
}

void LayoutObject::SetIsBackgroundAttachmentFixedObject(
    bool is_background_attachment_fixed_object) {
  DCHECK(GetFrameView());
  if (bitfields_.IsBackgroundAttachmentFixedObject() ==
      is_background_attachment_fixed_object)
    return;
  bitfields_.SetIsBackgroundAttachmentFixedObject(
      is_background_attachment_fixed_object);
  if (is_background_attachment_fixed_object)
    GetFrameView()->AddBackgroundAttachmentFixedObject(this);
  else
    GetFrameView()->RemoveBackgroundAttachmentFixedObject(this);
}

LayoutRect LayoutObject::DebugRect() const {
  LayoutRect rect;
  LayoutBlock* block = ContainingBlock();
  if (block)
    block->AdjustChildDebugRect(rect);

  return rect;
}

void LayoutObject::InvalidateSelectionOfSelectedChildren() {
  // setSelectionState() propagates the state up the containing block chain to
  // tell if a block contains selected nodes or not. If this layout object is
  // not a block, we need to get the selection state from the containing block
  // to tell if we have any selected node children.
  LayoutBlock* block =
      IsLayoutBlock() ? ToLayoutBlock(this) : ContainingBlock();
  if (!block)
    return;
  if (!block->HasSelectedChildren())
    return;

  // ::selection style only applies to direct selection leaf children of the
  // element on which the ::selection style is set. Thus, we only walk the
  // direct children here.
  for (LayoutObject* child = SlowFirstChild(); child;
       child = child->NextSibling()) {
    if (!child->CanBeSelectionLeaf())
      continue;
    if (child->GetSelectionState() == SelectionState::kNone)
      continue;
    child->SetShouldInvalidateSelection();
  }
}

void LayoutObject::InvalidateIfControlStateChanged(ControlState control_state) {
  if (LayoutTheme::GetTheme().ControlStateChanged(GetNode(), StyleRef(),
                                                  control_state)) {
    SetShouldDoFullPaintInvalidationIncludingNonCompositingDescendants();
  }
}

// Note about ::first-letter pseudo-element:
//   When an element has ::first-letter pseudo-element, first letter characters
//   are taken from |Text| node and first letter characters are considered
//   as content of <pseudo:first-letter>.
//   For following HTML,
//      <style>div::first-letter {color: red}</style>
//      <div>abc</div>
//   we have following layout tree:
//      LayoutBlockFlow {DIV} at (0,0) size 784x55
//        LayoutInline {<pseudo:first-letter>} at (0,0) size 22x53
//          LayoutTextFragment (anonymous) at (0,1) size 22x53
//            text run at (0,1) width 22: "a"
//        LayoutTextFragment {#text} at (21,30) size 16x17
//          text run at (21,30) width 16: "bc"
//  In this case, |Text::layoutObject()| for "abc" returns |LayoutTextFragment|
//  containing "bc", and it is called remaining part.
//
//  Even if |Text| node contains only first-letter characters, e.g. just "a",
//  remaining part of |LayoutTextFragment|, with |fragmentLength()| == 0, is
//  appeared in layout tree.
//
//  When |Text| node contains only first-letter characters and whitespaces, e.g.
//  "B\n", associated |LayoutTextFragment| is first-letter part instead of
//  remaining part.
//
//  Punctuation characters are considered as first-letter. For "(1)ab",
//  "(1)" are first-letter part and "ab" are remaining part.
const LayoutObject* AssociatedLayoutObjectOf(const Node& node,
                                             int offset_in_node,
                                             LayoutObjectSide object_side) {
  DCHECK_GE(offset_in_node, 0);
  LayoutObject* layout_object = node.GetLayoutObject();
  if (!node.IsTextNode() || !layout_object ||
      !ToLayoutText(layout_object)->IsTextFragment())
    return layout_object;
  LayoutTextFragment* layout_text_fragment =
      ToLayoutTextFragment(layout_object);
  if (!layout_text_fragment->IsRemainingTextLayoutObject()) {
    DCHECK_LE(
        static_cast<unsigned>(offset_in_node),
        layout_text_fragment->Start() + layout_text_fragment->FragmentLength());
    return layout_text_fragment;
  }
  if (layout_text_fragment->FragmentLength()) {
    const unsigned threshold =
        object_side == LayoutObjectSide::kRemainingTextIfOnBoundary
            ? layout_text_fragment->Start()
            : layout_text_fragment->Start() + 1;
    if (static_cast<unsigned>(offset_in_node) >= threshold)
      return layout_object;
  }
  LayoutObject* first_letter_layout_object =
      layout_text_fragment->GetFirstLetterPseudoElement()->GetLayoutObject();
  // TODO(yosin): We're not sure when |firstLetterLayoutObject| has
  // multiple child layout object.
  LayoutObject* child = first_letter_layout_object->SlowFirstChild();
  CHECK(child && child->IsText());
  DCHECK_EQ(child, first_letter_layout_object->SlowLastChild());
  return child;
}

bool LayoutObject::CanBeSelectionLeaf() const {
  if (SlowFirstChild() || Style()->Visibility() != EVisibility::kVisible)
    return false;
  return CanBeSelectionLeafInternal();
}

void LayoutObject::InvalidateClipPathCache() {
  SetNeedsPaintPropertyUpdate();
  for (auto* fragment = &fragment_; fragment;
       fragment = fragment->NextFragment())
    fragment->InvalidateClipPathCache();
}

}  // namespace blink

#ifndef NDEBUG

void showTree(const blink::LayoutObject* object) {
  if (object)
    object->ShowTreeForThis();
  else
    DLOG(INFO) << "Cannot showTree. Root is (nil)";
}

void showLineTree(const blink::LayoutObject* object) {
  if (object)
    object->ShowLineTreeForThis();
  else
    DLOG(INFO) << "Cannot showLineTree. Root is (nil)";
}

void showLayoutTree(const blink::LayoutObject* object1) {
  showLayoutTree(object1, nullptr);
}

void showLayoutTree(const blink::LayoutObject* object1,
                    const blink::LayoutObject* object2) {
  if (object1) {
    const blink::LayoutObject* root = object1;
    while (root->Parent())
      root = root->Parent();
    if (object1) {
      StringBuilder string_builder;
      root->DumpLayoutTreeAndMark(string_builder, object1, "*", object2, "-",
                                  0);
      DLOG(INFO) << "\n" << string_builder.ToString().Utf8().data();
    }
  } else {
    DLOG(INFO) << "Cannot showLayoutTree. Root is (nil)";
  }
}

#endif
