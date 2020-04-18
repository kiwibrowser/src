// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_HIT_REGION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_HIT_REGION_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/canvas/canvas2d/hit_region_options.h"
#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class HitRegion final : public GarbageCollectedFinalized<HitRegion> {
 public:
  static HitRegion* Create(const Path& path, const HitRegionOptions& options) {
    return new HitRegion(path, options);
  }

  virtual ~HitRegion() = default;

  void RemovePixels(const Path&);

  bool Contains(const FloatPoint&) const;

  const String& Id() const { return id_; }
  const Path& GetPath() const { return path_; }
  Element* Control() const { return control_.Get(); }

  void Trace(blink::Visitor*);

 private:
  HitRegion(const Path&, const HitRegionOptions&);

  String id_;
  Member<Element> control_;
  Path path_;
  WindRule fill_rule_;
};

class HitRegionManager final : public GarbageCollected<HitRegionManager> {
  WTF_MAKE_NONCOPYABLE(HitRegionManager);

 public:
  static HitRegionManager* Create() { return new HitRegionManager; }

  void AddHitRegion(HitRegion*);

  void RemoveHitRegion(HitRegion*);
  void RemoveHitRegionById(const String& id);
  void RemoveHitRegionByControl(const Element*);
  void RemoveHitRegionsInRect(const FloatRect&, const AffineTransform&);
  void RemoveAllHitRegions();

  HitRegion* GetHitRegionById(const String& id) const;
  HitRegion* GetHitRegionByControl(const Element*) const;
  HitRegion* GetHitRegionAtPoint(const FloatPoint&) const;

  unsigned GetHitRegionsCount() const;

  void Trace(blink::Visitor*);

 private:
  HitRegionManager() = default;

  typedef HeapListHashSet<Member<HitRegion>> HitRegionList;
  typedef HitRegionList::const_reverse_iterator HitRegionIterator;
  typedef HeapHashMap<String, Member<HitRegion>> HitRegionIdMap;
  typedef HeapHashMap<Member<const Element>, Member<HitRegion>>
      HitRegionControlMap;

  HitRegionList hit_region_list_;
  HitRegionIdMap hit_region_id_map_;
  HitRegionControlMap hit_region_control_map_;
};

}  // namespace blink

#endif
