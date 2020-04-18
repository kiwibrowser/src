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

import android.app.ProgressDialog;
import android.content.Context;
import android.os.Handler;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.Spinner;
import android.widget.TextView;
import com.android.i18n.addressinput.AddressUiComponent.UiComponent;
import com.google.i18n.addressinput.common.AddressAutocompleteApi;
import com.google.i18n.addressinput.common.AddressData;
import com.google.i18n.addressinput.common.AddressDataKey;
import com.google.i18n.addressinput.common.AddressField;
import com.google.i18n.addressinput.common.AddressField.WidthType;
import com.google.i18n.addressinput.common.AddressProblemType;
import com.google.i18n.addressinput.common.AddressProblems;
import com.google.i18n.addressinput.common.AddressVerificationNodeData;
import com.google.i18n.addressinput.common.CacheData;
import com.google.i18n.addressinput.common.ClientCacheManager;
import com.google.i18n.addressinput.common.ClientData;
import com.google.i18n.addressinput.common.DataLoadListener;
import com.google.i18n.addressinput.common.FieldVerifier;
import com.google.i18n.addressinput.common.FormController;
import com.google.i18n.addressinput.common.FormOptions;
import com.google.i18n.addressinput.common.FormatInterpreter;
import com.google.i18n.addressinput.common.LookupKey;
import com.google.i18n.addressinput.common.LookupKey.KeyType;
import com.google.i18n.addressinput.common.LookupKey.ScriptType;
import com.google.i18n.addressinput.common.OnAddressSelectedListener;
import com.google.i18n.addressinput.common.RegionData;
import com.google.i18n.addressinput.common.StandardAddressVerifier;
import com.google.i18n.addressinput.common.Util;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Address widget that lays out address fields, validate and format addresses according to local
 * customs.
 */
public class AddressWidget implements AdapterView.OnItemSelectedListener {

  private Context context;

  private ViewGroup rootView;

  private CacheData cacheData;

  private ClientData clientData;

  // A map for all address fields.
  // TODO(user): Fix this to avoid needing to map specific address lines.
  private final EnumMap<AddressField, AddressUiComponent> inputWidgets =
      new EnumMap<AddressField, AddressUiComponent>(AddressField.class);

  private FormController formController;

  private FormatInterpreter formatInterpreter;

  private FormOptions.Snapshot formOptions;

  private StandardAddressVerifier verifier;

  private ProgressDialog progressDialog;

  private String currentRegion;

  private boolean autocompleteEnabled = false;

  private AddressAutocompleteController autocompleteController;

  // The current language the widget uses in BCP47 format. It differs from the default locale of
  // the phone in that it contains information on the script to use.
  private String widgetLocale;

  private ScriptType script;

  // Possible labels that could be applied to the admin area field of the current country.
  // Examples include "state", "province", "emirate", etc.
  private static final Map<String, Integer> ADMIN_LABELS;
  // Possible labels that could be applied to the locality (city) field of the current country.
  // Examples include "city" or "district".
  private static final Map<String, Integer> LOCALITY_LABELS;
  // Possible labels that could be applied to the sublocality field of the current country.
  // Examples include "suburb" or "neighborhood".
  private static final Map<String, Integer> SUBLOCALITY_LABELS;

  private static final FormOptions.Snapshot SHOW_ALL_FIELDS = new FormOptions().createSnapshot();

  // The appropriate label that should be applied to the zip code field of the current country.
  private enum ZipLabel {
    ZIP,
    POSTAL,
    PIN,
    EIRCODE
  }

  private ZipLabel zipLabel;

  static {
    Map<String, Integer> adminLabelMap = new HashMap<String, Integer>(15);
    adminLabelMap.put("area", R.string.i18n_area);
    adminLabelMap.put("county", R.string.i18n_county);
    adminLabelMap.put("department", R.string.i18n_department);
    adminLabelMap.put("district", R.string.i18n_district);
    adminLabelMap.put("do_si", R.string.i18n_do_si);
    adminLabelMap.put("emirate", R.string.i18n_emirate);
    adminLabelMap.put("island", R.string.i18n_island);
    adminLabelMap.put("oblast", R.string.i18n_oblast);
    adminLabelMap.put("parish", R.string.i18n_parish);
    adminLabelMap.put("prefecture", R.string.i18n_prefecture);
    adminLabelMap.put("province", R.string.i18n_province);
    adminLabelMap.put("state", R.string.i18n_state);
    ADMIN_LABELS = Collections.unmodifiableMap(adminLabelMap);

    Map<String, Integer> localityLabelMap = new HashMap<String, Integer>(2);
    localityLabelMap.put("city", R.string.i18n_locality_label);
    localityLabelMap.put("district", R.string.i18n_district);
    localityLabelMap.put("post_town", R.string.i18n_post_town);
    localityLabelMap.put("suburb", R.string.i18n_suburb);
    LOCALITY_LABELS = Collections.unmodifiableMap(localityLabelMap);

    Map<String, Integer> sublocalityLabelMap = new HashMap<String, Integer>(2);
    sublocalityLabelMap.put("suburb", R.string.i18n_suburb);
    sublocalityLabelMap.put("district", R.string.i18n_district);
    sublocalityLabelMap.put("neighborhood", R.string.i18n_neighborhood);
    sublocalityLabelMap.put("village_township", R.string.i18n_village_township);
    sublocalityLabelMap.put("townland", R.string.i18n_townland);
    SUBLOCALITY_LABELS = Collections.unmodifiableMap(sublocalityLabelMap);
  }

  // Need handler for callbacks to the UI thread
  final Handler handler = new Handler();

  final Runnable updateMultipleFields = new Runnable() {
    @Override
    public void run() {
      updateFields();
    }
  };

  private class UpdateRunnable implements Runnable {
    private AddressField myId;

    public UpdateRunnable(AddressField id) {
      myId = id;
    }

    @Override
    public void run() {
      updateInputWidget(myId);
    }
  }

  private static class AddressSpinnerInfo {
    private Spinner view;

    private AddressField id;

    private AddressField parentId;

    private ArrayAdapter<String> adapter;

    private List<RegionData> currentRegions;

    @SuppressWarnings("unchecked")
    public AddressSpinnerInfo(Spinner view, AddressField id, AddressField parentId) {
      this.view = view;
      this.id = id;
      this.parentId = parentId;
      this.adapter = (ArrayAdapter<String>) view.getAdapter();
    }

    public void setSpinnerList(List<RegionData> list, String defaultKey) {
      currentRegions = list;
      adapter.clear();
      for (RegionData item : list) {
        adapter.add(item.getDisplayName());
      }
      adapter.sort(Collator.getInstance(Locale.getDefault()));
      if (defaultKey.length() == 0) {
        view.setSelection(0);
      } else {
        int position = adapter.getPosition(defaultKey);
        view.setSelection(position);
      }
    }

    // Returns the region key of the currently selected region in the Spinner.
    public String getRegionCode(int position) {
      if (adapter.getCount() <= position) {
        return "";
      }
      String value = adapter.getItem(position);
      return getRegionDataKeyForValue(value);
    }

    // Returns the region key for the region value.
    public String getRegionDataKeyForValue(String value) {
      for (RegionData data : currentRegions) {
        if (data.getDisplayName().endsWith(value)) {
          return data.getKey();
        }
      }
      return "";
    }
  }

  private final ArrayList<AddressSpinnerInfo> spinners = new ArrayList<AddressSpinnerInfo>();

  private AddressWidgetUiComponentProvider componentProvider;

  private WidthType getFieldWidthType(AddressUiComponent field) {
    // TODO(user): For drop-downs (spinners), derive the width-type from the list of values.
    return field.getId().getWidthTypeForRegion(currentRegion);
  }

  private void createView(ViewGroup rootView, AddressUiComponent field, String defaultKey,
      boolean readOnly) {
    @SuppressWarnings("deprecation")  // FILL_PARENT renamed MATCH_PARENT in API Level 8.
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LayoutParams.FILL_PARENT,
            LayoutParams.WRAP_CONTENT);
    String fieldText = field.getFieldName();
    WidthType widthType = getFieldWidthType(field);

    if (fieldText.length() > 0) {
      TextView textView = componentProvider.createUiLabel(fieldText, widthType);
      rootView.addView(textView, lp);
    }
    if (field.getUiType().equals(UiComponent.EDIT)) {
      if (autocompleteEnabled && field.getId() == AddressField.ADDRESS_LINE_1) {
        AutoCompleteTextView autocomplete =
            componentProvider.createUiAutoCompleteTextField(widthType);
        autocomplete.setEnabled(!readOnly);
        autocompleteController.setView(autocomplete);
        autocompleteController.setOnAddressSelectedListener(
            new OnAddressSelectedListener() {
              @Override
              public void onAddressSelected(AddressData addressData) {
                // Autocompletion will never return the recipient or the organization, so we don't
                // want to overwrite those fields. We copy the recipient and organization fields
                // over to avoid this.
                AddressData current = AddressWidget.this.getAddressData();
                AddressWidget.this.renderFormWithSavedAddress(AddressData.builder(addressData)
                    .setRecipient(current.getRecipient())
                    .setOrganization(current.getOrganization())
                    .build());
              }
            });
        field.setView(autocomplete);
        rootView.addView(autocomplete, lp);
      } else {
        EditText editText = componentProvider.createUiTextField(widthType);
        field.setView(editText);
        editText.setEnabled(!readOnly);
        rootView.addView(editText, lp);
      }
    } else if (field.getUiType().equals(UiComponent.SPINNER)) {
      ArrayAdapter<String> adapter = componentProvider.createUiPickerAdapter(widthType);
      Spinner spinner = componentProvider.createUiPickerSpinner(widthType);

      field.setView(spinner);
      spinner.setEnabled(!readOnly);
      rootView.addView(spinner, lp);
      spinner.setAdapter(adapter);
      AddressSpinnerInfo spinnerInfo =
          new AddressSpinnerInfo(spinner, field.getId(), field.getParentId());
      spinnerInfo.setSpinnerList(field.getCandidatesList(), defaultKey);

      if (fieldText.length() > 0) {
        spinner.setPrompt(fieldText);
      }
      spinner.setOnItemSelectedListener(this);
      spinners.add(spinnerInfo);
    }
  }

  private void createViewForCountry() {
    if (!formOptions.isHidden(AddressField.COUNTRY)) {
      // For initialization when the form is first created.
      if (!inputWidgets.containsKey(AddressField.COUNTRY)) {
        buildCountryListBox();
      }
      createView(
          rootView,
          inputWidgets.get(AddressField.COUNTRY),
          getLocalCountryName(currentRegion),
          formOptions.isReadonly(AddressField.COUNTRY));
    }
  }

  /**
   *  Associates each field with its corresponding AddressUiComponent.
   */
  private void buildFieldWidgets() {
    AddressData data = new AddressData.Builder().setCountry(currentRegion).build();
    LookupKey key = new LookupKey.Builder(LookupKey.KeyType.DATA).setAddressData(data).build();
    AddressVerificationNodeData countryNode = clientData.getDefaultData(key.toString());

    // Set up AddressField.ADMIN_AREA
    AddressUiComponent adminAreaUi = new AddressUiComponent(AddressField.ADMIN_AREA);
    adminAreaUi.setFieldName(getAdminAreaFieldName(countryNode));
    inputWidgets.put(AddressField.ADMIN_AREA, adminAreaUi);

    // Set up AddressField.LOCALITY
    AddressUiComponent localityUi = new AddressUiComponent(AddressField.LOCALITY);
    localityUi.setFieldName(getLocalityFieldName(countryNode));
    inputWidgets.put(AddressField.LOCALITY, localityUi);

    // Set up AddressField.DEPENDENT_LOCALITY
    AddressUiComponent subLocalityUi = new AddressUiComponent(AddressField.DEPENDENT_LOCALITY);
    subLocalityUi.setFieldName(getSublocalityFieldName(countryNode));
    inputWidgets.put(AddressField.DEPENDENT_LOCALITY, subLocalityUi);

    // Set up AddressField.ADDRESS_LINE_1
    AddressUiComponent addressLine1Ui = new AddressUiComponent(AddressField.ADDRESS_LINE_1);
    addressLine1Ui.setFieldName(context.getString(R.string.i18n_address_line1_label));
    inputWidgets.put(AddressField.ADDRESS_LINE_1, addressLine1Ui);

    // Set up AddressField.ADDRESS_LINE_2
    AddressUiComponent addressLine2Ui = new AddressUiComponent(AddressField.ADDRESS_LINE_2);
    addressLine2Ui.setFieldName("");
    inputWidgets.put(AddressField.ADDRESS_LINE_2, addressLine2Ui);

    // Set up AddressField.ORGANIZATION
    AddressUiComponent organizationUi = new AddressUiComponent(AddressField.ORGANIZATION);
    organizationUi.setFieldName(context.getString(R.string.i18n_organization_label));
    inputWidgets.put(AddressField.ORGANIZATION, organizationUi);

    // Set up AddressField.RECIPIENT
    AddressUiComponent recipientUi = new AddressUiComponent(AddressField.RECIPIENT);
    recipientUi.setFieldName(context.getString(R.string.i18n_recipient_label));
    inputWidgets.put(AddressField.RECIPIENT, recipientUi);

    // Set up AddressField.POSTAL_CODE
    AddressUiComponent postalCodeUi = new AddressUiComponent(AddressField.POSTAL_CODE);
    postalCodeUi.setFieldName(getZipFieldName(countryNode));
    inputWidgets.put(AddressField.POSTAL_CODE, postalCodeUi);

    // Set up AddressField.SORTING_CODE
    AddressUiComponent sortingCodeUi = new AddressUiComponent(AddressField.SORTING_CODE);
    sortingCodeUi.setFieldName("CEDEX");
    inputWidgets.put(AddressField.SORTING_CODE, sortingCodeUi);
  }

  private void initializeDropDowns() {
    AddressUiComponent adminAreaUi = inputWidgets.get(AddressField.ADMIN_AREA);
    List<RegionData> adminAreaList = getRegionData(AddressField.COUNTRY);
    adminAreaUi.initializeCandidatesList(adminAreaList);

    AddressUiComponent localityUi = inputWidgets.get(AddressField.LOCALITY);
    List<RegionData> localityList = getRegionData(AddressField.ADMIN_AREA);
    localityUi.initializeCandidatesList(localityList);
  }

  // ZIP code is called postal code in some countries, and PIN code in India. This method returns
  // the appropriate name for the given countryNode.
  private String getZipFieldName(AddressVerificationNodeData countryNode) {
    String zipName;
    String zipType = countryNode.get(AddressDataKey.ZIP_NAME_TYPE);
    if (zipType == null || zipType.equals("postal")) {
      zipLabel = ZipLabel.POSTAL;
      zipName = context.getString(R.string.i18n_postal_code_label);
    } else if (zipType.equals("eircode")) {
      zipLabel = ZipLabel.EIRCODE;
      zipName = context.getString(R.string.i18n_eir_code_label);
    } else if (zipType.equals("pin")) {
      zipLabel = ZipLabel.PIN;
      zipName = context.getString(R.string.i18n_pin_code_label);
    } else {
      zipLabel = ZipLabel.ZIP;
      zipName = context.getString(R.string.i18n_zip_code_label);
    }
    return zipName;
  }

  private String getLocalityFieldName(AddressVerificationNodeData countryNode) {
    String localityLabelType = countryNode.get(AddressDataKey.LOCALITY_NAME_TYPE);
    Integer result = LOCALITY_LABELS.get(localityLabelType);
    if (result == null) {
      // Fallback to city.
      result = R.string.i18n_locality_label;
    }
    return context.getString(result);
  }

  private String getSublocalityFieldName(AddressVerificationNodeData countryNode) {
    String sublocalityLabelType = countryNode.get(AddressDataKey.SUBLOCALITY_NAME_TYPE);
    Integer result = SUBLOCALITY_LABELS.get(sublocalityLabelType);
    if (result == null) {
      // Fallback to suburb.
      result = R.string.i18n_suburb;
    }
    return context.getString(result);
  }

  private String getAdminAreaFieldName(AddressVerificationNodeData countryNode) {
    String adminLabelType = countryNode.get(AddressDataKey.STATE_NAME_TYPE);
    Integer result = ADMIN_LABELS.get(adminLabelType);
    if (result == null) {
      // Fallback to province.
      result = R.string.i18n_province;
    }
    return context.getString(result);
  }

  private void buildCountryListBox() {
    // Set up AddressField.COUNTRY
    AddressUiComponent countryUi = new AddressUiComponent(AddressField.COUNTRY);
    countryUi.setFieldName(context.getString(R.string.i18n_country_or_region_label));
    ArrayList<RegionData> countries = new ArrayList<RegionData>();
    for (RegionData regionData : formController.getRegionData(new LookupKey.Builder(
        KeyType.DATA).build())) {
      String regionKey = regionData.getKey();
      Log.i(this.toString(), "Looking at regionKey: " + regionKey);
      // ZZ represents an unknown region code.
      if (!regionKey.equals("ZZ") && !formOptions.isBlacklistedRegion(regionKey)) {
        Log.i(this.toString(), "Adding " + regionKey);
        String localCountryName = getLocalCountryName(regionKey);
        RegionData country = new RegionData.Builder().setKey(regionKey).setName(
            localCountryName).build();
        countries.add(country);
      }
    }
    countryUi.initializeCandidatesList(countries);
    inputWidgets.put(AddressField.COUNTRY, countryUi);
  }

  private String getLocalCountryName(String regionCode) {
    return (new Locale("", regionCode)).getDisplayCountry(Locale.getDefault());
  }

  private AddressSpinnerInfo findSpinnerByView(View view) {
    for (AddressSpinnerInfo spinnerInfo : spinners) {
      if (spinnerInfo.view == view) {
        return spinnerInfo;
      }
    }
    return null;
  }

  private void updateFields() {
    removePreviousViews();
    createViewForCountry();
    buildFieldWidgets();
    initializeDropDowns();
    layoutAddressFields();
  }

  private void removePreviousViews() {
    if (rootView == null) {
      return;
    }
    rootView.removeAllViews();
  }

  private void layoutAddressFields() {
    for (AddressField field : formatInterpreter.getAddressFieldOrder(script,
          currentRegion)) {
      if (!formOptions.isHidden(field)) {
        createView(rootView, inputWidgets.get(field), "", formOptions.isReadonly(field));
      }
    }
  }

  private void updateChildNodes(AdapterView<?> parent, int position) {
    AddressSpinnerInfo spinnerInfo = findSpinnerByView(parent);
    if (spinnerInfo == null) {
      return;
    }

    // Find all the child spinners, if any, that depend on this one.
    final AddressField myId = spinnerInfo.id;
    if (myId != AddressField.COUNTRY && myId != AddressField.ADMIN_AREA
        && myId != AddressField.LOCALITY) {
      // Only a change in the three AddressFields above will trigger a change in other
      // AddressFields. Therefore, for all other AddressFields, we return immediately.
      return;
    }

    String regionCode = spinnerInfo.getRegionCode(position);
    if (myId == AddressField.COUNTRY) {
      updateWidgetOnCountryChange(regionCode);
      return;
    }

    formController.requestDataForAddress(getAddressData(), new DataLoadListener() {
      @Override
      public void dataLoadingBegin(){
      }

      @Override
      public void dataLoadingEnd() {
        Runnable updateChild = new UpdateRunnable(myId);
        handler.post(updateChild);
      }
    });
  }

  public void updateWidgetOnCountryChange(String regionCode) {
    if (currentRegion.equalsIgnoreCase(regionCode)) {
      return;
    }
    currentRegion = regionCode;
    formController.setCurrentCountry(currentRegion);
    renderForm();
  }

  private void updateInputWidget(AddressField myId) {
    for (AddressSpinnerInfo child : spinners) {
      if (child.parentId == myId) {
        List<RegionData> candidates = getRegionData(child.parentId);
        child.setSpinnerList(candidates, "");
      }
    }
  }

  public void renderForm() {
    createViewForCountry();
    setWidgetLocaleAndScript();
    AddressData data = new AddressData.Builder().setCountry(currentRegion)
        .setLanguageCode(widgetLocale).build();
    formController.requestDataForAddress(data, new DataLoadListener() {
      @Override
      public void dataLoadingBegin() {
        progressDialog = componentProvider.getUiActivityIndicatorView();
        progressDialog.setMessage(context.getString(R.string.address_data_loading));
        Log.d(this.toString(), "Progress dialog started.");
      }
      @Override
      public void dataLoadingEnd() {
        Log.d(this.toString(), "Data loading completed.");
        progressDialog.dismiss();
        Log.d(this.toString(), "Progress dialog stopped.");
        handler.post(updateMultipleFields);
      }
    });
  }

  private void setWidgetLocaleAndScript() {
    widgetLocale = Util.getWidgetCompatibleLanguageCode(Locale.getDefault(), currentRegion);
    formController.setLanguageCode(widgetLocale);
    script = Util.isExplicitLatinScript(widgetLocale) ? ScriptType.LATIN : ScriptType.LOCAL;
  }

  private List<RegionData> getRegionData(AddressField parentField) {
    AddressData address = getAddressData();

    // Removes language code from address if it is default. This address is used to build
    // lookup key, which neglects default language. For example, instead of "data/US--en/CA",
    // the right lookup key is "data/US/CA".
    if (formController.isDefaultLanguage(address.getLanguageCode())) {
      address = AddressData.builder(address).setLanguageCode(null).build();
    }

    LookupKey parentKey = formController.getDataKeyFor(address).getKeyForUpperLevelField(
        parentField);
    List<RegionData> candidates;
    // Can't build a key with parent field, quit.
    if (parentKey == null) {
      Log.w(this.toString(), "Can't build key with parent field " + parentField + ". One of"
          + " the ancestor fields might be empty");

      // Removes candidates that exist from previous settings. For example, data/US has a
      // list of candidates AB, BC, CA, etc, that list should be cleaned up when user updates
      // the address by changing country to Channel Islands.
      candidates = new ArrayList<RegionData>(1);
    } else {
      candidates = formController.getRegionData(parentKey);
    }
    return candidates;
  }

  /**
   * Creates an AddressWidget to be attached to rootView for the specific context using the
   * default UI component provider.
   */
  public AddressWidget(Context context, ViewGroup rootView, FormOptions formOptions,
      ClientCacheManager cacheManager) {
    this(context, rootView, formOptions, cacheManager,
        new AddressWidgetUiComponentProvider(context));
  }

  /**
   * Creates an AddressWidget to be attached to rootView for the specific context using UI
   * component provided by the provider.
   */
  public AddressWidget(Context context, ViewGroup rootView, FormOptions formOptions,
      ClientCacheManager cacheManager, AddressWidgetUiComponentProvider provider) {
    componentProvider = provider;
    currentRegion =
        ((TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE))
        .getSimCountryIso().toUpperCase(Locale.US);
    if (currentRegion.length() == 0) {
      currentRegion = "US";
    }
    init(context, rootView, formOptions.createSnapshot(), cacheManager);
    renderForm();
  }

  /**
   * Creates an AddressWidget to be attached to rootView for the specific context using the
   * default UI component provider, and fill out the address form with savedAddress.
   */
  public AddressWidget(Context context, ViewGroup rootView, FormOptions formOptions,
      ClientCacheManager cacheManager, AddressData savedAddress) {
    this(context, rootView, formOptions, cacheManager, savedAddress,
        new AddressWidgetUiComponentProvider(context));
  }

  /**
   * Creates an AddressWidget to be attached to rootView for the specific context using UI
   * component provided by the provider, and fill out the address form with savedAddress.
   */
  public AddressWidget(Context context, ViewGroup rootView, FormOptions formOptions,
      ClientCacheManager cacheManager, AddressData savedAddress,
      AddressWidgetUiComponentProvider provider) {
    componentProvider = provider;
    currentRegion = savedAddress.getPostalCountry();
    // Postal country must be 2 letter country code. Otherwise default to US.
    if (currentRegion == null || currentRegion.length() != 2) {
      currentRegion = "US";
    } else {
      currentRegion = currentRegion.toUpperCase();
    }
    init(context, rootView, formOptions.createSnapshot(), cacheManager);
    renderFormWithSavedAddress(savedAddress);
  }

  /*
   * Enables autocompletion for the ADDRESS_LINE_1 field. With autocompletion enabled, the user
   * will see suggested addresses in a dropdown menu below the ADDRESS_LINE_1 field as they are
   * typing, and when they select an address, the form fields will be autopopulated with the
   * selected address.
   *
   * NOTE: This feature is currently experimental.
   *
   * If the AddressAutocompleteApi is not configured correctly, then the AddressWidget will degrade
   * gracefully to an ordinary plain text input field without autocomplete.
   */
  public void enableAutocomplete(
      AddressAutocompleteApi autocompleteApi, PlaceDetailsApi placeDetailsApi) {
    AddressAutocompleteController autocompleteController =
        new AddressAutocompleteController(context, autocompleteApi, placeDetailsApi);
    if (autocompleteApi.isConfiguredCorrectly()) {
      this.autocompleteEnabled = true;
      this.autocompleteController = autocompleteController;

      // The autocompleteEnabled variable set above is used in createView to determine whether to
      // use an EditText or an AutoCompleteTextView. Re-rendering the form here ensures that
      // createView is called with the updated value of autocompleteEnabled.
      renderFormWithSavedAddress(getAddressData());
    } else {
      Log.w(
          this.toString(),
          "Autocomplete not configured correctly, falling back to a plain text " + "input field.");
    }
  }

  public void disableAutocomplete() {
    this.autocompleteEnabled = false;
  }

  public void renderFormWithSavedAddress(AddressData savedAddress) {
    setWidgetLocaleAndScript();
    removePreviousViews();
    createViewForCountry();
    buildFieldWidgets();
    initializeDropDowns();
    layoutAddressFields();
    initializeFieldsWithAddress(savedAddress);
  }

  private void initializeFieldsWithAddress(AddressData savedAddress) {
    for (AddressField field : formatInterpreter.getAddressFieldOrder(script, currentRegion)) {
      String value = savedAddress.getFieldValue(field);
      if (value == null) {
        value = "";
      }

      AddressUiComponent uiComponent = inputWidgets.get(field);
      if (uiComponent != null) {
        uiComponent.setValue(value);
      }
    }
  }

  private void init(Context context, ViewGroup rootView, FormOptions.Snapshot formOptions,
      ClientCacheManager cacheManager) {
    this.context = context;
    this.rootView = rootView;
    this.formOptions = formOptions;
    // Inject Adnroid specific async request implementation here.
    this.cacheData = new CacheData(cacheManager, new AndroidAsyncEncodedRequestApi());
    this.clientData = new ClientData(cacheData);
    this.formController = new FormController(clientData, widgetLocale, currentRegion);
    this.formatInterpreter = new FormatInterpreter(formOptions);
    this.verifier = new StandardAddressVerifier(new FieldVerifier(clientData));
  }

  /**
   * Sets address data server URL. Input URL cannot be null.
   *
   * @param url The service URL.
   */
  // TODO: Remove this method and set the URL in the constructor or via the cacheData directly.
  public void setUrl(String url) {
    cacheData.setUrl(url);
  }

  /** Gets user input address in AddressData format. */
  public AddressData getAddressData() {
    AddressData.Builder builder = new AddressData.Builder();
    builder.setCountry(currentRegion);
    for (AddressField field : formatInterpreter.getAddressFieldOrder(script,
          currentRegion)) {
      AddressUiComponent addressUiComponent = inputWidgets.get(field);
      if (addressUiComponent != null) {
        String value = addressUiComponent.getValue();
        if (addressUiComponent.getUiType() == UiComponent.SPINNER) {
          // For drop-downs, return the key of the region selected instead of the value.
          View view = getViewForField(field);
          AddressSpinnerInfo spinnerInfo = findSpinnerByView(view);
          if (spinnerInfo != null) {
            value = spinnerInfo.getRegionDataKeyForValue(value);
          }
        }
        builder.set(field, value);
      }
    }
    builder.setLanguageCode(widgetLocale);
    return builder.build();
  }

  /**
   * Gets the formatted address.
   * <p>
   * This method does not validate addresses. Also, it will "normalize" the result strings by
   * removing redundant spaces and empty lines.
   *
   * @return the formatted address
   */
  public List<String> getEnvelopeAddress() {
    return getEnvelopeAddress(getAddressData());
  }

  /**
   * Gets the formatted address based on the AddressData passed in.
   */
  public List<String> getEnvelopeAddress(AddressData address) {
    return getEnvelopeAddress(formatInterpreter, address);
  }

  /**
   * Gets the formatted address based on the AddressData passed in with none of the relevant
   * fields hidden.
   */
  public static List<String> getFullEnvelopeAddress(AddressData address) {
    return getEnvelopeAddress(new FormatInterpreter(SHOW_ALL_FIELDS), address);
  }

  /**
   * Helper function for getting the formatted address based on the FormatInterpreter and
   * AddressData passed in.
   */
  private static List<String> getEnvelopeAddress(FormatInterpreter interpreter,
      AddressData address) {
    String countryCode = address.getPostalCountry();
    if (countryCode.length() != 2) {
      return Collections.emptyList();
    }
    // Avoid crashes due to lower-case country codes (leniency at the input).
    String upperCountryCode = countryCode.toUpperCase();
    if (!countryCode.equals(upperCountryCode)) {
      address = AddressData.builder(address).setCountry(upperCountryCode).build();
    }
    return interpreter.getEnvelopeAddress(address);
  }

  /**
   * Get problems found in the address data entered by the user.
   */
  public AddressProblems getAddressProblems() {
    AddressProblems problems = new AddressProblems();
    AddressData addressData = getAddressData();
    verifier.verify(addressData, problems);
    return problems;
  }

  /**
   * Displays an appropriate error message for an AddressField with a problem.
   *
   * @return the View object representing the AddressField.
   */
  public View displayErrorMessageForField(AddressData address,
      AddressField field, AddressProblemType problem) {
    Log.d(this.toString(), "Display error message for the field: " + field.toString());
    AddressUiComponent addressUiComponent = inputWidgets.get(field);
    if (addressUiComponent != null && addressUiComponent.getUiType() == UiComponent.EDIT) {
      EditText view = (EditText) addressUiComponent.getView();
      view.setError(getErrorMessageForInvalidEntry(address, field, problem));
      return view;
    }
    return null;
  }

  public String getErrorMessageForInvalidEntry(AddressData address, AddressField field,
      AddressProblemType problem) {
    switch (problem) {
      case MISSING_REQUIRED_FIELD:
        return context.getString(R.string.i18n_missing_required_field);
      case UNKNOWN_VALUE:
        return context.getString(R.string.unknown_entry);
      case INVALID_FORMAT:
        // We only support this error type for the Postal Code field.
        if (zipLabel == ZipLabel.POSTAL) {
          return context.getString(R.string.unrecognized_format_postal_code);
        } else if (zipLabel == ZipLabel.PIN) {
          return context.getString(R.string.unrecognized_format_pin_code);
        } else {
          return context.getString(R.string.unrecognized_format_zip_code);
        }
      case MISMATCHING_VALUE:
        // We only support this error type for the Postal Code field.
        if (zipLabel == ZipLabel.POSTAL) {
          return context.getString(R.string.mismatching_value_postal_code);
        } else if (zipLabel == ZipLabel.PIN) {
          return context.getString(R.string.mismatching_value_pin_code);
        } else {
          return context.getString(R.string.mismatching_value_zip_code);
        }
      case UNEXPECTED_FIELD:
        throw new IllegalStateException("unexpected problem type: " + problem);
      default:
        throw new IllegalStateException("unknown problem type: " + problem);
    }
  }

  /**
   * Clears all error messages in the UI.
   */
  public void clearErrorMessage() {
    for (AddressField field : formatInterpreter.getAddressFieldOrder(script,
          currentRegion)) {
      AddressUiComponent addressUiComponent = inputWidgets.get(field);

      if (addressUiComponent != null && addressUiComponent.getUiType() == UiComponent.EDIT) {
        EditText view = (EditText) addressUiComponent.getView();
        if (view != null) {
          view.setError(null);
        }
      }
    }
  }

  public View getViewForField(AddressField field) {
    AddressUiComponent component = inputWidgets.get(field);
    if (component == null) {
      return null;
    }
    return component.getView();
  }

  @Override
  public void onNothingSelected(AdapterView<?> arg0) {
  }

  @Override
  public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
    updateChildNodes(parent, position);
  }
}
