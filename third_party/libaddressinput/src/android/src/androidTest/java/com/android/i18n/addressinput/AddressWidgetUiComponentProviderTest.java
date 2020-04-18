/*
 * Copyright (C) 2014 Google Inc.
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

import com.google.i18n.addressinput.common.AddressData;
import com.google.i18n.addressinput.common.AddressField;
import com.google.i18n.addressinput.common.FormOptions;
import com.google.i18n.addressinput.common.SimpleClientCacheManager;

import android.app.ProgressDialog;
import android.content.Context;
import android.test.ActivityInstrumentationTestCase2;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import com.android.i18n.addressinput.testing.TestActivity;

/**
 * Test class for {@link AddressWidgetUiComponentProvider}.
 */
public class AddressWidgetUiComponentProviderTest
    extends ActivityInstrumentationTestCase2<TestActivity> {
  private AddressWidget widget;
  private AddressWidgetUiComponentProvider componentProvider;
  private LinearLayout container;
  private AddressData address;
  private Context context;
  private int customTextViewCounter;
  private int customProgressDialogCounter;

  public AddressWidgetUiComponentProviderTest() {
    super(TestActivity.class);
  }

  @Override
  protected void setUp() throws Exception {
    super.setUp();
    AddressData.Builder builder = new AddressData.Builder()
        .setCountry("US")
        .setLanguageCode("en")
        .setAddress("1098 Alta Ave")
        .setAdminArea("CA");
    address = builder.build();
    context = getActivity();
    container = new LinearLayout(context);
  }

  public void testCustomWidgets() {
    customTextViewCounter = 0;
    customProgressDialogCounter = 0;
    componentProvider = new TestComponentProvider(context);
    widget = new AddressWidget(context, container, new FormOptions(),
        new SimpleClientCacheManager(), componentProvider);
    widget.renderFormWithSavedAddress(address);

    for (AddressField field : AddressField.values()) {
      if (field.equals(AddressField.COUNTRY)) {
        continue;
      }

      View view = widget.getViewForField(field);
      if (view instanceof EditText) {
        assertTrue("Field " + field + " does not use customized edit text widget.",
            view instanceof CustomEditText);
      } else if (view instanceof Spinner) {
        assertTrue("Field " + field + " does not use customized spinner widget.",
            view instanceof CustomSpinner);
        assertTrue("Field " + field + " does not use customized ArrayAdapter.",
            ((Spinner) view).getAdapter() instanceof CustomArrayAdapter);
      }
    }

    assertTrue("Custom TextView label not used.", customTextViewCounter > 0);
    assertTrue("Custom ProgressDialog not used.", customProgressDialogCounter > 0);
  }

  private void increaseTextViewCounter() {
    customTextViewCounter++;
  }

  private void increaseProgressDialogCounter() {
    customProgressDialogCounter++;
  }

  private class CustomEditText extends EditText {
    CustomEditText(Context context) {
      super(context);
    }
  }

  private class CustomSpinner extends Spinner {
    CustomSpinner(Context context) {
      super(context);
    }
  }

  private class CustomArrayAdapter<String> extends ArrayAdapter<String> {
    CustomArrayAdapter(Context context, int id) {
      super(context, id);
    }
  }

  private class TestComponentProvider extends AddressWidgetUiComponentProvider {
    TestComponentProvider(Context context) {
      super(context);
    }

    @Override
    protected TextView createUiLabel(CharSequence label, AddressField.WidthType widthType) {
      TextView result = new TextView(context);
      result.setText(label);
      AddressWidgetUiComponentProviderTest.this.increaseTextViewCounter();
      return result;
    }

    @Override
    protected EditText createUiTextField(AddressField.WidthType widthType) {
      return new CustomEditText(context);
    }

    @Override
    protected Spinner createUiPickerSpinner(AddressField.WidthType widthType) {
      return new CustomSpinner(context);
    }

    @Override
    protected ArrayAdapter<String> createUiPickerAdapter(AddressField.WidthType widthType) {
      ArrayAdapter<String> result = new CustomArrayAdapter<String>(
          context, android.R.layout.simple_spinner_item);
      result.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
      return result;
    }

    @Override
    protected ProgressDialog getUiActivityIndicatorView() {
      AddressWidgetUiComponentProviderTest.this.increaseProgressDialogCounter();
      return super.getUiActivityIndicatorView();
    }
  }
}
