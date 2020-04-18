// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "ash/note_taking_controller.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/system/palette/mock_palette_tool_delegate.h"
#include "ash/system/palette/palette_ids.h"
#include "ash/system/palette/palette_tool.h"
#include "ash/system/palette/tools/create_note_action.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ui/views/view.h"

namespace ash {

class TestNoteTakingControllerClient
    : public ash::mojom::NoteTakingControllerClient {
 public:
  TestNoteTakingControllerClient() : binding_(this) {}
  ~TestNoteTakingControllerClient() override = default;

  void Attach() {
    DCHECK(!controller_);
    Shell::Get()->note_taking_controller()->BindRequest(
        mojo::MakeRequest(&controller_));
    ash::mojom::NoteTakingControllerClientPtr client;
    binding_.Bind(mojo::MakeRequest(&client));
    controller_->SetClient(std::move(client));
    controller_.FlushForTesting();
  }

  void Detach() {
    DCHECK(controller_);
    controller_ = nullptr;
    DCHECK(binding_.is_bound());
    binding_.Close();
    FlushClientMojo();
  }

  int GetCreateNoteCount() {
    FlushClientMojo();
    return create_note_count_;
  }

  // ash::mojom::NoteTakingControllerClient:
  void CreateNote() override { create_note_count_++; }

 private:
  void FlushClientMojo() {
    Shell::Get()->note_taking_controller()->FlushMojoForTesting();
  }

  int create_note_count_ = 0;

  mojo::Binding<ash::mojom::NoteTakingControllerClient> binding_;
  ash::mojom::NoteTakingControllerPtr controller_;

  DISALLOW_COPY_AND_ASSIGN(TestNoteTakingControllerClient);
};

namespace {

// Base class for all create note ash tests.
class CreateNoteTest : public AshTestBase {
 public:
  CreateNoteTest() = default;
  ~CreateNoteTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    palette_tool_delegate_ = std::make_unique<MockPaletteToolDelegate>();
    tool_ = std::make_unique<CreateNoteAction>(palette_tool_delegate_.get());
    note_taking_client_ = std::make_unique<TestNoteTakingControllerClient>();
  }

 protected:
  std::unique_ptr<MockPaletteToolDelegate> palette_tool_delegate_;
  std::unique_ptr<PaletteTool> tool_;
  std::unique_ptr<TestNoteTakingControllerClient> note_taking_client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CreateNoteTest);
};

}  // namespace

// The note tool is only visible when there is a note-taking app available.
TEST_F(CreateNoteTest, ViewOnlyCreatedWhenNoteAppIsAvailable) {
  EXPECT_FALSE(tool_->CreateView());
  tool_->OnViewDestroyed();

  note_taking_client_->Attach();
  std::unique_ptr<views::View> view = base::WrapUnique(tool_->CreateView());
  EXPECT_TRUE(view);
  tool_->OnViewDestroyed();

  note_taking_client_->Detach();
  EXPECT_FALSE(tool_->CreateView());
  tool_->OnViewDestroyed();
}

// Activating the note tool both creates a note on the client and also
// disables the tool and hides the palette.
TEST_F(CreateNoteTest, EnablingToolCreatesNewNoteAndDisablesTool) {
  note_taking_client_->Attach();
  std::unique_ptr<views::View> view = base::WrapUnique(tool_->CreateView());

  EXPECT_CALL(*palette_tool_delegate_.get(),
              DisableTool(PaletteToolId::CREATE_NOTE));
  EXPECT_CALL(*palette_tool_delegate_.get(), HidePalette());

  tool_->OnEnable();
  EXPECT_EQ(1, note_taking_client_->GetCreateNoteCount());
}

}  // namespace ash
