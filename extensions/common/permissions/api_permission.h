// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_PERMISSIONS_API_PERMISSION_H_
#define EXTENSIONS_COMMON_PERMISSIONS_API_PERMISSION_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/pickle.h"
#include "base/values.h"

namespace extensions {

class PermissionIDSet;
class APIPermissionInfo;
class ChromeAPIPermissions;

// APIPermission is for handling some complex permissions. Please refer to
// extensions::SocketPermission as an example.
// There is one instance per permission per loaded extension.
class APIPermission {
 public:
  // The IDs of all permissions available to apps. Add as many permissions here
  // as needed to generate meaningful permission messages. Add the rules for the
  // messages to ChromePermissionMessageProvider.
  // Do not reorder this enumeration or remove any entries. To deprecate an
  // entry, prefix it with the "kDeleted_" specifier and to add a new entry, add
  // it just prior to kEnumBoundary, and ensure to update the
  // "ExtensionPermission3" enum in tools/metrics/histograms/histograms.xml (by
  // running update_extension_permission.py).
  // TODO(sashab): Move this to a more central location, and rename it to
  // PermissionID.
  enum ID {
    // Error codes.
    kInvalid,
    kUnknown,

    // Actual permission IDs. Not all of these are valid permissions on their
    // own; some are just needed by various manifest permissions to represent
    // their permission message rule combinations.
    kAccessibilityFeaturesModify,
    kAccessibilityFeaturesRead,
    kAccessibilityPrivate,
    kActiveTab,
    kActivityLogPrivate,
    kAlarms,
    kAlphaEnabled,
    kAlwaysOnTopWindows,
    kAppView,
    kAudio,
    kAudioCapture,
    kDeleted_AudioModem,
    kAutofillPrivate,
    kAutomation,
    kAutoTestPrivate,
    kBackground,
    kBluetoothPrivate,
    kBookmark,
    kBookmarkManagerPrivate,
    kBrailleDisplayPrivate,
    kBrowser,
    kBrowsingData,
    kCast,
    kCastStreaming,
    kChromeosInfoPrivate,
    kClipboardRead,
    kClipboardWrite,
    kCloudPrintPrivate,
    kCommandLinePrivate,
    kCommandsAccessibility,
    kContentSettings,
    kContextMenus,
    kCookie,
    kDeleted_Copresence,
    kDeleted_CopresencePrivate,
    kCryptotokenPrivate,
    kDataReductionProxy,
    kDiagnostics,
    kDeleted_Dial,  // API removed.
    kDebugger,
    kDeclarative,
    kDeclarativeContent,
    kDeclarativeWebRequest,
    kDesktopCapture,
    kDesktopCapturePrivate,
    kDeveloperPrivate,
    kDevtools,
    kDns,
    kDocumentScan,
    kDownloads,
    kDownloadsInternal,
    kDownloadsOpen,
    kDownloadsShelf,
    kEasyUnlockPrivate,
    kEchoPrivate,
    kEmbeddedExtensionOptions,
    kEnterprisePlatformKeys,
    kEnterprisePlatformKeysPrivate,
    kDeleted_ExperienceSamplingPrivate,
    kExperimental,
    kExtensionView,
    kExternallyConnectableAllUrls,
    kFeedbackPrivate,
    kFileBrowserHandler,
    kFileBrowserHandlerInternal,
    kFileManagerPrivate,
    kFileSystem,
    kFileSystemDirectory,
    kFileSystemProvider,
    kFileSystemRequestFileSystem,
    kFileSystemRetainEntries,
    kFileSystemWrite,
    kDeleted_FileSystemWriteDirectory,
    kFirstRunPrivate,
    kFontSettings,
    kFullscreen,
    kDeleted_GcdPrivate,
    kGcm,
    kGeolocation,
    kHid,
    kHistory,
    kHomepage,
    kHotwordPrivate,
    kIdentity,
    kIdentityEmail,
    kIdentityPrivate,
    kIdltest,
    kIdle,
    kImeWindowEnabled,
    kInlineInstallPrivate,
    kInput,
    kInputMethodPrivate,
    kDeleted_InterceptAllKeys,
    kLauncherSearchProvider,
    kLocation,
    kDeleted_LogPrivate,
    kManagement,
    kMediaGalleries,
    kMediaPlayerPrivate,
    kMediaRouterPrivate,
    kMetricsPrivate,
    kMDns,
    kMusicManagerPrivate,
    kNativeMessaging,
    kNetworkingConfig,
    kNetworkingPrivate,
    kDeleted_NotificationProvider,
    kNotifications,
    kOverrideEscFullscreen,
    kPageCapture,
    kPointerLock,
    kPlatformKeys,
    kDeleted_Plugin,
    kPower,
    kPreferencesPrivate,
    kDeleted_PrincipalsPrivate,
    kPrinterProvider,
    kPrivacy,
    kProcesses,
    kProxy,
    kImageWriterPrivate,
    kDeleted_ReadingListPrivate,
    kRtcPrivate,
    kSearchProvider,
    kSearchEnginesPrivate,
    kSerial,
    kSessions,
    kSettingsPrivate,
    kSignedInDevices,
    kSocket,
    kStartupPages,
    kStorage,
    kStreamsPrivate,
    kSyncFileSystem,
    kSystemPrivate,
    kSystemDisplay,
    kSystemStorage,
    kTab,
    kTabCapture,
    kTabCaptureForTab,
    kTerminalPrivate,
    kTopSites,
    kTts,
    kTtsEngine,
    kUnlimitedStorage,
    kU2fDevices,
    kUsb,
    kUsbDevice,
    kVideoCapture,
    kVirtualKeyboardPrivate,
    kVpnProvider,
    kWallpaper,
    kWallpaperPrivate,
    kWebcamPrivate,
    kWebConnectable,  // for externally_connectable manifest key
    kWebNavigation,
    kWebRequest,
    kWebRequestBlocking,
    kWebrtcAudioPrivate,
    kWebrtcDesktopCapturePrivate,
    kWebrtcLoggingPrivate,
    kWebstorePrivate,
    kWebstoreWidgetPrivate,
    kWebView,
    kWindowShape,
    kScreenlockPrivate,
    kSystemCpu,
    kSystemMemory,
    kSystemNetwork,
    kSystemInfoCpu,
    kSystemInfoMemory,
    kBluetooth,
    kBluetoothDevices,
    kFavicon,
    kFullAccess,
    kHostReadOnly,
    kHostReadWrite,
    kHostsAll,
    kHostsAllReadOnly,
    kMediaGalleriesAllGalleriesCopyTo,
    kMediaGalleriesAllGalleriesDelete,
    kMediaGalleriesAllGalleriesRead,
    kNetworkState,
    kOverrideBookmarksUI,
    kShouldWarnAllHosts,
    kSocketAnyHost,
    kSocketDomainHosts,
    kSocketSpecificHosts,
    kDeleted_UsbDeviceList,
    kUsbDeviceUnknownProduct,
    kUsbDeviceUnknownVendor,
    kUsersPrivate,
    kPasswordsPrivate,
    kLanguageSettingsPrivate,
    kEnterpriseDeviceAttributes,
    kCertificateProvider,
    kResourcesPrivate,
    kDisplaySource,
    kClipboard,
    kNetworkingOnc,
    kVirtualKeyboard,
    kNetworkingCastPrivate,
    kMediaPerceptionPrivate,
    kLockScreen,
    kNewTabPageOverride,
    kDeclarativeNetRequest,
    kLockWindowFullscreenPrivate,
    kWebrtcLoggingPrivateAudioDebug,
    kEnterpriseReportingPrivate,
    kCecPrivate,
    kSafeBrowsingPrivate,
    // Last entry: Add new entries above and ensure to update the
    // "ExtensionPermission3" enum in tools/metrics/histograms/histograms.xml
    // (by running update_extension_permission.py).
    kEnumBoundary
  };

  struct CheckParam {
  };

  explicit APIPermission(const APIPermissionInfo* info);

  virtual ~APIPermission();

  // Returns the id of this permission.
  ID id() const;

  // Returns the name of this permission.
  const char* name() const;

  // Returns the APIPermission of this permission.
  const APIPermissionInfo* info() const {
    return info_;
  }

  // The set of permissions an app/extension with this API permission has. These
  // permissions are used by PermissionMessageProvider to generate meaningful
  // permission messages for the app/extension.
  //
  // For simple API permissions, this will return a set containing only the ID
  // of the permission. More complex permissions might have multiple IDs, one
  // for each of the capabilities the API permission has (e.g. read, write and
  // copy, in the case of the media gallery permission). Permissions that
  // require parameters may also contain a parameter string (along with the
  // permission's ID) which can be substituted into the permission message if a
  // rule is defined to do so.
  //
  // Permissions with multiple values, such as host permissions, are represented
  // by multiple entries in this set. Each permission in the subset has the same
  // ID (e.g. kHostReadOnly) but a different parameter (e.g. google.com). These
  // are grouped to form different kinds of permission messages (e.g. 'Access to
  // 2 hosts') depending on the number that are in the set. The rules that
  // define the grouping of related permissions with the same ID is defined in
  // ChromePermissionMessageProvider.
  virtual PermissionIDSet GetPermissions() const = 0;

  // Returns true if the given permission is allowed.
  virtual bool Check(const CheckParam* param) const = 0;

  // Returns true if |rhs| is a subset of this.
  virtual bool Contains(const APIPermission* rhs) const = 0;

  // Returns true if |rhs| is equal to this.
  virtual bool Equal(const APIPermission* rhs) const = 0;

  // Parses the APIPermission from |value|. Returns false if an error happens
  // and optionally set |error| if |error| is not NULL. If |value| represents
  // multiple permissions, some are invalid, and |unhandled_permissions| is
  // not NULL, the invalid ones are put into |unhandled_permissions| and the
  // function returns true.
  virtual bool FromValue(const base::Value* value,
                         std::string* error,
                         std::vector<std::string>* unhandled_permissions) = 0;

  // Stores this into a new created |value|.
  virtual std::unique_ptr<base::Value> ToValue() const = 0;

  // Clones this.
  virtual APIPermission* Clone() const = 0;

  // Returns a new API permission which equals this - |rhs|.
  virtual APIPermission* Diff(const APIPermission* rhs) const = 0;

  // Returns a new API permission which equals the union of this and |rhs|.
  virtual APIPermission* Union(const APIPermission* rhs) const = 0;

  // Returns a new API permission which equals the intersect of this and |rhs|.
  virtual APIPermission* Intersect(const APIPermission* rhs) const = 0;

  // IPC functions
  // Writes this into the given IPC message |m|.
  virtual void Write(base::Pickle* m) const = 0;

  // Reads from the given IPC message |m|.
  virtual bool Read(const base::Pickle* m, base::PickleIterator* iter) = 0;

  // Logs this permission.
  virtual void Log(std::string* log) const = 0;

 private:
  const APIPermissionInfo* const info_;
};


// The APIPermissionInfo is an immutable class that describes a single
// named permission (API permission).
// There is one instance per permission.
class APIPermissionInfo {
 public:
  enum Flag {
    kFlagNone = 0,

    // Plugins (NPAPI) are deprecated.
    // kFlagImpliesFullAccess = 1 << 0,

    // Indicates if the permission implies full URL access.
    kFlagImpliesFullURLAccess = 1 << 1,

    // Indicates that extensions cannot specify the permission as optional.
    kFlagCannotBeOptional = 1 << 3,

    // Indicates that the permission is internal to the extensions
    // system and cannot be specified in the "permissions" list.
    kFlagInternal = 1 << 4,

    // Indicates that the permission may be granted to web contents by
    // extensions using the content_capabilities manifest feature.
    kFlagSupportsContentCapabilities = 1 << 5,
  };

  typedef APIPermission* (*APIPermissionConstructor)(const APIPermissionInfo*);

  typedef std::set<APIPermission::ID> IDSet;

  ~APIPermissionInfo();

  // Creates a APIPermission instance.
  APIPermission* CreateAPIPermission() const;

  int flags() const { return flags_; }

  APIPermission::ID id() const { return id_; }

  // Returns the name of this permission.
  const char* name() const { return name_; }

  // Returns true if this permission implies full URL access.
  bool implies_full_url_access() const {
    return (flags_ & kFlagImpliesFullURLAccess) != 0;
  }

  // Returns true if this permission can be added and removed via the
  // optional permissions extension API.
  bool supports_optional() const {
    return (flags_ & kFlagCannotBeOptional) == 0;
  }

  // Returns true if this permission is internal rather than a
  // "permissions" list entry.
  bool is_internal() const {
    return (flags_ & kFlagInternal) != 0;
  }

  // Returns true if this permission can be granted to web contents by an
  // extension through the content_capabilities manifest feature.
  bool supports_content_capabilities() const {
    return (flags_ & kFlagSupportsContentCapabilities) != 0;
  }

 private:
  // Instances should only be constructed from within a PermissionsProvider.
  friend class CastAPIPermissions;
  friend class ChromeAPIPermissions;
  friend class ExtensionsAPIPermissions;
  // Implementations of APIPermission will want to get the permission message,
  // but this class's implementation should be hidden from everyone else.
  friend class APIPermission;

  // This exists to allow aggregate initialization, so that default values
  // for flags, etc. can be omitted.
  // TODO(yoz): Simplify the way initialization is done. APIPermissionInfo
  // should be the simple data struct.
  struct InitInfo {
    APIPermission::ID id;
    const char* name;
    int flags;
    APIPermissionInfo::APIPermissionConstructor constructor;
  };

  explicit APIPermissionInfo(const InitInfo& info);

  const APIPermission::ID id_;
  const char* const name_;
  const int flags_;
  const APIPermissionConstructor api_permission_constructor_;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_PERMISSIONS_API_PERMISSION_H_
