/*
    Copyright (C) 2010 Rob Buis <rwlbuis@gmail.com>
    Copyright (C) 2011 Cosmin Truta <ctruta@gmail.com>
    Copyright (C) 2012 University of Szeged
    Copyright (C) 2012 Renata Hodovan <reni@webkit.org>

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
*/

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_RESOURCE_DOCUMENT_RESOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_RESOURCE_DOCUMENT_RESOURCE_H_

#include <memory>
#include "third_party/blink/renderer/core/loader/resource/text_resource.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_client.h"
#include "third_party/blink/renderer/platform/loader/fetch/text_resource_decoder_options.h"

namespace blink {

class Document;
class FetchParameters;
class ResourceFetcher;

class CORE_EXPORT DocumentResource final : public TextResource {
 public:
  static DocumentResource* FetchSVGDocument(FetchParameters&,
                                            ResourceFetcher*,
                                            ResourceClient*);
  ~DocumentResource() override;
  void Trace(blink::Visitor*) override;

  Document* GetDocument() const { return document_.Get(); }

  void NotifyFinished() override;

 private:
  class SVGDocumentResourceFactory : public ResourceFactory {
   public:
    SVGDocumentResourceFactory()
        : ResourceFactory(Resource::kSVGDocument,
                          TextResourceDecoderOptions::kXMLContent) {}

    Resource* Create(
        const ResourceRequest& request,
        const ResourceLoaderOptions& options,
        const TextResourceDecoderOptions& decoder_options) const override {
      return new DocumentResource(request, Resource::kSVGDocument, options,
                                  decoder_options);
    }
  };
  DocumentResource(const ResourceRequest&,
                   Type,
                   const ResourceLoaderOptions&,
                   const TextResourceDecoderOptions&);

  bool MimeTypeAllowed() const;
  Document* CreateDocument(const KURL&);

  Member<Document> document_;
};

DEFINE_TYPE_CASTS(DocumentResource,
                  Resource,
                  resource,
                  resource->GetType() == Resource::kSVGDocument,
                  resource.GetType() == Resource::kSVGDocument);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_RESOURCE_DOCUMENT_RESOURCE_H_
