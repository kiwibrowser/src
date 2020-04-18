// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/accelerators/accelerator_manager.h"

#include <map>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator_manager_delegate.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ui {
namespace test {

namespace {

class TestTarget : public AcceleratorTarget {
 public:
  TestTarget() = default;
  ~TestTarget() override = default;

  int accelerator_pressed_count() const {
    return accelerator_pressed_count_;
  }

  void set_accelerator_pressed_count(int accelerator_pressed_count) {
    accelerator_pressed_count_ = accelerator_pressed_count;
  }

  // Overridden from AcceleratorTarget:
  bool AcceleratorPressed(const Accelerator& accelerator) override;
  bool CanHandleAccelerators() const override;

 private:
  int accelerator_pressed_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestTarget);
};

bool TestTarget::AcceleratorPressed(const Accelerator& accelerator) {
  ++accelerator_pressed_count_;
  return true;
}

bool TestTarget::CanHandleAccelerators() const {
  return true;
}

Accelerator GetAccelerator(KeyboardCode code, int mask) {
  return Accelerator(code, mask);
}

// Possible flags used for accelerators.
const int kAcceleratorModifiers[] = {EF_SHIFT_DOWN, EF_CONTROL_DOWN,
                                     EF_ALT_DOWN, EF_COMMAND_DOWN};

// Returns a set of flags from id, where id is a bitmask into
// kAcceleratorModifiers used to determine which flags are set.
int BuildAcceleratorModifier(int id) {
  int result = 0;
  for (size_t i = 0; i < arraysize(kAcceleratorModifiers); ++i) {
    if (((1 << i) & id) != 0)
      result |= kAcceleratorModifiers[i];
  }
  return result;
}

// AcceleratorManagerDelegate implementation that records calls to interface
// using the following format.
// . OnAcceleratorsRegistered() -> A list of "'Register ' + <id>" separated by
// whitespaces.
// . OnAcceleratorUnregistered() -> 'Unregister' + id
// where the id is specified using SetIdForAccelerator().
class TestAcceleratorManagerDelegate : public AcceleratorManagerDelegate {
 public:
  TestAcceleratorManagerDelegate() {}
  ~TestAcceleratorManagerDelegate() override {}

  void SetIdForAccelerator(const Accelerator& accelerator,
                           const std::string& id) {
    accelerator_to_id_[accelerator] = id;
  }

  std::string GetAndClearCommands() {
    std::string commands;
    std::swap(commands, commands_);
    return commands;
  }

  // AcceleratorManagerDelegate:
  void OnAcceleratorsRegistered(
      const std::vector<Accelerator>& accelerators) override {
    for (const Accelerator& accelerator : accelerators) {
      if (!commands_.empty())
        commands_ += " ";
      commands_ += "Register " + accelerator_to_id_[accelerator];
    }
  }
  void OnAcceleratorUnregistered(const Accelerator& accelerator) override {
    if (!commands_.empty())
      commands_ += " ";
    commands_ += "Unregister " + accelerator_to_id_[accelerator];
  }

 private:
  std::map<Accelerator, std::string> accelerator_to_id_;
  std::string commands_;

  DISALLOW_COPY_AND_ASSIGN(TestAcceleratorManagerDelegate);
};

}  // namespace

class AcceleratorManagerTest : public testing::Test {
 public:
  AcceleratorManagerTest() : manager_(&delegate_) {}
  ~AcceleratorManagerTest() override {}

 protected:
  TestAcceleratorManagerDelegate delegate_;
  AcceleratorManager manager_;
};

TEST_F(AcceleratorManagerTest, Register) {
  TestTarget target;
  const Accelerator accelerator_a(VKEY_A, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_a, "a");

  const Accelerator accelerator_b(VKEY_B, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_b, "b");

  const Accelerator accelerator_c(VKEY_C, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_c, "c");

  const Accelerator accelerator_d(VKEY_D, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_d, "d");

  manager_.Register(
      {accelerator_a, accelerator_b, accelerator_c, accelerator_d},
      AcceleratorManager::kNormalPriority, &target);
  EXPECT_EQ("Register a Register b Register c Register d",
            delegate_.GetAndClearCommands());

  // The registered accelerators are processed.
  EXPECT_TRUE(manager_.Process(accelerator_a));
  EXPECT_TRUE(manager_.Process(accelerator_b));
  EXPECT_TRUE(manager_.Process(accelerator_c));
  EXPECT_TRUE(manager_.Process(accelerator_d));
  EXPECT_EQ(4, target.accelerator_pressed_count());
}

TEST_F(AcceleratorManagerTest, RegisterMultipleTarget) {
  const Accelerator accelerator_a(VKEY_A, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_a, "a");
  TestTarget target1;
  manager_.Register({accelerator_a}, AcceleratorManager::kNormalPriority,
                    &target1);
  EXPECT_EQ("Register a", delegate_.GetAndClearCommands());
  TestTarget target2;
  manager_.Register({accelerator_a}, AcceleratorManager::kNormalPriority,
                    &target2);
  // Registering the same command shouldn't notify the delegate.
  EXPECT_TRUE(delegate_.GetAndClearCommands().empty());

  // If multiple targets are registered with the same accelerator, the target
  // registered later processes the accelerator.
  EXPECT_TRUE(manager_.Process(accelerator_a));
  EXPECT_EQ(0, target1.accelerator_pressed_count());
  EXPECT_EQ(1, target2.accelerator_pressed_count());
}

TEST_F(AcceleratorManagerTest, Unregister) {
  const Accelerator accelerator_a(VKEY_A, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_a, "a");
  TestTarget target;
  const Accelerator accelerator_b(VKEY_B, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_b, "b");
  manager_.Register({accelerator_a, accelerator_b},
                    AcceleratorManager::kNormalPriority, &target);
  EXPECT_EQ("Register a Register b", delegate_.GetAndClearCommands());

  // Unregistering a different accelerator does not affect the other
  // accelerator.
  manager_.Unregister(accelerator_b, &target);
  EXPECT_EQ("Unregister b", delegate_.GetAndClearCommands());
  EXPECT_TRUE(manager_.Process(accelerator_a));
  EXPECT_EQ(1, target.accelerator_pressed_count());

  // The unregistered accelerator is no longer processed.
  target.set_accelerator_pressed_count(0);
  manager_.Unregister(accelerator_a, &target);
  EXPECT_EQ("Unregister a", delegate_.GetAndClearCommands());
  EXPECT_FALSE(manager_.Process(accelerator_a));
  EXPECT_EQ(0, target.accelerator_pressed_count());
}

TEST_F(AcceleratorManagerTest, UnregisterAll) {
  const Accelerator accelerator_a(VKEY_A, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_a, "a");
  TestTarget target1;
  const Accelerator accelerator_b(VKEY_B, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_b, "b");
  manager_.Register({accelerator_a, accelerator_b},
                    AcceleratorManager::kNormalPriority, &target1);
  const Accelerator accelerator_c(VKEY_C, EF_NONE);
  delegate_.SetIdForAccelerator(accelerator_c, "c");
  TestTarget target2;
  manager_.Register({accelerator_c}, AcceleratorManager::kNormalPriority,
                    &target2);
  EXPECT_EQ("Register a Register b Register c",
            delegate_.GetAndClearCommands());
  manager_.UnregisterAll(&target1);
  {
    const std::string commands = delegate_.GetAndClearCommands();
    // Ordering is not guaranteed.
    EXPECT_TRUE(commands == "Unregister a Unregister b" ||
                commands == "Unregister b Unregister a");
  }

  // All the accelerators registered for |target1| are no longer processed.
  EXPECT_FALSE(manager_.Process(accelerator_a));
  EXPECT_FALSE(manager_.Process(accelerator_b));
  EXPECT_EQ(0, target1.accelerator_pressed_count());

  // UnregisterAll with a different target does not affect the other target.
  EXPECT_TRUE(manager_.Process(accelerator_c));
  EXPECT_EQ(1, target2.accelerator_pressed_count());
}

TEST_F(AcceleratorManagerTest, Process) {
  TestTarget target;

  // Test all cases of possible modifiers.
  for (size_t i = 0; i < (1 << arraysize(kAcceleratorModifiers)); ++i) {
    const int modifiers = BuildAcceleratorModifier(i);
    Accelerator accelerator(GetAccelerator(VKEY_A, modifiers));
    manager_.Register({accelerator}, AcceleratorManager::kNormalPriority,
                      &target);

    // The registered accelerator is processed.
    const int last_count = target.accelerator_pressed_count();
    EXPECT_TRUE(manager_.Process(accelerator)) << i;
    EXPECT_EQ(last_count + 1, target.accelerator_pressed_count()) << i;

    // The non-registered accelerators are not processed.
    accelerator.set_key_state(Accelerator::KeyState::RELEASED);
    EXPECT_FALSE(manager_.Process(accelerator)) << i;  // different type

    EXPECT_FALSE(manager_.Process(GetAccelerator(VKEY_UNKNOWN, modifiers)))
        << i;  // different vkey
    EXPECT_FALSE(manager_.Process(GetAccelerator(VKEY_B, modifiers)))
        << i;  // different vkey
    EXPECT_FALSE(manager_.Process(GetAccelerator(VKEY_SHIFT, modifiers)))
        << i;  // different vkey

    for (size_t test_i = 0; test_i < (1 << arraysize(kAcceleratorModifiers));
         ++test_i) {
      if (test_i == i)
        continue;
      const int test_modifiers = BuildAcceleratorModifier(test_i);
      const Accelerator test_accelerator(
          GetAccelerator(VKEY_A, test_modifiers));
      EXPECT_FALSE(manager_.Process(test_accelerator)) << " i=" << i
                                                       << " test_i=" << test_i;
    }

    EXPECT_EQ(last_count + 1, target.accelerator_pressed_count()) << i;
    manager_.UnregisterAll(&target);
  }
}

// Verifies delegate is notifed correctly when unregistering and registering
// with the same accelerator.
TEST_F(AcceleratorManagerTest, Reregister) {
  const Accelerator accelerator_a(VKEY_A, EF_NONE);
  TestTarget target;
  delegate_.SetIdForAccelerator(accelerator_a, "a");
  manager_.Register({accelerator_a}, AcceleratorManager::kNormalPriority,
                    &target);
  EXPECT_EQ("Register a", delegate_.GetAndClearCommands());
  manager_.UnregisterAll(&target);
  EXPECT_EQ("Unregister a", delegate_.GetAndClearCommands());
  manager_.Register({accelerator_a}, AcceleratorManager::kNormalPriority,
                    &target);
  EXPECT_EQ("Register a", delegate_.GetAndClearCommands());
}

}  // namespace test
}  // namespace ui
