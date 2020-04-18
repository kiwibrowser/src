// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_h264_accelerator.h"

#include <windows.h>

#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"
#include "media/gpu/h264_decoder.h"
#include "media/gpu/h264_dpb.h"
#include "media/gpu/windows/d3d11_picture_buffer.h"
#include "media/gpu/windows/return_on_failure.h"
#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"
#include "ui/gfx/color_space.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image_dxgi.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/scoped_binders.h"

namespace media {

class D3D11H264Picture : public H264Picture {
 public:
  D3D11H264Picture(D3D11PictureBuffer* picture)
      : picture(picture), level_(picture->level()) {
    picture->set_in_picture_use(true);
  }

  D3D11PictureBuffer* picture;
  size_t level_;

 protected:
  ~D3D11H264Picture() override;
};

D3D11H264Picture::~D3D11H264Picture() {
  picture->set_in_picture_use(false);
}

D3D11H264Accelerator::D3D11H264Accelerator(
    D3D11VideoDecoderClient* client,
    Microsoft::WRL::ComPtr<ID3D11VideoDecoder> video_decoder,
    Microsoft::WRL::ComPtr<ID3D11VideoDevice> video_device,
    Microsoft::WRL::ComPtr<ID3D11VideoContext> video_context)
    : client_(client),
      video_decoder_(video_decoder),
      video_device_(video_device),
      video_context_(video_context) {}

D3D11H264Accelerator::~D3D11H264Accelerator() {}

scoped_refptr<H264Picture> D3D11H264Accelerator::CreateH264Picture() {
  D3D11PictureBuffer* picture = client_->GetPicture();
  if (!picture) {
    return nullptr;
  }
  return base::MakeRefCounted<D3D11H264Picture>(picture);
}

bool D3D11H264Accelerator::SubmitFrameMetadata(
    const H264SPS* sps,
    const H264PPS* pps,
    const H264DPB& dpb,
    const H264Picture::Vector& ref_pic_listp0,
    const H264Picture::Vector& ref_pic_listb0,
    const H264Picture::Vector& ref_pic_listb1,
    const scoped_refptr<H264Picture>& pic) {
  scoped_refptr<D3D11H264Picture> our_pic(
      static_cast<D3D11H264Picture*>(pic.get()));

  HRESULT hr;
  for (;;) {
    hr = video_context_->DecoderBeginFrame(
        video_decoder_.Get(), our_pic->picture->output_view().Get(), 0,
        nullptr);

    if (hr == E_PENDING || hr == D3DERR_WASSTILLDRAWING) {
      // Hardware is busy.  We should make the call again.
      // TODO(liberato): For now, just busy wait.
      ;
    } else if (!SUCCEEDED(hr)) {
      LOG(ERROR) << "DecoderBeginFrame failed";
      return false;
    } else {
      break;
    }
  }

  sps_ = *sps;
  for (size_t i = 0; i < 16; i++) {
    ref_frame_list_[i].bPicEntry = 0xFF;
    field_order_cnt_list_[i][0] = 0;
    field_order_cnt_list_[i][1] = 0;
    frame_num_list_[i] = 0;
  }
  used_for_reference_flags_ = 0;
  non_existing_frame_flags_ = 0;

  int i = 0;

  // TODO(liberato): this is similar to H264Accelerator.  can they share code?

  for (auto it = dpb.begin(); it != dpb.end(); it++) {
    scoped_refptr<D3D11H264Picture> our_ref_pic(
        static_cast<D3D11H264Picture*>(it->get()));
    if (!our_ref_pic->ref) {
      i++;
      continue;
    }
    ref_frame_list_[i].Index7Bits = our_ref_pic->level_;
    ref_frame_list_[i].AssociatedFlag = our_ref_pic->long_term;
    field_order_cnt_list_[i][0] = our_ref_pic->top_field_order_cnt;
    field_order_cnt_list_[i][1] = our_ref_pic->bottom_field_order_cnt;
    frame_num_list_[i] = ref_frame_list_[i].AssociatedFlag
                             ? our_ref_pic->long_term_pic_num
                             : our_ref_pic->frame_num;
    int ref = 3;
    used_for_reference_flags_ |= ref << (2 * i);
    non_existing_frame_flags_ |= (our_ref_pic->nonexisting) << i;
    i++;
  }
  slice_info_.clear();
  return RetrieveBitstreamBuffer();
}

bool D3D11H264Accelerator::RetrieveBitstreamBuffer() {
  DCHECK(!bitstream_buffer_bytes_);
  DCHECK(!bitstream_buffer_size_);

  current_offset_ = 0;
  void* buffer;
  UINT buffer_size;
  HRESULT hr = video_context_->GetDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_BITSTREAM, &buffer_size,
      &buffer);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "GetDecoderBuffer (Bitstream) failed";
    return false;
  }
  bitstream_buffer_bytes_ = (uint8_t*)buffer;
  bitstream_buffer_size_ = buffer_size;

  return true;
}

bool D3D11H264Accelerator::SubmitSlice(const H264PPS* pps,
                                       const H264SliceHeader* slice_hdr,
                                       const H264Picture::Vector& ref_pic_list0,
                                       const H264Picture::Vector& ref_pic_list1,
                                       const scoped_refptr<H264Picture>& pic,
                                       const uint8_t* data,
                                       size_t size) {
  scoped_refptr<D3D11H264Picture> our_pic(
      static_cast<D3D11H264Picture*>(pic.get()));
  DXVA_PicParams_H264 pic_param = {};
#define FROM_SPS_TO_PP(a) pic_param.a = sps_.a
#define FROM_SPS_TO_PP2(a, b) pic_param.a = sps_.b
#define FROM_PPS_TO_PP(a) pic_param.a = pps->a
#define FROM_PPS_TO_PP2(a, b) pic_param.a = pps->b
#define FROM_SLICE_TO_PP(a) pic_param.a = slice_hdr->a
#define FROM_SLICE_TO_PP2(a, b) pic_param.a = slice_hdr->b
  FROM_SPS_TO_PP2(wFrameWidthInMbsMinus1, pic_width_in_mbs_minus1);
  FROM_SPS_TO_PP2(wFrameHeightInMbsMinus1, pic_height_in_map_units_minus1);
  pic_param.CurrPic.Index7Bits = our_pic->level_;
  pic_param.CurrPic.AssociatedFlag = slice_hdr->bottom_field_flag;
  FROM_SPS_TO_PP2(num_ref_frames, max_num_ref_frames);

  FROM_SLICE_TO_PP(field_pic_flag);
  pic_param.MbaffFrameFlag =
      sps_.mb_adaptive_frame_field_flag && pic_param.field_pic_flag;
  FROM_SPS_TO_PP2(residual_colour_transform_flag, separate_colour_plane_flag);
  FROM_SLICE_TO_PP(sp_for_switch_flag);
  FROM_SPS_TO_PP(chroma_format_idc);
  pic_param.RefPicFlag = pic->ref;
  FROM_PPS_TO_PP(constrained_intra_pred_flag);
  FROM_PPS_TO_PP(weighted_pred_flag);
  FROM_PPS_TO_PP(weighted_bipred_idc);
  pic_param.MbsConsecutiveFlag = 1;
  FROM_SPS_TO_PP(frame_mbs_only_flag);
  FROM_PPS_TO_PP(transform_8x8_mode_flag);
  // TODO(liberato): sandersd@ believes that this should only be set for level
  // >= 3.1 .  verify this and fix as needed.
  pic_param.MinLumaBipredSize8x8Flag = 1;
  pic_param.IntraPicFlag = slice_hdr->IsISlice();
  FROM_SPS_TO_PP(bit_depth_luma_minus8);
  FROM_SPS_TO_PP(bit_depth_chroma_minus8);
  // The latest DXVA decoding guide says to set this to 3 if the software
  // decoder (this class) is following the guide.
  pic_param.Reserved16Bits = 3;
  memcpy(pic_param.RefFrameList, ref_frame_list_,
         sizeof pic_param.RefFrameList);
  if (pic_param.field_pic_flag && pic_param.CurrPic.AssociatedFlag) {
    pic_param.CurrFieldOrderCnt[1] = pic->bottom_field_order_cnt;
    pic_param.CurrFieldOrderCnt[0] = 0;
  } else if (pic_param.field_pic_flag && !pic_param.CurrPic.AssociatedFlag) {
    pic_param.CurrFieldOrderCnt[0] = pic->top_field_order_cnt;
    pic_param.CurrFieldOrderCnt[1] = 0;
  } else {
    pic_param.CurrFieldOrderCnt[0] = pic->top_field_order_cnt;
    pic_param.CurrFieldOrderCnt[1] = pic->bottom_field_order_cnt;
  }
  memcpy(pic_param.FieldOrderCntList, field_order_cnt_list_,
         sizeof pic_param.FieldOrderCntList);
  FROM_PPS_TO_PP(pic_init_qs_minus26);
  FROM_PPS_TO_PP(chroma_qp_index_offset);
  FROM_PPS_TO_PP(second_chroma_qp_index_offset);
  pic_param.ContinuationFlag = 1;
  FROM_PPS_TO_PP(pic_init_qp_minus26);
  FROM_PPS_TO_PP2(num_ref_idx_l0_active_minus1,
                  num_ref_idx_l0_default_active_minus1);
  FROM_PPS_TO_PP2(num_ref_idx_l1_active_minus1,
                  num_ref_idx_l1_default_active_minus1);
  // UNUSED: Reserved8BitsA
  memcpy(pic_param.FrameNumList, frame_num_list_,
         sizeof pic_param.FrameNumList);
  pic_param.UsedForReferenceFlags = used_for_reference_flags_;
  pic_param.NonExistingFrameFlags = non_existing_frame_flags_;
  pic_param.frame_num = pic->frame_num;
  FROM_SPS_TO_PP(log2_max_frame_num_minus4);
  FROM_SPS_TO_PP(pic_order_cnt_type);
  FROM_SPS_TO_PP(log2_max_pic_order_cnt_lsb_minus4);
  FROM_SPS_TO_PP(delta_pic_order_always_zero_flag);
  FROM_SPS_TO_PP(direct_8x8_inference_flag);
  FROM_PPS_TO_PP(entropy_coding_mode_flag);
  FROM_PPS_TO_PP2(pic_order_present_flag,
                  bottom_field_pic_order_in_frame_present_flag);
  FROM_PPS_TO_PP(num_slice_groups_minus1);
  CHECK_EQ(0u, pic_param.num_slice_groups_minus1);
  // UNUSED: slice_group_map_type
  FROM_PPS_TO_PP(deblocking_filter_control_present_flag);
  FROM_PPS_TO_PP(redundant_pic_cnt_present_flag);
  // UNUSED: Reserved8BitsB
  // UNUSED: slice_group_change_rate
  //
  //
  //

  pic_param.StatusReportFeedbackNumber = 1;

  UINT buffer_size;
  void* buffer;
  HRESULT hr = video_context_->GetDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS,
      &buffer_size, &buffer);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "ReleaseDecoderBuffer (PictureParams) failed";
    return false;
  }

  memcpy(buffer, &pic_param, sizeof(pic_param));
  hr = video_context_->ReleaseDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "ReleaseDecoderBuffer (PictureParams) failed";
    return false;
  }

  DXVA_Qmatrix_H264 iq_matrix_buf = {};

  if (pps->pic_scaling_matrix_present_flag) {
    for (int i = 0; i < 6; ++i) {
      for (int j = 0; j < 16; ++j)
        iq_matrix_buf.bScalingLists4x4[i][j] = pps->scaling_list4x4[i][j];
    }

    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 64; ++j)
        iq_matrix_buf.bScalingLists8x8[i][j] = pps->scaling_list8x8[i][j];
    }
  } else {
    for (int i = 0; i < 6; ++i) {
      for (int j = 0; j < 16; ++j)
        iq_matrix_buf.bScalingLists4x4[i][j] = sps_.scaling_list4x4[i][j];
    }

    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 64; ++j)
        iq_matrix_buf.bScalingLists8x8[i][j] = sps_.scaling_list8x8[i][j];
    }
  }
  hr = video_context_->GetDecoderBuffer(
      video_decoder_.Get(),
      D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX, &buffer_size,
      &buffer);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "GetDecoderBuffer (QuantMatrix) failed";
    return false;
  }
  memcpy(buffer, &iq_matrix_buf, sizeof(iq_matrix_buf));
  hr = video_context_->ReleaseDecoderBuffer(
      video_decoder_.Get(),
      D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "ReleaseDecoderBuffer (QuantMatrix) failed";
    return false;
  }

  // Ideally all slices in a frame are put in the same bitstream buffer.
  // However the bitstream buffer may not fit all the data, so split on the
  // necessary boundaries.

  size_t out_bitstream_size = size + 3;

  size_t remaining_bitstream = out_bitstream_size;
  size_t start_location = 0;

  while (remaining_bitstream > 0) {
    if (bitstream_buffer_size_ < remaining_bitstream &&
        slice_info_.size() > 0) {
      if (!SubmitSliceData()) {
        LOG(ERROR) << "SubmitSliceData failed";
        return false;
      }
      if (!RetrieveBitstreamBuffer()) {
        LOG(ERROR) << "RetrieveBitstreamBuffer failed";
        return false;
      }
    }

    size_t bytes_to_copy = remaining_bitstream;
    bool contains_end = true;
    if (bytes_to_copy > bitstream_buffer_size_) {
      bytes_to_copy = bitstream_buffer_size_;
      contains_end = false;
    }
    size_t real_bytes_to_copy = bytes_to_copy;
    // TODO(jbauman): fix hack
    uint8_t* out_start = bitstream_buffer_bytes_;
    if (bytes_to_copy >= 3 && start_location == 0) {
      *(out_start++) = 0;
      *(out_start++) = 0;
      *(out_start++) = 1;
      real_bytes_to_copy -= 3;
    }
    memcpy(out_start, data + start_location, real_bytes_to_copy);

    DXVA_Slice_H264_Short slice_info = {};
    slice_info.BSNALunitDataLocation = (UINT)current_offset_;
    slice_info.SliceBytesInBuffer = (UINT)bytes_to_copy;
    if (contains_end && start_location == 0)
      slice_info.wBadSliceChopping = 0;
    else if (!contains_end && start_location == 0)
      slice_info.wBadSliceChopping = 1;
    else if (contains_end && start_location != 0)
      slice_info.wBadSliceChopping = 2;
    else
      slice_info.wBadSliceChopping = 3;

    slice_info_.push_back(slice_info);
    bitstream_buffer_size_ -= bytes_to_copy;
    current_offset_ += bytes_to_copy;
    start_location += bytes_to_copy;
    remaining_bitstream -= bytes_to_copy;
    bitstream_buffer_bytes_ += bytes_to_copy;
  }

  return true;
}

bool D3D11H264Accelerator::SubmitSliceData() {
  CHECK(slice_info_.size() > 0);
  UINT buffer_size;
  void* buffer;

  // TODO(liberato): Should we release the other buffers on failure?

  HRESULT hr = video_context_->GetDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL,
      &buffer_size, &buffer);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "GetDecoderBuffer (SliceControl) failed";
    return false;
  }

  CHECK_LE(sizeof(slice_info_[0]) * slice_info_.size(), buffer_size);
  memcpy(buffer, &slice_info_[0], sizeof(slice_info_[0]) * slice_info_.size());
  hr = video_context_->ReleaseDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "ReleaseDecoderBuffer (SliceControl) failed";
    return false;
  }

  hr = video_context_->ReleaseDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_BITSTREAM);
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "ReleaseDecoderBuffer (BitStream) failed";
    return false;
  }

  D3D11_VIDEO_DECODER_BUFFER_DESC buffers[4] = {};
  buffers[0].BufferType = D3D11_VIDEO_DECODER_BUFFER_PICTURE_PARAMETERS;
  buffers[0].DataOffset = 0;
  buffers[0].DataSize = sizeof(DXVA_PicParams_H264);
  buffers[1].BufferType =
      D3D11_VIDEO_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX;
  buffers[1].DataOffset = 0;
  buffers[1].DataSize = sizeof(DXVA_Qmatrix_H264);
  buffers[2].BufferType = D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL;
  buffers[2].DataOffset = 0;
  buffers[2].DataSize = (UINT)(sizeof(slice_info_[0]) * slice_info_.size());
  buffers[3].BufferType = D3D11_VIDEO_DECODER_BUFFER_BITSTREAM;
  buffers[3].DataOffset = 0;
  buffers[3].DataSize = (UINT)current_offset_;

  hr = video_context_->SubmitDecoderBuffers(video_decoder_.Get(), 4, buffers);
  current_offset_ = 0;
  slice_info_.clear();
  bitstream_buffer_bytes_ = nullptr;
  bitstream_buffer_size_ = 0;
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "SubmitDecoderBuffers failed";
    return false;
  }

  return true;
}

bool D3D11H264Accelerator::SubmitDecode(const scoped_refptr<H264Picture>& pic) {
  if (!SubmitSliceData()) {
    LOG(ERROR) << "SubmitSliceData failed";
    return false;
  }

  HRESULT hr = video_context_->DecoderEndFrame(video_decoder_.Get());
  if (!SUCCEEDED(hr)) {
    LOG(ERROR) << "DecoderEndFrame failed";
    return false;
  }

  return true;
}

void D3D11H264Accelerator::Reset() {
  if (!bitstream_buffer_bytes_)
    return;

  HRESULT hr = video_context_->ReleaseDecoderBuffer(
      video_decoder_.Get(), D3D11_VIDEO_DECODER_BUFFER_BITSTREAM);

  bitstream_buffer_bytes_ = nullptr;
  bitstream_buffer_size_ = 0;
  current_offset_ = 0;
  CHECK(SUCCEEDED(hr));
}

bool D3D11H264Accelerator::OutputPicture(
    const scoped_refptr<H264Picture>& pic) {
  scoped_refptr<D3D11H264Picture> our_pic(
      static_cast<D3D11H264Picture*>(pic.get()));
  client_->OutputResult(our_pic->picture);
  return true;
}

}  // namespace media
