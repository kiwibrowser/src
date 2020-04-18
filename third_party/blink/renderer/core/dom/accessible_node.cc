// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/accessible_node.h"

#include "third_party/blink/renderer/core/dom/accessible_node_list.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

using namespace HTMLNames;

namespace {

QualifiedName GetCorrespondingARIAAttribute(AOMStringProperty property) {
  switch (property) {
    case AOMStringProperty::kAutocomplete:
      return aria_autocompleteAttr;
    case AOMStringProperty::kChecked:
      return aria_checkedAttr;
    case AOMStringProperty::kCurrent:
      return aria_currentAttr;
    case AOMStringProperty::kHasPopUp:
      return aria_haspopupAttr;
    case AOMStringProperty::kInvalid:
      return aria_invalidAttr;
    case AOMStringProperty::kKeyShortcuts:
      return aria_keyshortcutsAttr;
    case AOMStringProperty::kLabel:
      return aria_labelAttr;
    case AOMStringProperty::kLive:
      return aria_liveAttr;
    case AOMStringProperty::kOrientation:
      return aria_orientationAttr;
    case AOMStringProperty::kPlaceholder:
      return aria_placeholderAttr;
    case AOMStringProperty::kPressed:
      return aria_pressedAttr;
    case AOMStringProperty::kRelevant:
      return aria_relevantAttr;
    case AOMStringProperty::kRole:
      return roleAttr;
    case AOMStringProperty::kRoleDescription:
      return aria_roledescriptionAttr;
    case AOMStringProperty::kSort:
      return aria_sortAttr;
    case AOMStringProperty::kValueText:
      return aria_valuetextAttr;
  }

  NOTREACHED();
  return g_null_name;
}

QualifiedName GetCorrespondingARIAAttribute(AOMRelationProperty property) {
  switch (property) {
    case AOMRelationProperty::kActiveDescendant:
      return aria_activedescendantAttr;
      break;
    case AOMRelationProperty::kDetails:
      return aria_detailsAttr;
      break;
    case AOMRelationProperty::kErrorMessage:
      return aria_errormessageAttr;
      break;
  }

  NOTREACHED();
  return g_null_name;
}

QualifiedName GetCorrespondingARIAAttribute(AOMRelationListProperty property) {
  switch (property) {
    case AOMRelationListProperty::kDescribedBy:
      return aria_describedbyAttr;
      break;
    case AOMRelationListProperty::kControls:
      return aria_controlsAttr;
      break;
    case AOMRelationListProperty::kFlowTo:
      return aria_flowtoAttr;
      break;
    case AOMRelationListProperty::kLabeledBy:
      // Note that there are two allowed spellings of this attribute.
      // Callers should check both.
      return aria_labelledbyAttr;
      break;
    case AOMRelationListProperty::kOwns:
      return aria_ownsAttr;
      break;
  }

  NOTREACHED();
  return g_null_name;
}

QualifiedName GetCorrespondingARIAAttribute(AOMBooleanProperty property) {
  switch (property) {
    case AOMBooleanProperty::kAtomic:
      return aria_atomicAttr;
      break;
    case AOMBooleanProperty::kBusy:
      return aria_busyAttr;
      break;
    case AOMBooleanProperty::kDisabled:
      return aria_disabledAttr;
      break;
    case AOMBooleanProperty::kExpanded:
      return aria_expandedAttr;
      break;
    case AOMBooleanProperty::kHidden:
      return aria_hiddenAttr;
      break;
    case AOMBooleanProperty::kModal:
      return aria_modalAttr;
      break;
    case AOMBooleanProperty::kMultiline:
      return aria_multilineAttr;
      break;
    case AOMBooleanProperty::kMultiselectable:
      return aria_multiselectableAttr;
      break;
    case AOMBooleanProperty::kReadOnly:
      return aria_readonlyAttr;
      break;
    case AOMBooleanProperty::kRequired:
      return aria_requiredAttr;
      break;
    case AOMBooleanProperty::kSelected:
      return aria_selectedAttr;
      break;
  }

  NOTREACHED();
  return g_null_name;
}

QualifiedName GetCorrespondingARIAAttribute(AOMFloatProperty property) {
  AtomicString attr_value;
  switch (property) {
    case AOMFloatProperty::kValueMax:
      return aria_valuemaxAttr;
      break;
    case AOMFloatProperty::kValueMin:
      return aria_valueminAttr;
      break;
    case AOMFloatProperty::kValueNow:
      return aria_valuenowAttr;
      break;
  }

  NOTREACHED();
  return g_null_name;
}

QualifiedName GetCorrespondingARIAAttribute(AOMUIntProperty property) {
  switch (property) {
    case AOMUIntProperty::kColIndex:
      return aria_colindexAttr;
      break;
    case AOMUIntProperty::kColSpan:
      return aria_colspanAttr;
      break;
    case AOMUIntProperty::kLevel:
      return aria_levelAttr;
      break;
    case AOMUIntProperty::kPosInSet:
      return aria_posinsetAttr;
      break;
    case AOMUIntProperty::kRowIndex:
      return aria_rowindexAttr;
      break;
    case AOMUIntProperty::kRowSpan:
      return aria_rowspanAttr;
      break;
  }

  NOTREACHED();
  return g_null_name;
}

QualifiedName GetCorrespondingARIAAttribute(AOMIntProperty property) {
  switch (property) {
    case AOMIntProperty::kColCount:
      return aria_colcountAttr;
      break;
    case AOMIntProperty::kRowCount:
      return aria_rowcountAttr;
      break;
    case AOMIntProperty::kSetSize:
      return aria_setsizeAttr;
      break;
  }

  NOTREACHED();
  return g_null_name;
}

}  // namespace

AccessibleNode::AccessibleNode(Element* element)
    : element_(element), document_(nullptr) {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
}

AccessibleNode::AccessibleNode(Document& document)
    : element_(nullptr), document_(document) {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
}

AccessibleNode::~AccessibleNode() = default;

// static
AccessibleNode* AccessibleNode::Create(Document& document) {
  return new AccessibleNode(document);
}

Document* AccessibleNode::GetDocument() const {
  if (document_)
    return document_;
  if (element_)
    return &element_->GetDocument();

  NOTREACHED();
  return nullptr;
}

const AtomicString& AccessibleNode::GetProperty(
    AOMStringProperty property) const {
  for (const auto& item : string_properties_) {
    if (item.first == property && !item.second.IsNull())
      return item.second;
  }

  return g_null_atom;
}

// static
AccessibleNode* AccessibleNode::GetProperty(Element* element,
                                            AOMRelationProperty property) {
  if (!element)
    return nullptr;

  if (AccessibleNode* accessible_node = element->ExistingAccessibleNode()) {
    for (const auto& item : accessible_node->relation_properties_) {
      if (item.first == property && item.second)
        return item.second;
    }
  }

  return nullptr;
}

// static
AccessibleNodeList* AccessibleNode::GetProperty(
    Element* element,
    AOMRelationListProperty property) {
  if (!element)
    return nullptr;

  if (AccessibleNode* accessible_node = element->ExistingAccessibleNode()) {
    for (const auto& item : accessible_node->relation_list_properties_) {
      if (item.first == property && item.second)
        return item.second;
    }
  }

  return nullptr;
}

// static
bool AccessibleNode::GetProperty(Element* element,
                                 AOMRelationListProperty property,
                                 HeapVector<Member<Element>>& targets) {
  AccessibleNodeList* node_list = GetProperty(element, property);
  if (!node_list)
    return false;

  for (size_t i = 0; i < node_list->length(); ++i) {
    AccessibleNode* accessible_node = node_list->item(i);
    if (accessible_node) {
      Element* element = accessible_node->element();
      if (element)
        targets.push_back(element);
    }
  }

  return true;
}

template <typename P, typename T>
static T FindPropertyValue(P property,
                           bool& is_null,
                           const Vector<std::pair<P, T>>& properties,
                           T default_value) {
  for (const auto& item : properties) {
    if (item.first == property) {
      is_null = false;
      return item.second;
    }
  }

  return default_value;
}

bool AccessibleNode::GetProperty(AOMBooleanProperty property,
                                 bool& is_null) const {
  is_null = true;
  return FindPropertyValue(property, is_null, boolean_properties_, false);
}

// static
float AccessibleNode::GetProperty(Element* element,
                                  AOMFloatProperty property,
                                  bool& is_null) {
  is_null = true;

  float default_value = 0.0;
  if (!element || !element->ExistingAccessibleNode())
    return default_value;

  return FindPropertyValue(property, is_null,
                           element->ExistingAccessibleNode()->float_properties_,
                           default_value);
}

// static
int32_t AccessibleNode::GetProperty(Element* element,
                                    AOMIntProperty property,
                                    bool& is_null) {
  is_null = true;

  int32_t default_value = 0;
  if (!element || !element->ExistingAccessibleNode())
    return default_value;

  return FindPropertyValue(property, is_null,
                           element->ExistingAccessibleNode()->int_properties_,
                           default_value);
}

// static
uint32_t AccessibleNode::GetProperty(Element* element,
                                     AOMUIntProperty property,
                                     bool& is_null) {
  is_null = true;

  uint32_t default_value = 0;
  if (!element || !element->ExistingAccessibleNode())
    return default_value;

  return FindPropertyValue(property, is_null,
                           element->ExistingAccessibleNode()->uint_properties_,
                           default_value);
}

bool AccessibleNode::IsUndefinedAttrValue(const AtomicString& value) {
  return value.IsEmpty() || EqualIgnoringASCIICase(value, "undefined");
}

// static
const AtomicString& AccessibleNode::GetPropertyOrARIAAttribute(
    Element* element,
    AOMStringProperty property) {
  if (!element)
    return g_null_atom;

  const bool is_token_attr = IsStringTokenProperty(property);
  AccessibleNode* accessible_node = element->ExistingAccessibleNode();
  if (accessible_node) {
    const AtomicString& result = accessible_node->GetProperty(property);
    if (!result.IsNull()) {
      if (is_token_attr && IsUndefinedAttrValue(result))
        return g_null_atom;  // Property specifically set to undefined value.
      return result;
    }
  }

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  const AtomicString& attr_value = element->FastGetAttribute(attribute);
  if (is_token_attr && IsUndefinedAttrValue(attr_value))
    return g_null_atom;  // Attribute not set or explicitly undefined.

  return attr_value;
}

// static
Element* AccessibleNode::GetPropertyOrARIAAttribute(
    Element* element,
    AOMRelationProperty property) {
  if (!element)
    return nullptr;

  if (AccessibleNode* result = GetProperty(element, property))
    return result->element();

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  AtomicString value = element->FastGetAttribute(attribute);
  return element->GetTreeScope().getElementById(value);
}

// static
bool AccessibleNode::GetPropertyOrARIAAttribute(
    Element* element,
    AOMRelationListProperty property,
    HeapVector<Member<Element>>& targets) {
  if (!element)
    return false;

  if (GetProperty(element, property, targets))
    return true;

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  String value = element->FastGetAttribute(attribute).GetString();
  if (value.IsEmpty() && property == AOMRelationListProperty::kLabeledBy)
    value = element->FastGetAttribute(aria_labeledbyAttr).GetString();
  if (value.IsEmpty())
    return false;

  value.SimplifyWhiteSpace();
  Vector<String> ids;
  value.Split(' ', ids);
  if (ids.IsEmpty())
    return false;

  TreeScope& scope = element->GetTreeScope();
  for (const auto& id : ids) {
    if (Element* id_element = scope.getElementById(AtomicString(id)))
      targets.push_back(id_element);
  }
  return true;
}

// static
bool AccessibleNode::GetPropertyOrARIAAttribute(Element* element,
                                                AOMBooleanProperty property,
                                                bool& is_null) {
  is_null = true;
  if (!element)
    return false;

  AccessibleNode* accessible_node = element->ExistingAccessibleNode();
  if (accessible_node) {
    bool result = accessible_node->GetProperty(property, is_null);
    if (!is_null)
      return result;
  }

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  AtomicString attr_value = element->FastGetAttribute(attribute);
  is_null = IsUndefinedAttrValue(attr_value);
  return !is_null && !EqualIgnoringASCIICase(attr_value, "false");
}

// static
float AccessibleNode::GetPropertyOrARIAAttribute(Element* element,
                                                 AOMFloatProperty property,
                                                 bool& is_null) {
  is_null = true;
  if (!element)
    return 0.0;

  float result = GetProperty(element, property, is_null);
  if (!is_null)
    return result;

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  AtomicString attr_value = element->FastGetAttribute(attribute);
  is_null = attr_value.IsNull();
  return attr_value.ToFloat();
}

// static
uint32_t AccessibleNode::GetPropertyOrARIAAttribute(Element* element,
                                                    AOMUIntProperty property,
                                                    bool& is_null) {
  is_null = true;
  if (!element)
    return 0;

  int32_t result = GetProperty(element, property, is_null);
  if (!is_null)
    return result;

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  AtomicString attr_value = element->FastGetAttribute(attribute);
  is_null = attr_value.IsNull();
  return attr_value.GetString().ToUInt();
}

// static
int32_t AccessibleNode::GetPropertyOrARIAAttribute(Element* element,
                                                   AOMIntProperty property,
                                                   bool& is_null) {
  is_null = true;
  if (!element)
    return 0;

  int32_t result = GetProperty(element, property, is_null);
  if (!is_null)
    return result;

  // Fall back on the equivalent ARIA attribute.
  QualifiedName attribute = GetCorrespondingARIAAttribute(property);
  AtomicString attr_value = element->FastGetAttribute(attribute);
  is_null = attr_value.IsNull();
  return attr_value.ToInt();
}

void AccessibleNode::GetAllAOMProperties(
    AOMPropertyClient* client,
    HashSet<QualifiedName>& shadowed_aria_attributes) {
  for (auto& item : string_properties_) {
    client->AddStringProperty(item.first, item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
  for (auto& item : boolean_properties_) {
    client->AddBooleanProperty(item.first, item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
  for (auto& item : float_properties_) {
    client->AddFloatProperty(item.first, item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
  for (auto& item : int_properties_) {
    client->AddIntProperty(item.first, item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
  for (auto& item : uint_properties_) {
    client->AddUIntProperty(item.first, item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
  for (auto& item : relation_properties_) {
    if (!item.second)
      continue;
    client->AddRelationProperty(item.first, *item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
  for (auto& item : relation_list_properties_) {
    if (!item.second)
      continue;
    client->AddRelationListProperty(item.first, *item.second);
    shadowed_aria_attributes.insert(GetCorrespondingARIAAttribute(item.first));
  }
}

AccessibleNode* AccessibleNode::activeDescendant() const {
  return GetProperty(element_, AOMRelationProperty::kActiveDescendant);
}

void AccessibleNode::setActiveDescendant(AccessibleNode* active_descendant) {
  SetRelationProperty(AOMRelationProperty::kActiveDescendant,
                      active_descendant);
  NotifyAttributeChanged(aria_activedescendantAttr);
}

bool AccessibleNode::atomic(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kAtomic, is_null);
}

void AccessibleNode::setAtomic(bool atomic, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kAtomic, atomic, is_null);
  NotifyAttributeChanged(aria_atomicAttr);
}

AtomicString AccessibleNode::autocomplete() const {
  return GetProperty(AOMStringProperty::kAutocomplete);
}

void AccessibleNode::setAutocomplete(const AtomicString& autocomplete) {
  SetStringProperty(AOMStringProperty::kAutocomplete, autocomplete);
  NotifyAttributeChanged(aria_autocompleteAttr);
}

bool AccessibleNode::busy(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kBusy, is_null);
}

void AccessibleNode::setBusy(bool busy, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kBusy, busy, is_null);
  NotifyAttributeChanged(aria_busyAttr);
}

AtomicString AccessibleNode::checked() const {
  return GetProperty(AOMStringProperty::kChecked);
}

void AccessibleNode::setChecked(const AtomicString& checked) {
  SetStringProperty(AOMStringProperty::kChecked, checked);
  NotifyAttributeChanged(aria_checkedAttr);
}

int32_t AccessibleNode::colCount(bool& is_null) const {
  return GetProperty(element_, AOMIntProperty::kColCount, is_null);
}

void AccessibleNode::setColCount(int32_t col_count, bool is_null) {
  SetIntProperty(AOMIntProperty::kColCount, col_count, is_null);
  NotifyAttributeChanged(aria_colcountAttr);
}

uint32_t AccessibleNode::colIndex(bool& is_null) const {
  return GetProperty(element_, AOMUIntProperty::kColIndex, is_null);
}

void AccessibleNode::setColIndex(uint32_t col_index, bool is_null) {
  SetUIntProperty(AOMUIntProperty::kColIndex, col_index, is_null);
  NotifyAttributeChanged(aria_colindexAttr);
}

uint32_t AccessibleNode::colSpan(bool& is_null) const {
  return GetProperty(element_, AOMUIntProperty::kColSpan, is_null);
}

void AccessibleNode::setColSpan(uint32_t col_span, bool is_null) {
  SetUIntProperty(AOMUIntProperty::kColSpan, col_span, is_null);
  NotifyAttributeChanged(aria_colspanAttr);
}

AccessibleNodeList* AccessibleNode::controls() const {
  return GetProperty(element_, AOMRelationListProperty::kControls);
}

void AccessibleNode::setControls(AccessibleNodeList* controls) {
  SetRelationListProperty(AOMRelationListProperty::kControls, controls);
  NotifyAttributeChanged(aria_controlsAttr);
}

AtomicString AccessibleNode::current() const {
  return GetProperty(AOMStringProperty::kCurrent);
}

void AccessibleNode::setCurrent(const AtomicString& current) {
  SetStringProperty(AOMStringProperty::kCurrent, current);

  if (AXObjectCache* cache = GetAXObjectCache())
    cache->HandleAttributeChanged(aria_currentAttr, element_);
}

AccessibleNodeList* AccessibleNode::describedBy() {
  return GetProperty(element_, AOMRelationListProperty::kDescribedBy);
}

void AccessibleNode::setDescribedBy(AccessibleNodeList* described_by) {
  SetRelationListProperty(AOMRelationListProperty::kDescribedBy, described_by);
  NotifyAttributeChanged(aria_describedbyAttr);
}

AccessibleNode* AccessibleNode::details() const {
  return GetProperty(element_, AOMRelationProperty::kDetails);
}

void AccessibleNode::setDetails(AccessibleNode* details) {
  SetRelationProperty(AOMRelationProperty::kDetails, details);
  NotifyAttributeChanged(aria_detailsAttr);
}

bool AccessibleNode::disabled(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kDisabled, is_null);
}

void AccessibleNode::setDisabled(bool disabled, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kDisabled, disabled, is_null);
  NotifyAttributeChanged(aria_disabledAttr);
}

AccessibleNode* AccessibleNode::errorMessage() const {
  return GetProperty(element_, AOMRelationProperty::kErrorMessage);
}

void AccessibleNode::setErrorMessage(AccessibleNode* error_message) {
  SetRelationProperty(AOMRelationProperty::kErrorMessage, error_message);
  NotifyAttributeChanged(aria_errormessageAttr);
}

bool AccessibleNode::expanded(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kExpanded, is_null);
}

void AccessibleNode::setExpanded(bool expanded, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kExpanded, expanded, is_null);
  NotifyAttributeChanged(aria_expandedAttr);
}

AccessibleNodeList* AccessibleNode::flowTo() const {
  return GetProperty(element_, AOMRelationListProperty::kFlowTo);
}

void AccessibleNode::setFlowTo(AccessibleNodeList* flow_to) {
  SetRelationListProperty(AOMRelationListProperty::kFlowTo, flow_to);
  NotifyAttributeChanged(aria_flowtoAttr);
}

AtomicString AccessibleNode::hasPopUp() const {
  return GetProperty(AOMStringProperty::kHasPopUp);
}

void AccessibleNode::setHasPopUp(const AtomicString& has_popup) {
  SetStringProperty(AOMStringProperty::kHasPopUp, has_popup);
  NotifyAttributeChanged(aria_haspopupAttr);
}

bool AccessibleNode::hidden(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kHidden, is_null);
}

void AccessibleNode::setHidden(bool hidden, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kHidden, hidden, is_null);
  NotifyAttributeChanged(aria_hiddenAttr);
}

AtomicString AccessibleNode::invalid() const {
  return GetProperty(AOMStringProperty::kInvalid);
}

void AccessibleNode::setInvalid(const AtomicString& invalid) {
  SetStringProperty(AOMStringProperty::kInvalid, invalid);
  NotifyAttributeChanged(aria_invalidAttr);
}

AtomicString AccessibleNode::keyShortcuts() const {
  return GetProperty(AOMStringProperty::kKeyShortcuts);
}

void AccessibleNode::setKeyShortcuts(const AtomicString& key_shortcuts) {
  SetStringProperty(AOMStringProperty::kKeyShortcuts, key_shortcuts);
  NotifyAttributeChanged(aria_keyshortcutsAttr);
}

AtomicString AccessibleNode::label() const {
  return GetProperty(AOMStringProperty::kLabel);
}

void AccessibleNode::setLabel(const AtomicString& label) {
  SetStringProperty(AOMStringProperty::kLabel, label);
  NotifyAttributeChanged(aria_labelAttr);
}

AccessibleNodeList* AccessibleNode::labeledBy() {
  return GetProperty(element_, AOMRelationListProperty::kLabeledBy);
}

void AccessibleNode::setLabeledBy(AccessibleNodeList* labeled_by) {
  SetRelationListProperty(AOMRelationListProperty::kLabeledBy, labeled_by);
  NotifyAttributeChanged(aria_labelledbyAttr);
}

uint32_t AccessibleNode::level(bool& is_null) const {
  return GetProperty(element_, AOMUIntProperty::kLevel, is_null);
}

void AccessibleNode::setLevel(uint32_t level, bool is_null) {
  SetUIntProperty(AOMUIntProperty::kLevel, level, is_null);
  NotifyAttributeChanged(aria_levelAttr);
}

AtomicString AccessibleNode::live() const {
  return GetProperty(AOMStringProperty::kLive);
}

void AccessibleNode::setLive(const AtomicString& live) {
  SetStringProperty(AOMStringProperty::kLive, live);
  NotifyAttributeChanged(aria_liveAttr);
}

bool AccessibleNode::modal(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kModal, is_null);
}

void AccessibleNode::setModal(bool modal, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kModal, modal, is_null);
  NotifyAttributeChanged(aria_modalAttr);
}

bool AccessibleNode::multiline(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kMultiline, is_null);
}

void AccessibleNode::setMultiline(bool multiline, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kMultiline, multiline, is_null);
  NotifyAttributeChanged(aria_multilineAttr);
}

bool AccessibleNode::multiselectable(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kMultiselectable, is_null);
}

void AccessibleNode::setMultiselectable(bool multiselectable, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kMultiselectable, multiselectable,
                     is_null);
  NotifyAttributeChanged(aria_multiselectableAttr);
}

AtomicString AccessibleNode::orientation() const {
  return GetProperty(AOMStringProperty::kOrientation);
}

void AccessibleNode::setOrientation(const AtomicString& orientation) {
  SetStringProperty(AOMStringProperty::kOrientation, orientation);
  NotifyAttributeChanged(aria_orientationAttr);
}

AccessibleNodeList* AccessibleNode::owns() const {
  return GetProperty(element_, AOMRelationListProperty::kOwns);
}

void AccessibleNode::setOwns(AccessibleNodeList* owns) {
  SetRelationListProperty(AOMRelationListProperty::kOwns, owns);
  NotifyAttributeChanged(aria_ownsAttr);
}

AtomicString AccessibleNode::placeholder() const {
  return GetProperty(AOMStringProperty::kPlaceholder);
}

void AccessibleNode::setPlaceholder(const AtomicString& placeholder) {
  SetStringProperty(AOMStringProperty::kPlaceholder, placeholder);
  NotifyAttributeChanged(aria_placeholderAttr);
}

uint32_t AccessibleNode::posInSet(bool& is_null) const {
  return GetProperty(element_, AOMUIntProperty::kPosInSet, is_null);
}

void AccessibleNode::setPosInSet(uint32_t pos_in_set, bool is_null) {
  SetUIntProperty(AOMUIntProperty::kPosInSet, pos_in_set, is_null);
  NotifyAttributeChanged(aria_posinsetAttr);
}

AtomicString AccessibleNode::pressed() const {
  return GetProperty(AOMStringProperty::kPressed);
}

void AccessibleNode::setPressed(const AtomicString& pressed) {
  SetStringProperty(AOMStringProperty::kPressed, pressed);
  NotifyAttributeChanged(aria_pressedAttr);
}

bool AccessibleNode::readOnly(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kReadOnly, is_null);
}

void AccessibleNode::setReadOnly(bool read_only, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kReadOnly, read_only, is_null);
  NotifyAttributeChanged(aria_readonlyAttr);
}

AtomicString AccessibleNode::relevant() const {
  return GetProperty(AOMStringProperty::kRelevant);
}

void AccessibleNode::setRelevant(const AtomicString& relevant) {
  SetStringProperty(AOMStringProperty::kRelevant, relevant);
  NotifyAttributeChanged(aria_relevantAttr);
}

bool AccessibleNode::required(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kRequired, is_null);
}

void AccessibleNode::setRequired(bool required, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kRequired, required, is_null);
  NotifyAttributeChanged(aria_requiredAttr);
}

AtomicString AccessibleNode::role() const {
  return GetProperty(AOMStringProperty::kRole);
}

void AccessibleNode::setRole(const AtomicString& role) {
  SetStringProperty(AOMStringProperty::kRole, role);
  NotifyAttributeChanged(roleAttr);
}

AtomicString AccessibleNode::roleDescription() const {
  return GetProperty(AOMStringProperty::kRoleDescription);
}

void AccessibleNode::setRoleDescription(const AtomicString& role_description) {
  SetStringProperty(AOMStringProperty::kRoleDescription, role_description);
  NotifyAttributeChanged(aria_roledescriptionAttr);
}

int32_t AccessibleNode::rowCount(bool& is_null) const {
  return GetProperty(element_, AOMIntProperty::kRowCount, is_null);
}

void AccessibleNode::setRowCount(int32_t row_count, bool is_null) {
  SetIntProperty(AOMIntProperty::kRowCount, row_count, is_null);
  NotifyAttributeChanged(aria_rowcountAttr);
}

uint32_t AccessibleNode::rowIndex(bool& is_null) const {
  return GetProperty(element_, AOMUIntProperty::kRowIndex, is_null);
}

void AccessibleNode::setRowIndex(uint32_t row_index, bool is_null) {
  SetUIntProperty(AOMUIntProperty::kRowIndex, row_index, is_null);
  NotifyAttributeChanged(aria_rowindexAttr);
}

uint32_t AccessibleNode::rowSpan(bool& is_null) const {
  return GetProperty(element_, AOMUIntProperty::kRowSpan, is_null);
}

void AccessibleNode::setRowSpan(uint32_t row_span, bool is_null) {
  SetUIntProperty(AOMUIntProperty::kRowSpan, row_span, is_null);
  NotifyAttributeChanged(aria_rowspanAttr);
}

bool AccessibleNode::selected(bool& is_null) const {
  return GetProperty(AOMBooleanProperty::kSelected, is_null);
}

void AccessibleNode::setSelected(bool selected, bool is_null) {
  SetBooleanProperty(AOMBooleanProperty::kSelected, selected, is_null);
  NotifyAttributeChanged(aria_selectedAttr);
}

int32_t AccessibleNode::setSize(bool& is_null) const {
  return GetProperty(element_, AOMIntProperty::kSetSize, is_null);
}

void AccessibleNode::setSetSize(int32_t set_size, bool is_null) {
  SetIntProperty(AOMIntProperty::kSetSize, set_size, is_null);
  NotifyAttributeChanged(aria_setsizeAttr);
}

AtomicString AccessibleNode::sort() const {
  return GetProperty(AOMStringProperty::kSort);
}

void AccessibleNode::setSort(const AtomicString& sort) {
  SetStringProperty(AOMStringProperty::kSort, sort);
  NotifyAttributeChanged(aria_sortAttr);
}

float AccessibleNode::valueMax(bool& is_null) const {
  return GetProperty(element_, AOMFloatProperty::kValueMax, is_null);
}

void AccessibleNode::setValueMax(float value_max, bool is_null) {
  SetFloatProperty(AOMFloatProperty::kValueMax, value_max, is_null);
  NotifyAttributeChanged(aria_valuemaxAttr);
}

float AccessibleNode::valueMin(bool& is_null) const {
  return GetProperty(element_, AOMFloatProperty::kValueMin, is_null);
}

void AccessibleNode::setValueMin(float value_min, bool is_null) {
  SetFloatProperty(AOMFloatProperty::kValueMin, value_min, is_null);
  NotifyAttributeChanged(aria_valueminAttr);
}

float AccessibleNode::valueNow(bool& is_null) const {
  return GetProperty(element_, AOMFloatProperty::kValueNow, is_null);
}

void AccessibleNode::setValueNow(float value_now, bool is_null) {
  SetFloatProperty(AOMFloatProperty::kValueNow, value_now, is_null);
  NotifyAttributeChanged(aria_valuenowAttr);
}

AtomicString AccessibleNode::valueText() const {
  return GetProperty(AOMStringProperty::kValueText);
}

void AccessibleNode::setValueText(const AtomicString& value_text) {
  SetStringProperty(AOMStringProperty::kValueText, value_text);
  NotifyAttributeChanged(aria_valuetextAttr);
}

AccessibleNodeList* AccessibleNode::childNodes() {
  return AccessibleNodeList::Create(children_);
}

void AccessibleNode::appendChild(AccessibleNode* child,
                                 ExceptionState& exception_state) {
  if (child->element()) {
    exception_state.ThrowDOMException(
        kInvalidAccessError,
        "An AccessibleNode associated with an Element cannot be a child.");
    return;
  }

  if (child->parent_) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Reparenting is not supported yet.");
    return;
  }
  child->parent_ = this;

  if (!GetDocument()->GetSecurityOrigin()->CanAccess(
          child->GetDocument()->GetSecurityOrigin())) {
    exception_state.ThrowDOMException(
        kInvalidAccessError,
        "Trying to access an AccessibleNode from a different origin.");
    return;
  }

  children_.push_back(child);
  if (AXObjectCache* cache = GetAXObjectCache())
    cache->ChildrenChanged(this);
}

void AccessibleNode::removeChild(AccessibleNode* old_child,
                                 ExceptionState& exception_state) {
  if (old_child->parent_ != this) {
    exception_state.ThrowDOMException(
        kInvalidAccessError, "Node to remove is not a child of this node.");
    return;
  }
  auto* ix = std::find_if(children_.begin(), children_.end(),
                          [old_child](const Member<AccessibleNode> child) {
                            return child.Get() == old_child;
                          });
  if (ix == children_.end()) {
    exception_state.ThrowDOMException(
        kInvalidAccessError, "Node to remove is not a child of this node.");
    return;
  }
  old_child->parent_ = nullptr;
  children_.erase(ix);

  if (AXObjectCache* cache = GetAXObjectCache())
    cache->ChildrenChanged(this);
}

// These properties support a list of tokens, and "undefined"/"" is
// equivalent to not setting the attribute.
bool AccessibleNode::IsStringTokenProperty(AOMStringProperty property) {
  switch (property) {
    case AOMStringProperty::kAutocomplete:
    case AOMStringProperty::kChecked:
    case AOMStringProperty::kCurrent:
    case AOMStringProperty::kHasPopUp:
    case AOMStringProperty::kInvalid:
    case AOMStringProperty::kLive:
    case AOMStringProperty::kOrientation:
    case AOMStringProperty::kPressed:
    case AOMStringProperty::kRelevant:
    case AOMStringProperty::kSort:
      return true;
    case AOMStringProperty::kKeyShortcuts:
    case AOMStringProperty::kLabel:
    case AOMStringProperty::kPlaceholder:
    case AOMStringProperty::kRole:  // Is token, but ""/"undefined" not
                                    // supported.
    case AOMStringProperty::kRoleDescription:
    case AOMStringProperty::kValueText:
      break;
  }
  return false;
}

const AtomicString& AccessibleNode::InterfaceName() const {
  return EventTargetNames::AccessibleNode;
}

ExecutionContext* AccessibleNode::GetExecutionContext() const {
  if (element_)
    return element_->GetExecutionContext();

  if (parent_)
    return parent_->GetExecutionContext();

  return nullptr;
}

void AccessibleNode::SetStringProperty(AOMStringProperty property,
                                       const AtomicString& value) {
  for (auto& item : string_properties_) {
    if (item.first == property) {
      item.second = value;
      return;
    }
  }

  string_properties_.push_back(std::make_pair(property, value));
}

void AccessibleNode::SetRelationProperty(AOMRelationProperty property,
                                         AccessibleNode* value) {
  for (auto& item : relation_properties_) {
    if (item.first == property) {
      item.second = value;
      return;
    }
  }

  relation_properties_.push_back(std::make_pair(property, value));
}

void AccessibleNode::SetRelationListProperty(AOMRelationListProperty property,
                                             AccessibleNodeList* value) {
  for (auto& item : relation_list_properties_) {
    if (item.first == property) {
      if (item.second)
        item.second->RemoveOwner(property, this);
      if (value)
        value->AddOwner(property, this);
      item.second = value;
      return;
    }
  }

  relation_list_properties_.push_back(std::make_pair(property, value));
}

template <typename P, typename T>
static void SetProperty(P property,
                        T value,
                        bool is_null,
                        Vector<std::pair<P, T>>& properties) {
  for (size_t i = 0; i < properties.size(); i++) {
    auto& item = properties[i];
    if (item.first == property) {
      if (is_null)
        properties.EraseAt(i);
      else
        item.second = value;
      return;
    }
  }

  properties.push_back(std::make_pair(property, value));
}

void AccessibleNode::SetBooleanProperty(AOMBooleanProperty property,
                                        bool value,
                                        bool is_null) {
  SetProperty(property, value, is_null, boolean_properties_);
}

void AccessibleNode::SetIntProperty(AOMIntProperty property,
                                    int32_t value,
                                    bool is_null) {
  SetProperty(property, value, is_null, int_properties_);
}

void AccessibleNode::SetUIntProperty(AOMUIntProperty property,
                                     uint32_t value,
                                     bool is_null) {
  SetProperty(property, value, is_null, uint_properties_);
}

void AccessibleNode::SetFloatProperty(AOMFloatProperty property,
                                      float value,
                                      bool is_null) {
  SetProperty(property, value, is_null, float_properties_);
}

void AccessibleNode::OnRelationListChanged(AOMRelationListProperty property) {
  NotifyAttributeChanged(GetCorrespondingARIAAttribute(property));
}

void AccessibleNode::NotifyAttributeChanged(
    const blink::QualifiedName& attribute) {
  // TODO(dmazzoni): Make a cleaner API for this rather than pretending
  // the DOM attribute changed.
  if (AXObjectCache* cache = GetAXObjectCache())
    cache->HandleAttributeChanged(attribute, element_);
}

AXObjectCache* AccessibleNode::GetAXObjectCache() {
  return GetDocument()->ExistingAXObjectCache();
}

void AccessibleNode::Trace(blink::Visitor* visitor) {
  visitor->Trace(element_);
  visitor->Trace(document_);
  visitor->Trace(relation_properties_);
  visitor->Trace(relation_list_properties_);
  visitor->Trace(children_);
  visitor->Trace(parent_);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
