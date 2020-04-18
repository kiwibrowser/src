/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/css/document_style_sheet_collection.h"

#include "third_party/blink/renderer/core/css/document_style_sheet_collector.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/css/resolver/viewport_style_resolver.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/css/style_sheet_candidate.h"
#include "third_party/blink/renderer/core/css/style_sheet_list.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/processing_instruction.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

DocumentStyleSheetCollection::DocumentStyleSheetCollection(
    TreeScope& tree_scope)
    : TreeScopeStyleSheetCollection(tree_scope) {
  DCHECK_EQ(tree_scope.RootNode(), tree_scope.RootNode().GetDocument());
}

void DocumentStyleSheetCollection::CollectStyleSheetsFromCandidates(
    StyleEngine& master_engine,
    DocumentStyleSheetCollector& collector) {
  // TODO(keishi) Check added for crbug.com/699269 diagnosis. Remove once done.
  CHECK(HeapObjectHeader::FromPayload(this)->IsValid());
  CHECK(ThreadState::Current()->IsOnThreadHeap(this));
  for (Node* n : style_sheet_candidate_nodes_) {
    CHECK(HeapObjectHeader::FromPayload(n)->IsValid());
    CHECK(ThreadState::Current()->IsOnThreadHeap(n));
    StyleSheetCandidate candidate(*n);

    DCHECK(!candidate.IsXSL());
    if (candidate.IsImport()) {
      Document* document = candidate.ImportedDocument();
      if (!document)
        continue;
      if (collector.HasVisited(document))
        continue;
      collector.WillVisit(document);

      document->GetStyleEngine().UpdateActiveStyleSheetsInImport(master_engine,
                                                                 collector);
      continue;
    }

    if (candidate.IsEnabledAndLoading())
      continue;

    StyleSheet* sheet = candidate.Sheet();
    if (!sheet)
      continue;

    collector.AppendSheetForList(sheet);
    if (!candidate.CanBeActivated(
            GetDocument().GetStyleEngine().PreferredStylesheetSetName()))
      continue;

    CSSStyleSheet* css_sheet = ToCSSStyleSheet(sheet);
    collector.AppendActiveStyleSheet(
        std::make_pair(css_sheet, master_engine.RuleSetForSheet(*css_sheet)));
  }

  if (!GetTreeScope().HasMoreStyleSheets())
    return;

  StyleSheetList& more_style_sheets = GetTreeScope().MoreStyleSheets();
  unsigned length = more_style_sheets.length();
  for (unsigned index = 0; index < length; ++index) {
    StyleSheet* sheet = more_style_sheets.item(index);
    if (!sheet)
      continue;
    CSSStyleSheet* css_sheet = ToCSSStyleSheet(sheet);
    if (!css_sheet ||
        !css_sheet->CanBeActivated(
            GetDocument().GetStyleEngine().PreferredStylesheetSetName()))
      continue;
    collector.AppendSheetForList(sheet);
    collector.AppendActiveStyleSheet(
        std::make_pair(css_sheet, master_engine.RuleSetForSheet(*css_sheet)));
  }
}

void DocumentStyleSheetCollection::CollectStyleSheets(
    StyleEngine& master_engine,
    DocumentStyleSheetCollector& collector) {
  for (auto& sheet :
       GetDocument().GetStyleEngine().InjectedAuthorStyleSheets()) {
    collector.AppendActiveStyleSheet(std::make_pair(
        sheet.second,
        GetDocument().GetStyleEngine().RuleSetForSheet(*sheet.second)));
  }
  CollectStyleSheetsFromCandidates(master_engine, collector);
  if (CSSStyleSheet* inspector_sheet =
          GetDocument().GetStyleEngine().InspectorStyleSheet()) {
    collector.AppendActiveStyleSheet(std::make_pair(
        inspector_sheet,
        GetDocument().GetStyleEngine().RuleSetForSheet(*inspector_sheet)));
  }
}

void DocumentStyleSheetCollection::UpdateActiveStyleSheets(
    StyleEngine& master_engine) {
  // StyleSheetCollection is GarbageCollected<>, allocate it on the heap.
  StyleSheetCollection* collection = StyleSheetCollection::Create();
  ActiveDocumentStyleSheetCollector collector(*collection);
  CollectStyleSheets(master_engine, collector);
  ApplyActiveStyleSheetChanges(*collection);
}

void DocumentStyleSheetCollection::CollectViewportRules(
    ViewportStyleResolver& viewport_resolver) {
  for (Node* node : style_sheet_candidate_nodes_) {
    StyleSheetCandidate candidate(*node);

    if (candidate.IsImport())
      continue;
    StyleSheet* sheet = candidate.Sheet();
    if (!sheet)
      continue;
    if (!candidate.CanBeActivated(
            GetDocument().GetStyleEngine().PreferredStylesheetSetName()))
      continue;
    viewport_resolver.CollectViewportRulesFromAuthorSheet(
        *ToCSSStyleSheet(sheet));
  }
}

}  // namespace blink
