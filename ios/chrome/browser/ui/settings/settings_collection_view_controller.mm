// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/settings_collection_view_controller.h"

#include <memory>

#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/util.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#import "ios/chrome/browser/signin/authentication_service.h"
#include "ios/chrome/browser/signin/authentication_service_factory.h"
#import "ios/chrome/browser/signin/chrome_identity_service_observer_bridge.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/sync/sync_observer_bridge.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_item.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_consumer.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_account_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/commands/settings_main_page_commands.h"
#import "ios/chrome/browser/ui/settings/about_chrome_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/accounts_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/autofill_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/bandwidth_management_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/cells/account_signin_item.h"
#import "ios/chrome/browser/ui/settings/content_settings_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/material_cell_catalog_view_controller.h"
#import "ios/chrome/browser/ui/settings/privacy_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/save_passwords_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/search_engine_settings_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/sync_utils/sync_util.h"
#import "ios/chrome/browser/ui/settings/table_cell_catalog_view_controller.h"
#import "ios/chrome/browser/ui/settings/utils/pref_backed_boolean.h"
#import "ios/chrome/browser/ui/settings/voicesearch_collection_view_controller.h"
#import "ios/chrome/browser/ui/signin_interaction/public/signin_presenter.h"
#import "ios/chrome/browser/ui/signin_interaction/signin_interaction_coordinator.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/browser/voice/speech_input_locale_config.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/signin_resources_provider.h"
#include "ios/public/provider/chrome/browser/voice/voice_search_prefs.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kSettingsCollectionViewId = @"kSettingsCollectionViewId";
NSString* const kSettingsDoneButtonId = @"kSettingsDoneButtonId";
NSString* const kSettingsSignInCellId = @"kSettingsSignInCellId";
NSString* const kSettingsAccountCellId = @"kSettingsAccountCellId";
NSString* const kSettingsSearchEngineCellId = @"Search Engine";
NSString* const kSettingsVoiceSearchCellId = @"Voice Search Settings";

@interface SettingsCollectionViewController (NotificationBridgeDelegate)
// Notifies this controller that the sign in state has changed.
- (void)onSignInStateChanged;
@end

namespace {

const CGFloat kAccountProfilePhotoDimension = 40.0f;

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierSignIn = kSectionIdentifierEnumZero,
  SectionIdentifierBasics,
  SectionIdentifierAdvanced,
  SectionIdentifierInfo,
  SectionIdentifierDebug,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeSignInButton = kItemTypeEnumZero,
  ItemTypeSigninPromo,
  ItemTypeAccount,
  ItemTypeHeader,
  ItemTypeSearchEngine,
  ItemTypeSavedPasswords,
  ItemTypeAutofill,
  ItemTypeVoiceSearch,
  ItemTypePrivacy,
  ItemTypeContentSettings,
  ItemTypeBandwidth,
  ItemTypeAboutChrome,
  ItemTypeMemoryDebugging,
  ItemTypeViewSource,
  ItemTypeLogJavascript,
  ItemTypeCollectionCellCatalog,
  ItemTypeTableCellCatalog,
  ItemTypeArticlesForYou,
};

#if CHROMIUM_BUILD && !defined(NDEBUG)
NSString* kDevViewSourceKey = @"DevViewSource";
NSString* kLogJavascriptKey = @"LogJavascript";
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)

#pragma mark - SigninObserverBridge Class

class SigninObserverBridge : public SigninManagerBase::Observer {
 public:
  SigninObserverBridge(ios::ChromeBrowserState* browserState,
                       SettingsCollectionViewController* owner);
  ~SigninObserverBridge() override {}

  // SigninManagerBase::Observer implementation:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

 private:
  __weak SettingsCollectionViewController* owner_;
  ScopedObserver<SigninManager, SigninObserverBridge> observer_;
};

SigninObserverBridge::SigninObserverBridge(
    ios::ChromeBrowserState* browserState,
    SettingsCollectionViewController* owner)
    : owner_(owner), observer_(this) {
  DCHECK(owner_);
  SigninManager* signin_manager =
      ios::SigninManagerFactory::GetForBrowserState(browserState);
  if (!signin_manager)
    return;
  observer_.Add(signin_manager);
}

void SigninObserverBridge::GoogleSigninSucceeded(const std::string& account_id,
                                                 const std::string& username) {
  [owner_ onSignInStateChanged];
}

void SigninObserverBridge::GoogleSignedOut(const std::string& account_id,
                                           const std::string& username) {
  [owner_ onSignInStateChanged];
}

}  // namespace

#pragma mark - SettingsCollectionViewController

@interface SettingsCollectionViewController ()<BooleanObserver,
                                               ChromeIdentityServiceObserver,
                                               PrefObserverDelegate,
                                               SettingsControllerProtocol,
                                               SettingsMainPageCommands,
                                               SigninPresenter,
                                               SigninPromoViewConsumer,
                                               SyncObserverModelBridge> {
  // The current browser state that hold the settings. Never off the record.
  ios::ChromeBrowserState* _browserState;  // weak

  std::unique_ptr<SigninObserverBridge> _notificationBridge;
  std::unique_ptr<SyncObserverBridge> _syncObserverBridge;
  // Whether the impression of the Signin button has already been recorded.
  BOOL _hasRecordedSigninImpression;
  // PrefBackedBoolean for ShowMemoryDebugTools switch.
  PrefBackedBoolean* _showMemoryDebugToolsEnabled;
  // PrefBackedBoolean for ArticlesForYou switch.
  PrefBackedBoolean* _articlesEnabled;
  // The item related to the switch for the show suggestions setting.
  CollectionViewSwitchItem* _showMemoryDebugToolsItem;
  // The item related to the switch for the show suggestions setting.
  CollectionViewSwitchItem* _articlesForYouItem;

  // Mediator to configure the sign-in promo cell. Also used to received
  // identity update notifications.
  SigninPromoViewMediator* _signinPromoViewMediator;

  // Cached resized profile image.
  UIImage* _resizedImage;
  __weak UIImage* _oldImage;

  // Identity object and observer used for Account Item refresh.
  ChromeIdentity* _identity;
  std::unique_ptr<ChromeIdentityServiceObserverBridge> _identityServiceObserver;

  // PrefMember for voice locale code.
  StringPrefMember _voiceLocaleCode;
  // Pref observer to track changes to prefs.
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  // TODO(crbug.com/662435): Refactor PrefObserverBridge so it owns the
  // PrefChangeRegistrar.
  // Registrar for pref changes notifications.
  PrefChangeRegistrar _prefChangeRegistrar;

  // Updatable Items.
  CollectionViewDetailItem* _voiceSearchDetailItem;
  CollectionViewDetailItem* _defaultSearchEngineItem;
  CollectionViewDetailItem* _savePasswordsDetailItem;
  CollectionViewDetailItem* _autoFillDetailItem;

  // YES if the user used at least once the sign-in promo view buttons.
  BOOL _signinStarted;
  // YES if view has been dismissed.
  BOOL _settingsHasBeenDismissed;
}

@property(nonatomic, readonly, weak) id<ApplicationCommands> dispatcher;

// The SigninInteractionCoordinator that presents Sign In UI for the
// Settings page.
@property(nonatomic, strong)
    SigninInteractionCoordinator* signinInteractionCoordinator;

// Stops observing browser state services. This is required during the shutdown
// phase to avoid observing services for a profile that is being killed.
- (void)stopBrowserStateServiceObservers;

@end

@implementation SettingsCollectionViewController
@synthesize settingsMainPageDispatcher = _settingsMainPageDispatcher;
@synthesize dispatcher = _dispatcher;
@synthesize signinInteractionCoordinator = _signinInteractionCoordinator;

#pragma mark Initialization

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                          dispatcher:(id<ApplicationCommands>)dispatcher {
  DCHECK(!browserState->IsOffTheRecord());
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    _browserState = browserState;
    self.title = l10n_util::GetNSStringWithFixup(IDS_IOS_SETTINGS_TITLE);
    self.collectionViewAccessibilityIdentifier = kSettingsCollectionViewId;
    _notificationBridge.reset(new SigninObserverBridge(_browserState, self));
    syncer::SyncService* syncService =
        IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState);
    _syncObserverBridge.reset(new SyncObserverBridge(self, syncService));

    _showMemoryDebugToolsEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:GetApplicationContext()->GetLocalState()
                   prefName:prefs::kShowMemoryDebuggingTools];
    [_showMemoryDebugToolsEnabled setObserver:self];

    AuthenticationService* authService =
        AuthenticationServiceFactory::GetForBrowserState(_browserState);
    _identity = authService->GetAuthenticatedIdentity();
    _identityServiceObserver.reset(
        new ChromeIdentityServiceObserverBridge(self));

    PrefService* prefService = _browserState->GetPrefs();

    _articlesEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:prefService
                   prefName:prefs::kArticlesForYouEnabled];
    [_articlesEnabled setObserver:self];

    _voiceLocaleCode.Init(prefs::kVoiceSearchLocale, prefService);

    _prefChangeRegistrar.Init(prefService);
    _prefObserverBridge.reset(new PrefObserverBridge(self));
    // Register to observe any changes on Perf backed values displayed by the
    // screen.
    _prefObserverBridge->ObserveChangesForPreference(prefs::kVoiceSearchLocale,
                                                     &_prefChangeRegistrar);
    _prefObserverBridge->ObserveChangesForPreference(
        password_manager::prefs::kCredentialsEnableService,
        &_prefChangeRegistrar);
    _prefObserverBridge->ObserveChangesForPreference(
        autofill::prefs::kAutofillEnabled, &_prefChangeRegistrar);

    _settingsMainPageDispatcher = self;
    _dispatcher = dispatcher;

    // TODO(crbug.com/764578): -loadModel should not be called from
    // initializer. A possible fix is to move this call to -viewDidLoad.
    [self loadModel];
  }
  return self;
}

- (void)dealloc {
  DCHECK(_settingsHasBeenDismissed)
      << "-settingsWillBeDismissed must be called before -dealloc";
}

- (void)stopBrowserStateServiceObservers {
  _syncObserverBridge.reset();
  _notificationBridge.reset();
  _identityServiceObserver.reset();
  [_showMemoryDebugToolsEnabled setObserver:nil];
  [_articlesEnabled setObserver:nil];
}

#pragma mark View lifecycle

// TODO(crbug.com/661915): Refactor TemplateURLObserver and re-implement this so
// it observes the default search engine name instead of reloading on
// ViewWillAppear.
- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [self updateSearchCell];
}

#pragma mark SettingsRootCollectionViewController

- (void)loadModel {
  [super loadModel];

  CollectionViewModel* model = self.collectionViewModel;

  // Sign in/Account section
  [model addSectionWithIdentifier:SectionIdentifierSignIn];
  AuthenticationService* authService =
      AuthenticationServiceFactory::GetForBrowserState(_browserState);
  if (!authService->IsAuthenticated()) {
    if ([SigninPromoViewMediator
            shouldDisplaySigninPromoViewWithAccessPoint:
                signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS
                                           browserState:_browserState]) {
      if (!_signinPromoViewMediator) {
        _signinPromoViewMediator = [[SigninPromoViewMediator alloc]
            initWithBrowserState:_browserState
                     accessPoint:signin_metrics::AccessPoint::
                                     ACCESS_POINT_SETTINGS
                       presenter:self /* id<SigninPresenter> */];
        _signinPromoViewMediator.consumer = self;
      }
    } else {
      [_signinPromoViewMediator signinPromoViewRemoved];
      _signinPromoViewMediator = nil;
    }
    [model addItem:[self signInTextItem]
        toSectionWithIdentifier:SectionIdentifierSignIn];
  } else {
    _hasRecordedSigninImpression = NO;
    [_signinPromoViewMediator signinPromoViewRemoved];
    _signinPromoViewMediator = nil;
    [model addItem:[self accountCellItem]
        toSectionWithIdentifier:SectionIdentifierSignIn];
  }

  // Basics section
  [model addSectionWithIdentifier:SectionIdentifierBasics];
  CollectionViewTextItem* basicsHeader =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeHeader];
  basicsHeader.text = l10n_util::GetNSString(IDS_IOS_OPTIONS_GENERAL_TAB_LABEL);
  basicsHeader.textColor = [[MDCPalette greyPalette] tint500];
  [model setHeader:basicsHeader
      forSectionWithIdentifier:SectionIdentifierBasics];
  [model addItem:[self searchEngineDetailItem]
      toSectionWithIdentifier:SectionIdentifierBasics];
  [model addItem:[self savePasswordsDetailItem]
      toSectionWithIdentifier:SectionIdentifierBasics];
  [model addItem:[self autoFillDetailItem]
      toSectionWithIdentifier:SectionIdentifierBasics];

  // Advanced Section
  [model addSectionWithIdentifier:SectionIdentifierAdvanced];
  CollectionViewTextItem* advancedHeader =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeHeader];
  advancedHeader.text =
      l10n_util::GetNSString(IDS_IOS_OPTIONS_ADVANCED_TAB_LABEL);
  advancedHeader.textColor = [[MDCPalette greyPalette] tint500];
  [model setHeader:advancedHeader
      forSectionWithIdentifier:SectionIdentifierAdvanced];
  [model addItem:[self voiceSearchDetailItem]
      toSectionWithIdentifier:SectionIdentifierAdvanced];
  [model addItem:[self privacyDetailItem]
      toSectionWithIdentifier:SectionIdentifierAdvanced];
  _articlesForYouItem = [self articlesForYouSwitchItem];
  [model addItem:_articlesForYouItem
      toSectionWithIdentifier:SectionIdentifierAdvanced];
  [model addItem:[self contentSettingsDetailItem]
      toSectionWithIdentifier:SectionIdentifierAdvanced];
  [model addItem:[self bandwidthManagementDetailItem]
      toSectionWithIdentifier:SectionIdentifierAdvanced];

  // Info Section
  [model addSectionWithIdentifier:SectionIdentifierInfo];
  [model addItem:[self aboutChromeDetailItem]
      toSectionWithIdentifier:SectionIdentifierInfo];

  // Debug Section
  if ([self hasDebugSection]) {
    [model addSectionWithIdentifier:SectionIdentifierDebug];
    CollectionViewTextItem* debugHeader =
        [[CollectionViewTextItem alloc] initWithType:ItemTypeHeader];
    debugHeader.text = @"Debug";
    debugHeader.textColor = [[MDCPalette greyPalette] tint500];
    [model setHeader:debugHeader
        forSectionWithIdentifier:SectionIdentifierDebug];
  }

  if (experimental_flags::IsMemoryDebuggingEnabled()) {
    _showMemoryDebugToolsItem = [self showMemoryDebugSwitchItem];
    [model addItem:_showMemoryDebugToolsItem
        toSectionWithIdentifier:SectionIdentifierDebug];
  }

#if CHROMIUM_BUILD && !defined(NDEBUG)
  [model addItem:[self viewSourceSwitchItem]
      toSectionWithIdentifier:SectionIdentifierDebug];
  [model addItem:[self logJavascriptConsoleSwitchItem]
      toSectionWithIdentifier:SectionIdentifierDebug];
  [model addItem:[self collectionViewCatalogDetailItem]
      toSectionWithIdentifier:SectionIdentifierDebug];
  [model addItem:[self tableViewCatalogDetailItem]
      toSectionWithIdentifier:SectionIdentifierDebug];
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)
}

#pragma mark - Model Items

- (CollectionViewItem*)signInTextItem {
  if (_signinPromoViewMediator) {
    SigninPromoItem* signinPromoItem =
        [[SigninPromoItem alloc] initWithType:ItemTypeSigninPromo];
    signinPromoItem.configurator =
        [_signinPromoViewMediator createConfigurator];
    [_signinPromoViewMediator signinPromoViewVisible];
    return signinPromoItem;
  }
  if (!_hasRecordedSigninImpression) {
    // Once the Settings are open, this button impression will at most be
    // recorded once until they are closed.
    signin_metrics::RecordSigninImpressionUserActionForAccessPoint(
        signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS);
    _hasRecordedSigninImpression = YES;
  }
  AccountSignInItem* signInTextItem =
      [[AccountSignInItem alloc] initWithType:ItemTypeSignInButton];
  signInTextItem.accessibilityIdentifier = kSettingsSignInCellId;
  UIImage* image = CircularImageFromImage(ios::GetChromeBrowserProvider()
                                              ->GetSigninResourcesProvider()
                                              ->GetDefaultAvatar(),
                                          kAccountProfilePhotoDimension);
  signInTextItem.image = image;
  return signInTextItem;
}

- (CollectionViewItem*)accountCellItem {
  CollectionViewAccountItem* identityAccountItem =
      [[CollectionViewAccountItem alloc] initWithType:ItemTypeAccount];
  identityAccountItem.accessoryType =
      MDCCollectionViewCellAccessoryDisclosureIndicator;
  identityAccountItem.accessibilityIdentifier = kSettingsAccountCellId;
  [self updateIdentityAccountItem:identityAccountItem];
  return identityAccountItem;
}

- (CollectionViewItem*)searchEngineDetailItem {
  NSString* defaultSearchEngineName =
      base::SysUTF16ToNSString(GetDefaultSearchEngineName(
          ios::TemplateURLServiceFactory::GetForBrowserState(_browserState)));

  _defaultSearchEngineItem =
      [self detailItemWithType:ItemTypeSearchEngine
                          text:l10n_util::GetNSString(
                                   IDS_IOS_SEARCH_ENGINE_SETTING_TITLE)
                    detailText:defaultSearchEngineName];
  _defaultSearchEngineItem.accessibilityIdentifier =
      kSettingsSearchEngineCellId;
  return _defaultSearchEngineItem;
}

- (CollectionViewItem*)savePasswordsDetailItem {
  _savePasswordsDetailItem =
      [self detailItemWithType:ItemTypeSavedPasswords
                          text:l10n_util::GetNSString(IDS_IOS_PASSWORDS)
                    detailText:nil];

  return _savePasswordsDetailItem;
}

- (CollectionViewItem*)autoFillDetailItem {
  BOOL autofillEnabled =
      _browserState->GetPrefs()->GetBoolean(autofill::prefs::kAutofillEnabled);
  NSString* autofillDetail = autofillEnabled
                                 ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
                                 : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
  _autoFillDetailItem =
      [self detailItemWithType:ItemTypeAutofill
                          text:l10n_util::GetNSString(IDS_IOS_AUTOFILL)
                    detailText:autofillDetail];

  return _autoFillDetailItem;
}

- (CollectionViewItem*)voiceSearchDetailItem {
  voice::SpeechInputLocaleConfig* localeConfig =
      voice::SpeechInputLocaleConfig::GetInstance();
  voice::SpeechInputLocale locale =
      _voiceLocaleCode.GetValue().length()
          ? localeConfig->GetLocaleForCode(_voiceLocaleCode.GetValue())
          : localeConfig->GetDefaultLocale();
  NSString* languageName = base::SysUTF16ToNSString(locale.display_name);
  _voiceSearchDetailItem =
      [self detailItemWithType:ItemTypeVoiceSearch
                          text:l10n_util::GetNSString(
                                   IDS_IOS_VOICE_SEARCH_SETTING_TITLE)
                    detailText:languageName];
  _voiceSearchDetailItem.accessibilityIdentifier = kSettingsVoiceSearchCellId;
  return _voiceSearchDetailItem;
}

- (CollectionViewItem*)privacyDetailItem {
  return
      [self detailItemWithType:ItemTypePrivacy
                          text:l10n_util::GetNSString(
                                   IDS_OPTIONS_ADVANCED_SECTION_TITLE_PRIVACY)
                    detailText:nil];
}

- (CollectionViewItem*)contentSettingsDetailItem {
  return [self
      detailItemWithType:ItemTypeContentSettings
                    text:l10n_util::GetNSString(IDS_IOS_CONTENT_SETTINGS_TITLE)
              detailText:nil];
}

- (CollectionViewItem*)bandwidthManagementDetailItem {
  return [self detailItemWithType:ItemTypeBandwidth
                             text:l10n_util::GetNSString(
                                      IDS_IOS_BANDWIDTH_MANAGEMENT_SETTINGS)
                       detailText:nil];
}

- (CollectionViewItem*)aboutChromeDetailItem {
  return [self detailItemWithType:ItemTypeAboutChrome
                             text:l10n_util::GetNSString(IDS_IOS_PRODUCT_NAME)
                       detailText:nil];
}

- (CollectionViewSwitchItem*)showMemoryDebugSwitchItem {
  CollectionViewSwitchItem* showMemoryDebugSwitchItem =
      [self switchItemWithType:ItemTypeMemoryDebugging
                         title:@"Show memory debug tools"
               withDefaultsKey:nil];
  showMemoryDebugSwitchItem.on = [_showMemoryDebugToolsEnabled value];

  return showMemoryDebugSwitchItem;
}

- (CollectionViewSwitchItem*)articlesForYouSwitchItem {
  CollectionViewSwitchItem* articlesForYouSwitchItem =
      [self switchItemWithType:ItemTypeArticlesForYou
                         title:l10n_util::GetNSString(
                                   IDS_IOS_CONTENT_SUGGESTIONS_SETTING_TITLE)
               withDefaultsKey:nil];
  articlesForYouSwitchItem.on = [_articlesEnabled value];

  return articlesForYouSwitchItem;
}
#if CHROMIUM_BUILD && !defined(NDEBUG)

- (CollectionViewSwitchItem*)viewSourceSwitchItem {
  return [self switchItemWithType:ItemTypeViewSource
                            title:@"View source menu"
                  withDefaultsKey:kDevViewSourceKey];
}

- (CollectionViewSwitchItem*)logJavascriptConsoleSwitchItem {
  return [self switchItemWithType:ItemTypeLogJavascript
                            title:@"Log JS"
                  withDefaultsKey:kLogJavascriptKey];
}

- (CollectionViewDetailItem*)collectionViewCatalogDetailItem {
  return [self detailItemWithType:ItemTypeCollectionCellCatalog
                             text:@"Collection Cell Catalog"
                       detailText:nil];
}

- (CollectionViewDetailItem*)tableViewCatalogDetailItem {
  return [self detailItemWithType:ItemTypeTableCellCatalog
                             text:@"TableView Cell Catalog"
                       detailText:nil];
}
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)

#pragma mark Item Updaters

- (void)updateSearchCell {
  NSString* defaultSearchEngineName =
      base::SysUTF16ToNSString(GetDefaultSearchEngineName(
          ios::TemplateURLServiceFactory::GetForBrowserState(_browserState)));

  _defaultSearchEngineItem.detailText = defaultSearchEngineName;
  [self reconfigureCellsForItems:@[ _defaultSearchEngineItem ]];
}

#pragma mark Item Constructors

- (CollectionViewDetailItem*)detailItemWithType:(NSInteger)type
                                           text:(NSString*)text
                                     detailText:(NSString*)detailText {
  CollectionViewDetailItem* detailItem =
      [[CollectionViewDetailItem alloc] initWithType:type];
  detailItem.text = text;
  detailItem.detailText = detailText;
  detailItem.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  detailItem.accessibilityTraits |= UIAccessibilityTraitButton;

  return detailItem;
}

- (CollectionViewSwitchItem*)switchItemWithType:(NSInteger)type
                                          title:(NSString*)title
                                withDefaultsKey:(NSString*)key {
  CollectionViewSwitchItem* switchItem =
      [[CollectionViewSwitchItem alloc] initWithType:type];
  switchItem.text = title;
  if (key) {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    switchItem.on = [defaults boolForKey:key];
  }

  return switchItem;
}

#pragma mark - UICollectionViewDataSource

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];

  if ([cell isKindOfClass:[CollectionViewDetailCell class]]) {
    CollectionViewDetailCell* detailCell =
        base::mac::ObjCCastStrict<CollectionViewDetailCell>(cell);
    if (itemType == ItemTypeSavedPasswords) {
      scoped_refptr<password_manager::PasswordStore> passwordStore =
          IOSChromePasswordStoreFactory::GetForBrowserState(
              _browserState, ServiceAccessType::EXPLICIT_ACCESS);
      if (!passwordStore) {
        // The password store factory returns a NULL password store if something
        // goes wrong during the password store initialization. Disable the save
        // passwords cell in this case.
        LOG(ERROR) << "Save passwords cell was disabled as the password store"
                      " cannot be created.";
        [detailCell setUserInteractionEnabled:NO];
        detailCell.textLabel.textColor = [[MDCPalette greyPalette] tint500];
        detailCell.detailTextLabel.textColor =
            [[MDCPalette greyPalette] tint400];
      }
    } else {
      [detailCell setUserInteractionEnabled:YES];
      detailCell.textLabel.textColor = [[MDCPalette greyPalette] tint900];
      detailCell.detailTextLabel.textColor = [[MDCPalette greyPalette] tint500];
    }
  }

  switch (itemType) {
    case ItemTypeMemoryDebugging: {
      CollectionViewSwitchCell* switchCell =
          base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
      [switchCell.switchView addTarget:self
                                action:@selector(memorySwitchToggled:)
                      forControlEvents:UIControlEventValueChanged];
      break;
    }
    case ItemTypeArticlesForYou: {
      CollectionViewSwitchCell* switchCell =
          base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
      [switchCell.switchView addTarget:self
                                action:@selector(articlesForYouSwitchToggled:)
                      forControlEvents:UIControlEventValueChanged];
      break;
    }
    case ItemTypeSigninPromo: {
      SigninPromoCell* signinPromoCell =
          base::mac::ObjCCast<SigninPromoCell>(cell);
      signinPromoCell.signinPromoView.delegate = _signinPromoViewMediator;
      break;
    }
    case ItemTypeViewSource: {
#if CHROMIUM_BUILD && !defined(NDEBUG)
      CollectionViewSwitchCell* switchCell =
          base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
      [switchCell.switchView addTarget:self
                                action:@selector(viewSourceSwitchToggled:)
                      forControlEvents:UIControlEventValueChanged];
#else
      NOTREACHED();
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)
      break;
    }
    case ItemTypeLogJavascript: {
#if CHROMIUM_BUILD && !defined(NDEBUG)
      CollectionViewSwitchCell* switchCell =
          base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
      [switchCell.switchView addTarget:self
                                action:@selector(logJSSwitchToggled:)
                      forControlEvents:UIControlEventValueChanged];
#else
      NOTREACHED();
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)
      break;
    }
    default:
      break;
  }

  return cell;
}

#pragma mark UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  id object = [self.collectionViewModel itemAtIndexPath:indexPath];
  if ([object respondsToSelector:@selector(isEnabled)] &&
      ![object performSelector:@selector(isEnabled)]) {
    // Don't perform any action if the cell isn't enabled.
    return;
  }

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];

  SettingsRootCollectionViewController* controller;

  switch (itemType) {
    case ItemTypeSignInButton:
      signin_metrics::RecordSigninUserActionForAccessPoint(
          signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS,
          signin_metrics::PromoAction::PROMO_ACTION_NO_SIGNIN_PROMO);
      [self showSignInWithIdentity:nil
                       promoAction:signin_metrics::PromoAction::
                                       PROMO_ACTION_NO_SIGNIN_PROMO
                        completion:nil];
      break;
    case ItemTypeAccount:
      controller = [[AccountsCollectionViewController alloc]
               initWithBrowserState:_browserState
          closeSettingsOnAddAccount:NO];
      break;
    case ItemTypeSearchEngine:
      controller = [[SearchEngineSettingsCollectionViewController alloc]
          initWithBrowserState:_browserState];
      break;
    case ItemTypeSavedPasswords: {
      controller = [[SavePasswordsCollectionViewController alloc]
          initWithBrowserState:_browserState];
      break;
    }
    case ItemTypeAutofill:
      controller = [[AutofillCollectionViewController alloc]
          initWithBrowserState:_browserState];
      break;
    case ItemTypeVoiceSearch:
      controller = [[VoicesearchCollectionViewController alloc]
          initWithPrefs:_browserState->GetPrefs()];
      break;
    case ItemTypePrivacy:
      controller = [[PrivacyCollectionViewController alloc]
          initWithBrowserState:_browserState];
      break;
    case ItemTypeContentSettings:
      controller = [[ContentSettingsCollectionViewController alloc]
          initWithBrowserState:_browserState];
      break;
    case ItemTypeBandwidth:
      controller = [[BandwidthManagementCollectionViewController alloc]
          initWithBrowserState:_browserState];
      break;
    case ItemTypeAboutChrome:
      controller = [[AboutChromeCollectionViewController alloc] init];
      break;
    case ItemTypeMemoryDebugging:
    case ItemTypeViewSource:
    case ItemTypeLogJavascript:
      // Taps on these don't do anything. They have a switch as accessory view
      // and only the switch is tappable.
      break;
    case ItemTypeCollectionCellCatalog:
      [self.settingsMainPageDispatcher showMaterialCellCatalog];
      break;
    case ItemTypeTableCellCatalog:
      [self.navigationController
          pushViewController:[[TableCellCatalogViewController alloc] init]
                    animated:YES];
      break;
    default:
      break;
  }

  if (controller) {
    controller.dispatcher = self.dispatcher;
    [self.navigationController pushViewController:controller animated:YES];
  }
}

#pragma mark MDCCollectionViewStylingDelegate

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];

  if (item.type == ItemTypeSigninPromo) {
    return [MDCCollectionViewCell
        cr_preferredHeightForWidth:CGRectGetWidth(collectionView.bounds)
                           forItem:item];
  }

  if (item.type == ItemTypeAccount) {
    return MDCCellDefaultTwoLineHeight;
  }

  if (item.type == ItemTypeSignInButton) {
    return MDCCellDefaultThreeLineHeight;
  }

  return MDCCellDefaultOneLineHeight;
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (type) {
    case ItemTypeLogJavascript:
    case ItemTypeMemoryDebugging:
    case ItemTypeSigninPromo:
    case ItemTypeViewSource:
      return YES;
    default:
      return NO;
  }
}

#pragma mark Switch Actions

- (void)memorySwitchToggled:(UISwitch*)sender {
  NSIndexPath* switchPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeMemoryDebugging
                                   sectionIdentifier:SectionIdentifierDebug];

  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);

  BOOL newSwitchValue = sender.isOn;
  switchItem.on = newSwitchValue;
  [_showMemoryDebugToolsEnabled setValue:newSwitchValue];
}

- (void)articlesForYouSwitchToggled:(UISwitch*)sender {
  NSIndexPath* switchPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeArticlesForYou
                                   sectionIdentifier:SectionIdentifierAdvanced];

  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);

  BOOL newSwitchValue = sender.isOn;
  switchItem.on = newSwitchValue;
  [_articlesEnabled setValue:newSwitchValue];
}

#if CHROMIUM_BUILD && !defined(NDEBUG)
- (void)viewSourceSwitchToggled:(UISwitch*)sender {
  NSIndexPath* switchPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeViewSource
                                   sectionIdentifier:SectionIdentifierDebug];

  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);

  BOOL newSwitchValue = sender.isOn;
  switchItem.on = newSwitchValue;
  [self setBooleanNSUserDefaultsValue:newSwitchValue forKey:kDevViewSourceKey];
}

- (void)logJSSwitchToggled:(UISwitch*)sender {
  NSIndexPath* switchPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeLogJavascript
                                   sectionIdentifier:SectionIdentifierDebug];

  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);

  BOOL newSwitchValue = sender.isOn;
  switchItem.on = newSwitchValue;
  [self setBooleanNSUserDefaultsValue:newSwitchValue forKey:kLogJavascriptKey];
}
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)

#pragma mark Private methods

// Sets the NSUserDefaults BOOL |value| for |key|.
- (void)setBooleanNSUserDefaultsValue:(BOOL)value forKey:(NSString*)key {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults setBool:value forKey:key];
  [defaults synchronize];
}

// Returns YES if a "Debug" section should be shown. This is always true for
// Chromium builds, but for official builds it is gated by an experimental flag
// because the "Debug" section should never be showing in stable channel.
- (BOOL)hasDebugSection {
#if CHROMIUM_BUILD && !defined(NDEBUG)
  return YES;
#else
  if (experimental_flags::IsMemoryDebuggingEnabled()) {
    return YES;
  }
  return NO;
#endif  // CHROMIUM_BUILD && !defined(NDEBUG)
}

// Updates the identity cell.
- (void)updateIdentityAccountItem:
    (CollectionViewAccountItem*)identityAccountItem {
  AuthenticationService* authService =
      AuthenticationServiceFactory::GetForBrowserState(_browserState);
  _identity = authService->GetAuthenticatedIdentity();
  if (!_identity) {
    // This could occur during the sign out process. Just ignore as the account
    // cell will be replaced by the "Sign in" button.
    return;
  }
  identityAccountItem.image = [self userAccountImage];
  identityAccountItem.text = [_identity userFullName];

  SyncSetupService* syncSetupService =
      SyncSetupServiceFactory::GetForBrowserState(_browserState);
  if (!syncSetupService->HasFinishedInitialSetup()) {
    identityAccountItem.detailText =
        l10n_util::GetNSString(IDS_IOS_SYNC_SETUP_IN_PROGRESS);
    identityAccountItem.shouldDisplayError = NO;
    return;
  }
  identityAccountItem.shouldDisplayError =
      !IsTransientSyncError(syncSetupService->GetSyncServiceState());
  if (identityAccountItem.shouldDisplayError) {
    identityAccountItem.detailText =
        GetSyncErrorDescriptionForBrowserState(_browserState);
  } else {
    identityAccountItem.detailText =
        syncSetupService->IsSyncEnabled()
            ? l10n_util::GetNSStringF(
                  IDS_IOS_SIGN_IN_TO_CHROME_SETTING_SYNCING,
                  base::SysNSStringToUTF16([_identity userEmail]))
            : l10n_util::GetNSString(
                  IDS_IOS_SIGN_IN_TO_CHROME_SETTING_SYNC_OFF);
  }
}

- (void)reloadAccountCell {
  if (![self.collectionViewModel hasItemForItemType:ItemTypeAccount
                                  sectionIdentifier:SectionIdentifierSignIn]) {
    return;
  }
  NSIndexPath* accountCellIndexPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeAccount
                                   sectionIdentifier:SectionIdentifierSignIn];
  CollectionViewAccountItem* identityAccountItem =
      base::mac::ObjCCast<CollectionViewAccountItem>(
          [self.collectionViewModel itemAtIndexPath:accountCellIndexPath]);
  if (identityAccountItem) {
    [self updateIdentityAccountItem:identityAccountItem];
    [self reconfigureCellsForItems:@[ identityAccountItem ]];
  }
}

#pragma mark - SigninPresenter

- (void)showSignin:(ShowSigninCommand*)command {
  [self.dispatcher showSignin:command baseViewController:self];
}

#pragma mark Sign in

- (void)showSignInWithIdentity:(ChromeIdentity*)identity
                   promoAction:(signin_metrics::PromoAction)promoAction
                    completion:(ShowSigninCommandCompletionCallback)completion {
  DCHECK(!self.signinInteractionCoordinator.isActive);
  if (!self.signinInteractionCoordinator) {
    self.signinInteractionCoordinator = [[SigninInteractionCoordinator alloc]
        initWithBrowserState:_browserState
                  dispatcher:self.dispatcher];
  }

  __weak SettingsCollectionViewController* weakSelf = self;
  [self.signinInteractionCoordinator
            signInWithIdentity:identity
                   accessPoint:signin_metrics::AccessPoint::
                                   ACCESS_POINT_SETTINGS
                   promoAction:promoAction
      presentingViewController:self.navigationController
                    completion:^(BOOL success) {
                      if (completion)
                        completion(success);
                      [weakSelf didFinishSignin:success];
                    }];
}

- (void)didFinishSignin:(BOOL)signedIn {
  // The sign-in is done. The sign-in promo cell or account cell can be
  // reloaded.
  if (!_settingsHasBeenDismissed)
    [self reloadData];
}

#pragma mark Material Cell Catalog

- (void)showMaterialCellCatalog {
  [self.navigationController
      pushViewController:[[MaterialCellCatalogViewController alloc] init]
                animated:YES];
}

#pragma mark NotificationBridgeDelegate

- (void)onSignInStateChanged {
  // While the sign-in interaction coordinator is presenting UI, the collection
  // view should not be updated. Otherwise, it would lead to have an UI glitch
  // either while the sign in UI is appearing or while it is disappearing. The
  // collection view will be reloaded once the animation is finished.
  // See: -[SettingsCollectionViewController didFinishSignin:].
  if (!self.signinInteractionCoordinator.isActive) {
    // Sign in state changes are rare. Just reload the entire collection when
    // this happens.
    [self reloadData];
  }
}

#pragma mark SettingsControllerProtocol

- (void)settingsWillBeDismissed {
  DCHECK(!_settingsHasBeenDismissed);
  _settingsHasBeenDismissed = YES;
  [self.signinInteractionCoordinator cancel];
  [_signinPromoViewMediator signinPromoViewRemoved];
  _signinPromoViewMediator = nil;
  [self stopBrowserStateServiceObservers];
}

#pragma mark SyncObserverModelBridge

- (void)onSyncStateChanged {
  [self reloadAccountCell];
}

#pragma mark - IdentityRefreshLogic

// Image used for loggedin user account that supports caching.
- (UIImage*)userAccountImage {
  UIImage* image = ios::GetChromeBrowserProvider()
                       ->GetChromeIdentityService()
                       ->GetCachedAvatarForIdentity(_identity);
  if (!image) {
    image = ios::GetChromeBrowserProvider()
                ->GetSigninResourcesProvider()
                ->GetDefaultAvatar();
    // No cached image, trigger a fetch, which will notify all observers
    // (including the corresponding AccountViewBase).
    ios::GetChromeBrowserProvider()
        ->GetChromeIdentityService()
        ->GetAvatarForIdentity(_identity, ^(UIImage*){
                               });
  }

  // If the currently used image has already been resized, use it.
  if (_resizedImage && _oldImage == image)
    return _resizedImage;

  _oldImage = image;

  // Resize the profile image.
  CGFloat dimension = kAccountProfilePhotoDimension;
  if (image.size.width != dimension || image.size.height != dimension) {
    image = ResizeImage(image, CGSizeMake(dimension, dimension),
                        ProjectionMode::kAspectFit);
  }
  _resizedImage = image;
  return _resizedImage;
}

#pragma mark ChromeIdentityServiceObserver

- (void)profileUpdate:(ChromeIdentity*)identity {
  if (identity == _identity) {
    [self reloadAccountCell];
  }
}

- (void)chromeIdentityServiceWillBeDestroyed {
  _identityServiceObserver.reset();
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  if (observableBoolean == _showMemoryDebugToolsEnabled) {
    // Update the Item.
    _showMemoryDebugToolsItem.on = [_showMemoryDebugToolsEnabled value];
    // Update the Cell.
    [self reconfigureCellsForItems:@[ _showMemoryDebugToolsItem ]];
  } else if (observableBoolean == _articlesEnabled) {
    _articlesForYouItem.on = [_articlesEnabled value];
    [self reconfigureCellsForItems:@[ _articlesForYouItem ]];
  } else {
    NOTREACHED();
  }
}

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (preferenceName == prefs::kVoiceSearchLocale) {
    voice::SpeechInputLocaleConfig* localeConfig =
        voice::SpeechInputLocaleConfig::GetInstance();
    voice::SpeechInputLocale locale =
        _voiceLocaleCode.GetValue().length()
            ? localeConfig->GetLocaleForCode(_voiceLocaleCode.GetValue())
            : localeConfig->GetDefaultLocale();
    NSString* languageName = base::SysUTF16ToNSString(locale.display_name);
    _voiceSearchDetailItem.detailText = languageName;
    [self reconfigureCellsForItems:@[ _voiceSearchDetailItem ]];
  }

  if (preferenceName == password_manager::prefs::kCredentialsEnableService) {
    BOOL savePasswordsEnabled =
        _browserState->GetPrefs()->GetBoolean(preferenceName);
    NSString* passwordsDetail =
        savePasswordsEnabled ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
                             : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);

    _savePasswordsDetailItem.detailText = passwordsDetail;
    [self reconfigureCellsForItems:@[ _savePasswordsDetailItem ]];
  }

  if (preferenceName == autofill::prefs::kAutofillEnabled) {
    BOOL autofillEnabled =
        _browserState->GetPrefs()->GetBoolean(preferenceName);
    NSString* autofillDetail =
        autofillEnabled ? l10n_util::GetNSString(IDS_IOS_SETTING_ON)
                        : l10n_util::GetNSString(IDS_IOS_SETTING_OFF);
    _autoFillDetailItem.detailText = autofillDetail;
    [self reconfigureCellsForItems:@[ _autoFillDetailItem ]];
  }
}

#pragma mark - SigninPromoViewConsumer

- (void)configureSigninPromoWithConfigurator:
            (SigninPromoViewConfigurator*)configurator
                             identityChanged:(BOOL)identityChanged {
  DCHECK(!self.signinInteractionCoordinator.isActive);
  if (![self.collectionViewModel hasItemForItemType:ItemTypeSigninPromo
                                  sectionIdentifier:SectionIdentifierSignIn]) {
    return;
  }
  NSIndexPath* signinPromoCellIndexPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeSigninPromo
                                   sectionIdentifier:SectionIdentifierSignIn];
  DCHECK(signinPromoCellIndexPath.item != NSNotFound);
  SigninPromoItem* signinPromoItem = base::mac::ObjCCast<SigninPromoItem>(
      [self.collectionViewModel itemAtIndexPath:signinPromoCellIndexPath]);
  if (signinPromoItem) {
    signinPromoItem.configurator = configurator;
    [self reconfigureCellsForItems:@[ signinPromoItem ]];
    if (identityChanged)
      [self.collectionViewLayout invalidateLayout];
  }
}

- (void)signinPromoViewMediator:(SigninPromoViewMediator*)mediator
    shouldOpenSigninWithIdentity:(ChromeIdentity*)identity
                     promoAction:(signin_metrics::PromoAction)promoAction
                      completion:
                          (ShowSigninCommandCompletionCallback)completion {
  [self showSignInWithIdentity:identity
                   promoAction:promoAction
                    completion:completion];
}

- (void)signinPromoViewMediatorCloseButtonWasTapped:
    (SigninPromoViewMediator*)mediator {
  [self reloadData];
}

@end
