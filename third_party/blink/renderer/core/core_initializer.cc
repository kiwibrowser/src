/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/core_initializer.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/script_streamer_thread.h"
#include "third_party/blink/renderer/core/css/media_feature_names.h"
#include "third_party/blink/renderer/core/css/media_query_evaluator.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/event_names.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/events/event_factory.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context_factory.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/html_tokenizer_names.h"
#include "third_party/blink/renderer/core/input_mode_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/mathml_names.h"
#include "third_party/blink/renderer/core/media_type_names.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/core/xlink_names.h"
#include "third_party/blink/renderer/core/xml_names.h"
#include "third_party/blink/renderer/core/xmlns_names.h"
#include "third_party/blink/renderer/platform/font_family_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_table.h"

namespace blink {

CoreInitializer* CoreInitializer::instance_ = nullptr;

void CoreInitializer::RegisterEventFactory() {
  static bool is_registered = false;
  if (is_registered)
    return;
  is_registered = true;

  Document::RegisterEventFactory(EventFactory::Create());
}

void CoreInitializer::Initialize() {
  // Initialize must be called once by singleton ModulesInitializer.
  DCHECK(!instance_);
  instance_ = this;
  // Note: in order to add core static strings for a new module (1)
  // the value of 'coreStaticStringsCount' must be updated with the
  // added strings count, (2) if the added strings are quialified names
  // the 'qualifiedNamesCount' must be updated as well, (3) the strings
  // 'init()' function call must be added.
  // TODO(mikhail.pozdnyakov@intel.com): We should generate static strings
  // initialization code.
  const unsigned kQualifiedNamesCount =
      HTMLNames::HTMLTagsCount + HTMLNames::HTMLAttrsCount +
      MathMLNames::MathMLTagsCount + MathMLNames::MathMLAttrsCount +
      SVGNames::SVGTagsCount + SVGNames::SVGAttrsCount +
      XLinkNames::XLinkAttrsCount + XMLNSNames::XMLNSAttrsCount +
      XMLNames::XMLAttrsCount;

  const unsigned kCoreStaticStringsCount =
      kQualifiedNamesCount + EventNames::EventNamesCount +
      EventTargetNames::EventTargetNamesCount +
      EventTypeNames::EventTypeNamesCount +
      FetchInitiatorTypeNames::FetchInitiatorTypeNamesCount +
      FontFamilyNames::FontFamilyNamesCount +
      HTMLTokenizerNames::HTMLTokenizerNamesCount + HTTPNames::HTTPNamesCount +
      InputModeNames::InputModeNamesCount +
      InputTypeNames::InputTypeNamesCount +
      MediaFeatureNames::MediaFeatureNamesCount +
      MediaTypeNames::MediaTypeNamesCount;

  StringImpl::ReserveStaticStringsCapacityForSize(
      kCoreStaticStringsCount + StringImpl::AllStaticStrings().size());
  QualifiedName::InitAndReserveCapacityForSize(kQualifiedNamesCount);

  AtomicStringTable::Instance().ReserveCapacity(kCoreStaticStringsCount);

  HTMLNames::init();
  SVGNames::init();
  XLinkNames::init();
  MathMLNames::init();
  XMLNSNames::init();
  XMLNames::init();

  EventNames::init();
  EventTargetNames::init();
  EventTypeNames::init();
  FetchInitiatorTypeNames::init();
  FontFamilyNames::init();
  HTMLTokenizerNames::init();
  HTTPNames::init();
  InputModeNames::init();
  InputTypeNames::init();
  MediaFeatureNames::init();
  MediaTypeNames::init();

  MediaQueryEvaluator::Init();
  CSSParserTokenRange::InitStaticEOFToken();

  StyleChangeExtraData::Init();

  KURL::Initialize();
  SchemeRegistry::Initialize();
  SecurityPolicy::Init();

  RegisterEventFactory();

  StringImpl::FreezeStaticStrings();

  ScriptStreamerThread::Init();
}

}  // namespace blink
