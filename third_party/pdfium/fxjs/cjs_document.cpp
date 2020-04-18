// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_document.h"

#include <utility>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfdoc/cpdf_interform.h"
#include "core/fpdfdoc/cpdf_nametree.h"
#include "fpdfsdk/cpdfsdk_annotiteration.h"
#include "fpdfsdk/cpdfsdk_interform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fxjs/cjs_annot.h"
#include "fxjs/cjs_app.h"
#include "fxjs/cjs_delaydata.h"
#include "fxjs/cjs_field.h"
#include "fxjs/cjs_icon.h"
#include "fxjs/cjs_printparamsobj.h"
#include "fxjs/js_resources.h"

const JSPropertySpec CJS_Document::PropertySpecs[] = {
    {"ADBE", get_ADBE_static, set_ADBE_static},
    {"author", get_author_static, set_author_static},
    {"baseURL", get_base_URL_static, set_base_URL_static},
    {"bookmarkRoot", get_bookmark_root_static, set_bookmark_root_static},
    {"calculate", get_calculate_static, set_calculate_static},
    {"Collab", get_collab_static, set_collab_static},
    {"creationDate", get_creation_date_static, set_creation_date_static},
    {"creator", get_creator_static, set_creator_static},
    {"delay", get_delay_static, set_delay_static},
    {"dirty", get_dirty_static, set_dirty_static},
    {"documentFileName", get_document_file_name_static,
     set_document_file_name_static},
    {"external", get_external_static, set_external_static},
    {"filesize", get_filesize_static, set_filesize_static},
    {"icons", get_icons_static, set_icons_static},
    {"info", get_info_static, set_info_static},
    {"keywords", get_keywords_static, set_keywords_static},
    {"layout", get_layout_static, set_layout_static},
    {"media", get_media_static, set_media_static},
    {"modDate", get_mod_date_static, set_mod_date_static},
    {"mouseX", get_mouse_x_static, set_mouse_x_static},
    {"mouseY", get_mouse_y_static, set_mouse_y_static},
    {"numFields", get_num_fields_static, set_num_fields_static},
    {"numPages", get_num_pages_static, set_num_pages_static},
    {"pageNum", get_page_num_static, set_page_num_static},
    {"pageWindowRect", get_page_window_rect_static,
     set_page_window_rect_static},
    {"path", get_path_static, set_path_static},
    {"producer", get_producer_static, set_producer_static},
    {"subject", get_subject_static, set_subject_static},
    {"title", get_title_static, set_title_static},
    {"URL", get_URL_static, set_URL_static},
    {"zoom", get_zoom_static, set_zoom_static},
    {"zoomType", get_zoom_type_static, set_zoom_type_static}};

const JSMethodSpec CJS_Document::MethodSpecs[] = {
    {"addAnnot", addAnnot_static},
    {"addField", addField_static},
    {"addLink", addLink_static},
    {"addIcon", addIcon_static},
    {"calculateNow", calculateNow_static},
    {"closeDoc", closeDoc_static},
    {"createDataObject", createDataObject_static},
    {"deletePages", deletePages_static},
    {"exportAsText", exportAsText_static},
    {"exportAsFDF", exportAsFDF_static},
    {"exportAsXFDF", exportAsXFDF_static},
    {"extractPages", extractPages_static},
    {"getAnnot", getAnnot_static},
    {"getAnnots", getAnnots_static},
    {"getAnnot3D", getAnnot3D_static},
    {"getAnnots3D", getAnnots3D_static},
    {"getField", getField_static},
    {"getIcon", getIcon_static},
    {"getLinks", getLinks_static},
    {"getNthFieldName", getNthFieldName_static},
    {"getOCGs", getOCGs_static},
    {"getPageBox", getPageBox_static},
    {"getPageNthWord", getPageNthWord_static},
    {"getPageNthWordQuads", getPageNthWordQuads_static},
    {"getPageNumWords", getPageNumWords_static},
    {"getPrintParams", getPrintParams_static},
    {"getURL", getURL_static},
    {"gotoNamedDest", gotoNamedDest_static},
    {"importAnFDF", importAnFDF_static},
    {"importAnXFDF", importAnXFDF_static},
    {"importTextData", importTextData_static},
    {"insertPages", insertPages_static},
    {"mailForm", mailForm_static},
    {"print", print_static},
    {"removeField", removeField_static},
    {"replacePages", replacePages_static},
    {"resetForm", resetForm_static},
    {"removeIcon", removeIcon_static},
    {"saveAs", saveAs_static},
    {"submitForm", submitForm_static},
    {"syncAnnotScan", syncAnnotScan_static},
    {"mailDoc", mailDoc_static}};

int CJS_Document::ObjDefnID = -1;
const char CJS_Document::kName[] = "Document";

// static
int CJS_Document::GetObjDefnID() {
  return ObjDefnID;
}

// static
void CJS_Document::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_Document::kName, FXJSOBJTYPE_GLOBAL,
                                 JSConstructor<CJS_Document>, JSDestructor);
  DefineProps(pEngine, ObjDefnID, PropertySpecs, FX_ArraySize(PropertySpecs));
  DefineMethods(pEngine, ObjDefnID, MethodSpecs, FX_ArraySize(MethodSpecs));
}

CJS_Document::CJS_Document(v8::Local<v8::Object> pObject)
    : CJS_Object(pObject),
      m_pFormFillEnv(nullptr),
      m_cwBaseURL(L""),
      m_bDelay(false) {}

CJS_Document::~CJS_Document() = default;

void CJS_Document::InitInstance(IJS_Runtime* pIRuntime) {
  SetFormFillEnv(pIRuntime->GetFormFillEnv());
}

// The total number of fields in document.
CJS_Return CJS_Document::get_num_fields(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pPDFForm = pInterForm->GetInterForm();
  return CJS_Return(pRuntime->NewNumber(
      static_cast<int>(pPDFForm->CountFields(WideString()))));
}

CJS_Return CJS_Document::set_num_fields(CJS_Runtime* pRuntime,
                                        v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::get_dirty(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(pRuntime->NewBoolean(!!m_pFormFillEnv->GetChangeMark()));
}

CJS_Return CJS_Document::set_dirty(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  pRuntime->ToBoolean(vp) ? m_pFormFillEnv->SetChangeMark()
                          : m_pFormFillEnv->ClearChangeMark();
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_ADBE(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewUndefined());
}

CJS_Return CJS_Document::set_ADBE(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_page_num(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDFSDK_PageView* pPageView = m_pFormFillEnv->GetCurrentView();
  if (!pPageView)
    return CJS_Return(pRuntime->NewUndefined());
  return CJS_Return(pRuntime->NewNumber(pPageView->GetPageIndex()));
}

CJS_Return CJS_Document::set_page_num(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  int iPageCount = m_pFormFillEnv->GetPageCount();
  int iPageNum = pRuntime->ToInt32(vp);
  if (iPageNum >= 0 && iPageNum < iPageCount)
    m_pFormFillEnv->JS_docgotoPage(iPageNum);
  else if (iPageNum >= iPageCount)
    m_pFormFillEnv->JS_docgotoPage(iPageCount - 1);
  else if (iPageNum < 0)
    m_pFormFillEnv->JS_docgotoPage(0);

  return CJS_Return(true);
}

CJS_Return CJS_Document::addAnnot(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::addField(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::exportAsText(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::exportAsFDF(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::exportAsXFDF(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::getField(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.empty())
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  WideString wideName = pRuntime->ToWideString(params[0]);
  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pPDFForm = pInterForm->GetInterForm();
  if (pPDFForm->CountFields(wideName) <= 0)
    return CJS_Return(pRuntime->NewUndefined());

  v8::Local<v8::Object> pFieldObj =
      pRuntime->NewFXJSBoundObject(CJS_Field::GetObjDefnID());
  if (pFieldObj.IsEmpty())
    return CJS_Return(false);

  CJS_Field* pJSField =
      static_cast<CJS_Field*>(pRuntime->GetObjectPrivate(pFieldObj));
  pJSField->AttachField(this, wideName);
  if (!pJSField)
    return CJS_Return(false);

  return CJS_Return(pJSField->ToV8Object());
}

// Gets the name of the nth field in the document
CJS_Return CJS_Document::getNthFieldName(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  int nIndex = pRuntime->ToInt32(params[0]);
  if (nIndex < 0)
    return CJS_Return(JSGetStringFromID(JSMessage::kValueError));

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pPDFForm = pInterForm->GetInterForm();
  CPDF_FormField* pField = pPDFForm->GetField(nIndex, WideString());
  if (!pField)
    return CJS_Return(false);
  return CJS_Return(pRuntime->NewString(pField->GetFullName().c_str()));
}

CJS_Return CJS_Document::importAnFDF(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::importAnXFDF(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::importTextData(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

// exports the form data and mails the resulting fdf file as an attachment to
// all recipients.
// comment: need reader supports
CJS_Return CJS_Document::mailForm(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  if (!m_pFormFillEnv->GetPermissions(FPDFPERM_EXTRACT_ACCESS))
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  ByteString sTextBuf = pInterForm->ExportFormToFDFTextBuf();
  if (sTextBuf.GetLength() == 0)
    return CJS_Return(false);

  size_t nLength = params.size();
  bool bUI = nLength > 0 ? pRuntime->ToBoolean(params[0]) : true;
  WideString cTo = nLength > 1 ? pRuntime->ToWideString(params[1]) : L"";
  WideString cCc = nLength > 2 ? pRuntime->ToWideString(params[2]) : L"";
  WideString cBcc = nLength > 3 ? pRuntime->ToWideString(params[3]) : L"";
  WideString cSubject = nLength > 4 ? pRuntime->ToWideString(params[4]) : L"";
  WideString cMsg = nLength > 5 ? pRuntime->ToWideString(params[5]) : L"";

  size_t nBufSize = sTextBuf.GetLength();
  std::unique_ptr<char, FxFreeDeleter> pMutableBuf(FX_Alloc(char, nBufSize));
  memcpy(pMutableBuf.get(), sTextBuf.c_str(), nBufSize);

  pRuntime->BeginBlock();
  CPDFSDK_FormFillEnvironment* pFormFillEnv = pRuntime->GetFormFillEnv();
  pFormFillEnv->JS_docmailForm(pMutableBuf.get(), nBufSize, bUI, cTo, cSubject,
                               cCc, cBcc, cMsg);
  pRuntime->EndBlock();
  return CJS_Return(true);
}

CJS_Return CJS_Document::print(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  bool bUI = true;
  int nStart = 0;
  int nEnd = 0;
  bool bSilent = false;
  bool bShrinkToFit = false;
  bool bPrintAsImage = false;
  bool bReverse = false;
  bool bAnnotations = false;
  size_t nLength = params.size();
  if (nLength == 9) {
    if (params[8]->IsObject()) {
      v8::Local<v8::Object> pObj = pRuntime->ToObject(params[8]);
      if (CFXJS_Engine::GetObjDefnID(pObj) ==
          CJS_PrintParamsObj::GetObjDefnID()) {
        v8::Local<v8::Object> pObj = pRuntime->ToObject(params[8]);
        CJS_Object* pJSObj = pRuntime->GetObjectPrivate(pObj);
        if (pJSObj) {
          CJS_PrintParamsObj* printObj =
              static_cast<CJS_PrintParamsObj*>(pJSObj);
          bUI = printObj->GetUI();
          nStart = printObj->GetStart();
          nEnd = printObj->GetEnd();
          bSilent = printObj->GetSilent();
          bShrinkToFit = printObj->GetShrinkToFit();
          bPrintAsImage = printObj->GetPrintAsImage();
          bReverse = printObj->GetReverse();
          bAnnotations = printObj->GetAnnotations();
        }
      }
    }
  } else {
    if (nLength > 0)
      bUI = pRuntime->ToBoolean(params[0]);
    if (nLength > 1)
      nStart = pRuntime->ToInt32(params[1]);
    if (nLength > 2)
      nEnd = pRuntime->ToInt32(params[2]);
    if (nLength > 3)
      bSilent = pRuntime->ToBoolean(params[3]);
    if (nLength > 4)
      bShrinkToFit = pRuntime->ToBoolean(params[4]);
    if (nLength > 5)
      bPrintAsImage = pRuntime->ToBoolean(params[5]);
    if (nLength > 6)
      bReverse = pRuntime->ToBoolean(params[6]);
    if (nLength > 7)
      bAnnotations = pRuntime->ToBoolean(params[7]);
  }

  if (!m_pFormFillEnv)
    return CJS_Return(false);

  m_pFormFillEnv->JS_docprint(bUI, nStart, nEnd, bSilent, bShrinkToFit,
                              bPrintAsImage, bReverse, bAnnotations);
  return CJS_Return(true);
}

// removes the specified field from the document.
// comment:
// note: if the filed name is not rational, adobe is dumb for it.

CJS_Return CJS_Document::removeField(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  if (!(m_pFormFillEnv->GetPermissions(FPDFPERM_MODIFY) ||
        m_pFormFillEnv->GetPermissions(FPDFPERM_ANNOT_FORM)))
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));

  WideString sFieldName = pRuntime->ToWideString(params[0]);
  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  std::vector<CPDFSDK_Annot::ObservedPtr> widgets;
  pInterForm->GetWidgets(sFieldName, &widgets);
  if (widgets.empty())
    return CJS_Return(true);

  for (const auto& pAnnot : widgets) {
    CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot.Get());
    if (!pWidget)
      continue;

    CFX_FloatRect rcAnnot = pWidget->GetRect();
    --rcAnnot.left;
    --rcAnnot.bottom;
    ++rcAnnot.right;
    ++rcAnnot.top;

    std::vector<CFX_FloatRect> aRefresh(1, rcAnnot);
    UnderlyingPageType* pPage = pWidget->GetUnderlyingPage();
    ASSERT(pPage);

    // If there is currently no pageview associated with the page being used
    // do not create one. We may be in the process of tearing down the document
    // and creating a new pageview at this point will cause bad things.
    CPDFSDK_PageView* pPageView = m_pFormFillEnv->GetPageView(pPage, false);
    if (pPageView) {
#if PDF_ENABLE_XFA
      pPageView->DeleteAnnot(pWidget);
#endif  // PDF_ENABLE_XFA
      pPageView->UpdateRects(aRefresh);
    }
  }
  m_pFormFillEnv->SetChangeMark();

  return CJS_Return(true);
}

// reset filed values within a document.
// comment:
// note: if the fields names r not rational, aodbe is dumb for it.

CJS_Return CJS_Document::resetForm(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  if (!(m_pFormFillEnv->GetPermissions(FPDFPERM_MODIFY) ||
        m_pFormFillEnv->GetPermissions(FPDFPERM_ANNOT_FORM) ||
        m_pFormFillEnv->GetPermissions(FPDFPERM_FILL_FORM))) {
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));
  }

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pPDFForm = pInterForm->GetInterForm();
  if (params.empty()) {
    pPDFForm->ResetForm(true);
    m_pFormFillEnv->SetChangeMark();
    return CJS_Return(true);
  }

  v8::Local<v8::Array> array;
  if (params[0]->IsString()) {
    array = pRuntime->NewArray();
    pRuntime->PutArrayElement(array, 0, params[0]);
  } else {
    array = pRuntime->ToArray(params[0]);
  }

  std::vector<CPDF_FormField*> aFields;
  for (size_t i = 0; i < pRuntime->GetArrayLength(array); ++i) {
    WideString swVal =
        pRuntime->ToWideString(pRuntime->GetArrayElement(array, i));
    for (int j = 0, jsz = pPDFForm->CountFields(swVal); j < jsz; ++j)
      aFields.push_back(pPDFForm->GetField(j, swVal));
  }

  if (!aFields.empty()) {
    pPDFForm->ResetForm(aFields, true, true);
    m_pFormFillEnv->SetChangeMark();
  }

  return CJS_Return(true);
}

CJS_Return CJS_Document::saveAs(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::syncAnnotScan(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::submitForm(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  size_t nSize = params.size();
  if (nSize < 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  v8::Local<v8::Array> aFields;
  WideString strURL;
  bool bFDF = true;
  bool bEmpty = false;
  if (params[0]->IsString()) {
    strURL = pRuntime->ToWideString(params[0]);
    if (nSize > 1)
      bFDF = pRuntime->ToBoolean(params[1]);
    if (nSize > 2)
      bEmpty = pRuntime->ToBoolean(params[2]);
    if (nSize > 3)
      aFields = pRuntime->ToArray(params[3]);
  } else if (params[0]->IsObject()) {
    v8::Local<v8::Object> pObj = pRuntime->ToObject(params[0]);
    v8::Local<v8::Value> pValue = pRuntime->GetObjectProperty(pObj, L"cURL");
    if (!pValue.IsEmpty())
      strURL = pRuntime->ToWideString(pValue);

    bFDF = pRuntime->ToBoolean(pRuntime->GetObjectProperty(pObj, L"bFDF"));
    bEmpty = pRuntime->ToBoolean(pRuntime->GetObjectProperty(pObj, L"bEmpty"));
    aFields = pRuntime->ToArray(pRuntime->GetObjectProperty(pObj, L"aFields"));
  }

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  CPDF_InterForm* pPDFInterForm = pInterForm->GetInterForm();

  if (pRuntime->GetArrayLength(aFields) == 0 && bEmpty) {
    if (pPDFInterForm->CheckRequiredFields(nullptr, true)) {
      pRuntime->BeginBlock();
      pInterForm->SubmitForm(strURL, false);
      pRuntime->EndBlock();
    }
    return CJS_Return(true);
  }

  std::vector<CPDF_FormField*> fieldObjects;
  for (size_t i = 0; i < pRuntime->GetArrayLength(aFields); ++i) {
    WideString sName =
        pRuntime->ToWideString(pRuntime->GetArrayElement(aFields, i));
    CPDF_InterForm* pPDFForm = pInterForm->GetInterForm();
    for (int j = 0, jsz = pPDFForm->CountFields(sName); j < jsz; ++j) {
      CPDF_FormField* pField = pPDFForm->GetField(j, sName);
      if (!bEmpty && pField->GetValue().IsEmpty())
        continue;

      fieldObjects.push_back(pField);
    }
  }

  if (pPDFInterForm->CheckRequiredFields(&fieldObjects, true)) {
    pRuntime->BeginBlock();
    pInterForm->SubmitFields(strURL, fieldObjects, true, !bFDF);
    pRuntime->EndBlock();
  }
  return CJS_Return(true);
}

void CJS_Document::SetFormFillEnv(CPDFSDK_FormFillEnvironment* pFormFillEnv) {
  m_pFormFillEnv.Reset(pFormFillEnv);
}

CJS_Return CJS_Document::get_bookmark_root(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_bookmark_root(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::mailDoc(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // TODO(tsepez): Check maximum number of allowed params.
  size_t nLength = params.size();
  bool bUI = true;
  WideString cTo;
  WideString cCc;
  WideString cBcc;
  WideString cSubject;
  WideString cMsg;

  if (nLength > 0 && params[0]->IsObject()) {
    v8::Local<v8::Object> pObj = pRuntime->ToObject(params[0]);
    bUI = pRuntime->ToBoolean(pRuntime->GetObjectProperty(pObj, L"bUI"));
    cTo = pRuntime->ToWideString(pRuntime->GetObjectProperty(pObj, L"cTo"));
    cCc = pRuntime->ToWideString(pRuntime->GetObjectProperty(pObj, L"cCc"));
    cBcc = pRuntime->ToWideString(pRuntime->GetObjectProperty(pObj, L"cBcc"));
    cSubject =
        pRuntime->ToWideString(pRuntime->GetObjectProperty(pObj, L"cSubject"));
    cMsg = pRuntime->ToWideString(pRuntime->GetObjectProperty(pObj, L"cMsg"));
  } else {
    if (nLength > 0)
      bUI = pRuntime->ToBoolean(params[0]);
    if (nLength > 1)
      cTo = pRuntime->ToWideString(params[1]);
    if (nLength > 2)
      cCc = pRuntime->ToWideString(params[2]);
    if (nLength > 3)
      cBcc = pRuntime->ToWideString(params[3]);
    if (nLength > 4)
      cSubject = pRuntime->ToWideString(params[4]);
    if (nLength > 5)
      cMsg = pRuntime->ToWideString(params[5]);
  }

  pRuntime->BeginBlock();
  CPDFSDK_FormFillEnvironment* pFormFillEnv = pRuntime->GetFormFillEnv();
  pFormFillEnv->JS_docmailForm(nullptr, 0, bUI, cTo, cSubject, cCc, cBcc, cMsg);
  pRuntime->EndBlock();
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_author(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "Author");
}

CJS_Return CJS_Document::set_author(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "Author");
}

CJS_Return CJS_Document::get_info(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  const auto* pDictionary = m_pFormFillEnv->GetPDFDocument()->GetInfo();
  if (!pDictionary)
    return CJS_Return(false);

  WideString cwAuthor = pDictionary->GetUnicodeTextFor("Author");
  WideString cwTitle = pDictionary->GetUnicodeTextFor("Title");
  WideString cwSubject = pDictionary->GetUnicodeTextFor("Subject");
  WideString cwKeywords = pDictionary->GetUnicodeTextFor("Keywords");
  WideString cwCreator = pDictionary->GetUnicodeTextFor("Creator");
  WideString cwProducer = pDictionary->GetUnicodeTextFor("Producer");
  WideString cwCreationDate = pDictionary->GetUnicodeTextFor("CreationDate");
  WideString cwModDate = pDictionary->GetUnicodeTextFor("ModDate");
  WideString cwTrapped = pDictionary->GetUnicodeTextFor("Trapped");

  v8::Local<v8::Object> pObj = pRuntime->NewObject();
  pRuntime->PutObjectProperty(pObj, L"Author",
                              pRuntime->NewString(cwAuthor.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"Title",
                              pRuntime->NewString(cwTitle.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"Subject",
                              pRuntime->NewString(cwSubject.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"Keywords",
                              pRuntime->NewString(cwKeywords.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"Creator",
                              pRuntime->NewString(cwCreator.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"Producer",
                              pRuntime->NewString(cwProducer.AsStringView()));
  pRuntime->PutObjectProperty(
      pObj, L"CreationDate",
      pRuntime->NewString(cwCreationDate.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"ModDate",
                              pRuntime->NewString(cwModDate.AsStringView()));
  pRuntime->PutObjectProperty(pObj, L"Trapped",
                              pRuntime->NewString(cwTrapped.AsStringView()));

  // It's to be compatible to non-standard info dictionary.
  for (const auto& it : *pDictionary) {
    const ByteString& bsKey = it.first;
    CPDF_Object* pValueObj = it.second.get();
    WideString wsKey = WideString::FromUTF8(bsKey.AsStringView());
    if (pValueObj->IsString() || pValueObj->IsName()) {
      pRuntime->PutObjectProperty(
          pObj, wsKey,
          pRuntime->NewString(pValueObj->GetUnicodeText().AsStringView()));
    } else if (pValueObj->IsNumber()) {
      pRuntime->PutObjectProperty(pObj, wsKey,
                                  pRuntime->NewNumber(pValueObj->GetNumber()));
    } else if (pValueObj->IsBoolean()) {
      pRuntime->PutObjectProperty(
          pObj, wsKey, pRuntime->NewBoolean(!!pValueObj->GetInteger()));
    }
  }
  return CJS_Return(pObj);
}

CJS_Return CJS_Document::set_info(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::getPropertyInternal(CJS_Runtime* pRuntime,
                                             const ByteString& propName) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDF_Dictionary* pDictionary = m_pFormFillEnv->GetPDFDocument()->GetInfo();
  if (!pDictionary)
    return CJS_Return(false);
  return CJS_Return(
      pRuntime->NewString(pDictionary->GetUnicodeTextFor(propName).c_str()));
}

CJS_Return CJS_Document::setPropertyInternal(CJS_Runtime* pRuntime,
                                             v8::Local<v8::Value> vp,
                                             const ByteString& propName) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDF_Dictionary* pDictionary = m_pFormFillEnv->GetPDFDocument()->GetInfo();
  if (!pDictionary)
    return CJS_Return(false);

  if (!m_pFormFillEnv->GetPermissions(FPDFPERM_MODIFY))
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));

  WideString csProperty = pRuntime->ToWideString(vp);
  pDictionary->SetNewFor<CPDF_String>(propName, PDF_EncodeText(csProperty),
                                      false);
  m_pFormFillEnv->SetChangeMark();
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_creation_date(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "CreationDate");
}

CJS_Return CJS_Document::set_creation_date(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "CreationDate");
}

CJS_Return CJS_Document::get_creator(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "Creator");
}

CJS_Return CJS_Document::set_creator(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "Creator");
}

CJS_Return CJS_Document::get_delay(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(pRuntime->NewBoolean(m_bDelay));
}

CJS_Return CJS_Document::set_delay(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  if (!m_pFormFillEnv->GetPermissions(FPDFPERM_MODIFY))
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));

  m_bDelay = pRuntime->ToBoolean(vp);
  if (m_bDelay) {
    m_DelayData.clear();
    return CJS_Return(true);
  }

  std::list<std::unique_ptr<CJS_DelayData>> DelayDataToProcess;
  DelayDataToProcess.swap(m_DelayData);
  for (const auto& pData : DelayDataToProcess)
    CJS_Field::DoDelay(m_pFormFillEnv.Get(), pData.get());

  return CJS_Return(true);
}

CJS_Return CJS_Document::get_keywords(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "Keywords");
}

CJS_Return CJS_Document::set_keywords(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "Keywords");
}

CJS_Return CJS_Document::get_mod_date(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "ModDate");
}

CJS_Return CJS_Document::set_mod_date(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "ModDate");
}

CJS_Return CJS_Document::get_producer(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "Producer");
}

CJS_Return CJS_Document::set_producer(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "Producer");
}

CJS_Return CJS_Document::get_subject(CJS_Runtime* pRuntime) {
  return getPropertyInternal(pRuntime, "Subject");
}

CJS_Return CJS_Document::set_subject(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return setPropertyInternal(pRuntime, vp, "Subject");
}

CJS_Return CJS_Document::get_title(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv || !m_pFormFillEnv->GetPDFDocument())
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return getPropertyInternal(pRuntime, "Title");
}

CJS_Return CJS_Document::set_title(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  if (!m_pFormFillEnv || !m_pFormFillEnv->GetPDFDocument())
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return setPropertyInternal(pRuntime, vp, "Title");
}

CJS_Return CJS_Document::get_num_pages(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(pRuntime->NewNumber(m_pFormFillEnv->GetPageCount()));
}

CJS_Return CJS_Document::set_num_pages(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::get_external(CJS_Runtime* pRuntime) {
  // In Chrome case, should always return true.
  return CJS_Return(pRuntime->NewBoolean(true));
}

CJS_Return CJS_Document::set_external(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_filesize(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewNumber(0));
}

CJS_Return CJS_Document::set_filesize(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::get_mouse_x(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_mouse_x(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_mouse_y(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_mouse_y(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_URL(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(
      pRuntime->NewString(m_pFormFillEnv->JS_docGetFilePath().c_str()));
}

CJS_Return CJS_Document::set_URL(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::get_base_URL(CJS_Runtime* pRuntime) {
  return CJS_Return(pRuntime->NewString(m_cwBaseURL.c_str()));
}

CJS_Return CJS_Document::set_base_URL(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  m_cwBaseURL = pRuntime->ToWideString(vp);
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_calculate(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  return CJS_Return(pRuntime->NewBoolean(!!pInterForm->IsCalculateEnabled()));
}

CJS_Return CJS_Document::set_calculate(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDFSDK_InterForm* pInterForm = m_pFormFillEnv->GetInterForm();
  pInterForm->EnableCalculate(pRuntime->ToBoolean(vp));
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_document_file_name(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  WideString wsFilePath = m_pFormFillEnv->JS_docGetFilePath();
  size_t i = wsFilePath.GetLength();
  for (; i > 0; i--) {
    if (wsFilePath[i - 1] == L'\\' || wsFilePath[i - 1] == L'/')
      break;
  }
  if (i > 0 && i < wsFilePath.GetLength())
    return CJS_Return(pRuntime->NewString(wsFilePath.c_str() + i));

  return CJS_Return(pRuntime->NewString(L""));
}

CJS_Return CJS_Document::set_document_file_name(CJS_Runtime* pRuntime,
                                                v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::get_path(CJS_Runtime* pRuntime) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(pRuntime->NewString(
      CJS_App::SysPathToPDFPath(m_pFormFillEnv->JS_docGetFilePath()).c_str()));
}

CJS_Return CJS_Document::set_path(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::get_page_window_rect(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_page_window_rect(CJS_Runtime* pRuntime,
                                              v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_layout(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_layout(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::addLink(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::closeDoc(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::getPageBox(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::getAnnot(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  int nPageNo = pRuntime->ToInt32(params[0]);
  WideString swAnnotName = pRuntime->ToWideString(params[1]);
  CPDFSDK_PageView* pPageView = m_pFormFillEnv->GetPageView(nPageNo);
  if (!pPageView)
    return CJS_Return(false);

  CPDFSDK_AnnotIteration annotIteration(pPageView, false);
  CPDFSDK_BAAnnot* pSDKBAAnnot = nullptr;
  for (const auto& pSDKAnnotCur : annotIteration) {
    CPDFSDK_BAAnnot* pBAAnnot =
        static_cast<CPDFSDK_BAAnnot*>(pSDKAnnotCur.Get());
    if (pBAAnnot && pBAAnnot->GetAnnotName() == swAnnotName) {
      pSDKBAAnnot = pBAAnnot;
      break;
    }
  }
  if (!pSDKBAAnnot)
    return CJS_Return(false);

  v8::Local<v8::Object> pObj =
      pRuntime->NewFXJSBoundObject(CJS_Annot::GetObjDefnID());
  if (pObj.IsEmpty())
    return CJS_Return(false);

  CJS_Annot* pJS_Annot =
      static_cast<CJS_Annot*>(pRuntime->GetObjectPrivate(pObj));
  if (!pJS_Annot)
    return CJS_Return(false);

  pJS_Annot->SetSDKAnnot(pSDKBAAnnot);
  return CJS_Return(pJS_Annot->ToV8Object());
}

CJS_Return CJS_Document::getAnnots(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  // TODO(tonikitoo): Add support supported parameters as per
  // the PDF spec.

  int nPageNo = m_pFormFillEnv->GetPageCount();
  v8::Local<v8::Array> annots = pRuntime->NewArray();
  for (int i = 0; i < nPageNo; ++i) {
    CPDFSDK_PageView* pPageView = m_pFormFillEnv->GetPageView(i);
    if (!pPageView)
      return CJS_Return(false);

    CPDFSDK_AnnotIteration annotIteration(pPageView, false);
    for (const auto& pSDKAnnotCur : annotIteration) {
      if (!pSDKAnnotCur)
        return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

      v8::Local<v8::Object> pObj =
          pRuntime->NewFXJSBoundObject(CJS_Annot::GetObjDefnID());
      if (pObj.IsEmpty())
        return CJS_Return(false);

      CJS_Annot* pJS_Annot =
          static_cast<CJS_Annot*>(pRuntime->GetObjectPrivate(pObj));
      pJS_Annot->SetSDKAnnot(static_cast<CPDFSDK_BAAnnot*>(pSDKAnnotCur.Get()));
      pRuntime->PutArrayElement(
          annots, i,
          pJS_Annot ? v8::Local<v8::Value>(pJS_Annot->ToV8Object())
                    : v8::Local<v8::Value>());
    }
  }
  return CJS_Return(annots);
}

CJS_Return CJS_Document::getAnnot3D(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(pRuntime->NewUndefined());
}

CJS_Return CJS_Document::getAnnots3D(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::getOCGs(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::getLinks(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  return CJS_Return(true);
}

bool CJS_Document::IsEnclosedInRect(CFX_FloatRect rect,
                                    CFX_FloatRect LinkRect) {
  return (rect.left <= LinkRect.left && rect.top <= LinkRect.top &&
          rect.right >= LinkRect.right && rect.bottom >= LinkRect.bottom);
}

CJS_Return CJS_Document::addIcon(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 2)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  if (!params[1]->IsObject())
    return CJS_Return(JSGetStringFromID(JSMessage::kTypeError));

  v8::Local<v8::Object> pJSIcon = pRuntime->ToObject(params[1]);
  if (CFXJS_Engine::GetObjDefnID(pJSIcon) != CJS_Icon::GetObjDefnID())
    return CJS_Return(JSGetStringFromID(JSMessage::kTypeError));

  v8::Local<v8::Object> pObj = pRuntime->ToObject(params[1]);
  CJS_Object* obj = pRuntime->GetObjectPrivate(pObj);
  if (!obj)
    return CJS_Return(JSGetStringFromID(JSMessage::kTypeError));

  WideString swIconName = pRuntime->ToWideString(params[0]);
  m_IconNames.push_back(swIconName);
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_icons(CJS_Runtime* pRuntime) {
  if (m_IconNames.empty())
    return CJS_Return(pRuntime->NewUndefined());

  v8::Local<v8::Array> Icons = pRuntime->NewArray();
  int i = 0;
  for (const auto& name : m_IconNames) {
    v8::Local<v8::Object> pObj =
        pRuntime->NewFXJSBoundObject(CJS_Icon::GetObjDefnID());
    if (pObj.IsEmpty())
      return CJS_Return(false);

    CJS_Icon* pJS_Icon =
        static_cast<CJS_Icon*>(pRuntime->GetObjectPrivate(pObj));
    pJS_Icon->SetIconName(name);
    pRuntime->PutArrayElement(Icons, i++,
                              pJS_Icon
                                  ? v8::Local<v8::Value>(pJS_Icon->ToV8Object())
                                  : v8::Local<v8::Value>());
  }
  return CJS_Return(Icons);
}

CJS_Return CJS_Document::set_icons(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  return CJS_Return(JSGetStringFromID(JSMessage::kReadOnlyError));
}

CJS_Return CJS_Document::getIcon(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));

  WideString swIconName = pRuntime->ToWideString(params[0]);
  auto it = std::find(m_IconNames.begin(), m_IconNames.end(), swIconName);
  if (it == m_IconNames.end())
    return CJS_Return(false);

  v8::Local<v8::Object> pObj =
      pRuntime->NewFXJSBoundObject(CJS_Icon::GetObjDefnID());
  if (pObj.IsEmpty())
    return CJS_Return(false);

  CJS_Icon* pJS_Icon = static_cast<CJS_Icon*>(pRuntime->GetObjectPrivate(pObj));
  if (!pJS_Icon)
    return CJS_Return(false);

  pJS_Icon->SetIconName(*it);
  return CJS_Return(pJS_Icon->ToV8Object());
}

CJS_Return CJS_Document::removeIcon(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, no supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::createDataObject(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not implemented.
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_media(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_media(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::calculateNow(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  if (!(m_pFormFillEnv->GetPermissions(FPDFPERM_MODIFY) ||
        m_pFormFillEnv->GetPermissions(FPDFPERM_ANNOT_FORM) ||
        m_pFormFillEnv->GetPermissions(FPDFPERM_FILL_FORM))) {
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));
  }

  m_pFormFillEnv->GetInterForm()->OnCalculate();
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_collab(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_collab(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::getPageNthWord(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  if (!m_pFormFillEnv->GetPermissions(FPDFPERM_EXTRACT_ACCESS))
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));

  // TODO(tsepez): check maximum allowable params.

  int nPageNo = params.size() > 0 ? pRuntime->ToInt32(params[0]) : 0;
  int nWordNo = params.size() > 1 ? pRuntime->ToInt32(params[1]) : 0;
  bool bStrip = params.size() > 2 ? pRuntime->ToBoolean(params[2]) : true;

  CPDF_Document* pDocument = m_pFormFillEnv->GetPDFDocument();
  if (!pDocument)
    return CJS_Return(false);

  if (nPageNo < 0 || nPageNo >= pDocument->GetPageCount())
    return CJS_Return(JSGetStringFromID(JSMessage::kValueError));

  CPDF_Dictionary* pPageDict = pDocument->GetPageDictionary(nPageNo);
  if (!pPageDict)
    return CJS_Return(false);

  CPDF_Page page(pDocument, pPageDict, true);
  page.ParseContent();

  int nWords = 0;
  WideString swRet;
  for (auto& pPageObj : *page.GetPageObjectList()) {
    if (pPageObj->IsText()) {
      CPDF_TextObject* pTextObj = pPageObj->AsText();
      int nObjWords = CountWords(pTextObj);
      if (nWords + nObjWords >= nWordNo) {
        swRet = GetObjWordStr(pTextObj, nWordNo - nWords);
        break;
      }
      nWords += nObjWords;
    }
  }

  if (bStrip)
    swRet.Trim();
  return CJS_Return(pRuntime->NewString(swRet.c_str()));
}

CJS_Return CJS_Document::getPageNthWordQuads(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  if (!m_pFormFillEnv->GetPermissions(FPDFPERM_EXTRACT_ACCESS))
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  return CJS_Return(false);
}

CJS_Return CJS_Document::getPageNumWords(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));
  if (!m_pFormFillEnv->GetPermissions(FPDFPERM_EXTRACT_ACCESS))
    return CJS_Return(JSGetStringFromID(JSMessage::kPermissionError));

  int nPageNo = params.size() > 0 ? pRuntime->ToInt32(params[0]) : 0;
  CPDF_Document* pDocument = m_pFormFillEnv->GetPDFDocument();
  if (nPageNo < 0 || nPageNo >= pDocument->GetPageCount())
    return CJS_Return(JSGetStringFromID(JSMessage::kValueError));

  CPDF_Dictionary* pPageDict = pDocument->GetPageDictionary(nPageNo);
  if (!pPageDict)
    return CJS_Return(false);

  CPDF_Page page(pDocument, pPageDict, true);
  page.ParseContent();

  int nWords = 0;
  for (auto& pPageObj : *page.GetPageObjectList()) {
    if (pPageObj->IsText())
      nWords += CountWords(pPageObj->AsText());
  }

  return CJS_Return(pRuntime->NewNumber(nWords));
}

CJS_Return CJS_Document::getPrintParams(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  v8::Local<v8::Object> pRetObj =
      pRuntime->NewFXJSBoundObject(CJS_PrintParamsObj::GetObjDefnID());
  if (pRetObj.IsEmpty())
    return CJS_Return(false);

  // Not implemented yet.

  return CJS_Return(pRetObj);
}

#define ISLATINWORD(u) (u != 0x20 && u <= 0x28FF)

int CJS_Document::CountWords(CPDF_TextObject* pTextObj) {
  if (!pTextObj)
    return 0;

  int nWords = 0;

  CPDF_Font* pFont = pTextObj->GetFont();
  if (!pFont)
    return 0;

  bool bIsLatin = false;

  for (size_t i = 0, sz = pTextObj->CountChars(); i < sz; ++i) {
    uint32_t charcode = CPDF_Font::kInvalidCharCode;
    float kerning;

    pTextObj->GetCharInfo(i, &charcode, &kerning);
    WideString swUnicode = pFont->UnicodeFromCharCode(charcode);

    uint16_t unicode = 0;
    if (swUnicode.GetLength() > 0)
      unicode = swUnicode[0];

    if (ISLATINWORD(unicode) && bIsLatin)
      continue;

    bIsLatin = ISLATINWORD(unicode);
    if (unicode != 0x20)
      nWords++;
  }

  return nWords;
}

WideString CJS_Document::GetObjWordStr(CPDF_TextObject* pTextObj,
                                       int nWordIndex) {
  WideString swRet;

  CPDF_Font* pFont = pTextObj->GetFont();
  if (!pFont)
    return L"";

  int nWords = 0;
  bool bIsLatin = false;

  for (size_t i = 0, sz = pTextObj->CountChars(); i < sz; ++i) {
    uint32_t charcode = CPDF_Font::kInvalidCharCode;
    float kerning;

    pTextObj->GetCharInfo(i, &charcode, &kerning);
    WideString swUnicode = pFont->UnicodeFromCharCode(charcode);

    uint16_t unicode = 0;
    if (swUnicode.GetLength() > 0)
      unicode = swUnicode[0];

    if (ISLATINWORD(unicode) && bIsLatin) {
    } else {
      bIsLatin = ISLATINWORD(unicode);
      if (unicode != 0x20)
        nWords++;
    }

    if (nWords - 1 == nWordIndex)
      swRet += unicode;
  }

  return swRet;
}

CJS_Return CJS_Document::get_zoom(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_zoom(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::get_zoom_type(CJS_Runtime* pRuntime) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::set_zoom_type(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  return CJS_Return(true);
}

CJS_Return CJS_Document::deletePages(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::extractPages(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::insertPages(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::replacePages(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::getURL(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  // Unsafe, not supported.
  return CJS_Return(true);
}

CJS_Return CJS_Document::gotoNamedDest(
    CJS_Runtime* pRuntime,
    const std::vector<v8::Local<v8::Value>>& params) {
  if (params.size() != 1)
    return CJS_Return(JSGetStringFromID(JSMessage::kParamError));
  if (!m_pFormFillEnv)
    return CJS_Return(JSGetStringFromID(JSMessage::kBadObjectError));

  CPDF_Document* pDocument = m_pFormFillEnv->GetPDFDocument();
  if (!pDocument)
    return CJS_Return(false);

  CPDF_NameTree nameTree(pDocument, "Dests");
  CPDF_Array* destArray =
      nameTree.LookupNamedDest(pDocument, pRuntime->ToWideString(params[0]));
  if (!destArray)
    return CJS_Return(false);

  CPDF_Dest dest(destArray);
  const CPDF_Array* arrayObject = ToArray(dest.GetObject());
  std::vector<float> scrollPositionArray;
  if (arrayObject) {
    for (size_t i = 2; i < arrayObject->GetCount(); i++)
      scrollPositionArray.push_back(arrayObject->GetFloatAt(i));
  }
  pRuntime->BeginBlock();
  m_pFormFillEnv->DoGoToAction(dest.GetDestPageIndex(pDocument),
                               dest.GetZoomMode(), scrollPositionArray.data(),
                               scrollPositionArray.size());
  pRuntime->EndBlock();
  return CJS_Return(true);
}

void CJS_Document::AddDelayData(std::unique_ptr<CJS_DelayData> pData) {
  m_DelayData.push_back(std::move(pData));
}

void CJS_Document::DoFieldDelay(const WideString& sFieldName,
                                int nControlIndex) {
  std::vector<std::unique_ptr<CJS_DelayData>> delayed_data;
  auto iter = m_DelayData.begin();
  while (iter != m_DelayData.end()) {
    auto old = iter++;
    if ((*old)->sFieldName == sFieldName &&
        (*old)->nControlIndex == nControlIndex) {
      delayed_data.push_back(std::move(*old));
      m_DelayData.erase(old);
    }
  }

  for (const auto& pData : delayed_data)
    CJS_Field::DoDelay(m_pFormFillEnv.Get(), pData.get());
}
