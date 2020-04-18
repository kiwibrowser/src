// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_RENDERER_FACTORY_SELECTOR_H_
#define MEDIA_BASE_RENDERER_FACTORY_SELECTOR_H_

#include "base/callback.h"
#include "base/optional.h"
#include "media/base/renderer_factory.h"

namespace media {

// RendererFactorySelector owns RendererFactory instances used within WMPI.
// Its purpose is to aggregate the signals and centralize the logic behind
// choosing which RendererFactory should be used when creating a new Renderer.
class MEDIA_EXPORT RendererFactorySelector {
 public:
  using QueryIsRemotingActiveCB = base::Callback<bool()>;
  using QueryIsFlingingActiveCB = base::Callback<bool()>;

  enum FactoryType {
    DEFAULT,       // DefaultRendererFactory.
    MOJO,          // MojoRendererFactory.
    MEDIA_PLAYER,  // MediaPlayerRendererClientFactory.
    COURIER,       // CourierRendererFactory.
    FLINGING,      // FlingingRendererClientFactory
    FACTORY_TYPE_MAX = FLINGING,
  };

  RendererFactorySelector();
  ~RendererFactorySelector();

  // NOTE: There should be at most one factory per factory type.
  void AddFactory(FactoryType type, std::unique_ptr<RendererFactory> factory);

  // Sets the base factory to be returned, when there are no signals telling us
  // to select any specific factory.
  // NOTE: |type| can be different than FactoryType::DEFAULT. DEFAULT is used to
  // identify the DefaultRendererFactory, not to indicate that a factory should
  // be used by default.
  void SetBaseFactoryType(FactoryType type);

  // Updates |current_factory_| if necessary, and returns its value.
  // NOTE: SetBaseFactoryType() must be called before calling this method.
  RendererFactory* GetCurrentFactory();

#if defined(OS_ANDROID)
  // Sets whether we should be using the MEDIA_PLAYER factory instead of the
  // base factory.
  void SetUseMediaPlayer(bool use_media_player);
#endif

  // Sets the callback to query whether we are currently remoting, and if we
  // should temporarily use the COURIER factory.
  void SetQueryIsRemotingActiveCB(
      QueryIsRemotingActiveCB query_is_remoting_active_cb);

  // Sets the callback to query whether we are currently flinging media, and if
  // we should temporarily use the FLINGING factory.
  void SetQueryIsFlingingActiveCB(
      QueryIsFlingingActiveCB query_is_flinging_active_cb);

 private:
  bool use_media_player_ = false;

  QueryIsRemotingActiveCB query_is_remoting_active_cb_;
  QueryIsFlingingActiveCB query_is_flinging_active_cb_;

  base::Optional<FactoryType> base_factory_type_;
  std::unique_ptr<RendererFactory> factories_[FACTORY_TYPE_MAX + 1];
  DISALLOW_COPY_AND_ASSIGN(RendererFactorySelector);
};

}  // namespace media

#endif  // MEDIA_BASE_RENDERER_FACTORY_H_
