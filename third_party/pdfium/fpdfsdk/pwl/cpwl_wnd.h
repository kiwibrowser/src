// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_PWL_CPWL_WND_H_
#define FPDFSDK_PWL_CPWL_WND_H_

#include <memory>
#include <vector>

#include "core/fpdfdoc/cpdf_formcontrol.h"
#include "core/fxcrt/observable.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_color.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_widget.h"
#include "fpdfsdk/pwl/cpwl_timer.h"
#include "fpdfsdk/pwl/cpwl_timer_handler.h"

class CPWL_Edit;
class CPWL_MsgControl;
class CPWL_ScrollBar;
class CFX_SystemHandler;
class IPVT_FontMap;
struct PWL_SCROLL_INFO;

// window styles
#define PWS_CHILD 0x80000000L
#define PWS_BORDER 0x40000000L
#define PWS_BACKGROUND 0x20000000L
#define PWS_HSCROLL 0x10000000L
#define PWS_VSCROLL 0x08000000L
#define PWS_VISIBLE 0x04000000L
#define PWS_READONLY 0x01000000L
#define PWS_AUTOFONTSIZE 0x00800000L
#define PWS_AUTOTRANSPARENT 0x00400000L
#define PWS_NOREFRESHCLIP 0x00200000L

// edit and label styles
#define PES_MULTILINE 0x0001L
#define PES_PASSWORD 0x0002L
#define PES_LEFT 0x0004L
#define PES_RIGHT 0x0008L
#define PES_MIDDLE 0x0010L
#define PES_TOP 0x0020L
#define PES_BOTTOM 0x0040L
#define PES_CENTER 0x0080L
#define PES_CHARARRAY 0x0100L
#define PES_AUTOSCROLL 0x0200L
#define PES_AUTORETURN 0x0400L
#define PES_UNDO 0x0800L
#define PES_RICH 0x1000L
#define PES_SPELLCHECK 0x2000L
#define PES_TEXTOVERFLOW 0x4000L
#define PES_NOREAD 0x8000L

// listbox styles
#define PLBS_MULTIPLESEL 0x0001L
#define PLBS_HOVERSEL 0x0008L

// combobox styles
#define PCBS_ALLOWCUSTOMTEXT 0x0001L

struct CPWL_Dash {
  CPWL_Dash() : nDash(0), nGap(0), nPhase(0) {}
  CPWL_Dash(int32_t dash, int32_t gap, int32_t phase)
      : nDash(dash), nGap(gap), nPhase(phase) {}

  void Reset() {
    nDash = 0;
    nGap = 0;
    nPhase = 0;
  }

  int32_t nDash;
  int32_t nGap;
  int32_t nPhase;
};

inline bool operator==(const CFX_Color& c1, const CFX_Color& c2) {
  return c1.nColorType == c2.nColorType && c1.fColor1 - c2.fColor1 < 0.0001 &&
         c1.fColor1 - c2.fColor1 > -0.0001 &&
         c1.fColor2 - c2.fColor2 < 0.0001 &&
         c1.fColor2 - c2.fColor2 > -0.0001 &&
         c1.fColor3 - c2.fColor3 < 0.0001 &&
         c1.fColor3 - c2.fColor3 > -0.0001 &&
         c1.fColor4 - c2.fColor4 < 0.0001 && c1.fColor4 - c2.fColor4 > -0.0001;
}

inline bool operator!=(const CFX_Color& c1, const CFX_Color& c2) {
  return !(c1 == c2);
}

#define PWL_SCROLLBAR_WIDTH 12.0f
#define PWL_SCROLLBAR_TRANSPARENCY 150
#define PWL_DEFAULT_BLACKCOLOR CFX_Color(CFX_Color::kGray, 0)
#define PWL_DEFAULT_WHITECOLOR CFX_Color(CFX_Color::kGray, 1)

class CPWL_Wnd : public CPWL_TimerHandler, public Observable<CPWL_Wnd> {
 public:
  class PrivateData {
   protected:
    ~PrivateData() {}
  };

  class ProviderIface : public Observable<ProviderIface> {
   public:
    virtual ~ProviderIface() {}

    // get a matrix which map user space to CWnd client space
    virtual CFX_Matrix GetWindowMatrix(PrivateData* pAttached) = 0;
  };

  class FocusHandlerIface {
   public:
    virtual ~FocusHandlerIface() {}
    virtual void OnSetFocus(CPWL_Edit* pEdit) = 0;
  };

  class CreateParams {
   public:
    CreateParams();
    CreateParams(const CreateParams& other);
    ~CreateParams();

    CFX_FloatRect rcRectWnd;                          // required
    CFX_SystemHandler* pSystemHandler;                // required
    IPVT_FontMap* pFontMap;                           // required
    ProviderIface::ObservedPtr pProvider;             // required
    UnownedPtr<FocusHandlerIface> pFocusHandler;      // optional
    uint32_t dwFlags;                                 // optional
    CFX_Color sBackgroundColor;                       // optional
    CPDFSDK_Widget::ObservedPtr pAttachedWidget;      // required
    BorderStyle nBorderStyle;                         // optional
    int32_t dwBorderWidth;                            // optional
    CFX_Color sBorderColor;                           // optional
    CFX_Color sTextColor;                             // optional
    int32_t nTransparency;                            // optional
    float fFontSize;                                  // optional
    CPWL_Dash sDash;                                  // optional
    UnownedPtr<PrivateData> pAttachedData;            // optional
    CPWL_Wnd* pParentWnd;                             // ignore
    CPWL_MsgControl* pMsgControl;                     // ignore
    int32_t eCursorType;                              // ignore
    CFX_Matrix mtChild;                               // ignore
  };

  CPWL_Wnd();
  ~CPWL_Wnd() override;

  virtual ByteString GetClassName() const;

  // Returns |true| iff this instance is still allocated.
  virtual bool InvalidateRect(CFX_FloatRect* pRect);

  virtual bool OnKeyDown(uint16_t nChar, uint32_t nFlag);
  virtual bool OnChar(uint16_t nChar, uint32_t nFlag);
  virtual bool OnLButtonDblClk(const CFX_PointF& point, uint32_t nFlag);
  virtual bool OnLButtonDown(const CFX_PointF& point, uint32_t nFlag);
  virtual bool OnLButtonUp(const CFX_PointF& point, uint32_t nFlag);
  virtual bool OnRButtonDown(const CFX_PointF& point, uint32_t nFlag);
  virtual bool OnRButtonUp(const CFX_PointF& point, uint32_t nFlag);
  virtual bool OnMouseMove(const CFX_PointF& point, uint32_t nFlag);
  virtual bool OnMouseWheel(short zDelta,
                            const CFX_PointF& point,
                            uint32_t nFlag);
  virtual void SetScrollInfo(const PWL_SCROLL_INFO& info);
  virtual void SetScrollPosition(float pos);
  virtual void ScrollWindowVertically(float pos);
  virtual void NotifyLButtonDown(CPWL_Wnd* child, const CFX_PointF& pos);
  virtual void NotifyLButtonUp(CPWL_Wnd* child, const CFX_PointF& pos);
  virtual void NotifyMouseMove(CPWL_Wnd* child, const CFX_PointF& pos);
  virtual void SetFocus();
  virtual void KillFocus();
  virtual void SetCursor();

  // Returns |true| iff this instance is still allocated.
  virtual bool SetVisible(bool bVisible);
  virtual void SetFontSize(float fFontSize);
  virtual float GetFontSize() const;

  virtual WideString GetText();
  virtual WideString GetSelectedText();
  virtual void ReplaceSelection(const WideString& text);

  virtual bool CanUndo();
  virtual bool CanRedo();
  virtual bool Undo();
  virtual bool Redo();

  virtual CFX_FloatRect GetFocusRect() const;
  virtual CFX_FloatRect GetClientRect() const;

  void InvalidateFocusHandler(FocusHandlerIface* handler);
  void InvalidateProvider(ProviderIface* provider);
  void Create(const CreateParams& cp);
  void Destroy();
  bool Move(const CFX_FloatRect& rcNew, bool bReset, bool bRefresh);

  void SetCapture();
  void ReleaseCapture();

  void DrawAppearance(CFX_RenderDevice* pDevice,
                      const CFX_Matrix& mtUser2Device);

  CFX_Color GetBackgroundColor() const;
  void SetBackgroundColor(const CFX_Color& color);
  CFX_Color GetBorderColor() const;
  CFX_Color GetTextColor() const;
  void SetTextColor(const CFX_Color& color);
  CFX_Color GetBorderLeftTopColor(BorderStyle nBorderStyle) const;
  CFX_Color GetBorderRightBottomColor(BorderStyle nBorderStyle) const;

  void SetBorderStyle(BorderStyle eBorderStyle);
  BorderStyle GetBorderStyle() const;
  const CPWL_Dash& GetBorderDash() const;

  int32_t GetBorderWidth() const;
  int32_t GetInnerBorderWidth() const;
  CFX_FloatRect GetWindowRect() const;
  CFX_PointF GetCenterPoint() const;

  bool IsVisible() const { return m_bVisible; }
  bool HasFlag(uint32_t dwFlags) const;
  void AddFlag(uint32_t dwFlags);
  void RemoveFlag(uint32_t dwFlags);

  void SetClipRect(const CFX_FloatRect& rect);
  const CFX_FloatRect& GetClipRect() const;

  CPWL_Wnd* GetParentWindow() const;
  PrivateData* GetAttachedData() const;

  bool WndHitTest(const CFX_PointF& point) const;
  bool ClientHitTest(const CFX_PointF& point) const;
  bool IsCaptureMouse() const;

  void EnableWindow(bool bEnable);
  bool IsEnabled() const { return m_bEnabled; }
  const CPWL_Wnd* GetFocused() const;
  bool IsFocused() const;
  bool IsReadOnly() const;
  CPWL_ScrollBar* GetVScrollBar() const;

  IPVT_FontMap* GetFontMap() const;
  ProviderIface* GetProvider() const;
  FocusHandlerIface* GetFocusHandler() const;

  int32_t GetTransparency();
  void SetTransparency(int32_t nTransparency);

  CFX_Matrix GetChildToRoot() const;
  CFX_Matrix GetChildMatrix() const;
  void SetChildMatrix(const CFX_Matrix& mt);
  CFX_Matrix GetWindowMatrix() const;

  virtual void OnSetFocus();
  virtual void OnKillFocus();

 protected:
  // CPWL_TimerHandler
  CFX_SystemHandler* GetSystemHandler() const override;

  virtual void CreateChildWnd(const CreateParams& cp);

  // Returns |true| iff this instance is still allocated.
  virtual bool RePosChildWnd();

  virtual void DrawThisAppearance(CFX_RenderDevice* pDevice,
                                  const CFX_Matrix& mtUser2Device);

  virtual void OnCreate(CreateParams* pParamsToAdjust);
  virtual void OnCreated();
  virtual void OnDestroy();

  void SetNotifyFlag(bool bNotifying = true) { m_bNotifying = bNotifying; }
  bool IsNotifying() const { return m_bNotifying; }
  bool IsValid() const { return m_bCreated; }
  const CreateParams& GetCreationParams() const { return m_CreationParams; }

  // Returns |true| iff this instance is still allocated.
  bool InvalidateRectMove(const CFX_FloatRect& rcOld,
                          const CFX_FloatRect& rcNew);

  bool IsWndCaptureMouse(const CPWL_Wnd* pWnd) const;
  bool IsWndCaptureKeyboard(const CPWL_Wnd* pWnd) const;
  const CPWL_Wnd* GetRootWnd() const;

  static bool IsCTRLpressed(uint32_t nFlag) {
    return CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  }
  static bool IsSHIFTpressed(uint32_t nFlag) {
    return CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
  }
  static bool IsALTpressed(uint32_t nFlag) {
    return CPDFSDK_FormFillEnvironment::IsALTKeyDown(nFlag);
  }

 private:
  CFX_PointF ParentToChild(const CFX_PointF& point) const;
  CFX_FloatRect ParentToChild(const CFX_FloatRect& rect) const;

  void DrawChildAppearance(CFX_RenderDevice* pDevice,
                           const CFX_Matrix& mtUser2Device);

  CFX_FloatRect PWLtoWnd(const CFX_FloatRect& rect) const;

  void AddChild(CPWL_Wnd* pWnd);
  void RemoveChild(CPWL_Wnd* pWnd);

  void CreateScrollBar(const CreateParams& cp);
  void CreateVScrollBar(const CreateParams& cp);

  void AdjustStyle();
  void CreateMsgControl();
  void DestroyMsgControl();

  CPWL_MsgControl* GetMsgControl() const;

  CreateParams m_CreationParams;
  std::vector<CPWL_Wnd*> m_Children;
  UnownedPtr<CPWL_ScrollBar> m_pVScrollBar;
  CFX_FloatRect m_rcWindow;
  CFX_FloatRect m_rcClip;
  bool m_bCreated;
  bool m_bVisible;
  bool m_bNotifying;
  bool m_bEnabled;
};

#endif  // FPDFSDK_PWL_CPWL_WND_H_
