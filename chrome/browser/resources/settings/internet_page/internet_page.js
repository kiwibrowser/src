// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-internet-page' is the settings page containing internet
 * settings.
 */
Polymer({
  is: 'settings-internet-page',

  behaviors:
      [I18nBehavior, settings.RouteObserverBehavior, WebUIListenerBehavior],

  properties: {
    /**
     * Interface for networkingPrivate calls. May be overriden by tests.
     * @type {NetworkingPrivate}
     */
    networkingPrivate: {
      type: Object,
      value: chrome.networkingPrivate,
    },

    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * The device state for each network device type. Set by network-summary.
     * @type {!Object<!CrOnc.DeviceStateProperties>|undefined}
     * @private
     */
    deviceStates: {
      type: Object,
      notify: true,
    },

    /**
     * Highest priority connected network or null. Set by network-summary.
     * @type {?CrOnc.NetworkStateProperties|undefined}
     */
    defaultNetwork: {
      type: Object,
      notify: true,
    },

    /**
     * Set by internet-subpage. Controls spinner visibility in subpage header.
     * @private
     */
    showSpinner_: Boolean,

    /**
     * The network type for the networks subpage. Used in the subpage header.
     * @private
     */
    subpageType_: String,

    /**
     * The network type for the known networks subpage.
     * @private
     */
    knownNetworksType_: String,

    /**
     * Whether the 'Add connection' section is expanded.
     * @private
     */
    addConnectionExpanded_: {
      type: Boolean,
      value: false,
    },

    /** @private {!chrome.networkingPrivate.GlobalPolicy|undefined} */
    globalPolicy_: Object,

    /**
     * List of third party VPN providers.
     * @type {!Array<!chrome.networkingPrivate.ThirdPartyVPNProperties>}
     * @private
     */
    thirdPartyVpnProviders_: {
      type: Array,
      value: function() {
        return [];
      }
    },

    /**
     * List of Arc VPN providers.
     * @type {!Array<!settings.ArcVpnProvider>}
     * @private
     */
    arcVpnProviders_: {
      type: Array,
      value: function() {
        return [];
      }
    },

    /** @private {!Map<string, string>} */
    focusConfig_: {
      type: Object,
      value: function() {
        return new Map();
      },
    },
  },

  /** @private {string} Type of last detail page visited. */
  detailType_: '',

  // Element event listeners
  listeners: {
    'device-enabled-toggled': 'onDeviceEnabledToggled_',
    'network-connect': 'onNetworkConnect_',
    'show-config': 'onShowConfig_',
    'show-detail': 'onShowDetail_',
    'show-known-networks': 'onShowKnownNetworks_',
    'show-networks': 'onShowNetworks_',
  },

  // chrome.networkingPrivate listeners
  /** @private {?function(!Array<string>)} */
  networkListChangedListener_: null,

  /** @private {?function(!Array<string>)} */
  networksChangedListener_: null,

  // chrome.management listeners
  /** @private {Function} */
  onExtensionAddedListener_: null,

  /** @private {Function} */
  onExtensionRemovedListener_: null,

  /** @private {Function} */
  onExtensionDisabledListener_: null,

  /** @private  {settings.InternetPageBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.InternetPageBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.browserProxy_.setUpdateArcVpnProvidersCallback(
        this.onArcVpnProvidersReceived_.bind(this));
    this.browserProxy_.requestArcVpnProviders();
  },

  /** @override */
  attached: function() {
    this.networkListChangedListener_ = this.networkListChangedListener_ ||
        this.onNetworkListChanged_.bind(this);
    this.networkingPrivate.onNetworkListChanged.addListener(
        this.networkListChangedListener_);

    this.networksChangedListener_ =
        this.networksChangedListener_ || this.onNetworksChanged_.bind(this);
    this.networkingPrivate.onNetworksChanged.addListener(
        this.networksChangedListener_);

    this.onExtensionAddedListener_ =
        this.onExtensionAddedListener_ || this.onExtensionAdded_.bind(this);
    chrome.management.onInstalled.addListener(this.onExtensionAddedListener_);
    chrome.management.onEnabled.addListener(this.onExtensionAddedListener_);

    this.onExtensionRemovedListener_ =
        this.onExtensionRemovedListener_ || this.onExtensionRemoved_.bind(this);
    chrome.management.onUninstalled.addListener(
        this.onExtensionRemovedListener_);

    this.onExtensionDisabledListener_ = this.onExtensionDisabledListener_ ||
        this.onExtensionDisabled_.bind(this);
    chrome.management.onDisabled.addListener(this.onExtensionDisabledListener_);

    chrome.management.getAll(this.onGetAllExtensions_.bind(this));

    this.networkingPrivate.getGlobalPolicy(policy => {
      this.globalPolicy_ = policy;
    });
  },

  /** @override */
  detached: function() {
    this.networkingPrivate.onNetworkListChanged.removeListener(
        assert(this.networkListChangedListener_));
    this.networkingPrivate.onNetworksChanged.removeListener(
        assert(this.networksChangedListener_));

    chrome.management.onInstalled.removeListener(
        assert(this.onExtensionAddedListener_));
    chrome.management.onEnabled.removeListener(
        assert(this.onExtensionAddedListener_));
    chrome.management.onUninstalled.removeListener(
        assert(this.onExtensionRemovedListener_));
    chrome.management.onDisabled.removeListener(
        assert(this.onExtensionDisabledListener_));
  },

  /**
   * settings.RouteObserverBehavior
   * @param {!settings.Route} route
   * @param {!settings.Route} oldRoute
   * @protected
   */
  currentRouteChanged: function(route, oldRoute) {
    if (route == settings.routes.INTERNET_NETWORKS) {
      // Handle direct navigation to the networks page,
      // e.g. chrome://settings/internet/networks?type=WiFi
      const queryParams = settings.getQueryParameters();
      const type = queryParams.get('type');
      if (type)
        this.subpageType_ = type;
    } else if (route == settings.routes.KNOWN_NETWORKS) {
      // Handle direct navigation to the known networks page,
      // e.g. chrome://settings/internet/knownNetworks?type=WiFi
      const queryParams = settings.getQueryParameters();
      const type = queryParams.get('type');
      if (type)
        this.knownNetworksType_ = type;
    } else if (
        route != settings.routes.INTERNET && route != settings.routes.BASIC) {
      // If we are navigating to a non internet section, do not set focus.
      return;
    }

    if (!settings.routes.INTERNET ||
        !settings.routes.INTERNET.contains(oldRoute))
      return;

    // Focus the subpage arrow where appropriate.
    let selector;
    if (route == settings.routes.INTERNET_NETWORKS) {
      // iron-list makes the correct timing to focus an item in the list
      // very complicated, and the item may not exist, so just focus the
      // entire list for now.
      selector = '* /deep/ #networkList';
    } else if (this.detailType_) {
      selector = '* /deep/ #' + this.detailType_ + ' /deep/ .subpage-arrow';
    }
    if (selector && this.querySelector(selector))
      this.focusConfig_.set(oldRoute.path, selector);
    else
      this.focusConfig_.delete(oldRoute.path);
  },

  /**
   * Event triggered by a device state enabled toggle.
   * @param {!{detail: {enabled: boolean,
   *                    type: chrome.networkingPrivate.NetworkType}}} event
   * @private
   */
  onDeviceEnabledToggled_: function(event) {
    if (event.detail.enabled)
      this.networkingPrivate.enableNetworkType(event.detail.type);
    else
      this.networkingPrivate.disableNetworkType(event.detail.type);
  },

  /**
   * @param {!{detail: !CrOnc.NetworkProperties}} event
   * @private
   */
  onShowConfig_: function(event) {
    const properties = event.detail;
    let configAndConnect = !properties.GUID;  // New configuration
    this.showConfig_(
        configAndConnect, properties.Type, properties.GUID,
        CrOnc.getNetworkName(properties));
  },

  /**
   * @param {boolean} configAndConnect
   * @param {string} type
   * @param {string=} guid
   * @param {string=} name
   * @private
   */
  showConfig_: function(configAndConnect, type, guid, name) {
    const configDialog =
        /** @type {!InternetConfigElement} */ (this.$.configDialog);
    configDialog.type =
        /** @type {chrome.networkingPrivate.NetworkType} */ (type);
    configDialog.guid = guid || '';
    configDialog.name = name || '';
    configDialog.showConnect = configAndConnect;
    configDialog.open();
  },

  /**
   * @param {!{detail: !CrOnc.NetworkStateProperties}} event
   * @private
   */
  onShowDetail_: function(event) {
    this.detailType_ = event.detail.Type;
    const params = new URLSearchParams;
    params.append('guid', event.detail.GUID);
    params.append('type', event.detail.Type);
    if (event.detail.Name)
      params.append('name', event.detail.Name);
    settings.navigateTo(settings.routes.NETWORK_DETAIL, params);
  },

  /**
   * @param {!{detail: {type: string}}} event
   * @private
   */
  onShowNetworks_: function(event) {
    this.showNetworksSubpage_(event.detail.type);
  },

  /**
   * @return {string}
   * @private
   */
  getNetworksPageTitle_: function() {
    return this.i18n('OncType' + this.subpageType_);
  },

  /**
   * @param {string} type
   * @return {string}
   * @private
   */
  getAddNetworkClass_: function(type) {
    if (loadTimeData.getBoolean('networkSettingsConfig')) {
      if (type == CrOnc.Type.WI_FI)
        return 'icon-add-wifi';
      return 'icon-add-circle';
    }
    return 'icon-external';
  },

  /**
   * @param {string} subpageType
   * @param {!Object<!CrOnc.DeviceStateProperties>|undefined} deviceStates
   * @return {!CrOnc.DeviceStateProperties|undefined}
   * @private
   */
  getDeviceState_: function(subpageType, deviceStates) {
    // If both Tether and Cellular are enabled, use the Cellular device state
    // when directly navigating to the Tether page.
    if (subpageType == CrOnc.Type.TETHER &&
        this.deviceStates[CrOnc.Type.CELLULAR]) {
      subpageType = CrOnc.Type.CELLULAR;
    }
    return deviceStates[subpageType];
  },

  /**
   * @param {!{detail: {type: string}}} event
   * @private
   */
  onShowKnownNetworks_: function(event) {
    this.detailType_ = event.detail.type;
    const params = new URLSearchParams;
    params.append('type', event.detail.type);
    this.knownNetworksType_ = event.detail.type;
    settings.navigateTo(settings.routes.KNOWN_NETWORKS, params);
  },

  /**
   * Event triggered when the 'Add connections' div is clicked.
   * @param {!Event} event
   * @private
   */
  onExpandAddConnectionsTap_: function(event) {
    if (event.target.id == 'expandAddConnections')
      return;
    this.addConnectionExpanded_ = !this.addConnectionExpanded_;
  },

  /** @private */
  onAddWiFiTap_: function() {
    if (loadTimeData.getBoolean('networkSettingsConfig'))
      this.showConfig_(true /* configAndConnect */, CrOnc.Type.WI_FI);
    else
      chrome.send('addNetwork', [CrOnc.Type.WI_FI]);
  },

  /** @private */
  onAddVPNTap_: function() {
    if (loadTimeData.getBoolean('networkSettingsConfig'))
      this.showConfig_(true /* configAndConnect */, CrOnc.Type.VPN);
    else
      chrome.send('addNetwork', [CrOnc.Type.VPN]);
  },

  /**
   * @param {!{model:
   *            !{item: !chrome.networkingPrivate.ThirdPartyVPNProperties},
   *        }} event
   * @private
   */
  onAddThirdPartyVpnTap_: function(event) {
    const provider = event.model.item;
    this.browserProxy_.addThirdPartyVpn(CrOnc.Type.VPN, provider.ExtensionID);
  },

  /** @private */
  onAddArcVpnTap_: function() {
    this.showNetworksSubpage_(CrOnc.Type.VPN);
  },

  /**
   * @param {string} type
   * @private
   */
  showNetworksSubpage_: function(type) {
    this.detailType_ = type;
    const params = new URLSearchParams;
    params.append('type', type);
    this.subpageType_ = type;
    settings.navigateTo(settings.routes.INTERNET_NETWORKS, params);
  },

  /**
   * chrome.management.getAll callback.
   * @param {!Array<!chrome.management.ExtensionInfo>} extensions
   * @private
   */
  onGetAllExtensions_: function(extensions) {
    const vpnProviders = [];
    for (let i = 0; i < extensions.length; ++i)
      this.addVpnProvider_(vpnProviders, extensions[i]);
    this.thirdPartyVpnProviders_ = vpnProviders;
  },

  /**
   * If |extension| is a third-party VPN provider, add it to |vpnProviders|.
   * @param {!Array<!chrome.networkingPrivate.ThirdPartyVPNProperties>}
   *     vpnProviders
   * @param {!chrome.management.ExtensionInfo} extension
   * @private
   */
  addVpnProvider_: function(vpnProviders, extension) {
    if (!extension.enabled ||
        extension.permissions.indexOf('vpnProvider') == -1) {
      return;
    }
    if (vpnProviders.find(function(provider) {
          return provider.ExtensionID == extension.id;
        })) {
      return;
    }
    const newProvider = {
      ExtensionID: extension.id,
      ProviderName: extension.name,
    };
    vpnProviders.push(newProvider);
  },

  /**
   * This event is triggered when the list of networks changes.
   * |networkIds| contains the ids for all visible or configured networks.
   * networkingPrivate.onNetworkListChanged event callback.
   * @param {!Array<string>} networkIds
   * @private
   */
  onNetworkListChanged_: function(networkIds) {
    const event = new CustomEvent('network-list-changed', {detail: networkIds});
    this.maybeDispatchEvent_('network-summary', event);
    this.maybeDispatchEvent_('settings-internet-detail-page', event);
    this.maybeDispatchEvent_('settings-internet-known-networks-page', event);
    this.maybeDispatchEvent_('settings-internet-subpage', event);
  },

  /**
   * This event is triggered when interesting properties of a network change.
   * |networkIds| contains the ids for networks whose properties have changed.
   * networkingPrivate.onNetworksChanged event callback.
   * @param {!Array<string>} networkIds
   * @private
   */
  onNetworksChanged_: function(networkIds) {
    const event = new CustomEvent('networks-changed', {detail: networkIds});
    this.maybeDispatchEvent_('network-summary', event);
    this.maybeDispatchEvent_('settings-internet-detail-page', event);
  },

  /**
   * @param {!Event} event
   * @private
   */
  maybeDispatchEvent_: function(identifier, event) {
    const element = this.$$(identifier);
    if (!element)
      return;
    element.dispatchEvent(event);
  },

  /**
   * chrome.management.onInstalled or onEnabled event.
   * @param {!chrome.management.ExtensionInfo} extension
   * @private
   */
  onExtensionAdded_: function(extension) {
    this.addVpnProvider_(this.thirdPartyVpnProviders_, extension);
  },

  /**
   * chrome.management.onUninstalled event.
   * @param {string} extensionId
   * @private
   */
  onExtensionRemoved_: function(extensionId) {
    for (let i = 0; i < this.thirdPartyVpnProviders_.length; ++i) {
      const provider = this.thirdPartyVpnProviders_[i];
      if (provider.ExtensionID == extensionId) {
        this.splice('thirdPartyVpnProviders_', i, 1);
        break;
      }
    }
  },

  /**
   * Compares Arc VPN Providers based on LastlauchTime
   * @param {!settings.ArcVpnProvider} arcVpnProvider1
   * @param {!settings.ArcVpnProvider} arcVpnProvider2
   * @private
   */
  compareArcVpnProviders_: function(arcVpnProvider1, arcVpnProvider2) {
    if (arcVpnProvider1.LastLaunchTime > arcVpnProvider2.LastLaunchTime)
      return -1;
    if (arcVpnProvider1.LastLaunchTime < arcVpnProvider2.LastLaunchTime)
      return 1;
    return 0;
  },

  /**
   * @param {?Array<!settings.ArcVpnProvider>} arcVpnProviders
   * @private
   */
  onArcVpnProvidersReceived_: function(arcVpnProviders) {
    arcVpnProviders.sort(this.compareArcVpnProviders_);
    this.arcVpnProviders_ = arcVpnProviders;
  },

  /**
   * chrome.management.onDisabled event.
   * @param {{id: string}} extension
   * @private
   */
  onExtensionDisabled_: function(extension) {
    this.onExtensionRemoved_(extension.id);
  },

  /**
   * @param {!CrOnc.DeviceStateProperties} deviceState
   * @return {boolean}
   * @private
   */
  deviceIsEnabled_: function(deviceState) {
    return !!deviceState && deviceState.State == CrOnc.DeviceState.ENABLED;
  },

  /**
   * @param {!chrome.networkingPrivate.GlobalPolicy} globalPolicy
   * @return {boolean}
   */
  allowAddConnection_: function(globalPolicy) {
    return !globalPolicy.AllowOnlyPolicyNetworksToConnect;
  },

  /**
   * @param {!chrome.networkingPrivate.ThirdPartyVPNProperties} provider
   * @return {string}
   */
  getAddThirdPartyVpnLabel_: function(provider) {
    return this.i18n('internetAddThirdPartyVPN', provider.ProviderName);
  },

  /**
   * Handles UI requests to connect to a network.
   * TODO(stevenjb): Handle Cellular activation, etc.
   * @param {!{detail:
   *            {networkProperties:
                   (!CrOnc.NetworkProperties|!CrOnc.NetworkStateProperties),
   *             bypassConnectionDialog: (boolean|undefined)}}} event
   * @private
   */
  onNetworkConnect_: function(event) {
    const properties = event.detail.networkProperties;
    const name = CrOnc.getNetworkName(properties);
    if (!event.detail.bypassConnectionDialog &&
        CrOnc.shouldShowTetherDialogBeforeConnection(properties)) {
      const params = new URLSearchParams;
      params.append('guid', properties.GUID);
      params.append('type', properties.Type);
      params.append('name', name);
      params.append('showConfigure', true.toString());

      settings.navigateTo(settings.routes.NETWORK_DETAIL, params);
      return;
    }

    if (properties.Connectable === false || properties.ErrorState) {
      this.showConfig_(
          true /* configAndConnect */, properties.Type, properties.GUID, name);
      return;
    }

    this.networkingPrivate.startConnect(properties.GUID, () => {
      if (chrome.runtime.lastError) {
        const message = chrome.runtime.lastError.message;
        if (message == 'connecting' || message == 'connect-canceled' ||
            message == 'connected' || message == 'Error.InvalidNetworkGuid') {
          return;
        }
        console.error(
            'networkingPrivate.startConnect error: ' + message +
            ' For: ' + properties.GUID);
        this.showConfig_(
            true /* configAndConnect */, properties.Type, properties.GUID,
            name);
      }
    });
  },
});
