/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/frame/frame_serializer.h"

#include "third_party/blink/renderer/core/css/css_font_face_rule.h"
#include "third_party/blink/renderer/core/css/css_font_face_src_value.h"
#include "third_party/blink/renderer/core/css/css_image_value.h"
#include "third_party/blink/renderer/core/css/css_import_rule.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/css_style_declaration.h"
#include "third_party/blink/renderer/core/css/css_style_rule.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/style_rule.h"
#include "third_party/blink/renderer/core/css/style_sheet_contents.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/editing/serializers/markup_accumulator.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/html_frame_element_base.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/html_link_element.h"
#include "third_party/blink/renderer/core/html/html_meta_element.h"
#include "third_party/blink/renderer/core/html/html_style_element.h"
#include "third_party/blink/renderer/core/html/image_document.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/loader/resource/font_resource.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource_content.h"
#include "third_party/blink/renderer/core/style/style_fetched_image.h"
#include "third_party/blink/renderer/core/style/style_image.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/serialized_resource.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace {

const int32_t secondsToMicroseconds = 1000 * 1000;
const int32_t maxSerializationTimeUmaMicroseconds = 10 * secondsToMicroseconds;

}  // namespace

namespace blink {

class SerializerMarkupAccumulator : public MarkupAccumulator {
  STACK_ALLOCATED();

 public:
  SerializerMarkupAccumulator(FrameSerializer::Delegate&,
                              const Document&,
                              HeapVector<Member<Node>>&);
  ~SerializerMarkupAccumulator() override;

 protected:
  void AppendCustomAttributes(StringBuilder&,
                              const Element&,
                              Namespaces*) override;
  void AppendText(StringBuilder& out, Text&) override;
  bool ShouldIgnoreAttribute(const Element&, const Attribute&) const override;
  bool ShouldIgnoreElement(const Element&) const override;
  void AppendElement(StringBuilder& out, const Element&, Namespaces*) override;
  void AppendAttribute(StringBuilder& out,
                       const Element&,
                       const Attribute&,
                       Namespaces*) override;
  void AppendStartTag(Node&, Namespaces* = nullptr) override;
  void AppendEndTag(const Element&) override;
  std::pair<Node*, Element*> GetAuxiliaryDOMTree(const Element&) const override;

 private:
  void AppendAttributeValue(StringBuilder& out, const String& attribute_value);
  void AppendRewrittenAttribute(StringBuilder& out,
                                const Element&,
                                const String& attribute_name,
                                const String& attribute_value);

  FrameSerializer::Delegate& delegate_;
  Member<const Document> document_;

  // FIXME: |FrameSerializer| uses |m_nodes| for collecting nodes in document
  // included into serialized text then extracts image, object, etc. The size
  // of this vector isn't small for large document. It is better to use
  // callback like functionality.
  HeapVector<Member<Node>>& nodes_;

  // Elements with links rewritten via appendAttribute method.
  HeapHashSet<Member<const Element>> elements_with_rewritten_links_;
};

SerializerMarkupAccumulator::SerializerMarkupAccumulator(
    FrameSerializer::Delegate& delegate,
    const Document& document,
    HeapVector<Member<Node>>& nodes)
    : MarkupAccumulator(kResolveAllURLs),
      delegate_(delegate),
      document_(&document),
      nodes_(nodes) {}

SerializerMarkupAccumulator::~SerializerMarkupAccumulator() = default;

void SerializerMarkupAccumulator::AppendCustomAttributes(
    StringBuilder& result,
    const Element& element,
    Namespaces* namespaces) {
  Vector<Attribute> attributes = delegate_.GetCustomAttributes(element);
  for (const auto& attribute : attributes)
    AppendAttribute(result, element, attribute, namespaces);
}

void SerializerMarkupAccumulator::AppendText(StringBuilder& result,
                                             Text& text) {
  MarkupAccumulator::AppendText(result, text);
}

bool SerializerMarkupAccumulator::ShouldIgnoreAttribute(
    const Element& element,
    const Attribute& attribute) const {
  return delegate_.ShouldIgnoreAttribute(element, attribute);
}

bool SerializerMarkupAccumulator::ShouldIgnoreElement(
    const Element& element) const {
  if (IsHTMLScriptElement(element))
    return true;
  if (IsHTMLNoScriptElement(element))
    return true;
  if (IsHTMLMetaElement(element) &&
      ToHTMLMetaElement(element).ComputeEncoding().IsValid()) {
    return true;
  }
  return delegate_.ShouldIgnoreElement(element);
}

void SerializerMarkupAccumulator::AppendElement(StringBuilder& result,
                                                const Element& element,
                                                Namespaces* namespaces) {
  MarkupAccumulator::AppendElement(result, element, namespaces);

  // TODO(tiger): Refactor MarkupAccumulator so it is easier to append an
  // element like this, without special cases for XHTML
  if (IsHTMLHeadElement(element)) {
    result.Append("<meta http-equiv=\"Content-Type\" content=\"");
    AppendAttributeValue(result, document_->SuggestedMIMEType());
    result.Append("; charset=");
    AppendAttributeValue(result, document_->characterSet());
    if (document_->IsXHTMLDocument())
      result.Append("\" />");
    else
      result.Append("\">");
  }

  // FIXME: For object (plugins) tags and video tag we could replace them by an
  // image of their current contents.
}

void SerializerMarkupAccumulator::AppendAttribute(StringBuilder& out,
                                                  const Element& element,
                                                  const Attribute& attribute,
                                                  Namespaces* namespaces) {
  // Check if link rewriting can affect the attribute.
  bool is_link_attribute = element.HasLegalLinkAttribute(attribute.GetName());
  bool is_src_doc_attribute = IsHTMLFrameElementBase(element) &&
                              attribute.GetName() == HTMLNames::srcdocAttr;
  if (is_link_attribute || is_src_doc_attribute) {
    // Check if the delegate wants to do link rewriting for the element.
    String new_link_for_the_element;
    if (delegate_.RewriteLink(element, new_link_for_the_element)) {
      if (is_link_attribute) {
        // Rewrite element links.
        AppendRewrittenAttribute(out, element, attribute.GetName().ToString(),
                                 new_link_for_the_element);
      } else {
        DCHECK(is_src_doc_attribute);
        // Emit src instead of srcdoc attribute for frame elements - we want the
        // serialized subframe to use html contents from the link provided by
        // Delegate::rewriteLink rather than html contents from srcdoc
        // attribute.
        AppendRewrittenAttribute(out, element, HTMLNames::srcAttr.LocalName(),
                                 new_link_for_the_element);
      }
      return;
    }
  }

  // Fallback to appending the original attribute.
  MarkupAccumulator::AppendAttribute(out, element, attribute, namespaces);
}

void SerializerMarkupAccumulator::AppendStartTag(Node& node,
                                                 Namespaces* namespaces) {
  MarkupAccumulator::AppendStartTag(node, namespaces);
  nodes_.push_back(&node);
}

void SerializerMarkupAccumulator::AppendEndTag(const Element& element) {
  MarkupAccumulator::AppendEndTag(element);
}

std::pair<Node*, Element*> SerializerMarkupAccumulator::GetAuxiliaryDOMTree(
    const Element& element) const {
  return delegate_.GetAuxiliaryDOMTree(element);
}

void SerializerMarkupAccumulator::AppendAttributeValue(
    StringBuilder& out,
    const String& attribute_value) {
  MarkupFormatter::AppendAttributeValue(out, attribute_value,
                                        document_->IsHTMLDocument());
}

void SerializerMarkupAccumulator::AppendRewrittenAttribute(
    StringBuilder& out,
    const Element& element,
    const String& attribute_name,
    const String& attribute_value) {
  if (elements_with_rewritten_links_.Contains(&element))
    return;
  elements_with_rewritten_links_.insert(&element);

  // Append the rewritten attribute.
  // TODO(tiger): Refactor MarkupAccumulator so it is easier to append an
  // attribute like this.
  out.Append(' ');
  out.Append(attribute_name);
  out.Append("=\"");
  AppendAttributeValue(out, attribute_value);
  out.Append("\"");
}

// TODO(tiger): Right now there is no support for rewriting URLs inside CSS
// documents which leads to bugs like <https://crbug.com/251898>. Not being
// able to rewrite URLs inside CSS documents means that resources imported from
// url(...) statements in CSS might not work when rewriting links for the
// "Webpage, Complete" method of saving a page. It will take some work but it
// needs to be done if we want to continue to support non-MHTML saved pages.

FrameSerializer::FrameSerializer(Deque<SerializedResource>& resources,
                                 Delegate& delegate)
    : resources_(&resources),
      is_serializing_css_(false),
      delegate_(delegate),
      total_image_count_(0),
      loaded_image_count_(0),
      total_css_count_(0),
      loaded_css_count_(0),
      should_collect_problem_metric_(false) {}

void FrameSerializer::SerializeFrame(const LocalFrame& frame) {
  TRACE_EVENT0("page-serialization", "FrameSerializer::serializeFrame");
  DCHECK(frame.GetDocument());
  Document& document = *frame.GetDocument();
  KURL url = document.Url();

  // If frame is an image document, add the image and don't continue
  if (document.IsImageDocument()) {
    ImageDocument& image_document = ToImageDocument(document);
    AddImageToResources(image_document.CachedImage(), url);
    return;
  }

  HeapVector<Member<Node>> serialized_nodes;
  {
    TRACE_EVENT0("page-serialization", "FrameSerializer::serializeFrame HTML");
    SCOPED_BLINK_UMA_HISTOGRAM_TIMER(
        "PageSerialization.SerializationTime.Html");
    SerializerMarkupAccumulator accumulator(delegate_, document,
                                            serialized_nodes);
    String text =
        SerializeNodes<EditingStrategy>(accumulator, document, kIncludeNode);

    CString frame_html =
        document.Encoding().Encode(text, WTF::kEntitiesForUnencodables);
    resources_->push_back(SerializedResource(
        url, document.SuggestedMIMEType(),
        SharedBuffer::Create(frame_html.data(), frame_html.length())));
  }

  should_collect_problem_metric_ =
      delegate_.ShouldCollectProblemMetric() && frame.IsMainFrame();
  for (Node* node : serialized_nodes) {
    DCHECK(node);
    if (!node->IsElementNode())
      continue;

    Element& element = ToElement(*node);
    // We have to process in-line style as it might contain some resources
    // (typically background images).
    if (element.IsStyledElement()) {
      RetrieveResourcesForProperties(element.InlineStyle(), document);
      RetrieveResourcesForProperties(element.PresentationAttributeStyle(),
                                     document);
    }

    if (auto* image = ToHTMLImageElementOrNull(element)) {
      KURL url = document.CompleteURL(image->getAttribute(HTMLNames::srcAttr));
      ImageResourceContent* cached_image = image->CachedImage();
      AddImageToResources(cached_image, url);
    } else if (auto* input = ToHTMLInputElementOrNull(element)) {
      if (input->type() == InputTypeNames::image && input->ImageLoader()) {
        KURL url = input->Src();
        ImageResourceContent* cached_image = input->ImageLoader()->GetContent();
        AddImageToResources(cached_image, url);
      }
    } else if (auto* link = ToHTMLLinkElementOrNull(element)) {
      if (CSSStyleSheet* sheet = link->sheet()) {
        KURL url =
            document.CompleteURL(link->getAttribute(HTMLNames::hrefAttr));
        SerializeCSSStyleSheet(*sheet, url);
      }
    } else if (auto* style = ToHTMLStyleElementOrNull(element)) {
      if (CSSStyleSheet* sheet = style->sheet())
        SerializeCSSStyleSheet(*sheet, NullURL());
    }
  }
  if (should_collect_problem_metric_) {
    // Report detectors through UMA.
    // We're having exact 21 buckets for percentage because we want to have 5%
    // in each bucket to avoid potential spikes in the distribution.
    UMA_HISTOGRAM_COUNTS_100(
        "PageSerialization.ProblemDetection.TotalImageCount",
        static_cast<int64_t>(total_image_count_));
    if (total_image_count_ > 0) {
      DCHECK_LE(loaded_image_count_, total_image_count_);
      DEFINE_STATIC_LOCAL(
          LinearHistogram, image_histogram,
          ("PageSerialization.ProblemDetection.LoadedImagePercentage", 1, 100,
           21));
      image_histogram.Count(
          static_cast<int64_t>(loaded_image_count_ * 100 / total_image_count_));
    }

    UMA_HISTOGRAM_COUNTS_100("PageSerialization.ProblemDetection.TotalCSSCount",
                             static_cast<int64_t>(total_css_count_));
    if (total_css_count_ > 0) {
      DCHECK_LE(loaded_css_count_, total_css_count_);
      DEFINE_STATIC_LOCAL(
          LinearHistogram, css_histogram,
          ("PageSerialization.ProblemDetection.LoadedCSSPercentage", 1, 100,
           21));
      css_histogram.Count(
          static_cast<int64_t>(loaded_css_count_ * 100 / total_css_count_));
    }
    should_collect_problem_metric_ = false;
  }
}

void FrameSerializer::SerializeCSSStyleSheet(CSSStyleSheet& style_sheet,
                                             const KURL& url) {
  // If the URL is invalid or if it is a data URL this means that this CSS is
  // defined inline, respectively in a <style> tag or in the data URL itself.
  bool is_inline_css = !url.IsValid() || url.ProtocolIsData();
  // If this CSS is not inline then it is identifiable by its URL. So just skip
  // it if it has already been analyzed before.
  if (!is_inline_css && (resource_urls_.Contains(url) ||
                         delegate_.ShouldSkipResourceWithURL(url))) {
    return;
  }
  if (!is_inline_css)
    resource_urls_.insert(url);
  if (should_collect_problem_metric_ && !is_inline_css) {
    total_css_count_++;
    if (style_sheet.LoadCompleted())
      loaded_css_count_++;
  }

  TRACE_EVENT2("page-serialization", "FrameSerializer::serializeCSSStyleSheet",
               "type", "CSS", "url", url.ElidedString().Utf8().data());
  // Only report UMA metric if this is not a reentrant CSS serialization call.
  double css_start_time = 0;
  if (!is_serializing_css_) {
    is_serializing_css_ = true;
    css_start_time = CurrentTimeTicksInSeconds();
  }

  // If this CSS is inlined its definition was already serialized with the frame
  // HTML code that was previously generated. No need to regenerate it here.
  if (!is_inline_css) {
    StringBuilder css_text;
    css_text.Append("@charset \"");
    css_text.Append(
        String(style_sheet.Contents()->Charset().GetName()).DeprecatedLower());
    css_text.Append("\";\n\n");

    for (unsigned i = 0; i < style_sheet.length(); ++i) {
      CSSRule* rule = style_sheet.item(i);
      String item_text = rule->cssText();
      if (!item_text.IsEmpty()) {
        css_text.Append(item_text);
        if (i < style_sheet.length() - 1)
          css_text.Append("\n\n");
      }
    }

    WTF::TextEncoding text_encoding(style_sheet.Contents()->Charset());
    DCHECK(text_encoding.IsValid());
    String text_string = css_text.ToString();
    CString text = text_encoding.Encode(
        text_string, WTF::kCSSEncodedEntitiesForUnencodables);
    resources_->push_back(
        SerializedResource(url, String("text/css"),
                           SharedBuffer::Create(text.data(), text.length())));
  }

  // Sub resources need to be serialized even if the CSS definition doesn't
  // need to be.
  for (unsigned i = 0; i < style_sheet.length(); ++i)
    SerializeCSSRule(style_sheet.item(i));

  if (css_start_time != 0) {
    is_serializing_css_ = false;
    DEFINE_STATIC_LOCAL(CustomCountHistogram, css_histogram,
                        ("PageSerialization.SerializationTime.CSSElement", 0,
                         maxSerializationTimeUmaMicroseconds, 50));
    css_histogram.Count(
        static_cast<int64_t>((CurrentTimeTicksInSeconds() - css_start_time) *
                             secondsToMicroseconds));
  }
}

void FrameSerializer::SerializeCSSRule(CSSRule* rule) {
  DCHECK(rule->parentStyleSheet()->OwnerDocument());
  Document& document = *rule->parentStyleSheet()->OwnerDocument();

  switch (rule->type()) {
    case CSSRule::kStyleRule:
      RetrieveResourcesForProperties(
          &ToCSSStyleRule(rule)->GetStyleRule()->Properties(), document);
      break;

    case CSSRule::kImportRule: {
      CSSImportRule* import_rule = ToCSSImportRule(rule);
      KURL sheet_base_url = rule->parentStyleSheet()->BaseURL();
      DCHECK(sheet_base_url.IsValid());
      KURL import_url = KURL(sheet_base_url, import_rule->href());
      if (import_rule->styleSheet())
        SerializeCSSStyleSheet(*import_rule->styleSheet(), import_url);
      break;
    }

    // Rules inheriting CSSGroupingRule
    case CSSRule::kMediaRule:
    case CSSRule::kSupportsRule: {
      CSSRuleList* rule_list = rule->cssRules();
      for (unsigned i = 0; i < rule_list->length(); ++i)
        SerializeCSSRule(rule_list->item(i));
      break;
    }

    case CSSRule::kFontFaceRule:
      RetrieveResourcesForProperties(
          &ToCSSFontFaceRule(rule)->StyleRule()->Properties(), document);
      break;

    // Rules in which no external resources can be referenced
    case CSSRule::kCharsetRule:
    case CSSRule::kPageRule:
    case CSSRule::kKeyframesRule:
    case CSSRule::kKeyframeRule:
    case CSSRule::kNamespaceRule:
    case CSSRule::kViewportRule:
      break;
  }
}

bool FrameSerializer::ShouldAddURL(const KURL& url) {
  return url.IsValid() && !resource_urls_.Contains(url) &&
         !url.ProtocolIsData() && !delegate_.ShouldSkipResourceWithURL(url);
}

void FrameSerializer::AddToResources(
    const String& mime_type,
    ResourceHasCacheControlNoStoreHeader has_cache_control_no_store_header,
    scoped_refptr<const SharedBuffer> data,
    const KURL& url) {
  if (delegate_.ShouldSkipResource(has_cache_control_no_store_header))
    return;

  if (!data) {
    DLOG(ERROR) << "No data for resource " << url.GetString();
    return;
  }

  resources_->push_back(SerializedResource(url, mime_type, std::move(data)));
}

void FrameSerializer::AddImageToResources(ImageResourceContent* image,
                                          const KURL& url) {
  if (!ShouldAddURL(url))
    return;
  resource_urls_.insert(url);
  if (should_collect_problem_metric_)
    total_image_count_++;
  if (!image || !image->HasImage() || image->ErrorOccurred())
    return;
  if (should_collect_problem_metric_ && image->IsLoaded())
    loaded_image_count_++;

  TRACE_EVENT2("page-serialization", "FrameSerializer::addImageToResources",
               "type", "image", "url", url.ElidedString().Utf8().data());
  double image_start_time = CurrentTimeTicksInSeconds();

  scoped_refptr<const SharedBuffer> data = image->GetImage()->Data();
  AddToResources(image->GetResponse().MimeType(),
                 image->HasCacheControlNoStoreHeader()
                     ? kHasCacheControlNoStoreHeader
                     : kNoCacheControlNoStoreHeader,
                 data, url);

  // If we're already reporting time for CSS serialization don't report it for
  // this image to avoid reporting the same time twice.
  if (!is_serializing_css_) {
    DEFINE_STATIC_LOCAL(CustomCountHistogram, image_histogram,
                        ("PageSerialization.SerializationTime.ImageElement", 0,
                         maxSerializationTimeUmaMicroseconds, 50));
    image_histogram.Count(
        static_cast<int64_t>((CurrentTimeTicksInSeconds() - image_start_time) *
                             secondsToMicroseconds));
  }
}

void FrameSerializer::AddFontToResources(FontResource& font) {
  if (!ShouldAddURL(font.Url()))
    return;
  resource_urls_.insert(font.Url());
  if (!font.IsLoaded() || !font.ResourceBuffer())
    return;

  scoped_refptr<const SharedBuffer> data(font.ResourceBuffer());

  AddToResources(font.GetResponse().MimeType(),
                 font.HasCacheControlNoStoreHeader()
                     ? kHasCacheControlNoStoreHeader
                     : kNoCacheControlNoStoreHeader,
                 data, font.Url());
}

void FrameSerializer::RetrieveResourcesForProperties(
    const CSSPropertyValueSet* style_declaration,
    Document& document) {
  if (!style_declaration)
    return;

  // The background-image and list-style-image (for ul or ol) are the CSS
  // properties that make use of images. We iterate to make sure we include any
  // other image properties there might be.
  unsigned property_count = style_declaration->PropertyCount();
  for (unsigned i = 0; i < property_count; ++i) {
    const CSSValue& css_value = style_declaration->PropertyAt(i).Value();
    RetrieveResourcesForCSSValue(css_value, document);
  }
}

void FrameSerializer::RetrieveResourcesForCSSValue(const CSSValue& css_value,
                                                   Document& document) {
  if (css_value.IsImageValue()) {
    const CSSImageValue& image_value = ToCSSImageValue(css_value);
    if (image_value.IsCachePending())
      return;
    StyleImage* style_image = image_value.CachedImage();
    if (!style_image || !style_image->IsImageResource())
      return;

    AddImageToResources(style_image->CachedImage(),
                        style_image->CachedImage()->Url());
  } else if (css_value.IsFontFaceSrcValue()) {
    const CSSFontFaceSrcValue& font_face_src_value =
        ToCSSFontFaceSrcValue(css_value);
    if (font_face_src_value.IsLocal()) {
      return;
    }

    AddFontToResources(font_face_src_value.Fetch(&document, nullptr));
  } else if (css_value.IsValueList()) {
    const CSSValueList& css_value_list = ToCSSValueList(css_value);
    for (unsigned i = 0; i < css_value_list.length(); i++)
      RetrieveResourcesForCSSValue(css_value_list.Item(i), document);
  }
}

// Returns MOTW (Mark of the Web) declaration before html tag which is in
// HTML comment, e.g. "<!-- saved from url=(%04d)%s -->"
// See http://msdn2.microsoft.com/en-us/library/ms537628(VS.85).aspx.
String FrameSerializer::MarkOfTheWebDeclaration(const KURL& url) {
  StringBuilder builder;
  bool emits_minus = false;
  CString orignal_url = url.GetString().Ascii();
  for (const char* string = orignal_url.data(); *string; ++string) {
    const char ch = *string;
    if (ch == '-' && emits_minus) {
      builder.Append("%2D");
      emits_minus = false;
      continue;
    }
    emits_minus = ch == '-';
    builder.Append(ch);
  }
  CString escaped_url = builder.ToString().Ascii();
  return String::Format("saved from url=(%04d)%s",
                        static_cast<int>(escaped_url.length()),
                        escaped_url.data());
}

}  // namespace blink
