// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/group_coordinator.h"

#include <algorithm>

#include "base/stl_util.h"
#include "services/audio/group_member.h"

namespace audio {

GroupCoordinator::GroupCoordinator() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

GroupCoordinator::~GroupCoordinator() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(groups_.empty());
}

void GroupCoordinator::RegisterGroupMember(GroupMember* member) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(member);

  const auto it = FindGroup(member->GetGroupId());
  std::vector<GroupMember*>& members = it->second.members;
  DCHECK(!base::ContainsValue(members, member));
  members.push_back(member);

  for (Observer* observer : it->second.observers) {
    observer->OnMemberJoinedGroup(member);
  }
}

void GroupCoordinator::UnregisterGroupMember(GroupMember* member) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(member);

  const auto group_it = FindGroup(member->GetGroupId());
  std::vector<GroupMember*>& members = group_it->second.members;
  const auto member_it = std::find(members.begin(), members.end(), member);
  DCHECK(member_it != members.end());
  members.erase(member_it);

  for (Observer* observer : group_it->second.observers) {
    observer->OnMemberLeftGroup(member);
  }

  MaybePruneGroupMapEntry(group_it);
}

void GroupCoordinator::AddObserver(const base::UnguessableToken& group_id,
                                   Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(observer);

  std::vector<Observer*>& observers = FindGroup(group_id)->second.observers;
  DCHECK(!base::ContainsValue(observers, observer));
  observers.push_back(observer);
}

void GroupCoordinator::RemoveObserver(const base::UnguessableToken& group_id,
                                      Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(observer);

  const auto group_it = FindGroup(group_id);
  std::vector<Observer*>& observers = group_it->second.observers;
  const auto it = std::find(observers.begin(), observers.end(), observer);
  DCHECK(it != observers.end());
  observers.erase(it);

  MaybePruneGroupMapEntry(group_it);
}

const std::vector<GroupMember*>& GroupCoordinator::GetCurrentMembers(
    const base::UnguessableToken& group_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for (const auto& entry : groups_) {
    if (entry.first == group_id) {
      return entry.second.members;
    }
  }

  static const std::vector<GroupMember*> empty_set;
  return empty_set;
}

GroupCoordinator::GroupMap::iterator GroupCoordinator::FindGroup(
    const base::UnguessableToken& group_id) {
  for (auto it = groups_.begin(); it != groups_.end(); ++it) {
    if (it->first == group_id) {
      return it;
    }
  }

  // Group does not exist. Create a new entry.
  groups_.emplace_back();
  const auto new_it = groups_.end() - 1;
  new_it->first = group_id;
  return new_it;
}

void GroupCoordinator::MaybePruneGroupMapEntry(GroupMap::iterator it) {
  if (it->second.members.empty() && it->second.observers.empty()) {
    groups_.erase(it);
  }
}

GroupCoordinator::Observer::~Observer() = default;

GroupCoordinator::Group::Group() = default;
GroupCoordinator::Group::~Group() = default;
GroupCoordinator::Group::Group(GroupCoordinator::Group&& other) = default;
GroupCoordinator::Group& GroupCoordinator::Group::operator=(
    GroupCoordinator::Group&& other) = default;

}  // namespace audio
