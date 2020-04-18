package com.android.i18n.addressinput.autocomplete.gmscore;

import com.google.android.gms.location.places.AutocompletePrediction;
import com.google.i18n.addressinput.common.AddressAutocompletePrediction;

/**
 * GMSCore implementation of {@link
 * com.google.i18n.addressinput.common.AddressAutocompletePrediction}.
 */
public class AddressAutocompletePredictionImpl extends AddressAutocompletePrediction {

  private AutocompletePrediction prediction;

  AddressAutocompletePredictionImpl(AutocompletePrediction prediction) {
    this.prediction = prediction;
  }

  @Override
  public String getPlaceId() {
    return prediction.getPlaceId();
  }

  @Override
  public CharSequence getPrimaryText() {
    return prediction.getPrimaryText(null);
  }

  @Override
  public CharSequence getSecondaryText() {
    return prediction.getSecondaryText(null);
  }
}
