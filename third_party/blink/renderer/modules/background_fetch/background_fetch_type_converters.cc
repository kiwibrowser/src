// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_type_converters.h"

#include <utility>
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_registration.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"

namespace mojo {

blink::BackgroundFetchRegistration*
TypeConverter<blink::BackgroundFetchRegistration*,
              blink::mojom::blink::BackgroundFetchRegistrationPtr>::
    Convert(const blink::mojom::blink::BackgroundFetchRegistrationPtr&
                mojoRegistration) {
  if (!mojoRegistration)
    return nullptr;

  return new blink::BackgroundFetchRegistration(
      mojoRegistration->developer_id, mojoRegistration->unique_id,
      mojoRegistration->upload_total, mojoRegistration->uploaded,
      mojoRegistration->download_total, mojoRegistration->downloaded);
}

blink::mojom::blink::BackgroundFetchOptionsPtr TypeConverter<
    blink::mojom::blink::BackgroundFetchOptionsPtr,
    blink::BackgroundFetchOptions>::Convert(const blink::BackgroundFetchOptions&
                                                options) {
  blink::mojom::blink::BackgroundFetchOptionsPtr mojoOptions =
      blink::mojom::blink::BackgroundFetchOptions::New();

  WTF::Vector<blink::mojom::blink::IconDefinitionPtr> mojoIcons;
  mojoIcons.ReserveInitialCapacity(options.icons().size());

  for (const auto& icon : options.icons())
    mojoIcons.push_back(blink::mojom::blink::IconDefinition::From(icon));

  mojoOptions->icons = std::move(mojoIcons);
  mojoOptions->download_total = options.downloadTotal();
  mojoOptions->title = options.title();

  return mojoOptions;
}

blink::IconDefinition
TypeConverter<blink::IconDefinition, blink::mojom::blink::IconDefinitionPtr>::
    Convert(const blink::mojom::blink::IconDefinitionPtr& mojoDefinition) {
  blink::IconDefinition definition;
  definition.setSrc(mojoDefinition->src);
  definition.setSizes(mojoDefinition->sizes);
  definition.setType(mojoDefinition->type);

  return definition;
}

blink::mojom::blink::IconDefinitionPtr TypeConverter<
    blink::mojom::blink::IconDefinitionPtr,
    blink::IconDefinition>::Convert(const blink::IconDefinition& definition) {
  blink::mojom::blink::IconDefinitionPtr mojoDefinition =
      blink::mojom::blink::IconDefinition::New();
  mojoDefinition->src = definition.src();
  mojoDefinition->sizes = definition.sizes();
  mojoDefinition->type = definition.type();

  return mojoDefinition;
}

}  // namespace mojo
