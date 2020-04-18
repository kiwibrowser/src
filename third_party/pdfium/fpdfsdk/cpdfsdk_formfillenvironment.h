// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_CPDFSDK_FORMFILLENVIRONMENT_H_
#define FPDFSDK_CPDFSDK_FORMFILLENVIRONMENT_H_

#include <map>
#include <memory>
#include <vector>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfdoc/cpdf_occontext.h"
#include "core/fxcrt/observable.h"
#include "fpdfsdk/cfx_systemhandler.h"
#include "fpdfsdk/cpdfsdk_annot.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/fpdf_formfill.h"
#include "public/fpdf_fwlevent.h"

class CFFL_InteractiveFormFiller;
class CFX_SystemHandler;
class CPDFSDK_ActionHandler;
class CPDFSDK_AnnotHandlerMgr;
class CPDFSDK_InterForm;
class CPDFSDK_PageView;
class IJS_Runtime;

// NOTE: |bsUTF16LE| must outlive the use of the result. Care must be taken
// since modifying the result would impact |bsUTF16LE|.
FPDF_WIDESTRING AsFPDFWideString(ByteString* bsUTF16LE);

// The CPDFSDK_FormFillEnvironment is "owned" by the embedder across the
// C API as a FPDF_FormHandle, and may pop out of existence at any time,
// so long as the associated embedder-owned FPDF_Document outlives it.
// Pointers from objects in the FPDF_Document ownership hierarchy should
// be ObservedPtr<> so as to clear themselves when the embedder "exits"
// the form fill environment.  Pointers from objects in this ownership
// heirarcy to objects in the FPDF_Document ownership hierarcy should be
// UnownedPtr<>, as should pointers from objects in this ownership
// hierarcy back to the form fill environment itself, so as to flag any
// lingering lifetime issues via the memory tools.

class CPDFSDK_FormFillEnvironment
    : public Observable<CPDFSDK_FormFillEnvironment> {
 public:
  CPDFSDK_FormFillEnvironment(CPDF_Document* pDoc, FPDF_FORMFILLINFO* pFFinfo);
  ~CPDFSDK_FormFillEnvironment();

  static bool IsSHIFTKeyDown(uint32_t nFlag) {
    return !!(nFlag & FWL_EVENTFLAG_ShiftKey);
  }
  static bool IsCTRLKeyDown(uint32_t nFlag) {
    return !!(nFlag & FWL_EVENTFLAG_ControlKey);
  }
  static bool IsALTKeyDown(uint32_t nFlag) {
    return !!(nFlag & FWL_EVENTFLAG_AltKey);
  }

  CPDFSDK_PageView* GetPageView(UnderlyingPageType* pPage, bool renew);
  CPDFSDK_PageView* GetPageView(int nIndex);
  CPDFSDK_PageView* GetCurrentView();
  void RemovePageView(UnderlyingPageType* pPage);
  void UpdateAllViews(CPDFSDK_PageView* pSender, CPDFSDK_Annot* pAnnot);

  CPDFSDK_Annot* GetFocusAnnot() { return m_pFocusAnnot.Get(); }
  bool SetFocusAnnot(CPDFSDK_Annot::ObservedPtr* pAnnot);
  bool KillFocusAnnot(uint32_t nFlag);
  void ClearAllFocusedAnnots();

  bool ExtractPages(const std::vector<uint16_t>& arrExtraPages,
                    CPDF_Document* pDstDoc);
  bool InsertPages(int nInsertAt,
                   const CPDF_Document* pSrcDoc,
                   const std::vector<uint16_t>& arrSrcPages);
  bool ReplacePages(int nPage,
                    const CPDF_Document* pSrcDoc,
                    const std::vector<uint16_t>& arrSrcPages);

  int GetPageCount() const;
  bool GetPermissions(int nFlag) const;

  bool GetChangeMark() const { return m_bChangeMask; }
  void SetChangeMark() { m_bChangeMask = true; }
  void ClearChangeMark() { m_bChangeMask = false; }

  void ProcJavascriptFun();
  bool ProcOpenAction();

  void Invalidate(UnderlyingPageType* page, const FX_RECT& rect);
  void OutputSelectedRect(UnderlyingPageType* page, const CFX_FloatRect& rect);

  void SetCursor(int nCursorType);
  int SetTimer(int uElapse, TimerCallback lpTimerFunc);
  void KillTimer(int nTimerID);

  FX_SYSTEMTIME GetLocalTime() const;
  FPDF_PAGE GetCurrentPage() const;

  void OnChange();
  void ExecuteNamedAction(const char* namedAction);
  void OnSetFieldInputFocus(FPDF_WIDESTRING focusText,
                            FPDF_DWORD nTextLen,
                            bool bFocus);
  void DoURIAction(const char* bsURI);
  void DoGoToAction(int nPageIndex,
                    int zoomMode,
                    float* fPosArray,
                    int sizeOfArray);

  CPDF_Document* GetPDFDocument() const { return m_pCPDFDoc.Get(); }

#ifdef PDF_ENABLE_XFA
  CPDFXFA_Context* GetXFAContext() const;
  int GetPageViewCount() const { return m_PageMap.size(); }

  void DisplayCaret(CPDFXFA_Page* page,
                    FPDF_BOOL bVisible,
                    double left,
                    double top,
                    double right,
                    double bottom);
  int GetCurrentPageIndex() const;
  void SetCurrentPage(int iCurPage);

  // TODO(dsinclair): This should probably change to PDFium?
  WideString FFI_GetAppName() const { return WideString(L"Acrobat"); }

  WideString GetPlatform();
  void GotoURL(const WideString& wsURL);
  void GetPageViewRect(CPDFXFA_Page* page, FS_RECTF& dstRect);
  bool PopupMenu(CPDFXFA_Page* page,
                 FPDF_WIDGET hWidget,
                 int menuFlag,
                 CFX_PointF pt);

  void Alert(FPDF_WIDESTRING Msg, FPDF_WIDESTRING Title, int Type, int Icon);
  void EmailTo(FPDF_FILEHANDLER* fileHandler,
               FPDF_WIDESTRING pTo,
               FPDF_WIDESTRING pSubject,
               FPDF_WIDESTRING pCC,
               FPDF_WIDESTRING pBcc,
               FPDF_WIDESTRING pMsg);
  void UploadTo(FPDF_FILEHANDLER* fileHandler,
                int fileFlag,
                FPDF_WIDESTRING uploadTo);
  FPDF_FILEHANDLER* OpenFile(int fileType,
                             FPDF_WIDESTRING wsURL,
                             const char* mode);
  RetainPtr<IFX_SeekableReadStream> DownloadFromURL(const WideString& url);
  WideString PostRequestURL(const WideString& wsURL,
                            const WideString& wsData,
                            const WideString& wsContentType,
                            const WideString& wsEncode,
                            const WideString& wsHeader);
  FPDF_BOOL PutRequestURL(const WideString& wsURL,
                          const WideString& wsData,
                          const WideString& wsEncode);
  WideString GetLanguage();

  void PageEvent(int iPageCount, uint32_t dwEventType) const;
#endif  // PDF_ENABLE_XFA

  int JS_appAlert(const WideString& Msg,
                  const WideString& Title,
                  uint32_t Type,
                  uint32_t Icon);
  int JS_appResponse(const WideString& Question,
                     const WideString& Title,
                     const WideString& Default,
                     const WideString& cLabel,
                     FPDF_BOOL bPassword,
                     void* response,
                     int length);
  void JS_appBeep(int nType);
  WideString JS_fieldBrowse();
  WideString JS_docGetFilePath();
  void JS_docSubmitForm(void* formData, int length, const WideString& URL);
  void JS_docmailForm(void* mailData,
                      int length,
                      FPDF_BOOL bUI,
                      const WideString& To,
                      const WideString& Subject,
                      const WideString& CC,
                      const WideString& BCC,
                      const WideString& Msg);
  void JS_docprint(FPDF_BOOL bUI,
                   int nStart,
                   int nEnd,
                   FPDF_BOOL bSilent,
                   FPDF_BOOL bShrinkToFit,
                   FPDF_BOOL bPrintAsImage,
                   FPDF_BOOL bReverse,
                   FPDF_BOOL bAnnotations);
  void JS_docgotoPage(int nPageNum);

  bool IsJSPlatformPresent() const { return m_pInfo && m_pInfo->m_pJsPlatform; }
  ByteString GetAppName() const { return ""; }
  CFX_SystemHandler* GetSysHandler() const { return m_pSysHandler.get(); }
  FPDF_FORMFILLINFO* GetFormFillInfo() const { return m_pInfo; }

  // Creates if not present.
  CFFL_InteractiveFormFiller* GetInteractiveFormFiller();
  CPDFSDK_AnnotHandlerMgr* GetAnnotHandlerMgr();  // Creates if not present.
  IJS_Runtime* GetIJSRuntime();                   // Creates if not present.
  CPDFSDK_ActionHandler* GetActionHandler();      // Creates if not present.
  CPDFSDK_InterForm* GetInterForm();              // Creates if not present.

 private:
  UnderlyingPageType* GetPage(int nIndex);

  FPDF_FORMFILLINFO* const m_pInfo;
  std::unique_ptr<CPDFSDK_AnnotHandlerMgr> m_pAnnotHandlerMgr;
  std::unique_ptr<CPDFSDK_ActionHandler> m_pActionHandler;
  std::unique_ptr<IJS_Runtime> m_pIJSRuntime;
  std::map<UnderlyingPageType*, std::unique_ptr<CPDFSDK_PageView>> m_PageMap;
  std::unique_ptr<CPDFSDK_InterForm> m_pInterForm;
  CPDFSDK_Annot::ObservedPtr m_pFocusAnnot;
  UnownedPtr<CPDF_Document> const m_pCPDFDoc;
  std::unique_ptr<CFFL_InteractiveFormFiller> m_pFormFiller;
  std::unique_ptr<CFX_SystemHandler> m_pSysHandler;
  bool m_bChangeMask;
  bool m_bBeingDestroyed;
};

#endif  // FPDFSDK_CPDFSDK_FORMFILLENVIRONMENT_H_
