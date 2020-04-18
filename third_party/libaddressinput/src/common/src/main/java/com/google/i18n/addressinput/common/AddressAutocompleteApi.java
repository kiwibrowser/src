package com.google.i18n.addressinput.common;

import com.google.common.util.concurrent.FutureCallback;
import java.util.List;

/**
 * AddressAutocompleteApi encapsulates the functionality required to fetch address autocomplete
 * suggestions for an unstructured address query string entered by the user.
 *
 * An implementation using GMSCore is provided under
 * libaddressinput/android/src/main.java/com/android/i18n/addressinput/autocomplete/gmscore.
 */
public interface AddressAutocompleteApi {
  /**
   * Returns true if the AddressAutocompleteApi is properly configured to fetch autocomplete
   * predictions. This allows the caller to enable autocomplete only if the AddressAutocompleteApi
   * is properly configured (e.g. the user has granted all the necessary permissions).
   */
  boolean isConfiguredCorrectly();

  /**
   * Given an unstructured address query, getAutocompletePredictions fetches autocomplete
   * suggestions for the intended address and provides these suggestions via the callback.
   */
  void getAutocompletePredictions(
      String query, FutureCallback<List<? extends AddressAutocompletePrediction>> callback);
}
