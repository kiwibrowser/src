// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_INTERNAL_EVENT_MODEL_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_INTERNAL_EVENT_MODEL_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "base/macros.h"

namespace feature_engagement {
class Event;

// A EventModel provides all necessary runtime state.
class EventModel {
 public:
  // Callback for when model initialization has finished. The |success|
  // argument denotes whether the model was successfully initialized.
  using OnModelInitializationFinished = base::Callback<void(bool success)>;

  virtual ~EventModel() = default;

  // Initialize the model, including all underlying sub systems. When all
  // required operations have been finished, a callback is posted.
  virtual void Initialize(const OnModelInitializationFinished& callback,
                          uint32_t current_day) = 0;

  // Returns whether the model is ready, i.e. whether it has been successfully
  // initialized.
  virtual bool IsReady() const = 0;

  // Retrieves the Event object for the event with the given name. If the event
  // is not found, a nullptr will be returned. Calling this before the
  // EventModel has finished initializing will result in undefined behavior.
  virtual const Event* GetEvent(const std::string& event_name) const = 0;

  // Increments the counter for today for how many times the event has happened.
  // If the event has never happened before, the Event object will be created.
  // The |current_day| should be the number of days since UNIX epoch (see
  // TimeProvider::GetCurrentDay()).
  virtual void IncrementEvent(const std::string& event_name,
                              uint32_t current_day) = 0;

 protected:
  EventModel() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(EventModel);
};

}  // namespace feature_engagement

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_INTERNAL_EVENT_MODEL_H_
