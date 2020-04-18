// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// On Mac, one can't make shortcuts with command-line arguments. Instead, we
// produce small app bundles which locate the Chromium framework and load it,
// passing the appropriate data. This is the entry point into the framework for
// those app bundles.

#import <Cocoa/Cocoa.h>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#import "base/mac/launch_services_util.h"
#include "base/mac/mac_logging.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/mac/app_mode_common.h"
#include "chrome/common/mac/app_shim_messages.h"
#include "chrome/grit/generated_resources.h"
#include "ipc/ipc_channel_mojo.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_message.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/peer_connection.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Timeout in seconds to wait for a reply for the initial Apple Event. Note that
// kAEDefaultTimeout on Mac is "about one minute" according to Apple's
// documentation, but is no longer supported for asynchronous Apple Events.
const int kPingChromeTimeoutSeconds = 60;

const app_mode::ChromeAppModeInfo* g_info;
base::Thread* g_io_thread = NULL;

}  // namespace

class AppShimController;

// An application delegate to catch user interactions and send the appropriate
// IPC messages to Chrome.
@interface AppShimDelegate : NSObject<NSApplicationDelegate> {
 @private
  AppShimController* appShimController_;  // Weak, initially NULL.
  BOOL terminateNow_;
  BOOL terminateRequested_;
  std::vector<base::FilePath> filesToOpenAtStartup_;
}

// The controller is initially NULL. Setting it indicates to the delegate that
// the controller has finished initialization.
- (void)setController:(AppShimController*)controller;

// Gets files that were queued because the controller was not ready.
// Returns whether any FilePaths were added to |out|.
- (BOOL)getFilesToOpenAtStartup:(std::vector<base::FilePath>*)out;

// If the controller is ready, this sends a FocusApp with the files to open.
// Otherwise, this adds the files to |filesToOpenAtStartup_|.
// Takes an array of NSString*.
- (void)openFiles:(NSArray*)filename;

// Terminate immediately. This is necessary as we override terminate: to send
// a QuitApp message.
- (void)terminateNow;

@end

// The AppShimController is responsible for communication with the main Chrome
// process, and generally controls the lifetime of the app shim process.
class AppShimController : public IPC::Listener {
 public:
  AppShimController();
  ~AppShimController() override;

  // Called when the main Chrome process responds to the Apple Event ping that
  // was sent, or when the ping fails (if |success| is false).
  void OnPingChromeReply(bool success);

  // Called |kPingChromeTimeoutSeconds| after startup, to allow a timeout on the
  // ping event to be detected.
  void OnPingChromeTimeout();

  // Connects to Chrome and sends a LaunchApp message.
  void Init();

  // Create a channel from |socket_path| and send a LaunchApp message.
  void CreateChannelAndSendLaunchApp(const base::FilePath& socket_path);

  // Builds main menu bar items.
  void SetUpMenu();

  void SendSetAppHidden(bool hidden);

  void SendQuitApp();

  // Called when the app is activated, e.g. by clicking on it in the dock, by
  // dropping a file on the dock icon, or by Cmd+Tabbing to it.
  // Returns whether the message was sent.
  bool SendFocusApp(apps::AppShimFocusType focus_type,
                    const std::vector<base::FilePath>& files);

 private:
  // IPC::Listener implemetation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelError() override;

  // If Chrome failed to launch the app, |success| will be false and the app
  // shim process should die.
  void OnLaunchAppDone(apps::AppShimLaunchResult result);

  // Hide this app.
  void OnHide();

  // Set this app to the unhidden state. Happens when an app window shows
  // itself.
  void OnUnhideWithoutActivation();

  // Requests user attention.
  void OnRequestUserAttention();
  void OnSetUserAttention(apps::AppShimAttentionType attention_type);

  // Terminates the app shim process.
  void Close();

  base::FilePath user_data_dir_;
  mojo::edk::PeerConnection peer_connection_;
  std::unique_ptr<IPC::ChannelProxy> channel_;
  base::scoped_nsobject<AppShimDelegate> delegate_;
  bool launch_app_done_;
  bool ping_chrome_reply_received_;
  NSInteger attention_request_id_;

  DISALLOW_COPY_AND_ASSIGN(AppShimController);
};

AppShimController::AppShimController()
    : delegate_([[AppShimDelegate alloc] init]),
      launch_app_done_(false),
      ping_chrome_reply_received_(false),
      attention_request_id_(0) {
  // Since AppShimController is created before the main message loop starts,
  // NSApp will not be set, so use sharedApplication.
  NSApplication* sharedApplication = [NSApplication sharedApplication];
  [sharedApplication setDelegate:delegate_];
}

AppShimController::~AppShimController() {
  // Un-set the delegate since NSApplication does not retain it.
  NSApplication* sharedApplication = [NSApplication sharedApplication];
  [sharedApplication setDelegate:nil];
}

void AppShimController::OnPingChromeReply(bool success) {
  ping_chrome_reply_received_ = true;
  if (!success) {
    [NSApp terminate:nil];
    return;
  }

  Init();
}

void AppShimController::OnPingChromeTimeout() {
  if (!ping_chrome_reply_received_)
    [NSApp terminate:nil];
}

void AppShimController::Init() {
  DCHECK(g_io_thread);

  SetUpMenu();

  // Chrome will relaunch shims when relaunching apps.
  [NSApp disableRelaunchOnLogin];

  // The user_data_dir for shims actually contains the app_data_path.
  // I.e. <user_data_dir>/<profile_dir>/Web Applications/_crx_extensionid/
  user_data_dir_ = g_info->user_data_dir.DirName().DirName().DirName();
  CHECK(!user_data_dir_.empty());

  base::FilePath symlink_path =
      user_data_dir_.Append(app_mode::kAppShimSocketSymlinkName);

  base::FilePath socket_path;
  if (!base::ReadSymbolicLink(symlink_path, &socket_path)) {
    // The path in the user data dir is not a symlink, try connecting directly.
    CreateChannelAndSendLaunchApp(symlink_path);
    return;
  }

  app_mode::VerifySocketPermissions(socket_path);

  CreateChannelAndSendLaunchApp(socket_path);
}

void AppShimController::CreateChannelAndSendLaunchApp(
    const base::FilePath& socket_path) {
  channel_ = IPC::ChannelProxy::Create(
      IPC::ChannelMojo::CreateClientFactory(
          peer_connection_.Connect(mojo::edk::ConnectionParams(
              mojo::edk::TransportProtocol::kLegacy,
              mojo::edk::CreateClientHandle(
                  mojo::edk::NamedPlatformHandle(socket_path.value())))),
          g_io_thread->task_runner().get(),
          base::ThreadTaskRunnerHandle::Get()),
      this, g_io_thread->task_runner().get(),
      base::ThreadTaskRunnerHandle::Get());

  bool launched_by_chrome = base::CommandLine::ForCurrentProcess()->HasSwitch(
      app_mode::kLaunchedByChromeProcessId);
  apps::AppShimLaunchType launch_type = launched_by_chrome ?
          apps::APP_SHIM_LAUNCH_REGISTER_ONLY : apps::APP_SHIM_LAUNCH_NORMAL;

  [delegate_ setController:this];

  std::vector<base::FilePath> files;
  [delegate_ getFilesToOpenAtStartup:&files];

  channel_->Send(new AppShimHostMsg_LaunchApp(
      g_info->profile_dir, g_info->app_mode_id, launch_type, files));
}

void AppShimController::SetUpMenu() {
  NSString* title = base::SysUTF16ToNSString(g_info->app_mode_name);

  // Create a main menu since [NSApp mainMenu] is nil.
  base::scoped_nsobject<NSMenu> main_menu([[NSMenu alloc] initWithTitle:title]);

  // The title of the first item is replaced by OSX with the name of the app and
  // bold styling. Create a dummy item for this and make it hidden.
  NSMenuItem* dummy_item = [main_menu addItemWithTitle:title
                                                action:nil
                                         keyEquivalent:@""];
  base::scoped_nsobject<NSMenu> dummy_submenu(
      [[NSMenu alloc] initWithTitle:title]);
  [dummy_item setSubmenu:dummy_submenu];
  [dummy_item setHidden:YES];

  // Construct an unbolded app menu, to match how it appears in the Chrome menu
  // bar when the app is focused.
  NSMenuItem* item = [main_menu addItemWithTitle:title
                                          action:nil
                                   keyEquivalent:@""];
  base::scoped_nsobject<NSMenu> submenu([[NSMenu alloc] initWithTitle:title]);
  [item setSubmenu:submenu];

  // Add a quit entry.
  NSString* quit_localized_string =
      l10n_util::GetNSStringF(IDS_EXIT_MAC, g_info->app_mode_name);
  [submenu addItemWithTitle:quit_localized_string
                     action:@selector(terminate:)
              keyEquivalent:@"q"];

  // Add File, Edit, and Window menus. These are just here to make the
  // transition smoother, i.e. from another application to the shim then to
  // Chrome.
  [main_menu addItemWithTitle:l10n_util::GetNSString(IDS_FILE_MENU_MAC)
                       action:nil
                keyEquivalent:@""];
  [main_menu addItemWithTitle:l10n_util::GetNSString(IDS_EDIT_MENU_MAC)
                       action:nil
                keyEquivalent:@""];
  [main_menu addItemWithTitle:l10n_util::GetNSString(IDS_WINDOW_MENU_MAC)
                       action:nil
                keyEquivalent:@""];

  [NSApp setMainMenu:main_menu];
}

void AppShimController::SendQuitApp() {
  channel_->Send(new AppShimHostMsg_QuitApp);
}

bool AppShimController::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AppShimController, message)
    IPC_MESSAGE_HANDLER(AppShimMsg_LaunchApp_Done, OnLaunchAppDone)
    IPC_MESSAGE_HANDLER(AppShimMsg_Hide, OnHide)
    IPC_MESSAGE_HANDLER(AppShimMsg_UnhideWithoutActivation,
                        OnUnhideWithoutActivation)
    IPC_MESSAGE_HANDLER(AppShimMsg_RequestUserAttention, OnRequestUserAttention)
    IPC_MESSAGE_HANDLER(AppShimMsg_SetUserAttention, OnSetUserAttention)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AppShimController::OnChannelError() {
  Close();
}

void AppShimController::OnLaunchAppDone(apps::AppShimLaunchResult result) {
  if (result != apps::APP_SHIM_LAUNCH_SUCCESS) {
    Close();
    return;
  }

  std::vector<base::FilePath> files;
  if ([delegate_ getFilesToOpenAtStartup:&files])
    SendFocusApp(apps::APP_SHIM_FOCUS_OPEN_FILES, files);

  launch_app_done_ = true;
}

void AppShimController::OnHide() {
  [NSApp hide:nil];
}

void AppShimController::OnUnhideWithoutActivation() {
  [NSApp unhideWithoutActivation];
}

void AppShimController::OnRequestUserAttention() {
  OnSetUserAttention(apps::APP_SHIM_ATTENTION_INFORMATIONAL);
}

void AppShimController::OnSetUserAttention(
    apps::AppShimAttentionType attention_type) {
  switch (attention_type) {
    case apps::APP_SHIM_ATTENTION_CANCEL:
      [NSApp cancelUserAttentionRequest:attention_request_id_];
      attention_request_id_ = 0;
      break;
    case apps::APP_SHIM_ATTENTION_CRITICAL:
      attention_request_id_ = [NSApp requestUserAttention:NSCriticalRequest];
      break;
    case apps::APP_SHIM_ATTENTION_INFORMATIONAL:
      attention_request_id_ =
          [NSApp requestUserAttention:NSInformationalRequest];
      break;
    case apps::APP_SHIM_ATTENTION_NUM_TYPES:
      NOTREACHED();
  }
}

void AppShimController::Close() {
  [delegate_ terminateNow];
}

bool AppShimController::SendFocusApp(apps::AppShimFocusType focus_type,
                                     const std::vector<base::FilePath>& files) {
  if (launch_app_done_) {
    channel_->Send(new AppShimHostMsg_FocusApp(focus_type, files));
    return true;
  }

  return false;
}

void AppShimController::SendSetAppHidden(bool hidden) {
  channel_->Send(new AppShimHostMsg_SetAppHidden(hidden));
}

@implementation AppShimDelegate

- (BOOL)getFilesToOpenAtStartup:(std::vector<base::FilePath>*)out {
  if (filesToOpenAtStartup_.empty())
    return NO;

  out->insert(out->end(),
              filesToOpenAtStartup_.begin(),
              filesToOpenAtStartup_.end());
  filesToOpenAtStartup_.clear();
  return YES;
}

- (void)setController:(AppShimController*)controller {
  appShimController_ = controller;
}

- (void)openFiles:(NSArray*)filenames {
  std::vector<base::FilePath> filePaths;
  for (NSString* filename in filenames)
    filePaths.push_back(base::mac::NSStringToFilePath(filename));

  // If the AppShimController is ready, try to send a FocusApp. If that fails,
  // (e.g. if launching has not finished), enqueue the files.
  if (appShimController_ &&
      appShimController_->SendFocusApp(apps::APP_SHIM_FOCUS_OPEN_FILES,
                                       filePaths)) {
    return;
  }

  filesToOpenAtStartup_.insert(filesToOpenAtStartup_.end(),
                               filePaths.begin(),
                               filePaths.end());
}

- (BOOL)application:(NSApplication*)app
           openFile:(NSString*)filename {
  [self openFiles:@[filename]];
  return YES;
}

- (void)application:(NSApplication*)app
          openFiles:(NSArray*)filenames {
  [self openFiles:filenames];
  [app replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (BOOL)applicationOpenUntitledFile:(NSApplication*)app {
  if (appShimController_) {
    return appShimController_->SendFocusApp(apps::APP_SHIM_FOCUS_REOPEN,
                                            std::vector<base::FilePath>());
  }

  return NO;
}

- (void)applicationWillBecomeActive:(NSNotification*)notification {
  if (appShimController_) {
    appShimController_->SendFocusApp(apps::APP_SHIM_FOCUS_NORMAL,
                                     std::vector<base::FilePath>());
  }
}

- (NSApplicationTerminateReply)
    applicationShouldTerminate:(NSApplication*)sender {
  if (terminateNow_ || !appShimController_)
    return NSTerminateNow;

  appShimController_->SendQuitApp();
  // Wait for the channel to close before terminating.
  terminateRequested_ = YES;
  return NSTerminateLater;
}

- (void)applicationWillHide:(NSNotification*)notification {
  if (appShimController_)
    appShimController_->SendSetAppHidden(true);
}

- (void)applicationWillUnhide:(NSNotification*)notification {
  if (appShimController_)
    appShimController_->SendSetAppHidden(false);
}

- (void)terminateNow {
  if (terminateRequested_) {
    [NSApp replyToApplicationShouldTerminate:NSTerminateNow];
    return;
  }

  terminateNow_ = YES;
  [NSApp terminate:nil];
}

@end

//-----------------------------------------------------------------------------

// A ReplyEventHandler is a helper class to send an Apple Event to a process
// and call a callback when the reply returns.
//
// This is used to 'ping' the main Chrome process -- once Chrome has sent back
// an Apple Event reply, it's guaranteed that it has opened the IPC channel
// that the app shim will connect to.
@interface ReplyEventHandler : NSObject {
  base::Callback<void(bool)> onReply_;
  AEDesc replyEvent_;
}
// Sends an Apple Event to the process identified by the bundle identifier,
// and calls |replyFn| when the reply is received. Internally this
// creates a ReplyEventHandler, which will delete itself once the reply event
// has been received.
+ (void)pingProcessAndCall:(base::Callback<void(bool)>)replyFn;
@end

@interface ReplyEventHandler (PrivateMethods)
// Initialise the reply event handler. Doesn't register any handlers until
// |-pingProcess:| is called. |replyFn| is the function to be called when the
// Apple Event reply arrives.
- (id)initWithCallback:(base::Callback<void(bool)>)replyFn;

// Sends an Apple Event ping to the process identified by the bundle
// identifier and registers to listen for a reply.
- (void)pingProcess;

// Called when a response is received from the target process for the ping sent
// by |-pingProcess:|.
- (void)message:(NSAppleEventDescriptor*)event
      withReply:(NSAppleEventDescriptor*)reply;

// Calls |onReply_|, passing it |success| to specify whether the ping was
// successful.
- (void)closeWithSuccess:(bool)success;
@end

@implementation ReplyEventHandler
+ (void)pingProcessAndCall:(base::Callback<void(bool)>)replyFn {
  // The object will release itself when the reply arrives, or possibly earlier
  // if an unrecoverable error occurs.
  ReplyEventHandler* handler =
      [[ReplyEventHandler alloc] initWithCallback:replyFn];
  [handler pingProcess];
}
@end

@implementation ReplyEventHandler (PrivateMethods)
- (id)initWithCallback:(base::Callback<void(bool)>)replyFn {
  if ((self = [super init])) {
    onReply_ = replyFn;
  }
  return self;
}

- (void)pingProcess {
  // Register the reply listener.
  NSAppleEventManager* em = [NSAppleEventManager sharedAppleEventManager];
  [em setEventHandler:self
          andSelector:@selector(message:withReply:)
        forEventClass:kCoreEventClass
           andEventID:kAEAnswer];

  // Craft the Apple Event to send.
  NSString* chromeBundleId = [base::mac::OuterBundle() bundleIdentifier];
  NSAppleEventDescriptor* target = [NSAppleEventDescriptor
      descriptorWithDescriptorType:typeApplicationBundleID
                              data:[chromeBundleId
                                       dataUsingEncoding:NSUTF8StringEncoding]];

  NSAppleEventDescriptor* initialEvent = [NSAppleEventDescriptor
      appleEventWithEventClass:app_mode::kAEChromeAppClass
                       eventID:app_mode::kAEChromeAppPing
              targetDescriptor:target
                      returnID:kAutoGenerateReturnID
                 transactionID:kAnyTransactionID];

  // Note that AESendMessage effectively ignores kAEDefaultTimeout, because this
  // call does not pass kAEWantReceipt (which is deprecated and unsupported on
  // Mac). Instead, rely on OnPingChromeTimeout().
  OSStatus status = AESendMessage([initialEvent aeDesc], &replyEvent_,
                                  kAEQueueReply, kAEDefaultTimeout);
  if (status != noErr) {
    OSSTATUS_LOG(ERROR, status) << "AESendMessage";
    [self closeWithSuccess:false];
  }
}

- (void)message:(NSAppleEventDescriptor*)event
      withReply:(NSAppleEventDescriptor*)reply {
  [self closeWithSuccess:true];
}

- (void)closeWithSuccess:(bool)success {
  onReply_.Run(success);
  NSAppleEventManager* em = [NSAppleEventManager sharedAppleEventManager];
  [em removeEventHandlerForEventClass:kCoreEventClass andEventID:kAEAnswer];
  [self release];
}
@end

//-----------------------------------------------------------------------------

extern "C" {

// |ChromeAppModeStart()| is the point of entry into the framework from the app
// mode loader. There are cases where the Chromium framework may have changed in
// a way that is incompatible with an older shim (e.g. change to libc++ library
// linking). The function name is versioned to provide a way to force shim
// upgrades if they are launched before an updated version of Chromium can
// upgrade them; the old shim will not be able to dyload the new
// ChromeAppModeStart, so it will fall back to the upgrade path. See
// https://crbug.com/561205.
__attribute__((visibility("default")))
int ChromeAppModeStart_v4(const app_mode::ChromeAppModeInfo* info);

}  // extern "C"

int ChromeAppModeStart_v4(const app_mode::ChromeAppModeInfo* info) {
  base::CommandLine::Init(info->argc, info->argv);

  base::mac::ScopedNSAutoreleasePool scoped_pool;
  base::AtExitManager exit_manager;
  chrome::RegisterPathProvider();

  if (info->major_version < app_mode::kCurrentChromeAppModeInfoMajorVersion) {
    RAW_LOG(ERROR, "App Mode Loader too old.");
    return 1;
  }
  if (info->major_version > app_mode::kCurrentChromeAppModeInfoMajorVersion) {
    RAW_LOG(ERROR, "Browser Framework too old to load App Shortcut.");
    return 1;
  }

  g_info = info;

  // Set bundle paths. This loads the bundles.
  base::mac::SetOverrideOuterBundlePath(g_info->chrome_outer_bundle_path);
  base::mac::SetOverrideFrameworkBundlePath(
      g_info->chrome_versioned_path.Append(chrome::kFrameworkName));

  // Calculate the preferred locale used by Chrome.
  // We can't use l10n_util::OverrideLocaleWithCocoaLocale() because it calls
  // [base::mac::OuterBundle() preferredLocalizations] which gets localizations
  // from the bundle of the running app (i.e. it is equivalent to
  // [[NSBundle mainBundle] preferredLocalizations]) instead of the target
  // bundle.
  NSArray* preferred_languages = [NSLocale preferredLanguages];
  NSArray* supported_languages = [base::mac::OuterBundle() localizations];
  std::string preferred_localization;
  for (NSString* language in preferred_languages) {
    if ([supported_languages containsObject:language]) {
      preferred_localization = base::SysNSStringToUTF8(language);
      break;
    }
  }
  std::string locale = l10n_util::NormalizeLocale(
      l10n_util::GetApplicationLocale(preferred_localization));

  // Load localized strings.
  ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, NULL, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);

  // Launch the IO thread.
  base::Thread::Options io_thread_options;
  io_thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  base::Thread *io_thread = new base::Thread("CrAppShimIO");
  io_thread->StartWithOptions(io_thread_options);
  g_io_thread = io_thread;

  mojo::edk::Init();
  mojo::edk::ScopedIPCSupport ipc_support(
      io_thread->task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::FAST);

  // Find already running instances of Chrome.
  pid_t pid = -1;
  std::string chrome_process_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          app_mode::kLaunchedByChromeProcessId);
  if (!chrome_process_id.empty()) {
    if (!base::StringToInt(chrome_process_id, &pid))
      LOG(FATAL) << "Invalid PID: " << chrome_process_id;
  } else {
    NSString* chrome_bundle_id = [base::mac::OuterBundle() bundleIdentifier];
    NSArray* existing_chrome = [NSRunningApplication
        runningApplicationsWithBundleIdentifier:chrome_bundle_id];
    if ([existing_chrome count] > 0)
      pid = [[existing_chrome objectAtIndex:0] processIdentifier];
  }

  AppShimController controller;
  base::MessageLoopForUI main_message_loop;
  base::PlatformThread::SetName("CrAppShimMain");

  // In tests, launching Chrome does nothing, and we won't get a ping response,
  // so just assume the socket exists.
  if (pid == -1 &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          app_mode::kLaunchedForTest)) {
    // Launch Chrome if it isn't already running.
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    command_line.AppendSwitch(switches::kSilentLaunch);

    // If the shim is the app launcher, pass --show-app-list when starting a new
    // Chrome process to inform startup codepaths and load the correct profile.
    if (info->app_mode_id == app_mode::kAppListModeId) {
      command_line.AppendSwitch(switches::kShowAppList);
    } else {
      command_line.AppendSwitchPath(switches::kProfileDirectory,
                                    info->profile_dir);
    }

    base::Process app = base::mac::OpenApplicationWithPath(
        base::mac::OuterBundlePath(), command_line, NSWorkspaceLaunchDefault);
    if (!app.IsValid())
      return 1;

    base::Callback<void(bool)> on_ping_chrome_reply = base::Bind(
        &AppShimController::OnPingChromeReply, base::Unretained(&controller));

    // This code abuses the fact that Apple Events sent before the process is
    // fully initialized don't receive a reply until its run loop starts. Once
    // the reply is received, Chrome will have opened its IPC port, guaranteed.
    [ReplyEventHandler pingProcessAndCall:on_ping_chrome_reply];

    main_message_loop.task_runner()->PostDelayedTask(
        FROM_HERE, base::Bind(&AppShimController::OnPingChromeTimeout,
                              base::Unretained(&controller)),
        base::TimeDelta::FromSeconds(kPingChromeTimeoutSeconds));
  } else {
    // Chrome already running. Proceed to init. This could still fail if Chrome
    // is still starting up or shutting down, but the process will exit quickly,
    // which is preferable to waiting for the Apple Event to timeout after one
    // minute.
    main_message_loop.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&AppShimController::Init, base::Unretained(&controller)));
  }

  base::RunLoop().Run();
  return 0;
}
