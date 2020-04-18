/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/base/stream_params.h"

#include <stdint.h>
#include <list>

#include "absl/algorithm/container.h"
#include "api/array_view.h"
#include "rtc_base/strings/string_builder.h"

namespace cricket {
namespace {

std::string SsrcsToString(const std::vector<uint32_t>& ssrcs) {
  char buf[1024];
  rtc::SimpleStringBuilder sb(buf);
  sb << "ssrcs:[";
  for (std::vector<uint32_t>::const_iterator it = ssrcs.begin();
       it != ssrcs.end(); ++it) {
    if (it != ssrcs.begin()) {
      sb << ",";
    }
    sb << *it;
  }
  sb << "]";
  return sb.str();
}

std::string RidsToString(const std::vector<RidDescription>& rids) {
  char buf[1024];
  rtc::SimpleStringBuilder sb(buf);
  sb << "rids:[";
  const char* delimiter = "";
  for (const RidDescription& rid : rids) {
    sb << delimiter << rid.rid;
    delimiter = ",";
  }
  sb << "]";
  return sb.str();
}

}  // namespace

const char kFecSsrcGroupSemantics[] = "FEC";
const char kFecFrSsrcGroupSemantics[] = "FEC-FR";
const char kFidSsrcGroupSemantics[] = "FID";
const char kSimSsrcGroupSemantics[] = "SIM";

bool GetStream(const StreamParamsVec& streams,
               const StreamSelector& selector,
               StreamParams* stream_out) {
  const StreamParams* found = GetStream(streams, selector);
  if (found && stream_out)
    *stream_out = *found;
  return found != nullptr;
}

SsrcGroup::SsrcGroup(const std::string& usage,
                     const std::vector<uint32_t>& ssrcs)
    : semantics(usage), ssrcs(ssrcs) {}
SsrcGroup::SsrcGroup(const SsrcGroup&) = default;
SsrcGroup::SsrcGroup(SsrcGroup&&) = default;
SsrcGroup::~SsrcGroup() = default;

SsrcGroup& SsrcGroup::operator=(const SsrcGroup&) = default;
SsrcGroup& SsrcGroup::operator=(SsrcGroup&&) = default;

bool SsrcGroup::has_semantics(const std::string& semantics_in) const {
  return (semantics == semantics_in && ssrcs.size() > 0);
}

std::string SsrcGroup::ToString() const {
  char buf[1024];
  rtc::SimpleStringBuilder sb(buf);
  sb << "{";
  sb << "semantics:" << semantics << ";";
  sb << SsrcsToString(ssrcs);
  sb << "}";
  return sb.str();
}

StreamParams::StreamParams() = default;
StreamParams::StreamParams(const StreamParams&) = default;
StreamParams::StreamParams(StreamParams&&) = default;
StreamParams::~StreamParams() = default;
StreamParams& StreamParams::operator=(const StreamParams&) = default;
StreamParams& StreamParams::operator=(StreamParams&&) = default;

bool StreamParams::operator==(const StreamParams& other) const {
  return (groupid == other.groupid && id == other.id && ssrcs == other.ssrcs &&
          ssrc_groups == other.ssrc_groups && cname == other.cname &&
          stream_ids_ == other.stream_ids_ &&
          // RIDs are not required to be in the same order for equality.
          absl::c_is_permutation(rids_, other.rids_));
}

std::string StreamParams::ToString() const {
  char buf[2 * 1024];
  rtc::SimpleStringBuilder sb(buf);
  sb << "{";
  if (!groupid.empty()) {
    sb << "groupid:" << groupid << ";";
  }
  if (!id.empty()) {
    sb << "id:" << id << ";";
  }
  sb << SsrcsToString(ssrcs) << ";";
  sb << "ssrc_groups:";
  for (std::vector<SsrcGroup>::const_iterator it = ssrc_groups.begin();
       it != ssrc_groups.end(); ++it) {
    if (it != ssrc_groups.begin()) {
      sb << ",";
    }
    sb << it->ToString();
  }
  sb << ";";
  if (!cname.empty()) {
    sb << "cname:" << cname << ";";
  }
  sb << "stream_ids:";
  for (std::vector<std::string>::const_iterator it = stream_ids_.begin();
       it != stream_ids_.end(); ++it) {
    if (it != stream_ids_.begin()) {
      sb << ",";
    }
    sb << *it;
  }
  sb << ";";
  if (!rids_.empty()) {
    sb << RidsToString(rids_) << ";";
  }
  sb << "}";
  return sb.str();
}

void StreamParams::GenerateSsrcs(int num_layers,
                                 bool generate_fid,
                                 bool generate_fec_fr,
                                 rtc::UniqueRandomIdGenerator* ssrc_generator) {
  RTC_DCHECK_GE(num_layers, 0);
  RTC_DCHECK(ssrc_generator);
  std::vector<uint32_t> primary_ssrcs;
  for (int i = 0; i < num_layers; ++i) {
    uint32_t ssrc = ssrc_generator->GenerateId();
    primary_ssrcs.push_back(ssrc);
    add_ssrc(ssrc);
  }

  if (num_layers > 1) {
    SsrcGroup simulcast(kSimSsrcGroupSemantics, primary_ssrcs);
    ssrc_groups.push_back(simulcast);
  }

  if (generate_fid) {
    for (uint32_t ssrc : primary_ssrcs) {
      AddFidSsrc(ssrc, ssrc_generator->GenerateId());
    }
  }

  if (generate_fec_fr) {
    for (uint32_t ssrc : primary_ssrcs) {
      AddFecFrSsrc(ssrc, ssrc_generator->GenerateId());
    }
  }
}

void StreamParams::GetPrimarySsrcs(std::vector<uint32_t>* ssrcs) const {
  const SsrcGroup* sim_group = get_ssrc_group(kSimSsrcGroupSemantics);
  if (sim_group == NULL) {
    ssrcs->push_back(first_ssrc());
  } else {
    for (size_t i = 0; i < sim_group->ssrcs.size(); ++i) {
      ssrcs->push_back(sim_group->ssrcs[i]);
    }
  }
}

void StreamParams::GetFidSsrcs(const std::vector<uint32_t>& primary_ssrcs,
                               std::vector<uint32_t>* fid_ssrcs) const {
  for (size_t i = 0; i < primary_ssrcs.size(); ++i) {
    uint32_t fid_ssrc;
    if (GetFidSsrc(primary_ssrcs[i], &fid_ssrc)) {
      fid_ssrcs->push_back(fid_ssrc);
    }
  }
}

bool StreamParams::AddSecondarySsrc(const std::string& semantics,
                                    uint32_t primary_ssrc,
                                    uint32_t secondary_ssrc) {
  if (!has_ssrc(primary_ssrc)) {
    return false;
  }

  ssrcs.push_back(secondary_ssrc);
  std::vector<uint32_t> ssrc_vector;
  ssrc_vector.push_back(primary_ssrc);
  ssrc_vector.push_back(secondary_ssrc);
  SsrcGroup ssrc_group = SsrcGroup(semantics, ssrc_vector);
  ssrc_groups.push_back(ssrc_group);
  return true;
}

bool StreamParams::GetSecondarySsrc(const std::string& semantics,
                                    uint32_t primary_ssrc,
                                    uint32_t* secondary_ssrc) const {
  for (std::vector<SsrcGroup>::const_iterator it = ssrc_groups.begin();
       it != ssrc_groups.end(); ++it) {
    if (it->has_semantics(semantics) && it->ssrcs.size() >= 2 &&
        it->ssrcs[0] == primary_ssrc) {
      *secondary_ssrc = it->ssrcs[1];
      return true;
    }
  }
  return false;
}

std::vector<std::string> StreamParams::stream_ids() const {
  return stream_ids_;
}

void StreamParams::set_stream_ids(const std::vector<std::string>& stream_ids) {
  stream_ids_ = stream_ids;
}

std::string StreamParams::first_stream_id() const {
  return stream_ids_.empty() ? "" : stream_ids_[0];
}

}  // namespace cricket
