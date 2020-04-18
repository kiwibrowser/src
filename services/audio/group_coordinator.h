// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_GROUP_COORDINATOR_H_
#define SERVICES_AUDIO_GROUP_COORDINATOR_H_

#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/unguessable_token.h"

namespace audio {

class GroupMember;

// Manages a registry of group members and notifies observers as membership in
// the group changes.
class GroupCoordinator {
 public:
  // Interface for entities that wish to montior and take action as members
  // join/leave a particular group.
  class Observer {
   public:
    virtual void OnMemberJoinedGroup(GroupMember* member) = 0;
    virtual void OnMemberLeftGroup(GroupMember* member) = 0;

   protected:
    virtual ~Observer();
  };

  GroupCoordinator();
  ~GroupCoordinator();

  // Registers/Unregisters a group |member|. The member must remain valid until
  // after UnregisterGroupMember() is called.
  void RegisterGroupMember(GroupMember* member);
  void UnregisterGroupMember(GroupMember* member);

  void AddObserver(const base::UnguessableToken& group_id, Observer* observer);
  void RemoveObserver(const base::UnguessableToken& group_id,
                      Observer* observer);

  // Returns the current members in the group having the given |group_id|. Note
  // that the validity of the returned reference is uncertain once any of the
  // other non-const methods are called.
  const std::vector<GroupMember*>& GetCurrentMembers(
      const base::UnguessableToken& group_id) const;

 private:
  struct Group {
    std::vector<GroupMember*> members;
    std::vector<Observer*> observers;

    Group();
    ~Group();
    Group(Group&& other);
    Group& operator=(Group&& other);

   private:
    DISALLOW_COPY_AND_ASSIGN(Group);
  };

  using GroupMap = std::vector<std::pair<base::UnguessableToken, Group>>;

  // Returns an iterator to the entry associated with the given |group_id|,
  // creating a new one if necessary.
  GroupMap::iterator FindGroup(const base::UnguessableToken& group_id);

  // Deletes the entry in |groups_| if it has no members or observers remaining.
  void MaybePruneGroupMapEntry(GroupMap::iterator it);

  GroupMap groups_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(GroupCoordinator);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_GROUP_COORDINATOR_H_
