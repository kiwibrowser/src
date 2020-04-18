// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_CSP_SOURCE_LIST_DIRECTIVE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_CSP_SOURCE_LIST_DIRECTIVE_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/web_content_security_policy.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/csp/csp_directive.h"
#include "third_party/blink/renderer/core/frame/csp/csp_source.h"
#include "third_party/blink/renderer/platform/crypto.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_parsers.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ContentSecurityPolicy;
class KURL;

class CORE_EXPORT SourceListDirective final : public CSPDirective {
 public:
  SourceListDirective(const String& name,
                      const String& value,
                      ContentSecurityPolicy*);
  void Trace(blink::Visitor*) override;

  void Parse(const UChar* begin, const UChar* end);

  bool Matches(const KURL&,
               ResourceRequest::RedirectStatus =
                   ResourceRequest::RedirectStatus::kNoRedirect) const;

  bool Allows(const KURL&,
              ResourceRequest::RedirectStatus =
                  ResourceRequest::RedirectStatus::kNoRedirect) const;
  bool AllowInline() const;
  bool AllowEval() const;
  bool AllowWasmEval() const;
  bool AllowDynamic() const;
  bool AllowNonce(const String& nonce) const;
  bool AllowHash(const CSPHashValue&) const;
  bool AllowHashedAttributes() const;
  bool AllowReportSample() const;
  bool IsNone() const;
  bool IsHashOrNoncePresent() const;
  uint8_t HashAlgorithmsUsed() const;
  bool AllowAllInline() const;

  // The algorothm is described more extensively here:
  // https://w3c.github.io/webappsec-csp/embedded/#subsume-source-list
  bool Subsumes(const HeapVector<Member<SourceListDirective>>&) const;

  // Export a subset of the source list that affect navigation.
  // It contains every source-expressions, '*', 'none' and 'self'.
  // It doesn't contain 'unsafe-inline' or 'unsafe-eval' for instance.
  WebContentSecurityPolicySourceList ExposeForNavigationalChecks() const;
  String DirectiveName() const { return directive_name_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(SourceListDirectiveTest, GetIntersectCSPSources);
  FRIEND_TEST_ALL_PREFIXES(SourceListDirectiveTest,
                           GetIntersectCSPSourcesSchemes);
  FRIEND_TEST_ALL_PREFIXES(SourceListDirectiveTest, GetIntersectNonces);
  FRIEND_TEST_ALL_PREFIXES(SourceListDirectiveTest, GetIntersectHashes);
  FRIEND_TEST_ALL_PREFIXES(SourceListDirectiveTest, GetSources);
  FRIEND_TEST_ALL_PREFIXES(SourceListDirectiveTest, ParseHost);
  FRIEND_TEST_ALL_PREFIXES(CSPDirectiveListTest, GetSourceVector);
  FRIEND_TEST_ALL_PREFIXES(CSPDirectiveListTest, OperativeDirectiveGivenType);

  bool ParseSource(const UChar* begin,
                   const UChar* end,
                   String& scheme,
                   String& host,
                   int& port,
                   String& path,
                   CSPSource::WildcardDisposition&,
                   CSPSource::WildcardDisposition&);
  bool ParseScheme(const UChar* begin, const UChar* end, String& scheme);
  static bool ParseHost(const UChar* begin,
                        const UChar* end,
                        String& host,
                        CSPSource::WildcardDisposition&);
  bool ParsePort(const UChar* begin,
                 const UChar* end,
                 int& port,
                 CSPSource::WildcardDisposition&);
  bool ParsePath(const UChar* begin, const UChar* end, String& path);
  bool ParseNonce(const UChar* begin, const UChar* end, String& nonce);
  bool ParseHash(const UChar* begin,
                 const UChar* end,
                 DigestValue& hash,
                 ContentSecurityPolicyHashAlgorithm&);

  void AddSourceSelf();
  void AddSourceStar();
  void AddSourceUnsafeAllowRedirects();
  void AddSourceUnsafeInline();
  void AddSourceUnsafeEval();
  void AddSourceWasmEval();
  void AddSourceStrictDynamic();
  void AddSourceUnsafeHashedAttributes();
  void AddReportSample();
  void AddSourceNonce(const String& nonce);
  void AddSourceHash(const ContentSecurityPolicyHashAlgorithm&,
                     const DigestValue& hash);

  static void AddSourceToMap(HeapHashMap<String, Member<CSPSource>>&,
                             CSPSource*);

  bool HasSourceMatchInList(const KURL&, ResourceRequest::RedirectStatus) const;
  HashSet<String> GetIntersectNonces(const HashSet<String>& other) const;
  HashSet<CSPHashValue> GetIntersectHashes(
      const HashSet<CSPHashValue>& other) const;
  HeapVector<Member<CSPSource>> GetIntersectCSPSources(
      const HeapVector<Member<CSPSource>>& other) const;
  HeapHashMap<String, Member<CSPSource>> GetIntersectSchemesOnly(
      const HeapVector<Member<CSPSource>>& other) const;
  bool SubsumesNoncesAndHashes(const HashSet<String>& nonces,
                               const HashSet<CSPHashValue> hashes) const;
  HeapVector<Member<CSPSource>> GetSources(Member<CSPSource>) const;

  Member<ContentSecurityPolicy> policy_;
  HeapVector<Member<CSPSource>> list_;
  String directive_name_;
  bool allow_self_;
  bool allow_star_;
  bool allow_inline_;
  bool allow_eval_;
  bool allow_wasm_eval_;
  bool allow_dynamic_;
  bool allow_hashed_attributes_;
  bool allow_redirects_;
  bool report_sample_;
  HashSet<String> nonces_;
  HashSet<CSPHashValue> hashes_;
  uint8_t hash_algorithms_used_;

  DISALLOW_COPY_AND_ASSIGN(SourceListDirective);
};

}  // namespace blink

#endif
