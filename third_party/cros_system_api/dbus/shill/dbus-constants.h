// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SHILL_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SHILL_DBUS_CONSTANTS_H_

// TODO(benchan): Reorganize shill constants and remove deprecated ones.
namespace shill {
// Flimflam D-Bus service identifiers.
const char kFlimflamManagerInterface[] = "org.chromium.flimflam.Manager";
const char kFlimflamServiceName[] = "org.chromium.flimflam";
const char kFlimflamServicePath[] = "/";  // crosbug.com/20135
const char kFlimflamServiceInterface[] = "org.chromium.flimflam.Service";
const char kFlimflamIPConfigInterface[] = "org.chromium.flimflam.IPConfig";
const char kFlimflamDeviceInterface[] = "org.chromium.flimflam.Device";
const char kFlimflamProfileInterface[] = "org.chromium.flimflam.Profile";
const char kFlimflamNetworkInterface[] = "org.chromium.flimflam.Network";
const char kFlimflamThirdPartyVpnInterface[] =
    "org.chromium.flimflam.ThirdPartyVpn";

// Flimflam function names.
const char kGetPropertiesFunction[] = "GetProperties";
const char kSetPropertyFunction[] = "SetProperty";
const char kClearPropertyFunction[] = "ClearProperty";
const char kConnectFunction[] = "Connect";
const char kDisconnectFunction[] = "Disconnect";
const char kRequestScanFunction[] = "RequestScan";
const char kGetServiceFunction[] = "GetService";
const char kRemoveServiceFunction[] = "Remove";
const char kEnableTechnologyFunction[] = "EnableTechnology";
const char kDisableTechnologyFunction[] = "DisableTechnology";
const char kRemoveConfigFunction[] = "Remove";
const char kGetEntryFunction[] = "GetEntry";
const char kDeleteEntryFunction[] = "DeleteEntry";
const char kActivateCellularModemFunction[] = "ActivateCellularModem";
const char kRequirePinFunction[] = "RequirePin";
const char kEnterPinFunction[] = "EnterPin";
const char kUnblockPinFunction[] = "UnblockPin";
const char kChangePinFunction[] = "ChangePin";
const char kRegisterFunction[] = "Register";
const char kConfigureServiceFunction[] = "ConfigureService";
const char kConfigureWifiServiceFunction[] = "ConfigureWifiService";
const char kFindMatchingServiceFunction[] = "FindMatchingService";
const char kSetNetworkThrottlingFunction[] = "SetNetworkThrottlingStatus";

// Flimflam Service property names.
const char kSecurityProperty[] = "Security";
const char kPriorityProperty[] = "Priority";
const char kPriorityWithinTechnologyProperty[] = "PriorityWithinTechnology";
const char kPassphraseProperty[] = "Passphrase";
const char kIdentityProperty[] = "Identity";
const char kAuthorityPathProperty[] = "AuthorityPath";
const char kPassphraseRequiredProperty[] = "PassphraseRequired";
const char kSaveCredentialsProperty[] = "SaveCredentials";
const char kSignalStrengthProperty[] = "Strength";
const char kNameProperty[] = "Name";
const char kGuidProperty[] = "GUID";
const char kStateProperty[] = "State";
const char kTypeProperty[] = "Type";
const char kDeviceProperty[] = "Device";
const char kProfileProperty[] = "Profile";
const char kConnectivityStateProperty[] = "ConnectivityState";
const char kConnectableProperty[] = "Connectable";
const char kAutoConnectProperty[] = "AutoConnect";
const char kIsActiveProperty[] = "IsActive";
const char kModeProperty[] = "Mode";
const char kErrorProperty[] = "Error";
const char kProviderProperty[] = "Provider";
const char kHostProperty[] = "Host";
const char kDomainProperty[] = "Domain";
const char kProxyConfigProperty[] = "ProxyConfig";
const char kCheckPortalProperty[] = "CheckPortal";
const char kSSIDProperty[] = "SSID";
const char kConnectedProperty[] = "Connected";
const char kUIDataProperty[] = "UIData";
const char kConnectionIdProperty[] = "ConnectionId";
const char kVisibleProperty[] = "Visible";
const char kDnsAutoFallbackProperty[] = "DNSAutoFallback";
const char kPortalDetectionFailedPhaseProperty[] =
    "PortalDetectionFailedPhase";
const char kPortalDetectionFailedStatusProperty[] =
    "PortalDetectionFailedStatus";
const char kSavedIPConfigProperty[] = "SavedIPConfig";
const char kStaticIPConfigProperty[] = "StaticIPConfig";
const char kLinkMonitorDisableProperty[] = "LinkMonitorDisable";
const char kSecurityClassProperty[] = "SecurityClass";

// Flimflam provider property names.
const char kProviderHostProperty[] = "Provider.Host";
const char kProviderNameProperty[] = "Provider.Name";
const char kProviderTypeProperty[] = "Provider.Type";

// Flimflam Wifi Service property names.
const char kWifiBSsid[] = "WiFi.BSSID";
const char kWifiHexSsid[] = "WiFi.HexSSID";
const char kWifiFrequency[] = "WiFi.Frequency";
const char kWifiHiddenSsid[] = "WiFi.HiddenSSID";
const char kWifiPhyMode[] = "WiFi.PhyMode";
const char kWifiAuthMode[] = "WiFi.AuthMode";
const char kWifiChannelProperty[] = "WiFi.Channel";
const char kWifiPreferredDeviceProperty[] = "WiFi.PreferredDevice";
const char kWifiRoamThresholdProperty[] = "WiFi.RoamThreshold";
const char kWifiFTEnabled[] = "WiFi.FTEnabled";

// Flimflam EAP property names.
const char kEapIdentityProperty[] = "EAP.Identity";
const char kEapMethodProperty[] = "EAP.EAP";
const char kEapPhase2AuthProperty[] = "EAP.InnerEAP";
const char kEapTLSVersionMaxProperty[] = "EAP.TLSVersionMax";
const char kEapAnonymousIdentityProperty[] = "EAP.AnonymousIdentity";
const char kEapClientCertProperty[] = "EAP.ClientCert";
const char kEapCertIdProperty[] = "EAP.CertID";
const char kEapClientCertNssProperty[] = "EAP.ClientCertNSS";
const char kEapPrivateKeyProperty[] = "EAP.PrivateKey";
const char kEapPrivateKeyPasswordProperty[] = "EAP.PrivateKeyPassword";
const char kEapKeyIdProperty[] = "EAP.KeyID";
const char kEapCaCertProperty[] = "EAP.CACert";
const char kEapCaCertIdProperty[] = "EAP.CACertID";
const char kEapCaCertNssProperty[] = "EAP.CACertNSS";
const char kEapUseSystemCasProperty[] = "EAP.UseSystemCAs";
const char kEapUseProactiveKeyCachingProperty[] = "EAP.UseProactiveKeyCaching";
const char kEapPinProperty[] = "EAP.PIN";
const char kEapPasswordProperty[] = "EAP.Password";
const char kEapKeyMgmtProperty[] = "EAP.KeyMgmt";
const char kEapUseLoginPasswordProperty[] = "EAP.UseLoginPassword";

// Flimflam Cellular Service property names.
const char kTechnologyFamilyProperty[] = "Cellular.Family";
const char kActivationStateProperty[] = "Cellular.ActivationState";
const char kNetworkTechnologyProperty[] = "Cellular.NetworkTechnology";
const char kRoamingStateProperty[] = "Cellular.RoamingState";
const char kOperatorNameProperty[] = "Cellular.OperatorName";
const char kOperatorCodeProperty[] = "Cellular.OperatorCode";
const char kServingOperatorProperty[] = "Cellular.ServingOperator";
const char kPaymentPortalProperty[] = "Cellular.Olp";
const char kUsageURLProperty[] = "Cellular.UsageUrl";
const char kCellularApnProperty[] = "Cellular.APN";
const char kCellularLastGoodApnProperty[] = "Cellular.LastGoodAPN";
const char kCellularApnListProperty[] = "Cellular.APNList";

// Flimflam Manager property names.
const char kProfilesProperty[] = "Profiles";
const char kServicesProperty[] = "Services";
const char kServiceWatchListProperty[] = "ServiceWatchList";
const char kAvailableTechnologiesProperty[] = "AvailableTechnologies";
const char kEnabledTechnologiesProperty[] = "EnabledTechnologies";
const char kConnectedTechnologiesProperty[] = "ConnectedTechnologies";
const char kDefaultTechnologyProperty[] = "DefaultTechnology";
const char kOfflineModeProperty[] = "OfflineMode";
const char kActiveProfileProperty[] = "ActiveProfile";
const char kDevicesProperty[] = "Devices";
const char kCheckPortalListProperty[] = "CheckPortalList";
const char kArpGatewayProperty[] = "ArpGateway";
const char kCountryProperty[] = "Country";
const char kPortalURLProperty[] = "PortalURL";
const char kConnectionStateProperty[] = "ConnectionState";
const char kClaimedDevicesProperty[] = "ClaimedDevices";

// Flimflam Profile property names.
const char kEntriesProperty[] = "Entries";

// Flimflam Device property names.
const char kScanningProperty[] = "Scanning";
const char kPoweredProperty[] = "Powered";
const char kScanIntervalProperty[] = "ScanInterval";
const char kBgscanMethodProperty[] = "BgscanMethod";
const char kBgscanShortIntervalProperty[] = "BgscanShortInterval";
const char kRoamThresholdProperty[] = "RoamThreshold";
const char kDBusObjectProperty[] = "DBus.Object";
const char kDBusServiceProperty[] = "DBus.Service";
const char kBgscanSignalThresholdProperty[] = "BgscanSignalThreshold";
const char kWakeToScanPeriodSecondsProperty[] = "WakeToScanPeriodSeconds";
const char kNetDetectScanPeriodSecondsProperty[] = "NetDetectScanPeriodSeconds";
const char kForceWakeToScanTimerProperty[] = "ForceWakeToScanTimer";
// The name of the network interface, ie. wlan0, eth0, etc.
const char kInterfaceProperty[] = "Interface";
const char kSelectedServiceProperty[] = "SelectedService";
const char kIPConfigsProperty[] = "IPConfigs";
const char kMACAddressRandomizationSupportedProperty[] =
    "MACAddressRandomizationSupported";
const char kMACAddressRandomizationEnabledProperty[] =
    "MACAddressRandomizationEnabled";

// Flimflam Cellular Device property names.
const char kCarrierProperty[] = "Cellular.Carrier";
const char kCellularAllowRoamingProperty[] = "Cellular.AllowRoaming";
const char kHomeProviderProperty[] = "Cellular.HomeProvider";
const char kMeidProperty[] = "Cellular.MEID";
const char kImeiProperty[] = "Cellular.IMEI";
const char kIccidProperty[] = "Cellular.ICCID";
const char kImsiProperty[] = "Cellular.IMSI";
const char kEsnProperty[] = "Cellular.ESN";
const char kMdnProperty[] = "Cellular.MDN";
const char kMinProperty[] = "Cellular.MIN";
const char kModelIdProperty[] = "Cellular.ModelID";
const char kEquipmentIdProperty[] = "Cellular.EquipmentID";
const char kManufacturerProperty[] = "Cellular.Manufacturer";
const char kFirmwareRevisionProperty[] = "Cellular.FirmwareRevision";
const char kHardwareRevisionProperty[] = "Cellular.HardwareRevision";
const char kDeviceIdProperty[] = "Cellular.DeviceID";
const char kPRLVersionProperty[] = "Cellular.PRLVersion";
const char kSelectedNetworkProperty[] = "Cellular.SelectedNetwork";
const char kSupportNetworkScanProperty[] = "Cellular.SupportNetworkScan";
const char kFoundNetworksProperty[] = "Cellular.FoundNetworks";

// Flimflam state options.
const char kStateIdle[] = "idle";
const char kStateCarrier[] = "carrier";
const char kStateAssociation[] = "association";
const char kStateConfiguration[] = "configuration";
const char kStateReady[] = "ready";
const char kStatePortal[] = "portal";
const char kStateOffline[] = "offline";
const char kStateOnline[] = "online";
const char kStateDisconnect[] = "disconnect";
const char kStateFailure[] = "failure";
const char kStateActivationFailure[] = "activation-failure";

// Flimflam portal phase and status.
const char kPortalDetectionPhaseConnection[] = "Connection";
const char kPortalDetectionPhaseDns[] = "DNS";
const char kPortalDetectionPhaseHttp[] = "HTTP";
const char kPortalDetectionPhaseContent[] = "Content";
const char kPortalDetectionPhaseUnknown[] = "Unknown";
const char kPortalDetectionStatusFailure[] = "Failure";
const char kPortalDetectionStatusTimeout[] = "Timeout";
const char kPortalDetectionStatusSuccess[] = "Success";

// Flimflam property names for SIMLock status.
const char kSIMLockStatusProperty[] = "Cellular.SIMLockStatus";
const char kSIMLockTypeProperty[] = "LockType";
const char kSIMLockRetriesLeftProperty[] = "RetriesLeft";
const char kSIMLockEnabledProperty[] = "LockEnabled";

// Flimflam property names for Cellular.FoundNetworks.
const char kLongNameProperty[] = "long_name";
const char kStatusProperty[] = "status";
const char kShortNameProperty[] = "short_name";
const char kTechnologyProperty[] = "technology";
const char kNetworkIdProperty[] = "network_id";

// Flimflam SIMLock status types.
const char kSIMLockPin[] = "sim-pin";
const char kSIMLockPuk[] = "sim-puk";

// APN info property names.
const char kApnProperty[] = "apn";
const char kApnNetworkIdProperty[] = "network_id";
const char kApnUsernameProperty[] = "username";
const char kApnPasswordProperty[] = "password";
const char kApnNameProperty[] = "name";
const char kApnLocalizedNameProperty[] = "localized_name";
const char kApnLanguageProperty[] = "language";
const char kApnAuthenticationProperty[] = "authentication";

// APN authentication property values (as expected by ModemManager).
const char kApnAuthenticationPap[] = "pap";
const char kApnAuthenticationChap[] = "chap";

// Payment Portal property names.
const char kPaymentPortalURL[] = "url";
const char kPaymentPortalMethod[] = "method";
const char kPaymentPortalPostData[] = "postdata";

// Operator info property names.
const char kOperatorNameKey[] = "name";
const char kOperatorCodeKey[] = "code";
const char kOperatorCountryKey[] = "country";
const char kOperatorUuidKey[] = "uuid";

// Flimflam network technology options.
const char kNetworkTechnology1Xrtt[] = "1xRTT";
const char kNetworkTechnologyEvdo[] = "EVDO";
const char kNetworkTechnologyGsm[] = "GSM";
const char kNetworkTechnologyGprs[] = "GPRS";
const char kNetworkTechnologyEdge[] = "EDGE";
const char kNetworkTechnologyUmts[] = "UMTS";
const char kNetworkTechnologyHspa[] = "HSPA";
const char kNetworkTechnologyHspaPlus[] = "HSPA+";
const char kNetworkTechnologyLte[] = "LTE";
const char kNetworkTechnologyLteAdvanced[] = "LTE Advanced";

// Flimflam roaming state options
const char kRoamingStateHome[] = "home";
const char kRoamingStateRoaming[] = "roaming";
const char kRoamingStateUnknown[] = "unknown";

// Flimflam activation state options
const char kActivationStateActivated[] = "activated";
const char kActivationStateActivating[] = "activating";
const char kActivationStateNotActivated[] = "not-activated";
const char kActivationStatePartiallyActivated[] = "partially-activated";
const char kActivationStateUnknown[] = "unknown";

// Flimflam EAP method options.
const char kEapMethodPEAP[] = "PEAP";
const char kEapMethodTLS[] = "TLS";
const char kEapMethodTTLS[] = "TTLS";
const char kEapMethodLEAP[] = "LEAP";

// Flimflam EAP phase 2 auth options.
const char kEapPhase2AuthPEAPMD5[] = "auth=MD5";
const char kEapPhase2AuthPEAPMSCHAPV2[] = "auth=MSCHAPV2";
const char kEapPhase2AuthPEAPGTC[] = "auth=GTC";
const char kEapPhase2AuthTTLSMD5[] = "autheap=MD5";  // crosbug/26822
const char kEapPhase2AuthTTLSEAPMD5[] = "autheap=MD5";
const char kEapPhase2AuthTTLSEAPMSCHAPV2[] = "autheap=MSCHAPV2";
const char kEapPhase2AuthTTLSMSCHAPV2[] = "auth=MSCHAPV2";
const char kEapPhase2AuthTTLSMSCHAP[] = "auth=MSCHAP";
const char kEapPhase2AuthTTLSPAP[] = "auth=PAP";
const char kEapPhase2AuthTTLSCHAP[] = "auth=CHAP";
const char kEapPhase2AuthTTLSGTC[] = "auth=GTC";
const char kEapPhase2AuthTTLSEAPGTC[] = "autheap=GTC";

// Flimflam EAP TLS versions.
const char kEapTLSVersion1p0[] = "1.0";
const char kEapTLSVersion1p1[] = "1.1";
const char kEapTLSVersion1p2[] = "1.2";

// Flimflam VPN provider types.
const char kProviderL2tpIpsec[] = "l2tpipsec";
const char kProviderOpenVpn[] = "openvpn";
const char kProviderThirdPartyVpn[] = "thirdpartyvpn";
const char kProviderArcVpn[] = "arcvpn";

// Flimflam VPN service properties
const char kVPNDomainProperty[] = "VPN.Domain";

// Flimflam monitored properties
const char kMonitorPropertyChanged[] = "PropertyChanged";

// Flimflam type options.
const char kTypeEthernet[] = "ethernet";
const char kTypeWifi[] = "wifi";
const char kTypeWimax[] = "wimax";
const char kTypeBluetooth[] = "bluetooth";
const char kTypeCellular[] = "cellular";
const char kTypeVPN[] = "vpn";
const char kTypePPPoE[] = "pppoe";

// Flimflam mode options.
const char kModeManaged[] = "managed";
const char kModeAdhoc[] = "adhoc";

// Flimflam security options.
const char kSecurityWpa[] = "wpa";
const char kSecurityWep[] = "wep";
const char kSecurityRsn[] = "rsn";
const char kSecurity8021x[] = "802_1x";
const char kSecurityPsk[] = "psk";
const char kSecurityNone[] = "none";

// Flimflam L2TPIPsec property names.
const char kL2tpIpsecAuthenticationType[] = "L2TPIPsec.AuthenticationType";
const char kL2tpIpsecCaCertNssProperty[] = "L2TPIPsec.CACertNSS";
const char kL2tpIpsecClientCertIdProperty[] = "L2TPIPsec.ClientCertID";
const char kL2tpIpsecClientCertSlotProperty[] = "L2TPIPsec.ClientCertSlot";
const char kL2tpIpsecIkeVersion[] = "L2TPIPsec.IKEVersion";
const char kL2tpIpsecPinProperty[] = "L2TPIPsec.PIN";
const char kL2tpIpsecPskProperty[] = "L2TPIPsec.PSK";
const char kL2tpIpsecPskRequiredProperty[] = "L2TPIPsec.PSKRequired";
const char kL2tpIpsecUserProperty[] = "L2TPIPsec.User";
const char kL2tpIpsecPasswordProperty[] = "L2TPIPsec.Password";

// Flimflam OpenVPN property names.
const char kOpenVPNAuthNoCacheProperty[] = "OpenVPN.AuthNoCache";
const char kOpenVPNAuthProperty[] = "OpenVPN.Auth";
const char kOpenVPNAuthRetryProperty[] = "OpenVPN.AuthRetry";
const char kOpenVPNAuthUserPassProperty[] = "OpenVPN.AuthUserPass";
const char kOpenVPNCaCertProperty[] = "OpenVPN.CACert";
const char kOpenVPNCaCertNSSProperty[] = "OpenVPN.CACertNSS";
const char kOpenVPNClientCertIdProperty[] = "OpenVPN.Pkcs11.ID";
const char kOpenVPNClientCertSlotProperty[] = "OpenVPN.Pkcs11.Slot";
const char kOpenVPNCipherProperty[] = "OpenVPN.Cipher";
const char kOpenVPNCompLZOProperty[] = "OpenVPN.CompLZO";
const char kOpenVPNCompNoAdaptProperty[] = "OpenVPN.CompNoAdapt";
const char kOpenVPNIgnoreDefaultRouteProperty[] = "OpenVPN.IgnoreDefaultRoute";
const char kOpenVPNKeyDirectionProperty[] = "OpenVPN.KeyDirection";
const char kOpenVPNMgmtEnableProperty[] = "OpenVPN.Mgmt.Enable";
const char kOpenVPNNsCertTypeProperty[] = "OpenVPN.NsCertType";
const char kOpenVPNOTPProperty[] = "OpenVPN.OTP";
const char kOpenVPNPasswordProperty[] = "OpenVPN.Password";
const char kOpenVPNPinProperty[] = "OpenVPN.Pkcs11.PIN";
const char kOpenVPNPortProperty[] = "OpenVPN.Port";
const char kOpenVPNProtoProperty[] = "OpenVPN.Proto";
const char kOpenVPNProviderProperty[] = "OpenVPN.Pkcs11.Provider";
const char kOpenVPNPushPeerInfoProperty[] = "OpenVPN.PushPeerInfo";
const char kOpenVPNRemoteCertEKUProperty[] = "OpenVPN.RemoteCertEKU";
const char kOpenVPNRemoteCertKUProperty[] = "OpenVPN.RemoteCertKU";
const char kOpenVPNRemoteCertTLSProperty[] = "OpenVPN.RemoteCertTLS";
const char kOpenVPNRenegSecProperty[] = "OpenVPN.RenegSec";
const char kOpenVPNServerPollTimeoutProperty[] = "OpenVPN.ServerPollTimeout";
const char kOpenVPNShaperProperty[] = "OpenVPN.Shaper";
const char kOpenVPNStaticChallengeProperty[] = "OpenVPN.StaticChallenge";
const char kOpenVPNTLSAuthContentsProperty[] = "OpenVPN.TLSAuthContents";
const char kOpenVPNTLSRemoteProperty[] = "OpenVPN.TLSRemote";
const char kOpenVPNUserProperty[] = "OpenVPN.User";

// Flimflam ARCVPN property names.
const char kArcVpnTunnelChromeProperty[] = "ArcVpn.TunnelChrome";

// FlimFlam PPPoE property names.
const char kPPPoEUsernameProperty[] = "PPPoE.Username";
const char kPPPoEPasswordProperty[] = "PPPoE.Password";
const char kPPPoELCPEchoIntervalProperty[] = "PPPoE.LCPEchoInterval";
const char kPPPoELCPEchoFailureProperty[] = "PPPoE.LCPEchoFailure";
const char kPPPoEMaxAuthFailureProperty[] = "PPPoE.MaxAuthFailure";

// FlimFlam technology family options
const char kTechnologyFamilyCdma[] = "CDMA";
const char kTechnologyFamilyGsm[] = "GSM";

// IPConfig property names.
const char kMethodProperty[] = "Method";
const char kAddressProperty[] = "Address";
const char kMtuProperty[] = "Mtu";
const char kPrefixlenProperty[] = "Prefixlen";
const char kBroadcastProperty[] = "Broadcast";
const char kPeerAddressProperty[] = "PeerAddress";
const char kGatewayProperty[] = "Gateway";
const char kDomainNameProperty[] = "DomainName";
const char kAcceptedHostnameProperty[] = "AcceptedHostname";
const char kNameServersProperty[] = "NameServers";
const char kSearchDomainsProperty[] = "SearchDomains";
const char kDhcpv6AddressesProperty[] = "Dhcpv6Addresses";
const char kDhcpv6DelegatedPrefixesProperty[] = "Dhcpv6DelegatedPrefixes";
const char kLeaseDurationSecondsProperty[] = "LeaseDurationSeconds";
const char kVendorEncapsulatedOptionsProperty[] = "VendorEncapsulatedOptions";
const char kWebProxyAutoDiscoveryUrlProperty[] = "WebProxyAutoDiscoveryUrl";
// DHCP Option for iSNS (RFC 4174)
const char kiSNSOptionDataProperty[] = "iSNSOptionData";
const char kIncludedRoutesProperty[] = "IncludedRoutes";
const char kExcludedRoutesProperty[] = "ExcludedRoutes";

// These constants are deprecated in favor of kDhcpv6DelegatedPrefixesProperty.
// TODO(tjennison): Remove when shill no longer uses them b/26778228
const char kDelegatedPrefixProperty[] = "DelegatedPrefix";
const char kDelegatedPrefixLengthProperty[] = "DelegatedPrefixLength";

// IPConfig DHCPv6 address/prefix property names.
const char kDhcpv6AddressProperty[] = "Address";
const char kDhcpv6LengthProperty[] = "Length";
const char kDhcpv6LeaseDurationSecondsProperty[] = "LeaseDurationSeconds";
const char kDhcpv6PreferredLeaseDurationSecondsProperty[] =
    "PreferredLeaseDurationSeconds";

// IPConfig type options.
const char kTypeIPv4[] = "ipv4";
const char kTypeIPv6[] = "ipv6";
const char kTypeDHCP[] = "dhcp";
const char kTypeBOOTP[] = "bootp";
const char kTypeZeroConf[] = "zeroconf";
const char kTypeDHCP6[] = "dhcp6";
const char kTypePPP[] = "ppp";

// Flimflam error options.
const char kErrorAaaFailed[] = "aaa-failed";
const char kErrorActivationFailed[] = "activation-failed";
const char kErrorBadPassphrase[] = "bad-passphrase";
const char kErrorBadWEPKey[] = "bad-wepkey";
const char kErrorConnectFailed[] = "connect-failed";
const char kErrorDNSLookupFailed[] = "dns-lookup-failed";
const char kErrorDhcpFailed[] = "dhcp-failed";
const char kErrorHTTPGetFailed[] = "http-get-failed";
const char kErrorInternal[] = "internal-error";
const char kErrorInvalidFailure[] = "invalid-failure";
const char kErrorIpsecCertAuthFailed[] = "ipsec-cert-auth-failed";
const char kErrorIpsecPskAuthFailed[] = "ipsec-psk-auth-failed";
const char kErrorNeedEvdo[] = "need-evdo";
const char kErrorNeedHomeNetwork[] = "need-home-network";
const char kErrorNoFailure[] = "no-failure";
const char kErrorOtaspFailed[] = "otasp-failed";
const char kErrorOutOfRange[] = "out-of-range";
const char kErrorPinMissing[] = "pin-missing";
const char kErrorPppAuthFailed[] = "ppp-auth-failed";
const char kErrorUnknownFailure[] = "unknown-failure";

// Flimflam error result codes.
const char kErrorResultSuccess[] = "org.chromium.flimflam.Error.Success";
const char kErrorResultFailure[] = "org.chromium.flimflam.Error.Failure";
const char kErrorResultAlreadyConnected[] =
    "org.chromium.flimflam.Error.AlreadyConnected";
const char kErrorResultAlreadyExists[] =
    "org.chromium.flimflam.Error.AlreadyExists";
const char kErrorResultIncorrectPin[] =
     "org.chromium.flimflam.Error.IncorrectPin";
const char kErrorResultInProgress[] = "org.chromium.flimflam.Error.InProgress";
const char kErrorResultInternalError[] =
    "org.chromium.flimflam.Error.InternalError";
const char kErrorResultInvalidApn[] = "org.chromium.flimflam.Error.InvalidApn";
const char kErrorResultInvalidArguments[] =
    "org.chromium.flimflam.Error.InvalidArguments";
const char kErrorResultInvalidNetworkName[] =
    "org.chromium.flimflam.Error.InvalidNetworkName";
const char kErrorResultInvalidPassphrase[] =
    "org.chromium.flimflam.Error.InvalidPassphrase";
const char kErrorResultInvalidProperty[] =
    "org.chromium.flimflam.Error.InvalidProperty";
const char kErrorResultNoCarrier[] = "org.chromium.flimflam.Error.NoCarrier";
const char kErrorResultNotConnected[] =
     "org.chromium.flimflam.Error.NotConnected";
const char kErrorResultNotFound[] = "org.chromium.flimflam.Error.NotFound";
const char kErrorResultNotImplemented[] =
    "org.chromium.flimflam.Error.NotImplemented";
const char kErrorResultNotOnHomeNetwork[] =
    "org.chromium.flimflam.Error.NotOnHomeNetwork";
const char kErrorResultNotRegistered[] =
    "org.chromium.flimflam.Error.NotRegistered";
const char kErrorResultNotSupported[] =
     "org.chromium.flimflam.Error.NotSupported";
const char kErrorResultOperationAborted[] =
    "org.chromium.flimflam.Error.OperationAborted";
const char kErrorResultOperationInitiated[] =
    "org.chromium.flimflam.Error.OperationInitiated";
const char kErrorResultOperationTimeout[] =
    "org.chromium.flimflam.Error.OperationTimeout";
const char kErrorResultPassphraseRequired[] =
    "org.chromium.flimflam.Error.PassphraseRequired";
const char kErrorResultPermissionDenied[] =
    "org.chromium.flimflam.Error.PermissionDenied";
const char kErrorResultPinBlocked[] = "org.chromium.flimflam.Error.PinBlocked";
const char kErrorResultPinRequired[] =
     "org.chromium.flimflam.Error.PinRequired";
const char kErrorResultWrongState[] = "org.chromium.flimflam.Error.WrongState";

const char kUnknownString[] = "UNKNOWN";

// Function names.
const char kClearPropertiesFunction[] = "ClearProperties";
const char kCompleteCellularActivationFunction[] = "CompleteCellularActivation";
const char kConfigureServiceForProfileFunction[] = "ConfigureServiceForProfile";
const char kConnectToBestServicesFunction[] = "ConnectToBestServices";
const char kCreateConnectivityReportFunction[] = "CreateConnectivityReport";
const char kAddWakeOnPacketConnectionFunction[] = "AddWakeOnPacketConnection";
const char kAddWakeOnPacketOfTypesFunction[] = "AddWakeOnPacketOfTypes";
const char kRemoveWakeOnPacketConnectionFunction[] =
    "RemoveWakeOnPacketConnection";
const char kRemoveWakeOnPacketOfTypesFunction[] = "RemoveWakeOnPacketOfTypes";
const char kRemoveAllWakeOnPacketConnectionsFunction[] =
    "RemoveAllWakeOnPacketConnections";
const char kGetLoadableProfileEntriesFunction[] = "GetLoadableProfileEntries";
const char kGetNetworksForGeolocation[] = "GetNetworksForGeolocation";
const char kPerformTDLSOperationFunction[] = "PerformTDLSOperation";
const char kRefreshFunction[] = "Refresh";
const char kResetFunction[] = "Reset";
const char kSetCarrierFunction[] = "SetCarrier";
const char kSetPropertiesFunction[] = "SetProperties";
const char kVerifyAndEncryptCredentialsFunction[] =
    "VerifyAndEncryptCredentials";
const char kVerifyAndEncryptDataFunction[] = "VerifyAndEncryptData";
const char kVerifyDestinationFunction[] = "VerifyDestination";

// ThirdPartyVpn functions.
const char kSetParametersFunction[] = "SetParameters";
const char kSendPacketFunction[] = "SendPacket";
const char kUpdateConnectionStateFunction[] = "UpdateConnectionState";
const char kOnPacketReceivedFunction[] = "OnPacketReceived";
const char kOnPlatformMessageFunction[] = "OnPlatformMessage";

// Device property names.
const char kEapAuthenticationCompletedProperty[] = "EapAuthenticationCompleted";
const char kEapAuthenticatorDetectedProperty[] = "EapAuthenticatorDetected";
const char kLinkMonitorResponseTimeProperty[] = "LinkMonitorResponseTime";
const char kLinkUpProperty[] = "Ethernet.LinkUp";
const char kProviderRequiresRoamingProperty[] =
    "Cellular.ProviderRequiresRoaming";
const char kPPPoEProperty[] = "Ethernet.PPPoE";
const char kReceiveByteCountProperty[] = "ReceiveByteCount";
const char kSIMPresentProperty[] = "Cellular.SIMPresent";
const char kSupportedCarriersProperty[] = "Cellular.SupportedCarriers";
const char kTransmitByteCountProperty[] = "TransmitByteCount";
const char kWifiSupportedFrequenciesProperty[] = "WiFi.SupportedFrequencies";

// Technology types (augments "Flimflam type options" above).
const char kTypeEthernetEap[] = "etherneteap";

// Error strings.
const char kErrorEapAuthenticationFailed[] = "eap-authentication-failed";
const char kErrorEapLocalTlsFailed[] = "eap-local-tls-failed";
const char kErrorEapRemoteTlsFailed[] = "eap-remote-tls-failed";

// Manager property names.
const char kDefaultServiceProperty[] = "DefaultService";
const char kDisableWiFiVHTProperty[] = "DisableWiFiVHT";
const char kHostNameProperty[] = "HostName";
const char kIgnoredDNSSearchPathsProperty[] = "IgnoredDNSSearchPaths";
const char kLinkMonitorTechnologiesProperty[] =
    "LinkMonitorTechnologies";
const char kNoAutoConnectTechnologiesProperty[] = "NoAutoConnectTechnologies";
const char kPortalCheckIntervalProperty[] = "PortalCheckInterval";
const char kProhibitedTechnologiesProperty[] = "ProhibitedTechnologies";
const char kServiceCompleteListProperty[] = "ServiceCompleteList";
const char kShortDNSTimeoutTechnologiesProperty[] =
    "ShortDNSTimeoutTechnologies";
const char kUninitializedTechnologiesProperty[] = "UninitializedTechnologies";
const char kWakeOnLanEnabledProperty[] = "WakeOnLanEnabled";
const char kWakeOnWiFiFeaturesEnabledProperty[] = "WakeOnWiFiFeaturesEnabled";

// Service property names.
const char kActivationTypeProperty[] = "Cellular.ActivationType";
const char kDiagnosticsDisconnectsProperty[] = "Diagnostics.Disconnects";
const char kDiagnosticsMisconnectsProperty[] = "Diagnostics.Misconnects";
const char kEapRemoteCertificationProperty[] = "EAP.RemoteCertification";
const char kEapCaCertPemProperty[] = "EAP.CACertPEM";
const char kEapSubjectMatchProperty[] = "EAP.SubjectMatch";
const char kErrorDetailsProperty[] = "ErrorDetails";
const char kKeyManagementIEEE8021X[] = "IEEE8021X";
const char kIPConfigProperty[] = "IPConfig";
const char kL2tpIpsecCaCertPemProperty[] = "L2TPIPsec.CACertPEM";
const char kL2tpIpsecTunnelGroupProperty[] = "L2TPIPsec.TunnelGroup";
const char kL2tpIpsecXauthPasswordProperty[] = "L2TPIPsec.XauthPassword";
const char kL2tpIpsecXauthUserProperty[] = "L2TPIPsec.XauthUser";
const char kL2tpIpsecLcpEchoDisabledProperty[] = "L2TPIPsec.LCPEchoDisabled";
const char kManagedCredentialsProperty[] = "ManagedCredentials";
const char kOpenVPNCaCertPemProperty[] = "OpenVPN.CACertPEM";
const char kOpenVPNCertProperty[] = "OpenVPN.Cert";
const char kOpenVPNExtraCertPemProperty[] = "OpenVPN.ExtraCertPEM";
const char kOpenVPNExtraHostsProperty[] = "OpenVPN.ExtraHosts";
const char kOpenVPNKeyProperty[] = "OpenVPN.Key";
const char kOpenVPNPingProperty[] = "OpenVPN.Ping";
const char kOpenVPNPingExitProperty[] = "OpenVPN.PingExit";
const char kOpenVPNPingRestartProperty[] = "OpenVPN.PingRestart";
const char kOpenVPNTLSAuthProperty[] = "OpenVPN.TLSAuth";
const char kOpenVPNTLSVersionMinProperty[] = "OpenVPN.TLSVersionMin";
const char kOpenVPNTokenProperty[] = "OpenVPN.Token";
const char kOpenVPNVerbProperty[] = "OpenVPN.Verb";
const char kOpenVPNVerifyHashProperty[] = "OpenVPN.VerifyHash";
const char kOpenVPNVerifyX509NameProperty[] = "OpenVPN.VerifyX509Name";
const char kOpenVPNVerifyX509TypeProperty[] = "OpenVPN.VerifyX509Type";
const char kOutOfCreditsProperty[] = "Cellular.OutOfCredits";
const char kPhysicalTechnologyProperty[] = "PhysicalTechnology";
const char kPreviousErrorProperty[] = "PreviousError";
const char kPreviousErrorSerialNumberProperty[] = "PreviousErrorSerialNumber";
const char kStaticIPAddressProperty[] = "StaticIP.Address";
const char kStaticIPGatewayProperty[] = "StaticIP.Gateway";
const char kStaticIPMtuProperty[] = "StaticIP.Mtu";
const char kStaticIPNameServersProperty[] = "StaticIP.NameServers";
const char kStaticIPPeerAddressProperty[] = "StaticIP.PeerAddress";
const char kStaticIPPrefixlenProperty[] = "StaticIP.Prefixlen";
const char kSavedIPAddressProperty[] = "SavedIP.Address";
const char kSavedIPGatewayProperty[] = "SavedIP.Gateway";
const char kSavedIPMtuProperty[] = "SavedIP.Mtu";
const char kSavedIPNameServersProperty[] = "SavedIP.NameServers";
const char kSavedIPPeerAddressProperty[] = "SavedIP.PeerAddress";
const char kSavedIPPrefixlenProperty[] = "SavedIP.Prefixlen";
const char kTetheringProperty[] = "Tethering";
const char kVPNMTUProperty[] = "VPN.MTU";
const char kWifiFrequencyListProperty[] = "WiFi.FrequencyList";
const char kWifiVendorInformationProperty[] = "WiFi.VendorInformation";
const char kWifiProtectedManagementFrameRequiredProperty[] =
    "WiFi.ProtectedManagementFrameRequired";

// Profile property names.
const char kUserHashProperty[] = "UserHash";

// Service Tethering property values.
const char kTetheringNotDetectedState[] = "NotDetected";
const char kTetheringSuspectedState[] = "Suspected";
const char kTetheringConfirmedState[] = "Confirmed";

// WiFi Service Vendor Information dictionary properties.
const char kVendorWPSManufacturerProperty[] = "Manufacturer";
const char kVendorWPSModelNameProperty[] = "ModelName";
const char kVendorWPSModelNumberProperty[] = "ModelNumber";
const char kVendorWPSDeviceNameProperty[] = "DeviceName";
const char kVendorOUIListProperty[] = "OUIList";

// WiFi Device link property names.
const char kLinkStatisticsProperty[] = "LinkStatistics";
const char kAverageReceiveSignalDbmProperty[] = "AverageReceiveSignalDbm";
const char kInactiveTimeMillisecondsProperty[] =
    "InactiveTimeMilliseconds";
const char kLastReceiveSignalDbmProperty[] = "LastReceiveSignalDbm";
const char kPacketReceiveSuccessesProperty[] = "PacketReceiveSuccesses";
const char kPacketTransmitFailuresProperty[] = "PacketTransmitFailures";
const char kPacketTransmitSuccessesProperty[] = "PacketTrasmitSuccesses";
const char kTransmitBitrateProperty[] = "TransmitBitrate";
const char kTransmitRetriesProperty[] = "TransmitRetries";

// WiFi TDLS operations.
const char kTDLSDiscoverOperation[] = "Discover";
const char kTDLSSetupOperation[] = "Setup";
const char kTDLSStatusOperation[] = "Status";
const char kTDLSTeardownOperation[] = "Teardown";

// WiFi TDLS states.
const char kTDLSConnectedState[] = "Connected";
const char kTDLSDisabledState[] = "Disabled";
const char kTDLSDisconnectedState[] = "Disconnected";
const char kTDLSNonexistentState[] = "Nonexistent";
const char kTDLSUnknownState[] = "Unknown";

// Wake on WiFi features.
const char kWakeOnWiFiFeaturesEnabledPacket[] = "packet";
const char kWakeOnWiFiFeaturesEnabledDarkConnect[] = "darkconnect";
const char kWakeOnWiFiFeaturesEnabledPacketDarkConnect[] =
    "packet_and_darkconnect";
const char kWakeOnWiFiFeaturesEnabledNone[] = "none";
const char kWakeOnWiFiFeaturesEnabledNotSupported[] = "not_supported";

// Wake on WiFi Packet Type Constants.
const char kWakeOnTCP[] = "TCP";
const char kWakeOnUDP[] = "UDP";
const char kWakeOnIDP[] = "IDP";
const char kWakeOnIPIP[] = "IPIP";
const char kWakeOnIGMP[] = "IGMP";
const char kWakeOnICMP[] = "ICMP";
const char kWakeOnIP[] = "IP";

// Cellular service carriers.
const char kCarrierGenericUMTS[] = "Generic UMTS";
const char kCarrierSprint[] = "Sprint";
const char kCarrierVerizon[] = "Verizon Wireless";

// Cellular activation types.
const char kActivationTypeNonCellular[] = "NonCellular";  // For future use
const char kActivationTypeOMADM[] = "OMADM";  // For future use
const char kActivationTypeOTA[] = "OTA";
const char kActivationTypeOTASP[] = "OTASP";

// Geolocation property field names.
// Reference:
//    https://devsite.googleplex.com/maps/documentation/business/geolocation/
// Top level properties for a Geolocation request.
const char kGeoHomeMobileCountryCodeProperty[] = "homeMobileCountryCode";
const char kGeoHomeMobileNetworkCodeProperty[] = "homeMobileNetworkCode";
const char kGeoRadioTypePropertyProperty[] = "radioType";
const char kGeoCellTowersProperty[] = "cellTowers";
const char kGeoWifiAccessPointsProperty[] = "wifiAccessPoints";
// Cell tower object property names.
const char kGeoCellIdProperty[] = "cellId";
const char kGeoLocationAreaCodeProperty[] = "locationAreaCode";
const char kGeoMobileCountryCodeProperty[] = "mobileCountryCode";
const char kGeoMobileNetworkCodeProperty[] = "mobileNetworkCode";
const char kGeoTimingAdvanceProperty[] = "timingAdvance";
// WiFi access point property names.
const char kGeoMacAddressProperty[] = "macAddress";
const char kGeoChannelProperty[] = "channel";
const char kGeoSignalToNoiseRatioProperty[] = "signalToNoiseRatio";
// Common property names for geolocation objects.
const char kGeoAgeProperty[] = "age";
const char kGeoSignalStrengthProperty[] = "signalStrength";
// ThirdPartyVpn parameters, properties and constants.
const char kAddressParameterThirdPartyVpn[] = "address";
const char kBroadcastAddressParameterThirdPartyVpn[] = "broadcast_address";
const char kGatewayParameterThirdPartyVpn[] = "gateway";
const char kBypassTunnelForIpParameterThirdPartyVpn[] = "bypass_tunnel_for_ip";
const char kSubnetPrefixParameterThirdPartyVpn[] = "subnet_prefix";
const char kMtuParameterThirdPartyVpn[] = "mtu";
const char kDomainSearchParameterThirdPartyVpn[] = "domain_search";
const char kDnsServersParameterThirdPartyVpn[] = "dns_servers";
const char kInclusionListParameterThirdPartyVpn[] = "inclusion_list";
const char kExclusionListParameterThirdPartyVpn[] = "exclusion_list";
const char kReconnectParameterThirdPartyVpn[] = "reconnect";
const char kObjectPathSuffixProperty[] = "ObjectPathSuffix";
const char kExtensionNameProperty[] = "ExtensionName";
const char kConfigurationNameProperty[] = "ConfigurationName";
const char kObjectPathBase[] = "/thirdpartyvpn/";
const char kNonIPDelimiter = ':';
const char kIPDelimiter = ' ';
}  // namespace shill

#endif  // SYSTEM_API_DBUS_SHILL_DBUS_CONSTANTS_H_
