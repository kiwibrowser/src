// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_CONFIG_H_
#define ASH_PUBLIC_CPP_APP_CONFIG_H_

namespace ash {

// Enumeration of the possible configurations supported by ash.
enum class Config {
  // Classic mode does not use mus.
  CLASSIC,

  // Aura is backed by mus, but chrome and ash are still in the same process.
  // TODO(jamescook): Remove this mode. We are switching to window service as a
  // library, https://crbug.com/837684
  MUS,

  // Aura is backed by mus and chrome and ash are in separate processes. In this
  // mode chrome code can only use ash code in ash/public/cpp.
  MASH,
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_APP_CONFIG_H_
