// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_MEDIA_GALLERY_UTIL_PUBLIC_CPP_MEDIA_PARSER_PROVIDER_H_
#define CHROME_SERVICES_MEDIA_GALLERY_UTIL_PUBLIC_CPP_MEDIA_PARSER_PROVIDER_H_

#include "base/macros.h"
#include "chrome/services/media_gallery_util/public/mojom/media_parser.mojom.h"

namespace service_manager {
class Connector;
}

// Base class used by SafeMediaMetadataParser and SafeAudioVideoChecker to
// retrieve a MediaParserPtr.
class MediaParserProvider {
 public:
  MediaParserProvider();
  virtual ~MediaParserProvider();

 protected:
  // Retrieves the MediaParserPtr. OnMediaParserCreated() is called when the
  // media parser is available.
  void RetrieveMediaParser(service_manager::Connector* connector);

  // Invoked when the media parser was successfully created. It can then be
  // obtained by calling media_parser() and is guaranteed to be non null.
  virtual void OnMediaParserCreated() = 0;

  // Invoked when there was an error with the connection to the media gallerie
  // util service. When this call happens, it means any pending callback
  // expected from media_parser() will not happen.
  virtual void OnConnectionError() = 0;

  chrome::mojom::MediaParser* media_parser() const {
    return media_parser_ptr_.get();
  }

  // Clears all interface ptr to the media gallery service.
  void ResetMediaParser();

 private:
  void OnMediaParserCreatedImpl(chrome::mojom::MediaParserPtr media_parser_ptr);

  chrome::mojom::MediaParserFactoryPtr media_parser_factory_ptr_;
  chrome::mojom::MediaParserPtr media_parser_ptr_;

  DISALLOW_COPY_AND_ASSIGN(MediaParserProvider);
};

#endif  // CHROME_SERVICES_MEDIA_GALLERY_UTIL_PUBLIC_CPP_MEDIA_PARSER_PROVIDER_H_
