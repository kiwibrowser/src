/*
 * Copyright (C) 2006, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_HISTORY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_HISTORY_ITEM_H_

#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/web_scroll_anchor_data.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/loader/frame_loader_types.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/scroll/scroll_types.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class DocumentState;
class EncodedFormData;
class KURL;
class ResourceRequest;

class CORE_EXPORT HistoryItem final
    : public GarbageCollectedFinalized<HistoryItem> {
 public:
  static HistoryItem* Create() { return new HistoryItem; }
  ~HistoryItem();

  const String& UrlString() const;
  KURL Url() const;

  const Referrer& GetReferrer() const;

  EncodedFormData* FormData();
  const AtomicString& FormContentType() const;

  class ViewState {
   public:
    ViewState() : page_scale_factor_(0) {}
    ViewState(const ViewState&) = default;

    ScrollOffset visual_viewport_scroll_offset_;
    ScrollOffset scroll_offset_;
    float page_scale_factor_;
    ScrollAnchorData scroll_anchor_data_;
  };

  ViewState* GetViewState() const { return view_state_.get(); }
  void ClearViewState() { view_state_.reset(); }
  void CopyViewStateFrom(HistoryItem* other) {
    if (other->view_state_)
      view_state_ = std::make_unique<ViewState>(*other->view_state_.get());
    else
      view_state_.reset();
  }

  void SetVisualViewportScrollOffset(const ScrollOffset&);
  void SetScrollOffset(const ScrollOffset&);
  void SetPageScaleFactor(float);

  Vector<String> GetReferencedFilePaths();
  const Vector<String>& GetDocumentState();
  void SetDocumentState(const Vector<String>&);
  void SetDocumentState(DocumentState*);
  void ClearDocumentState();

  void SetURL(const KURL&);
  void SetURLString(const String&);
  void SetReferrer(const Referrer&);

  void SetStateObject(scoped_refptr<SerializedScriptValue>);
  SerializedScriptValue* StateObject() const { return state_object_.get(); }

  void SetItemSequenceNumber(long long number) {
    item_sequence_number_ = number;
  }
  long long ItemSequenceNumber() const { return item_sequence_number_; }

  void SetDocumentSequenceNumber(long long number) {
    document_sequence_number_ = number;
  }
  long long DocumentSequenceNumber() const { return document_sequence_number_; }

  void SetScrollRestorationType(HistoryScrollRestorationType type) {
    scroll_restoration_type_ = type;
  }
  HistoryScrollRestorationType ScrollRestorationType() {
    return scroll_restoration_type_;
  }

  void SetScrollAnchorData(const ScrollAnchorData&);

  void SetFormInfoFromRequest(const ResourceRequest&);
  void SetFormData(scoped_refptr<EncodedFormData>);
  void SetFormContentType(const AtomicString&);

  ResourceRequest GenerateResourceRequest(mojom::FetchCacheMode);

  void Trace(blink::Visitor*);

 private:
  HistoryItem();

  String url_string_;
  Referrer referrer_;

  Vector<String> document_state_vector_;
  Member<DocumentState> document_state_;

  std::unique_ptr<ViewState> view_state_;

  // If two HistoryItems have the same item sequence number, then they are
  // clones of one another. Traversing history from one such HistoryItem to
  // another is a no-op. HistoryItem clones are created for parent and
  // sibling frames when only a subframe navigates.
  int64_t item_sequence_number_;

  // If two HistoryItems have the same document sequence number, then they
  // refer to the same instance of a document. Traversing history from one
  // such HistoryItem to another preserves the document.
  int64_t document_sequence_number_;

  // Type of the scroll restoration for the history item determines if scroll
  // position should be restored when it is loaded during history traversal.
  HistoryScrollRestorationType scroll_restoration_type_;

  // Support for HTML5 History
  scoped_refptr<SerializedScriptValue> state_object_;

  // info used to repost form data
  scoped_refptr<EncodedFormData> form_data_;
  AtomicString form_content_type_;
};  // class HistoryItem

}  // namespace blink

#endif  // HISTORYITEM_H
