// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "device/gamepad/public/cpp/gamepad.h"
#include "device/gamepad/public/mojom/gamepad_mojom_traits_test.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

enum GamepadTestDataType {
  GamepadCommon = 0,
  GamepadPose_HasOrientation = 1,
  GamepadPose_HasPosition = 2,
  GamepadPose_Null = 3,
};

Gamepad GetWebGamepadInstance(GamepadTestDataType type) {
  GamepadButton wgb(true, false, 1.0f);

  GamepadVector wgv;
  memset(&wgv, 0, sizeof(GamepadVector));
  wgv.not_null = true;
  wgv.x = wgv.y = wgv.z = 1.0f;

  GamepadQuaternion wgq;
  memset(&wgq, 0, sizeof(GamepadQuaternion));
  wgq.not_null = true;
  wgq.x = wgq.y = wgq.z = wgq.w = 2.0f;

  GamepadPose wgp;
  memset(&wgp, 0, sizeof(GamepadPose));
  if (type == GamepadPose_Null) {
    wgp.not_null = false;
  } else if (type == GamepadCommon) {
    wgp.not_null = wgp.has_orientation = wgp.has_position = true;
    wgp.orientation = wgq;
    wgp.position = wgv;
    wgp.angular_acceleration = wgv;
  } else if (type == GamepadPose_HasOrientation) {
    wgp.not_null = wgp.has_orientation = true;
    wgp.has_position = false;
    wgp.orientation = wgq;
    wgp.angular_acceleration = wgv;
  } else if (type == GamepadPose_HasPosition) {
    wgp.not_null = wgp.has_position = true;
    wgp.has_orientation = false;
    wgp.position = wgv;
    wgp.angular_acceleration = wgv;
  }

  UChar wch[Gamepad::kMappingLengthCap] = {
      1,    8,    9,     127,   128,   1024,  1025,  1949,
      2047, 2048, 16383, 16384, 20000, 32767, 32768, 65535};

  Gamepad send;
  memset(&send, 0, sizeof(Gamepad));

  send.connected = true;
  for (size_t i = 0; i < Gamepad::kMappingLengthCap; i++) {
    send.id[i] = send.mapping[i] = wch[i];
  }
  send.timestamp = 1234567890123456789ULL;
  send.axes_length = 0U;
  for (size_t i = 0; i < Gamepad::kAxesLengthCap; i++) {
    send.axes_length++;
    send.axes[i] = 1.0;
  }
  send.buttons_length = 0U;
  for (size_t i = 0; i < Gamepad::kButtonsLengthCap; i++) {
    send.buttons_length++;
    send.buttons[i] = wgb;
  }
  send.pose = wgp;
  send.hand = GamepadHand::kRight;
  send.display_id = static_cast<unsigned short>(16);

  return send;
}

bool isWebGamepadButtonEqual(const GamepadButton& lhs,
                             const GamepadButton& rhs) {
  return (lhs.pressed == rhs.pressed && lhs.touched == rhs.touched &&
          lhs.value == rhs.value);
}

bool isWebGamepadVectorEqual(const GamepadVector& lhs,
                             const GamepadVector& rhs) {
  return ((lhs.not_null == false && rhs.not_null == false) ||
          (lhs.not_null == rhs.not_null && lhs.x == rhs.x && lhs.y == rhs.y &&
           lhs.z == rhs.z));
}

bool isWebGamepadQuaternionEqual(const GamepadQuaternion& lhs,
                                 const GamepadQuaternion& rhs) {
  return ((lhs.not_null == false && rhs.not_null == false) ||
          (lhs.not_null == rhs.not_null && lhs.x == rhs.x && lhs.y == rhs.y &&
           lhs.z == rhs.z && lhs.w == rhs.w));
}

bool isWebGamepadPoseEqual(const GamepadPose& lhs, const GamepadPose& rhs) {
  if (lhs.not_null == false && rhs.not_null == false) {
    return true;
  }
  if (lhs.not_null != rhs.not_null ||
      lhs.has_orientation != rhs.has_orientation ||
      lhs.has_position != rhs.has_position ||
      !isWebGamepadVectorEqual(lhs.angular_velocity, rhs.angular_velocity) ||
      !isWebGamepadVectorEqual(lhs.linear_velocity, rhs.linear_velocity) ||
      !isWebGamepadVectorEqual(lhs.angular_acceleration,
                               rhs.angular_acceleration) ||
      !isWebGamepadVectorEqual(lhs.linear_acceleration,
                               rhs.linear_acceleration)) {
    return false;
  }
  if (lhs.has_orientation &&
      !isWebGamepadQuaternionEqual(lhs.orientation, rhs.orientation)) {
    return false;
  }
  if (lhs.has_position &&
      !isWebGamepadVectorEqual(lhs.position, rhs.position)) {
    return false;
  }
  return true;
}

bool isWebGamepadEqual(const Gamepad& send, const Gamepad& echo) {
  if (send.connected != echo.connected || send.timestamp != echo.timestamp ||
      send.axes_length != echo.axes_length ||
      send.buttons_length != echo.buttons_length ||
      !isWebGamepadPoseEqual(send.pose, echo.pose) || send.hand != echo.hand ||
      send.display_id != echo.display_id) {
    return false;
  }
  for (size_t i = 0; i < Gamepad::kIdLengthCap; i++) {
    if (send.id[i] != echo.id[i]) {
      return false;
    }
  }
  for (size_t i = 0; i < Gamepad::kAxesLengthCap; i++) {
    if (send.axes[i] != echo.axes[i]) {
      return false;
    }
  }
  for (size_t i = 0; i < Gamepad::kButtonsLengthCap; i++) {
    if (!isWebGamepadButtonEqual(send.buttons[i], echo.buttons[i])) {
      return false;
    }
  }
  for (size_t i = 0; i < Gamepad::kMappingLengthCap; i++) {
    if (send.mapping[i] != echo.mapping[i]) {
      return false;
    }
  }
  return true;
}

void ExpectWebGamepad(const Gamepad& send,
                      base::OnceClosure closure,
                      const Gamepad& echo) {
  EXPECT_EQ(true, isWebGamepadEqual(send, echo));
  std::move(closure).Run();
}

}  // namespace

class GamepadStructTraitsTest : public testing::Test,
                                public mojom::GamepadStructTraitsTest {
 protected:
  GamepadStructTraitsTest() : binding_(this) {}

  void PassGamepad(const Gamepad& send, PassGamepadCallback callback) override {
    std::move(callback).Run(send);
  }

  mojom::GamepadStructTraitsTestPtr GetGamepadStructTraitsTestProxy() {
    mojom::GamepadStructTraitsTestPtr proxy;
    binding_.Bind(mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  base::MessageLoop message_loop_;
  mojo::Binding<mojom::GamepadStructTraitsTest> binding_;

  DISALLOW_COPY_AND_ASSIGN(GamepadStructTraitsTest);
};

TEST_F(GamepadStructTraitsTest, GamepadCommon) {
  Gamepad send = GetWebGamepadInstance(GamepadCommon);
  base::RunLoop loop;
  mojom::GamepadStructTraitsTestPtr proxy = GetGamepadStructTraitsTestProxy();
  proxy->PassGamepad(
      send, base::BindOnce(&ExpectWebGamepad, send, loop.QuitClosure()));
  loop.Run();
}

TEST_F(GamepadStructTraitsTest, GamepadPose_HasOrientation) {
  Gamepad send = GetWebGamepadInstance(GamepadPose_HasOrientation);
  base::RunLoop loop;
  mojom::GamepadStructTraitsTestPtr proxy = GetGamepadStructTraitsTestProxy();
  proxy->PassGamepad(
      send, base::BindOnce(&ExpectWebGamepad, send, loop.QuitClosure()));
  loop.Run();
}

TEST_F(GamepadStructTraitsTest, GamepadPose_HasPosition) {
  Gamepad send = GetWebGamepadInstance(GamepadPose_HasPosition);
  base::RunLoop loop;
  mojom::GamepadStructTraitsTestPtr proxy = GetGamepadStructTraitsTestProxy();
  proxy->PassGamepad(
      send, base::BindOnce(&ExpectWebGamepad, send, loop.QuitClosure()));
  loop.Run();
}

TEST_F(GamepadStructTraitsTest, GamepadPose_Null) {
  Gamepad send = GetWebGamepadInstance(GamepadPose_Null);
  base::RunLoop loop;
  mojom::GamepadStructTraitsTestPtr proxy = GetGamepadStructTraitsTestProxy();
  proxy->PassGamepad(
      send, base::BindOnce(&ExpectWebGamepad, send, loop.QuitClosure()));
  loop.Run();
}

}  // namespace device
