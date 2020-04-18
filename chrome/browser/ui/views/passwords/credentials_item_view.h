// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PASSWORDS_CREDENTIALS_ITEM_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PASSWORDS_CREDENTIALS_ITEM_VIEW_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/passwords/account_avatar_fetcher.h"
#include "ui/views/controls/button/button.h"

namespace autofill {
struct PasswordForm;
}

namespace gfx {
class ImageSkia;
}

namespace network {
namespace mojom {
class URLLoaderFactory;
}
}

namespace views {
class ImageView;
class Label;
}

// CredentialsItemView represents a credential view in the account chooser
// bubble.
class CredentialsItemView : public AccountAvatarFetcherDelegate,
                            public views::Button {
 public:
  CredentialsItemView(views::ButtonListener* button_listener,
                      const base::string16& upper_text,
                      const base::string16& lower_text,
                      SkColor hover_color,
                      const autofill::PasswordForm* form,
                      network::mojom::URLLoaderFactory* loader_factory);
  ~CredentialsItemView() override;

  const autofill::PasswordForm* form() const { return form_; }

  // AccountAvatarFetcherDelegate:
  void UpdateAvatar(const gfx::ImageSkia& image) override;

  void SetLowerLabelColor(SkColor color);
  void SetHoverColor(SkColor color);

  int GetPreferredHeight() const;

 private:
  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  int GetHeightForWidth(int w) const override;
  void Layout() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

  const autofill::PasswordForm* form_;

  views::ImageView* image_view_;
  views::Label* upper_label_;
  views::Label* lower_label_;
  views::ImageView* info_icon_;

  SkColor hover_color_;

  base::WeakPtrFactory<CredentialsItemView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CredentialsItemView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PASSWORDS_CREDENTIALS_ITEM_VIEW_H_
