// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_URL_MATCHER_SUBSTRING_SET_MATCHER_H_
#define COMPONENTS_URL_MATCHER_SUBSTRING_SET_MATCHER_H_

#include <stdint.h>

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/url_matcher/string_pattern.h"
#include "components/url_matcher/url_matcher_export.h"

namespace url_matcher {

// Class that store a set of string patterns and can find for a string S,
// which string patterns occur in S.
class URL_MATCHER_EXPORT SubstringSetMatcher {
 public:
  SubstringSetMatcher();
  ~SubstringSetMatcher();

  // Registers all |patterns|. The ownership remains with the caller.
  // The same pattern cannot be registered twice and each pattern needs to have
  // a unique ID.
  // Ownership of the patterns remains with the caller.
  void RegisterPatterns(const std::vector<const StringPattern*>& patterns);

  // Unregisters the passed |patterns|.
  void UnregisterPatterns(const std::vector<const StringPattern*>& patterns);

  // Analogous to RegisterPatterns and UnregisterPatterns but executes both
  // operations in one step, which is cheaper in the execution.
  void RegisterAndUnregisterPatterns(
      const std::vector<const StringPattern*>& to_register,
      const std::vector<const StringPattern*>& to_unregister);

  // Matches |text| against all registered StringPatterns. Stores the IDs
  // of matching patterns in |matches|. |matches| is not cleared before adding
  // to it.
  bool Match(const std::string& text,
             std::set<StringPattern::ID>* matches) const;

  // Returns true if this object retains no allocated data. Only for debugging.
  bool IsEmpty() const;

 private:
  // A node of an Aho Corasick Tree. This is implemented according to
  // http://www.cs.uku.fi/~kilpelai/BSA05/lectures/slides04.pdf
  //
  // The algorithm is based on the idea of building a trie of all registered
  // patterns. Each node of the tree is annotated with a set of pattern
  // IDs that are used to report matches.
  //
  // The root of the trie represents an empty match. If we were looking whether
  // any registered pattern matches a text at the beginning of the text (i.e.
  // whether any pattern is a prefix of the text), we could just follow
  // nodes in the trie according to the matching characters in the text.
  // E.g., if text == "foobar", we would follow the trie from the root node
  // to its child labeled 'f', from there to child 'o', etc. In this process we
  // would report all pattern IDs associated with the trie nodes as matches.
  //
  // As we are not looking for all prefix matches but all substring matches,
  // this algorithm would need to compare text.substr(0), text.substr(1), ...
  // against the trie, which is in O(|text|^2).
  //
  // The Aho Corasick algorithm improves this runtime by using failure edges.
  // In case we have found a partial match of length k in the text
  // (text[i, ..., i + k - 1]) in the trie starting at the root and ending at
  // a node at depth k, but cannot find a match in the trie for character
  // text[i + k] at depth k + 1, we follow a failure edge. This edge
  // corresponds to the longest proper suffix of text[i, ..., i + k - 1] that
  // is a prefix of any registered pattern.
  //
  // If your brain thinks "Forget it, let's go shopping.", don't worry.
  // Take a nap and read an introductory text on the Aho Corasick algorithm.
  // It will make sense. Eventually.
  class AhoCorasickNode {
   public:
    // Key: label of the edge, value: node index in |tree_| of parent class.
    typedef std::map<char, uint32_t> Edges;
    typedef std::set<StringPattern::ID> Matches;

    static const uint32_t kNoSuchEdge;  // Represents an invalid node index.

    AhoCorasickNode();
    ~AhoCorasickNode();
    AhoCorasickNode(const AhoCorasickNode& other);
    AhoCorasickNode& operator=(const AhoCorasickNode& other);

    uint32_t GetEdge(char c) const;
    void SetEdge(char c, uint32_t node);
    const Edges& edges() const { return edges_; }

    uint32_t failure() const { return failure_; }
    void set_failure(uint32_t failure) { failure_ = failure; }

    void AddMatch(StringPattern::ID id);
    void AddMatches(const Matches& matches);
    const Matches& matches() const { return matches_; }

   private:
    // Outgoing edges of current node.
    Edges edges_;

    // Node index that failure edge leads to.
    uint32_t failure_;

    // Identifiers of matches.
    Matches matches_;
  };

  typedef std::map<StringPattern::ID, const StringPattern*> SubstringPatternMap;
  typedef std::vector<const StringPattern*> SubstringPatternVector;

  // |sorted_patterns| is a copy of |patterns_| sorted by the pattern string.
  void RebuildAhoCorasickTree(const SubstringPatternVector& sorted_patterns);

  // Inserts a path for |pattern->pattern()| into the tree and adds
  // |pattern->id()| to the set of matches. Ownership of |pattern| remains with
  // the caller.
  void InsertPatternIntoAhoCorasickTree(const StringPattern* pattern);
  void CreateFailureEdges();

  // Set of all registered StringPatterns. Used to regenerate the
  // Aho-Corasick tree in case patterns are registered or unregistered.
  SubstringPatternMap patterns_;

  // The nodes of a Aho-Corasick tree.
  std::vector<AhoCorasickNode> tree_;

  DISALLOW_COPY_AND_ASSIGN(SubstringSetMatcher);
};

}  // namespace url_matcher

#endif  // COMPONENTS_URL_MATCHER_SUBSTRING_SET_MATCHER_H_
