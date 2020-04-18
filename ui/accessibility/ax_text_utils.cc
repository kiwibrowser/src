// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_text_utils.h"

#include "base/i18n/break_iterator.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

namespace ui {

// line_breaks is a Misnomer. Blink provides the start offsets of each line
// not the line breaks.
// TODO(nektar): Rename line_breaks a11y attribute and variable references.
size_t FindAccessibleTextBoundary(const base::string16& text,
                                  const std::vector<int>& line_breaks,
                                  TextBoundaryType boundary,
                                  size_t start_offset,
                                  TextBoundaryDirection direction,
                                  ax::mojom::TextAffinity affinity) {
  size_t text_size = text.size();
  DCHECK_LE(start_offset, text_size);

  if (boundary == CHAR_BOUNDARY) {
    if (direction == FORWARDS_DIRECTION && start_offset < text_size)
      return start_offset + 1;
    else
      return start_offset;
  }

  base::i18n::BreakIterator word_iter(text,
                                      base::i18n::BreakIterator::BREAK_WORD);
  if (boundary == WORD_BOUNDARY) {
    if (!word_iter.Init())
      return start_offset;
  }

  if (boundary == LINE_BOUNDARY) {
    if (direction == FORWARDS_DIRECTION) {
      for (size_t j = 0; j < line_breaks.size(); ++j) {
          size_t line_break = line_breaks[j] >= 0 ? line_breaks[j] : 0;
          if ((affinity == ax::mojom::TextAffinity::kDownstream &&
               line_break > start_offset) ||
              (affinity == ax::mojom::TextAffinity::kUpstream &&
               line_break >= start_offset)) {
            return line_break;
        }
      }
      return text_size;
    } else {
      for (size_t j = line_breaks.size(); j != 0; --j) {
        size_t line_break = line_breaks[j - 1] >= 0 ? line_breaks[j - 1] : 0;
        if ((affinity == ax::mojom::TextAffinity::kDownstream &&
             line_break <= start_offset) ||
            (affinity == ax::mojom::TextAffinity::kUpstream &&
             line_break < start_offset)) {
          return line_break;
        }
      }
      return 0;
    }
  }

  size_t result = start_offset;
  for (;;) {
    size_t pos;
    if (direction == FORWARDS_DIRECTION) {
      if (result >= text_size)
        return text_size;
      pos = result;
    } else {
      if (result == 0)
        return 0;
      pos = result - 1;
    }

    switch (boundary) {
      case CHAR_BOUNDARY:
      case LINE_BOUNDARY:
        NOTREACHED();  // These are handled above.
        break;
      case WORD_BOUNDARY:
        if (word_iter.IsStartOfWord(result)) {
          // If we are searching forward and we are still at the start offset,
          // we need to find the next word.
          if (direction == BACKWARDS_DIRECTION || result != start_offset)
            return result;
        }
        break;
      case PARAGRAPH_BOUNDARY:
        if (text[pos] == '\n')
          return result;
        break;
      case SENTENCE_BOUNDARY:
        if ((text[pos] == '.' || text[pos] == '!' || text[pos] == '?') &&
            (pos == text_size - 1 ||
             base::IsUnicodeWhitespace(text[pos + 1]))) {
          return result;
        }
        break;
      case ALL_BOUNDARY:
      default:
        break;
    }

    if (direction == FORWARDS_DIRECTION) {
      result++;
    } else {
      result--;
    }
  }
}

base::string16 ActionVerbToLocalizedString(
    const ax::mojom::DefaultActionVerb action_verb) {
  switch (action_verb) {
    case ax::mojom::DefaultActionVerb::kNone:
      return base::string16();
    case ax::mojom::DefaultActionVerb::kActivate:
      return l10n_util::GetStringUTF16(IDS_AX_ACTIVATE_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kCheck:
      return l10n_util::GetStringUTF16(IDS_AX_CHECK_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kClick:
      return l10n_util::GetStringUTF16(IDS_AX_CLICK_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kClickAncestor:
      return l10n_util::GetStringUTF16(IDS_AX_CLICK_ANCESTOR_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kJump:
      return l10n_util::GetStringUTF16(IDS_AX_JUMP_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kOpen:
      return l10n_util::GetStringUTF16(IDS_AX_OPEN_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kPress:
      return l10n_util::GetStringUTF16(IDS_AX_PRESS_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kSelect:
      return l10n_util::GetStringUTF16(IDS_AX_SELECT_ACTION_VERB);
    case ax::mojom::DefaultActionVerb::kUncheck:
      return l10n_util::GetStringUTF16(IDS_AX_UNCHECK_ACTION_VERB);
  }
  NOTREACHED();
  return base::string16();
}

// Some APIs on Linux and Windows need to return non-localized action names.
base::string16 ActionVerbToUnlocalizedString(
    const ax::mojom::DefaultActionVerb action_verb) {
  switch (action_verb) {
    case ax::mojom::DefaultActionVerb::kNone:
      return base::UTF8ToUTF16("none");
    case ax::mojom::DefaultActionVerb::kActivate:
      return base::UTF8ToUTF16("activate");
    case ax::mojom::DefaultActionVerb::kCheck:
      return base::UTF8ToUTF16("check");
    case ax::mojom::DefaultActionVerb::kClick:
      return base::UTF8ToUTF16("click");
    case ax::mojom::DefaultActionVerb::kClickAncestor:
      return base::UTF8ToUTF16("click-ancestor");
    case ax::mojom::DefaultActionVerb::kJump:
      return base::UTF8ToUTF16("jump");
    case ax::mojom::DefaultActionVerb::kOpen:
      return base::UTF8ToUTF16("open");
    case ax::mojom::DefaultActionVerb::kPress:
      return base::UTF8ToUTF16("press");
    case ax::mojom::DefaultActionVerb::kSelect:
      return base::UTF8ToUTF16("select");
    case ax::mojom::DefaultActionVerb::kUncheck:
      return base::UTF8ToUTF16("uncheck");
  }
  NOTREACHED();
  return base::string16();
}

}  // namespace ui
