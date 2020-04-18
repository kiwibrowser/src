// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/pdf_compositor/pdf_compositor_service.h"

#include <utility>

#include <memory>

#include "base/lazy_instance.h"
#include "base/memory/discardable_memory.h"
#include "build/build_config.h"
#include "components/services/pdf_compositor/pdf_compositor_impl.h"
#include "components/services/pdf_compositor/public/interfaces/pdf_compositor.mojom.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/base/ui_base_features.h"

#if defined(OS_WIN)
#include "content/public/child/dwrite_font_proxy_init_win.h"
#elif defined(OS_MACOSX)
#include "third_party/blink/public/platform/platform.h"
#include "third_party/skia/include/ports/SkFontMgr.h"
#elif defined(OS_POSIX) && !defined(OS_ANDROID)
#include "third_party/blink/public/platform/platform.h"
#endif

namespace {

void OnPdfCompositorRequest(
    const std::string& creator,
    service_manager::ServiceContextRefFactory* ref_factory,
    printing::mojom::PdfCompositorRequest request) {
  mojo::MakeStrongBinding(std::make_unique<printing::PdfCompositorImpl>(
                              creator, ref_factory->CreateRef()),
                          std::move(request));
}
}  // namespace

namespace printing {

PdfCompositorService::PdfCompositorService(const std::string& creator)
    : creator_(creator.empty() ? "Chromium" : creator), weak_factory_(this) {}

PdfCompositorService::~PdfCompositorService() {
#if defined(OS_WIN)
  content::UninitializeDWriteFontProxy();
#endif
}

// static
std::unique_ptr<service_manager::Service> PdfCompositorService::Create(
    const std::string& creator) {
#if defined(OS_WIN)
  // Initialize direct write font proxy so skia can use it.
  content::InitializeDWriteFontProxy();
#endif
  return std::make_unique<printing::PdfCompositorService>(creator);
}

void PdfCompositorService::PrepareToStart() {
  // Set up discardable memory manager.
  discardable_memory::mojom::DiscardableSharedMemoryManagerPtr manager_ptr;
  if (features::IsMashEnabled()) {
#if defined(USE_AURA)
    context()->connector()->BindInterface(ui::mojom::kServiceName,
                                          &manager_ptr);
#else
    NOTREACHED();
#endif
  } else {
    context()->connector()->BindInterface(content::mojom::kBrowserServiceName,
                                          &manager_ptr);
  }
  discardable_shared_memory_manager_ = std::make_unique<
      discardable_memory::ClientDiscardableSharedMemoryManager>(
      std::move(manager_ptr), content::UtilityThread::Get()->GetIOTaskRunner());
  DCHECK(discardable_shared_memory_manager_);
  base::DiscardableMemoryAllocator::SetInstance(
      discardable_shared_memory_manager_.get());

#if defined(OS_POSIX) && !defined(OS_ANDROID)
  // Check that we have sandbox support on this platform.
  DCHECK(blink::Platform::Current()->GetSandboxSupport());
#endif

#if defined(OS_MACOSX)
  // Check that font access is granted.
  // This doesn't do comprehensive tests to make sure fonts can work properly.
  // It is just a quick and simple check to catch things like improper sandbox
  // policy setup.
  DCHECK(SkFontMgr::RefDefault()->countFamilies());

  // Initialize a connection to FontLoaderMac service so blink platform's web
  // sandbox support can communicate with it to load font.
  content::UtilityThread::Get()->InitializeFontLoaderMac(
      context()->connector());
#endif
}

void PdfCompositorService::OnStart() {
  PrepareToStart();

  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::Bind(&OnPdfCompositorRequest, creator_, ref_factory_.get()));
}

void PdfCompositorService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace printing
