package com.google.i18n.addressinput.common;

import java.util.Objects;

/**
 * AddressAutocompletePrediction represents an autocomplete suggestion.
 *
 * Concrete inheriting classes must provide implementations of {@link #getPlaceId}, {@link
 * #getPrimaryText}, and {@link #getSecondaryText}. An implementation using GMSCore is provided
 * under libaddressinput/android/src/main.java/com/android/i18n/addressinput/autocomplete/gmscore.
 */
public abstract class AddressAutocompletePrediction {
  /**
   * Returns the place ID of the predicted place. A place ID is a textual identifier that uniquely
   * identifies a place, which you can use to retrieve the Place object again later (for example,
   * with Google's Place Details Web API).
   */
  public abstract String getPlaceId();

  /**
   * Returns the main text describing a place. This is usually the name of the place. Examples:
   * "Eiffel Tower", and "123 Pitt Street".
   */
  public abstract CharSequence getPrimaryText();

  /**
   * Returns the subsidiary text of a place description. This is useful, for example, as a second
   * line when showing autocomplete predictions. Examples: "Avenue Anatole France, Paris, France",
   * and "Sydney, New South Wales".
   */
  public abstract CharSequence getSecondaryText();

  // equals and hashCode overridden for testing.

  @Override
  public boolean equals(Object o) {
    if (!(o instanceof AddressAutocompletePrediction)) {
      return false;
    }
    AddressAutocompletePrediction p = (AddressAutocompletePrediction) o;

    return getPlaceId().equals(p.getPlaceId())
        && getPrimaryText().equals(p.getPrimaryText())
        && getSecondaryText().equals(p.getSecondaryText());
  }

  @Override
  public int hashCode() {
    return Objects.hash(getPlaceId(), getPrimaryText(), getSecondaryText());
  }
}
