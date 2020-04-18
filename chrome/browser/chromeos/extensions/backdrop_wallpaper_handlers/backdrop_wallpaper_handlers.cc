// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/backdrop_wallpaper_handlers/backdrop_wallpaper_handlers.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/extensions/backdrop_wallpaper_handlers/backdrop_wallpaper.pb.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/extensions/api/wallpaper_private.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "url/gurl.h"

namespace {

// The MIME type of the POST data sent to the server.
constexpr char kProtoMimeType[] = "application/x-protobuf";

// The url to download the proto of the complete list of wallpaper collections.
constexpr char kBackdropCollectionsUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collections?rt=b";

// The url to download the proto of a specific wallpaper collection.
constexpr char kBackdropImagesUrl[] =
    "https://clients3.google.com/cast/chromecast/home/wallpaper/"
    "collection-images?rt=b";

}  // namespace

namespace backdrop_wallpaper_handlers {

CollectionInfoFetcher::CollectionInfoFetcher() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

CollectionInfoFetcher::~CollectionInfoFetcher() = default;

void CollectionInfoFetcher::Start(OnCollectionsInfoFetched callback) {
  DCHECK(!simple_loader_ && callback_.is_null());
  callback_ = std::move(callback);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("backdrop_collection_names_download",
                                          R"(
        semantics {
          sender: "ChromeOS Wallpaper Picker"
          description:
            "The ChromeOS Wallpaper Picker extension displays a rich set of "
            "wallpapers for users to choose from. Each wallpaper belongs to a "
            "collection (e.g. Arts, Landscape etc.). The list of all available "
            "collections is downloaded from the Backdrop wallpaper service."
          trigger:
            "When ChromeOS Wallpaper Picker extension is open, and "
            "GOOGLE_CHROME_BUILD is defined."
          data:
            "The Backdrop protocol buffer messages. No user data is included."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "NA"
          policy_exception_justification:
            "Not implemented, considered not necessary."
        })");

  SystemNetworkContextManager* system_network_context_manager =
      g_browser_process->system_network_context_manager();
  // In unit tests, the browser process can return a null context manager
  if (!system_network_context_manager)
    return;

  network::mojom::URLLoaderFactory* loader_factory =
      system_network_context_manager->GetURLLoaderFactory();

  backdrop::GetCollectionsRequest request;
  // The language field may include the country code (e.g. "en-US").
  request.set_language(g_browser_process->GetApplicationLocale());
  std::string serialized_proto;
  request.SerializeToString(&serialized_proto);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kBackdropCollectionsUrl);
  resource_request->method = "POST";
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DO_NOT_SEND_COOKIES |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

  simple_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                    traffic_annotation);
  simple_loader_->AttachStringForUpload(serialized_proto, kProtoMimeType);
  simple_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory, base::BindOnce(&CollectionInfoFetcher::OnURLFetchComplete,
                                     base::Unretained(this)));
}

void CollectionInfoFetcher::OnURLFetchComplete(
    const std::unique_ptr<std::string> response_body) {
  std::vector<extensions::api::wallpaper_private::CollectionInfo>
      collections_info_list;

  if (!response_body) {
    int response_code = -1;
    if (simple_loader_->ResponseInfo() &&
        simple_loader_->ResponseInfo()->headers)
      response_code = simple_loader_->ResponseInfo()->headers->response_code();

    // TODO(crbug.com/800945): Adds retry mechanism and error handling.
    LOG(ERROR) << "Downloading Backdrop wallpaper proto for collection info "
                  "failed with error code: "
               << response_code;
    simple_loader_.reset();
    std::move(callback_).Run(false /*success=*/, collections_info_list);
    return;
  }

  backdrop::GetCollectionsResponse collections_response;
  if (!collections_response.ParseFromString(*response_body)) {
    LOG(ERROR) << "Deserializing Backdrop wallpaper proto for collection info "
                  "failed.";
    simple_loader_.reset();
    std::move(callback_).Run(false /*success=*/, collections_info_list);
    return;
  }

  for (int i = 0; i < collections_response.collections_size(); ++i) {
    backdrop::Collection collection = collections_response.collections(i);
    extensions::api::wallpaper_private::CollectionInfo collection_info;
    collection_info.collection_name = collection.collection_name();
    collection_info.collection_id = collection.collection_id();
    collections_info_list.push_back(std::move(collection_info));
  }

  simple_loader_.reset();
  std::move(callback_).Run(true /*success=*/, collections_info_list);
}

ImageInfoFetcher::ImageInfoFetcher(const std::string& collection_id)
    : collection_id_(collection_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ImageInfoFetcher::~ImageInfoFetcher() = default;

void ImageInfoFetcher::Start(OnImagesInfoFetched callback) {
  DCHECK(!simple_loader_ && callback_.is_null());
  callback_ = std::move(callback);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("backdrop_images_info_download", R"(
        semantics {
          sender: "ChromeOS Wallpaper Picker"
          description:
            "When user clicks on a particular wallpaper collection on the "
            "ChromeOS Wallpaper Picker, it displays the preview of the iamges "
            "and descriptive texts for each image. Such information is "
            "downloaded from the Backdrop wallpaper service."
          trigger:
            "When ChromeOS Wallpaper Picker extension is open, "
            "GOOGLE_CHROME_BUILD is defined and user clicks on a particular "
            "collection."
          data:
            "The Backdrop protocol buffer messages. No user data is included."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "NA"
          policy_exception_justification:
            "Not implemented, considered not necessary."
        })");

  SystemNetworkContextManager* system_network_context_manager =
      g_browser_process->system_network_context_manager();
  // In unit tests, the browser process can return a null context manager
  if (!system_network_context_manager)
    return;

  network::mojom::URLLoaderFactory* loader_factory =
      system_network_context_manager->GetURLLoaderFactory();

  backdrop::GetImagesInCollectionRequest request;
  // The language field may include the country code (e.g. "en-US").
  request.set_language(g_browser_process->GetApplicationLocale());
  request.set_collection_id(collection_id_);
  std::string serialized_proto;
  request.SerializeToString(&serialized_proto);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kBackdropImagesUrl);
  resource_request->method = "POST";
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DO_NOT_SEND_COOKIES |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;

  simple_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                    traffic_annotation);
  simple_loader_->AttachStringForUpload(serialized_proto, kProtoMimeType);
  simple_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory, base::BindOnce(&ImageInfoFetcher::OnURLFetchComplete,
                                     base::Unretained(this)));
}

void ImageInfoFetcher::OnURLFetchComplete(
    std::unique_ptr<std::string> response_body) {
  std::vector<extensions::api::wallpaper_private::ImageInfo> images_info_list;

  if (!response_body) {
    // TODO(crbug.com/800945): Adds retry mechanism and error handling.
    int response_code = -1;
    if (simple_loader_->ResponseInfo() &&
        simple_loader_->ResponseInfo()->headers)
      response_code = simple_loader_->ResponseInfo()->headers->response_code();
    LOG(ERROR) << "Downloading Backdrop wallpaper proto for collection "
               << collection_id_
               << " failed with error code: " << response_code;
    simple_loader_.reset();
    std::move(callback_).Run(false /*success=*/, images_info_list);
    return;
  }
  backdrop::GetImagesInCollectionResponse images_response;
  if (!images_response.ParseFromString(*response_body)) {
    LOG(ERROR) << "Deserializing Backdrop wallpaper proto for collection "
               << collection_id_ << " failed";
    simple_loader_.reset();
    std::move(callback_).Run(false /*success=*/, images_info_list);
    return;
  }

  for (int i = 0; i < images_response.images_size(); ++i) {
    backdrop::Image image = images_response.images()[i];

    // The info of each image should contain image url, action url and display
    // text.
    extensions::api::wallpaper_private::ImageInfo image_info;
    image_info.image_url = image.image_url();
    image_info.action_url = image.action_url();

    // Display text may have more than one strings.
    for (int j = 0; j < image.attribution_size(); ++j)
      image_info.display_text.push_back(image.attribution()[j].text());

    images_info_list.push_back(std::move(image_info));
  }

  simple_loader_.reset();
  std::move(callback_).Run(true /*success=*/, images_info_list);
}

}  // namespace backdrop_wallpaper_handlers
