// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/widget/thumbnail_generator.h"

#include <memory>

#include "base/android/jni_string.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/weak_ptr.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "jni/ThumbnailGenerator_jni.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/skbitmap_operations.h"

class SkBitmap;

using base::android::JavaParamRef;

namespace {

// Ignore image files that are too large to avoid long delays.
const int64_t kMaxImageSize = 10 * 1024 * 1024;  // 10 MB

std::string LoadImageData(const base::FilePath& path) {
  base::AssertBlockingAllowed();

  // Confirm that the file's size is within our threshold.
  int64_t file_size;
  if (!base::GetFileSize(path, &file_size) || file_size > kMaxImageSize) {
    LOG(ERROR) << "Unexpected file size: " << path.MaybeAsASCII() << ", "
               << file_size;
    return std::string();
  }

  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
  }

  return data;
}

SkBitmap ScaleDownBitmap(int icon_size, const SkBitmap& decoded_image) {
  DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (decoded_image.drawsNothing())
    return decoded_image;

  // Shrink the image down so that its smallest dimension is equal to or
  // smaller than the requested size.
  int min_dimension = std::min(decoded_image.width(), decoded_image.height());

  if (min_dimension <= icon_size)
    return decoded_image;

  uint64_t width = static_cast<uint64_t>(decoded_image.width());
  uint64_t height = static_cast<uint64_t>(decoded_image.height());
  return skia::ImageOperations::Resize(
      decoded_image, skia::ImageOperations::RESIZE_BEST,
      width * icon_size / min_dimension, height * icon_size / min_dimension);
}

class ImageThumbnailRequest : public ImageDecoder::ImageRequest {
 public:
  ImageThumbnailRequest(int icon_size,
                        base::OnceCallback<void(const SkBitmap&)> callback)
      : icon_size_(icon_size),
        callback_(std::move(callback)),
        weak_ptr_factory_(this) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  }

  ~ImageThumbnailRequest() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  }

  void Start(const base::FilePath& path) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
        base::BindOnce(&LoadImageData, path),
        base::BindOnce(&ImageThumbnailRequest::OnLoadComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  // ImageDecoder::ImageRequest implementation.
  void OnImageDecoded(const SkBitmap& decoded_image) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
        base::BindOnce(&ScaleDownBitmap, icon_size_, decoded_image),
        base::BindOnce(&ImageThumbnailRequest::FinishRequest,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void OnDecodeImageFailed() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    LOG(ERROR) << "Failed to decode image.";
    FinishRequest(SkBitmap());
  }

  void OnLoadComplete(const std::string& data) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (data.empty()) {
      FinishRequest(SkBitmap());
      return;
    }

    ImageDecoder::Start(this, data);
  }

  void FinishRequest(const SkBitmap& thumbnail) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(callback_), thumbnail));
    delete this;
  }

  const int icon_size_;
  base::OnceCallback<void(const SkBitmap&)> callback_;
  base::WeakPtrFactory<ImageThumbnailRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImageThumbnailRequest);
};

}  // namespace

ThumbnailGenerator::ThumbnailGenerator(const JavaParamRef<jobject>& jobj)
    : java_delegate_(jobj), weak_factory_(this) {
  DCHECK(!jobj.is_null());
}

ThumbnailGenerator::~ThumbnailGenerator() = default;

void ThumbnailGenerator::Destroy(JNIEnv* env,
                                 const JavaParamRef<jobject>& jobj) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  delete this;
}

void ThumbnailGenerator::OnThumbnailRetrieved(
    const base::android::ScopedJavaGlobalRef<jstring>& content_id,
    int icon_size,
    const base::android::ScopedJavaGlobalRef<jobject>& callback,
    const SkBitmap& thumbnail) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Send the bitmap back to Java-land.
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_ThumbnailGenerator_onThumbnailRetrieved(
      env, java_delegate_, content_id, icon_size,
      thumbnail.drawsNothing() ? NULL : gfx::ConvertToJavaBitmap(&thumbnail),
      callback);
}

void ThumbnailGenerator::RetrieveThumbnail(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    const JavaParamRef<jstring>& jcontent_id,
    const JavaParamRef<jstring>& jfile_path,
    jint icon_size,
    const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string file_path =
      base::android::ConvertJavaStringToUTF8(env, jfile_path);

  auto request = std::make_unique<ImageThumbnailRequest>(
      icon_size,
      base::BindOnce(
          &ThumbnailGenerator::OnThumbnailRetrieved, weak_factory_.GetWeakPtr(),
          base::android::ScopedJavaGlobalRef<jstring>(jcontent_id), icon_size,
          base::android::ScopedJavaGlobalRef<jobject>(callback)));
  request->Start(base::FilePath::FromUTF8Unsafe(file_path));

  // Dropping ownership of |request| here because it will clean itself up once
  // the started request finishes.
  request.release();
}

// static
static jlong JNI_ThumbnailGenerator_Init(JNIEnv* env,
                                         const JavaParamRef<jobject>& jobj) {
  return reinterpret_cast<intptr_t>(new ThumbnailGenerator(jobj));
}
