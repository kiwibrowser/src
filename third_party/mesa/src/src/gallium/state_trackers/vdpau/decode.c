/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/u_video.h"

#include "vl/vl_vlc.h"

#include "vdpau_private.h"

/**
 * Create a VdpDecoder.
 */
VdpStatus
vlVdpDecoderCreate(VdpDevice device,
                   VdpDecoderProfile profile,
                   uint32_t width, uint32_t height,
                   uint32_t max_references,
                   VdpDecoder *decoder)
{
   enum pipe_video_profile p_profile;
   struct pipe_context *pipe;
   struct pipe_screen *screen;
   vlVdpDevice *dev;
   vlVdpDecoder *vldecoder;
   VdpStatus ret;
   bool supported;

   if (!decoder)
      return VDP_STATUS_INVALID_POINTER;
   *decoder = 0;

   if (!(width && height))
      return VDP_STATUS_INVALID_VALUE;

   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)
      return VDP_STATUS_INVALID_DECODER_PROFILE;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = dev->context;
   screen = dev->vscreen->pscreen;

   pipe_mutex_lock(dev->mutex);

   supported = screen->get_video_param
   (
      screen,
      p_profile,
      PIPE_VIDEO_CAP_SUPPORTED
   );
   if (!supported) {
      pipe_mutex_unlock(dev->mutex);
      return VDP_STATUS_INVALID_DECODER_PROFILE;
   }

   vldecoder = CALLOC(1,sizeof(vlVdpDecoder));
   if (!vldecoder) {
      pipe_mutex_unlock(dev->mutex);
      return VDP_STATUS_RESOURCES;
   }

   vldecoder->device = dev;

   vldecoder->decoder = pipe->create_video_decoder
   (
      pipe, p_profile,
      PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
      PIPE_VIDEO_CHROMA_FORMAT_420,
      width, height, max_references,
      false
   );

   if (!vldecoder->decoder) {
      ret = VDP_STATUS_ERROR;
      goto error_decoder;
   }

   *decoder = vlAddDataHTAB(vldecoder);
   if (*decoder == 0) {
      ret = VDP_STATUS_ERROR;
      goto error_handle;
   }
   pipe_mutex_unlock(dev->mutex);

   return VDP_STATUS_OK;

error_handle:
   vldecoder->decoder->destroy(vldecoder->decoder);

error_decoder:
   pipe_mutex_unlock(dev->mutex);
   FREE(vldecoder);
   return ret;
}

/**
 * Destroy a VdpDecoder.
 */
VdpStatus
vlVdpDecoderDestroy(VdpDecoder decoder)
{
   vlVdpDecoder *vldecoder;

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   pipe_mutex_lock(vldecoder->device->mutex);
   vldecoder->decoder->destroy(vldecoder->decoder);
   pipe_mutex_unlock(vldecoder->device->mutex);

   FREE(vldecoder);

   return VDP_STATUS_OK;
}

/**
 * Retrieve the parameters used to create a VdpBitmapSurface.
 */
VdpStatus
vlVdpDecoderGetParameters(VdpDecoder decoder,
                          VdpDecoderProfile *profile,
                          uint32_t *width,
                          uint32_t *height)
{
   vlVdpDecoder *vldecoder;

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   *profile = PipeToProfile(vldecoder->decoder->profile);
   *width = vldecoder->decoder->width;
   *height = vldecoder->decoder->height;

   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpGetReferenceFrame(VdpVideoSurface handle, struct pipe_video_buffer **ref_frame)
{
   vlVdpSurface *surface;

   /* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
   if (handle ==  VDP_INVALID_HANDLE) {
      *ref_frame = NULL;
      return VDP_STATUS_OK;
   }

   surface = vlGetDataHTAB(handle);
   if (!surface)
      return VDP_STATUS_INVALID_HANDLE;

   *ref_frame = surface->video_buffer;
   if (!*ref_frame)
         return VDP_STATUS_INVALID_HANDLE;

   return VDP_STATUS_OK;
}

/**
 * Decode a mpeg 1/2 video.
 */
static VdpStatus
vlVdpDecoderRenderMpeg12(struct pipe_mpeg12_picture_desc *picture,
                         VdpPictureInfoMPEG1Or2 *picture_info)
{
   VdpStatus r;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding MPEG12\n");

   r = vlVdpGetReferenceFrame(picture_info->forward_reference, &picture->ref[0]);
   if (r != VDP_STATUS_OK)
      return r;

   r = vlVdpGetReferenceFrame(picture_info->backward_reference, &picture->ref[1]);
   if (r != VDP_STATUS_OK)
      return r;

   picture->picture_coding_type = picture_info->picture_coding_type;
   picture->picture_structure = picture_info->picture_structure;
   picture->frame_pred_frame_dct = picture_info->frame_pred_frame_dct;
   picture->q_scale_type = picture_info->q_scale_type;
   picture->alternate_scan = picture_info->alternate_scan;
   picture->intra_vlc_format = picture_info->intra_vlc_format;
   picture->concealment_motion_vectors = picture_info->concealment_motion_vectors;
   picture->intra_dc_precision = picture_info->intra_dc_precision;
   picture->f_code[0][0] = picture_info->f_code[0][0] - 1;
   picture->f_code[0][1] = picture_info->f_code[0][1] - 1;
   picture->f_code[1][0] = picture_info->f_code[1][0] - 1;
   picture->f_code[1][1] = picture_info->f_code[1][1] - 1;
   picture->num_slices = picture_info->slice_count;
   picture->top_field_first = picture_info->top_field_first;
   picture->full_pel_forward_vector = picture_info->full_pel_forward_vector;
   picture->full_pel_backward_vector = picture_info->full_pel_backward_vector;
   picture->intra_matrix = picture_info->intra_quantizer_matrix;
   picture->non_intra_matrix = picture_info->non_intra_quantizer_matrix;

   return VDP_STATUS_OK;
}

/**
 * Decode a mpeg 4 video.
 */
static VdpStatus
vlVdpDecoderRenderMpeg4(struct pipe_mpeg4_picture_desc *picture,
                        VdpPictureInfoMPEG4Part2 *picture_info)
{
   VdpStatus r;
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding MPEG4\n");

   r = vlVdpGetReferenceFrame(picture_info->forward_reference, &picture->ref[0]);
   if (r != VDP_STATUS_OK)
      return r;

   r = vlVdpGetReferenceFrame(picture_info->backward_reference, &picture->ref[1]);
   if (r != VDP_STATUS_OK)
      return r;

   for (i = 0; i < 2; ++i) {
      picture->trd[i] = picture_info->trd[i];
      picture->trb[i] = picture_info->trb[i];
   }
   picture->vop_time_increment_resolution = picture_info->vop_time_increment_resolution;
   picture->vop_coding_type = picture_info->vop_coding_type;
   picture->vop_fcode_forward = picture_info->vop_fcode_forward;
   picture->vop_fcode_backward = picture_info->vop_fcode_backward;
   picture->resync_marker_disable = picture_info->resync_marker_disable;
   picture->interlaced = picture_info->interlaced;
   picture->quant_type = picture_info->quant_type;
   picture->quarter_sample = picture_info->quarter_sample;
   picture->short_video_header = picture_info->short_video_header;
   picture->rounding_control = picture_info->rounding_control;
   picture->alternate_vertical_scan_flag = picture_info->alternate_vertical_scan_flag;
   picture->top_field_first = picture_info->top_field_first;
   picture->intra_matrix = picture_info->intra_quantizer_matrix;
   picture->non_intra_matrix = picture_info->non_intra_quantizer_matrix;

   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpDecoderRenderVC1(struct pipe_vc1_picture_desc *picture,
                      VdpPictureInfoVC1 *picture_info)
{
   VdpStatus r;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding VC-1\n");

   r = vlVdpGetReferenceFrame(picture_info->forward_reference, &picture->ref[0]);
   if (r != VDP_STATUS_OK)
      return r;

   r = vlVdpGetReferenceFrame(picture_info->backward_reference, &picture->ref[1]);
   if (r != VDP_STATUS_OK)
      return r;

   picture->slice_count = picture_info->slice_count;
   picture->picture_type = picture_info->picture_type;
   picture->frame_coding_mode = picture_info->frame_coding_mode;
   picture->postprocflag = picture_info->postprocflag;
   picture->pulldown = picture_info->pulldown;
   picture->interlace = picture_info->interlace;
   picture->tfcntrflag = picture_info->tfcntrflag;
   picture->finterpflag = picture_info->finterpflag;
   picture->psf = picture_info->psf;
   picture->dquant = picture_info->dquant;
   picture->panscan_flag = picture_info->panscan_flag;
   picture->refdist_flag = picture_info->refdist_flag;
   picture->quantizer = picture_info->quantizer;
   picture->extended_mv = picture_info->extended_mv;
   picture->extended_dmv = picture_info->extended_dmv;
   picture->overlap = picture_info->overlap;
   picture->vstransform = picture_info->vstransform;
   picture->loopfilter = picture_info->loopfilter;
   picture->fastuvmc = picture_info->fastuvmc;
   picture->range_mapy_flag = picture_info->range_mapy_flag;
   picture->range_mapy = picture_info->range_mapy;
   picture->range_mapuv_flag = picture_info->range_mapuv_flag;
   picture->range_mapuv = picture_info->range_mapuv;
   picture->multires = picture_info->multires;
   picture->syncmarker = picture_info->syncmarker;
   picture->rangered = picture_info->rangered;
   picture->maxbframes = picture_info->maxbframes;
   picture->deblockEnable = picture_info->deblockEnable;
   picture->pquant = picture_info->pquant;

   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpDecoderRenderH264(struct pipe_h264_picture_desc *picture,
                       VdpPictureInfoH264 *picture_info)
{
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding H264\n");

   picture->slice_count = picture_info->slice_count;
   picture->field_order_cnt[0] = picture_info->field_order_cnt[0];
   picture->field_order_cnt[1] = picture_info->field_order_cnt[1];
   picture->is_reference = picture_info->is_reference;
   picture->frame_num = picture_info->frame_num;
   picture->field_pic_flag = picture_info->field_pic_flag;
   picture->bottom_field_flag = picture_info->bottom_field_flag;
   picture->num_ref_frames = picture_info->num_ref_frames;
   picture->mb_adaptive_frame_field_flag = picture_info->mb_adaptive_frame_field_flag;
   picture->constrained_intra_pred_flag = picture_info->constrained_intra_pred_flag;
   picture->weighted_pred_flag = picture_info->weighted_pred_flag;
   picture->weighted_bipred_idc = picture_info->weighted_bipred_idc;
   picture->frame_mbs_only_flag = picture_info->frame_mbs_only_flag;
   picture->transform_8x8_mode_flag = picture_info->transform_8x8_mode_flag;
   picture->chroma_qp_index_offset = picture_info->chroma_qp_index_offset;
   picture->second_chroma_qp_index_offset = picture_info->second_chroma_qp_index_offset;
   picture->pic_init_qp_minus26 = picture_info->pic_init_qp_minus26;
   picture->num_ref_idx_l0_active_minus1 = picture_info->num_ref_idx_l0_active_minus1;
   picture->num_ref_idx_l1_active_minus1 = picture_info->num_ref_idx_l1_active_minus1;
   picture->log2_max_frame_num_minus4 = picture_info->log2_max_frame_num_minus4;
   picture->pic_order_cnt_type = picture_info->pic_order_cnt_type;
   picture->log2_max_pic_order_cnt_lsb_minus4 = picture_info->log2_max_pic_order_cnt_lsb_minus4;
   picture->delta_pic_order_always_zero_flag = picture_info->delta_pic_order_always_zero_flag;
   picture->direct_8x8_inference_flag = picture_info->direct_8x8_inference_flag;
   picture->entropy_coding_mode_flag = picture_info->entropy_coding_mode_flag;
   picture->pic_order_present_flag = picture_info->pic_order_present_flag;
   picture->deblocking_filter_control_present_flag = picture_info->deblocking_filter_control_present_flag;
   picture->redundant_pic_cnt_present_flag = picture_info->redundant_pic_cnt_present_flag;

   memcpy(picture->scaling_lists_4x4, picture_info->scaling_lists_4x4, 6*16);
   memcpy(picture->scaling_lists_8x8, picture_info->scaling_lists_8x8, 2*64);

   for (i = 0; i < 16; ++i) {
      VdpStatus ret = vlVdpGetReferenceFrame
      (
         picture_info->referenceFrames[i].surface,
         &picture->ref[i]
      );
      if (ret != VDP_STATUS_OK)
         return ret;

      picture->is_long_term[i] = picture_info->referenceFrames[i].is_long_term;
      picture->top_is_reference[i] = picture_info->referenceFrames[i].top_is_reference;
      picture->bottom_is_reference[i] = picture_info->referenceFrames[i].bottom_is_reference;
      picture->field_order_cnt_list[i][0] = picture_info->referenceFrames[i].field_order_cnt[0];
      picture->field_order_cnt_list[i][1] = picture_info->referenceFrames[i].field_order_cnt[1];
      picture->frame_num_list[i] = picture_info->referenceFrames[i].frame_idx;
   }

   return VDP_STATUS_OK;
}

static void
vlVdpDecoderFixVC1Startcode(uint32_t *num_buffers, const void *buffers[], unsigned sizes[])
{
   static const uint8_t vc1_startcode[] = { 0x00, 0x00, 0x01, 0x0D };
   struct vl_vlc vlc;
   unsigned i;

   /* search the first 64 bytes for a startcode */
   vl_vlc_init(&vlc, *num_buffers, buffers, sizes);
   for (i = 0; i < 64 && vl_vlc_bits_left(&vlc) >= 32; ++i) {
      uint32_t value = vl_vlc_peekbits(&vlc, 32);
      if (value == 0x0000010D ||
          value == 0x0000010C ||
          value == 0x0000010B)
         return;
      vl_vlc_eatbits(&vlc, 8);
      vl_vlc_fillbits(&vlc);
   }

   /* none found, ok add one manually */
   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Manually adding VC-1 startcode\n");
   for (i = *num_buffers; i > 0; --i) {
      buffers[i] = buffers[i - 1];
      sizes[i] = sizes[i - 1];
   }
   ++(*num_buffers);
   buffers[0] = vc1_startcode;
   sizes[0] = 4;
}

/**
 * Decode a compressed field/frame and render the result into a VdpVideoSurface.
 */
VdpStatus
vlVdpDecoderRender(VdpDecoder decoder,
                   VdpVideoSurface target,
                   VdpPictureInfo const *picture_info,
                   uint32_t bitstream_buffer_count,
                   VdpBitstreamBuffer const *bitstream_buffers)
{
   const void * buffers[bitstream_buffer_count + 1];
   unsigned sizes[bitstream_buffer_count + 1];
   vlVdpDecoder *vldecoder;
   vlVdpSurface *vlsurf;
   VdpStatus ret;
   struct pipe_screen *screen;
   struct pipe_video_decoder *dec;
   bool buffer_support[2];
   unsigned i;
   union {
      struct pipe_picture_desc base;
      struct pipe_mpeg12_picture_desc mpeg12;
      struct pipe_mpeg4_picture_desc mpeg4;
      struct pipe_vc1_picture_desc vc1;
      struct pipe_h264_picture_desc h264;
   } desc;

   if (!(picture_info && bitstream_buffers))
      return VDP_STATUS_INVALID_POINTER;

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;
   dec = vldecoder->decoder;
   screen = dec->context->screen;

   vlsurf = (vlVdpSurface *)vlGetDataHTAB(target);
   if (!vlsurf)
      return VDP_STATUS_INVALID_HANDLE;

   if (vlsurf->device != vldecoder->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   if (vlsurf->video_buffer != NULL && vlsurf->video_buffer->chroma_format != dec->chroma_format)
      // TODO: Recreate decoder with correct chroma
      return VDP_STATUS_INVALID_CHROMA_TYPE;

   pipe_mutex_lock(vlsurf->device->mutex);

   buffer_support[0] = screen->get_video_param(screen, dec->profile, PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE);
   buffer_support[1] = screen->get_video_param(screen, dec->profile, PIPE_VIDEO_CAP_SUPPORTS_INTERLACED);

   if (vlsurf->video_buffer == NULL ||
       !screen->is_video_format_supported(screen, vlsurf->video_buffer->buffer_format, dec->profile) ||
       !buffer_support[vlsurf->video_buffer->interlaced]) {

      /* destroy the old one */
      if (vlsurf->video_buffer)
         vlsurf->video_buffer->destroy(vlsurf->video_buffer);

      /* set the buffer format to the prefered one */
      vlsurf->templat.buffer_format = screen->get_video_param(screen, dec->profile, PIPE_VIDEO_CAP_PREFERED_FORMAT);

      /* also set interlacing to decoders preferences */
      vlsurf->templat.interlaced = screen->get_video_param(screen, dec->profile, PIPE_VIDEO_CAP_PREFERS_INTERLACED);

      /* and recreate the video buffer */
      vlsurf->video_buffer = dec->context->create_video_buffer(dec->context, &vlsurf->templat);

      /* still no luck? get me out of here... */
      if (!vlsurf->video_buffer) {
         pipe_mutex_unlock(vlsurf->device->mutex);
         return VDP_STATUS_NO_IMPLEMENTATION;
      }
      vlVdpVideoSurfaceClear(vlsurf);
   }

   for (i = 0; i < bitstream_buffer_count; ++i) {
      buffers[i] = bitstream_buffers[i].bitstream;
      sizes[i] = bitstream_buffers[i].bitstream_bytes;
   }

   memset(&desc, 0, sizeof(desc));
   desc.base.profile = dec->profile;
   switch (u_reduce_video_profile(dec->profile)) {
   case PIPE_VIDEO_CODEC_MPEG12:
      ret = vlVdpDecoderRenderMpeg12(&desc.mpeg12, (VdpPictureInfoMPEG1Or2 *)picture_info);
      break;
   case PIPE_VIDEO_CODEC_MPEG4:
      ret = vlVdpDecoderRenderMpeg4(&desc.mpeg4, (VdpPictureInfoMPEG4Part2 *)picture_info);
      break;
   case PIPE_VIDEO_CODEC_VC1:
      if (dec->profile == PIPE_VIDEO_PROFILE_VC1_ADVANCED)
         vlVdpDecoderFixVC1Startcode(&bitstream_buffer_count, buffers, sizes);
      ret = vlVdpDecoderRenderVC1(&desc.vc1, (VdpPictureInfoVC1 *)picture_info);
      break;
   case PIPE_VIDEO_CODEC_MPEG4_AVC:
      ret = vlVdpDecoderRenderH264(&desc.h264, (VdpPictureInfoH264 *)picture_info);
      break;
   default:
      pipe_mutex_unlock(vlsurf->device->mutex);
      return VDP_STATUS_INVALID_DECODER_PROFILE;
   }

   if (ret != VDP_STATUS_OK) {
      pipe_mutex_unlock(vlsurf->device->mutex);
      return ret;
   }

   dec->begin_frame(dec, vlsurf->video_buffer, &desc.base);
   dec->decode_bitstream(dec, vlsurf->video_buffer, &desc.base, bitstream_buffer_count, buffers, sizes);
   dec->end_frame(dec, vlsurf->video_buffer, &desc.base);
   pipe_mutex_unlock(vlsurf->device->mutex);
   return ret;
}
