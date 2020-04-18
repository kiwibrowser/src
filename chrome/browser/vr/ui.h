// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_UI_H_
#define CHROME_BROWSER_VR_UI_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/version.h"
#include "chrome/browser/vr/assets_load_status.h"
#include "chrome/browser/vr/browser_ui_interface.h"
#include "chrome/browser/vr/keyboard_ui_interface.h"
#include "chrome/browser/vr/model/tab_model.h"
#include "chrome/browser/vr/platform_controller.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "chrome/browser/vr/ui_test_input.h"

namespace vr {

class AudioDelegate;
class BrowserUiInterface;
class ContentElement;
class ContentInputDelegate;
class PlatformInputHandler;
class KeyboardDelegate;
class PlatformUiInputDelegate;
class SkiaSurfaceProvider;
class TextInputDelegate;
class UiBrowserInterface;
class UiInputManager;
class UiRenderer;
class UiScene;
struct Assets;
struct ControllerModel;
struct Model;
struct OmniboxSuggestions;
struct ReticleModel;

struct UiInitialState {
  UiInitialState();
  UiInitialState(const UiInitialState& other);
  bool in_cct = false;
  bool in_web_vr = false;
  bool web_vr_autopresentation_expected = false;
  bool browsing_disabled = false;
  bool has_or_can_request_audio_permission = true;
  bool skips_redraw_when_not_dirty = false;
  bool assets_supported = false;
  bool supports_selection = true;
  bool needs_keyboard_update = false;
  bool is_standalone_vr_device = false;
  bool create_tabs_view = false;
};

// This class manages all GLThread owned objects and GL rendering for VrShell.
// It is not threadsafe and must only be used on the GL thread.
class Ui : public BrowserUiInterface, public KeyboardUiInterface {
 public:
  Ui(UiBrowserInterface* browser,
     PlatformInputHandler* content_input_forwarder,
     KeyboardDelegate* keyboard_delegate,
     TextInputDelegate* text_input_delegate,
     AudioDelegate* audio_delegate,
     const UiInitialState& ui_initial_state);

  Ui(UiBrowserInterface* browser,
     std::unique_ptr<ContentInputDelegate> content_input_delegate,
     KeyboardDelegate* keyboard_delegate,
     TextInputDelegate* text_input_delegate,
     AudioDelegate* audio_delegate,
     const UiInitialState& ui_initial_state);

  ~Ui() override;

  // TODO(crbug.com/767957): Refactor to hide these behind the UI interface.
  UiScene* scene() { return scene_.get(); }
  UiElementRenderer* ui_element_renderer() {
    return ui_element_renderer_.get();
  }
  UiRenderer* ui_renderer() { return ui_renderer_.get(); }
  UiInputManager* input_manager() { return input_manager_.get(); }

  base::WeakPtr<BrowserUiInterface> GetBrowserUiWeakPtr();

  // BrowserUiInterface
  void SetWebVrMode(bool enabled) override;
  void SetFullscreen(bool enabled) override;
  void SetToolbarState(const ToolbarState& state) override;
  void SetIncognito(bool enabled) override;
  void SetLoading(bool loading) override;
  void SetLoadProgress(float progress) override;
  void SetIsExiting() override;
  void SetHistoryButtonsEnabled(bool can_go_back, bool can_go_forward) override;
  void SetCapturingState(const CapturingStateModel& state) override;
  void ShowExitVrPrompt(UiUnsupportedMode reason) override;
  void SetSpeechRecognitionEnabled(bool enabled) override;
  void SetRecognitionResult(const base::string16& result) override;
  void OnSpeechRecognitionStateChanged(int new_state) override;
  void SetOmniboxSuggestions(
      std::unique_ptr<OmniboxSuggestions> suggestions) override;
  void OnAssetsLoaded(AssetsLoadStatus status,
                      std::unique_ptr<Assets> assets,
                      const base::Version& component_version) override;
  void OnAssetsUnavailable() override;
  void WaitForAssets() override;
  void SetOverlayTextureEmpty(bool empty) override;
  void ShowSoftInput(bool show) override;
  void UpdateWebInputIndices(int selection_start,
                             int selection_end,
                             int composition_start,
                             int composition_end) override;
  void AddOrUpdateTab(int id,
                      bool incognito,
                      const base::string16& title) override;
  void RemoveTab(int id, bool incognito) override;
  void RemoveAllTabs() override;

  // TODO(ymalik): We expose this to stop sending VSync to the WebVR page until
  // the splash screen has been visible for its minimum duration. The visibility
  // logic currently lives in the UI, and it'd be much cleaner if the UI didn't
  // have to worry about this, and if it were told to hide the splash screen
  // like other WebVR phases (e.g. OnWebVrFrameAvailable below).
  bool CanSendWebVrVSync();

  void SetAlertDialogEnabled(bool enabled,
                             PlatformUiInputDelegate* delegate,
                             float width,
                             float height);
  void SetAlertDialogSize(float width, float height);
  void SetDialogLocation(float x, float y);
  void SetDialogFloating(bool floating);
  void ShowPlatformToast(const base::string16& text);
  void CancelPlatformToast();
  bool ShouldRenderWebVr();

  void OnGlInitialized(
      unsigned int content_texture_id,
      UiElementRenderer::TextureLocation content_location,
      unsigned int content_overlay_texture_id,
      UiElementRenderer::TextureLocation content_overlay_location,
      unsigned int ui_texture_id,
      bool use_ganesh);

  void OnAppButtonClicked();
  void OnAppButtonSwipePerformed(PlatformController::SwipeDirection direction);
  void OnControllerUpdated(const ControllerModel& controller_model,
                           const ReticleModel& reticle_model);
  void OnProjMatrixChanged(const gfx::Transform& proj_matrix);
  void OnWebVrFrameAvailable();
  void OnWebVrTimedOut();
  void OnWebVrTimeoutImminent();
  bool IsControllerVisible() const;
  bool IsAppButtonLongPressed() const;
  bool SkipsRedrawWhenNotDirty() const;
  void OnSwapContents(int new_content_id);
  void OnContentBoundsChanged(int width, int height);
  void OnPlatformControllerInitialized(PlatformController* controller);
  void OnUiRequestedNavigation();
  void SetFloorHeight(float floor_height);

  Model* model_for_test() { return model_.get(); }

  void ReinitializeForTest(const UiInitialState& ui_initial_state);
  ContentInputDelegate* GetContentInputDelegateForTest() {
    return content_input_delegate_.get();
  }

  void Dump(bool include_bindings);

  // Keyboard input related.
  void RequestFocus(int element_id);
  void RequestUnfocus(int element_id);
  void OnInputEdited(const EditedText& info) override;
  void OnInputCommitted(const EditedText& info) override;
  void OnKeyboardHidden() override;

  void AcceptDoffPromptForTesting();
  void PerformUiActionForTesting(UiTestInput test_input);

  bool IsContentVisibleAndOpaque();
  bool IsContentOverlayTextureEmpty();
  void SetContentUsesQuadLayer(bool uses_quad_buffers);
  gfx::Transform GetContentWorldSpaceTransform();

 private:
  void OnSpeechRecognitionEnded();
  void InitializeModel(const UiInitialState& ui_initial_state);
  UiBrowserInterface* browser_;
  ContentElement* GetContentElement();
  std::vector<TabModel>::iterator FindTab(int id, std::vector<TabModel>* tabs);

  // This state may be further abstracted into a SkiaUi object.
  std::unique_ptr<UiScene> scene_;
  std::unique_ptr<Model> model_;
  std::unique_ptr<ContentInputDelegate> content_input_delegate_;
  std::unique_ptr<UiElementRenderer> ui_element_renderer_;
  std::unique_ptr<UiInputManager> input_manager_;
  std::unique_ptr<UiRenderer> ui_renderer_;
  std::unique_ptr<SkiaSurfaceProvider> provider_;

  // Cache the content element so we don't have to get it multiple times per
  // frame.
  ContentElement* content_element_ = nullptr;

  AudioDelegate* audio_delegate_ = nullptr;

  base::WeakPtrFactory<Ui> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Ui);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_UI_H_
