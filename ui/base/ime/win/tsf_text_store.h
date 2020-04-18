// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_WIN_TSF_TEXT_STORE_H_
#define UI_BASE_IME_WIN_TSF_TEXT_STORE_H_

#include <msctf.h>
#include <wrl/client.h>
#include <deque>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/base/ime/ime_text_span.h"
#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/gfx/range/range.h"

namespace ui {
class TextInputClient;

// TSFTextStore is used to interact with the input method via TSF manager.
// TSFTextStore have a string buffer which is manipulated by TSF manager through
// ITextStoreACP interface methods such as SetText().
// When the input method updates the composition, TSFTextStore calls
// TextInputClient::SetCompositionText(). And when the input method finishes the
// composition, TSFTextStore calls TextInputClient::InsertText() and clears the
// buffer.
//
// How TSFTextStore works:
//  - The user enters "a".
//    - The input method set composition as "a".
//    - TSF manager calls TSFTextStore::RequestLock().
//    - TSFTextStore callbacks ITextStoreACPSink::OnLockGranted().
//    - In OnLockGranted(), TSF manager calls
//      - TSFTextStore::OnStartComposition()
//      - TSFTextStore::SetText()
//        The string buffer is set as "a".
//      - TSFTextStore::OnUpdateComposition()
//      - TSFTextStore::OnEndEdit()
//        TSFTextStore can get the composition information such as underlines.
//   - TSFTextStore calls TextInputClient::SetCompositionText().
//     "a" is shown with an underline as composition string.
// - The user enters <space>.
//    - The input method set composition as "A".
//    - TSF manager calls TSFTextStore::RequestLock().
//    - TSFTextStore callbacks ITextStoreACPSink::OnLockGranted().
//    - In OnLockGranted(), TSF manager calls
//      - TSFTextStore::SetText()
//        The string buffer is set as "A".
//      - TSFTextStore::OnUpdateComposition()
//      - TSFTextStore::OnEndEdit()
//   - TSFTextStore calls TextInputClient::SetCompositionText().
//     "A" is shown with an underline as composition string.
// - The user enters <enter>.
//    - The input method commits "A".
//    - TSF manager calls TSFTextStore::RequestLock().
//    - TSFTextStore callbacks ITextStoreACPSink::OnLockGranted().
//    - In OnLockGranted(), TSF manager calls
//      - TSFTextStore::OnEndComposition()
//      - TSFTextStore::OnEndEdit()
//        TSFTextStore knows "A" is committed.
//   - TSFTextStore calls TextInputClient::InsertText().
//     "A" is shown as committed string.
//   - TSFTextStore clears the string buffer.
//   - TSFTextStore calls OnSelectionChange(), OnLayoutChange() and
//     OnTextChange() of ITextStoreACPSink to let TSF manager know that the
//     string buffer has been changed.
//
// About the locking scheme:
// When TSF manager manipulates the string buffer it calls RequestLock() to get
// the lock of the document. If TSFTextStore can grant the lock request, it
// callbacks ITextStoreACPSink::OnLockGranted().
// RequestLock() is called from only one thread, but called recursively in
// OnLockGranted() or OnSelectionChange() or OnLayoutChange() or OnTextChange().
// If the document is locked and the lock request is asynchronous, TSFTextStore
// queues the request. The queued requests will be handled after the current
// lock is removed.
// More information about document locks can be found here:
//   http://msdn.microsoft.com/en-us/library/ms538064
//
// More information about TSF can be found here:
//   http://msdn.microsoft.com/en-us/library/ms629032
class UI_BASE_IME_EXPORT TSFTextStore : public ITextStoreACP,
                                        public ITfContextOwnerCompositionSink,
                                        public ITfTextEditSink {
 public:
  TSFTextStore();
  virtual ~TSFTextStore();

  // ITextStoreACP:
  STDMETHOD_(ULONG, AddRef)() override;
  STDMETHOD_(ULONG, Release)() override;
  STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override;
  STDMETHOD(AdviseSink)(REFIID iid, IUnknown* unknown, DWORD mask) override;
  STDMETHOD(FindNextAttrTransition)
  (LONG acp_start,
   LONG acp_halt,
   ULONG num_filter_attributes,
   const TS_ATTRID* filter_attributes,
   DWORD flags,
   LONG* acp_next,
   BOOL* found,
   LONG* found_offset) override;
  STDMETHOD(GetACPFromPoint)
  (TsViewCookie view_cookie,
   const POINT* point,
   DWORD flags,
   LONG* acp) override;
  STDMETHOD(GetActiveView)(TsViewCookie* view_cookie) override;
  STDMETHOD(GetEmbedded)
  (LONG acp_pos, REFGUID service, REFIID iid, IUnknown** unknown) override;
  STDMETHOD(GetEndACP)(LONG* acp) override;
  STDMETHOD(GetFormattedText)
  (LONG acp_start, LONG acp_end, IDataObject** data_object) override;
  STDMETHOD(GetScreenExt)(TsViewCookie view_cookie, RECT* rect) override;
  STDMETHOD(GetSelection)
  (ULONG selection_index,
   ULONG selection_buffer_size,
   TS_SELECTION_ACP* selection_buffer,
   ULONG* fetched_count) override;
  STDMETHOD(GetStatus)(TS_STATUS* pdcs) override;
  STDMETHOD(GetText)
  (LONG acp_start,
   LONG acp_end,
   wchar_t* text_buffer,
   ULONG text_buffer_size,
   ULONG* text_buffer_copied,
   TS_RUNINFO* run_info_buffer,
   ULONG run_info_buffer_size,
   ULONG* run_info_buffer_copied,
   LONG* next_acp) override;
  STDMETHOD(GetTextExt)
  (TsViewCookie view_cookie,
   LONG acp_start,
   LONG acp_end,
   RECT* rect,
   BOOL* clipped) override;
  STDMETHOD(GetWnd)(TsViewCookie view_cookie, HWND* window_handle) override;
  STDMETHOD(InsertEmbedded)
  (DWORD flags,
   LONG acp_start,
   LONG acp_end,
   IDataObject* data_object,
   TS_TEXTCHANGE* change) override;
  STDMETHOD(InsertEmbeddedAtSelection)
  (DWORD flags,
   IDataObject* data_object,
   LONG* acp_start,
   LONG* acp_end,
   TS_TEXTCHANGE* change) override;
  STDMETHOD(InsertTextAtSelection)
  (DWORD flags,
   const wchar_t* text_buffer,
   ULONG text_buffer_size,
   LONG* acp_start,
   LONG* acp_end,
   TS_TEXTCHANGE* text_change) override;
  STDMETHOD(QueryInsert)
  (LONG acp_test_start,
   LONG acp_test_end,
   ULONG text_size,
   LONG* acp_result_start,
   LONG* acp_result_end) override;
  STDMETHOD(QueryInsertEmbedded)
  (const GUID* service, const FORMATETC* format, BOOL* insertable) override;
  STDMETHOD(RequestAttrsAtPosition)
  (LONG acp_pos,
   ULONG attribute_buffer_size,
   const TS_ATTRID* attribute_buffer,
   DWORD flags) override;
  STDMETHOD(RequestAttrsTransitioningAtPosition)
  (LONG acp_pos,
   ULONG attribute_buffer_size,
   const TS_ATTRID* attribute_buffer,
   DWORD flags) override;
  STDMETHOD(RequestLock)(DWORD lock_flags, HRESULT* result) override;
  STDMETHOD(RequestSupportedAttrs)
  (DWORD flags,
   ULONG attribute_buffer_size,
   const TS_ATTRID* attribute_buffer) override;
  STDMETHOD(RetrieveRequestedAttrs)
  (ULONG attribute_buffer_size,
   TS_ATTRVAL* attribute_buffer,
   ULONG* attribute_buffer_copied) override;
  STDMETHOD(SetSelection)
  (ULONG selection_buffer_size,
   const TS_SELECTION_ACP* selection_buffer) override;
  STDMETHOD(SetText)
  (DWORD flags,
   LONG acp_start,
   LONG acp_end,
   const wchar_t* text_buffer,
   ULONG text_buffer_size,
   TS_TEXTCHANGE* text_change) override;
  STDMETHOD(UnadviseSink)(IUnknown* unknown) override;

  // ITfContextOwnerCompositionSink:
  STDMETHOD(OnStartComposition)
  (ITfCompositionView* composition_view, BOOL* ok) override;
  STDMETHOD(OnUpdateComposition)
  (ITfCompositionView* composition_view, ITfRange* range) override;
  STDMETHOD(OnEndComposition)(ITfCompositionView* composition_view) override;

  // ITfTextEditSink:
  STDMETHOD(OnEndEdit)
  (ITfContext* context,
   TfEditCookie read_only_edit_cookie,
   ITfEditRecord* edit_record) override;

  // Sets currently focused TextInputClient.
  void SetFocusedTextInputClient(HWND focused_window,
                                 TextInputClient* text_input_client);
  // Removes currently focused TextInputClient.
  void RemoveFocusedTextInputClient(TextInputClient* text_input_client);

  // Cancels the ongoing composition if exists.
  bool CancelComposition();

  // Confirms the ongoing composition if exists.
  bool ConfirmComposition();

  // Sends OnLayoutChange() via |text_store_acp_sink_|.
  void SendOnLayoutChange();

 private:
  friend class TSFTextStoreTest;
  friend class TSFTextStoreTestCallback;

  // Checks if the document has a read-only lock.
  bool HasReadLock() const;

  // Checks if the document has a read and write lock.
  bool HasReadWriteLock() const;

  // Gets the display attribute structure.
  bool GetDisplayAttribute(TfGuidAtom guid_atom,
                           TF_DISPLAYATTRIBUTE* attribute);

  // Gets the committed string size and underline information of the context.
  bool GetCompositionStatus(ITfContext* context,
                            const TfEditCookie read_only_edit_cookie,
                            size_t* committed_size,
                            ImeTextSpans* spans);

  // The refrence count of this instance.
  volatile LONG ref_count_ = 0;

  // A pointer of ITextStoreACPSink, this instance is given in AdviseSink.
  Microsoft::WRL::ComPtr<ITextStoreACPSink> text_store_acp_sink_;

  // The current mask of |text_store_acp_sink_|.
  DWORD text_store_acp_sink_mask_ = 0;

  // HWND of the current view window which is set in SetFocusedTextInputClient.
  HWND window_handle_ = nullptr;

  // Current TextInputClient which is set in SetFocusedTextInputClient.
  TextInputClient* text_input_client_ = nullptr;

  // TODO(dtapuska): determine if we can expose more the entire document
  // more than the committed string and composition string to the TIP.
  //  |string_buffer_| contains committed string and composition string.
  //  Example: "aoi" is committed, and "umi" is under composition.
  //    |string_buffer_|: "aoiumi"
  //    |committed_size_|: 3
  base::string16 string_buffer_;
  size_t committed_size_ = 0;

  //  |selection_start_| and |selection_end_| indicates the selection range.
  //  Example: "iue" is selected
  //    |string_buffer_|: "aiueo"
  //    |selection_.start()|: 1
  //    |selection_.end()|: 4
  gfx::Range selection_;

  //  |start_offset| and |end_offset| of |text_spans_| indicates
  //  the offsets in |string_buffer_|.
  //  Example: "aoi" is committed. There are two underlines in "umi" and "no".
  //    |string_buffer_|: "aoiumino"
  //    |committed_size_|: 3
  //    text_spans_[0].start_offset: 3
  //    text_spans_[0].end_offset: 6
  //    text_spans_[1].start_offset: 6
  //    text_spans_[1].end_offset: 8
  ImeTextSpans text_spans_;

  // |edit_flag_| indicates that the status is edited during
  // ITextStoreACPSink::OnLockGranted().
  bool edit_flag_ = false;

  // The type of current lock.
  //   0: No lock.
  //   TS_LF_READ: read-only lock.
  //   TS_LF_READWRITE: read/write lock.
  DWORD current_lock_type_ = 0;

  // Queue of the lock request used in RequestLock().
  std::deque<DWORD> lock_queue_;

  // Category manager and Display attribute manager are used to obtain the
  // attributes of the composition string.
  Microsoft::WRL::ComPtr<ITfCategoryMgr> category_manager_;
  Microsoft::WRL::ComPtr<ITfDisplayAttributeMgr> display_attribute_manager_;

  DISALLOW_COPY_AND_ASSIGN(TSFTextStore);
};

}  // namespace ui

#endif  // UI_BASE_IME_WIN_TSF_TEXT_STORE_H_
