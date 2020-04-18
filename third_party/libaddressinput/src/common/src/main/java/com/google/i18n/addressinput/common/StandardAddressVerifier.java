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

package com.google.i18n.addressinput.common;

import static com.google.i18n.addressinput.common.AddressField.ADMIN_AREA;
import static com.google.i18n.addressinput.common.AddressField.COUNTRY;
import static com.google.i18n.addressinput.common.AddressField.DEPENDENT_LOCALITY;
import static com.google.i18n.addressinput.common.AddressField.LOCALITY;
import static com.google.i18n.addressinput.common.AddressField.ORGANIZATION;
import static com.google.i18n.addressinput.common.AddressField.POSTAL_CODE;
import static com.google.i18n.addressinput.common.AddressField.RECIPIENT;
import static com.google.i18n.addressinput.common.AddressField.SORTING_CODE;
import static com.google.i18n.addressinput.common.AddressField.STREET_ADDRESS;

import com.google.i18n.addressinput.common.LookupKey.ScriptType;

import java.util.Collections;
import java.util.EnumSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Performs various consistency checks on an AddressData. This uses a {@link FieldVerifier} to check
 * each field in the address.
 */
public final class StandardAddressVerifier {

  private static final String LOCALE_DELIMITER = "--";

  protected final FieldVerifier rootVerifier;

  protected final Map<AddressField, List<AddressProblemType>> problemMap;

  /**
   * Uses the rootVerifier to perform the standard checks on the address fields, as defined in
   * {@link StandardChecks}.
   */
  public StandardAddressVerifier(FieldVerifier rootVerifier) {
    this(rootVerifier, StandardChecks.PROBLEM_MAP);
  }

  /**
   * Uses the rootVerifier to perform the given checks on the address fields. A reference to
   * problemMap is maintained. It is not modified by this class, and should not be modified
   * subsequent to this call.
   */
  public StandardAddressVerifier(FieldVerifier rootVerifier,
      Map<AddressField, List<AddressProblemType>> problemMap) {
    this.rootVerifier = rootVerifier;
    this.problemMap = problemMap;
  }

  /**
   * Verifies the address, reporting problems to problems.
   */
  public void verify(AddressData address, AddressProblems problems) {
    NotifyingListener listener = new NotifyingListener();
    verifyAsync(address, problems, listener);
    try {
      listener.waitLoadingEnd();
    } catch (InterruptedException e) {
      throw new RuntimeException(e);
    }
  }

  public void verifyAsync(
      AddressData address, AddressProblems problems, DataLoadListener listener) {
    Thread verifier = new Thread(new Verifier(address, problems, listener));
    verifier.start();
  }

  /**
   * Verifies only the specified fields in the address.
   */
  public void verifyFields(
      AddressData address, AddressProblems problems, EnumSet<AddressField> addressFieldsToVerify) {
    new Verifier(address, problems, new NotifyingListener(), addressFieldsToVerify).run();
  }

  private class Verifier implements Runnable {
    private AddressData address;
    private AddressProblems problems;
    private DataLoadListener listener;
    private EnumSet<AddressField> addressFieldsToVerify;

    Verifier(AddressData address, AddressProblems problems, DataLoadListener listener) {
      this(address, problems, listener, EnumSet.allOf(AddressField.class));
    }

    Verifier(
        AddressData address, AddressProblems problems, DataLoadListener listener,
        EnumSet<AddressField> addressFieldsToVerify) {
      this.address = address;
      this.problems = problems;
      this.listener = listener;
      this.addressFieldsToVerify = addressFieldsToVerify;
    }

    @Override
    public void run() {
      listener.dataLoadingBegin();

      FieldVerifier v = rootVerifier;

      ScriptType script = null;
      if (address.getLanguageCode() != null) {
        if (Util.isExplicitLatinScript(address.getLanguageCode())) {
          script = ScriptType.LATIN;
        } else {
          script = ScriptType.LOCAL;
        }
      }

      // The first four calls refine the verifier, so must come first, and in this
      // order.
      verifyFieldIfSelected(script, v, COUNTRY, address.getPostalCountry(), problems);
      if (isFieldSelected(COUNTRY) && problems.isEmpty()) {
        // Ensure we start with the right language country sub-key.
        String countrySubKey = address.getPostalCountry();
        if (address.getLanguageCode() != null && !address.getLanguageCode().equals("")) {
          countrySubKey += (LOCALE_DELIMITER + address.getLanguageCode());
        }
        v = v.refineVerifier(countrySubKey);
        verifyFieldIfSelected(script, v, ADMIN_AREA, address.getAdministrativeArea(), problems);
        if (isFieldSelected(ADMIN_AREA) && problems.isEmpty()) {
          v = v.refineVerifier(address.getAdministrativeArea());
          verifyFieldIfSelected(script, v, LOCALITY, address.getLocality(), problems);
          if (isFieldSelected(LOCALITY) && problems.isEmpty()) {
            v = v.refineVerifier(address.getLocality());
            verifyFieldIfSelected(
                script, v, DEPENDENT_LOCALITY, address.getDependentLocality(), problems);
            if (isFieldSelected(DEPENDENT_LOCALITY) && problems.isEmpty()) {
              v = v.refineVerifier(address.getDependentLocality());
            }
          }
        }
      }

      // This concatenation is for the purpose of validation only - the important part is to check
      // we have at least one value filled in for lower-level components.
      String street =
          Util.joinAndSkipNulls("\n", address.getAddressLine1(),
              address.getAddressLine2());

      // Remaining calls don't change the field verifier.
      verifyFieldIfSelected(script, v, POSTAL_CODE, address.getPostalCode(), problems);
      verifyFieldIfSelected(script, v, STREET_ADDRESS, street, problems);
      verifyFieldIfSelected(script, v, SORTING_CODE, address.getSortingCode(), problems);
      verifyFieldIfSelected(script, v, ORGANIZATION, address.getOrganization(), problems);
      verifyFieldIfSelected(script, v, RECIPIENT, address.getRecipient(), problems);

      postVerify(v, address, problems);

      listener.dataLoadingEnd();
    }

    /**
     * Skips address fields that are not included in {@code addressFieldsToVerify}.
     */
    private boolean verifyFieldIfSelected(LookupKey.ScriptType script, FieldVerifier verifier,
        AddressField field, String value, AddressProblems problems) {
      if (!isFieldSelected(field)) {
        return true;
      }

      return verifyField(script, verifier, field, value, problems);
    }

    private boolean isFieldSelected(AddressField field) {
      return addressFieldsToVerify.contains(field);
    }
  }

  /**
   * Hook to perform any final processing using the final verifier.  Default does no additional
   * verification.
   */
  protected void postVerify(FieldVerifier verifier, AddressData address, AddressProblems problems) {
  }

  /**
   * Hook called by verify with each verifiable field, in order.  Override to provide pre- or
   * post-checks for all fields.
   */
  protected boolean verifyField(LookupKey.ScriptType script, FieldVerifier verifier,
      AddressField field, String value, AddressProblems problems) {
    Iterator<AddressProblemType> iter = getProblemIterator(field);
    while (iter.hasNext()) {
      AddressProblemType prob = iter.next();
      if (!verifyProblemField(script, verifier, prob, field, value, problems)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Hook for on-the-fly modification of the problem list.  Override to change the problems to
   * check for a particular field.  Generally, changing the problemMap passed to the constructor
   * is a better approach.
   */
  protected Iterator<AddressProblemType> getProblemIterator(AddressField field) {
    List<AddressProblemType> list = problemMap.get(field);
    if (list == null) {
      list = Collections.emptyList();
    }
    return list.iterator();
  }

  /**
   * Hook for adding special checks for particular problems and/or fields.
   */
  protected boolean verifyProblemField(LookupKey.ScriptType script, FieldVerifier verifier,
      AddressProblemType problem, AddressField field, String datum, AddressProblems problems) {
    return verifier.check(script, problem, field, datum, problems);
  }
}
