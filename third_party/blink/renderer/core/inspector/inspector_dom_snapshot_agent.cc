// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/inspector_dom_snapshot_agent.h"

#include "third_party/blink/renderer/bindings/core/v8/script_event_listener.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/css/css_computed_style_declaration.h"
#include "third_party/blink/renderer/core/dom/attribute.h"
#include "third_party/blink/renderer/core/dom/attribute_collection.h"
#include "third_party/blink/renderer/core/dom/character_data.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_type.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/forms/html_option_element.h"
#include "third_party/blink/renderer/core/html/forms/html_text_area_element.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/html_link_element.h"
#include "third_party/blink/renderer/core/html/html_template_element.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_debugger_agent.h"
#include "third_party/blink/renderer/core/inspector/thread_debugger.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/line/inline_text_box.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node.h"
#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node_iterator.h"
#include "v8/include/v8-inspector.h"

namespace blink {

using protocol::Maybe;
using protocol::Response;

namespace DOMSnapshotAgentState {
static const char kDomSnapshotAgentEnabled[] = "DOMSnapshotAgentEnabled";
};

namespace {

std::unique_ptr<protocol::DOM::Rect> BuildRectForFloatRect(
    const FloatRect& rect) {
  return protocol::DOM::Rect::create()
      .setX(rect.X())
      .setY(rect.Y())
      .setWidth(rect.Width())
      .setHeight(rect.Height())
      .build();
}

Document* GetEmbeddedDocument(PaintLayer* layer) {
  // Documents are embedded on their own PaintLayer via a LayoutEmbeddedContent.
  if (layer->GetLayoutObject().IsLayoutEmbeddedContent()) {
    FrameView* frame_view =
        ToLayoutEmbeddedContent(layer->GetLayoutObject()).ChildFrameView();
    if (frame_view && frame_view->IsLocalFrameView()) {
      LocalFrameView* local_frame_view = ToLocalFrameView(frame_view);
      return local_frame_view->GetFrame().GetDocument();
    }
  }
  return nullptr;
}

}  // namespace

struct InspectorDOMSnapshotAgent::VectorStringHashTraits
    : public WTF::GenericHashTraits<Vector<String>> {
  static unsigned GetHash(const Vector<String>& vec) {
    unsigned h = DefaultHash<size_t>::Hash::GetHash(vec.size());
    for (size_t i = 0; i < vec.size(); i++) {
      h = WTF::HashInts(h, DefaultHash<String>::Hash::GetHash(vec[i]));
    }
    return h;
  }

  static bool Equal(const Vector<String>& a, const Vector<String>& b) {
    if (a.size() != b.size())
      return false;
    for (size_t i = 0; i < a.size(); i++) {
      if (a[i] != b[i])
        return false;
    }
    return true;
  }

  static void ConstructDeletedValue(Vector<String>& vec, bool) {
    new (NotNull, &vec) Vector<String>(WTF::kHashTableDeletedValue);
  }

  static bool IsDeletedValue(const Vector<String>& vec) {
    return vec.IsHashTableDeletedValue();
  }

  static bool IsEmptyValue(const Vector<String>& vec) { return vec.IsEmpty(); }

  static const bool kEmptyValueIsZero = false;
  static const bool safe_to_compare_to_empty_or_deleted = false;
  static const bool kHasIsEmptyValueFunction = true;
};

InspectorDOMSnapshotAgent::InspectorDOMSnapshotAgent(
    InspectedFrames* inspected_frames,
    InspectorDOMDebuggerAgent* dom_debugger_agent)
    : inspected_frames_(inspected_frames),
      dom_debugger_agent_(dom_debugger_agent) {}

InspectorDOMSnapshotAgent::~InspectorDOMSnapshotAgent() = default;

bool InspectorDOMSnapshotAgent::Enabled() const {
  return state_->booleanProperty(
      DOMSnapshotAgentState::kDomSnapshotAgentEnabled, false);
}

void InspectorDOMSnapshotAgent::GetOriginUrl(String* origin_url_ptr,
                                             const Node* node) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  ThreadDebugger* debugger = ThreadDebugger::From(isolate);
  if (!isolate || !isolate->InContext() || !debugger) {
    origin_url_ptr = nullptr;
    return;
  }
  // First try searching in one frame, since grabbing full trace is
  // expensive.
  auto trace = debugger->GetV8Inspector()->captureStackTrace(false);
  if (!trace) {
    origin_url_ptr = nullptr;
    return;
  }
  if (!trace->firstNonEmptySourceURL().length())
    trace = debugger->GetV8Inspector()->captureStackTrace(true);
  String origin_url = ToCoreString(trace->firstNonEmptySourceURL());
  if (origin_url.IsEmpty()) {
    // Fall back to document url.
    origin_url = node->GetDocument().Url().GetString();
  }
  *origin_url_ptr = origin_url;
}

void InspectorDOMSnapshotAgent::CharacterDataModified(
    CharacterData* character_data) {
  String origin_url;
  GetOriginUrl(&origin_url, character_data);
  if (origin_url)
    origin_url_map_->insert(DOMNodeIds::IdForNode(character_data), origin_url);
}

void InspectorDOMSnapshotAgent::DidInsertDOMNode(Node* node) {
  String origin_url;
  GetOriginUrl(&origin_url, node);
  if (origin_url)
    origin_url_map_->insert(DOMNodeIds::IdForNode(node), origin_url);
}

void InspectorDOMSnapshotAgent::InnerEnable() {
  state_->setBoolean(DOMSnapshotAgentState::kDomSnapshotAgentEnabled, true);
  origin_url_map_ = std::make_unique<OriginUrlMap>();
  instrumenting_agents_->addInspectorDOMSnapshotAgent(this);
}

void InspectorDOMSnapshotAgent::Restore() {
  if (!Enabled())
    return;
  InnerEnable();
}

Response InspectorDOMSnapshotAgent::enable() {
  if (!Enabled())
    InnerEnable();
  return Response::OK();
}

Response InspectorDOMSnapshotAgent::disable() {
  if (!Enabled())
    return Response::Error("DOM snapshot agent hasn't been enabled.");
  state_->setBoolean(DOMSnapshotAgentState::kDomSnapshotAgentEnabled, false);
  origin_url_map_.reset();
  instrumenting_agents_->removeInspectorDOMSnapshotAgent(this);
  return Response::OK();
}

Response InspectorDOMSnapshotAgent::getSnapshot(
    std::unique_ptr<protocol::Array<String>> style_whitelist,
    protocol::Maybe<bool> include_event_listeners,
    protocol::Maybe<bool> include_paint_order,
    protocol::Maybe<bool> include_user_agent_shadow_tree,
    std::unique_ptr<protocol::Array<protocol::DOMSnapshot::DOMNode>>* dom_nodes,
    std::unique_ptr<protocol::Array<protocol::DOMSnapshot::LayoutTreeNode>>*
        layout_tree_nodes,
    std::unique_ptr<protocol::Array<protocol::DOMSnapshot::ComputedStyle>>*
        computed_styles) {
  DCHECK(!dom_nodes_ && !layout_tree_nodes_ && !computed_styles_);

  Document* document = inspected_frames_->Root()->GetDocument();
  if (!document)
    return Response::Error("Document is not available");

  // Setup snapshot.
  dom_nodes_ = protocol::Array<protocol::DOMSnapshot::DOMNode>::create();
  layout_tree_nodes_ =
      protocol::Array<protocol::DOMSnapshot::LayoutTreeNode>::create();
  computed_styles_ =
      protocol::Array<protocol::DOMSnapshot::ComputedStyle>::create();
  computed_styles_map_ = std::make_unique<ComputedStylesMap>();
  css_property_whitelist_ = std::make_unique<CSSPropertyWhitelist>();

  // Look up the CSSPropertyIDs for each entry in |style_whitelist|.
  for (size_t i = 0; i < style_whitelist->length(); i++) {
    CSSPropertyID property_id = cssPropertyID(style_whitelist->get(i));
    if (property_id == CSSPropertyInvalid)
      continue;
    css_property_whitelist_->push_back(
        std::make_pair(style_whitelist->get(i), property_id));
  }

  if (include_paint_order.fromMaybe(false)) {
    paint_order_map_ = std::make_unique<PaintOrderMap>();
    next_paint_order_index_ = 0;
    TraversePaintLayerTree(document);
  }

  // Actual traversal.
  VisitNode(document, include_event_listeners.fromMaybe(false),
            include_user_agent_shadow_tree.fromMaybe(false));

  // Extract results from state and reset.
  *dom_nodes = std::move(dom_nodes_);
  *layout_tree_nodes = std::move(layout_tree_nodes_);
  *computed_styles = std::move(computed_styles_);
  computed_styles_map_.reset();
  css_property_whitelist_.reset();
  paint_order_map_.reset();
  return Response::OK();
}

int InspectorDOMSnapshotAgent::VisitNode(Node* node,
                                         bool include_event_listeners,
                                         bool include_user_agent_shadow_tree) {
  // Update layout tree before traversal of document so that we inspect a
  // current and consistent state of all trees. No need to do this if paint
  // order was calculated, since layout trees were already updated during
  // TraversePaintLayerTree().
  if (node->IsDocumentNode() && !paint_order_map_)
    node->GetDocument().UpdateStyleAndLayoutTree();

  String node_value;
  switch (node->getNodeType()) {
    case Node::kTextNode:
    case Node::kAttributeNode:
    case Node::kCommentNode:
    case Node::kCdataSectionNode:
    case Node::kDocumentFragmentNode:
      node_value = node->nodeValue();
      break;
    default:
      break;
  }

  // Create DOMNode object and add it to the result array before traversing
  // children, so that parents appear before their children in the array.
  std::unique_ptr<protocol::DOMSnapshot::DOMNode> owned_value =
      protocol::DOMSnapshot::DOMNode::create()
          .setNodeType(static_cast<int>(node->getNodeType()))
          .setNodeName(node->nodeName())
          .setNodeValue(node_value)
          .setBackendNodeId(DOMNodeIds::IdForNode(node))
          .build();
  if (origin_url_map_ &&
      origin_url_map_->Contains(owned_value->getBackendNodeId())) {
    String origin_url = origin_url_map_->at(owned_value->getBackendNodeId());
    // In common cases, it is implicit that a child node would have the same
    // origin url as its parent, so no need to mark twice.
    if (!node->parentNode() || origin_url_map_->at(DOMNodeIds::IdForNode(
                                   node->parentNode())) != origin_url) {
      owned_value->setOriginURL(
          origin_url_map_->at(owned_value->getBackendNodeId()));
    }
  }
  protocol::DOMSnapshot::DOMNode* value = owned_value.get();
  int index = dom_nodes_->length();
  dom_nodes_->addItem(std::move(owned_value));

  int layoutNodeIndex = VisitLayoutTreeNode(node, index);
  if (layoutNodeIndex != -1)
    value->setLayoutNodeIndex(layoutNodeIndex);

  if (node->WillRespondToMouseClickEvents())
    value->setIsClickable(true);

  if (include_event_listeners && node->GetDocument().GetFrame()) {
    ScriptState* script_state =
        ToScriptStateForMainWorld(node->GetDocument().GetFrame());
    if (script_state->ContextIsValid()) {
      ScriptState::Scope scope(script_state);
      v8::Local<v8::Context> context = script_state->GetContext();
      V8EventListenerInfoList event_information;
      InspectorDOMDebuggerAgent::CollectEventListeners(
          script_state->GetIsolate(), node, v8::Local<v8::Value>(), node, true,
          &event_information);
      if (!event_information.IsEmpty()) {
        value->setEventListeners(
            dom_debugger_agent_->BuildObjectsForEventListeners(
                event_information, context, v8_inspector::StringView()));
      }
    }
  }

  if (node->IsElementNode()) {
    Element* element = ToElement(node);
    value->setAttributes(BuildArrayForElementAttributes(element));

    if (node->IsFrameOwnerElement()) {
      const HTMLFrameOwnerElement* frame_owner = ToHTMLFrameOwnerElement(node);
      if (LocalFrame* frame =
              frame_owner->ContentFrame() &&
                      frame_owner->ContentFrame()->IsLocalFrame()
                  ? ToLocalFrame(frame_owner->ContentFrame())
                  : nullptr) {
        value->setFrameId(IdentifiersFactory::FrameId(frame));
      }
      if (Document* doc = frame_owner->contentDocument()) {
        value->setContentDocumentIndex(VisitNode(
            doc, include_event_listeners, include_user_agent_shadow_tree));
      }
    }

    if (node->parentNode() && node->parentNode()->IsDocumentNode()) {
      LocalFrame* frame = node->GetDocument().GetFrame();
      if (frame)
        value->setFrameId(IdentifiersFactory::FrameId(frame));
    }

    if (auto* link_element = ToHTMLLinkElementOrNull(*element)) {
      if (link_element->IsImport() && link_element->import() &&
          InspectorDOMAgent::InnerParentNode(link_element->import()) ==
              link_element) {
        value->setImportedDocumentIndex(
            VisitNode(link_element->import(), include_event_listeners,
                      include_user_agent_shadow_tree));
      }
    }

    if (auto* template_element = ToHTMLTemplateElementOrNull(*element)) {
      value->setTemplateContentIndex(VisitNode(template_element->content(),
                                               include_event_listeners,
                                               include_user_agent_shadow_tree));
    }

    if (auto* textarea_element = ToHTMLTextAreaElementOrNull(*element))
      value->setTextValue(textarea_element->value());

    if (auto* input_element = ToHTMLInputElementOrNull(*element)) {
      value->setInputValue(input_element->value());
      if ((input_element->type() == InputTypeNames::radio) ||
          (input_element->type() == InputTypeNames::checkbox)) {
        value->setInputChecked(input_element->checked());
      }
    }

    if (auto* option_element = ToHTMLOptionElementOrNull(*element))
      value->setOptionSelected(option_element->Selected());

    if (element->GetPseudoId()) {
      protocol::DOM::PseudoType pseudo_type;
      if (InspectorDOMAgent::GetPseudoElementType(element->GetPseudoId(),
                                                  &pseudo_type)) {
        value->setPseudoType(pseudo_type);
      }
    } else {
      value->setPseudoElementIndexes(VisitPseudoElements(
          element, include_event_listeners, include_user_agent_shadow_tree));
    }

    HTMLImageElement* image_element = ToHTMLImageElementOrNull(node);
    if (image_element)
      value->setCurrentSourceURL(image_element->currentSrc());
  } else if (node->IsDocumentNode()) {
    Document* document = ToDocument(node);
    value->setDocumentURL(InspectorDOMAgent::DocumentURLString(document));
    value->setBaseURL(InspectorDOMAgent::DocumentBaseURLString(document));
    if (document->ContentLanguage())
      value->setContentLanguage(document->ContentLanguage().Utf8().data());
    if (document->EncodingName())
      value->setDocumentEncoding(document->EncodingName().Utf8().data());
    value->setFrameId(IdentifiersFactory::FrameId(document->GetFrame()));
  } else if (node->IsDocumentTypeNode()) {
    DocumentType* doc_type = ToDocumentType(node);
    value->setPublicId(doc_type->publicId());
    value->setSystemId(doc_type->systemId());
  }
  if (node->IsInShadowTree()) {
    value->setShadowRootType(
        InspectorDOMAgent::GetShadowRootType(node->ContainingShadowRoot()));
  }

  if (node->IsContainerNode()) {
    value->setChildNodeIndexes(VisitContainerChildren(
        node, include_event_listeners, include_user_agent_shadow_tree));
  }
  return index;
}

Node* InspectorDOMSnapshotAgent::FirstChild(
    const Node& node,
    bool include_user_agent_shadow_tree) {
  DCHECK(include_user_agent_shadow_tree || !node.IsInUserAgentShadowRoot());
  if (!include_user_agent_shadow_tree) {
    ShadowRoot* shadow_root = node.GetShadowRoot();
    if (shadow_root && shadow_root->GetType() == ShadowRootType::kUserAgent) {
      Node* child = node.firstChild();
      while (child && !child->CanParticipateInFlatTree())
        child = child->nextSibling();
      return child;
    }
  }
  return FlatTreeTraversal::FirstChild(node);
}

bool InspectorDOMSnapshotAgent::HasChildren(
    const Node& node,
    bool include_user_agent_shadow_tree) {
  return FirstChild(node, include_user_agent_shadow_tree);
}

Node* InspectorDOMSnapshotAgent::NextSibling(
    const Node& node,
    bool include_user_agent_shadow_tree) {
  DCHECK(include_user_agent_shadow_tree || !node.IsInUserAgentShadowRoot());
  if (!include_user_agent_shadow_tree) {
    if (node.ParentElementShadowRoot() &&
        node.ParentElementShadowRoot()->GetType() ==
            ShadowRootType::kUserAgent) {
      Node* sibling = node.nextSibling();
      while (sibling && !sibling->CanParticipateInFlatTree())
        sibling = sibling->nextSibling();
      return sibling;
    }
  }
  return FlatTreeTraversal::NextSibling(node);
}

std::unique_ptr<protocol::Array<int>>
InspectorDOMSnapshotAgent::VisitContainerChildren(
    Node* container,
    bool include_event_listeners,
    bool include_user_agent_shadow_tree) {
  auto children = protocol::Array<int>::create();

  if (!HasChildren(*container, include_user_agent_shadow_tree))
    return nullptr;

  Node* child = FirstChild(*container, include_user_agent_shadow_tree);
  while (child) {
    children->addItem(VisitNode(child, include_event_listeners,
                                include_user_agent_shadow_tree));
    child = NextSibling(*child, include_user_agent_shadow_tree);
  }

  return children;
}

std::unique_ptr<protocol::Array<int>>
InspectorDOMSnapshotAgent::VisitPseudoElements(
    Element* parent,
    bool include_event_listeners,
    bool include_user_agent_shadow_tree) {
  if (!parent->GetPseudoElement(kPseudoIdBefore) &&
      !parent->GetPseudoElement(kPseudoIdAfter)) {
    return nullptr;
  }

  auto pseudo_elements = protocol::Array<int>::create();

  if (parent->GetPseudoElement(kPseudoIdBefore)) {
    pseudo_elements->addItem(
        VisitNode(parent->GetPseudoElement(kPseudoIdBefore),
                  include_event_listeners, include_user_agent_shadow_tree));
  }
  if (parent->GetPseudoElement(kPseudoIdAfter)) {
    pseudo_elements->addItem(VisitNode(parent->GetPseudoElement(kPseudoIdAfter),
                                       include_event_listeners,
                                       include_user_agent_shadow_tree));
  }

  return pseudo_elements;
}

std::unique_ptr<protocol::Array<protocol::DOMSnapshot::NameValue>>
InspectorDOMSnapshotAgent::BuildArrayForElementAttributes(Element* element) {
  auto attributes_value =
      protocol::Array<protocol::DOMSnapshot::NameValue>::create();
  AttributeCollection attributes = element->Attributes();
  for (const auto& attribute : attributes) {
    attributes_value->addItem(protocol::DOMSnapshot::NameValue::create()
                                  .setName(attribute.GetName().ToString())
                                  .setValue(attribute.Value())
                                  .build());
  }
  if (attributes_value->length() == 0)
    return nullptr;
  return attributes_value;
}

int InspectorDOMSnapshotAgent::VisitLayoutTreeNode(Node* node, int node_index) {
  LayoutObject* layout_object = node->GetLayoutObject();
  if (!layout_object)
    return -1;

  auto layout_tree_node = protocol::DOMSnapshot::LayoutTreeNode::create()
                              .setDomNodeIndex(node_index)
                              .setBoundingBox(BuildRectForFloatRect(
                                  layout_object->AbsoluteBoundingBoxRect()))
                              .build();

  int style_index = GetStyleIndexForNode(node);
  if (style_index != -1)
    layout_tree_node->setStyleIndex(style_index);

  if (paint_order_map_) {
    PaintLayer* paint_layer = layout_object->EnclosingLayer();

    // We visited all PaintLayers when building |paint_order_map_|.
    DCHECK(paint_order_map_->Contains(paint_layer));

    if (int paint_order = paint_order_map_->at(paint_layer))
      layout_tree_node->setPaintOrder(paint_order);
  }

  if (layout_object->IsText()) {
    LayoutText* layout_text = ToLayoutText(layout_object);
    layout_tree_node->setLayoutText(layout_text->GetText());
    if (layout_text->HasTextBoxes()) {
      std::unique_ptr<protocol::Array<protocol::DOMSnapshot::InlineTextBox>>
          inline_text_nodes =
              protocol::Array<protocol::DOMSnapshot::InlineTextBox>::create();
      for (const InlineTextBox* text_box : layout_text->TextBoxes()) {
        FloatRect local_coords_text_box_rect(text_box->FrameRect());
        FloatRect absolute_coords_text_box_rect =
            layout_object->LocalToAbsoluteQuad(local_coords_text_box_rect)
                .BoundingBox();
        inline_text_nodes->addItem(
            protocol::DOMSnapshot::InlineTextBox::create()
                .setStartCharacterIndex(text_box->Start())
                .setNumCharacters(text_box->Len())
                .setBoundingBox(
                    BuildRectForFloatRect(absolute_coords_text_box_rect))
                .build());
      }
      layout_tree_node->setInlineTextNodes(std::move(inline_text_nodes));
    }
  }

  int index = layout_tree_nodes_->length();
  layout_tree_nodes_->addItem(std::move(layout_tree_node));
  return index;
}

int InspectorDOMSnapshotAgent::GetStyleIndexForNode(Node* node) {
  CSSComputedStyleDeclaration* computed_style_info =
      CSSComputedStyleDeclaration::Create(node, true);

  Vector<String> style;
  bool all_properties_empty = true;
  for (const auto& pair : *css_property_whitelist_) {
    String value = computed_style_info->GetPropertyValue(pair.second);
    if (!value.IsEmpty())
      all_properties_empty = false;
    style.push_back(value);
  }

  // -1 means an empty style.
  if (all_properties_empty)
    return -1;

  ComputedStylesMap::iterator it = computed_styles_map_->find(style);
  if (it != computed_styles_map_->end())
    return it->value;

  // It's a distinct style, so append to |computedStyles|.
  auto style_properties =
      protocol::Array<protocol::DOMSnapshot::NameValue>::create();

  for (size_t i = 0; i < style.size(); i++) {
    if (style[i].IsEmpty())
      continue;
    style_properties->addItem(protocol::DOMSnapshot::NameValue::create()
                                  .setName((*css_property_whitelist_)[i].first)
                                  .setValue(style[i])
                                  .build());
  }

  size_t index = computed_styles_->length();
  computed_styles_->addItem(protocol::DOMSnapshot::ComputedStyle::create()
                                .setProperties(std::move(style_properties))
                                .build());
  computed_styles_map_->insert(std::move(style), index);
  return index;
}

void InspectorDOMSnapshotAgent::TraversePaintLayerTree(Document* document) {
  // Update layout tree before traversal of document so that we inspect a
  // current and consistent state of all trees.
  document->UpdateStyleAndLayoutTree();

  PaintLayer* root_layer = document->GetLayoutView()->Layer();
  // LayoutView requires a PaintLayer.
  DCHECK(root_layer);

  VisitPaintLayer(root_layer);
}

void InspectorDOMSnapshotAgent::VisitPaintLayer(PaintLayer* layer) {
  DCHECK(!paint_order_map_->Contains(layer));

  paint_order_map_->Set(layer, next_paint_order_index_);
  next_paint_order_index_++;

  // If there is an embedded document, integrate it into the painting order.
  Document* embedded_document = GetEmbeddedDocument(layer);
  if (embedded_document)
    TraversePaintLayerTree(embedded_document);

  // If there's an embedded document, there shouldn't be any children.
  DCHECK(!embedded_document || !layer->FirstChild());

  if (!embedded_document) {
    PaintLayerStackingNode* node = layer->StackingNode();
    PaintLayerStackingNodeIterator iterator(*node, kAllChildren);
    while (PaintLayerStackingNode* child_node = iterator.Next()) {
      VisitPaintLayer(child_node->Layer());
    }
  }
}

void InspectorDOMSnapshotAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  visitor->Trace(dom_debugger_agent_);
  InspectorBaseAgent::Trace(visitor);
}

}  // namespace blink
