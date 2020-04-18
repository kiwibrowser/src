// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/edge_effect.h"

#include "base/macros.h"
#include "cc/layers/layer.h"
#include "cc/layers/ui_resource_layer.h"
#include "ui/android/animation_utils.h"
#include "ui/android/resources/resource_manager.h"
#include "ui/android/resources/system_ui_resource_type.h"
#include "ui/android/window_android_compositor.h"

namespace ui {

namespace {

const ui::SystemUIResourceType kEdgeResourceId = ui::OVERSCROLL_EDGE;
const ui::SystemUIResourceType kGlowResourceId = ui::OVERSCROLL_GLOW;

// Time it will take the effect to fully recede in ms
const int kRecedeTimeMs = 1000;

// Time it will take before a pulled glow begins receding in ms
const int kPullTimeMs = 167;

// Time it will take in ms for a pulled glow to decay before release
const int kPullDecayTimeMs = 1000;

const float kMaxAlpha = 1.f;
const float kHeldEdgeScaleY = .5f;

const float kMaxGlowHeight = 4.f;

const float kPullGlowBegin = 1.f;
const float kPullEdgeBegin = 0.6f;

// Min/max velocity that will be absorbed
const float kMinVelocity = 100.f;
const float kMaxVelocity = 10000.f;

const float kEpsilon = 0.001f;

const float kGlowHeightWidthRatio = 0.25f;

// How much dragging should effect the height of the edge image.
// Number determined by user testing.
const int kPullDistanceEdgeFactor = 7;

// How much dragging should effect the height of the glow image.
// Number determined by user testing.
const int kPullDistanceGlowFactor = 7;
const float kPullDistanceAlphaGlowFactor = 1.1f;

const int kVelocityEdgeFactor = 8;
const int kVelocityGlowFactor = 12;

const float kEdgeHeightAtMdpi = 12.f;
const float kGlowHeightAtMdpi = 128.f;

}  // namespace

class EdgeEffect::EffectLayer {
 public:
  EffectLayer(ui::SystemUIResourceType resource_type,
              ui::ResourceManager* resource_manager)
      : ui_resource_layer_(cc::UIResourceLayer::Create()),
        resource_type_(resource_type),
        resource_manager_(resource_manager) {}

  ~EffectLayer() { ui_resource_layer_->RemoveFromParent(); }

  void SetParent(cc::Layer* parent) {
    if (ui_resource_layer_->parent() != parent)
      parent->AddChild(ui_resource_layer_);
  }

  void Disable() { ui_resource_layer_->SetIsDrawable(false); }

  void Update(const gfx::Size& size,
              const gfx::Transform& transform,
              float opacity) {
    ui_resource_layer_->SetUIResourceId(resource_manager_->GetUIResourceId(
        ui::ANDROID_RESOURCE_TYPE_SYSTEM, resource_type_));
    ui_resource_layer_->SetIsDrawable(true);
    ui_resource_layer_->SetTransformOrigin(
        gfx::Point3F(size.width() * 0.5f, 0, 0));
    ui_resource_layer_->SetTransform(transform);
    ui_resource_layer_->SetBounds(size);
    ui_resource_layer_->SetOpacity(Clamp(opacity, 0.f, 1.f));
  }

  scoped_refptr<cc::UIResourceLayer> ui_resource_layer_;
  ui::SystemUIResourceType resource_type_;
  ui::ResourceManager* resource_manager_;

  DISALLOW_COPY_AND_ASSIGN(EffectLayer);
};

EdgeEffect::EdgeEffect(ui::ResourceManager* resource_manager,
                       float device_scale_factor)
    : edge_(new EffectLayer(kEdgeResourceId, resource_manager)),
      glow_(new EffectLayer(kGlowResourceId, resource_manager)),
      base_edge_height_(kEdgeHeightAtMdpi * device_scale_factor),
      base_glow_height_(kGlowHeightAtMdpi * device_scale_factor),
      edge_alpha_(0),
      edge_scale_y_(0),
      glow_alpha_(0),
      glow_scale_y_(0),
      edge_alpha_start_(0),
      edge_alpha_finish_(0),
      edge_scale_y_start_(0),
      edge_scale_y_finish_(0),
      glow_alpha_start_(0),
      glow_alpha_finish_(0),
      glow_scale_y_start_(0),
      glow_scale_y_finish_(0),
      state_(STATE_IDLE),
      pull_distance_(0) {
}

EdgeEffect::~EdgeEffect() {
}

bool EdgeEffect::IsFinished() const {
  return state_ == STATE_IDLE;
}

void EdgeEffect::Finish() {
  edge_->Disable();
  glow_->Disable();
  pull_distance_ = 0;
  state_ = STATE_IDLE;
}

void EdgeEffect::Pull(base::TimeTicks current_time,
                      float delta_distance,
                      float displacement) {
  if (state_ == STATE_PULL_DECAY && current_time - start_time_ < duration_) {
    return;
  }
  if (state_ != STATE_PULL) {
    glow_scale_y_ = kPullGlowBegin;
  }
  state_ = STATE_PULL;

  start_time_ = current_time;
  duration_ = base::TimeDelta::FromMilliseconds(kPullTimeMs);

  float abs_delta_distance = std::abs(delta_distance);
  pull_distance_ += delta_distance;
  float distance = std::abs(pull_distance_);

  edge_alpha_ = edge_alpha_start_ = Clamp(distance, kPullEdgeBegin, kMaxAlpha);
  edge_scale_y_ = edge_scale_y_start_ =
      Clamp(distance * kPullDistanceEdgeFactor, kHeldEdgeScaleY, 1.f);

  glow_alpha_ = glow_alpha_start_ =
      std::min(kMaxAlpha,
               glow_alpha_ + abs_delta_distance * kPullDistanceAlphaGlowFactor);

  float glow_change = abs_delta_distance;
  if (delta_distance > 0 && pull_distance_ < 0)
    glow_change = -glow_change;
  if (pull_distance_ == 0)
    glow_scale_y_ = 0;

  // Do not allow glow to get larger than kMaxGlowHeight.
  glow_scale_y_ = glow_scale_y_start_ =
      Clamp(glow_scale_y_ + glow_change * kPullDistanceGlowFactor,
            0.f,
            kMaxGlowHeight);

  edge_alpha_finish_ = edge_alpha_;
  edge_scale_y_finish_ = edge_scale_y_;
  glow_alpha_finish_ = glow_alpha_;
  glow_scale_y_finish_ = glow_scale_y_;
}

void EdgeEffect::Release(base::TimeTicks current_time) {
  pull_distance_ = 0;

  if (state_ != STATE_PULL && state_ != STATE_PULL_DECAY)
    return;

  state_ = STATE_RECEDE;
  edge_alpha_start_ = edge_alpha_;
  edge_scale_y_start_ = edge_scale_y_;
  glow_alpha_start_ = glow_alpha_;
  glow_scale_y_start_ = glow_scale_y_;

  edge_alpha_finish_ = 0.f;
  edge_scale_y_finish_ = 0.f;
  glow_alpha_finish_ = 0.f;
  glow_scale_y_finish_ = 0.f;

  start_time_ = current_time;
  duration_ = base::TimeDelta::FromMilliseconds(kRecedeTimeMs);
}

void EdgeEffect::Absorb(base::TimeTicks current_time, float velocity) {
  state_ = STATE_ABSORB;
  velocity = Clamp(std::abs(velocity), kMinVelocity, kMaxVelocity);

  start_time_ = current_time;
  // This should never be less than 1 millisecond.
  duration_ = base::TimeDelta::FromMilliseconds(0.15f + (velocity * 0.02f));

  // The edge should always be at least partially visible, regardless
  // of velocity.
  edge_alpha_start_ = 0.f;
  edge_scale_y_ = edge_scale_y_start_ = 0.f;
  // The glow depends more on the velocity, and therefore starts out
  // nearly invisible.
  glow_alpha_start_ = 0.3f;
  glow_scale_y_start_ = 0.f;

  // Factor the velocity by 8. Testing on device shows this works best to
  // reflect the strength of the user's scrolling.
  edge_alpha_finish_ = Clamp(velocity * kVelocityEdgeFactor, 0.f, 1.f);
  // Edge should never get larger than the size of its asset.
  edge_scale_y_finish_ =
      Clamp(velocity * kVelocityEdgeFactor, kHeldEdgeScaleY, 1.f);

  // Growth for the size of the glow should be quadratic to properly
  // respond
  // to a user's scrolling speed. The faster the scrolling speed, the more
  // intense the effect should be for both the size and the saturation.
  glow_scale_y_finish_ =
      std::min(0.025f + (velocity * (velocity / 100) * 0.00015f), 1.75f);
  // Alpha should change for the glow as well as size.
  glow_alpha_finish_ = Clamp(
      glow_alpha_start_, velocity * kVelocityGlowFactor * .00001f, kMaxAlpha);
}

bool EdgeEffect::Update(base::TimeTicks current_time) {
  if (IsFinished())
    return false;

  const double dt = (current_time - start_time_).InMilliseconds();
  const double t = std::min(dt / duration_.InMilliseconds(), 1.);
  const float interp = static_cast<float>(Damp(t, 1.));

  edge_alpha_ = Lerp(edge_alpha_start_, edge_alpha_finish_, interp);
  edge_scale_y_ = Lerp(edge_scale_y_start_, edge_scale_y_finish_, interp);
  glow_alpha_ = Lerp(glow_alpha_start_, glow_alpha_finish_, interp);
  glow_scale_y_ = Lerp(glow_scale_y_start_, glow_scale_y_finish_, interp);

  if (t >= 1.f - kEpsilon) {
    switch (state_) {
      case STATE_ABSORB:
        state_ = STATE_RECEDE;
        start_time_ = current_time;
        duration_ = base::TimeDelta::FromMilliseconds(kRecedeTimeMs);

        edge_alpha_start_ = edge_alpha_;
        edge_scale_y_start_ = edge_scale_y_;
        glow_alpha_start_ = glow_alpha_;
        glow_scale_y_start_ = glow_scale_y_;

        // After absorb, the glow and edge should fade to nothing.
        edge_alpha_finish_ = 0.f;
        edge_scale_y_finish_ = 0.f;
        glow_alpha_finish_ = 0.f;
        glow_scale_y_finish_ = 0.f;
        break;
      case STATE_PULL:
        state_ = STATE_PULL_DECAY;
        start_time_ = current_time;
        duration_ = base::TimeDelta::FromMilliseconds(kPullDecayTimeMs);

        edge_alpha_start_ = edge_alpha_;
        edge_scale_y_start_ = edge_scale_y_;
        glow_alpha_start_ = glow_alpha_;
        glow_scale_y_start_ = glow_scale_y_;

        // After pull, the glow and edge should fade to nothing.
        edge_alpha_finish_ = 0.f;
        edge_scale_y_finish_ = 0.f;
        glow_alpha_finish_ = 0.f;
        glow_scale_y_finish_ = 0.f;
        break;
      case STATE_PULL_DECAY: {
        // When receding, we want edge to decrease more slowly
        // than the glow.
        const float factor =
            glow_scale_y_finish_
                ? 1 / (glow_scale_y_finish_ * glow_scale_y_finish_)
                : std::numeric_limits<float>::max();
        edge_scale_y_ =
            edge_scale_y_start_ +
            (edge_scale_y_finish_ - edge_scale_y_start_) * interp * factor;
        state_ = STATE_RECEDE;
      } break;
      case STATE_RECEDE:
        Finish();
        break;
      default:
        break;
    }
  }

  if (state_ == STATE_RECEDE && glow_scale_y_ <= 0 && edge_scale_y_ <= 0)
    Finish();

  return !IsFinished();
}

float EdgeEffect::GetAlpha() const {
  return IsFinished() ? 0.f : std::max(glow_alpha_, edge_alpha_);
}

void EdgeEffect::ApplyToLayers(Edge edge,
                               const gfx::SizeF& viewport_size,
                               float offset) {
  if (IsFinished())
    return;

  // An empty window size, while meaningless, is also relatively harmless, and
  // will simply prevent any drawing of the layers.
  if (viewport_size.IsEmpty()) {
    edge_->Disable();
    glow_->Disable();
    return;
  }

  gfx::SizeF size = ComputeOrientedSize(edge, viewport_size);
  gfx::Transform transform = ComputeTransform(edge, viewport_size, offset);

  // Glow
  const int scaled_glow_height = static_cast<int>(
      std::min(base_glow_height_ * glow_scale_y_ * kGlowHeightWidthRatio * 0.6f,
               base_glow_height_ * kMaxGlowHeight) +
      0.5f);
  const gfx::Size glow_size(size.width(), scaled_glow_height);
  glow_->Update(glow_size, transform, glow_alpha_);

  // Edge
  const int scaled_edge_height =
      static_cast<int>(base_edge_height_ * edge_scale_y_);
  const gfx::Size edge_size(size.width(), scaled_edge_height);
  edge_->Update(edge_size, transform, edge_alpha_);
}

void EdgeEffect::SetParent(cc::Layer* parent) {
  edge_->SetParent(parent);
  glow_->SetParent(parent);
}

// static
void EdgeEffect::PreloadResources(ui::ResourceManager* resource_manager) {
  DCHECK(resource_manager);
  resource_manager->PreloadResource(ui::ANDROID_RESOURCE_TYPE_SYSTEM,
                                    kEdgeResourceId);
  resource_manager->PreloadResource(ui::ANDROID_RESOURCE_TYPE_SYSTEM,
                                    kGlowResourceId);
}

}  // namespace ui
