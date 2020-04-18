// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PICTURE_IN_PICTURE_PICTURE_IN_PICTURE_CONTROLLER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PICTURE_IN_PICTURE_PICTURE_IN_PICTURE_CONTROLLER_IMPL_H_

#include "third_party/blink/renderer/core/frame/picture_in_picture_controller.h"

namespace blink {

class HTMLVideoElement;
class PictureInPictureWindow;

// The PictureInPictureControllerImpl is keeping the state and implementing the
// logic around the Picture-in-Picture feature. It is meant to be used as well
// by the Picture-in-Picture Web API and internally (eg. media controls). All
// consumers inside Blink modules/ should use this class to access
// Picture-in-Picture. In core/, they should use PictureInPictureController.
// PictureInPictureControllerImpl instance is associated to a Document. It is
// supplement and therefore can be lazy-initiated. Callers should consider
// whether they want to instantiate an object when they make a call.
class PictureInPictureControllerImpl : public PictureInPictureController {
  USING_GARBAGE_COLLECTED_MIXIN(PictureInPictureControllerImpl);
  WTF_MAKE_NONCOPYABLE(PictureInPictureControllerImpl);

 public:
  ~PictureInPictureControllerImpl() override;

  // Meant to be called internally by PictureInPictureController::From()
  // through ModulesInitializer.
  static PictureInPictureControllerImpl* Create(Document&);

  // Gets, or creates, PictureInPictureControllerImpl supplement on Document.
  // Should be called before any other call to make sure a document is attached.
  static PictureInPictureControllerImpl& From(Document&);

  // Returns whether system allows Picture-in-Picture feature or not for
  // the associated document.
  bool PictureInPictureEnabled() const;

  // Returns whether the document associated with the controller is allowed to
  // request Picture-in-Picture.
  Status IsDocumentAllowed() const;

  // Enter Picture-in-Picture for a video element and resolve promise.
  void EnterPictureInPicture(HTMLVideoElement*, ScriptPromiseResolver*);

  // Meant to be called internally when an element has entered successfully
  // Picture-in-Picture.
  void OnEnteredPictureInPicture(HTMLVideoElement*,
                                 ScriptPromiseResolver*,
                                 const WebSize& picture_in_picture_window_size);

  // Exit Picture-in-Picture for a video element and resolve promise if any.
  void ExitPictureInPicture(HTMLVideoElement*, ScriptPromiseResolver*);

  // Returns element currently in Picture-in-Picture if any. Null otherwise.
  Element* PictureInPictureElement(TreeScope&) const;

  // Implementation of PictureInPictureController.
  void OnExitedPictureInPicture(ScriptPromiseResolver*) override;
  Status IsElementAllowed(const HTMLVideoElement&) const override;
  bool IsPictureInPictureElement(const Element*) const override;

  void Trace(blink::Visitor*) override;

 private:
  explicit PictureInPictureControllerImpl(Document&);

  // The Picture-in-Picture element for the associated document.
  Member<HTMLVideoElement> picture_in_picture_element_;

  // The Picture-in-Picture window for the associated document.
  Member<PictureInPictureWindow> picture_in_picture_window_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PICTURE_IN_PICTURE_PICTURE_IN_PICTURE_CONTROLLER_IMPL_H_
