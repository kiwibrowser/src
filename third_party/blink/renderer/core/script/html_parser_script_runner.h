/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_HTML_PARSER_SCRIPT_RUNNER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_HTML_PARSER_SCRIPT_RUNNER_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/script_streamer.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_reentry_permit.h"
#include "third_party/blink/renderer/core/script/pending_script.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_client.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"

namespace blink {

class Document;
class Element;
class HTMLParserScriptRunnerHost;
class ScriptLoader;

// HTMLParserScriptRunner is responsible for for arranging the execution of
// script elements inserted by the parser, according to the rules for
// 'An end tag whose tag name is "script"':
// https://html.spec.whatwg.org/multipage/parsing.html#scriptEndTag
//
// If a script blocks parsing, this class is responsible for holding it, and
// executing it when required.
//
// An HTMLParserScriptRunner is owned by its host, an HTMLDocumentParser.
class HTMLParserScriptRunner final
    : public GarbageCollectedFinalized<HTMLParserScriptRunner>,
      public PendingScriptClient,
      public TraceWrapperBase {
  USING_GARBAGE_COLLECTED_MIXIN(HTMLParserScriptRunner);

 public:
  static HTMLParserScriptRunner* Create(HTMLParserReentryPermit* reentry_permit,
                                        Document* document,
                                        HTMLParserScriptRunnerHost* host) {
    return new HTMLParserScriptRunner(reentry_permit, document, host);
  }
  ~HTMLParserScriptRunner() override;

  // Invoked when the parser is detached.
  //
  // We don't need to call Detach() as a prefinalizer, because PendingScripts
  // are Dispose()d in PendingScripts' prefinalizers.
  void Detach();

  // Processes the passed in script and any pending scripts if possible.
  // This does not necessarily run the script immediately. For instance,
  // execution may not happen until the script loads from the network, or after
  // the document finishes parsing.
  void ProcessScriptElement(Element*,
                            const TextPosition& script_start_position);

  // Invoked when the parsing-blocking script resource has loaded, to execute
  // parsing-blocking scripts.
  void ExecuteScriptsWaitingForLoad(PendingScript*);

  // Invoked when all script-blocking resources (e.g., stylesheets) have loaded,
  // to execute parsing-blocking scripts.
  void ExecuteScriptsWaitingForResources();

  // Invoked when parsing is stopping, to execute any deferred scripts.
  bool ExecuteScriptsWaitingForParsing();

  bool HasParserBlockingScript() const;
  bool IsExecutingScript() const {
    return !!reentry_permit_->ScriptNestingLevel();
  }

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "HTMLParserScriptRunner";
  }

 private:
  HTMLParserScriptRunner(HTMLParserReentryPermit*,
                         Document*,
                         HTMLParserScriptRunnerHost*);

  // PendingScriptClient
  void PendingScriptFinished(PendingScript*) override;

  void ExecutePendingScriptAndDispatchEvent(PendingScript*,
                                            ScriptStreamer::Type);
  void ExecuteParsingBlockingScripts();

  void RequestParsingBlockingScript(ScriptLoader*);
  void RequestDeferredScript(ScriptLoader*);

  // Processes the provided script element, but does not execute any
  // parsing-blocking scripts that may remain after execution.
  void ProcessScriptElementInternal(Element*,
                                    const TextPosition& script_start_position);

  const PendingScript* ParserBlockingScript() const {
    return parser_blocking_script_;
  }

  bool IsParserBlockingScriptReady();

  void PossiblyFetchBlockedDocWriteScript(PendingScript*);

  scoped_refptr<HTMLParserReentryPermit> reentry_permit_;
  Member<Document> document_;
  Member<HTMLParserScriptRunnerHost> host_;

  // https://html.spec.whatwg.org/multipage/scripting.html#pending-parsing-blocking-script
  TraceWrapperMember<PendingScript> parser_blocking_script_;

  // https://html.spec.whatwg.org/multipage/scripting.html#list-of-scripts-that-will-execute-when-the-document-has-finished-parsing
  HeapDeque<TraceWrapperMember<PendingScript>>
      scripts_to_execute_after_parsing_;

  DISALLOW_COPY_AND_ASSIGN(HTMLParserScriptRunner);
};

}  // namespace blink

#endif
