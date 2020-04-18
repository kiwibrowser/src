// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_OBJECT_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_OBJECT_H_

#include <wayland-client-core.h>

#include <memory>

struct wl_buffer;
struct wl_callback;
struct wl_compositor;
struct wl_data_device_manager;
struct wl_data_device;
struct wl_data_offer;
struct wl_data_source;
struct wl_keyboard;
struct wl_output;
struct wl_pointer;
struct wl_registry;
struct wl_seat;
struct wl_shm;
struct wl_shm_pool;
struct wl_surface;
struct wl_touch;
struct xdg_shell;
struct xdg_surface;
struct zxdg_shell_v6;
struct zxdg_surface_v6;
struct zxdg_toplevel_v6;

namespace wl {

template <typename T>
struct ObjectTraits;

template <>
struct ObjectTraits<wl_buffer> {
  static const wl_interface* interface;
  static void (*deleter)(wl_buffer*);
};

template <>
struct ObjectTraits<wl_callback> {
  static const wl_interface* interface;
  static void (*deleter)(wl_callback*);
};

template <>
struct ObjectTraits<wl_compositor> {
  static const wl_interface* interface;
  static void (*deleter)(wl_compositor*);
};

template <>
struct ObjectTraits<wl_data_device_manager> {
  static const wl_interface* interface;
  static void (*deleter)(wl_data_device_manager*);
};

template <>
struct ObjectTraits<wl_data_device> {
  static const wl_interface* interface;
  static void (*deleter)(wl_data_device*);
};

template <>
struct ObjectTraits<wl_data_offer> {
  static const wl_interface* interface;
  static void (*deleter)(wl_data_offer*);
};

template <>
struct ObjectTraits<wl_data_source> {
  static const wl_interface* interface;
  static void (*deleter)(wl_data_source*);
};

template <>
struct ObjectTraits<wl_display> {
  static const wl_interface* interface;
  static void (*deleter)(wl_display*);
};

template <>
struct ObjectTraits<wl_output> {
  static const wl_interface* interface;
  static void (*deleter)(wl_output*);
};

template <>
struct ObjectTraits<wl_keyboard> {
  static const wl_interface* interface;
  static void (*deleter)(wl_keyboard*);
};

template <>
struct ObjectTraits<wl_pointer> {
  static const wl_interface* interface;
  static void (*deleter)(wl_pointer*);
};

template <>
struct ObjectTraits<wl_registry> {
  static const wl_interface* interface;
  static void (*deleter)(wl_registry*);
};

template <>
struct ObjectTraits<wl_seat> {
  static const wl_interface* interface;
  static void (*deleter)(wl_seat*);
};

template <>
struct ObjectTraits<wl_shm> {
  static const wl_interface* interface;
  static void (*deleter)(wl_shm*);
};

template <>
struct ObjectTraits<wl_shm_pool> {
  static const wl_interface* interface;
  static void (*deleter)(wl_shm_pool*);
};

template <>
struct ObjectTraits<wl_surface> {
  static const wl_interface* interface;
  static void (*deleter)(wl_surface*);
};

template <>
struct ObjectTraits<wl_touch> {
  static const wl_interface* interface;
  static void (*deleter)(wl_touch*);
};

template <>
struct ObjectTraits<xdg_shell> {
  static const wl_interface* interface;
  static void (*deleter)(xdg_shell*);
};

template <>
struct ObjectTraits<xdg_surface> {
  static const wl_interface* interface;
  static void (*deleter)(xdg_surface*);
};

template <>
struct ObjectTraits<zxdg_shell_v6> {
  static const wl_interface* interface;
  static void (*deleter)(zxdg_shell_v6*);
};

template <>
struct ObjectTraits<zxdg_surface_v6> {
  static const wl_interface* interface;
  static void (*deleter)(zxdg_surface_v6*);
};

template <>
struct ObjectTraits<zxdg_toplevel_v6> {
  static const wl_interface* interface;
  static void (*deleter)(zxdg_toplevel_v6*);
};

struct Deleter {
  template <typename T>
  void operator()(T* obj) {
    ObjectTraits<T>::deleter(obj);
  }
};

template <typename T>
class Object : public std::unique_ptr<T, Deleter> {
 public:
  Object() {}
  explicit Object(T* obj) : std::unique_ptr<T, Deleter>(obj) {}

  uint32_t id() {
    return wl_proxy_get_id(
        reinterpret_cast<wl_proxy*>(std::unique_ptr<T, Deleter>::get()));
  }
};

template <typename T>
wl::Object<T> Bind(wl_registry* registry, uint32_t name, uint32_t version) {
  return wl::Object<T>(static_cast<T*>(
      wl_registry_bind(registry, name, ObjectTraits<T>::interface, version)));
}

}  // namespace wl

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_OBJECT_H_
