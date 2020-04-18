// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/server/http_handler.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"  // For CHECK macros.
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/test/chromedriver/alert_commands.h"
#include "chrome/test/chromedriver/chrome/adb_impl.h"
#include "chrome/test/chromedriver/chrome/device_manager.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/net/url_request_context_getter.h"
#include "chrome/test/chromedriver/session.h"
#include "chrome/test/chromedriver/session_thread_map.h"
#include "chrome/test/chromedriver/util.h"
#include "chrome/test/chromedriver/version.h"
#include "net/server/http_server_request_info.h"
#include "net/server/http_server_response_info.h"
#include "url/url_util.h"

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif

namespace {

const char kLocalStorage[] = "localStorage";
const char kSessionStorage[] = "sessionStorage";
const char kShutdownPath[] = "shutdown";

}  // namespace

CommandMapping::CommandMapping(HttpMethod method,
                               const std::string& path_pattern,
                               const Command& command)
    : method(method), path_pattern(path_pattern), command(command) {}

CommandMapping::CommandMapping(const CommandMapping& other) = default;

CommandMapping::~CommandMapping() {}

HttpHandler::HttpHandler(const std::string& url_base)
    : url_base_(url_base),
      received_shutdown_(false),
      command_map_(new CommandMap()),
      weak_ptr_factory_(this) {}

HttpHandler::HttpHandler(
    const base::Closure& quit_func,
    const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    const std::string& url_base,
    int adb_port)
    : quit_func_(quit_func),
      url_base_(url_base),
      received_shutdown_(false),
      weak_ptr_factory_(this) {
#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool autorelease_pool;
#endif
  context_getter_ = new URLRequestContextGetter(io_task_runner);
  socket_factory_ = CreateSyncWebSocketFactory(context_getter_.get());
  adb_.reset(new AdbImpl(io_task_runner, adb_port));
  device_manager_.reset(new DeviceManager(adb_.get()));

  CommandMapping commands[] = {
      CommandMapping(
          kPost, internal::kNewSessionPathPattern,
          base::Bind(
              &ExecuteCreateSession, &session_thread_map_,
              WrapToCommand(
                  "InitSession",
                  base::Bind(&ExecuteInitSession,
                             InitSessionParams(context_getter_, socket_factory_,
                                               device_manager_.get()))))),
      CommandMapping(kDelete, "session/:sessionId",
                     base::Bind(&ExecuteSessionCommand, &session_thread_map_,
                                "Quit", base::Bind(&ExecuteQuit, false), true)),
      CommandMapping(kGet, "status", base::Bind(&ExecuteGetStatus)),
      CommandMapping(
          kGet, "session/:sessionId/timeouts",
          WrapToCommand("GetTimeouts", base::Bind(&ExecuteGetTimeouts))),
      CommandMapping(
          kPost, "session/:sessionId/timeouts",
          WrapToCommand("SetTimeout", base::Bind(&ExecuteSetTimeout))),
      CommandMapping(kPost, "session/:sessionId/url",
                     WrapToCommand("Navigate", base::Bind(&ExecuteGet))),
      CommandMapping(
          kGet, "session/:sessionId/url",
          WrapToCommand("GetUrl", base::Bind(&ExecuteGetCurrentUrl))),
      CommandMapping(kPost, "session/:sessionId/back",
                     WrapToCommand("GoBack", base::Bind(&ExecuteGoBack))),
      CommandMapping(kPost, "session/:sessionId/forward",
                     WrapToCommand("GoForward", base::Bind(&ExecuteGoForward))),

      CommandMapping(kPost, "session/:sessionId/refresh",
                     WrapToCommand("Refresh", base::Bind(&ExecuteRefresh))),
      CommandMapping(kGet, "session/:sessionId/title",
                     WrapToCommand("GetTitle", base::Bind(&ExecuteGetTitle))),
      CommandMapping(kGet, "session/:sessionId/window",
                     WrapToCommand("GetWindow",
                                   base::Bind(&ExecuteGetCurrentWindowHandle))),
      CommandMapping(kDelete, "session/:sessionId/window",
                     WrapToCommand("CloseWindow", base::Bind(&ExecuteClose))),
      CommandMapping(
          kPost, "session/:sessionId/window",
          WrapToCommand("SwitchToWindow", base::Bind(&ExecuteSwitchToWindow))),
      CommandMapping(
          kGet, "session/:sessionId/window/handles",
          WrapToCommand("GetWindows", base::Bind(&ExecuteGetWindowHandles))),
      CommandMapping(
          kPost, "session/:sessionId/frame",
          WrapToCommand("SwitchToFrame", base::Bind(&ExecuteSwitchToFrame))),
      CommandMapping(kPost, "session/:sessionId/frame/parent",
                     WrapToCommand("SwitchToParentFrame",
                                   base::Bind(&ExecuteSwitchToParentFrame))),
      CommandMapping(
          kGet, "session/:sessionId/window/rect",
          WrapToCommand("GetWindowRect", base::Bind(&ExecuteGetWindowRect))),

      // minimize/maximize oss version
      CommandMapping(
          kPost, "session/:sessionId/window/:windowHandle/maximize",
          WrapToCommand("MaximizeWindow", base::Bind(&ExecuteMaximizeWindow))),
      CommandMapping(
          kPost, "session/:sessionId/window/:windowHandle/minimize",
          WrapToCommand("MinimizeWindow", base::Bind(&ExecuteMinimizeWindow))),

      // minimize/maximize w3c version
      CommandMapping(
          kPost, "session/:sessionId/window/maximize",
          WrapToCommand("MaximizeWindow", base::Bind(&ExecuteMaximizeWindow))),
      CommandMapping(
          kPost, "session/:sessionId/window/minimize",
          WrapToCommand("MinimizeWindow", base::Bind(&ExecuteMinimizeWindow))),

      CommandMapping(kPost, "session/:sessionId/window/fullscreen",
                     WrapToCommand("FullscreenWindow",
                                   base::Bind(&ExecuteFullScreenWindow))),

      CommandMapping(kGet, "session/:sessionId/element/active",
                     WrapToCommand("GetActiveElement",
                                   base::Bind(&ExecuteGetActiveElement))),
      CommandMapping(
          kPost, "session/:sessionId/element",
          WrapToCommand("FindElement", base::Bind(&ExecuteFindElement, 50))),
      CommandMapping(
          kPost, "session/:sessionId/elements",
          WrapToCommand("FindElements", base::Bind(&ExecuteFindElements, 50))),
      CommandMapping(kPost, "session/:sessionId/element/:id/element",
                     WrapToCommand("FindChildElement",
                                   base::Bind(&ExecuteFindChildElement, 50))),
      CommandMapping(kPost, "session/:sessionId/element/:id/elements",
                     WrapToCommand("FindChildElements",
                                   base::Bind(&ExecuteFindChildElements, 50))),
      CommandMapping(kGet, "session/:sessionId/element/:id/selected",
                     WrapToCommand("IsElementSelected",
                                   base::Bind(&ExecuteIsElementSelected))),
      CommandMapping(kGet, "session/:sessionId/element/:id/attribute/:name",
                     WrapToCommand("GetElementAttribute",
                                   base::Bind(&ExecuteGetElementAttribute))),
      CommandMapping(
          kGet, "session/:sessionId/element/:id/css/:propertyName",
          WrapToCommand("GetElementCSSProperty",
                        base::Bind(&ExecuteGetElementValueOfCSSProperty))),
      CommandMapping(
          kGet, "session/:sessionId/element/:id/text",
          WrapToCommand("GetElementText", base::Bind(&ExecuteGetElementText))),
      CommandMapping(kGet, "session/:sessionId/element/:id/name",
                     WrapToCommand("GetElementTagName",
                                   base::Bind(&ExecuteGetElementTagName))),
      CommandMapping(kGet, "session/:sessionId/element/:id/enabled",
                     WrapToCommand("IsElementEnabled",
                                   base::Bind(&ExecuteIsElementEnabled))),
      CommandMapping(
          kPost, "session/:sessionId/element/:id/click",
          WrapToCommand("ClickElement", base::Bind(&ExecuteClickElement))),
      CommandMapping(
          kPost, "session/:sessionId/element/:id/clear",
          WrapToCommand("ClearElement", base::Bind(&ExecuteClearElement))),

      CommandMapping(
          kPost, "session/:sessionId/element/:id/value",
          WrapToCommand("TypeElement", base::Bind(&ExecuteSendKeysToElement))),
      CommandMapping(
          kGet, "session/:sessionId/source",
          WrapToCommand("GetSource", base::Bind(&ExecuteGetPageSource))),
      CommandMapping(
          kPost, "session/:sessionId/execute/sync",
          WrapToCommand("ExecuteScript", base::Bind(&ExecuteExecuteScript))),
      CommandMapping(kPost, "session/:sessionId/execute/async",
                     WrapToCommand("ExecuteAsyncScript",
                                   base::Bind(&ExecuteExecuteAsyncScript))),
      CommandMapping(
          kGet, "session/:sessionId/cookie",
          WrapToCommand("GetCookies", base::Bind(&ExecuteGetCookies))),
      CommandMapping(
          kGet, "session/:sessionId/cookie/:name",
          WrapToCommand("GetNamedCookie", base::Bind(&ExecuteGetNamedCookie))),
      CommandMapping(kPost, "session/:sessionId/cookie",
                     WrapToCommand("AddCookie", base::Bind(&ExecuteAddCookie))),
      CommandMapping(kDelete, "session/:sessionId/cookie",
                     WrapToCommand("DeleteAllCookies",
                                   base::Bind(&ExecuteDeleteAllCookies))),
      CommandMapping(
          kDelete, "session/:sessionId/cookie/:name",
          WrapToCommand("DeleteCookie", base::Bind(&ExecuteDeleteCookie))),
      CommandMapping(kPost, "session/:sessionId/actions",
                     WrapToCommand("PerformActions",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(kDelete, "session/:sessionId/actions",
                     WrapToCommand("DeleteActions",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kPost, "session/:sessionId/alert/dismiss",
          WrapToCommand("DismissAlert",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteDismissAlert)))),
      CommandMapping(
          kPost, "session/:sessionId/alert/accept",
          WrapToCommand("AcceptAlert",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteAcceptAlert)))),
      CommandMapping(
          kGet, "session/:sessionId/alert/text",
          WrapToCommand("GetAlertMessage",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteGetAlertText)))),
      CommandMapping(
          kPost, "session/:sessionId/alert/text",
          WrapToCommand("SetAlertPrompt",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteSetAlertText)))),
      CommandMapping(
          kGet, "session/:sessionId/screenshot",
          WrapToCommand("Screenshot", base::Bind(&ExecuteScreenshot))),

      // Json wire protocol only
      // similar to /session/{session id}/window
      CommandMapping(kGet, "session/:sessionId/window_handle",
                     WrapToCommand("GetWindow",
                                   base::Bind(&ExecuteGetCurrentWindowHandle))),

      // similar to /session/{session id}/window/handles
      CommandMapping(
          kGet, "session/:sessionId/window_handles",
          WrapToCommand("GetWindows", base::Bind(&ExecuteGetWindowHandles))),

      // similar to /session/{session id}/alert/dismiss
      CommandMapping(
          kPost, "session/:sessionId/dismiss_alert",
          WrapToCommand("DismissAlert",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteDismissAlert)))),

      // similar to /session/{session id}/alert/accept
      CommandMapping(
          kPost, "session/:sessionId/accept_alert",
          WrapToCommand("AcceptAlert",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteAcceptAlert)))),
      // similar to /session/{session id}/alert/text
      CommandMapping(
          kGet, "session/:sessionId/alert_text",
          WrapToCommand("GetAlertMessage",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteGetAlertText)))),
      // similar to /session/{session id}/alert/text
      CommandMapping(
          kPost, "session/:sessionId/alert_text",
          WrapToCommand("SetAlertPrompt",
                        base::Bind(&ExecuteAlertCommand,
                                   base::Bind(&ExecuteSetAlertText)))),
      // similar to /session/{session id}/execute/sync
      CommandMapping(
          kPost, "session/:sessionId/execute",
          WrapToCommand("ExecuteScript", base::Bind(&ExecuteExecuteScript))),

      // similar to /session/{session id}/execute/async
      CommandMapping(kPost, "session/:sessionId/execute_async",
                     WrapToCommand("ExecuteAsyncScript",
                                   base::Bind(&ExecuteExecuteAsyncScript))),

      // similar to /session/{session id}/execute/sync but GET request
      CommandMapping(kPost, "session/:sessionId/element/active",
                     WrapToCommand("GetActiveElement",
                                   base::Bind(&ExecuteGetActiveElement))),

      CommandMapping(
          kGet, "sessions",
          base::Bind(&ExecuteGetSessions,
                     WrapToCommand("GetSessions",
                                   base::Bind(&ExecuteGetSessionCapabilities)),
                     &session_thread_map_)),

      CommandMapping(kGet, "session/:sessionId",
                     WrapToCommand("GetSessionCapabilities",
                                   base::Bind(&ExecuteGetSessionCapabilities))),
      CommandMapping(
          kPost, "session/:sessionId/element/:id/submit",
          WrapToCommand("SubmitElement", base::Bind(&ExecuteSubmitElement))),
      CommandMapping(kGet, "session/:sessionId/element/:id/displayed",
                     WrapToCommand("IsElementDisplayed",
                                   base::Bind(&ExecuteIsElementDisplayed))),
      CommandMapping(kGet, "session/:sessionId/element/:id/location",
                     WrapToCommand("GetElementLocation",
                                   base::Bind(&ExecuteGetElementLocation))),
      CommandMapping(
          kGet, "session/:sessionId/element/:id/rect",
          WrapToCommand("GetElementRect", base::Bind(&ExecuteGetElementRect))),
      CommandMapping(
          kGet, "session/:sessionId/element/:id/location_in_view",
          WrapToCommand(
              "GetElementLocationInView",
              base::Bind(&ExecuteGetElementLocationOnceScrolledIntoView))),
      CommandMapping(
          kGet, "session/:sessionId/element/:id/size",
          WrapToCommand("GetElementSize", base::Bind(&ExecuteGetElementSize))),
      CommandMapping(
          kGet, "session/:sessionId/element/:id/equals/:other",
          WrapToCommand("IsElementEqual", base::Bind(&ExecuteElementEquals))),

      CommandMapping(
          kGet, "session/:sessionId/window/:windowHandle/size",
          WrapToCommand("GetWindowSize", base::Bind(&ExecuteGetWindowSize))),
      CommandMapping(kGet, "session/:sessionId/window/:windowHandle/position",
                     WrapToCommand("GetWindowPosition",
                                   base::Bind(&ExecuteGetWindowPosition))),
      CommandMapping(
          kPost, "session/:sessionId/window/:windowHandle/size",
          WrapToCommand("SetWindowSize", base::Bind(&ExecuteSetWindowSize))),
      CommandMapping(
          kPost, "session/:sessionId/window/rect",
          WrapToCommand("SetWindowRect", base::Bind(&ExecuteSetWindowRect))),
      CommandMapping(kPost, "session/:sessionId/window/:windowHandle/position",
                     WrapToCommand("SetWindowPosition",
                                   base::Bind(&ExecuteSetWindowPosition))),
      CommandMapping(
          kPost, "session/:sessionId/timeouts/implicit_wait",
          WrapToCommand("SetImplicitWait", base::Bind(&ExecuteImplicitlyWait))),
      CommandMapping(kPost, "session/:sessionId/timeouts/async_script",
                     WrapToCommand("SetScriptTimeout",
                                   base::Bind(&ExecuteSetScriptTimeout))),
      CommandMapping(
          kGet, "session/:sessionId/location",
          WrapToCommand("GetGeolocation", base::Bind(&ExecuteGetLocation))),
      CommandMapping(
          kPost, "session/:sessionId/location",
          WrapToCommand("SetGeolocation", base::Bind(&ExecuteSetLocation))),
      CommandMapping(kGet, "session/:sessionId/application_cache/status",
                     base::Bind(&ExecuteGetStatus)),
      CommandMapping(
          kGet, "session/:sessionId/local_storage/key/:key",
          WrapToCommand("GetLocalStorageItem",
                        base::Bind(&ExecuteGetStorageItem, kLocalStorage))),
      CommandMapping(
          kDelete, "session/:sessionId/local_storage/key/:key",
          WrapToCommand("RemoveLocalStorageItem",
                        base::Bind(&ExecuteRemoveStorageItem, kLocalStorage))),
      CommandMapping(
          kGet, "session/:sessionId/local_storage",
          WrapToCommand("GetLocalStorageKeys",
                        base::Bind(&ExecuteGetStorageKeys, kLocalStorage))),
      CommandMapping(
          kPost, "session/:sessionId/local_storage",
          WrapToCommand("SetLocalStorageKeys",
                        base::Bind(&ExecuteSetStorageItem, kLocalStorage))),
      CommandMapping(
          kDelete, "session/:sessionId/local_storage",
          WrapToCommand("ClearLocalStorage",
                        base::Bind(&ExecuteClearStorage, kLocalStorage))),
      CommandMapping(
          kGet, "session/:sessionId/local_storage/size",
          WrapToCommand("GetLocalStorageSize",
                        base::Bind(&ExecuteGetStorageSize, kLocalStorage))),
      CommandMapping(
          kGet, "session/:sessionId/session_storage/key/:key",
          WrapToCommand("GetSessionStorageItem",
                        base::Bind(&ExecuteGetStorageItem, kSessionStorage))),
      CommandMapping(kDelete, "session/:sessionId/session_storage/key/:key",
                     WrapToCommand("RemoveSessionStorageItem",
                                   base::Bind(&ExecuteRemoveStorageItem,
                                              kSessionStorage))),
      CommandMapping(
          kGet, "session/:sessionId/session_storage",
          WrapToCommand("GetSessionStorageKeys",
                        base::Bind(&ExecuteGetStorageKeys, kSessionStorage))),
      CommandMapping(
          kPost, "session/:sessionId/session_storage",
          WrapToCommand("SetSessionStorageItem",
                        base::Bind(&ExecuteSetStorageItem, kSessionStorage))),
      CommandMapping(
          kDelete, "session/:sessionId/session_storage",
          WrapToCommand("ClearSessionStorage",
                        base::Bind(&ExecuteClearStorage, kSessionStorage))),
      CommandMapping(
          kGet, "session/:sessionId/session_storage/size",
          WrapToCommand("GetSessionStorageSize",
                        base::Bind(&ExecuteGetStorageSize, kSessionStorage))),
      CommandMapping(kGet, "session/:sessionId/orientation",
                     WrapToCommand("GetScreenOrientation",
                                   base::Bind(&ExecuteGetScreenOrientation))),
      CommandMapping(kPost, "session/:sessionId/orientation",
                     WrapToCommand("SetScreenOrientation",
                                   base::Bind(&ExecuteSetScreenOrientation))),
      CommandMapping(kPost, "session/:sessionId/click",
                     WrapToCommand("Click", base::Bind(&ExecuteMouseClick))),
      CommandMapping(
          kPost, "session/:sessionId/doubleclick",
          WrapToCommand("DoubleClick", base::Bind(&ExecuteMouseDoubleClick))),
      CommandMapping(
          kPost, "session/:sessionId/buttondown",
          WrapToCommand("MouseDown", base::Bind(&ExecuteMouseButtonDown))),
      CommandMapping(
          kPost, "session/:sessionId/buttonup",
          WrapToCommand("MouseUp", base::Bind(&ExecuteMouseButtonUp))),
      CommandMapping(
          kPost, "session/:sessionId/moveto",
          WrapToCommand("MouseMove", base::Bind(&ExecuteMouseMoveTo))),
      CommandMapping(
          kPost, "session/:sessionId/keys",
          WrapToCommand("Type", base::Bind(&ExecuteSendKeysToActiveElement))),
      CommandMapping(kGet, "session/:sessionId/ime/available_engines",
                     WrapToCommand("GetAvailableEngines",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(kGet, "session/:sessionId/ime/active_engine",
                     WrapToCommand("GetActiveEngine",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kGet, "session/:sessionId/ime/activated",
          WrapToCommand("Activated", base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(kPost, "session/:sessionId/ime/deactivate",
                     WrapToCommand("Deactivate",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kPost, "session/:sessionId/ime/activate",
          WrapToCommand("Activate", base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(kPost, "session/:sessionId/touch/click",
                     WrapToCommand("Tap", base::Bind(&ExecuteTouchSingleTap))),
      CommandMapping(kPost, "session/:sessionId/touch/down",
                     WrapToCommand("TouchDown", base::Bind(&ExecuteTouchDown))),
      CommandMapping(kPost, "session/:sessionId/touch/up",
                     WrapToCommand("TouchUp", base::Bind(&ExecuteTouchUp))),
      CommandMapping(kPost, "session/:sessionId/touch/move",
                     WrapToCommand("TouchMove", base::Bind(&ExecuteTouchMove))),
      CommandMapping(
          kPost, "session/:sessionId/touch/scroll",
          WrapToCommand("TouchScroll", base::Bind(&ExecuteTouchScroll))),
      CommandMapping(
          kPost, "session/:sessionId/touch/doubleclick",
          WrapToCommand("TouchDoubleTap", base::Bind(&ExecuteTouchDoubleTap))),
      CommandMapping(
          kPost, "session/:sessionId/touch/longclick",
          WrapToCommand("TouchLongPress", base::Bind(&ExecuteTouchLongPress))),
      CommandMapping(kPost, "session/:sessionId/touch/flick",
                     WrapToCommand("TouchFlick", base::Bind(&ExecuteFlick))),
      CommandMapping(kPost, "session/:sessionId/log",
                     WrapToCommand("GetLog", base::Bind(&ExecuteGetLog))),
      CommandMapping(kGet, "session/:sessionId/log/types",
                     WrapToCommand("GetLogTypes",
                                   base::Bind(&ExecuteGetAvailableLogTypes))),

      // chromedriver only
      CommandMapping(kPost, "session/:sessionId/chromium/launch_app",
                     WrapToCommand("LaunchApp", base::Bind(&ExecuteLaunchApp))),
      CommandMapping(kGet, "session/:sessionId/alert",
                     WrapToCommand("IsAlertOpen",
                                   base::Bind(&ExecuteAlertCommand,
                                              base::Bind(&ExecuteGetAlert)))),
      CommandMapping(
          kGet, "session/:sessionId/chromium/heap_snapshot",
          WrapToCommand("HeapSnapshot", base::Bind(&ExecuteTakeHeapSnapshot))),
      CommandMapping(
          kPost, "session/:sessionId/visible",
          WrapToCommand("Visible", base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kGet, "session/:sessionId/visible",
          WrapToCommand("Visible", base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kPost, "session/:sessionId/file",
          WrapToCommand("UploadFile", base::Bind(&ExecuteUploadFile))),
      CommandMapping(kGet, "session/:sessionId/element/:id/value",
                     WrapToCommand("GetElementValue",
                                   base::Bind(&ExecuteGetElementValue))),
      CommandMapping(
          kPost, "session/:sessionId/element/:id/hover",
          WrapToCommand("HoverElement", base::Bind(&ExecuteHoverOverElement))),
      CommandMapping(
          kPost, "session/:sessionId/element/:id/drag",
          WrapToCommand("Drag", base::Bind(&ExecuteUnimplementedCommand))),

      CommandMapping(kPost, "session/:sessionId/execute_sql",
                     WrapToCommand("ExecuteSql",
                                   base::Bind(&ExecuteUnimplementedCommand))),

      CommandMapping(kGet, "session/:sessionId/network_connection",
                     WrapToCommand("GetNetworkConnection",
                                   base::Bind(&ExecuteGetNetworkConnection))),
      CommandMapping(kGet, "session/:sessionId/chromium/network_conditions",
                     WrapToCommand("GetNetworkConditions",
                                   base::Bind(&ExecuteGetNetworkConditions))),
      CommandMapping(kPost, "session/:sessionId/chromium/network_conditions",
                     WrapToCommand("SetNetworkConditions",
                                   base::Bind(&ExecuteSetNetworkConditions))),
      CommandMapping(
          kDelete, "session/:sessionId/chromium/network_conditions",
          WrapToCommand("DeleteNetworkConditions",
                        base::Bind(&ExecuteDeleteNetworkConditions))),

      CommandMapping(kGet, "session/:sessionId/browser_connection",
                     WrapToCommand("GetBrowserConnection",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(kPost, "session/:sessionId/browser_connection",
                     WrapToCommand("SetBrowserConnection",
                                   base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kDelete, "session/:sessionId/orientation",
          WrapToCommand("DeleteScreenOrientation",
                        base::Bind(&ExecuteDeleteScreenOrientation))),
      CommandMapping(
          kPost, "Logs",
          WrapToCommand("Logs", base::Bind(&ExecuteUnimplementedCommand))),
      CommandMapping(
          kGet, kShutdownPath,
          base::Bind(&ExecuteQuitAll,
                     WrapToCommand("QuitAll", base::Bind(&ExecuteQuit, true)),
                     &session_thread_map_)),
      CommandMapping(
          kPost, kShutdownPath,
          base::Bind(&ExecuteQuitAll,
                     WrapToCommand("QuitAll", base::Bind(&ExecuteQuit, true)),
                     &session_thread_map_)),
      CommandMapping(kGet, "session/:sessionId/is_loading",
                     WrapToCommand("IsLoading", base::Bind(&ExecuteIsLoading))),
      CommandMapping(kGet, "session/:sessionId/autoreport",
                     WrapToCommand("IsAutoReporting",
                                   base::Bind(&ExecuteIsAutoReporting))),
      CommandMapping(kPost, "session/:sessionId/autoreport",
                     WrapToCommand("SetAutoReporting",
                                   base::Bind(&ExecuteSetAutoReporting))),
      CommandMapping(
          kPost, "session/:sessionId/touch/pinch",
          WrapToCommand("TouchPinch", base::Bind(&ExecuteTouchPinch))),
      CommandMapping(
          kPost, "session/:sessionId/chromium/send_command",
          WrapToCommand("SendCommand", base::Bind(&ExecuteSendCommand))),
      CommandMapping(
          kPost, "session/:sessionId/chromium/send_command_and_get_result",
          WrapToCommand("SendCommandAndGetResult",
                        base::Bind(&ExecuteSendCommandAndGetResult))),

      // mobile json protocol cmomand
      CommandMapping(kPost, "session/:sessionId/network_connection",
                     WrapToCommand("SetNetworkConnection",
                                   base::Bind(&ExecuteSetNetworkConnection))),
  };
  command_map_.reset(
      new CommandMap(commands, commands + arraysize(commands)));
}

HttpHandler::~HttpHandler() {}

void HttpHandler::Handle(const net::HttpServerRequestInfo& request,
                         const HttpResponseSenderFunc& send_response_func) {
  CHECK(thread_checker_.CalledOnValidThread());

  if (received_shutdown_)
    return;

  std::string path = request.path;
  if (!base::StartsWith(path, url_base_, base::CompareCase::SENSITIVE)) {
    std::unique_ptr<net::HttpServerResponseInfo> response(
        new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
    response->SetBody("unhandled request", "text/plain");
    send_response_func.Run(std::move(response));
    return;
  }

  path.erase(0, url_base_.length());

  HandleCommand(request, path, send_response_func);

  if (path == kShutdownPath)
    received_shutdown_ = true;
}

Command HttpHandler::WrapToCommand(
    const char* name,
    const SessionCommand& session_command) {
  return base::Bind(&ExecuteSessionCommand,
                    &session_thread_map_,
                    name,
                    session_command,
                    false);
}

Command HttpHandler::WrapToCommand(
    const char* name,
    const WindowCommand& window_command) {
  return WrapToCommand(name, base::Bind(&ExecuteWindowCommand, window_command));
}

Command HttpHandler::WrapToCommand(
    const char* name,
    const ElementCommand& element_command) {
  return WrapToCommand(name,
                       base::Bind(&ExecuteElementCommand, element_command));
}

void HttpHandler::HandleCommand(
    const net::HttpServerRequestInfo& request,
    const std::string& trimmed_path,
    const HttpResponseSenderFunc& send_response_func) {
  base::DictionaryValue params;
  std::string session_id;
  CommandMap::const_iterator iter = command_map_->begin();
  while (true) {
    if (iter == command_map_->end()) {
      std::unique_ptr<net::HttpServerResponseInfo> response(
          new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      response->SetBody("unknown command: " + trimmed_path, "text/plain");
      send_response_func.Run(std::move(response));
      return;
    }
    if (internal::MatchesCommand(
            request.method, trimmed_path, *iter, &session_id, &params)) {
      break;
    }
    ++iter;
  }

  if (request.data.length()) {
    base::DictionaryValue* body_params;
    std::unique_ptr<base::Value> parsed_body =
        base::JSONReader::Read(request.data);
    if (!parsed_body || !parsed_body->GetAsDictionary(&body_params)) {
      std::unique_ptr<net::HttpServerResponseInfo> response(
          new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      response->SetBody("missing command parameters", "text/plain");
      send_response_func.Run(std::move(response));
      return;
    }
    params.MergeDictionary(body_params);
  }

  iter->command.Run(params,
                    session_id,
                    base::Bind(&HttpHandler::PrepareResponse,
                               weak_ptr_factory_.GetWeakPtr(),
                               trimmed_path,
                               send_response_func));
}

void HttpHandler::PrepareResponse(
    const std::string& trimmed_path,
    const HttpResponseSenderFunc& send_response_func,
    const Status& status,
    std::unique_ptr<base::Value> value,
    const std::string& session_id,
    bool w3c_compliant) {
  CHECK(thread_checker_.CalledOnValidThread());
  std::unique_ptr<net::HttpServerResponseInfo> response;
  if (w3c_compliant)
    response = PrepareStandardResponse(
        trimmed_path, status, std::move(value), session_id);
  else
    response = PrepareLegacyResponse(trimmed_path,
                                     status,
                                     std::move(value),
                                     session_id);
  send_response_func.Run(std::move(response));
  if (trimmed_path == kShutdownPath)
    quit_func_.Run();
}

std::unique_ptr<net::HttpServerResponseInfo> HttpHandler::PrepareLegacyResponse(
    const std::string& trimmed_path,
    const Status& status,
    std::unique_ptr<base::Value> value,
    const std::string& session_id) {
  if (status.code() == kUnknownCommand) {
    std::unique_ptr<net::HttpServerResponseInfo> response(
        new net::HttpServerResponseInfo(net::HTTP_NOT_IMPLEMENTED));
    response->SetBody("unimplemented command: " + trimmed_path, "text/plain");
    return response;
  }

  if (status.IsError()) {
    Status full_status(status);
    full_status.AddDetails(base::StringPrintf(
        "Driver info: chromedriver=%s,platform=%s %s %s",
        kChromeDriverVersion,
        base::SysInfo::OperatingSystemName().c_str(),
        base::SysInfo::OperatingSystemVersion().c_str(),
        base::SysInfo::OperatingSystemArchitecture().c_str()));
    std::unique_ptr<base::DictionaryValue> error(new base::DictionaryValue());
    error->SetString("message", full_status.message());
    value = std::move(error);
  }
  if (!value)
    value = std::make_unique<base::Value>();

  base::DictionaryValue body_params;
  body_params.SetInteger("status", status.code());
  body_params.Set("value", std::move(value));
  body_params.SetString("sessionId", session_id);
  std::string body;
  base::JSONWriter::WriteWithOptions(
      body_params, base::JSONWriter::OPTIONS_OMIT_DOUBLE_TYPE_PRESERVATION,
      &body);
  std::unique_ptr<net::HttpServerResponseInfo> response(
      new net::HttpServerResponseInfo(net::HTTP_OK));
  response->SetBody(body, "application/json; charset=utf-8");
  return response;
}

std::unique_ptr<net::HttpServerResponseInfo>
HttpHandler::PrepareStandardResponse(
    const std::string& trimmed_path,
    const Status& status,
    std::unique_ptr<base::Value> value,
    const std::string& session_id) {
  std::unique_ptr<net::HttpServerResponseInfo> response;
  switch (status.code()) {
    case kOk:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_OK));
      break;
    // error codes
    case kElementNotInteractable:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kInvalidArgument:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kInvalidCookieDomain:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kInvalidElementState:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kInvalidSelector:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kJavaScriptError:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kMoveTargetOutOfBounds:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
    case kNoSuchAlert:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kNoSuchCookie:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kNoSuchElement:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kNoSuchFrame:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kNoSuchWindow:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kScriptTimeout:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_REQUEST_TIMEOUT));
      break;
    case kSessionNotCreatedException:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
    case kStaleElementReference:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kTimeout:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_REQUEST_TIMEOUT));
      break;
    case kUnableToSetCookie:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
    case kUnexpectedAlertOpen:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
    case kUnknownCommand:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;
    case kUnknownError:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
    case kUnsupportedOperation:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
    case kTargetDetached:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_NOT_FOUND));
      break;

    // TODO(kereliuk): evaluate the usage of these as they relate to the spec
    case kElementNotVisible:
    case kXPathLookupError:
    case kNoSuchExecutionContext:
      response.reset(new net::HttpServerResponseInfo(net::HTTP_BAD_REQUEST));
      break;
    case kNoSuchSession:
    case kChromeNotReachable:
    case kDisconnected:
    case kForbidden:
    case kTabCrashed:
      response.reset(
          new net::HttpServerResponseInfo(net::HTTP_INTERNAL_SERVER_ERROR));
      break;
  }

  if (!value)
    value = std::make_unique<base::Value>();

  base::DictionaryValue body_params;
  if (status.IsError()){
    // Separates status default message from additional details.
    std::vector<std::string> status_details = base::SplitString(
        status.message(), ":\n", base::TRIM_WHITESPACE,
        base::SPLIT_WANT_NONEMPTY);
    std::string message;
    for (size_t i=1; i<status_details.size();++i)
      message += status_details[i];
    std::unique_ptr<base::DictionaryValue> inner_params(
        new base::DictionaryValue());
    inner_params->SetString("error", status_details[0]);
    inner_params->SetString("message", message);
    inner_params->SetString("stacktrace", status.stack_trace());
    body_params.SetDictionary("value", std::move(inner_params));
  } else {
    body_params.Set("value", std::move(value));
  }

  std::string body;
  base::JSONWriter::WriteWithOptions(
      body_params, base::JSONWriter::OPTIONS_OMIT_DOUBLE_TYPE_PRESERVATION,
      &body);
  response->SetBody(body, "application/json; charset=utf-8");
  return response;
}


namespace internal {

const char kNewSessionPathPattern[] = "session";

bool MatchesMethod(HttpMethod command_method, const std::string& method) {
  std::string lower_method = base::ToLowerASCII(method);
  switch (command_method) {
    case kGet:
      return lower_method == "get";
    case kPost:
      return lower_method == "post" || lower_method == "put";
    case kDelete:
      return lower_method == "delete";
  }
  return false;
}

bool MatchesCommand(const std::string& method,
                    const std::string& path,
                    const CommandMapping& command,
                    std::string* session_id,
                    base::DictionaryValue* out_params) {
  if (!MatchesMethod(command.method, method))
    return false;

  std::vector<std::string> path_parts = base::SplitString(
      path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::vector<std::string> command_path_parts = base::SplitString(
      command.path_pattern, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (path_parts.size() != command_path_parts.size())
    return false;

  base::DictionaryValue params;
  for (size_t i = 0; i < path_parts.size(); ++i) {
    CHECK(command_path_parts[i].length());
    if (command_path_parts[i][0] == ':') {
      std::string name = command_path_parts[i];
      name.erase(0, 1);
      CHECK(name.length());
      url::RawCanonOutputT<base::char16> output;
      url::DecodeURLEscapeSequences(
          path_parts[i].data(), path_parts[i].length(), &output);
      std::string decoded = base::UTF16ToASCII(
          base::string16(output.data(), output.length()));
      // Due to crbug.com/533361, the url decoding libraries decodes all of the
      // % escape sequences except for %%. We need to handle this case manually.
      // So, replacing all the instances of "%%" with "%".
      base::ReplaceSubstringsAfterOffset(&decoded, 0 , "%%" , "%");
      if (name == "sessionId")
        *session_id = decoded;
      else
        params.SetString(name, decoded);
    } else if (command_path_parts[i] != path_parts[i]) {
      return false;
    }
  }
  out_params->MergeDictionary(&params);
  return true;
}

}  // namespace internal
