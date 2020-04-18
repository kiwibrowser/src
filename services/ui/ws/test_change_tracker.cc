// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/test_change_tracker.h"

#include <stddef.h>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "mojo/public/cpp/bindings/map.h"
#include "services/ui/common/util.h"
#include "ui/base/cursor/cursor.h"
#include "ui/gfx/geometry/point_conversions.h"

namespace ui {

namespace ws {

std::string WindowIdToString(Id id) {
  return (id == 0) ? "null"
                   : base::StringPrintf("%d,%d", ClientIdFromTransportId(id),
                                        ClientWindowIdFromTransportId(id));
}

namespace {

std::string DirectionToString(mojom::OrderDirection direction) {
  return direction == mojom::OrderDirection::ABOVE ? "above" : "below";
}

enum class ChangeDescriptionType {
  ONE,
  TWO,
  // Includes display id and location of events.
  THREE,
};

std::string ChangeToDescription(const Change& change,
                                ChangeDescriptionType type) {
  switch (change.type) {
    case CHANGE_TYPE_EMBED:
      if (type == ChangeDescriptionType::ONE)
        return "OnEmbed";
      return base::StringPrintf("OnEmbed drawn=%s",
                                change.bool_value ? "true" : "false");

    case CHANGE_TYPE_EMBEDDED_APP_DISCONNECTED:
      return base::StringPrintf("OnEmbeddedAppDisconnected window=%s",
                                WindowIdToString(change.window_id).c_str());

    case CHANGE_TYPE_UNEMBED:
      return base::StringPrintf("OnUnembed window=%s",
                                WindowIdToString(change.window_id).c_str());

    case CHANGE_TYPE_CAPTURE_CHANGED:
      return base::StringPrintf("OnCaptureChanged new_window=%s old_window=%s",
                                WindowIdToString(change.window_id).c_str(),
                                WindowIdToString(change.window_id2).c_str());

    case CHANGE_TYPE_FRAME_SINK_ID_ALLOCATED:
      return base::StringPrintf("OnFrameSinkIdAllocated window=%s %s",
                                WindowIdToString(change.window_id).c_str(),
                                change.frame_sink_id.ToString().c_str());

    case CHANGE_TYPE_NODE_ADD_TRANSIENT_WINDOW:
      return base::StringPrintf("AddTransientWindow parent = %s child = %s",
                                WindowIdToString(change.window_id).c_str(),
                                WindowIdToString(change.window_id2).c_str());

    case CHANGE_TYPE_NODE_BOUNDS_CHANGED:
      return base::StringPrintf(
          "BoundsChanged window=%s old_bounds=%s new_bounds=%s "
          "local_surface_id=%s",
          WindowIdToString(change.window_id).c_str(),
          change.bounds.ToString().c_str(), change.bounds2.ToString().c_str(),
          change.local_surface_id ? change.local_surface_id->ToString().c_str()
                                  : "(none)");

    case CHANGE_TYPE_NODE_HIERARCHY_CHANGED:
      return base::StringPrintf(
          "HierarchyChanged window=%s old_parent=%s new_parent=%s",
          WindowIdToString(change.window_id).c_str(),
          WindowIdToString(change.window_id2).c_str(),
          WindowIdToString(change.window_id3).c_str());

    case CHANGE_TYPE_NODE_REMOVE_TRANSIENT_WINDOW_FROM_PARENT:
      return base::StringPrintf(
          "RemoveTransientWindowFromParent parent = %s child = %s",
          WindowIdToString(change.window_id).c_str(),
          WindowIdToString(change.window_id2).c_str());

    case CHANGE_TYPE_NODE_REORDERED:
      return base::StringPrintf("Reordered window=%s relative=%s direction=%s",
                                WindowIdToString(change.window_id).c_str(),
                                WindowIdToString(change.window_id2).c_str(),
                                DirectionToString(change.direction).c_str());

    case CHANGE_TYPE_NODE_DELETED:
      return base::StringPrintf("WindowDeleted window=%s",
                                WindowIdToString(change.window_id).c_str());

    case CHANGE_TYPE_NODE_VISIBILITY_CHANGED:
      return base::StringPrintf("VisibilityChanged window=%s visible=%s",
                                WindowIdToString(change.window_id).c_str(),
                                change.bool_value ? "true" : "false");

    case CHANGE_TYPE_NODE_DRAWN_STATE_CHANGED:
      return base::StringPrintf("DrawnStateChanged window=%s drawn=%s",
                                WindowIdToString(change.window_id).c_str(),
                                change.bool_value ? "true" : "false");

    case CHANGE_TYPE_INPUT_EVENT: {
      std::string result = base::StringPrintf(
          "InputEvent window=%s event_action=%d",
          WindowIdToString(change.window_id).c_str(), change.event_action);
      if (change.matches_pointer_watcher)
        result += " matches_pointer_watcher";
      return result;
    }

    case CHANGE_TYPE_POINTER_WATCHER_EVENT:
      return base::StringPrintf("PointerWatcherEvent event_action=%d window=%s",
                                change.event_action,
                                WindowIdToString(change.window_id).c_str());

    case CHANGE_TYPE_PROPERTY_CHANGED:
      return base::StringPrintf("PropertyChanged window=%s key=%s value=%s",
                                WindowIdToString(change.window_id).c_str(),
                                change.property_key.c_str(),
                                change.property_value.c_str());

    case CHANGE_TYPE_FOCUSED:
      return base::StringPrintf("Focused id=%s",
                                WindowIdToString(change.window_id).c_str());

    case CHANGE_TYPE_CURSOR_CHANGED:
      return base::StringPrintf("CursorChanged id=%s cursor_type=%d",
                                WindowIdToString(change.window_id).c_str(),
                                static_cast<int>(change.cursor_type));
    case CHANGE_TYPE_ON_CHANGE_COMPLETED:
      return base::StringPrintf("ChangeCompleted id=%d sucess=%s",
                                change.change_id,
                                change.bool_value ? "true" : "false");

    case CHANGE_TYPE_ON_TOP_LEVEL_CREATED:
      return base::StringPrintf("TopLevelCreated id=%d window_id=%s drawn=%s",
                                change.change_id,
                                WindowIdToString(change.window_id).c_str(),
                                change.bool_value ? "true" : "false");
    case CHANGE_TYPE_OPACITY:
      return base::StringPrintf("OpacityChanged window_id=%s opacity=%.2f",
                                WindowIdToString(change.window_id).c_str(),
                                change.float_value);
    case CHANGE_TYPE_SURFACE_CHANGED:
      return base::StringPrintf("SurfaceCreated window_id=%s surface_id=%s",
                                WindowIdToString(change.window_id).c_str(),
                                change.surface_id.ToString().c_str());
    case CHANGE_TYPE_TRANSFORM_CHANGED:
      return base::StringPrintf("TransformChanged window_id=%s",
                                WindowIdToString(change.window_id).c_str());
  }
  return std::string();
}

std::string SingleChangeToDescriptionImpl(const std::vector<Change>& changes,
                                          ChangeDescriptionType change_type) {
  std::string result;
  for (auto& change : changes) {
    if (!result.empty())
      result += "\n";
    result += ChangeToDescription(change, change_type);
  }
  return result;
}

}  // namespace

std::vector<std::string> ChangesToDescription1(
    const std::vector<Change>& changes) {
  std::vector<std::string> strings(changes.size());
  for (size_t i = 0; i < changes.size(); ++i)
    strings[i] = ChangeToDescription(changes[i], ChangeDescriptionType::ONE);
  return strings;
}

std::string SingleChangeToDescription(const std::vector<Change>& changes) {
  return SingleChangeToDescriptionImpl(changes, ChangeDescriptionType::ONE);
}

std::string SingleChangeToDescription2(const std::vector<Change>& changes) {
  return SingleChangeToDescriptionImpl(changes, ChangeDescriptionType::TWO);
}

std::string SingleWindowDescription(const std::vector<TestWindow>& windows) {
  if (windows.empty())
    return "no windows";
  std::string result;
  for (const TestWindow& window : windows)
    result += window.ToString();
  return result;
}

std::string ChangeWindowDescription(const std::vector<Change>& changes) {
  if (changes.size() != 1)
    return std::string();
  std::vector<std::string> window_strings(changes[0].windows.size());
  for (size_t i = 0; i < changes[0].windows.size(); ++i)
    window_strings[i] = "[" + changes[0].windows[i].ToString() + "]";
  return base::JoinString(window_strings, ",");
}

TestWindow WindowDataToTestWindow(const mojom::WindowDataPtr& data) {
  TestWindow window;
  window.parent_id = data->parent_id;
  window.window_id = data->window_id;
  window.visible = data->visible;
  window.properties = mojo::FlatMapToMap(data->properties);
  return window;
}

void WindowDatasToTestWindows(const std::vector<mojom::WindowDataPtr>& data,
                              std::vector<TestWindow>* test_windows) {
  for (size_t i = 0; i < data.size(); ++i)
    test_windows->push_back(WindowDataToTestWindow(data[i]));
}

Change::Change()
    : type(CHANGE_TYPE_EMBED),
      window_id(0),
      window_id2(0),
      window_id3(0),
      event_action(0),
      matches_pointer_watcher(false),
      direction(mojom::OrderDirection::ABOVE),
      bool_value(false),
      float_value(0.f),
      cursor_type(ui::CursorType::kNull),
      change_id(0u),
      display_id(0) {}

Change::Change(const Change& other) = default;

Change::~Change() {}

TestChangeTracker::TestChangeTracker() : delegate_(NULL) {}

TestChangeTracker::~TestChangeTracker() {}

void TestChangeTracker::OnEmbed(mojom::WindowDataPtr root, bool drawn) {
  Change change;
  change.type = CHANGE_TYPE_EMBED;
  change.bool_value = drawn;
  change.windows.push_back(WindowDataToTestWindow(root));
  AddChange(change);
}

void TestChangeTracker::OnEmbeddedAppDisconnected(Id window_id) {
  Change change;
  change.type = CHANGE_TYPE_EMBEDDED_APP_DISCONNECTED;
  change.window_id = window_id;
  AddChange(change);
}

void TestChangeTracker::OnWindowBoundsChanged(
    Id window_id,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  Change change;
  change.type = CHANGE_TYPE_NODE_BOUNDS_CHANGED;
  change.window_id = window_id;
  change.bounds = old_bounds;
  change.bounds2 = new_bounds;
  change.local_surface_id = local_surface_id;
  AddChange(change);
}

void TestChangeTracker::OnWindowTransformChanged(Id window_id) {
  Change change;
  change.type = CHANGE_TYPE_TRANSFORM_CHANGED;
  change.window_id = window_id;
  AddChange(change);
}

void TestChangeTracker::OnUnembed(Id window_id) {
  Change change;
  change.type = CHANGE_TYPE_UNEMBED;
  change.window_id = window_id;
  AddChange(change);
}

void TestChangeTracker::OnTransientWindowAdded(Id window_id,
                                               Id transient_window_id) {
  Change change;
  change.type = CHANGE_TYPE_NODE_ADD_TRANSIENT_WINDOW;
  change.window_id = window_id;
  change.window_id2 = transient_window_id;
  AddChange(change);
}

void TestChangeTracker::OnTransientWindowRemoved(Id window_id,
                                                 Id transient_window_id) {
  Change change;
  change.type = CHANGE_TYPE_NODE_REMOVE_TRANSIENT_WINDOW_FROM_PARENT;
  change.window_id = window_id;
  change.window_id2 = transient_window_id;
  AddChange(change);
}

void TestChangeTracker::OnCaptureChanged(Id new_capture_window_id,
                                         Id old_capture_window_id) {
  Change change;
  change.type = CHANGE_TYPE_CAPTURE_CHANGED;
  change.window_id = new_capture_window_id;
  change.window_id2 = old_capture_window_id;
  AddChange(change);
}

void TestChangeTracker::OnFrameSinkIdAllocated(
    Id window_id,
    const viz::FrameSinkId& frame_sink_id) {
  Change change;
  change.type = CHANGE_TYPE_FRAME_SINK_ID_ALLOCATED;
  change.window_id = window_id;
  change.frame_sink_id = frame_sink_id;
  AddChange(change);
}

void TestChangeTracker::OnWindowHierarchyChanged(
    Id window_id,
    Id old_parent_id,
    Id new_parent_id,
    std::vector<mojom::WindowDataPtr> windows) {
  Change change;
  change.type = CHANGE_TYPE_NODE_HIERARCHY_CHANGED;
  change.window_id = window_id;
  change.window_id2 = old_parent_id;
  change.window_id3 = new_parent_id;
  WindowDatasToTestWindows(windows, &change.windows);
  AddChange(change);
}

void TestChangeTracker::OnWindowReordered(Id window_id,
                                          Id relative_window_id,
                                          mojom::OrderDirection direction) {
  Change change;
  change.type = CHANGE_TYPE_NODE_REORDERED;
  change.window_id = window_id;
  change.window_id2 = relative_window_id;
  change.direction = direction;
  AddChange(change);
}

void TestChangeTracker::OnWindowDeleted(Id window_id) {
  Change change;
  change.type = CHANGE_TYPE_NODE_DELETED;
  change.window_id = window_id;
  AddChange(change);
}

void TestChangeTracker::OnWindowVisibilityChanged(Id window_id, bool visible) {
  Change change;
  change.type = CHANGE_TYPE_NODE_VISIBILITY_CHANGED;
  change.window_id = window_id;
  change.bool_value = visible;
  AddChange(change);
}

void TestChangeTracker::OnWindowOpacityChanged(Id window_id, float opacity) {
  Change change;
  change.type = CHANGE_TYPE_OPACITY;
  change.window_id = window_id;
  change.float_value = opacity;
  AddChange(change);
}

void TestChangeTracker::OnWindowParentDrawnStateChanged(Id window_id,
                                                        bool drawn) {
  Change change;
  change.type = CHANGE_TYPE_NODE_DRAWN_STATE_CHANGED;
  change.window_id = window_id;
  change.bool_value = drawn;
  AddChange(change);
}

void TestChangeTracker::OnWindowInputEvent(
    Id window_id,
    const ui::Event& event,
    int64_t display_id,
    const gfx::PointF& event_location_in_screen_pixel_layout,
    bool matches_pointer_watcher) {
  Change change;
  change.type = CHANGE_TYPE_INPUT_EVENT;
  change.window_id = window_id;
  change.event_action = static_cast<int32_t>(event.type());
  change.matches_pointer_watcher = matches_pointer_watcher;
  change.display_id = display_id;
  if (event.IsLocatedEvent())
    change.location1 = event.AsLocatedEvent()->root_location();
  change.location2 = event_location_in_screen_pixel_layout;
  if (event.IsKeyEvent() && event.AsKeyEvent()->properties())
    change.key_event_properties = *event.AsKeyEvent()->properties();
  AddChange(change);
}

void TestChangeTracker::OnPointerEventObserved(const ui::Event& event,
                                               Id window_id) {
  Change change;
  change.type = CHANGE_TYPE_POINTER_WATCHER_EVENT;
  change.event_action = static_cast<int32_t>(event.type());
  change.window_id = window_id;
  AddChange(change);
}

void TestChangeTracker::OnWindowSharedPropertyChanged(
    Id window_id,
    const std::string& name,
    const base::Optional<std::vector<uint8_t>>& data) {
  Change change;
  change.type = CHANGE_TYPE_PROPERTY_CHANGED;
  change.window_id = window_id;
  change.property_key = name;
  if (!data)
    change.property_value = "NULL";
  else
    change.property_value.assign(data->begin(), data->end());
  AddChange(change);
}

void TestChangeTracker::OnWindowFocused(Id window_id) {
  Change change;
  change.type = CHANGE_TYPE_FOCUSED;
  change.window_id = window_id;
  AddChange(change);
}

void TestChangeTracker::OnWindowCursorChanged(Id window_id,
                                              const ui::CursorData& cursor) {
  Change change;
  change.type = CHANGE_TYPE_CURSOR_CHANGED;
  change.window_id = window_id;
  change.cursor_type = cursor.cursor_type();
  AddChange(change);
}

void TestChangeTracker::OnChangeCompleted(uint32_t change_id, bool success) {
  Change change;
  change.type = CHANGE_TYPE_ON_CHANGE_COMPLETED;
  change.change_id = change_id;
  change.bool_value = success;
  AddChange(change);
}

void TestChangeTracker::OnTopLevelCreated(uint32_t change_id,
                                          mojom::WindowDataPtr window_data,
                                          bool drawn) {
  Change change;
  change.type = CHANGE_TYPE_ON_TOP_LEVEL_CREATED;
  change.change_id = change_id;
  change.window_id = window_data->window_id;
  change.bool_value = drawn;
  AddChange(change);
}

void TestChangeTracker::OnWindowSurfaceChanged(
    Id window_id,
    const viz::SurfaceInfo& surface_info) {
  Change change;
  change.type = CHANGE_TYPE_SURFACE_CHANGED;
  change.window_id = window_id;
  change.surface_id = surface_info.id();
  change.frame_size = surface_info.size_in_pixels();
  change.device_scale_factor = surface_info.device_scale_factor();
  AddChange(change);
}

void TestChangeTracker::AddChange(const Change& change) {
  changes_.push_back(change);
  if (delegate_)
    delegate_->OnChangeAdded();
}

TestWindow::TestWindow() {}

TestWindow::TestWindow(const TestWindow& other) = default;

TestWindow::~TestWindow() {}

std::string TestWindow::ToString() const {
  return base::StringPrintf("window=%s parent=%s",
                            WindowIdToString(window_id).c_str(),
                            WindowIdToString(parent_id).c_str());
}

std::string TestWindow::ToString2() const {
  return base::StringPrintf(
      "window=%s parent=%s visible=%s", WindowIdToString(window_id).c_str(),
      WindowIdToString(parent_id).c_str(), visible ? "true" : "false");
}

}  // namespace ws

}  // namespace ui
