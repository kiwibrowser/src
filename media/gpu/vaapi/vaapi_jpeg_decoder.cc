// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vaapi_jpeg_decoder.h"

#include <stddef.h>
#include <string.h>

#include "base/logging.h"
#include "base/macros.h"
#include "media/filters/jpeg_parser.h"

namespace media {

// VAAPI only support subset of JPEG profiles. This function determines a given
// parsed JPEG result is supported or not.
static bool IsVaapiSupportedJpeg(const JpegParseResult& jpeg) {
  if (jpeg.frame_header.visible_width < 1 ||
      jpeg.frame_header.visible_height < 1) {
    DLOG(ERROR) << "width(" << jpeg.frame_header.visible_width
                << ") and height(" << jpeg.frame_header.visible_height
                << ") should be at least 1";
    return false;
  }

  // Size 64k*64k is the maximum in the JPEG standard. VAAPI doesn't support
  // resolutions larger than 16k*16k.
  const int kMaxDimension = 16384;
  if (jpeg.frame_header.coded_width > kMaxDimension ||
      jpeg.frame_header.coded_height > kMaxDimension) {
    DLOG(ERROR) << "VAAPI doesn't support size("
                << jpeg.frame_header.coded_width << "*"
                << jpeg.frame_header.coded_height << ") larger than "
                << kMaxDimension << "*" << kMaxDimension;
    return false;
  }

  if (jpeg.frame_header.num_components != 3) {
    DLOG(ERROR) << "VAAPI doesn't support num_components("
                << static_cast<int>(jpeg.frame_header.num_components)
                << ") != 3";
    return false;
  }

  if (jpeg.frame_header.components[0].horizontal_sampling_factor <
          jpeg.frame_header.components[1].horizontal_sampling_factor ||
      jpeg.frame_header.components[0].horizontal_sampling_factor <
          jpeg.frame_header.components[2].horizontal_sampling_factor) {
    DLOG(ERROR) << "VAAPI doesn't supports horizontal sampling factor of Y"
                << " smaller than Cb and Cr";
    return false;
  }

  if (jpeg.frame_header.components[0].vertical_sampling_factor <
          jpeg.frame_header.components[1].vertical_sampling_factor ||
      jpeg.frame_header.components[0].vertical_sampling_factor <
          jpeg.frame_header.components[2].vertical_sampling_factor) {
    DLOG(ERROR) << "VAAPI doesn't supports vertical sampling factor of Y"
                << " smaller than Cb and Cr";
    return false;
  }

  return true;
}

static void FillPictureParameters(
    const JpegFrameHeader& frame_header,
    VAPictureParameterBufferJPEGBaseline* pic_param) {
  memset(pic_param, 0, sizeof(*pic_param));
  pic_param->picture_width = frame_header.coded_width;
  pic_param->picture_height = frame_header.coded_height;
  pic_param->num_components = frame_header.num_components;

  for (int i = 0; i < pic_param->num_components; i++) {
    pic_param->components[i].component_id = frame_header.components[i].id;
    pic_param->components[i].h_sampling_factor =
        frame_header.components[i].horizontal_sampling_factor;
    pic_param->components[i].v_sampling_factor =
        frame_header.components[i].vertical_sampling_factor;
    pic_param->components[i].quantiser_table_selector =
        frame_header.components[i].quantization_table_selector;
  }
}

static void FillIQMatrix(const JpegQuantizationTable* q_table,
                         VAIQMatrixBufferJPEGBaseline* iq_matrix) {
  memset(iq_matrix, 0, sizeof(*iq_matrix));
  static_assert(kJpegMaxQuantizationTableNum ==
                    arraysize(iq_matrix->load_quantiser_table),
                "max number of quantization table mismatched");
  for (size_t i = 0; i < kJpegMaxQuantizationTableNum; i++) {
    if (!q_table[i].valid)
      continue;
    iq_matrix->load_quantiser_table[i] = 1;
    static_assert(
        arraysize(iq_matrix->quantiser_table[i]) == arraysize(q_table[i].value),
        "number of quantization entries mismatched");
    for (size_t j = 0; j < arraysize(q_table[i].value); j++)
      iq_matrix->quantiser_table[i][j] = q_table[i].value[j];
  }
}

static void FillHuffmanTable(const JpegHuffmanTable* dc_table,
                             const JpegHuffmanTable* ac_table,
                             VAHuffmanTableBufferJPEGBaseline* huffman_table) {
  memset(huffman_table, 0, sizeof(*huffman_table));
  // Use default huffman tables if not specified in header.
  bool has_huffman_table = false;
  for (size_t i = 0; i < kJpegMaxHuffmanTableNumBaseline; i++) {
    if (dc_table[i].valid || ac_table[i].valid) {
      has_huffman_table = true;
      break;
    }
  }
  if (!has_huffman_table) {
    dc_table = kDefaultDcTable;
    ac_table = kDefaultAcTable;
  }

  static_assert(kJpegMaxHuffmanTableNumBaseline ==
                    arraysize(huffman_table->load_huffman_table),
                "max number of huffman table mismatched");
  static_assert(sizeof(huffman_table->huffman_table[0].num_dc_codes) ==
                    sizeof(dc_table[0].code_length),
                "size of huffman table code length mismatch");
  static_assert(sizeof(huffman_table->huffman_table[0].dc_values[0]) ==
                    sizeof(dc_table[0].code_value[0]),
                "size of huffman table code value mismatch");
  for (size_t i = 0; i < kJpegMaxHuffmanTableNumBaseline; i++) {
    if (!dc_table[i].valid || !ac_table[i].valid)
      continue;
    huffman_table->load_huffman_table[i] = 1;

    memcpy(huffman_table->huffman_table[i].num_dc_codes,
           dc_table[i].code_length,
           sizeof(huffman_table->huffman_table[i].num_dc_codes));
    memcpy(huffman_table->huffman_table[i].dc_values, dc_table[i].code_value,
           sizeof(huffman_table->huffman_table[i].dc_values));
    memcpy(huffman_table->huffman_table[i].num_ac_codes,
           ac_table[i].code_length,
           sizeof(huffman_table->huffman_table[i].num_ac_codes));
    memcpy(huffman_table->huffman_table[i].ac_values, ac_table[i].code_value,
           sizeof(huffman_table->huffman_table[i].ac_values));
  }
}

static void FillSliceParameters(
    const JpegParseResult& parse_result,
    VASliceParameterBufferJPEGBaseline* slice_param) {
  memset(slice_param, 0, sizeof(*slice_param));
  slice_param->slice_data_size = parse_result.data_size;
  slice_param->slice_data_offset = 0;
  slice_param->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
  slice_param->slice_horizontal_position = 0;
  slice_param->slice_vertical_position = 0;
  slice_param->num_components = parse_result.scan.num_components;
  for (int i = 0; i < slice_param->num_components; i++) {
    slice_param->components[i].component_selector =
        parse_result.scan.components[i].component_selector;
    slice_param->components[i].dc_table_selector =
        parse_result.scan.components[i].dc_selector;
    slice_param->components[i].ac_table_selector =
        parse_result.scan.components[i].ac_selector;
  }
  slice_param->restart_interval = parse_result.restart_interval;

  // Cast to int to prevent overflow.
  int max_h_factor =
      parse_result.frame_header.components[0].horizontal_sampling_factor;
  int max_v_factor =
      parse_result.frame_header.components[0].vertical_sampling_factor;
  int mcu_cols = parse_result.frame_header.coded_width / (max_h_factor * 8);
  DCHECK_GT(mcu_cols, 0);
  int mcu_rows = parse_result.frame_header.coded_height / (max_v_factor * 8);
  DCHECK_GT(mcu_rows, 0);
  slice_param->num_mcus = mcu_rows * mcu_cols;
}

// static
bool VaapiJpegDecoder::Decode(VaapiWrapper* vaapi_wrapper,
                              const JpegParseResult& parse_result,
                              VASurfaceID va_surface) {
  DCHECK_NE(va_surface, VA_INVALID_SURFACE);
  if (!IsVaapiSupportedJpeg(parse_result))
    return false;

  // Set picture parameters.
  VAPictureParameterBufferJPEGBaseline pic_param;
  FillPictureParameters(parse_result.frame_header, &pic_param);
  if (!vaapi_wrapper->SubmitBuffer(VAPictureParameterBufferType,
                                   sizeof(pic_param), &pic_param))
    return false;

  // Set quantization table.
  VAIQMatrixBufferJPEGBaseline iq_matrix;
  FillIQMatrix(parse_result.q_table, &iq_matrix);
  if (!vaapi_wrapper->SubmitBuffer(VAIQMatrixBufferType, sizeof(iq_matrix),
                                   &iq_matrix))
    return false;

  // Set huffman table.
  VAHuffmanTableBufferJPEGBaseline huffman_table;
  FillHuffmanTable(parse_result.dc_table, parse_result.ac_table,
                   &huffman_table);
  if (!vaapi_wrapper->SubmitBuffer(VAHuffmanTableBufferType,
                                   sizeof(huffman_table), &huffman_table))
    return false;

  // Set slice parameters.
  VASliceParameterBufferJPEGBaseline slice_param;
  FillSliceParameters(parse_result, &slice_param);
  if (!vaapi_wrapper->SubmitBuffer(VASliceParameterBufferType,
                                   sizeof(slice_param), &slice_param))
    return false;

  // Set scan data.
  if (!vaapi_wrapper->SubmitBuffer(VASliceDataBufferType,
                                   parse_result.data_size,
                                   const_cast<char*>(parse_result.data)))
    return false;

  if (!vaapi_wrapper->ExecuteAndDestroyPendingBuffers(va_surface))
    return false;

  return true;
}

}  // namespace media
