// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vaapi_vp8_accelerator.h"

#include "media/gpu/vaapi/vaapi_common.h"
#include "media/gpu/vaapi/vaapi_video_decode_accelerator.h"
#include "media/gpu/vaapi/vaapi_wrapper.h"
#include "media/gpu/vp8_picture.h"

#define ARRAY_MEMCPY_CHECKED(to, from)                               \
  do {                                                               \
    static_assert(sizeof(to) == sizeof(from),                        \
                  #from " and " #to " arrays must be of same size"); \
    memcpy(to, from, sizeof(to));                                    \
  } while (0)

namespace media {

VaapiVP8Accelerator::VaapiVP8Accelerator(
    VaapiVideoDecodeAccelerator* vaapi_dec,
    scoped_refptr<VaapiWrapper> vaapi_wrapper)
    : vaapi_wrapper_(vaapi_wrapper), vaapi_dec_(vaapi_dec) {
  DCHECK(vaapi_wrapper_);
  DCHECK(vaapi_dec_);
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

VaapiVP8Accelerator::~VaapiVP8Accelerator() {
  // TODO(mcasas): consider enabling the checker, https://crbug.com/789160
  // DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

scoped_refptr<VP8Picture> VaapiVP8Accelerator::CreateVP8Picture() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const auto va_surface = vaapi_dec_->CreateVASurface();
  if (!va_surface)
    return nullptr;

  return new VaapiVP8Picture(std::move(va_surface));
}

bool VaapiVP8Accelerator::SubmitDecode(
    scoped_refptr<VP8Picture> pic,
    const Vp8ReferenceFrameVector& reference_frames) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VAIQMatrixBufferVP8 iq_matrix_buf;
  memset(&iq_matrix_buf, 0, sizeof(VAIQMatrixBufferVP8));

  const auto& frame_hdr = pic->frame_hdr;
  const Vp8SegmentationHeader& sgmnt_hdr = frame_hdr->segmentation_hdr;
  const Vp8QuantizationHeader& quant_hdr = frame_hdr->quantization_hdr;
  static_assert(arraysize(iq_matrix_buf.quantization_index) == kMaxMBSegments,
                "incorrect quantization matrix size");
  for (size_t i = 0; i < kMaxMBSegments; ++i) {
    int q = quant_hdr.y_ac_qi;

    if (sgmnt_hdr.segmentation_enabled) {
      if (sgmnt_hdr.segment_feature_mode ==
          Vp8SegmentationHeader::FEATURE_MODE_ABSOLUTE)
        q = sgmnt_hdr.quantizer_update_value[i];
      else
        q += sgmnt_hdr.quantizer_update_value[i];
    }

#define CLAMP_Q(q) std::min(std::max(q, 0), 127)
    static_assert(arraysize(iq_matrix_buf.quantization_index[i]) == 6,
                  "incorrect quantization matrix size");
    iq_matrix_buf.quantization_index[i][0] = CLAMP_Q(q);
    iq_matrix_buf.quantization_index[i][1] = CLAMP_Q(q + quant_hdr.y_dc_delta);
    iq_matrix_buf.quantization_index[i][2] = CLAMP_Q(q + quant_hdr.y2_dc_delta);
    iq_matrix_buf.quantization_index[i][3] = CLAMP_Q(q + quant_hdr.y2_ac_delta);
    iq_matrix_buf.quantization_index[i][4] = CLAMP_Q(q + quant_hdr.uv_dc_delta);
    iq_matrix_buf.quantization_index[i][5] = CLAMP_Q(q + quant_hdr.uv_ac_delta);
#undef CLAMP_Q
  }

  if (!vaapi_wrapper_->SubmitBuffer(
          VAIQMatrixBufferType, sizeof(VAIQMatrixBufferVP8), &iq_matrix_buf))
    return false;

  VAProbabilityDataBufferVP8 prob_buf;
  memset(&prob_buf, 0, sizeof(VAProbabilityDataBufferVP8));

  const Vp8EntropyHeader& entr_hdr = frame_hdr->entropy_hdr;
  ARRAY_MEMCPY_CHECKED(prob_buf.dct_coeff_probs, entr_hdr.coeff_probs);

  if (!vaapi_wrapper_->SubmitBuffer(VAProbabilityBufferType,
                                    sizeof(VAProbabilityDataBufferVP8),
                                    &prob_buf))
    return false;

  VAPictureParameterBufferVP8 pic_param;
  memset(&pic_param, 0, sizeof(VAPictureParameterBufferVP8));
  pic_param.frame_width = frame_hdr->width;
  pic_param.frame_height = frame_hdr->height;

  const auto last_frame = reference_frames.GetFrame(Vp8RefType::VP8_FRAME_LAST);
  if (last_frame) {
    pic_param.last_ref_frame =
        last_frame->AsVaapiVP8Picture()->GetVASurfaceID();
  } else {
    pic_param.last_ref_frame = VA_INVALID_SURFACE;
  }

  const auto golden_frame =
      reference_frames.GetFrame(Vp8RefType::VP8_FRAME_GOLDEN);
  if (golden_frame) {
    pic_param.golden_ref_frame =
        golden_frame->AsVaapiVP8Picture()->GetVASurfaceID();
  } else {
    pic_param.golden_ref_frame = VA_INVALID_SURFACE;
  }

  const auto alt_frame =
      reference_frames.GetFrame(Vp8RefType::VP8_FRAME_ALTREF);
  if (alt_frame) {
    pic_param.alt_ref_frame = alt_frame->AsVaapiVP8Picture()->GetVASurfaceID();
  } else {
    pic_param.alt_ref_frame = VA_INVALID_SURFACE;
  }

  pic_param.out_of_loop_frame = VA_INVALID_SURFACE;

  const Vp8LoopFilterHeader& lf_hdr = frame_hdr->loopfilter_hdr;

#define FHDR_TO_PP_PF(a, b) pic_param.pic_fields.bits.a = (b)
  FHDR_TO_PP_PF(key_frame, frame_hdr->IsKeyframe() ? 0 : 1);
  FHDR_TO_PP_PF(version, frame_hdr->version);
  FHDR_TO_PP_PF(segmentation_enabled, sgmnt_hdr.segmentation_enabled);
  FHDR_TO_PP_PF(update_mb_segmentation_map,
                sgmnt_hdr.update_mb_segmentation_map);
  FHDR_TO_PP_PF(update_segment_feature_data,
                sgmnt_hdr.update_segment_feature_data);
  FHDR_TO_PP_PF(filter_type, lf_hdr.type);
  FHDR_TO_PP_PF(sharpness_level, lf_hdr.sharpness_level);
  FHDR_TO_PP_PF(loop_filter_adj_enable, lf_hdr.loop_filter_adj_enable);
  FHDR_TO_PP_PF(mode_ref_lf_delta_update, lf_hdr.mode_ref_lf_delta_update);
  FHDR_TO_PP_PF(sign_bias_golden, frame_hdr->sign_bias_golden);
  FHDR_TO_PP_PF(sign_bias_alternate, frame_hdr->sign_bias_alternate);
  FHDR_TO_PP_PF(mb_no_coeff_skip, frame_hdr->mb_no_skip_coeff);
  FHDR_TO_PP_PF(loop_filter_disable, lf_hdr.level == 0);
#undef FHDR_TO_PP_PF

  ARRAY_MEMCPY_CHECKED(pic_param.mb_segment_tree_probs, sgmnt_hdr.segment_prob);

  static_assert(arraysize(sgmnt_hdr.lf_update_value) ==
                    arraysize(pic_param.loop_filter_level),
                "loop filter level arrays mismatch");
  for (size_t i = 0; i < arraysize(sgmnt_hdr.lf_update_value); ++i) {
    int lf_level = lf_hdr.level;
    if (sgmnt_hdr.segmentation_enabled) {
      if (sgmnt_hdr.segment_feature_mode ==
          Vp8SegmentationHeader::FEATURE_MODE_ABSOLUTE)
        lf_level = sgmnt_hdr.lf_update_value[i];
      else
        lf_level += sgmnt_hdr.lf_update_value[i];
    }

    // Clamp to [0..63] range.
    lf_level = std::min(std::max(lf_level, 0), 63);
    pic_param.loop_filter_level[i] = lf_level;
  }

  static_assert(
      arraysize(lf_hdr.ref_frame_delta) ==
              arraysize(pic_param.loop_filter_deltas_ref_frame) &&
          arraysize(lf_hdr.mb_mode_delta) ==
              arraysize(pic_param.loop_filter_deltas_mode) &&
          arraysize(lf_hdr.ref_frame_delta) == arraysize(lf_hdr.mb_mode_delta),
      "loop filter deltas arrays size mismatch");
  for (size_t i = 0; i < arraysize(lf_hdr.ref_frame_delta); ++i) {
    pic_param.loop_filter_deltas_ref_frame[i] = lf_hdr.ref_frame_delta[i];
    pic_param.loop_filter_deltas_mode[i] = lf_hdr.mb_mode_delta[i];
  }

#define FHDR_TO_PP(a) pic_param.a = frame_hdr->a
  FHDR_TO_PP(prob_skip_false);
  FHDR_TO_PP(prob_intra);
  FHDR_TO_PP(prob_last);
  FHDR_TO_PP(prob_gf);
#undef FHDR_TO_PP

  ARRAY_MEMCPY_CHECKED(pic_param.y_mode_probs, entr_hdr.y_mode_probs);
  ARRAY_MEMCPY_CHECKED(pic_param.uv_mode_probs, entr_hdr.uv_mode_probs);
  ARRAY_MEMCPY_CHECKED(pic_param.mv_probs, entr_hdr.mv_probs);

  pic_param.bool_coder_ctx.range = frame_hdr->bool_dec_range;
  pic_param.bool_coder_ctx.value = frame_hdr->bool_dec_value;
  pic_param.bool_coder_ctx.count = frame_hdr->bool_dec_count;

  if (!vaapi_wrapper_->SubmitBuffer(VAPictureParameterBufferType,
                                    sizeof(pic_param), &pic_param))
    return false;

  VASliceParameterBufferVP8 slice_param;
  memset(&slice_param, 0, sizeof(slice_param));
  slice_param.slice_data_size = frame_hdr->frame_size;
  slice_param.slice_data_offset = frame_hdr->first_part_offset;
  slice_param.slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
  slice_param.macroblock_offset = frame_hdr->macroblock_bit_offset;
  // Number of DCT partitions plus control partition.
  slice_param.num_of_partitions = frame_hdr->num_of_dct_partitions + 1;

  // Per VAAPI, this size only includes the size of the macroblock data in
  // the first partition (in bytes), so we have to subtract the header size.
  slice_param.partition_size[0] =
      frame_hdr->first_part_size - ((frame_hdr->macroblock_bit_offset + 7) / 8);

  for (size_t i = 0; i < frame_hdr->num_of_dct_partitions; ++i)
    slice_param.partition_size[i + 1] = frame_hdr->dct_partition_sizes[i];

  if (!vaapi_wrapper_->SubmitBuffer(VASliceParameterBufferType,
                                    sizeof(VASliceParameterBufferVP8),
                                    &slice_param))
    return false;

  void* non_const_ptr = const_cast<uint8_t*>(frame_hdr->data);
  if (!vaapi_wrapper_->SubmitBuffer(VASliceDataBufferType,
                                    frame_hdr->frame_size, non_const_ptr))
    return false;

  return vaapi_dec_->DecodeVASurface(pic->AsVaapiVP8Picture()->va_surface());
}

bool VaapiVP8Accelerator::OutputPicture(const scoped_refptr<VP8Picture>& pic) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const VaapiVP8Picture* vaapi_pic = pic->AsVaapiVP8Picture();
  vaapi_dec_->VASurfaceReady(vaapi_pic->va_surface(), vaapi_pic->bitstream_id(),
                             vaapi_pic->visible_rect());
  return true;
}

}  // namespace media
