// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/discovery/mdns/domain_name.h"

#include <algorithm>
#include <iterator>

#include "osp_base/stringprintf.h"

namespace openscreen {
namespace mdns {

// static
DomainName DomainName::GetLocalDomain() {
  return DomainName{{5, 'l', 'o', 'c', 'a', 'l', 0}};
}

// static
ErrorOr<DomainName> DomainName::Append(const DomainName& first,
                                       const DomainName& second) {
  OSP_CHECK(first.domain_name_.size());
  OSP_CHECK(second.domain_name_.size());

  // Both vectors should represent null terminated domain names.
  OSP_DCHECK_EQ(first.domain_name_.back(), '\0');
  OSP_DCHECK_EQ(second.domain_name_.back(), '\0');
  if ((first.domain_name_.size() + second.domain_name_.size() - 1) >
      kDomainNameMaxLength) {
    return Error::Code::kDomainNameTooLong;
  }

  DomainName result;
  result.domain_name_.clear();
  result.domain_name_.insert(result.domain_name_.begin(),
                             first.domain_name_.begin(),
                             first.domain_name_.end());
  result.domain_name_.insert(result.domain_name_.end() - 1,
                             second.domain_name_.begin(),
                             second.domain_name_.end() - 1);
  return result;
}

DomainName::DomainName() : domain_name_{'\0'} {}
DomainName::DomainName(std::vector<uint8_t>&& domain_name)
    : domain_name_(std::move(domain_name)) {
  OSP_CHECK_LE(domain_name_.size(), kDomainNameMaxLength);
}
DomainName::DomainName(const DomainName&) = default;
DomainName::DomainName(DomainName&&) = default;
DomainName::~DomainName() = default;
DomainName& DomainName::operator=(const DomainName& domain_name) = default;
DomainName& DomainName::operator=(DomainName&& domain_name) = default;

bool DomainName::operator==(const DomainName& other) const {
  // TODO: case-insensitive comparison
  return domain_name_ == other.domain_name_;
}

bool DomainName::operator!=(const DomainName& other) const {
  return !(*this == other);
}

bool DomainName::EndsWithLocalDomain() const {
  const DomainName local_domain = GetLocalDomain();
  if (domain_name_.size() < local_domain.domain_name_.size())
    return false;

  return std::equal(local_domain.domain_name_.begin(),
                    local_domain.domain_name_.end(),
                    domain_name_.end() - local_domain.domain_name_.size());
}

Error DomainName::Append(const DomainName& after) {
  OSP_CHECK(after.domain_name_.size());
  OSP_DCHECK_EQ(after.domain_name_.back(), '\0');

  if ((domain_name_.size() + after.domain_name_.size() - 1) >
      kDomainNameMaxLength) {
    return Error::Code::kDomainNameTooLong;
  }

  domain_name_.insert(domain_name_.end() - 1, after.domain_name_.begin(),
                      after.domain_name_.end() - 1);
  return Error::None();
}

std::vector<std::string> DomainName::GetLabels() const {
  OSP_DCHECK_GT(domain_name_.size(), 0u);
  std::vector<std::string> result;
  auto it = domain_name_.begin();
  while (*it != 0) {
    OSP_DCHECK_LT(it - domain_name_.begin(), kDomainNameMaxLength);
    OSP_DCHECK_LT((it + 1 + *it) - domain_name_.begin(), kDomainNameMaxLength);
    result.emplace_back(it + 1, it + 1 + *it);
    it += 1 + *it;
  }
  return result;
}

bool DomainNameComparator::operator()(const DomainName& a,
                                      const DomainName& b) const {
  return a.domain_name() < b.domain_name();
}

std::ostream& operator<<(std::ostream& os, const DomainName& domain_name) {
  const auto& data = domain_name.domain_name();
  OSP_DCHECK_GT(data.size(), 0u);
  auto it = data.begin();
  while (*it != 0) {
    size_t length = *it++;
    PrettyPrintAsciiHex(os, it, it + length);
    it += length;
    os << ".";
  }
  return os;
}

}  // namespace mdns
}  // namespace openscreen
