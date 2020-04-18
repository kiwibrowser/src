// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_ui.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/browser/accessibility/accessibility_tree_formatter.h"
#include "content/browser/accessibility/accessibility_tree_formatter_blink.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/browser/webui/web_ui_data_source_impl.h"
#include "content/common/view_message_enums.h"
#include "content/grit/content_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "net/base/escape.h"
#include "ui/accessibility/platform/ax_platform_node.h"

static const char kTargetsDataFile[] = "targets-data.json";

static const char kProcessIdField[]  = "processId";
static const char kRouteIdField[]  = "routeId";
static const char kUrlField[]  = "url";
static const char kNameField[]  = "name";
static const char kFaviconUrlField[] = "favicon_url";
static const char kPidField[]  = "pid";
static const char kAccessibilityModeField[] = "a11y_mode";

// Global flags
static const char kInternal[] = "internal";
static const char kNative[] = "native";
static const char kWeb[] = "web";
static const char kText[] = "text";
static const char kScreenReader[] = "screenreader";
static const char kHTML[] = "html";

// Possible global flag values
static const char kOff[]= "off";
static const char kOn[] = "on";
static const char kDisabled[] = "disabled";

namespace content {

namespace {

bool g_show_internal_accessibility_tree = false;

std::unique_ptr<base::DictionaryValue> BuildTargetDescriptor(
    const GURL& url,
    const std::string& name,
    const GURL& favicon_url,
    int process_id,
    int route_id,
    ui::AXMode accessibility_mode,
    base::ProcessHandle handle = base::kNullProcessHandle) {
  std::unique_ptr<base::DictionaryValue> target_data(
      new base::DictionaryValue());
  target_data->SetInteger(kProcessIdField, process_id);
  target_data->SetInteger(kRouteIdField, route_id);
  target_data->SetString(kUrlField, url.spec());
  target_data->SetString(kNameField, net::EscapeForHTML(name));
  target_data->SetInteger(kPidField, base::GetProcId(handle));
  target_data->SetString(kFaviconUrlField, favicon_url.spec());
  target_data->SetInteger(kAccessibilityModeField, accessibility_mode.mode());
  return target_data;
}

std::unique_ptr<base::DictionaryValue> BuildTargetDescriptor(
    RenderViewHost* rvh) {
  WebContentsImpl* web_contents = static_cast<WebContentsImpl*>(
      WebContents::FromRenderViewHost(rvh));
  ui::AXMode accessibility_mode;

  std::string title;
  GURL url;
  GURL favicon_url;
  if (web_contents) {
    // TODO(nasko): Fix the following code to use a consistent set of data
    // across the URL, title, and favicon.
    url = web_contents->GetURL();
    title = base::UTF16ToUTF8(web_contents->GetTitle());
    NavigationController& controller = web_contents->GetController();
    NavigationEntry* entry = controller.GetVisibleEntry();
    if (entry != nullptr && entry->GetURL().is_valid())
      favicon_url = entry->GetFavicon().url;
    accessibility_mode = web_contents->GetAccessibilityMode();
  }

  return BuildTargetDescriptor(url,
                               title,
                               favicon_url,
                               rvh->GetProcess()->GetID(),
                               rvh->GetRoutingID(),
                               accessibility_mode);
}

bool HandleAccessibilityRequestCallback(
    BrowserContext* current_context,
    const std::string& path,
    const WebUIDataSource::GotDataCallback& callback) {
  if (path != kTargetsDataFile)
    return false;
  std::unique_ptr<base::ListValue> rvh_list(new base::ListValue());

  std::unique_ptr<RenderWidgetHostIterator> widgets(
      RenderWidgetHost::GetRenderWidgetHosts());

  while (RenderWidgetHost* widget = widgets->GetNextHost()) {
    // Ignore processes that don't have a connection, such as crashed tabs.
    if (!widget->GetProcess()->HasConnection())
      continue;
    RenderViewHost* rvh = RenderViewHost::From(widget);
    if (!rvh)
      continue;
    // Ignore views that are never visible, like background pages.
    if (static_cast<RenderViewHostImpl*>(rvh)->GetDelegate()->IsNeverVisible())
      continue;
    BrowserContext* context = rvh->GetProcess()->GetBrowserContext();
    if (context != current_context)
      continue;

    rvh_list->Append(BuildTargetDescriptor(rvh));
  }

  base::DictionaryValue data;
  data.Set("list", std::move(rvh_list));
  ui::AXMode mode =
      BrowserAccessibilityStateImpl::GetInstance()->accessibility_mode();
  bool disabled = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableRendererAccessibility);
  bool native = mode.has_mode(ui::AXMode::kNativeAPIs);
  bool web = mode.has_mode(ui::AXMode::kWebContents);
  bool text = mode.has_mode(ui::AXMode::kInlineTextBoxes);
  bool screenreader = mode.has_mode(ui::AXMode::kScreenReader);
  bool html = mode.has_mode(ui::AXMode::kHTML);

  // The "native" and "web" flags are disabled if
  // --disable-renderer-accessibility is set.
  data.SetString(kNative, disabled ? kDisabled : (native ? kOn : kOff));
  data.SetString(kWeb, disabled ? kDisabled : (web ? kOn : kOff));

  // The "text", "screenreader", and "html" flags are only meaningful if
  // "web" is enabled.
  data.SetString(kText, web ? (text ? kOn : kOff) : kDisabled);
  data.SetString(kScreenReader, web ? (screenreader ? kOn : kOff) : kDisabled);
  data.SetString(kHTML, web ? (html ? kOn : kOff) : kDisabled);

  data.SetString(kInternal,
                 g_show_internal_accessibility_tree ? kOn : kOff);

  std::string json_string;
  base::JSONWriter::Write(data, &json_string);

  callback.Run(base::RefCountedString::TakeString(&json_string));
  return true;
}

std::string RecursiveDumpAXPlatformNodeAsString(ui::AXPlatformNode* node,
                                                int indent) {
  std::string str(2 * indent, '+');
  str += node->GetDelegate()->GetData().ToString() + "\n";
  for (int i = 0; i < node->GetDelegate()->GetChildCount(); i++) {
    gfx::NativeViewAccessible child = node->GetDelegate()->ChildAtIndex(i);
    ui::AXPlatformNode* child_node =
        ui::AXPlatformNode::FromNativeViewAccessible(child);
    if (child_node)
      str += RecursiveDumpAXPlatformNodeAsString(child_node, indent + 1);
  }
  return str;
}

}  // namespace

AccessibilityUI::AccessibilityUI(WebUI* web_ui) : WebUIController(web_ui) {
  // Set up the chrome://accessibility source.
  WebUIDataSourceImpl* html_source = static_cast<WebUIDataSourceImpl*>(
      WebUIDataSource::Create(kChromeUIAccessibilityHost));

  // Add required resources.
  html_source->SetJsonPath("strings.js");
  html_source->AddResourcePath("accessibility.css", IDR_ACCESSIBILITY_CSS);
  html_source->AddResourcePath("accessibility.js", IDR_ACCESSIBILITY_JS);
  html_source->SetDefaultResource(IDR_ACCESSIBILITY_HTML);
  html_source->SetRequestFilter(
      base::Bind(&HandleAccessibilityRequestCallback,
                 web_ui->GetWebContents()->GetBrowserContext()));

  html_source->UseGzip({kTargetsDataFile});

  BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  WebUIDataSource::Add(browser_context, html_source);

  web_ui->AddMessageHandler(std::make_unique<AccessibilityUIMessageHandler>());
}

AccessibilityUI::~AccessibilityUI() {}

AccessibilityUIMessageHandler::AccessibilityUIMessageHandler() {}

AccessibilityUIMessageHandler::~AccessibilityUIMessageHandler() {}

void AccessibilityUIMessageHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  web_ui()->RegisterMessageCallback(
      "toggleAccessibility",
      base::BindRepeating(&AccessibilityUIMessageHandler::ToggleAccessibility,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setGlobalFlag",
      base::BindRepeating(&AccessibilityUIMessageHandler::SetGlobalFlag,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "requestWebContentsTree",
      base::BindRepeating(
          &AccessibilityUIMessageHandler::RequestWebContentsTree,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "requestNativeUITree",
      base::BindRepeating(&AccessibilityUIMessageHandler::RequestNativeUITree,
                          base::Unretained(this)));
}

void AccessibilityUIMessageHandler::ToggleAccessibility(
    const base::ListValue* args) {
  std::string process_id_str;
  std::string route_id_str;
  int process_id;
  int route_id;
  int mode;
  CHECK_EQ(3U, args->GetSize());
  CHECK(args->GetString(0, &process_id_str));
  CHECK(args->GetString(1, &route_id_str));
  // TODO(695247): We should pass each ax flag seperately
  CHECK(args->GetInteger(2, &mode));
  CHECK(base::StringToInt(process_id_str, &process_id));
  CHECK(base::StringToInt(route_id_str, &route_id));

  AllowJavascript();
  RenderViewHost* rvh = RenderViewHost::FromID(process_id, route_id);
  if (!rvh)
    return;
  auto* web_contents =
      static_cast<WebContentsImpl*>(WebContents::FromRenderViewHost(rvh));
  ui::AXMode current_mode = web_contents->GetAccessibilityMode();

  if (mode & ui::AXMode::kNativeAPIs)
    current_mode.set_mode(ui::AXMode::kNativeAPIs, true);

  if (mode & ui::AXMode::kWebContents)
    current_mode.set_mode(ui::AXMode::kWebContents, true);

  if (mode & ui::AXMode::kInlineTextBoxes)
    current_mode.set_mode(ui::AXMode::kInlineTextBoxes, true);

  if (mode & ui::AXMode::kScreenReader)
    current_mode.set_mode(ui::AXMode::kScreenReader, true);

  if (mode & ui::AXMode::kHTML)
    current_mode.set_mode(ui::AXMode::kHTML, true);

  web_contents->SetAccessibilityMode(current_mode);
}

void AccessibilityUIMessageHandler::SetGlobalFlag(const base::ListValue* args) {
  std::string flag_name_str;
  bool enabled;
  CHECK_EQ(2U, args->GetSize());
  CHECK(args->GetString(0, &flag_name_str));
  CHECK(args->GetBoolean(1, &enabled));

  AllowJavascript();
  if (flag_name_str == kInternal) {
    g_show_internal_accessibility_tree = enabled;
    LOG(ERROR) << "INTERNAL: " << g_show_internal_accessibility_tree;
    return;
  }

  ui::AXMode new_mode;
  if (flag_name_str == kNative) {
    new_mode = ui::AXMode::kNativeAPIs;
  } else if (flag_name_str == kWeb) {
    new_mode = ui::AXMode::kWebContents;
  } else if (flag_name_str == kText) {
    new_mode = ui::AXMode::kInlineTextBoxes;
  } else if (flag_name_str == kScreenReader) {
    new_mode = ui::AXMode::kScreenReader;
  } else if (flag_name_str == kHTML) {
    new_mode = ui::AXMode::kHTML;
  } else {
    NOTREACHED();
    return;
  }

  // It doesn't make sense to enable one of the flags that depends on
  // web contents without enabling web contents accessibility too.
  if (enabled && (new_mode.has_mode(ui::AXMode::kInlineTextBoxes) ||
                  new_mode.has_mode(ui::AXMode::kScreenReader) ||
                  new_mode.has_mode(ui::AXMode::kHTML))) {
    new_mode.set_mode(ui::AXMode::kWebContents, true);
  }

  // Similarly if you disable web accessibility we should remove all
  // flags that depend on it.
  if (!enabled && new_mode.has_mode(ui::AXMode::kWebContents)) {
    new_mode.set_mode(ui::AXMode::kInlineTextBoxes, true);
    new_mode.set_mode(ui::AXMode::kScreenReader, true);
    new_mode.set_mode(ui::AXMode::kHTML, true);
  }

  BrowserAccessibilityStateImpl* state =
      BrowserAccessibilityStateImpl::GetInstance();
  if (enabled)
    state->AddAccessibilityModeFlags(new_mode);
  else
    state->RemoveAccessibilityModeFlags(new_mode);
}

void AccessibilityUIMessageHandler::RequestWebContentsTree(
    const base::ListValue* args) {
  std::string process_id_str;
  std::string route_id_str;
  int process_id;
  int route_id;
  CHECK_EQ(2U, args->GetSize());
  CHECK(args->GetString(0, &process_id_str));
  CHECK(args->GetString(1, &route_id_str));
  CHECK(base::StringToInt(process_id_str, &process_id));
  CHECK(base::StringToInt(route_id_str, &route_id));

  AllowJavascript();
  RenderViewHost* rvh = RenderViewHost::FromID(process_id, route_id);
  if (!rvh) {
    std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
    result->SetInteger(kProcessIdField, process_id);
    result->SetInteger(kRouteIdField, route_id);
    result->SetString("error", "Renderer no longer exists.");
    CallJavascriptFunction("accessibility.showTree", *(result.get()));
    return;
  }

  std::unique_ptr<base::DictionaryValue> result(BuildTargetDescriptor(rvh));
  auto* web_contents =
      static_cast<WebContentsImpl*>(WebContents::FromRenderViewHost(rvh));
  // No matter the state of the current web_contents, we want to force the mode
  // because we are about to show the accessibility tree
  web_contents->SetAccessibilityMode(
      ui::AXMode(ui::AXMode::kNativeAPIs | ui::AXMode::kWebContents));

  std::unique_ptr<AccessibilityTreeFormatter> formatter;
  if (g_show_internal_accessibility_tree)
    formatter.reset(new AccessibilityTreeFormatterBlink());
  else
    formatter.reset(AccessibilityTreeFormatter::Create());
  base::string16 accessibility_contents_utf16;
  std::vector<AccessibilityTreeFormatter::Filter> filters;
  filters.push_back(AccessibilityTreeFormatter::Filter(
      base::ASCIIToUTF16("*"),
      AccessibilityTreeFormatter::Filter::ALLOW));
  formatter->SetFilters(filters);
  auto* ax_mgr = web_contents->GetOrCreateRootBrowserAccessibilityManager();
  DCHECK(ax_mgr);
  formatter->FormatAccessibilityTree(ax_mgr->GetRoot(),
                                     &accessibility_contents_utf16);
  result->SetString("tree", base::UTF16ToUTF8(accessibility_contents_utf16));
  CallJavascriptFunction("accessibility.showTree", *(result.get()));
}

void AccessibilityUIMessageHandler::RequestNativeUITree(
    const base::ListValue* args) {
  AllowJavascript();
  WebContentsImpl* web_contents =
      static_cast<WebContentsImpl*>(web_ui()->GetWebContents());
  gfx::NativeWindow native_window =
      web_contents->GetView()->GetTopLevelNativeWindow();
  ui::AXPlatformNode* node =
      ui::AXPlatformNode::FromNativeWindow(native_window);
  std::string str = RecursiveDumpAXPlatformNodeAsString(node, 0);

  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  result->SetString("tree", str);
  CallJavascriptFunction("accessibility.showNativeUITree", *(result.get()));
}

}  // namespace content
