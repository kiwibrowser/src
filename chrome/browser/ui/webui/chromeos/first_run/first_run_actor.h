// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_FIRST_RUN_FIRST_RUN_ACTOR_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_FIRST_RUN_FIRST_RUN_ACTOR_H_

#include <memory>
#include <string>


namespace base {
class DictionaryValue;
}

namespace chromeos {

class FirstRunActor {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called after actor was initialized.
    virtual void OnActorInitialized() = 0;

    // Called when user clicked "Next" button in step with name |step_name|.
    virtual void OnNextButtonClicked(const std::string& step_name) = 0;

    // Called when user clicked "Keep exploring" button.
    virtual void OnHelpButtonClicked() = 0;

    // Called after step with |step_name| has been shown.
    virtual void OnStepShown(const std::string& step_name) = 0;

    // Called after step with |step_name| has been shown.
    virtual void OnStepHidden(const std::string& step_name) = 0;

    // Called in answer to Finalize() call.
    virtual void OnActorFinalized() = 0;

    // Notifies about about actor destruction.
    virtual void OnActorDestroyed() = 0;
  };

  class StepPosition {
   public:
    // Initializes fields in "non-set" state.
    StepPosition();

    // Setters for properties. Return |*this|.
    StepPosition& SetTop(int top);
    StepPosition& SetRight(int right);
    StepPosition& SetBottom(int bottom);
    StepPosition& SetLeft(int left);

    // Returns DictionaryValue containing set properties.
    base::DictionaryValue AsValue() const;

   private:
    int top_;
    int right_;
    int bottom_;
    int left_;
  };

  FirstRunActor();
  virtual ~FirstRunActor();

  // Returns |true| if actor is initialized. Other public methods can be called
  // only if |IsInitialized| returns |true|.
  virtual bool IsInitialized() = 0;

  // Changes background visibility.
  virtual void SetBackgroundVisible(bool visible) = 0;

  // Adds rectangular hole to background with given position and dimensions.
  virtual void AddRectangularHole(int x, int y, int width, int height) = 0;

  // Adds round hole to background with given position and dimensions.
  virtual void AddRoundHole(int x, int y, float radius) = 0;

  // Removes all holes from background.
  virtual void RemoveBackgroundHoles() = 0;

  // Shows step with given name and position.
  virtual void ShowStepPositioned(const std::string& name,
                                  const StepPosition& position) = 0;

  // Shows step with given name that points to given point.
  virtual void ShowStepPointingTo(const std::string& name,
                                  int x,
                                  int y,
                                  int offset) = 0;

  // Hides currently shown step.
  virtual void HideCurrentStep() = 0;

  // Hides all the UI.
  virtual void Finalize() = 0;

  // Whether actor is finalizing now.
  virtual bool IsFinalizing() = 0;

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() const { return delegate_; }

 private:
  Delegate* delegate_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_FIRST_RUN_FIRST_RUN_ACTOR_H_

