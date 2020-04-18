// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ui/base/win/accessibility_misc_utils.h"

#include "base/logging.h"
#include "ui/base/win/atl_module.h"

namespace base {
namespace win {

// UIA TextProvider implementation.
UIATextProvider::UIATextProvider()
    : editable_(false) {}

UIATextProvider::~UIATextProvider() {
}

// static
bool UIATextProvider::CreateTextProvider(const string16& value,
                                         bool editable,
                                         IUnknown** provider) {
  // Make sure ATL is initialized in this module.
  ui::win::CreateATLModuleIfNeeded();

  CComObject<UIATextProvider>* text_provider = NULL;
  HRESULT hr = CComObject<UIATextProvider>::CreateInstance(&text_provider);
  if (SUCCEEDED(hr)) {
    DCHECK(text_provider);
    text_provider->set_editable(editable);
    text_provider->set_value(value);
    text_provider->AddRef();
    *provider = static_cast<ITextProvider*>(text_provider);
    return true;
  }
  return false;
}

STDMETHODIMP UIATextProvider::get_IsReadOnly(BOOL* read_only) {
  *read_only = !editable_;
  return S_OK;
}

//
// IValueProvider methods.
//

STDMETHODIMP UIATextProvider::SetValue(const wchar_t* val) {
  return E_NOTIMPL;
}

STDMETHODIMP UIATextProvider::get_Value(BSTR* value) {
  *value = SysAllocString(value_.c_str());
  return S_OK;
}

//
// ITextProvider methods.
//

STDMETHODIMP UIATextProvider::GetSelection(SAFEARRAY** ret) {
  return E_NOTIMPL;
}

STDMETHODIMP UIATextProvider::GetVisibleRanges(SAFEARRAY** ret) {
  return E_NOTIMPL;
}

STDMETHODIMP UIATextProvider::RangeFromChild(IRawElementProviderSimple* child,
                                             ITextRangeProvider** ret) {
  return E_NOTIMPL;
}

STDMETHODIMP UIATextProvider::RangeFromPoint(struct UiaPoint point,
                                             ITextRangeProvider** ret) {
  return E_NOTIMPL;
}

STDMETHODIMP UIATextProvider::get_DocumentRange(ITextRangeProvider** ret) {
  return E_NOTIMPL;
}

STDMETHODIMP UIATextProvider::get_SupportedTextSelection(
    enum SupportedTextSelection* ret) {
  return E_NOTIMPL;
}

}  // namespace win
}  // namespace base
