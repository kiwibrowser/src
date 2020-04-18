/*
 * Copyright (C) 2016 Google Inc.
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

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.database.DataSetObserver;
import android.test.ActivityInstrumentationTestCase2;
import android.widget.AutoCompleteTextView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import com.android.i18n.addressinput.AddressAutocompleteController.AddressAdapter;
import com.android.i18n.addressinput.AddressAutocompleteController.AddressPrediction;
import com.android.i18n.addressinput.testing.TestActivity;
import com.google.common.collect.Lists;
import com.google.common.util.concurrent.FutureCallback;
import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.SettableFuture;
import com.google.i18n.addressinput.common.AddressAutocompleteApi;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;
import com.google.i18n.addressinput.common.AddressData;
import com.google.i18n.addressinput.common.OnAddressSelectedListener;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/** Unit tests for {@link AddressAutocompleteController}. */
public class AddressAutocompleteControllerTest
    extends ActivityInstrumentationTestCase2<TestActivity> {
  private static final String TEST_QUERY = "TEST_QUERY";

  private Context context;

  private AddressAutocompleteController controller;
  private AutoCompleteTextView textView;

  // Mock services
  private @Mock AddressAutocompleteApi autocompleteApi;
  private @Mock PlaceDetailsApi placeDetailsApi;

  // Mock data
  private @Captor ArgumentCaptor<FutureCallback<List<? extends AddressAutocompletePrediction>>>
      autocompleteCallback;
  private @Mock AddressAutocompletePrediction autocompletePrediction;

  public AddressAutocompleteControllerTest() {
    super(TestActivity.class);
  }

  @Override
  protected void setUp() {
    MockitoAnnotations.initMocks(this);

    context = getActivity();

    textView = new AutoCompleteTextView(context);
    controller =
        new AddressAutocompleteController(context, autocompleteApi, placeDetailsApi)
            .setView(textView);
  }

  // Tests for the AddressAutocompleteController

  public void testAddressAutocompleteController() throws InterruptedException, ExecutionException {
    final AddressData expectedAddress = AddressData.builder()
        .addAddressLine("1600 Amphitheatre Parkway")
        .setLocality("Mountain View")
        .setAdminArea("California")
        .setCountry("US")
        .build();

    Future<AddressData> actualAddress = getAutocompletePredictions(expectedAddress);

    assertEquals(1, textView.getAdapter().getCount());
    assertEquals(actualAddress.get(), expectedAddress);
  }

  // Tests for the AddressAdapter

  public void testAddressAdapter_getItem() {
    AddressAdapter adapter = new AddressAdapter(context);
    List<AddressPrediction> predictions =
        Lists.newArrayList(new AddressPrediction(TEST_QUERY, autocompletePrediction));

    adapter.refresh(predictions);
    assertEquals(adapter.getCount(), predictions.size());
    for (int i = 0; i < predictions.size(); i++) {
      assertEquals("Item #" + i, predictions.get(0), adapter.getItem(0));
    }
  }

  public void testAddressAdapter_getView() {
    AddressAdapter adapter = new AddressAdapter(context);
    List<AddressPrediction> predictions =
        Lists.newArrayList(new AddressPrediction(TEST_QUERY, autocompletePrediction));

    adapter.refresh(predictions);
    for (int i = 0; i < predictions.size(); i++) {
      assertNotNull("Item #" + i, adapter.getView(0, null, new LinearLayout(context)));
    }
  }

  // Helper functions

  private Future<AddressData> getAutocompletePredictions(AddressData expectedAddress) {
    // Set up the AddressData to be returned from the AddressAutocompleteApi and PlaceDetailsApi.
    when(autocompleteApi.isConfiguredCorrectly()).thenReturn(true);
    when(autocompletePrediction.getPlaceId()).thenReturn("TEST_PLACE_ID");
    when(placeDetailsApi.getAddressData(autocompletePrediction))
        .thenReturn(Futures.immediateFuture(expectedAddress));

    // Perform a click on the first autocomplete suggestion once it is loaded.
    textView
        .getAdapter()
        .registerDataSetObserver(
            new DataSetObserver() {
              @Override
              public void onInvalidated() {}

              @Override
              public void onChanged() {
                // For some reason, performing a click on the view or dropdown view associated with
                // the first item in the list doesn't trigger the onItemClick listener in tests, so
                // we trigger it manually here.
                textView
                    .getOnItemClickListener()
                    .onItemClick(new ListView(context), new TextView(context), 0, 0);
              }
            });

    // The OnAddressSelectedListener is the way for the AddressWidget to consume the AddressData
    // produced by autocompletion.
    final SettableFuture<AddressData> result = SettableFuture.create();
    controller.setOnAddressSelectedListener(
        new OnAddressSelectedListener() {
          @Override
          public void onAddressSelected(AddressData address) {
            result.set(address);
          }
        });

    // Actually trigger the behaviors mocked above.
    textView.setText(TEST_QUERY);

    verify(autocompleteApi)
        .getAutocompletePredictions(any(String.class), autocompleteCallback.capture());
    autocompleteCallback.getValue().onSuccess(Lists.newArrayList(autocompletePrediction));

    return result;
  }
}
