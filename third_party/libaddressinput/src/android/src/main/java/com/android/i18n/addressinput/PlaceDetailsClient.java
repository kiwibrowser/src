package com.android.i18n.addressinput;

import android.net.Uri;
import android.util.Log;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.SettableFuture;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;
import com.google.i18n.addressinput.common.AddressData;
import com.google.i18n.addressinput.common.AsyncRequestApi;
import com.google.i18n.addressinput.common.AsyncRequestApi.AsyncCallback;
import com.google.i18n.addressinput.common.JsoMap;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Implementation of the PlaceDetailsApi using the Place Details Web API
 * (https://developers.google.com/places/web-service/details). Unfortunately, the Google Place
 * Details API for Android does not include a structured representation of the address.
 */
public class PlaceDetailsClient implements PlaceDetailsApi {

  private AsyncRequestApi asyncRequestApi;
  private String apiKey;

  @VisibleForTesting static final int TIMEOUT = 5000;

  private static final String TAG = "PlaceDetailsClient";

  public PlaceDetailsClient(String apiKey, AsyncRequestApi asyncRequestApi) {
    this.asyncRequestApi = asyncRequestApi;
    this.apiKey = apiKey;
  }

  @Override
  public ListenableFuture<AddressData> getAddressData(AddressAutocompletePrediction prediction) {
    final SettableFuture<AddressData> addressData = SettableFuture.create();

    asyncRequestApi.requestObject(
        new Uri.Builder()
            .scheme("https")
            .authority("maps.googleapis.com")
            .path("maps/api/place/details/json")
            .appendQueryParameter("key", apiKey)
            .appendQueryParameter("placeid", prediction.getPlaceId())
            .build()
            .toString(),
        new AsyncCallback() {
          @Override
          public void onFailure() {
            addressData.cancel(false);
          }

          @Override
          public void onSuccess(JsoMap response) {
            // Can't use JSONObject#getJSONObject to get the 'result' because #getJSONObject calls
            // #get, which has been broken by JsoMap to only return String values
            // *grinds teeth in frustration*.
            try {
              if (response.has("result")) {
                Object result = response.getObject("result");
                if (result instanceof JSONObject) {
                  addressData.set(getAddressData((JSONObject) result));
                  Log.i(TAG, "Successfully set AddressData from Place Details API response.");
                } else {
                  Log.e(
                      TAG,
                      "Error parsing JSON response from Place Details API: "
                          + "expected 'result' field to be a JSONObject.");
                  onFailure();
                }
              } else if (response.has("error_message")) {
                String error_message = response.getString("error_message");
                Log.e(TAG, "Place Details API request failed: " + error_message);
              } else {
                Log.e(
                    TAG,
                    "Expected either result or error_message in response "
                        + "from Place Details API");
              }
            } catch (JSONException e) {
              Log.e(TAG, "Error parsing JSON response from Place Details API", e);
              onFailure();
            }
          }
        },
        TIMEOUT);

    return addressData;
  }

  private AddressData getAddressData(JSONObject result) throws JSONException {
    AddressData.Builder addressData = AddressData.builder();

    // Get the country code from address_components.
    JSONArray addressComponents = result.getJSONArray("address_components");
    // Iterate backwards since country is usually at the end.
    for (int i = addressComponents.length() - 1; i >= 0; i--) {
      JSONObject addressComponent = addressComponents.getJSONObject(i);

      List<String> types = new ArrayList<>();
      JSONArray componentTypes = addressComponent.getJSONArray("types");
      for (int j = 0; j < componentTypes.length(); j++) {
        types.add(componentTypes.getString(j));
      }

      if (types.contains("country")) {
        addressData.setCountry(addressComponent.getString("short_name"));
        break;
      }
    }

    String unescapedAdrAddress =
        result
            .getString("adr_address")
            .replace("\\\"", "\"")
            .replace("\\u003c", "<")
            .replace("\\u003e", ">");

    Pattern adrComponentPattern = Pattern.compile("[, ]{0,2}<span class=\"(.*)\">(.*)<");

    for (String adrComponent : unescapedAdrAddress.split("/span>")) {
      Matcher m = adrComponentPattern.matcher(adrComponent);
      Log.i(TAG, adrComponent + " matches: " + m.matches());
      if (m.matches() && m.groupCount() == 2) {
        String key = m.group(1);
        String value = m.group(2);
        switch (key) {
          case "street-address":
            addressData.setAddress(value);
            // TODO(b/33790911): Include the 'extended-address' and 'post-office-box' adr_address
            // fields in the AddressData address.
            break;
          case "locality":
            addressData.setLocality(value);
            break;
          case "region":
            addressData.setAdminArea(value);
            break;
          case "postal-code":
            addressData.setPostalCode(value);
            break;
          case "country-name":
            // adr_address country names are not in CLDR format, which is the format used by the
            // AddressWidget. We fetch the country code from the address_components instead.
            break;
          default:
            Log.e(TAG, "Key " + key + " not recognized in Place Details API response.");
        }
      } else {
        Log.e(TAG, "Failed to match " + adrComponent + " with regexp " + m.pattern().toString());
      }
    }

    return addressData.build();
  }
}
