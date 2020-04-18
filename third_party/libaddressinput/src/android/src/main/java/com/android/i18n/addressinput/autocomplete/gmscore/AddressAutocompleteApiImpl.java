package com.android.i18n.addressinput.autocomplete.gmscore;

import android.location.Location;
import android.util.Log;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.location.FusedLocationProviderApi;
import com.google.android.gms.location.places.AutocompleteFilter;
import com.google.android.gms.location.places.AutocompletePrediction;
import com.google.android.gms.location.places.AutocompletePredictionBuffer;
import com.google.android.gms.location.places.GeoDataApi;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;
import com.google.common.util.concurrent.FutureCallback;
import com.google.i18n.addressinput.common.AddressAutocompleteApi;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;
import java.util.ArrayList;
import java.util.List;

/**
 * GMSCore implementation of {@link com.google.i18n.addressinput.common.AddressAutocompleteApi}.
 *
 * <p>Internal clients should use the gcoreclient implementation instead.
 *
 * Callers should provide a GoogleApiClient with the Places.GEO_DATA_API and
 * LocationServices.API enabled. The GoogleApiClient should be connected before
 * it is passed to AddressWidget#enableAutocomplete. The caller will also need to request the
 * following permissions in their AndroidManifest.xml:
 *
 * <pre>
 *   <uses-permission android:name="com.google.android.providers.gsf.permission.READ_GSERVICES"/>
 *   <uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
 * </pre>
 *
 * Callers should check that the required permissions are actually present.
 * TODO(b/32559817): Handle permission check in libaddressinput so that callers don't need to.
 */
public class AddressAutocompleteApiImpl implements AddressAutocompleteApi {

  private static final String TAG = "GmsCoreAddrAutocmplt";
  private GoogleApiClient googleApiClient;

  // Use Places.GeoDataApi.
  private GeoDataApi geoDataApi;

  // Use LocationServices.FusedLocationApi.
  private FusedLocationProviderApi locationApi;

  public AddressAutocompleteApiImpl(
      GoogleApiClient googleApiClient,
      GeoDataApi geoDataApi,
      FusedLocationProviderApi locationApi) {
    this.googleApiClient = googleApiClient;
    this.geoDataApi = geoDataApi;
    this.locationApi = locationApi;
  }

  // TODO(b/32559817): Add a check to ensure that the required permissions have been granted.
  @Override
  public boolean isConfiguredCorrectly() {
    if (!googleApiClient.isConnected()) {
      Log.e(TAG, "Cannot get autocomplete predictions because Google API client is not connected.");
      return false;
    }

    return true;
  }

  @Override
  public void getAutocompletePredictions(
      String query, final FutureCallback<List<? extends AddressAutocompletePrediction>> callback) {
    Location deviceLocation = locationApi.getLastLocation(googleApiClient);
    LatLngBounds bounds =
        deviceLocation == null
            ? null
            : LatLngBounds.builder()
                .include(new LatLng(deviceLocation.getLatitude(), deviceLocation.getLongitude()))
                .build();

    geoDataApi
        .getAutocompletePredictions(
            googleApiClient,
            query,
            bounds,
            new AutocompleteFilter.Builder()
                .setTypeFilter(AutocompleteFilter.TYPE_FILTER_ADDRESS)
                .build())
        .setResultCallback(
            new ResultCallback<AutocompletePredictionBuffer>() {
              @Override
              public void onResult(AutocompletePredictionBuffer resultBuffer) {
                callback.onSuccess(convertPredictions(resultBuffer));
              }
            });
  }

  private List<? extends AddressAutocompletePrediction> convertPredictions(
      AutocompletePredictionBuffer resultBuffer) {
    List<AddressAutocompletePrediction> predictions = new ArrayList<>();

    for (AutocompletePrediction prediction : resultBuffer) {
      predictions.add(new AddressAutocompletePredictionImpl(prediction));
    }

    return predictions;
  }
}
