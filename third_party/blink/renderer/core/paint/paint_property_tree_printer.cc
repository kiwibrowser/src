// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/paint_property_tree_printer.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"

#include <iomanip>
#include <sstream>

#if DCHECK_IS_ON()

namespace blink {
namespace {

template <typename PropertyTreeNode>
class PropertyTreePrinterTraits;

template <typename PropertyTreeNode>
class FrameViewPropertyTreePrinter
    : public PropertyTreePrinter<PropertyTreeNode> {
 public:
  String TreeAsString(const LocalFrameView& frame_view) {
    CollectNodes(frame_view);
    return PropertyTreePrinter<PropertyTreeNode>::NodesAsTreeString();
  }

 private:
  using Traits = PropertyTreePrinterTraits<PropertyTreeNode>;

  void CollectNodes(const LocalFrameView& frame_view) {
    if (LayoutView* layout_view = frame_view.GetLayoutView())
      CollectNodes(*layout_view);
    for (Frame* child = frame_view.GetFrame().Tree().FirstChild(); child;
         child = child->Tree().NextSibling()) {
      if (!child->IsLocalFrame())
        continue;
      if (LocalFrameView* child_view = ToLocalFrame(child)->View())
        CollectNodes(*child_view);
    }
  }

  void CollectNodes(const LayoutObject& object) {
    for (const auto* fragment = &object.FirstFragment(); fragment;
         fragment = fragment->NextFragment()) {
      if (const auto* properties = fragment->PaintProperties())
        Traits::AddObjectPaintProperties(object, *properties, *this);
    }
    for (const auto* child = object.SlowFirstChild(); child;
         child = child->NextSibling()) {
      CollectNodes(*child);
    }
  }
};

template <>
class PropertyTreePrinterTraits<TransformPaintPropertyNode> {
 public:
  static void AddObjectPaintProperties(
      const LayoutObject& object,
      const ObjectPaintProperties& properties,
      PropertyTreePrinter<TransformPaintPropertyNode>& printer) {
    printer.AddNode(properties.PaintOffsetTranslation());
    printer.AddNode(properties.Transform());
    printer.AddNode(properties.Perspective());
    printer.AddNode(properties.SvgLocalToBorderBoxTransform());
    printer.AddNode(properties.ScrollTranslation());
  }
};

template <>
class PropertyTreePrinterTraits<ClipPaintPropertyNode> {
 public:
  static void AddObjectPaintProperties(
      const LayoutObject& object,
      const ObjectPaintProperties& properties,
      PropertyTreePrinter<ClipPaintPropertyNode>& printer) {
    printer.AddNode(properties.FragmentClip());
    printer.AddNode(properties.ClipPathClip());
    printer.AddNode(properties.MaskClip());
    printer.AddNode(properties.CssClip());
    printer.AddNode(properties.CssClipFixedPosition());
    printer.AddNode(properties.OverflowControlsClip());
    printer.AddNode(properties.InnerBorderRadiusClip());
    printer.AddNode(properties.OverflowClip());
  }
};

template <>
class PropertyTreePrinterTraits<EffectPaintPropertyNode> {
 public:
  static void AddObjectPaintProperties(
      const LayoutObject& object,
      const ObjectPaintProperties& properties,
      PropertyTreePrinter<EffectPaintPropertyNode>& printer) {
    printer.AddNode(properties.Effect());
    printer.AddNode(properties.Filter());
    printer.AddNode(properties.Mask());
    printer.AddNode(properties.ClipPath());
  }
};

template <>
class PropertyTreePrinterTraits<ScrollPaintPropertyNode> {
 public:
  static void AddObjectPaintProperties(
      const LayoutObject& object,
      const ObjectPaintProperties& properties,
      PropertyTreePrinter<ScrollPaintPropertyNode>& printer) {
    printer.AddNode(properties.Scroll());
  }
};

template <typename PropertyTreeNode>
void SetDebugName(const PropertyTreeNode* node, const String& debug_name) {
  if (node)
    const_cast<PropertyTreeNode*>(node)->SetDebugName(debug_name);
}

template <typename PropertyTreeNode>
void SetDebugName(const PropertyTreeNode* node,
                  const String& name,
                  const LayoutObject& object) {
  if (node)
    SetDebugName(node, name + " (" + object.DebugName() + ")");
}

}  // namespace

namespace PaintPropertyTreePrinter {

void UpdateDebugNames(const LayoutObject& object,
                      ObjectPaintProperties& properties) {
  SetDebugName(properties.PaintOffsetTranslation(), "PaintOffsetTranslation",
               object);
  SetDebugName(properties.Transform(), "Transform", object);
  SetDebugName(properties.Perspective(), "Perspective", object);
  SetDebugName(properties.SvgLocalToBorderBoxTransform(),
               "SvgLocalToBorderBoxTransform", object);
  SetDebugName(properties.ScrollTranslation(), "ScrollTranslation", object);

  SetDebugName(properties.FragmentClip(), "FragmentClip", object);
  SetDebugName(properties.ClipPathClip(), "ClipPathClip", object);
  SetDebugName(properties.MaskClip(), "MaskClip", object);
  SetDebugName(properties.CssClip(), "CssClip", object);
  SetDebugName(properties.CssClipFixedPosition(), "CssClipFixedPosition",
               object);
  SetDebugName(properties.OverflowControlsClip(), "OverflowControlsClip",
               object);
  SetDebugName(properties.InnerBorderRadiusClip(), "InnerBorderRadiusClip",
               object);
  SetDebugName(properties.OverflowClip(), "OverflowClip", object);

  SetDebugName(properties.Effect(), "Effect", object);
  SetDebugName(properties.Filter(), "Filter", object);
  SetDebugName(properties.Mask(), "Mask", object);
  SetDebugName(properties.ClipPath(), "ClipPath", object);
  SetDebugName(properties.Scroll(), "Scroll", object);
}

}  // namespace PaintPropertyTreePrinter

}  // namespace blink

CORE_EXPORT void showAllPropertyTrees(const blink::LocalFrameView& rootFrame) {
  showTransformPropertyTree(rootFrame);
  showClipPropertyTree(rootFrame);
  showEffectPropertyTree(rootFrame);
  showScrollPropertyTree(rootFrame);
}

void showTransformPropertyTree(const blink::LocalFrameView& rootFrame) {
  LOG(ERROR) << "Transform tree:\n"
             << transformPropertyTreeAsString(rootFrame).Utf8().data();
}

void showClipPropertyTree(const blink::LocalFrameView& rootFrame) {
  LOG(ERROR) << "Clip tree:\n"
             << clipPropertyTreeAsString(rootFrame).Utf8().data();
}

void showEffectPropertyTree(const blink::LocalFrameView& rootFrame) {
  LOG(ERROR) << "Effect tree:\n"
             << effectPropertyTreeAsString(rootFrame).Utf8().data();
}

void showScrollPropertyTree(const blink::LocalFrameView& rootFrame) {
  LOG(ERROR) << "Scroll tree:\n"
             << scrollPropertyTreeAsString(rootFrame).Utf8().data();
}

String transformPropertyTreeAsString(const blink::LocalFrameView& rootFrame) {
  return blink::FrameViewPropertyTreePrinter<
             blink::TransformPaintPropertyNode>()
      .TreeAsString(rootFrame);
}

String clipPropertyTreeAsString(const blink::LocalFrameView& rootFrame) {
  return blink::FrameViewPropertyTreePrinter<blink::ClipPaintPropertyNode>()
      .TreeAsString(rootFrame);
}

String effectPropertyTreeAsString(const blink::LocalFrameView& rootFrame) {
  return blink::FrameViewPropertyTreePrinter<blink::EffectPaintPropertyNode>()
      .TreeAsString(rootFrame);
}

String scrollPropertyTreeAsString(const blink::LocalFrameView& rootFrame) {
  return blink::FrameViewPropertyTreePrinter<blink::ScrollPaintPropertyNode>()
      .TreeAsString(rootFrame);
}

#endif  // DCHECK_IS_ON()
