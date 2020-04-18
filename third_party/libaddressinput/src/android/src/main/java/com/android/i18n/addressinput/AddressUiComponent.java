/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.i18n.addressinput;

import com.google.i18n.addressinput.common.AddressField;
import com.google.i18n.addressinput.common.RegionData;

import android.view.View;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;
import android.widget.Spinner;

import java.util.ArrayList;
import java.util.List;

/**
 * Represents a component in the address widget UI. It could be either a text box (when there is no
 * candidate) or a spinner.
 */
class AddressUiComponent {
  // The label for the UI component
  private String fieldName;

  // The type of the UI component
  private UiComponent uiType;

  // The list of elements in the UI component
  private List<RegionData> candidatesList = new ArrayList<RegionData>();

  // The id of this UI component
  private AddressField id;

  // The id of the parent UI component. When the parent UI component is updated, this UI
  // component should be updated.
  private AddressField parentId;

  // The View representing the UI component
  private View view;

  /**
   * Type of UI component. There are only EDIT (text-box) and SPINNER (drop-down) components.
   */
  enum UiComponent {
    EDIT, SPINNER,
  }

  AddressUiComponent(AddressField id) {
    this.id = id;
    // By default, an AddressUiComponent doesn't depend on anything else.
    this.parentId = null;
    this.uiType = UiComponent.EDIT;
  }

  /**
   * Initializes the candidatesList, and set the uiType and parentId.
   * @param candidatesList
   */
  void initializeCandidatesList(List<RegionData> candidatesList) {
    this.candidatesList = candidatesList;
    if (candidatesList.size() > 1) {
      uiType = UiComponent.SPINNER;
      switch (id) {
        case DEPENDENT_LOCALITY:
          parentId = AddressField.LOCALITY;
          break;
        case LOCALITY:
          parentId = AddressField.ADMIN_AREA;
          break;
        case ADMIN_AREA:
          parentId = AddressField.COUNTRY;
          break;
        default:
          // Ignore.
      }
    }
  }

  /**
   * Gets the value entered in the UI component.
   */
  String getValue() {
    if (view == null) {
      return (candidatesList.size() == 0) ? "" : candidatesList.get(0).getDisplayName();
    }
    switch (uiType) {
      case SPINNER:
        Object selectedItem = ((Spinner) view).getSelectedItem();
        if (selectedItem == null) {
          return "";
        }
        return selectedItem.toString();
      case EDIT:
        return ((EditText) view).getText().toString();
      default:
        return "";
    }
  }

  /**
   * Sets the value displayed in the input field.
   */
  void setValue(String value) {
    if (view == null) {
      return;
    }

    switch(uiType) {
      case SPINNER:
        for (int i = 0; i < candidatesList.size(); i++) {
          // Assumes that the indices in the candidate list are the same as those used in the
          // Adapter backing the Spinner.
          if (candidatesList.get(i).getKey().equals(value)) {
            ((Spinner) view).setSelection(i);
          }
        }
        return;
      case EDIT:
        if (view instanceof AutoCompleteTextView) {
          // Prevent the AutoCompleteTextView from showing the dropdown.
          ((AutoCompleteTextView) view).setText(value, false);
        } else {
          ((EditText) view).setText(value);
        }
        return;
      default:
        return;
    }
  }

  String getFieldName() {
    return fieldName;
  }

  void setFieldName(String fieldName) {
    this.fieldName = fieldName;
  }

  UiComponent getUiType() {
    return uiType;
  }

  void setUiType(UiComponent uiType) {
    this.uiType = uiType;
  }

  List<RegionData> getCandidatesList() {
    return candidatesList;
  }

  void setCandidatesList(List<RegionData> candidatesList) {
    this.candidatesList = candidatesList;
  }

  AddressField getId() {
    return id;
  }

  void setId(AddressField id) {
    this.id = id;
  }

  AddressField getParentId() {
    return parentId;
  }

  void setParentId(AddressField parentId) {
    this.parentId = parentId;
  }

  void setView(View view) {
    this.view = view;
  }

  View getView() {
    return view;
  }
}
