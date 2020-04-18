// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview loadTimeData override values for ONC strings used in
 *     network_config.html and other network configuration UI.
 */

var CrOncTest = CrOncTest || {};

CrOncTest.overrideCrOncStrings = function() {
  // From network_element_localized_string_provider.cc:AddOncLocalizedStrings.
  var oncKeys = {
    'OncConnected': 'OncConnected',
    'OncConnecting': 'OncConnecting',
    'OncEAP-AnonymousIdentity': 'OncEAP-AnonymousIdentity',
    'OncEAP-Identity': 'OncEAP-Identity',
    'OncEAP-Inner': 'OncEAP-Inner',
    'OncEAP-Inner_Automatic': 'OncEAP-Inner_Automatic',
    'OncEAP-Inner_CHAP': 'OncEAP-Inner_CHAP',
    'OncEAP-Inner_GTC': 'OncEAP-Inner_GTC',
    'OncEAP-Inner_MD5': 'OncEAP-Inner_MD5',
    'OncEAP-Inner_MSCHAP': 'OncEAP-Inner_MSCHAP',
    'OncEAP-Inner_MSCHAPv2': 'OncEAP-Inner_MSCHAPv2',
    'OncEAP-Inner_PAP': 'OncEAP-Inner_PAP',
    'OncEAP-Outer': 'OncEAP-Outer',
    'OncEAP-Outer_LEAP': 'OncEAP-Outer_LEAP',
    'OncEAP-Outer_PEAP': 'OncEAP-Outer_PEAP',
    'OncEAP-Outer_EAP-TLS': 'OncEAP-Outer_EAP-TLS',
    'OncEAP-Outer_EAP-TTLS': 'OncEAP-Outer_EAP-TTLS',
    'OncEAP-Password': 'OncEAP-Password',
    'OncEAP-ServerCA': 'OncEAP-ServerCA',
    'OncEAP-SubjectMatch': 'OncEAP-SubjectMatch',
    'OncEAP-UserCert': 'OncEAP-UserCert',
    'OncMacAddress': 'OncMacAddress',
    'OncName': 'OncName',
    'OncNotConnected': 'OncNotConnected',
    'OncRestrictedConnectivity': 'OncRestrictedConnectivity',
    'OncTether-BatteryPercentage': 'OncTether-BatteryPercentage',
    'OncTether-BatteryPercentage_Value': 'OncTether-BatteryPercentage_Value',
    'OncTether-SignalStrength': 'OncTether-SignalStrength',
    'OncTether-SignalStrength_Weak': 'OncTether-SignalStrength_Weak',
    'OncTether-SignalStrength_Okay': 'OncTether-SignalStrength_Okay',
    'OncTether-SignalStrength_Good': 'OncTether-SignalStrength_Good',
    'OncTether-SignalStrength_Strong': 'OncTether-SignalStrength_Strong',
    'OncTether-SignalStrength_VeryStrong':
        'OncTether-SignalStrength_VeryStrong',
    'OncTether-Carrier': 'OncTether-Carrier',
    'OncTether-Carrier_Unknown': 'OncTether-Carrier_Unknown',
    'OncVPN-Host': 'OncVPN-Host',
    'OncVPN-IPsec-Group': 'OncVPN-IPsec-Group',
    'OncVPN-IPsec-PSK': 'OncVPN-IPsec-PSK',
    'OncVPN-L2TP-Password': 'OncVPN-L2TP-Password',
    'OncVPN-L2TP-Username': 'OncVPN-L2TP-Username',
    'OncVPN-OpenVPN-OTP': 'OncVPN-OpenVPN-OTP',
    'OncVPN-OpenVPN-Password': 'OncVPN-OpenVPN-Password',
    'OncVPN-OpenVPN-Username': 'OncVPN-OpenVPN-Username',
    'OncVPN-ThirdPartyVPN-ProviderName': 'OncVPN-ThirdPartyVPN-ProviderName',
    'OncVPN-Type': 'OncVPN-Type',
    'OncVPN-Type_L2TP_IPsec': 'OncVPN-Type_L2TP_IPsec',
    'OncVPN-Type_L2TP_IPsec_PSK': 'OncVPN-Type_L2TP_IPsec_PSK',
    'OncVPN-Type_L2TP_IPsec_Cert': 'OncVPN-Type_L2TP_IPsec_Cert',
    'OncVPN-Type_OpenVPN': 'OncVPN-Type_OpenVPN',
    'OncVPN-Type_ARCVPN': 'OncVPN-Type_ARCVPN',
    'OncWiFi-Frequency': 'OncWiFi-Frequency',
    'OncWiFi-Passphrase': 'OncWiFi-Passphrase',
    'OncWiFi-SSID': 'OncWiFi-SSID',
    'OncWiFi-Security': 'OncWiFi-Security',
    'OncWiFi-Security_None': 'OncWiFi-Security_None',
    'OncWiFi-Security_WEP-PSK': 'OncWiFi-Security_WEP-PSK',
    'OncWiFi-Security_WPA-EAP': 'OncWiFi-Security_WPA-EAP',
    'OncWiFi-Security_WPA-PSK': 'OncWiFi-Security_WPA-PSK',
    'OncWiFi-Security_WEP-8021X': 'OncWiFi-Security_WEP-8021X',
    'OncWiFi-SignalStrength': 'OncWiFi-SignalStrength',
    'OncWiMAX-EAP-Identity': 'OncWiMAX-EAP-Identity',
    'Oncipv4-Gateway': 'Oncipv4-Gateway',
    'Oncipv4-IPAddress': 'Oncipv4-IPAddress',
    'Oncipv4-RoutingPrefix': 'Oncipv4-RoutingPrefix',
    'Oncipv6-IPAddress': 'Oncipv6-IPAddress',
  };
  loadTimeData.overrideValues(oncKeys);
};
