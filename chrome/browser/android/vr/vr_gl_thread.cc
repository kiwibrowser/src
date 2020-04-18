// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/vr_gl_thread.h"

#include <utility>

#include "base/strings/string16.h"
#include "base/version.h"
#include "chrome/browser/android/vr/vr_input_connection.h"
#include "chrome/browser/android/vr/vr_shell.h"
#include "chrome/browser/android/vr/vr_shell_gl.h"
#include "chrome/browser/vr/assets_loader.h"
#include "chrome/browser/vr/browser_ui_interface.h"
#include "chrome/browser/vr/model/assets.h"
#include "chrome/browser/vr/model/omnibox_suggestions.h"
#include "chrome/browser/vr/model/toolbar_state.h"
#include "chrome/browser/vr/sounds_manager_audio_delegate.h"
#include "chrome/browser/vr/ui.h"
#include "chrome/browser/vr/ui_test_input.h"
#include "chrome/common/chrome_features.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace vr {

VrGLThread::VrGLThread(
    const base::WeakPtr<VrShell>& weak_vr_shell,
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
    gvr_context* gvr_api,
    const UiInitialState& ui_initial_state,
    bool reprojected_rendering,
    bool daydream_support,
    bool pause_content,
    bool low_density)
    : base::android::JavaHandlerThread("VrShellGL"),
      weak_vr_shell_(weak_vr_shell),
      main_thread_task_runner_(std::move(main_thread_task_runner)),
      gvr_api_(gvr_api),
      ui_initial_state_(ui_initial_state),
      reprojected_rendering_(reprojected_rendering),
      daydream_support_(daydream_support),
      pause_content_(pause_content),
      low_density_(low_density) {}

VrGLThread::~VrGLThread() {
  Stop();
}

base::WeakPtr<VrShellGl> VrGLThread::GetVrShellGl() {
  return vr_shell_gl_->GetWeakPtr();
}

void VrGLThread::SetInputConnection(VrInputConnection* input_connection) {
  DCHECK(OnGlThread());
  input_connection_ = input_connection;
}

void VrGLThread::Init() {
  bool keyboard_enabled =
      !ui_initial_state_.web_vr_autopresentation_expected;
  if (keyboard_enabled) {
    keyboard_delegate_ = GvrKeyboardDelegate::Create();
    text_input_delegate_ = std::make_unique<TextInputDelegate>();
  }
  auto* keyboard_delegate =
      !keyboard_delegate_ ? nullptr : keyboard_delegate_.get();
  if (!keyboard_delegate)
    ui_initial_state_.needs_keyboard_update = true;

  audio_delegate_ = std::make_unique<SoundsManagerAudioDelegate>();

  auto ui = std::make_unique<Ui>(this, this, keyboard_delegate,
                                 text_input_delegate_.get(),
                                 audio_delegate_.get(), ui_initial_state_);
  if (keyboard_enabled) {
    text_input_delegate_->SetRequestFocusCallback(
        base::BindRepeating(&Ui::RequestFocus, base::Unretained(ui.get())));
    text_input_delegate_->SetRequestUnfocusCallback(
        base::BindRepeating(&Ui::RequestUnfocus, base::Unretained(ui.get())));
    if (keyboard_delegate) {
      keyboard_delegate_->SetUiInterface(ui.get());
      text_input_delegate_->SetUpdateInputCallback(
          base::BindRepeating(&GvrKeyboardDelegate::UpdateInput,
                              base::Unretained(keyboard_delegate_.get())));
    }
  }

  vr_shell_gl_ = std::make_unique<VrShellGl>(
      this, std::move(ui), gvr_api_, reprojected_rendering_, daydream_support_,
      ui_initial_state_.in_web_vr, pause_content_, low_density_);

  weak_browser_ui_ = vr_shell_gl_->GetBrowserUiWeakPtr();

  vr_shell_gl_->Initialize();
}

void VrGLThread::CleanUp() {
  audio_delegate_.reset();
  vr_shell_gl_.reset();
}

void VrGLThread::ContentSurfaceCreated(jobject surface,
                                       gl::SurfaceTexture* texture) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ContentSurfaceCreated, weak_vr_shell_,
                                surface, base::Unretained(texture)));
}

void VrGLThread::ContentOverlaySurfaceCreated(jobject surface,
                                              gl::SurfaceTexture* texture) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::ContentOverlaySurfaceCreated, weak_vr_shell_,
                     surface, base::Unretained(texture)));
}

void VrGLThread::DialogSurfaceCreated(jobject surface,
                                      gl::SurfaceTexture* texture) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::DialogSurfaceCreated, weak_vr_shell_,
                                surface, base::Unretained(texture)));
}

void VrGLThread::GvrDelegateReady(gvr::ViewerType viewer_type) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::GvrDelegateReady, weak_vr_shell_, viewer_type));
}

void VrGLThread::SendRequestPresentReply(
    bool success,
    device::mojom::VRDisplayFrameTransportOptionsPtr transport_options) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::SendRequestPresentReply, weak_vr_shell_, success,
                     std::move(transport_options)));
}

void VrGLThread::UpdateGamepadData(device::GvrGamepadData pad) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::UpdateGamepadData, weak_vr_shell_, pad));
}

void VrGLThread::ForwardEventToContent(
    std::unique_ptr<blink::WebInputEvent> event,
    int content_id) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ProcessContentGesture, weak_vr_shell_,
                                base::Passed(std::move(event)), content_id));
}

void VrGLThread::ClearFocusedElement() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ClearFocusedElement, weak_vr_shell_));
}

void VrGLThread::OnWebInputEdited(const TextEdits& edits) {
  DCHECK(OnGlThread());
  DCHECK(input_connection_);
  input_connection_->OnKeyboardEdit(edits);
}

void VrGLThread::SubmitWebInput() {
  DCHECK(OnGlThread());
  DCHECK(input_connection_);
  input_connection_->SubmitInput();
}

void VrGLThread::RequestWebInputText(TextStateUpdateCallback callback) {
  DCHECK(OnGlThread());
  DCHECK(input_connection_);
  input_connection_->RequestTextState(std::move(callback));
}

void VrGLThread::ForwardEventToPlatformUi(
    std::unique_ptr<blink::WebInputEvent> event) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ProcessDialogGesture, weak_vr_shell_,
                                base::Passed(std::move(event))));
}

void VrGLThread::ForceExitVr() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ForceExitVr, weak_vr_shell_));
}

void VrGLThread::ExitPresent() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ExitPresent, weak_vr_shell_));
  // TODO(vollick): Ui should hang onto the appropriate pointer rather than
  // bouncing through VrGLThread.
  vr_shell_gl_->OnExitPresent();
}

void VrGLThread::ExitFullscreen() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ExitFullscreen, weak_vr_shell_));
}

void VrGLThread::Navigate(GURL gurl, NavigationMethod method) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::Navigate, weak_vr_shell_, gurl, method));
}

void VrGLThread::NavigateBack() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::NavigateBack, weak_vr_shell_));
}

void VrGLThread::NavigateForward() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::NavigateForward, weak_vr_shell_));
}

void VrGLThread::ReloadTab() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ReloadTab, weak_vr_shell_));
}

void VrGLThread::OpenNewTab(bool incognito) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::OpenNewTab, weak_vr_shell_, incognito));
}

void VrGLThread::SelectTab(int id, bool incognito) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::SelectTab, weak_vr_shell_, id, incognito));
}

void VrGLThread::OpenBookmarks() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenBookmarks, weak_vr_shell_));
}

void VrGLThread::OpenRecentTabs() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenRecentTabs, weak_vr_shell_));
}

void VrGLThread::OpenHistory() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenHistory, weak_vr_shell_));
}

void VrGLThread::OpenDownloads() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenDownloads, weak_vr_shell_));
}

void VrGLThread::OpenShare() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenShare, weak_vr_shell_));
}

void VrGLThread::OpenSettings() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenSettings, weak_vr_shell_));
}

void VrGLThread::CloseTab(int id, bool incognito) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::CloseTab, weak_vr_shell_, id, incognito));
}

void VrGLThread::CloseAllTabs() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::CloseAllTabs, weak_vr_shell_));
}

void VrGLThread::CloseAllIncognitoTabs() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::CloseAllIncognitoTabs, weak_vr_shell_));
}

void VrGLThread::OpenFeedback() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OpenFeedback, weak_vr_shell_));
}

void VrGLThread::ExitCct() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ExitCct, weak_vr_shell_));
}

void VrGLThread::CloseHostedDialog() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::CloseHostedDialog, weak_vr_shell_));
}

void VrGLThread::ToggleCardboardGamepad(bool enabled) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ToggleCardboardGamepad,
                                weak_vr_shell_, enabled));
}

void VrGLThread::OnUnsupportedMode(UiUnsupportedMode mode) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::OnUnsupportedMode, weak_vr_shell_, mode));
}

void VrGLThread::OnExitVrPromptResult(ExitVrPromptChoice choice,
                                      UiUnsupportedMode reason) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OnExitVrPromptResult, weak_vr_shell_,
                                reason, choice));
}

void VrGLThread::OnContentScreenBoundsChanged(const gfx::SizeF& bounds) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::OnContentScreenBoundsChanged,
                                weak_vr_shell_, bounds));
}

void VrGLThread::SetVoiceSearchActive(bool active) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::SetVoiceSearchActive, weak_vr_shell_, active));
}

void VrGLThread::StartAutocomplete(const AutocompleteRequest& request) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VrShell::StartAutocomplete, weak_vr_shell_, request));
}

void VrGLThread::StopAutocomplete() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::StopAutocomplete, weak_vr_shell_));
}

void VrGLThread::ShowPageInfo() {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ShowPageInfo, weak_vr_shell_));
}

void VrGLThread::SetFullscreen(bool enabled) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetFullscreen,
                                         weak_browser_ui_, enabled));
}

void VrGLThread::SetIncognito(bool incognito) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetIncognito,
                                         weak_browser_ui_, incognito));
}

void VrGLThread::SetHistoryButtonsEnabled(bool can_go_back,
                                          bool can_go_forward) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserUiInterface::SetHistoryButtonsEnabled,
                                weak_browser_ui_, can_go_back, can_go_forward));
}

void VrGLThread::SetLoadProgress(float progress) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetLoadProgress,
                                         weak_browser_ui_, progress));
}

void VrGLThread::SetLoading(bool loading) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetLoading,
                                         weak_browser_ui_, loading));
}

void VrGLThread::SetToolbarState(const ToolbarState& state) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetToolbarState,
                                         weak_browser_ui_, state));
}

void VrGLThread::SetWebVrMode(bool enabled) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetWebVrMode,
                                         weak_browser_ui_, enabled));
}

void VrGLThread::SetCapturingState(const CapturingStateModel& state) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::SetCapturingState,
                                         weak_browser_ui_, state));
}

void VrGLThread::SetIsExiting() {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::SetIsExiting, weak_browser_ui_));
}

void VrGLThread::ShowExitVrPrompt(UiUnsupportedMode reason) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::ShowExitVrPrompt,
                                         weak_browser_ui_, reason));
}

void VrGLThread::SetSpeechRecognitionEnabled(bool enabled) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::SetSpeechRecognitionEnabled,
                     weak_browser_ui_, enabled));
}

void VrGLThread::SetRecognitionResult(const base::string16& result) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserUiInterface::SetRecognitionResult,
                                weak_browser_ui_, result));
}

void VrGLThread::OnSpeechRecognitionStateChanged(int new_state) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::OnSpeechRecognitionStateChanged,
                     weak_browser_ui_, new_state));
}

void VrGLThread::SetOmniboxSuggestions(
    std::unique_ptr<OmniboxSuggestions> suggestions) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::SetOmniboxSuggestions,
                     weak_browser_ui_, base::Passed(std::move(suggestions))));
}

void VrGLThread::OnAssetsLoaded(AssetsLoadStatus status,
                                std::unique_ptr<Assets> assets,
                                const base::Version& component_version) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::OnAssetsLoaded, weak_browser_ui_,
                     status, std::move(assets), component_version));
}

void VrGLThread::OnAssetsUnavailable() {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserUiInterface::OnAssetsUnavailable,
                                weak_browser_ui_));
}

void VrGLThread::WaitForAssets() {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::WaitForAssets, weak_browser_ui_));
}

void VrGLThread::SetOverlayTextureEmpty(bool empty) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserUiInterface::SetOverlayTextureEmpty,
                                weak_browser_ui_, empty));
}

void VrGLThread::ShowSoftInput(bool show) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::ShowSoftInput,
                                         weak_browser_ui_, show));
}

void VrGLThread::UpdateWebInputIndices(int selection_start,
                                       int selection_end,
                                       int composition_start,
                                       int composition_end) {
  task_runner()->PostTask(
      FROM_HERE,
      base::BindRepeating(&BrowserUiInterface::UpdateWebInputIndices,
                          weak_browser_ui_, selection_start, selection_end,
                          composition_start, composition_end));
}

void VrGLThread::AddOrUpdateTab(int id,
                                bool incognito,
                                const base::string16& title) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&BrowserUiInterface::AddOrUpdateTab,
                                weak_browser_ui_, id, incognito, title));
}

void VrGLThread::RemoveTab(int id, bool incognito) {
  DCHECK(OnMainThread());
  task_runner()->PostTask(FROM_HERE,
                          base::BindOnce(&BrowserUiInterface::RemoveTab,
                                         weak_browser_ui_, id, incognito));
}

void VrGLThread::RemoveAllTabs() {
  DCHECK(OnMainThread());
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&BrowserUiInterface::RemoveAllTabs, weak_browser_ui_));
}

void VrGLThread::ReportUiActivityResultForTesting(
    const VrUiTestActivityResult& result) {
  DCHECK(OnGlThread());
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrShell::ReportUiActivityResultForTesting,
                                weak_vr_shell_, result));
}

bool VrGLThread::OnMainThread() const {
  return main_thread_task_runner_->BelongsToCurrentThread();
}

bool VrGLThread::OnGlThread() const {
  return task_runner()->BelongsToCurrentThread();
}

}  // namespace vr
