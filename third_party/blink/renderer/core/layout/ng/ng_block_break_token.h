// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBlockBreakToken_h
#define NGBlockBreakToken_h

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/ng_break_token.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Represents a break token for a block node.
class CORE_EXPORT NGBlockBreakToken : public NGBreakToken {
 public:
  // Creates a break token for a node which did fragment, and can potentially
  // produce more fragments.
  //
  // The NGBlockBreakToken takes ownership of child_break_tokens, leaving it
  // empty for the caller.
  //
  // The node is NGBlockNode, or any other NGLayoutInputNode that produces
  // anonymous box.
  static scoped_refptr<NGBlockBreakToken> Create(
      NGLayoutInputNode node,
      LayoutUnit used_block_size,
      Vector<scoped_refptr<NGBreakToken>>& child_break_tokens,
      bool has_last_resort_break = false) {
    return base::AdoptRef(new NGBlockBreakToken(
        node, used_block_size, child_break_tokens, has_last_resort_break));
  }

  // Creates a break token for a node which cannot produce any more fragments.
  static scoped_refptr<NGBlockBreakToken> Create(
      NGLayoutInputNode node,
      LayoutUnit used_block_size,
      bool has_last_resort_break = false) {
    return base::AdoptRef(
        new NGBlockBreakToken(node, used_block_size, has_last_resort_break));
  }

  // Creates a break token for a node that needs to produce its first fragment
  // in the next fragmentainer. In this case we create a break token for a node
  // that hasn't yet produced any fragments.
  static scoped_refptr<NGBlockBreakToken> CreateBreakBefore(
      NGLayoutInputNode node) {
    auto* token = new NGBlockBreakToken(node);
    token->is_break_before_ = true;
    return base::AdoptRef(token);
  }

  // Represents the amount of block size used in previous fragments.
  //
  // E.g. if the layout block specifies a block size of 200px, and the previous
  // fragments of this block used 150px (used block size), the next fragment
  // should have a size of 50px (assuming no additional fragmentation).
  LayoutUnit UsedBlockSize() const { return used_block_size_; }

  // Return true if this is a break token that was produced without any
  // "preceding" fragment. This happens when we determine that the first
  // fragment for a node needs to be created in a later fragmentainer than the
  // one it was it was first encountered, due to block space shortage.
  bool IsBreakBefore() const { return is_break_before_; }

  bool HasLastResortBreak() const { return has_last_resort_break_; }

  // The break tokens for children of the layout node.
  //
  // Each child we have visited previously in the block-flow layout algorithm
  // has an associated break token. This may be either finished (we should skip
  // this child) or unfinished (we should try and produce the next fragment for
  // this child).
  //
  // A child which we haven't visited yet doesn't have a break token here.
  const Vector<scoped_refptr<NGBreakToken>>& ChildBreakTokens() const {
    return child_break_tokens_;
  }

#ifndef NDEBUG
  String ToString() const override;
#endif

 private:
  NGBlockBreakToken(NGLayoutInputNode node,
                    LayoutUnit used_block_size,
                    Vector<scoped_refptr<NGBreakToken>>& child_break_tokens,
                    bool has_last_resort_break);

  NGBlockBreakToken(NGLayoutInputNode node,
                    LayoutUnit used_block_size,
                    bool has_last_resort_break);

  explicit NGBlockBreakToken(NGLayoutInputNode node);

  Vector<scoped_refptr<NGBreakToken>> child_break_tokens_;
  LayoutUnit used_block_size_;

  bool is_break_before_ = false;

  // We're attempting to break at an undesirable place. Sometimes that's
  // unavoidable, but we should only break here if we cannot find a better break
  // point further up in the ancestry.
  bool has_last_resort_break_ = false;
};

DEFINE_TYPE_CASTS(NGBlockBreakToken,
                  NGBreakToken,
                  token,
                  token->IsBlockType(),
                  token.IsBlockType());

}  // namespace blink

#endif  // NGBlockBreakToken_h
