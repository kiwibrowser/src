// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/clipboard/arc_clipboard_bridge.h"

#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/utf_string_conversions.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace arc {
namespace {

// Payload in an Android Binder Parcel should be less than 800 Kb. Save 512
// bytes for headers, descriptions and mime types.
constexpr size_t kMaxBinderParcelSizeInBytes = 800 * 1024 - 512;
constexpr char kMimeTypeTextError[] = "text/error";
constexpr char kErrorSizeTooBigForBinder[] = "size too big for binder";

// Singleton factory for ArcClipboardBridge.
class ArcClipboardBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcClipboardBridge,
          ArcClipboardBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcClipboardBridgeFactory";

  static ArcClipboardBridgeFactory* GetInstance() {
    return base::Singleton<ArcClipboardBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcClipboardBridgeFactory>;
  ArcClipboardBridgeFactory() = default;
  ~ArcClipboardBridgeFactory() override = default;
};

mojom::ClipRepresentationPtr CreateHTML(const ui::Clipboard* clipboard) {
  DCHECK(clipboard);

  base::string16 markup16;
  // Unused. URL is sent from CreatePlainText() by reading it from the Bookmark.
  std::string url;
  uint32_t fragment_start, fragment_end;

  clipboard->ReadHTML(ui::CLIPBOARD_TYPE_COPY_PASTE, &markup16, &url,
                      &fragment_start, &fragment_end);

  std::string text(base::UTF16ToUTF8(
      markup16.substr(fragment_start, fragment_end - fragment_start)));

  std::string mime_type(ui::Clipboard::kMimeTypeHTML);

  // Send non-sanitized HTML content. Instance should sanitize it if needed.
  return mojom::ClipRepresentation::New(mime_type,
                                        mojom::ClipValue::NewText(text));
}

mojom::ClipRepresentationPtr CreatePlainText(const ui::Clipboard* clipboard) {
  DCHECK(clipboard);

  // Unused. Title is not used at Instance.
  base::string16 title;
  std::string text;
  std::string mime_type(ui::Clipboard::kMimeTypeText);

  // Both Bookmark and AsciiText are represented by text/plain. If both are
  // present, only use Bookmark.
  clipboard->ReadBookmark(&title, &text);
  if (text.size() == 0)
    clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);

  return mojom::ClipRepresentation::New(mime_type,
                                        mojom::ClipValue::NewText(text));
}

mojom::ClipDataPtr GetClipData(const ui::Clipboard* clipboard) {
  DCHECK(clipboard);

  std::vector<base::string16> mime_types;
  bool contains_files;
  clipboard->ReadAvailableTypes(ui::CLIPBOARD_TYPE_COPY_PASTE, &mime_types,
                                &contains_files);

  mojom::ClipDataPtr clip_data(mojom::ClipData::New());

  // Populate ClipData with ClipRepresentation objects.
  for (const auto& mime_type16 : mime_types) {
    const std::string mime_type(base::UTF16ToUTF8(mime_type16));
    if (mime_type == ui::Clipboard::kMimeTypeHTML) {
      clip_data->representations.push_back(CreateHTML(clipboard));
    } else if (mime_type == ui::Clipboard::kMimeTypeText) {
      clip_data->representations.push_back(CreatePlainText(clipboard));
    } else {
      // TODO(ricardoq): Add other supported mime_types here.
      DLOG(WARNING) << "Unsupported mime type: " << mime_type;
    }
  }
  return clip_data;
}

void ProcessHTML(const mojom::ClipRepresentation* repr,
                 ui::ScopedClipboardWriter* writer) {
  DCHECK(repr);
  DCHECK(repr->value->is_text());
  DCHECK(writer);

  writer->WriteHTML(base::UTF8ToUTF16(repr->value->get_text()), std::string());
}

void ProcessPlainText(const mojom::ClipRepresentation* repr,
                      ui::ScopedClipboardWriter* writer) {
  DCHECK(repr);
  DCHECK(repr->value->is_text());
  DCHECK(writer);

  writer->WriteText(base::UTF8ToUTF16(repr->value->get_text()));
}

bool DoesClipFitIntoInstance(const mojom::ClipDataPtr& clip_data) {
  // Checks whether the ClipData will fit at Instance's Binder.Parcel.
  // (See: android.os.Binder.java # checkParcel() for details).
  //
  // It calculates an upper-bound limit by multiplying UTF8 strings' size by 2.
  //
  // A precise check could be done at Instance, but it will require:
  // 1: Sending the Clip via Mojo to Instance (memory * 2 + time O(memory))
  // 2: Converting the char* (UTF8) to Java UTF16 Strings (memory * 2 again +
  //    time O(memory))
  // 3: Creating a temp Parcel with the clip data (memory * 2 again +
  //    time O(memory))
  //
  // An estimate (non-precise) check could be done at Instance as well, but will
  // require at least steps 1 and 2.
  //
  // A simple screenshot + copy to clipboard at Host could take about 4Mb, since
  // it is encoded in an HTML <IMG> tag.
  //
  // The purpose of this hack, is to avoid sending and converting this 4Mb
  // several times.

  // TODO(ricardoq): Instead of doing UTF8.size() * 2, get the real size from
  // the unconverted UTF16 string.

  size_t size_at_instance_in_bytes = 0;
  for (const auto& repr : clip_data->representations) {
    if (repr->value->is_text())
      size_at_instance_in_bytes +=
          repr->value->get_text().size() * sizeof(base::string16::value_type);
    else
      size_at_instance_in_bytes += repr->value->get_blob().size();
  }

  return size_at_instance_in_bytes < kMaxBinderParcelSizeInBytes;
}

}  // namespace

// static
ArcClipboardBridge* ArcClipboardBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcClipboardBridgeFactory::GetForBrowserContext(context);
}

ArcClipboardBridge::ArcClipboardBridge(content::BrowserContext* context,
                                       ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      event_originated_at_instance_(false) {
  arc_bridge_service_->clipboard()->SetHost(this);
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);
}

ArcClipboardBridge::~ArcClipboardBridge() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ui::ClipboardMonitor::GetInstance()->RemoveObserver(this);
  arc_bridge_service_->clipboard()->SetHost(nullptr);
}

void ArcClipboardBridge::OnClipboardDataChanged() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (event_originated_at_instance_) {
    // Ignore this event, since this event was triggered by a 'copy' in
    // Instance, and not by Host.
    return;
  }

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->clipboard(), OnHostClipboardUpdated);
  if (!clipboard_instance)
    return;

  // TODO(ricardoq): should only inform Instance when a supported mime_type is
  // copied to the clipboard.
  clipboard_instance->OnHostClipboardUpdated();
}

void ArcClipboardBridge::SetClipContent(mojom::ClipDataPtr clip_data) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  if (!clipboard)
    return;

  // Order is important. AutoReset should outlive ScopedClipboardWriter.
  base::AutoReset<bool> auto_reset(&event_originated_at_instance_, true);
  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);

  for (const auto& repr : clip_data->representations) {
    const std::string& mime_type(repr->mime_type);
    if (mime_type == ui::Clipboard::kMimeTypeHTML) {
      ProcessHTML(repr.get(), &writer);
    } else if (mime_type == ui::Clipboard::kMimeTypeText) {
      ProcessPlainText(repr.get(), &writer);
    }
  }
}

void ArcClipboardBridge::GetClipContent(GetClipContentCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  mojom::ClipDataPtr clip_data = GetClipData(clipboard);
  std::move(callback).Run(std::move(clip_data));
}

void ArcClipboardBridge::GetClipContentDeprecated(
    GetClipContentCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  mojom::ClipDataPtr clip_data = GetClipData(clipboard);

  // Old version of ClipboardInstance can't handle a ClipData larger than 800K.
  if (!DoesClipFitIntoInstance(clip_data)) {
    clip_data->representations.clear();
    clip_data->representations.push_back(mojom::ClipRepresentation::New(
        kMimeTypeTextError,
        mojom::ClipValue::NewText(kErrorSizeTooBigForBinder)));
  }
  std::move(callback).Run(std::move(clip_data));
}

}  // namespace arc
