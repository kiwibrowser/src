// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays information on installed Chrome extensions / apps as well
 * as Winsock layered service providers and namespace providers.
 *
 * For each layered service provider, shows the name, dll, and type
 * information.  For each namespace provider, shows the name and
 * whether or not it's active.
 */
var ModulesView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function ModulesView() {
    assertFirstConstructorCall(ModulesView);

    // Call superclass's constructor.
    superClass.call(this, ModulesView.MAIN_BOX_ID);

    this.serviceProvidersTbody_ = $(ModulesView.SERVICE_PROVIDERS_TBODY_ID);
    this.namespaceProvidersTbody_ = $(ModulesView.NAMESPACE_PROVIDERS_TBODY_ID);

    g_browser.addServiceProvidersObserver(this, false);
    g_browser.addExtensionInfoObserver(this, true);
  }

  ModulesView.TAB_ID = 'tab-handle-modules';
  ModulesView.TAB_NAME = 'Modules';
  ModulesView.TAB_HASH = '#modules';

  // IDs for special HTML elements in modules_view.html.
  ModulesView.MAIN_BOX_ID = 'modules-view-tab-content';
  ModulesView.EXTENSION_INFO_ID = 'modules-view-extension-info';
  ModulesView.WINDOWS_SERVICE_PROVIDERS_ID =
      'modules-view-windows-service-providers';

  cr.addSingletonGetter(ModulesView);

  ModulesView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onLoadLogFinish: function(data) {
      // Show the tab if there are either service providers or extension info.
      var hasExtensionInfo = this.onExtensionInfoChanged(data.extensionInfo);
      var hasSpiInfo = this.onServiceProvidersChanged(data.serviceProviders);
      return hasExtensionInfo || hasSpiInfo;
    },

    onExtensionInfoChanged: function(extensionInfo) {
      var input = new JsEvalContext({extensionInfo: extensionInfo});
      jstProcess(input, $(ModulesView.EXTENSION_INFO_ID));
      return !!extensionInfo;
    },

    onServiceProvidersChanged: function(serviceProviders) {
      var input = new JsEvalContext(serviceProviders);
      jstProcess(input, $(ModulesView.WINDOWS_SERVICE_PROVIDERS_ID));
      return !!serviceProviders;
    },
  };

  /**
   * Returns type of a layered service provider.
   */
  ModulesView.getLayeredServiceProviderType = function(serviceProvider) {
    if (serviceProvider.chain_length == 0)
      return 'Layer';
    if (serviceProvider.chain_length == 1)
      return 'Base';
    return 'Chain';
  };

  var SOCKET_TYPE = {
    '1': 'SOCK_STREAM',
    '2': 'SOCK_DGRAM',
    '3': 'SOCK_RAW',
    '4': 'SOCK_RDM',
    '5': 'SOCK_SEQPACKET'
  };

  /**
   * Returns socket type of a layered service provider as a string.
   */
  ModulesView.getLayeredServiceProviderSocketType = function(serviceProvider) {
    return tryGetValueWithKey(SOCKET_TYPE, serviceProvider.socket_type);
  };

  var PROTOCOL_TYPE = {
    '1': 'IPPROTO_ICMP',
    '6': 'IPPROTO_TCP',
    '17': 'IPPROTO_UDP',
    '58': 'IPPROTO_ICMPV6'
  };

  /**
   * Returns protocol type of a layered service provider as a string.
   */
  ModulesView.getLayeredServiceProviderProtocolType = function(
      serviceProvider) {
    return tryGetValueWithKey(PROTOCOL_TYPE, serviceProvider.socket_protocol);
  };

  var NAMESPACE_PROVIDER_PTYPE = {
    '12': 'NS_DNS',
    '15': 'NS_NLA',
    '16': 'NS_BTH',
    '32': 'NS_NTDS',
    '37': 'NS_EMAIL',
    '38': 'NS_PNRPNAME',
    '39': 'NS_PNRPCLOUD'
  };

  /**
   * Returns the type of a namespace provider as a string.
   */
  ModulesView.getNamespaceProviderType = function(namespaceProvider) {
    return tryGetValueWithKey(NAMESPACE_PROVIDER_PTYPE, namespaceProvider.type);
  };

  return ModulesView;
})();
