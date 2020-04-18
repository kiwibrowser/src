// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_PUBLIC_OBJECT_ID_INVALIDATION_MAP_H_
#define COMPONENTS_INVALIDATION_PUBLIC_OBJECT_ID_INVALIDATION_MAP_H_

#include <map>
#include <memory>
#include <vector>

#include "components/invalidation/public/invalidation.h"
#include "components/invalidation/public/invalidation_export.h"
#include "components/invalidation/public/invalidation_util.h"
#include "components/invalidation/public/single_object_invalidation_set.h"

namespace syncer {

// A set of notifications with some helper methods to organize them by object ID
// and version number.
class INVALIDATION_EXPORT ObjectIdInvalidationMap {
  public:
   // Creates an invalidation map that includes an 'unknown version'
   // invalidation for each specified ID in |ids|.
   static ObjectIdInvalidationMap InvalidateAll(const ObjectIdSet& ids);

   ObjectIdInvalidationMap();
   ObjectIdInvalidationMap(const ObjectIdInvalidationMap& other);
   ~ObjectIdInvalidationMap();

   // Returns set of ObjectIds for which at least one invalidation is present.
   ObjectIdSet GetObjectIds() const;

   // Returns true if this map contains no invalidations.
   bool Empty() const;

   // Returns true if both maps contain the same set of invalidations.
   bool operator==(const ObjectIdInvalidationMap& other) const;

   // Inserts a new invalidation into this map.
   void Insert(const Invalidation& invalidation);

   // Returns a new map containing the subset of invaliations from this map
   // whose IDs were in the specified |ids| set.
   ObjectIdInvalidationMap GetSubsetWithObjectIds(const ObjectIdSet& ids) const;

   // Returns the subset of invalidations with IDs matching |id|.
   const SingleObjectInvalidationSet& ForObject(
       invalidation::ObjectId id) const;

   // Returns the contents of this map in a single vector.
   void GetAllInvalidations(std::vector<syncer::Invalidation>* out) const;

   // Call Acknowledge() on all contained Invalidations.
   void AcknowledgeAll() const;

   // Serialize this map to a value.
   std::unique_ptr<base::ListValue> ToValue() const;

   // Deserialize the value into a map and use it to re-initialize this object.
   bool ResetFromValue(const base::ListValue& value);

   // Prints the contentes of this map as a human-readable string.
   std::string ToString() const;

  private:
   typedef std::map<invalidation::ObjectId,
                    SingleObjectInvalidationSet,
                    ObjectIdLessThan> IdToListMap;

   ObjectIdInvalidationMap(const IdToListMap& map);

   IdToListMap map_;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_PUBLIC_OBJECT_ID_INVALIDATION_MAP_H_
