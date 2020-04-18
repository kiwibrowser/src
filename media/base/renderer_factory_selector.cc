// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/renderer_factory_selector.h"

#include "base/logging.h"

namespace media {

RendererFactorySelector::RendererFactorySelector() = default;

RendererFactorySelector::~RendererFactorySelector() = default;

void RendererFactorySelector::AddFactory(
    FactoryType type,
    std::unique_ptr<RendererFactory> factory) {
  DCHECK(!factories_[type]);

  factories_[type] = std::move(factory);
}

void RendererFactorySelector::SetBaseFactoryType(FactoryType type) {
  DCHECK(factories_[type]);
  base_factory_type_ = type;
}

RendererFactory* RendererFactorySelector::GetCurrentFactory() {
  DCHECK(base_factory_type_);
  FactoryType next_factory_type = base_factory_type_.value();

  if (use_media_player_)
    next_factory_type = FactoryType::MEDIA_PLAYER;

  if (query_is_remoting_active_cb_ && query_is_remoting_active_cb_.Run())
    next_factory_type = FactoryType::COURIER;

  if (query_is_flinging_active_cb_ && query_is_flinging_active_cb_.Run())
    next_factory_type = FactoryType::FLINGING;

  DVLOG(1) << __func__ << " Selecting factory type: " << next_factory_type;

  RendererFactory* current_factory = factories_[next_factory_type].get();

  DCHECK(current_factory);

  return current_factory;
}

#if defined(OS_ANDROID)
void RendererFactorySelector::SetUseMediaPlayer(bool use_media_player) {
  use_media_player_ = use_media_player;
}
#endif

void RendererFactorySelector::SetQueryIsRemotingActiveCB(
    QueryIsRemotingActiveCB query_is_remoting_active_cb) {
  DCHECK(!query_is_remoting_active_cb_);
  query_is_remoting_active_cb_ = query_is_remoting_active_cb;
}

void RendererFactorySelector::SetQueryIsFlingingActiveCB(
    QueryIsFlingingActiveCB query_is_flinging_active_cb) {
  DCHECK(!query_is_flinging_active_cb_);
  query_is_flinging_active_cb_ = query_is_flinging_active_cb;
}

}  // namespace media
