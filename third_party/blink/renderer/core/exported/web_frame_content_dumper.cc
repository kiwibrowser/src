// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/web_frame_content_dumper.h"

#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/iterators/text_iterator.h"
#include "third_party/blink/renderer/core/editing/serializers/serialization.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html_element_type_helpers.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/core/layout/layout_table_row.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/layout_tree_as_text.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

const int text_dumper_max_depth = 512;

bool IsRenderedAndVisible(const Node& node) {
  if (node.GetLayoutObject() &&
      node.GetLayoutObject()->Style()->Visibility() == EVisibility::kVisible)
    return true;
  if (node.IsElementNode() && ToElement(node).HasDisplayContentsStyle())
    return true;
  return false;
}

size_t RequiredLineBreaksAround(const Node& node) {
  if (!IsRenderedAndVisible(node))
    return 0;
  if (node.IsTextNode())
    return 0;
  if (IsHTMLParagraphElement(node))
    return 2;
  if (LayoutObject* layout_object = node.GetLayoutObject()) {
    if (!layout_object->Style()->IsDisplayInlineType())
      return 1;
    if (layout_object->Style()->Display() == EDisplay::kTableCaption)
      return 1;
  }
  return 0;
}

// This class dumps innerText of a node into a StringBuilder, following the spec
// [*] but with a simplified whitespace handling algorithm when processing text
// nodes: only leading and trailing collapsed whitespaces are removed; all other
// whitespace characters are left as-is, without any collapsing or conversion.
// For example, from HTML <p>\na\n\nb\n</p>, we get text dump "a\n\nb".
// [*] https://developer.mozilla.org/en-US/docs/Web/API/Node/innerText
class TextDumper final {
  STACK_ALLOCATED();

 public:
  TextDumper(StringBuilder& builder, size_t max_length)
      : builder_(builder), max_length_(max_length) {}

  void DumpTextFrom(const Node& node) {
    DCHECK(!has_emitted_);
    DCHECK(!required_line_breaks_);
    HandleNode(node, 0);
  }

 private:
  void HandleNode(const Node& node, int depth) {
    const size_t required_line_breaks_around = RequiredLineBreaksAround(node);
    AddRequiredLineBreaks(required_line_breaks_around);

    if (depth < text_dumper_max_depth) {
      for (const Node& child : NodeTraversal::ChildrenOf(node)) {
        HandleNode(child, depth + 1);
        if (builder_.length() >= max_length_)
          return;
      }
    }

    if (!IsRenderedAndVisible(node))
      return;

    if (node.IsTextNode())
      return HandleTextNode(ToText(node));

    if (IsHTMLBRElement(node))
      return DumpText("\n");

    if (LayoutObject* layout_object = node.GetLayoutObject()) {
      if (layout_object->IsTableCell() &&
          ToLayoutTableCell(layout_object)->NextCell())
        return DumpText("\t");
      if (layout_object->IsTableRow() &&
          ToLayoutTableRow(layout_object)->NextRow())
        return DumpText("\n");
    }

    AddRequiredLineBreaks(required_line_breaks_around);
  }

  void HandleTextNode(const Text& node) {
    const LayoutText* layout_text = node.GetLayoutObject();
    if (!layout_text)
      return;
    if (layout_text->IsTextFragment() &&
        ToLayoutTextFragment(layout_text)->IsRemainingTextLayoutObject()) {
      const LayoutText* first_letter =
          ToLayoutText(AssociatedLayoutObjectOf(node, 0));
      if (first_letter && first_letter != layout_text)
        HandleLayoutText(*first_letter);
    }
    HandleLayoutText(*layout_text);
  }

  void HandleLayoutText(const LayoutText& text) {
    if (!text.HasNonCollapsedText())
      return;
    size_t text_start = text.CaretMinOffset();
    size_t text_end = text.CaretMaxOffset();
    String dump = text.GetText().Substring(text_start, text_end - text_start);
    DumpText(dump);
  }

  void AddRequiredLineBreaks(size_t required) {
    required_line_breaks_ = std::max(required, required_line_breaks_);
  }

  void DumpText(String text) {
    if (!text.length())
      return;

    if (has_emitted_ && required_line_breaks_) {
      for (size_t i = 0; i < required_line_breaks_; ++i)
        builder_.Append('\n');
    }
    required_line_breaks_ = 0;
    builder_.Append(text);
    has_emitted_ = true;

    if (builder_.length() > max_length_)
      builder_.Resize(max_length_);
  }

  bool has_emitted_ = false;
  size_t required_line_breaks_ = 0;

  StringBuilder& builder_;
  const size_t max_length_;

  DISALLOW_COPY_AND_ASSIGN(TextDumper);
};

void FrameContentAsPlainText(size_t max_chars,
                             LocalFrame* frame,
                             StringBuilder& output) {
  Document* document = frame->GetDocument();
  if (!document)
    return;

  if (!frame->View() || frame->View()->ShouldThrottleRendering())
    return;

  DCHECK(!frame->View()->NeedsLayout());
  DCHECK(!document->NeedsLayoutTreeUpdate());

  if (document->documentElement())
    TextDumper(output, max_chars).DumpTextFrom(*document->documentElement());

  // The separator between frames when the frames are converted to plain text.
  const LChar kFrameSeparator[] = {'\n', '\n'};
  const size_t frame_separator_length = arraysize(kFrameSeparator);

  // Recursively walk the children.
  const FrameTree& frame_tree = frame->Tree();
  for (Frame* cur_child = frame_tree.FirstChild(); cur_child;
       cur_child = cur_child->Tree().NextSibling()) {
    if (!cur_child->IsLocalFrame())
      continue;
    LocalFrame* cur_local_child = ToLocalFrame(cur_child);
    // Ignore the text of non-visible frames.
    LayoutView* layout_view = cur_local_child->ContentLayoutObject();
    LayoutObject* owner_layout_object = cur_local_child->OwnerLayoutObject();
    if (!layout_view || !layout_view->Size().Width() ||
        !layout_view->Size().Height() ||
        (layout_view->Location().X() + layout_view->Size().Width() <= 0) ||
        (layout_view->Location().Y() + layout_view->Size().Height() <= 0) ||
        (owner_layout_object && owner_layout_object->Style() &&
         owner_layout_object->Style()->Visibility() != EVisibility::kVisible)) {
      continue;
    }

    // Make sure the frame separator won't fill up the buffer, and give up if
    // it will. The danger is if the separator will make the buffer longer than
    // maxChars. This will cause the computation above:
    //   maxChars - output->size()
    // to be a negative number which will crash when the subframe is added.
    if (output.length() >= max_chars - frame_separator_length)
      return;

    output.Append(kFrameSeparator, frame_separator_length);
    FrameContentAsPlainText(max_chars, cur_local_child, output);
    if (output.length() >= max_chars)
      return;  // Filled up the buffer.
  }
}

}  // namespace

WebString WebFrameContentDumper::DeprecatedDumpFrameTreeAsText(
    WebLocalFrame* frame,
    size_t max_chars) {
  if (!frame)
    return WebString();
  StringBuilder text;
  FrameContentAsPlainText(max_chars, ToWebLocalFrameImpl(frame)->GetFrame(),
                          text);
  return text.ToString();
}

WebString WebFrameContentDumper::DumpWebViewAsText(WebView* web_view,
                                                   size_t max_chars) {
  DCHECK(web_view);
  WebLocalFrame* frame = web_view->MainFrame()->ToWebLocalFrame();
  if (!frame)
    return WebString();

  web_view->UpdateAllLifecyclePhases();

  StringBuilder text;
  FrameContentAsPlainText(max_chars, ToWebLocalFrameImpl(frame)->GetFrame(),
                          text);
  return text.ToString();
}

WebString WebFrameContentDumper::DumpAsMarkup(WebLocalFrame* frame) {
  if (!frame)
    return WebString();
  return CreateMarkup(ToWebLocalFrameImpl(frame)->GetFrame()->GetDocument());
}

WebString WebFrameContentDumper::DumpLayoutTreeAsText(
    WebLocalFrame* frame,
    LayoutAsTextControls to_show) {
  if (!frame)
    return WebString();
  LayoutAsTextBehavior behavior = kLayoutAsTextShowAllLayers;

  if (to_show & kLayoutAsTextWithLineTrees)
    behavior |= kLayoutAsTextShowLineTrees;

  if (to_show & kLayoutAsTextDebug) {
    behavior |= kLayoutAsTextShowCompositedLayers | kLayoutAsTextShowAddresses |
                kLayoutAsTextShowIDAndClass | kLayoutAsTextShowLayerNesting;
  }

  if (to_show & kLayoutAsTextPrinting)
    behavior |= kLayoutAsTextPrintingMode;

  return ExternalRepresentation(ToWebLocalFrameImpl(frame)->GetFrame(),
                                behavior);
}
}
