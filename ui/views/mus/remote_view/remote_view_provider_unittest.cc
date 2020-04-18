// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/remote_view/remote_view_provider.h"

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/unguessable_token.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/aura/window.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/mus/remote_view/remote_view_provider_test_api.h"

namespace views {

class RemoteViewProviderTest : public aura::test::AuraTestBase {
 public:
  RemoteViewProviderTest() = default;
  ~RemoteViewProviderTest() override = default;

  // aura::test::AuraTestBase
  void SetUp() override {
    EnableMusWithTestWindowTree();
    AuraTestBase::SetUp();

    test::RemoteViewProviderTestApi::SetWindowTreeClient(
        window_tree_client_impl());

    embedded_ = std::make_unique<aura::Window>(nullptr);
    embedded_->set_owned_by_parent(false);
    embedded_->Init(ui::LAYER_NOT_DRAWN);
    embedded_->SetBounds(gfx::Rect(100, 50));

    provider_ = std::make_unique<RemoteViewProvider>(embedded_.get());
  }

  void TearDown() override {
    // EmbedRoot in |provider_| must be released before WindowTreeClient.
    provider_.reset();

    AuraTestBase::TearDown();
  }

  // Gets the embed token and waits for it.
  base::UnguessableToken GetEmbedToken() {
    base::RunLoop run_loop;
    base::UnguessableToken token;
    provider_->GetEmbedToken(base::BindOnce(
        [](base::RunLoop* run_loop, base::UnguessableToken* out_token,
           const base::UnguessableToken& token) {
          *out_token = token;
          run_loop->Quit();
        },
        &run_loop, &token));
    run_loop.Run();
    return token;
  }

  // Simulates EmbedUsingToken call on embedder side and waits for
  // WindowTreeClient to create a local embedder window.
  aura::Window* SimulateEmbedUsingTokenAndGetEmbedder(
      const base::UnguessableToken& token) {
    base::RunLoop run_loop;
    aura::Window* embedder = nullptr;
    provider_->SetCallbacks(
        base::BindRepeating(
            [](base::RunLoop* run_loop, aura::Window** out_embedder,
               aura::Window* embedder) {
              *out_embedder = embedder;
              run_loop->Quit();
            },
            &run_loop, &embedder),
        base::DoNothing() /* OnUnembedCallback */);
    window_tree()->AddEmbedRootForToken(token);
    run_loop.Run();
    return embedder;
  }

  // Helper to simulate embed.
  aura::Window* SimulateEmbed() {
    base::UnguessableToken token = GetEmbedToken();
    if (token.is_empty()) {
      ADD_FAILURE() << "Failed to get embed token.";
      return nullptr;
    }

    return SimulateEmbedUsingTokenAndGetEmbedder(token);
  }

  // Simulates the embedder window close.
  void SimulateEmbedderClose(aura::Window* embedder) {
    base::RunLoop run_loop;
    provider_->SetCallbacks(
        base::DoNothing() /* OnEmbedCallback */,
        base::BindRepeating([](base::RunLoop* run_loop) { run_loop->Quit(); },
                            &run_loop));

    const ui::Id embedder_window_id =
        aura::WindowMus::Get(embedder)->server_id();
    window_tree()->RemoveEmbedderWindow(embedder_window_id);
    run_loop.Run();
  }

 protected:
  std::unique_ptr<aura::Window> embedded_;
  std::unique_ptr<RemoteViewProvider> provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoteViewProviderTest);
};

// Tests the basics on the embedded client.
TEST_F(RemoteViewProviderTest, Embed) {
  aura::Window* embedder = SimulateEmbed();
  ASSERT_TRUE(embedder);

  // |embedded_| has the same non-empty size with |embedder| after embed.
  EXPECT_EQ(embedded_->bounds().size(), embedder->bounds().size());
  EXPECT_FALSE(embedded_->bounds().IsEmpty());
  EXPECT_FALSE(embedder->bounds().IsEmpty());

  // |embedded_| resizes with |embedder|.
  const gfx::Rect new_bounds(embedder->bounds().width() + 100,
                             embedder->bounds().height() + 50);
  embedder->SetBounds(new_bounds);
  EXPECT_EQ(embedded_->bounds().size(), embedder->bounds().size());
  EXPECT_FALSE(embedded_->bounds().IsEmpty());
  EXPECT_FALSE(embedder->bounds().IsEmpty());
}

// Tests when |embedded_| is released first.
TEST_F(RemoteViewProviderTest, EmbeddedReleasedFirst) {
  SimulateEmbed();
  embedded_.reset();
}

// Tests when |provider_| is released first.
TEST_F(RemoteViewProviderTest, ClientReleasedFirst) {
  SimulateEmbed();
  provider_.reset();
}

// Tests when embedder goes away first.
TEST_F(RemoteViewProviderTest, EmbedderReleasedFirst) {
  aura::Window* embedder = SimulateEmbed();
  ASSERT_TRUE(embedder);

  SimulateEmbedderClose(embedder);
}

// Tests that the client can embed again.
TEST_F(RemoteViewProviderTest, EmbedAgain) {
  aura::Window* embedder = SimulateEmbed();
  ASSERT_TRUE(embedder);

  SimulateEmbedderClose(embedder);

  aura::Window* new_embedder = SimulateEmbed();
  ASSERT_TRUE(new_embedder);
  EXPECT_NE(new_embedder, embedder);
}

}  // namespace views
