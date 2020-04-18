// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/detection_utils_win.h"

#include <windows.foundation.collections.h>
#include <windows.media.faceanalysis.h>
#include <windows.media.ocr.h>
#include <wrl/implements.h>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/numerics/checked_math.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/win/winrt_storage_util.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace shape_detection {

using ABI::Windows::Foundation::Collections::IVector;
using ABI::Windows::Foundation::IAsyncOperationCompletedHandler;
using ABI::Windows::Media::FaceAnalysis::DetectedFace;
using ABI::Windows::Media::Ocr::OcrResult;
using ABI::Windows::Media::FaceAnalysis::FaceDetector;

template <typename RuntimeType>
HRESULT AsyncOperation<RuntimeType>::BeginAsyncOperation(
    typename AsyncOperation<RuntimeType>::Callback callback,
    typename AsyncOperation<RuntimeType>::IAsyncOperationPtr async_op_ptr) {
  auto instance =
      new AsyncOperation<RuntimeType>(std::move(callback), async_op_ptr);

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunnerHandle::Get();

  typedef WRL::Implements<WRL::RuntimeClassFlags<WRL::ClassicCom>,
                          IAsyncOperationCompletedHandler<RuntimeType*>,
                          WRL::FtmBase>
      AsyncCallback;
  auto async_callback = WRL::Callback<AsyncCallback>(
      [instance, task_runner](IAsyncOperation<RuntimeType*>* async_op,
                              AsyncStatus status) {
        // A reference to |async_op| is kept in |async_op_ptr_|, safe to pass
        // outside.  This is happening on an OS thread.
        task_runner->PostTask(
            FROM_HERE, base::BindOnce(&AsyncOperation::AsyncCallbackInternal,
                                      base::Owned(instance),
                                      base::Unretained(async_op), status));

        return S_OK;
      });

  return async_op_ptr->put_Completed(async_callback.Get());
}

template HRESULT AsyncOperation<IVector<DetectedFace*>>::BeginAsyncOperation(
    AsyncOperation<IVector<DetectedFace*>>::Callback callback,
    AsyncOperation<IVector<DetectedFace*>>::IAsyncOperationPtr async_op_ptr);

template HRESULT AsyncOperation<FaceDetector>::BeginAsyncOperation(
    AsyncOperation<FaceDetector>::Callback callback,
    AsyncOperation<FaceDetector>::IAsyncOperationPtr async_op_ptr);

template HRESULT AsyncOperation<OcrResult>::BeginAsyncOperation(
    AsyncOperation<OcrResult>::Callback callback,
    AsyncOperation<OcrResult>::IAsyncOperationPtr async_op_ptr);

template <typename RuntimeType>
void AsyncOperation<RuntimeType>::AsyncCallbackInternal(
    IAsyncOperation<RuntimeType*>* async_op,
    AsyncStatus status) {
  DCHECK_EQ(async_op, async_op_ptr_.Get());

  std::move(callback_).Run((async_op && status == AsyncStatus::Completed)
                               ? std::move(async_op_ptr_)
                               : nullptr);
}

WRL::ComPtr<ISoftwareBitmap> CreateWinBitmapFromSkBitmap(
    const SkBitmap& bitmap,
    ISoftwareBitmapStatics* bitmap_factory) {
  DCHECK(bitmap_factory);
  DCHECK_EQ(bitmap.colorType(), kN32_SkColorType);
  if (!base::CheckedNumeric<uint32_t>(bitmap.computeByteSize()).IsValid()) {
    DLOG(ERROR) << "Data overflow.";
    return nullptr;
  }

  // Create IBuffer from bitmap data.
  WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> buffer;
  HRESULT hr = base::win::CreateIBufferFromData(
      static_cast<uint8_t*>(bitmap.getPixels()),
      static_cast<UINT32>(bitmap.computeByteSize()), &buffer);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Create IBuffer from bitmap data failed: "
                << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }

  WRL::ComPtr<ISoftwareBitmap> win_bitmap;
  const BitmapPixelFormat pixelFormat =
      (kN32_SkColorType == kRGBA_8888_SkColorType)
          ? ABI::Windows::Graphics::Imaging::BitmapPixelFormat_Rgba8
          : ABI::Windows::Graphics::Imaging::BitmapPixelFormat_Bgra8;
  // Create ISoftwareBitmap from SKBitmap that is kN32_SkColorType and copy the
  // IBuffer into it.
  hr = bitmap_factory->CreateCopyFromBuffer(buffer.Get(), pixelFormat,
                                            bitmap.width(), bitmap.height(),
                                            win_bitmap.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "Create ISoftwareBitmap from buffer failed: "
                << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }

  return win_bitmap;
}

WRL::ComPtr<ISoftwareBitmap> CreateWinBitmapWithPixelFormat(
    const SkBitmap& bitmap,
    ISoftwareBitmapStatics* bitmap_factory,
    BitmapPixelFormat pixel_format) {
  WRL::ComPtr<ISoftwareBitmap> win_bitmap =
      CreateWinBitmapFromSkBitmap(bitmap, bitmap_factory);

  // Convert Rgba8/Bgra8 to Gray8/Nv12 SoftwareBitmap.
  const HRESULT hr = bitmap_factory->Convert(win_bitmap.Get(), pixel_format,
                                             win_bitmap.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "Convert Rgba8/Bgra8 to Gray8/Nv12 failed: "
                << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }

  return win_bitmap;
}

}  // namespace shape_detection
