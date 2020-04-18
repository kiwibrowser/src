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

package com.android.i18n.addressinput.autocomplete.gmscore;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.location.Location;
import android.test.ActivityInstrumentationTestCase2;
import com.android.i18n.addressinput.testing.TestActivity;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.PendingResults;
import com.google.android.gms.location.FusedLocationProviderApi;
import com.google.android.gms.location.places.AutocompleteFilter;
import com.google.android.gms.location.places.AutocompletePrediction;
import com.google.android.gms.location.places.AutocompletePredictionBuffer;
import com.google.android.gms.location.places.GeoDataApi;
import com.google.android.gms.maps.model.LatLngBounds;
import com.google.common.collect.Lists;
import com.google.common.util.concurrent.FutureCallback;
import com.google.common.util.concurrent.SettableFuture;
import com.google.i18n.addressinput.common.AddressAutocompleteApi;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

/** Unit tests for {@link AddressAutocompleteApi}. */
public class AddressAutocompleteApiImplTest extends ActivityInstrumentationTestCase2<TestActivity> {
  private static final String TAG = "AddrAutoApiTest";
  private static final String TEST_QUERY = "TEST_QUERY";

  private AddressAutocompleteApi addressAutocompleteApi;

  // Mock services
  private GeoDataApi geoDataApi = mock(GeoDataApi.class);
  private GoogleApiClient googleApiClient = mock(GoogleApiClient.class);
  private FusedLocationProviderApi locationApi = mock(FusedLocationProviderApi.class);

  // Mock data
  private AutocompletePredictionBuffer autocompleteResults =
      mock(AutocompletePredictionBuffer.class);
  private PendingResult<AutocompletePredictionBuffer> autocompletePendingResults =
      PendingResults.immediatePendingResult(autocompleteResults);
  private AutocompletePrediction autocompletePrediction = mock(AutocompletePrediction.class);

  public AddressAutocompleteApiImplTest() {
    super(TestActivity.class);
  }

  @Override
  protected void setUp() {
    addressAutocompleteApi =
        new AddressAutocompleteApiImpl(googleApiClient, geoDataApi, locationApi);
  }

  // Tests for the AddressAutocompleteApi

  public void testAddressAutocompleteApi() throws InterruptedException, ExecutionException {
    when(googleApiClient.isConnected()).thenReturn(true);
    when(locationApi.getLastLocation(googleApiClient)).thenReturn(new Location("TEST_PROVIDER"));

    Future<List<? extends AddressAutocompletePrediction>> actualPredictions =
        getAutocompleteSuggestions();

    List<AddressAutocompletePrediction> expectedPredictions =
        Lists.newArrayList(new AddressAutocompletePredictionImpl(autocompletePrediction));

    assertEquals(actualPredictions.get(), expectedPredictions);
  }

  public void testAddressAutocompleteApi_deviceLocationMissing()
      throws InterruptedException, ExecutionException {
    when(googleApiClient.isConnected()).thenReturn(true);
    when(locationApi.getLastLocation(googleApiClient)).thenReturn(null);

    Future<List<? extends AddressAutocompletePrediction>> actualPredictions =
        getAutocompleteSuggestions();

    List<AddressAutocompletePrediction> expectedPredictions =
        Lists.newArrayList(new AddressAutocompletePredictionImpl(autocompletePrediction));

    assertEquals(actualPredictions.get(), expectedPredictions);
  }

  public void testAddressAutocompleteApi_isConfiguredCorrectly() {
    when(googleApiClient.isConnected()).thenReturn(true);
    assertTrue(addressAutocompleteApi.isConfiguredCorrectly());
  }

  // Helper functions

  private Future<List<? extends AddressAutocompletePrediction>> getAutocompleteSuggestions() {
    // Set up the AddressData to be returned from the PlaceAutocomplete API + PlaceDetailsApi.
    // Most of the objects that are mocked here are not services, but simply data without any
    // public constructors.
    when(geoDataApi.getAutocompletePredictions(
            eq(googleApiClient),
            eq(TEST_QUERY),
            any(LatLngBounds.class),
            any(AutocompleteFilter.class)))
        .thenReturn(autocompletePendingResults);
    when(autocompletePrediction.getPlaceId()).thenReturn("TEST_PLACE_ID");
    when(autocompletePrediction.getFullText(null)).thenReturn("TEST_PREDICTION");
    when(autocompleteResults.iterator())
        .thenReturn(Arrays.asList(autocompletePrediction).iterator());
    when(autocompletePrediction.getPlaceId()).thenReturn("TEST_PLACE_ID");
    when(autocompletePrediction.getPrimaryText(null)).thenReturn("TEST_PRIMARY_ID");
    when(autocompletePrediction.getSecondaryText(null)).thenReturn("TEST_SECONDARY_ID");

    SettableFuture<List<? extends AddressAutocompletePrediction>> actualPredictions =
        SettableFuture.create();
    addressAutocompleteApi.getAutocompletePredictions(
        TEST_QUERY,
        new FutureCallback<List<? extends AddressAutocompletePrediction>>() {
          @Override
          public void onSuccess(List<? extends AddressAutocompletePrediction> predictions) {
            actualPredictions.set(predictions);
          }

          @Override
          public void onFailure(Throwable error) {
            assertTrue("Error getting autocomplete predictions: " + error.toString(), false);
          }
        });

    return actualPredictions;
  }
}
