// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard_monitor.h"

#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_observer.h"

namespace ui {

ClipboardMonitor::ClipboardMonitor() {}

ClipboardMonitor::~ClipboardMonitor() {
  DCHECK(CalledOnValidThread());
}

// static
ClipboardMonitor* ClipboardMonitor::GetInstance() {
  return base::Singleton<ClipboardMonitor,
                         base::LeakySingletonTraits<ClipboardMonitor>>::get();
}

void ClipboardMonitor::NotifyClipboardDataChanged() {
  DCHECK(CalledOnValidThread());
  for (ClipboardObserver& observer : observers_)
    observer.OnClipboardDataChanged();
}

void ClipboardMonitor::AddObserver(ClipboardObserver* observer) {
  DCHECK(CalledOnValidThread());
  observers_.AddObserver(observer);
}

void ClipboardMonitor::RemoveObserver(ClipboardObserver* observer) {
  DCHECK(CalledOnValidThread());
  observers_.RemoveObserver(observer);
}

}  // namespace ui
