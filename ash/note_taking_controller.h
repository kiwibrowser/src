// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_NOTETAKING_CONTROLLER_H_
#define ASH_NOTETAKING_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/note_taking_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

// Controller for the note taking functionality.
class ASH_EXPORT NoteTakingController : public mojom::NoteTakingController {
 public:
  NoteTakingController();
  ~NoteTakingController() override;

  void BindRequest(mojom::NoteTakingControllerRequest request);

  // mojom::NoteTakingController:
  void SetClient(mojom::NoteTakingControllerClientPtr client) override;

  // Returns true if the client is attached.
  bool CanCreateNote() const;

  // Calls the method of the same name on |client_|.
  void CreateNote();

 private:
  friend class TestNoteTakingControllerClient;

  void OnClientConnectionLost();

  void FlushMojoForTesting();

  // Binding for mojom::NoteTakingController interface.
  mojo::Binding<ash::mojom::NoteTakingController> binding_;

  // Interface to NoteTaking controller client (chrome).
  mojom::NoteTakingControllerClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(NoteTakingController);
};

}  // namespace ash

#endif  // ASH_NOTETAKING_CONTROLLER_H_
