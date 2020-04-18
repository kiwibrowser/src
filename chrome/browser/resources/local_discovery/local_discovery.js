// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for local_discovery.html, served from chrome://devices/
 * This is used to show discoverable devices near the user as well as
 * cloud devices registered to them.
 *
 * The object defined in this javascript file listens for callbacks from the
 * C++ code saying that a new device is available as well as manages the UI for
 * registering a device on the local network.
 */

cr.define('local_discovery', function() {
  'use strict';

  // Histogram buckets for UMA tracking.
  /** @const */ var DEVICES_PAGE_EVENTS = {
    OPENED: 0,
    LOG_IN_STARTED_FROM_REGISTER_PROMO: 1,
    LOG_IN_STARTED_FROM_DEVICE_LIST_PROMO: 2,
    ADD_PRINTER_CLICKED: 3,
    REGISTER_CLICKED: 4,
    REGISTER_CONFIRMED: 5,
    REGISTER_SUCCESS: 6,
    REGISTER_CANCEL: 7,
    REGISTER_FAILURE: 8,
    MANAGE_CLICKED: 9,
    REGISTER_CANCEL_ON_PRINTER: 10,
    REGISTER_TIMEOUT: 11,
    LOG_IN_STARTED_FROM_REGISTER_OVERLAY_PROMO: 12,
    MAX_EVENT: 13,
  };

  /**
   * Map of service names to corresponding service objects.
   * @type {Object<string,Service>}
   */
  var devices = {};

  /**
   * Whether or not the user is currently logged in.
   * @type bool
   */
  var isUserLoggedIn = true;

  /**
   * Whether or not the user is supervised or off the record.
   * @type bool
   */
  var isUserSupervisedOrOffTheRecord = false;

  /**
   * Whether or not the path-based dialog has been shown.
   * @type bool
   */
  var dialogFromPathHasBeenShown = false;

  /**
   * Focus manager for page.
   */
  var focusManager = null;

  /**
   * Object that represents a device in the device list.
   * @param {Object} info Information about the device.
   * @constructor
   */
  function Device(info, registerEnabled) {
    this.info = info;
    this.domElement = null;
    this.registerButton = null;
    this.registerEnabled = registerEnabled;
  }

  Device.prototype = {
    /**
     * Update the device.
     * @param {Object} info New information about the device.
     */
    updateDevice: function(info) {
      this.info = info;
      this.renderDevice();
    },

    /**
     * Delete the device.
     */
    removeDevice: function() {
      this.deviceContainer().removeChild(this.domElement);
    },

    /**
     * Render the device to the device list.
     */
    renderDevice: function() {
      if (this.domElement) {
        clearElement(this.domElement);
      } else {
        this.domElement = document.createElement('div');
        this.deviceContainer().appendChild(this.domElement);
      }

      this.registerButton = fillDeviceDescription(
          this.domElement, this.info.display_name, this.info.description,
          this.info.type, loadTimeData.getString('serviceRegister'),
          this.showRegister.bind(this, this.info.type));

      this.setRegisterEnabled(this.registerEnabled);
    },

    /**
     * Return the correct container for the device.
     * @param {boolean} is_mine Whether or not the device is in the 'Registered'
     *    section.
     */
    deviceContainer: function() {
      return $('register-device-list');
    },

    /**
     * Register the device.
     */
    register: function() {
      recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_CONFIRMED);
      chrome.send('registerDevice', [this.info.service_name]);
      setRegisterPage(
          isPrinter(this.info.type) ? 'register-printer-page-adding1' :
                                      'register-device-page-adding1');
    },
    /**
     * Show registrtation UI for device.
     */
    showRegister: function() {
      recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_CLICKED);
      $('register-continue').onclick = this.register.bind(this);

      showRegisterOverlay();
    },
    /**
     * Set registration button enabled/disabled
     */
    setRegisterEnabled: function(isEnabled) {
      this.registerEnabled = isEnabled;
      if (this.registerButton) {
        this.registerButton.disabled = !isEnabled;
      }
    }
  };

  /**
   * Manages focus for local devices page.
   * @constructor
   * @extends {cr.ui.FocusManager}
   */
  function LocalDiscoveryFocusManager() {
    cr.ui.FocusManager.call(this);
    this.focusParent_ = document.body;
  }

  LocalDiscoveryFocusManager.prototype = {
    __proto__: cr.ui.FocusManager.prototype,
    /** @override */
    getFocusParent: function() {
      return document.querySelector('#overlay .showing') || $('main-page');
    }
  };

  /**
   * Returns a textual representation of the number of printers on the network.
   * @return {string} Number of printers on the network as localized string.
   */
  function generateNumberPrintersAvailableText(numberPrinters) {
    if (numberPrinters == 0) {
      return loadTimeData.getString('printersOnNetworkZero');
    } else if (numberPrinters == 1) {
      return loadTimeData.getString('printersOnNetworkOne');
    } else {
      return loadTimeData.getStringF(
          'printersOnNetworkMultiple', numberPrinters);
    }
  }

  /**
   * Fill device element with the description of a device.
   * @param {HTMLElement} device_dom_element Element to be filled.
   * @param {string} name Name of device.
   * @param {string} description Description of device.
   * @param {string} type Type of device.
   * @param {string} button_text Text to appear on button.
   * @param {function()?} button_action Action for button.
   * @return {HTMLElement} The button (for enabling/disabling/rebinding)
   */
  function fillDeviceDescription(
      device_dom_element, name, description, type, button_text, button_action) {
    device_dom_element.classList.add('device');
    if (isPrinter(type))
      device_dom_element.classList.add('printer');

    var deviceInfo = document.createElement('div');
    deviceInfo.className = 'device-info';
    device_dom_element.appendChild(deviceInfo);

    var deviceName = document.createElement('h3');
    deviceName.className = 'device-name';
    deviceName.textContent = name;
    deviceInfo.appendChild(deviceName);

    var deviceDescription = document.createElement('div');
    deviceDescription.className = 'device-subline';
    deviceDescription.textContent = description;
    deviceInfo.appendChild(deviceDescription);

    if (button_action) {
      var button = document.createElement('button');
      button.textContent = button_text;
      button.addEventListener('click', button_action);
      device_dom_element.appendChild(button);
    }

    return button;
  }

  /**
   * Show the register overlay.
   */
  function showRegisterOverlay() {
    recordUmaEvent(DEVICES_PAGE_EVENTS.ADD_PRINTER_CLICKED);

    var registerOverlay = $('register-overlay');
    registerOverlay.classList.add('showing');
    registerOverlay.focus();

    $('overlay').hidden = false;
    setRegisterPage('register-page-confirm');
  }

  /**
   * Hide the register overlay.
   */
  function hideRegisterOverlay() {
    $('register-overlay').classList.remove('showing');
    $('overlay').hidden = true;
  }

  /**
   * Clear a DOM element of all children.
   * @param {HTMLElement} element DOM element to clear.
   */
  function clearElement(element) {
    while (element.firstChild) {
      element.removeChild(element.firstChild);
    }
  }

  /**
   * Announce that a registration failed.
   */
  function onRegistrationFailed() {
    $('error-message').textContent =
        loadTimeData.getString('addingErrorMessage');
    setRegisterPage('register-page-error');
    recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_FAILURE);
  }

  /**
   * Announce that a registration has been canceled on the printer.
   */
  function onRegistrationCanceledPrinter() {
    $('error-message').textContent =
        loadTimeData.getString('addingCanceledMessage');
    setRegisterPage('register-page-error');
    recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_CANCEL_ON_PRINTER);
  }

  /**
   * Announce that a registration has timed out.
   */
  function onRegistrationTimeout() {
    $('error-message').textContent =
        loadTimeData.getString('addingTimeoutMessage');
    setRegisterPage('register-page-error');
    recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_TIMEOUT);
  }

  /**
   * Update UI to reflect that registration has been confirmed on the printer.
   */
  function onRegistrationConfirmedOnPrinter() {
    setRegisterPage('register-printer-page-adding2');
  }

  /**
   * Shows UI to confirm security code.
   * @param {string} code The security code to confirm.
   */
  function onRegistrationConfirmDeviceCode(code) {
    setRegisterPage('register-device-page-adding2');
    $('register-device-page-code').textContent = code;
  }

  /**
   * Update device unregistered device list, and update related strings to
   * reflect the number of devices available to register.
   * @param {string} name Name of the device.
   * @param {string} info Additional info of the device or null if the device
   *                          has been removed.
   */
  function onUnregisteredDeviceUpdate(name, info) {
    if (info) {
      if (devices.hasOwnProperty(name)) {
        devices[name].updateDevice(info);
      } else {
        devices[name] = new Device(info, isUserLoggedIn);
        devices[name].renderDevice();
      }

      if (name == getOverlayIDFromPath() && !dialogFromPathHasBeenShown) {
        dialogFromPathHasBeenShown = true;
        devices[name].showRegister();
      }
    } else {
      if (devices.hasOwnProperty(name)) {
        devices[name].removeDevice();
        delete devices[name];
      }
    }

    updateUIToReflectState();
  }

  /**
   * Create the DOM for a cloud device described by the device section.
   * @param {Array<Object>} devices_list List of devices.
   */
  function createCloudDeviceDOM(device) {
    var devicesDomElement = document.createElement('div');

    var description;
    if (device.description == '') {
      if (isPrinter(device.type))
        description = loadTimeData.getString('noDescriptionPrinter');
      else
        description = loadTimeData.getString('noDescriptionDevice');
    } else {
      description = device.description;
    }

    fillDeviceDescription(
        devicesDomElement, device.display_name, description, device.type,
        loadTimeData.getString('manageDevice'),
        isPrinter(device.type) ? manageCloudDevice.bind(null, device.id) :
                                 null);
    return devicesDomElement;
  }

  /**
   * Handle a list of cloud devices available to the user globally.
   * @param {Array<Object>} devices_list List of devices.
   */
  function onCloudDeviceListAvailable(devices_list) {
    var devicesListLength = devices_list.length;
    var devicesContainer = $('cloud-devices');

    clearElement(devicesContainer);
    $('cloud-devices-loading').hidden = true;

    for (var i = 0; i < devicesListLength; i++) {
      devicesContainer.appendChild(createCloudDeviceDOM(devices_list[i]));
    }
  }

  /**
   * Handle the case where the list of cloud devices is not available.
   */
  function onCloudDeviceListUnavailable() {
    if (isUserLoggedIn) {
      $('cloud-devices-loading').hidden = true;
      $('cloud-devices-unavailable').hidden = false;
    }
  }

  /**
   * Handle the case where the cache for local devices has been flushed..
   */
  function onDeviceCacheFlushed() {
    for (var deviceName in devices) {
      devices[deviceName].removeDevice();
      delete devices[deviceName];
    }

    updateUIToReflectState();
  }

  /**
   * Update UI strings to reflect the number of local devices.
   */
  function updateUIToReflectState() {
    var numberPrinters = $('register-device-list').children.length;
    if (numberPrinters == 0) {
      $('no-printers-message').hidden = false;

      $('register-login-promo').hidden = true;
    } else {
      $('no-printers-message').hidden = true;
      $('register-login-promo').hidden =
          isUserLoggedIn || isUserSupervisedOrOffTheRecord;
    }
    if (!($('register-login-promo').hidden) ||
        !($('cloud-devices-login-promo').hidden) ||
        !($('register-overlay-login-promo').hidden)) {
      chrome.send(
          'metricsHandler:recordAction', ['Signin_Impression_FromDevicesPage']);
    }
  }

  /**
   * Announce that a registration succeeeded.
   */
  function onRegistrationSuccess(device_data) {
    hideRegisterOverlay();

    if (device_data.service_name == getOverlayIDFromPath()) {
      window.close();
    }

    var deviceDOM = createCloudDeviceDOM(device_data);
    $('cloud-devices').insertBefore(deviceDOM, $('cloud-devices').firstChild);
    recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_SUCCESS);
  }

  /**
   * Update visibility status for page.
   */
  function updateVisibility() {
    chrome.send('isVisible', [!document.hidden]);
  }

  /**
   * Set the page that the register wizard is on.
   * @param {string} page_id ID string for page.
   */
  function setRegisterPage(page_id) {
    var pages = $('register-overlay').querySelectorAll('.register-page');
    var pagesLength = pages.length;
    for (var i = 0; i < pagesLength; i++) {
      pages[i].hidden = true;
    }

    $(page_id).hidden = false;
  }

  /**
   * Request the device list.
   */
  function requestDeviceList() {
    if (isUserLoggedIn) {
      clearElement($('cloud-devices'));
      $('cloud-devices-loading').hidden = false;
      $('cloud-devices-unavailable').hidden = true;

      chrome.send('requestDeviceList');
    }
  }

  /**
   * Go to management page for a cloud device.
   * @param {string} device_id ID of device.
   */
  function manageCloudDevice(device_id) {
    recordUmaEvent(DEVICES_PAGE_EVENTS.MANAGE_CLICKED);
    chrome.send('openCloudPrintURL', [device_id]);
  }

  /**
   * Record an event in the UMA histogram.
   * @param {number} eventId The id of the event to be recorded.
   * @private
   */
  function recordUmaEvent(eventId) {
    chrome.send(
        'metricsHandler:recordInHistogram',
        ['LocalDiscovery.DevicesPage', eventId, DEVICES_PAGE_EVENTS.MAX_EVENT]);
  }

  /**
   * Cancel the registration.
   */
  function cancelRegistration() {
    hideRegisterOverlay();
    chrome.send('cancelRegistration');
    recordUmaEvent(DEVICES_PAGE_EVENTS.REGISTER_CANCEL);
  }

  /**
   * Confirms device code.
   */
  function confirmCode() {
    chrome.send('confirmCode');
    setRegisterPage('register-device-page-adding1');
  }

  /**
   * Retry loading the devices from Google Cloud Print.
   */
  function retryLoadCloudDevices() {
    requestDeviceList();
  }

  /**
   * User is not logged in.
   */
  function setUserLoggedIn(userLoggedIn, userSupervisedOrOffTheRecord) {
    isUserLoggedIn = userLoggedIn;
    isUserSupervisedOrOffTheRecord = userSupervisedOrOffTheRecord;

    $('cloud-devices-login-promo').hidden =
        isUserLoggedIn || isUserSupervisedOrOffTheRecord;
    $('register-overlay-login-promo').hidden =
        isUserLoggedIn || isUserSupervisedOrOffTheRecord;
    $('register-continue').disabled =
        !isUserLoggedIn || isUserSupervisedOrOffTheRecord;

    $('my-devices-container').hidden = userSupervisedOrOffTheRecord;

    if (isUserSupervisedOrOffTheRecord) {
      $('cloud-print-connector-section').hidden = true;
    }

    if (isUserLoggedIn && !isUserSupervisedOrOffTheRecord) {
      requestDeviceList();
      $('register-login-promo').hidden = true;
    } else {
      $('cloud-devices-loading').hidden = true;
      $('cloud-devices-unavailable').hidden = true;
      clearElement($('cloud-devices'));
      hideRegisterOverlay();
    }

    updateUIToReflectState();

    for (var device in devices) {
      devices[device].setRegisterEnabled(isUserLoggedIn);
    }
  }

  function openSignInPage() {
    chrome.send('showSyncUI');
  }

  function registerLoginButtonClicked() {
    recordUmaEvent(DEVICES_PAGE_EVENTS.LOG_IN_STARTED_FROM_REGISTER_PROMO);
    openSignInPage();
  }

  function registerOverlayLoginButtonClicked() {
    recordUmaEvent(
        DEVICES_PAGE_EVENTS.LOG_IN_STARTED_FROM_REGISTER_OVERLAY_PROMO);
    openSignInPage();
  }

  function cloudDevicesLoginButtonClicked() {
    recordUmaEvent(DEVICES_PAGE_EVENTS.LOG_IN_STARTED_FROM_DEVICE_LIST_PROMO);
    openSignInPage();
  }

  /**
   * Set the Cloud Print proxy UI to enabled, disabled, or processing.
   * @private
   */
  function setupCloudPrintConnectorSection(disabled, label, allowed) {
    if (!cr.isChromeOS) {
      $('cloudPrintConnectorLabel').textContent = label;
      if (disabled || !allowed) {
        $('cloudPrintConnectorSetupButton').textContent =
            loadTimeData.getString('cloudPrintConnectorDisabledButton');
      } else {
        $('cloudPrintConnectorSetupButton').textContent =
            loadTimeData.getString('cloudPrintConnectorEnabledButton');
      }
      $('cloudPrintConnectorSetupButton').disabled = !allowed;

      if (disabled) {
        $('cloudPrintConnectorSetupButton').onclick = function(event) {
          // Disable the button, set its text to the intermediate state.
          $('cloudPrintConnectorSetupButton').textContent =
              loadTimeData.getString('cloudPrintConnectorEnablingButton');
          $('cloudPrintConnectorSetupButton').disabled = true;
          chrome.send('showCloudPrintSetupDialog');
        };
      } else {
        $('cloudPrintConnectorSetupButton').onclick = function(event) {
          chrome.send('disableCloudPrintConnector');
          requestDeviceList();
        };
      }
    }
  }

  function getOverlayIDFromPath() {
    if (document.location.pathname == '/register') {
      var params = parseQueryParams(document.location);
      return params['id'] || null;
    }
  }

  /**
   * Returns true of device is printer.
   * @param {string} type Type of printer.
   */
  function isPrinter(type) {
    return type == 'printer';
  }

  document.addEventListener('DOMContentLoaded', function() {
    cr.ui.overlay.setupOverlay($('overlay'));
    cr.ui.overlay.globalInitialization();
    $('overlay').addEventListener('cancelOverlay', cancelRegistration);

    [].forEach.call(
        document.querySelectorAll('.register-cancel'), function(button) {
          button.addEventListener('click', cancelRegistration);
        });

    [].forEach.call(
        document.querySelectorAll('.confirm-code'), function(button) {
          button.addEventListener('click', confirmCode);
        });

    $('register-error-exit').addEventListener('click', cancelRegistration);


    $('cloud-devices-retry-link')
        .addEventListener('click', retryLoadCloudDevices);

    $('cloud-devices-login-link')
        .addEventListener('click', cloudDevicesLoginButtonClicked);

    $('register-login-link')
        .addEventListener('click', registerLoginButtonClicked);

    $('register-overlay-login-button')
        .addEventListener('click', registerOverlayLoginButtonClicked);

    if (loadTimeData.valueExists('backButtonURL')) {
      $('back-link').hidden = false;
      $('back-link').addEventListener('click', function() {
        window.location.href = loadTimeData.getString('backButtonURL');
      });
    }

    updateVisibility();
    document.addEventListener('visibilitychange', updateVisibility, false);

    focusManager = new LocalDiscoveryFocusManager();
    focusManager.initialize();

    chrome.send('start');
    recordUmaEvent(DEVICES_PAGE_EVENTS.OPENED);
  });

  return {
    onRegistrationSuccess: onRegistrationSuccess,
    onRegistrationFailed: onRegistrationFailed,
    onUnregisteredDeviceUpdate: onUnregisteredDeviceUpdate,
    onRegistrationConfirmedOnPrinter: onRegistrationConfirmedOnPrinter,
    onRegistrationConfirmDeviceCode: onRegistrationConfirmDeviceCode,
    onCloudDeviceListAvailable: onCloudDeviceListAvailable,
    onCloudDeviceListUnavailable: onCloudDeviceListUnavailable,
    onDeviceCacheFlushed: onDeviceCacheFlushed,
    onRegistrationCanceledPrinter: onRegistrationCanceledPrinter,
    onRegistrationTimeout: onRegistrationTimeout,
    setUserLoggedIn: setUserLoggedIn,
    setupCloudPrintConnectorSection: setupCloudPrintConnectorSection,
  };
});
