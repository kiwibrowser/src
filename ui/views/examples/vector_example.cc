// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/vector_example.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace views {
namespace examples {

namespace {

class VectorIconGallery : public View,
                          public TextfieldController,
                          public ButtonListener {
 public:
  VectorIconGallery()
      : image_view_(new ImageView()),
        image_view_container_(new views::View()),
        size_input_(new Textfield()),
        color_input_(new Textfield()),
        file_chooser_(new Textfield()),
        file_go_button_(
            MdTextButton::Create(this, base::ASCIIToUTF16("Render"))),
        // 36dp is one of the natural sizes for MD icons, and corresponds
        // roughly to a 32dp usable area.
        size_(36),
        color_(SK_ColorRED) {
    AddChildView(size_input_);
    AddChildView(color_input_);

    image_view_container_->AddChildView(image_view_);
    auto image_layout = std::make_unique<BoxLayout>(BoxLayout::kHorizontal);
    image_layout->set_cross_axis_alignment(
        BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
    image_layout->set_main_axis_alignment(
        BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
    image_view_container_->SetLayoutManager(std::move(image_layout));
    image_view_->SetBorder(CreateSolidSidedBorder(1, 1, 1, 1, SK_ColorBLACK));
    AddChildView(image_view_container_);

    BoxLayout* box = SetLayoutManager(
        std::make_unique<BoxLayout>(BoxLayout::kVertical, gfx::Insets(10), 10));
    box->SetFlexForView(image_view_container_, 1);

    file_chooser_->set_placeholder_text(
        base::ASCIIToUTF16("Enter a file to read"));
    View* file_container = new View();
    BoxLayout* file_box =
        file_container->SetLayoutManager(std::make_unique<BoxLayout>(
            BoxLayout::kHorizontal, gfx::Insets(10), 10));
    file_container->AddChildView(file_chooser_);
    file_container->AddChildView(file_go_button_);
    file_box->SetFlexForView(file_chooser_, 1);
    AddChildView(file_container);

    size_input_->set_placeholder_text(base::ASCIIToUTF16("Size in dip"));
    size_input_->set_controller(this);
    color_input_->set_placeholder_text(base::ASCIIToUTF16("Color (AARRGGBB)"));
    color_input_->set_controller(this);
  }

  ~VectorIconGallery() override {}

  // TextfieldController implementation.
  void ContentsChanged(Textfield* sender,
                       const base::string16& new_contents) override {
    if (sender == size_input_) {
      if (base::StringToInt(new_contents, &size_) && (size_ > 0))
        Update();
      else
        size_input_->SetText(base::string16());

      return;
    }

    DCHECK_EQ(color_input_, sender);
    if (new_contents.size() != 8u)
      return;
    unsigned new_color =
        strtoul(base::UTF16ToASCII(new_contents).c_str(), nullptr, 16);
    if (new_color <= 0xffffffff) {
      color_ = new_color;
      Update();
    }
  }

  // ButtonListener
  void ButtonPressed(Button* sender, const ui::Event& event) override {
    DCHECK_EQ(file_go_button_, sender);
    base::ScopedAllowBlockingForTesting allow_blocking;
#if defined(OS_POSIX)
    base::FilePath path(base::UTF16ToUTF8(file_chooser_->text()));
#elif defined(OS_WIN)
    base::FilePath path(file_chooser_->text());
#endif
    base::ReadFileToString(path, &contents_);
    // Skip over comments.
    for (size_t slashes = contents_.find("//"); slashes != std::string::npos;
         slashes = contents_.find("//")) {
      size_t eol = contents_.find("\n", slashes);
      contents_.erase(slashes, eol - slashes);
    }
    Update();
  }

 private:
  void Update() {
    if (!contents_.empty()) {
      image_view_->SetImage(
          gfx::CreateVectorIconFromSource(contents_, size_, color_));
    }
    Layout();
  }

  ImageView* image_view_;
  View* image_view_container_;
  Textfield* size_input_;
  Textfield* color_input_;
  Textfield* file_chooser_;
  Button* file_go_button_;
  std::string contents_;

  int size_;
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(VectorIconGallery);
};

}  // namespace

VectorExample::VectorExample() : ExampleBase("Vector Icon") {}

VectorExample::~VectorExample() {}

void VectorExample::CreateExampleView(View* container) {
  container->SetLayoutManager(std::make_unique<FillLayout>());
  container->AddChildView(new VectorIconGallery());
}

}  // namespace examples
}  // namespace views
