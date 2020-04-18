// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/renderer_save_password_progress_logger.h"

#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "components/autofill/content/common/autofill_driver.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

namespace {

const char kTestText[] = "test";

class FakeContentPasswordManagerDriver : public mojom::PasswordManagerDriver {
 public:
  FakeContentPasswordManagerDriver()
      : called_record_save_(false), binding_(this) {}
  ~FakeContentPasswordManagerDriver() override {}

  mojom::PasswordManagerDriverPtr CreateInterfacePtrAndBind() {
    mojom::PasswordManagerDriverPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  bool GetLogMessage(std::string* log) {
    if (!called_record_save_)
      return false;

    EXPECT_TRUE(log_);
    *log = *log_;
    return true;
  }

 private:
  // autofill::mojom::PasswordManagerDriver:
  void PasswordFormsParsed(
      const std::vector<autofill::PasswordForm>& forms) override {}

  void PasswordFormsRendered(
      const std::vector<autofill::PasswordForm>& visible_forms,
      bool did_stop_loading) override {}

  void PasswordFormSubmitted(
      const autofill::PasswordForm& password_form) override {}

  void ShowManualFallbackForSaving(
      const autofill::PasswordForm& password_form) override {}

  void HideManualFallbackForSaving() override {}

  void SameDocumentNavigation(
      const autofill::PasswordForm& password_form) override {}

  void PresaveGeneratedPassword(
      const autofill::PasswordForm& password_form) override {}

  void PasswordNoLongerGenerated(
      const autofill::PasswordForm& password_form) override {}

  void ShowPasswordSuggestions(int key,
                               base::i18n::TextDirection text_direction,
                               const base::string16& typed_username,
                               int options,
                               const gfx::RectF& bounds) override {}

  void ShowManualFallbackSuggestion(base::i18n::TextDirection text_direction,
                                    const gfx::RectF& bounds) override {}

  void RecordSavePasswordProgress(const std::string& log) override {
    called_record_save_ = true;
    log_ = log;
  }

  void UserModifiedPasswordField() override {}

  void SaveGenerationFieldDetectedByClassifier(
      const autofill::PasswordForm& password_form,
      const base::string16& generation_field) override {}

  void CheckSafeBrowsingReputation(const GURL& form_action,
                                   const GURL& frame_url) override {}

  // Records whether RecordSavePasswordProgress() gets called.
  bool called_record_save_;
  // Records data received via RecordSavePasswordProgress() call.
  base::Optional<std::string> log_;

  mojo::Binding<mojom::PasswordManagerDriver> binding_;
};

class TestLogger : public RendererSavePasswordProgressLogger {
 public:
  TestLogger(mojom::PasswordManagerDriver* driver)
      : RendererSavePasswordProgressLogger(driver) {}

  using RendererSavePasswordProgressLogger::SendLog;
};

}  // namespace

TEST(RendererSavePasswordProgressLoggerTest, SendLog) {
  base::MessageLoop loop;
  FakeContentPasswordManagerDriver fake_driver;
  mojom::PasswordManagerDriverPtr driver_ptr =
      fake_driver.CreateInterfacePtrAndBind();
  TestLogger logger(driver_ptr.get());
  logger.SendLog(kTestText);

  base::RunLoop().RunUntilIdle();
  std::string sent_log;
  EXPECT_TRUE(fake_driver.GetLogMessage(&sent_log));
  EXPECT_EQ(kTestText, sent_log);
}

}  // namespace autofill
