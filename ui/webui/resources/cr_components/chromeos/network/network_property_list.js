// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying a list of network properties
 * in a list. This also supports editing fields inline for fields listed in
 * editFieldTypes.
 */
Polymer({
  is: 'network-property-list',

  behaviors: [I18nBehavior, CrPolicyNetworkBehavior],

  properties: {
    /**
     * The dictionary containing the properties to display.
     * @type {!Object|undefined}
     */
    propertyDict: {type: Object},

    /**
     * Fields to display.
     * @type {!Array<string>}
     */
    fields: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * Edit type of editable fields. May contain a property for any field in
     * |fields|. Other properties will be ignored. Property values can be:
     *   'String' - A text input will be displayed.
     *   'StringArray' - A text input will be displayed that expects a comma
     *       separated list of strings.
     *   'Password' - A string with input type = password.
     *   TODO(stevenjb): Support types with custom validation, e.g. IPAddress.
     *   TODO(stevenjb): Support 'Number'.
     * When a field changes, the 'property-change' event will be fired with
     * the field name and the new value provided in the event detail.
     */
    editFieldTypes: {
      type: Object,
      value: function() {
        return {};
      },
    },

    /** Prefix used to look up property key translations. */
    prefix: {
      type: String,
      value: '',
    },
  },

  /**
   * Event triggered when an input field changes. Fires a 'property-change'
   * event with the field (property) name set to the target id, and the value
   * set to the target input value.
   * @param {!Event} event The input change event.
   * @private
   */
  onValueChange_: function(event) {
    if (!this.propertyDict)
      return;
    var key = event.target.id;
    var curValue = this.get(key, this.propertyDict);
    if (typeof curValue == 'object' && !Array.isArray(curValue)) {
      // Extract the property from an ONC managed dictionary.
      curValue = CrOnc.getActiveValue(
          /** @type {!CrOnc.ManagedProperty} */ (curValue));
    }
    var newValue = this.getValueFromEditField_(key, event.target.value);
    if (newValue == curValue)
      return;
    this.fire('property-change', {field: key, value: newValue});
  },

  /**
   * @param {string} key The property key.
   * @param {string} prefix
   * @return {string} The text to display for the property label.
   * @private
   */
  getPropertyLabel_: function(key, prefix) {
    var oncKey = 'Onc' + prefix + key;
    oncKey = oncKey.replace(/\./g, '-');
    if (this.i18nExists(oncKey))
      return this.i18n(oncKey);
    // We do not provide translations for every possible network property key.
    // For keys specific to a type, strip the type prefix.
    var result = prefix + key;
    for (var entry in chrome.networkingPrivate.NetworkType) {
      var type = chrome.networkingPrivate.NetworkType[entry];
      if (result.startsWith(type + '.')) {
        result = result.substr(type.length + 1);
        break;
      }
    }
    return result;
  },

  /**
   * Generates a filter function dependent on propertyDict and editFieldTypes.
   * @param {string} prefix
   * @param {!Object} propertyDict
   * @param {!Object} editFieldTypes
   * @private
   */
  computeFilter_: function(prefix, propertyDict, editFieldTypes) {
    return key => {
      if (editFieldTypes.hasOwnProperty(key))
        return true;
      var value = this.getPropertyValue_(key, prefix, propertyDict);
      return value !== undefined && value !== '';
    };
  },

  /**
   * @param {string} key The property key.
   * @param {!Object} propertyDict
   * @return {boolean}
   * @private
   */
  isPropertyEditable_: function(key, propertyDict) {
    var property = /** @type {!CrOnc.ManagedProperty|undefined} */ (
        this.get(key, propertyDict));
    if (property === undefined) {
      // Unspecified properties in policy configurations are not user
      // modifiable. https://crbug.com/819837.
      var source = propertyDict.Source;
      return source != 'UserPolicy' && source != 'DevicePolicy';
    }
    return !this.isNetworkPolicyEnforced(property);
  },

  /**
   * @param {string} key The property key.
   * @param {!Object} editFieldTypes
   * @return {boolean}
   * @private
   */
  isEditTypeAny_: function(key, editFieldTypes) {
    return editFieldTypes[key] !== undefined;
  },

  /**
   * @param {string} key The property key.
   * @param {!Object} propertyDict
   * @param {!Object} editFieldTypes
   * @return {boolean}
   * @private
   */
  isEditTypeInput_: function(key, propertyDict, editFieldTypes) {
    if (!this.isPropertyEditable_(key, propertyDict))
      return false;
    var editType = editFieldTypes[key];
    return editType == 'String' || editType == 'StringArray' ||
        editType == 'Password';
  },

  /**
   * @param {string} key The property key.
   * @param {!Object} editFieldTypes
   * @return {string}
   * @private
   */
  getEditInputType_: function(key, editFieldTypes) {
    return editFieldTypes[key] == 'Password' ? 'password' : 'text';
  },

  /**
   * @param {string} key The property key.
   * @param {!Object} propertyDict
   * @param {!Object} editFieldTypes
   * @return {boolean}
   * @private
   */
  isEditable_: function(key, propertyDict, editFieldTypes) {
    if (!this.isPropertyEditable_(key, propertyDict))
      return false;
    return this.isEditTypeAny_(key, editFieldTypes);
  },

  /**
   * @param {string} key The property key.
   * @param {!Object} propertyDict
   * @return {*} The managed property dictionary associated with |key|.
   * @private
   */
  getProperty_: function(key, propertyDict) {
    var property = this.get(key, propertyDict);
    if (property === undefined && propertyDict.Source) {
      // Provide an empty property object with the network policy source.
      // See https://crbug.com/819837 for more info.
      return {Effective: propertyDict.Source};
    }
    return property;
  },

  /**
   * @param {string} key The property key.
   * @param {string} prefix
   * @param {!Object} propertyDict
   * @return {string} The text to display for the property value.
   * @private
   */
  getPropertyValue_: function(key, prefix, propertyDict) {
    var value = this.get(key, propertyDict);
    if (value === undefined)
      return '';
    if (typeof value == 'object' && !Array.isArray(value)) {
      // Extract the property from an ONC managed dictionary
      value =
          CrOnc.getActiveValue(/** @type {!CrOnc.ManagedProperty} */ (value));
    }
    if (Array.isArray(value))
      return value.join(', ');

    var customValue = this.getCustomPropertyValue_(key, value);
    if (customValue)
      return customValue;
    if (typeof value == 'number' || typeof value == 'boolean')
      return value.toString();

    assert(typeof value == 'string');
    var valueStr = /** @type {string} */ (value);
    var oncKey = 'Onc' + prefix + key;
    oncKey = oncKey.replace(/\./g, '-');
    oncKey += '_' + valueStr;
    if (this.i18nExists(oncKey))
      return this.i18n(oncKey);
    return valueStr;
  },

  /**
   * Converts edit field values to the correct edit type.
   * @param {string} key The property key.
   * @param {*} fieldValue The value from the field.
   * @return {*}
   * @private
   */
  getValueFromEditField_(key, fieldValue) {
    var editType = this.editFieldTypes[key];
    if (editType == 'StringArray')
      return fieldValue.toString().split(/, */);
    return fieldValue;
  },

  /**
   * @param {string} key The property key.
   * @param {*} value The property value.
   * @return {string} The text to display for the property value. If the key
   *     does not correspond to a custom property, an empty string is returned.
   */
  getCustomPropertyValue_: function(key, value) {
    if (key == 'Tether.BatteryPercentage') {
      assert(typeof value == 'number');
      return this.i18n('OncTether-BatteryPercentage_Value', value.toString());
    }

    if (key == 'Tether.SignalStrength') {
      assert(typeof value == 'number');
      // Possible |signalStrength| values should be 0, 25, 50, 75, and 100. Add
      // <= checks for robustness.
      if (value <= 24)
        return this.i18n('OncTether-SignalStrength_Weak');
      if (value <= 49)
        return this.i18n('OncTether-SignalStrength_Okay');
      if (value <= 74)
        return this.i18n('OncTether-SignalStrength_Good');
      if (value <= 99)
        return this.i18n('OncTether-SignalStrength_Strong');
      return this.i18n('OncTether-SignalStrength_VeryStrong');
    }

    if (key == 'Tether.Carrier') {
      assert(typeof value == 'string');
      return (!value || value == 'unknown-carrier') ?
          this.i18n('tetherUnknownCarrier') :
          value;
    }

    return '';
  },
});
