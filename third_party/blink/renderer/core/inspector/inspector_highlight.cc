// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/inspector_highlight.h"

#include "base/macros.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_grid.h"
#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/shapes/shape_outside_info.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/style/computed_style_constants.h"
#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/platform_chrome_client.h"

namespace blink {

namespace {

class PathBuilder {
  STACK_ALLOCATED();

 public:
  PathBuilder() : path_(protocol::ListValue::create()) {}
  virtual ~PathBuilder() = default;

  std::unique_ptr<protocol::ListValue> Release() { return std::move(path_); }

  void AppendPath(const Path& path, float scale) {
    Path transform_path(path);
    transform_path.Transform(AffineTransform().Scale(scale));
    transform_path.Apply(this, &PathBuilder::AppendPathElement);
  }

 protected:
  virtual FloatPoint TranslatePoint(const FloatPoint& point) { return point; }

 private:
  static void AppendPathElement(void* path_builder,
                                const PathElement* path_element) {
    static_cast<PathBuilder*>(path_builder)->AppendPathElement(path_element);
  }

  void AppendPathElement(const PathElement*);
  void AppendPathCommandAndPoints(const char* command,
                                  const FloatPoint points[],
                                  size_t length);

  std::unique_ptr<protocol::ListValue> path_;
  DISALLOW_COPY_AND_ASSIGN(PathBuilder);
};

void PathBuilder::AppendPathCommandAndPoints(const char* command,
                                             const FloatPoint points[],
                                             size_t length) {
  path_->pushValue(protocol::StringValue::create(command));
  for (size_t i = 0; i < length; i++) {
    FloatPoint point = TranslatePoint(points[i]);
    path_->pushValue(protocol::FundamentalValue::create(point.X()));
    path_->pushValue(protocol::FundamentalValue::create(point.Y()));
  }
}

void PathBuilder::AppendPathElement(const PathElement* path_element) {
  switch (path_element->type) {
    // The points member will contain 1 value.
    case kPathElementMoveToPoint:
      AppendPathCommandAndPoints("M", path_element->points, 1);
      break;
    // The points member will contain 1 value.
    case kPathElementAddLineToPoint:
      AppendPathCommandAndPoints("L", path_element->points, 1);
      break;
    // The points member will contain 3 values.
    case kPathElementAddCurveToPoint:
      AppendPathCommandAndPoints("C", path_element->points, 3);
      break;
    // The points member will contain 2 values.
    case kPathElementAddQuadCurveToPoint:
      AppendPathCommandAndPoints("Q", path_element->points, 2);
      break;
    // The points member will contain no values.
    case kPathElementCloseSubpath:
      AppendPathCommandAndPoints("Z", nullptr, 0);
      break;
  }
}

class ShapePathBuilder : public PathBuilder {
 public:
  ShapePathBuilder(LocalFrameView& view,
                   LayoutObject& layout_object,
                   const ShapeOutsideInfo& shape_outside_info)
      : view_(&view),
        layout_object_(layout_object),
        shape_outside_info_(shape_outside_info) {}

  static std::unique_ptr<protocol::ListValue> BuildPath(
      LocalFrameView& view,
      LayoutObject& layout_object,
      const ShapeOutsideInfo& shape_outside_info,
      const Path& path,
      float scale) {
    ShapePathBuilder builder(view, layout_object, shape_outside_info);
    builder.AppendPath(path, scale);
    return builder.Release();
  }

 protected:
  FloatPoint TranslatePoint(const FloatPoint& point) override {
    FloatPoint layout_object_point =
        shape_outside_info_.ShapeToLayoutObjectPoint(point);
    return view_->ContentsToViewport(
        RoundedIntPoint(layout_object_.LocalToAbsolute(layout_object_point)));
  }

 private:
  Member<LocalFrameView> view_;
  LayoutObject& layout_object_;
  const ShapeOutsideInfo& shape_outside_info_;
};

std::unique_ptr<protocol::Array<double>> BuildArrayForQuad(
    const FloatQuad& quad) {
  std::unique_ptr<protocol::Array<double>> array =
      protocol::Array<double>::create();
  array->addItem(quad.P1().X());
  array->addItem(quad.P1().Y());
  array->addItem(quad.P2().X());
  array->addItem(quad.P2().Y());
  array->addItem(quad.P3().X());
  array->addItem(quad.P3().Y());
  array->addItem(quad.P4().X());
  array->addItem(quad.P4().Y());
  return array;
}

Path QuadToPath(const FloatQuad& quad) {
  Path quad_path;
  quad_path.MoveTo(quad.P1());
  quad_path.AddLineTo(quad.P2());
  quad_path.AddLineTo(quad.P3());
  quad_path.AddLineTo(quad.P4());
  quad_path.CloseSubpath();
  return quad_path;
}

FloatPoint ContentsPointToViewport(const LocalFrameView* view,
                                   FloatPoint point_in_contents) {
  LayoutPoint point_in_frame =
      view->ContentsToFrame(LayoutPoint(point_in_contents));
  FloatPoint point_in_root_frame =
      FloatPoint(view->ConvertToRootFrame(point_in_frame));
  return FloatPoint(view->GetPage()->GetVisualViewport().RootFrameToViewport(
      point_in_root_frame));
}

void ContentsQuadToViewport(const LocalFrameView* view, FloatQuad& quad) {
  quad.SetP1(ContentsPointToViewport(view, quad.P1()));
  quad.SetP2(ContentsPointToViewport(view, quad.P2()));
  quad.SetP3(ContentsPointToViewport(view, quad.P3()));
  quad.SetP4(ContentsPointToViewport(view, quad.P4()));
}

const ShapeOutsideInfo* ShapeOutsideInfoForNode(Node* node,
                                                Shape::DisplayPaths* paths,
                                                FloatQuad* bounds) {
  LayoutObject* layout_object = node->GetLayoutObject();
  if (!layout_object || !layout_object->IsBox() ||
      !ToLayoutBox(layout_object)->GetShapeOutsideInfo())
    return nullptr;

  LocalFrameView* containing_view = node->GetDocument().View();
  LayoutBox* layout_box = ToLayoutBox(layout_object);
  const ShapeOutsideInfo* shape_outside_info =
      layout_box->GetShapeOutsideInfo();

  shape_outside_info->ComputedShape().BuildDisplayPaths(*paths);

  LayoutRect shape_bounds =
      shape_outside_info->ComputedShapePhysicalBoundingBox();
  *bounds = layout_box->LocalToAbsoluteQuad(FloatRect(shape_bounds));
  ContentsQuadToViewport(containing_view, *bounds);

  return shape_outside_info;
}

std::unique_ptr<protocol::DictionaryValue> BuildElementInfo(Element* element) {
  std::unique_ptr<protocol::DictionaryValue> element_info =
      protocol::DictionaryValue::create();
  Element* real_element = element;
  PseudoElement* pseudo_element = nullptr;
  if (element->IsPseudoElement()) {
    pseudo_element = ToPseudoElement(element);
    real_element = element->ParentOrShadowHostElement();
  }
  bool is_xhtml = real_element->GetDocument().IsXHTMLDocument();
  element_info->setString(
      "tagName", is_xhtml ? real_element->nodeName()
                          : real_element->nodeName().DeprecatedLower());
  element_info->setString("idValue", real_element->GetIdAttribute());
  StringBuilder class_names;
  if (real_element->HasClass() && real_element->IsStyledElement()) {
    HashSet<AtomicString> used_class_names;
    const SpaceSplitString& class_names_string = real_element->ClassNames();
    size_t class_name_count = class_names_string.size();
    for (size_t i = 0; i < class_name_count; ++i) {
      const AtomicString& class_name = class_names_string[i];
      if (!used_class_names.insert(class_name).is_new_entry)
        continue;
      class_names.Append('.');
      class_names.Append(class_name);
    }
  }
  if (pseudo_element) {
    if (pseudo_element->GetPseudoId() == kPseudoIdBefore)
      class_names.Append("::before");
    else if (pseudo_element->GetPseudoId() == kPseudoIdAfter)
      class_names.Append("::after");
  }
  if (!class_names.IsEmpty())
    element_info->setString("className", class_names.ToString());

  LayoutObject* layout_object = element->GetLayoutObject();
  LocalFrameView* containing_view = element->GetDocument().View();
  if (!layout_object || !containing_view)
    return element_info;

  // layoutObject the getBoundingClientRect() data in the tooltip
  // to be consistent with the rulers (see http://crbug.com/262338).
  DOMRect* bounding_box = element->getBoundingClientRect();
  element_info->setString("nodeWidth", String::Number(bounding_box->width()));
  element_info->setString("nodeHeight", String::Number(bounding_box->height()));

  return element_info;
}

std::unique_ptr<protocol::Value> BuildGapAndPositions(
    double origin,
    LayoutUnit gap,
    const Vector<LayoutUnit>& positions,
    float scale) {
  std::unique_ptr<protocol::DictionaryValue> result =
      protocol::DictionaryValue::create();
  result->setDouble("origin", floor(origin * scale));
  result->setDouble("gap", round(gap * scale));

  std::unique_ptr<protocol::ListValue> spans = protocol::ListValue::create();
  for (const LayoutUnit& position : positions) {
    spans->pushValue(
        protocol::FundamentalValue::create(round(position * scale)));
  }
  result->setValue("positions", std::move(spans));

  return result;
}

std::unique_ptr<protocol::DictionaryValue> BuildGridInfo(
    LayoutGrid* layout_grid,
    FloatPoint origin,
    Color color,
    float scale,
    bool isPrimary) {
  std::unique_ptr<protocol::DictionaryValue> grid_info =
      protocol::DictionaryValue::create();

  grid_info->setValue(
      "rows", BuildGapAndPositions(origin.Y(),
                                   layout_grid->GridGap(kForRows) +
                                       layout_grid->GridItemOffset(kForRows),
                                   layout_grid->RowPositions(), scale));
  grid_info->setValue(
      "columns",
      BuildGapAndPositions(origin.X(),
                           layout_grid->GridGap(kForColumns) +
                               layout_grid->GridItemOffset(kForColumns),
                           layout_grid->ColumnPositions(), scale));
  grid_info->setString("color", color.Serialized());
  grid_info->setBoolean("isPrimaryGrid", isPrimary);
  return grid_info;
}

}  // namespace

InspectorHighlight::InspectorHighlight(float scale)
    : highlight_paths_(protocol::ListValue::create()),
      show_rulers_(false),
      show_extension_lines_(false),
      display_as_material_(false),
      scale_(scale) {}

InspectorHighlightConfig::InspectorHighlightConfig()
    : show_info(false),
      show_rulers(false),
      show_extension_lines(false),
      display_as_material(false) {}

InspectorHighlight::InspectorHighlight(
    Node* node,
    const InspectorHighlightConfig& highlight_config,
    bool append_element_info)
    : highlight_paths_(protocol::ListValue::create()),
      show_rulers_(highlight_config.show_rulers),
      show_extension_lines_(highlight_config.show_extension_lines),
      display_as_material_(highlight_config.display_as_material),
      scale_(1.f) {
  LocalFrameView* frame_view = node->GetDocument().View();
  if (frame_view)
    scale_ = 1.f / frame_view->GetChromeClient()->WindowToViewportScalar(1.f);
  AppendPathsForShapeOutside(node, highlight_config);
  AppendNodeHighlight(node, highlight_config);
  if (append_element_info && node->IsElementNode())
    element_info_ = BuildElementInfo(ToElement(node));
}

InspectorHighlight::~InspectorHighlight() = default;

void InspectorHighlight::AppendQuad(const FloatQuad& quad,
                                    const Color& fill_color,
                                    const Color& outline_color,
                                    const String& name) {
  Path path = QuadToPath(quad);
  PathBuilder builder;
  builder.AppendPath(path, scale_);
  AppendPath(builder.Release(), fill_color, outline_color, name);
}

void InspectorHighlight::AppendPath(std::unique_ptr<protocol::ListValue> path,
                                    const Color& fill_color,
                                    const Color& outline_color,
                                    const String& name) {
  std::unique_ptr<protocol::DictionaryValue> object =
      protocol::DictionaryValue::create();
  object->setValue("path", std::move(path));
  object->setString("fillColor", fill_color.Serialized());
  if (outline_color != Color::kTransparent)
    object->setString("outlineColor", outline_color.Serialized());
  if (!name.IsEmpty())
    object->setString("name", name);
  highlight_paths_->pushValue(std::move(object));
}

void InspectorHighlight::AppendEventTargetQuads(
    Node* event_target_node,
    const InspectorHighlightConfig& highlight_config) {
  if (event_target_node->GetLayoutObject()) {
    FloatQuad border, unused;
    if (BuildNodeQuads(event_target_node, &unused, &unused, &border, &unused))
      AppendQuad(border, highlight_config.event_target);
  }
}

void InspectorHighlight::AppendPathsForShapeOutside(
    Node* node,
    const InspectorHighlightConfig& config) {
  Shape::DisplayPaths paths;
  FloatQuad bounds_quad;

  const ShapeOutsideInfo* shape_outside_info =
      ShapeOutsideInfoForNode(node, &paths, &bounds_quad);
  if (!shape_outside_info)
    return;

  if (!paths.shape.length()) {
    AppendQuad(bounds_quad, config.shape);
    return;
  }

  AppendPath(ShapePathBuilder::BuildPath(
                 *node->GetDocument().View(), *node->GetLayoutObject(),
                 *shape_outside_info, paths.shape, scale_),
             config.shape, Color::kTransparent);
  if (paths.margin_shape.length())
    AppendPath(ShapePathBuilder::BuildPath(
                   *node->GetDocument().View(), *node->GetLayoutObject(),
                   *shape_outside_info, paths.margin_shape, scale_),
               config.shape_margin, Color::kTransparent);
}

void InspectorHighlight::AppendNodeHighlight(
    Node* node,
    const InspectorHighlightConfig& highlight_config) {
  LayoutObject* layout_object = node->GetLayoutObject();
  if (!layout_object)
    return;

  // LayoutSVGRoot should be highlighted through the isBox() code path, all
  // other SVG elements should just dump their absoluteQuads().
  if (layout_object->GetNode() && layout_object->GetNode()->IsSVGElement() &&
      !layout_object->IsSVGRoot()) {
    Vector<FloatQuad> quads;
    layout_object->AbsoluteQuads(quads);
    LocalFrameView* containing_view = layout_object->GetFrameView();
    for (size_t i = 0; i < quads.size(); ++i) {
      if (containing_view)
        ContentsQuadToViewport(containing_view, quads[i]);
      AppendQuad(quads[i], highlight_config.content,
                 highlight_config.content_outline);
    }
    return;
  }

  FloatQuad content, padding, border, margin;
  if (!BuildNodeQuads(node, &content, &padding, &border, &margin))
    return;
  AppendQuad(content, highlight_config.content,
             highlight_config.content_outline, "content");
  AppendQuad(padding, highlight_config.padding, Color::kTransparent, "padding");
  AppendQuad(border, highlight_config.border, Color::kTransparent, "border");
  AppendQuad(margin, highlight_config.margin, Color::kTransparent, "margin");

  if (highlight_config.css_grid == Color::kTransparent)
    return;
  grid_info_ = protocol::ListValue::create();
  if (layout_object->IsLayoutGrid()) {
    grid_info_->pushValue(BuildGridInfo(ToLayoutGrid(layout_object),
                                        border.P1(), highlight_config.css_grid,
                                        scale_, true));
  }
  LayoutObject* parent = layout_object->Parent();
  if (!parent || !parent->IsLayoutGrid())
    return;
  if (!BuildNodeQuads(parent->GetNode(), &content, &padding, &border, &margin))
    return;
  grid_info_->pushValue(BuildGridInfo(ToLayoutGrid(parent), border.P1(),
                                      highlight_config.css_grid, scale_,
                                      false));
}

std::unique_ptr<protocol::DictionaryValue> InspectorHighlight::AsProtocolValue()
    const {
  std::unique_ptr<protocol::DictionaryValue> object =
      protocol::DictionaryValue::create();
  object->setValue("paths", highlight_paths_->clone());
  object->setBoolean("showRulers", show_rulers_);
  object->setBoolean("showExtensionLines", show_extension_lines_);
  if (element_info_)
    object->setValue("elementInfo", element_info_->clone());
  object->setBoolean("displayAsMaterial", display_as_material_);
  if (grid_info_ && grid_info_->size() > 0)
    object->setValue("gridInfo", grid_info_->clone());
  return object;
}

// static
bool InspectorHighlight::GetBoxModel(
    Node* node,
    std::unique_ptr<protocol::DOM::BoxModel>* model) {
  node->GetDocument().EnsurePaintLocationDataValidForNode(node);
  LayoutObject* layout_object = node->GetLayoutObject();
  LocalFrameView* view = node->GetDocument().View();
  if (!layout_object || !view)
    return false;

  FloatQuad content, padding, border, margin;
  if (!BuildNodeQuads(node, &content, &padding, &border, &margin))
    return false;

  AdjustForAbsoluteZoom::AdjustFloatQuad(content, *layout_object);
  AdjustForAbsoluteZoom::AdjustFloatQuad(padding, *layout_object);
  AdjustForAbsoluteZoom::AdjustFloatQuad(border, *layout_object);
  AdjustForAbsoluteZoom::AdjustFloatQuad(margin, *layout_object);

  float scale = 1 / view->GetPage()->GetVisualViewport().Scale();
  content.Scale(scale, scale);
  padding.Scale(scale, scale);
  border.Scale(scale, scale);
  margin.Scale(scale, scale);

  IntRect bounding_box =
      view->ContentsToRootFrame(layout_object->AbsoluteBoundingBoxRect());
  LayoutBoxModelObject* model_object =
      layout_object->IsBoxModelObject() ? ToLayoutBoxModelObject(layout_object)
                                        : nullptr;

  *model =
      protocol::DOM::BoxModel::create()
          .setContent(BuildArrayForQuad(content))
          .setPadding(BuildArrayForQuad(padding))
          .setBorder(BuildArrayForQuad(border))
          .setMargin(BuildArrayForQuad(margin))
          .setWidth(model_object ? AdjustForAbsoluteZoom::AdjustInt(
                                       model_object->PixelSnappedOffsetWidth(
                                           model_object->OffsetParent()),
                                       model_object)
                                 : bounding_box.Width())
          .setHeight(model_object ? AdjustForAbsoluteZoom::AdjustInt(
                                        model_object->PixelSnappedOffsetHeight(
                                            model_object->OffsetParent()),
                                        model_object)
                                  : bounding_box.Height())
          .build();

  Shape::DisplayPaths paths;
  FloatQuad bounds_quad;
  protocol::ErrorSupport errors;
  if (const ShapeOutsideInfo* shape_outside_info =
          ShapeOutsideInfoForNode(node, &paths, &bounds_quad)) {
    (*model)->setShapeOutside(
        protocol::DOM::ShapeOutsideInfo::create()
            .setBounds(BuildArrayForQuad(bounds_quad))
            .setShape(protocol::Array<protocol::Value>::fromValue(
                ShapePathBuilder::BuildPath(*view, *layout_object,
                                            *shape_outside_info, paths.shape,
                                            1.f)
                    .get(),
                &errors))
            .setMarginShape(protocol::Array<protocol::Value>::fromValue(
                ShapePathBuilder::BuildPath(*view, *layout_object,
                                            *shape_outside_info,
                                            paths.margin_shape, 1.f)
                    .get(),
                &errors))
            .build());
  }

  return true;
}

bool InspectorHighlight::BuildNodeQuads(Node* node,
                                        FloatQuad* content,
                                        FloatQuad* padding,
                                        FloatQuad* border,
                                        FloatQuad* margin) {
  LayoutObject* layout_object = node->GetLayoutObject();
  if (!layout_object)
    return false;

  LocalFrameView* containing_view = layout_object->GetFrameView();
  if (!containing_view)
    return false;
  if (!layout_object->IsBox() && !layout_object->IsLayoutInline())
    return false;

  LayoutRect content_box;
  LayoutRect padding_box;
  LayoutRect border_box;
  LayoutRect margin_box;

  if (layout_object->IsBox()) {
    LayoutBox* layout_box = ToLayoutBox(layout_object);

    // LayoutBox returns the "pure" content area box, exclusive of the
    // scrollbars (if present), which also count towards the content area in
    // CSS.
    const int vertical_scrollbar_width = layout_box->VerticalScrollbarWidth();
    const int horizontal_scrollbar_height =
        layout_box->HorizontalScrollbarHeight();
    content_box = layout_box->ContentBoxRect();
    content_box.SetWidth(content_box.Width() + vertical_scrollbar_width);
    content_box.SetHeight(content_box.Height() + horizontal_scrollbar_height);

    padding_box = layout_box->PaddingBoxRect();
    padding_box.SetWidth(padding_box.Width() + vertical_scrollbar_width);
    padding_box.SetHeight(padding_box.Height() + horizontal_scrollbar_height);

    border_box = layout_box->BorderBoxRect();

    margin_box = LayoutRect(border_box.X() - layout_box->MarginLeft(),
                            border_box.Y() - layout_box->MarginTop(),
                            border_box.Width() + layout_box->MarginWidth(),
                            border_box.Height() + layout_box->MarginHeight());
  } else {
    LayoutInline* layout_inline = ToLayoutInline(layout_object);

    // LayoutInline's bounding box includes paddings and borders, excludes
    // margins.
    border_box = LayoutRect(layout_inline->LinesBoundingBox());
    padding_box = LayoutRect(border_box.X() + layout_inline->BorderLeft(),
                             border_box.Y() + layout_inline->BorderTop(),
                             border_box.Width() - layout_inline->BorderLeft() -
                                 layout_inline->BorderRight(),
                             border_box.Height() - layout_inline->BorderTop() -
                                 layout_inline->BorderBottom());
    content_box =
        LayoutRect(padding_box.X() + layout_inline->PaddingLeft(),
                   padding_box.Y() + layout_inline->PaddingTop(),
                   padding_box.Width() - layout_inline->PaddingLeft() -
                       layout_inline->PaddingRight(),
                   padding_box.Height() - layout_inline->PaddingTop() -
                       layout_inline->PaddingBottom());
    // Ignore marginTop and marginBottom for inlines.
    margin_box = LayoutRect(
        border_box.X() - layout_inline->MarginLeft(), border_box.Y(),
        border_box.Width() + layout_inline->MarginWidth(), border_box.Height());
  }

  *content = layout_object->LocalToAbsoluteQuad(FloatRect(content_box));
  *padding = layout_object->LocalToAbsoluteQuad(FloatRect(padding_box));
  *border = layout_object->LocalToAbsoluteQuad(FloatRect(border_box));
  *margin = layout_object->LocalToAbsoluteQuad(FloatRect(margin_box));

  ContentsQuadToViewport(containing_view, *content);
  ContentsQuadToViewport(containing_view, *padding);
  ContentsQuadToViewport(containing_view, *border);
  ContentsQuadToViewport(containing_view, *margin);

  return true;
}

// static
InspectorHighlightConfig InspectorHighlight::DefaultConfig() {
  InspectorHighlightConfig config;
  config.content = Color(255, 0, 0, 0);
  config.content_outline = Color(128, 0, 0, 0);
  config.padding = Color(0, 255, 0, 0);
  config.border = Color(0, 0, 255, 0);
  config.margin = Color(255, 255, 255, 0);
  config.event_target = Color(128, 128, 128, 0);
  config.shape = Color(0, 0, 0, 0);
  config.shape_margin = Color(128, 128, 128, 0);
  config.show_info = true;
  config.show_rulers = true;
  config.show_extension_lines = true;
  config.display_as_material = false;
  config.css_grid = Color(128, 128, 128, 0);
  return config;
}

}  // namespace blink
