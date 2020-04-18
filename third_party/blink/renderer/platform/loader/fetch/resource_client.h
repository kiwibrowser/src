/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All
    rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style
    sheets and html pages from the web. It has a memory cache for these objects.
*/

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_CLIENT_H_

#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class PLATFORM_EXPORT ResourceClient : public GarbageCollectedMixin {
  USING_PRE_FINALIZER(ResourceClient, ClearResource);

 public:
  enum ResourceClientType {
    kBaseResourceType,
    kFontType,
    kRawResourceType
  };

  virtual ~ResourceClient() = default;

  // DataReceived() is called each time a chunk of data is received.
  // For cache hits, the data is replayed before NotifyFinished() is called.
  // For successful revalidation responses, the data is NOT replayed, because
  // the Resource may not be in an entirely consistent state in the middle of
  // completing the revalidation, when DataReceived() would have to be called.
  // Some RawResourceClients depends on receiving all bytes via DataReceived(),
  // but RawResources forbid revalidation attempts, so they still are guaranteed
  // to get all data via DataReceived().
  virtual void DataReceived(Resource*,
                            const char* /* data */,
                            size_t /* length */) {}
  virtual void NotifyFinished(Resource*) {}

  static bool IsExpectedType(ResourceClient*) { return true; }
  virtual ResourceClientType GetResourceClientType() const {
    return kBaseResourceType;
  }

  Resource* GetResource() const { return resource_; }

  // Name for debugging, e.g. shown in memory-infra.
  virtual String DebugName() const = 0;

  void Trace(blink::Visitor* visitor) override { visitor->Trace(resource_); }

 protected:
  ResourceClient() = default;

  void ClearResource() { SetResource(nullptr, nullptr); }

 private:
  // ResourceFetcher is primarily responsible for calling SetResource() with a
  // non-null Resource*. ResourceClient subclasses are responsible for calling
  // ClearResource().
  friend class ResourceFetcher;
  // TODO(japhet): There isn't a clean way for SVGResourceClients to determine
  // whether SVGElementProxy is holding a Resource that it should register with,
  // so SVGElementProxy handles it for those clients. SVGResourceClients should
  // have a better way to register themselves as clients. crbug.com/789198
  friend class SVGElementProxy;
  // CSSFontFaceSrcValue only ever requests a Resource once, and acts as an
  // intermediate caching layer of sorts. It needs to be able to register
  // additional clients.
  friend class CSSFontFaceSrcValue;

  void SetResource(Resource* new_resource,
                   base::SingleThreadTaskRunner* task_runner) {
    if (new_resource == resource_)
      return;

    // Some ResourceClient implementations reenter this so
    // we need to prevent double removal.
    if (Resource* old_resource = resource_.Release())
      old_resource->RemoveClient(this);
    resource_ = new_resource;
    if (resource_)
      resource_->AddClient(this, task_runner);
  }

  Member<Resource> resource_;
};

}  // namespace blink

#endif
