#ifdef GET_NEON_BUILTINS
BUILTIN(__builtin_neon_vabd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vabdd_f64, "ddd", "n")
BUILTIN(__builtin_neon_vabdq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vabds_f32, "fff", "n")
BUILTIN(__builtin_neon_vabs_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vabsd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vabsq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vaddd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vaddd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vaddhn_v, "V8ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vaddlv_s16, "iV4s", "n")
BUILTIN(__builtin_neon_vaddlv_s32, "WiV2i", "n")
BUILTIN(__builtin_neon_vaddlv_s8, "sV8Sc", "n")
BUILTIN(__builtin_neon_vaddlv_u16, "UiV4Us", "n")
BUILTIN(__builtin_neon_vaddlv_u32, "UWiV2Ui", "n")
BUILTIN(__builtin_neon_vaddlv_u8, "UsV8Sc", "n")
BUILTIN(__builtin_neon_vaddlvq_s16, "iV8s", "n")
BUILTIN(__builtin_neon_vaddlvq_s32, "WiV4i", "n")
BUILTIN(__builtin_neon_vaddlvq_s8, "sV16Sc", "n")
BUILTIN(__builtin_neon_vaddlvq_u16, "UiV8Us", "n")
BUILTIN(__builtin_neon_vaddlvq_u32, "UWiV4Ui", "n")
BUILTIN(__builtin_neon_vaddlvq_u8, "UsV16Sc", "n")
BUILTIN(__builtin_neon_vaddv_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vaddv_s16, "sV4s", "n")
BUILTIN(__builtin_neon_vaddv_s32, "iV2i", "n")
BUILTIN(__builtin_neon_vaddv_s8, "ScV8Sc", "n")
BUILTIN(__builtin_neon_vaddv_u16, "UsV4Us", "n")
BUILTIN(__builtin_neon_vaddv_u32, "UiV2Ui", "n")
BUILTIN(__builtin_neon_vaddv_u8, "ScV8Sc", "n")
BUILTIN(__builtin_neon_vaddvq_f32, "fV4f", "n")
BUILTIN(__builtin_neon_vaddvq_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vaddvq_s16, "sV8s", "n")
BUILTIN(__builtin_neon_vaddvq_s32, "iV4i", "n")
BUILTIN(__builtin_neon_vaddvq_s64, "WiV2Wi", "n")
BUILTIN(__builtin_neon_vaddvq_s8, "ScV16Sc", "n")
BUILTIN(__builtin_neon_vaddvq_u16, "UsV8Us", "n")
BUILTIN(__builtin_neon_vaddvq_u32, "UiV4Ui", "n")
BUILTIN(__builtin_neon_vaddvq_u64, "UWiV2UWi", "n")
BUILTIN(__builtin_neon_vaddvq_u8, "ScV16Sc", "n")
BUILTIN(__builtin_neon_vaesdq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vaeseq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vaesimcq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vaesmcq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vbsl_v, "V8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vbslq_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcage_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcaged_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcageq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcages_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vcagt_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcagtd_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcagtq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcagts_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vcale_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcaled_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcaleq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcales_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vcalt_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcaltd_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcaltq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcalts_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vceqd_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vceqd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vceqd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vceqs_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vceqz_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vceqzd_f64, "UWid", "n")
BUILTIN(__builtin_neon_vceqzd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vceqzd_u64, "UWiUWi", "n")
BUILTIN(__builtin_neon_vceqzq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vceqzs_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcged_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcged_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vcged_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vcges_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vcgez_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcgezd_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcgezd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vcgezq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcgezs_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcgtd_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcgtd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vcgtd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vcgts_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vcgtz_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcgtzd_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcgtzd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vcgtzq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcgtzs_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcled_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcled_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vcled_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vcles_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vclez_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vclezd_f64, "UWid", "n")
BUILTIN(__builtin_neon_vclezd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vclezq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vclezs_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcls_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vclsq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcltd_f64, "UWidd", "n")
BUILTIN(__builtin_neon_vcltd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vcltd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vclts_f32, "Uiff", "n")
BUILTIN(__builtin_neon_vcltz_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcltzd_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcltzd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vcltzq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcltzs_f32, "Uif", "n")
BUILTIN(__builtin_neon_vclz_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vclzq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcnt_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcntq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvt_f16_f32, "V8ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvt_f32_f16, "V16ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_f32_f64, "V8ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvt_f32_v, "V2fV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_f64_f32, "V16ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_f64_v, "V1dV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_n_f32_v, "V2fV8ScIii", "n")
BUILTIN(__builtin_neon_vcvt_n_f64_v, "V1dV8ScIii", "n")
BUILTIN(__builtin_neon_vcvt_n_s32_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vcvt_n_s64_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vcvt_n_u32_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vcvt_n_u64_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vcvt_s32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_s64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_u32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvt_u64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvta_s32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvta_s64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvta_u32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvta_u64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtad_s64_f64, "Wid", "n")
BUILTIN(__builtin_neon_vcvtad_u64_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcvtaq_s32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtaq_s64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtaq_u32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtaq_u64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtas_s32_f32, "if", "n")
BUILTIN(__builtin_neon_vcvtas_u32_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcvtd_f64_s64, "dWi", "n")
BUILTIN(__builtin_neon_vcvtd_f64_u64, "dUWi", "n")
BUILTIN(__builtin_neon_vcvtd_n_f64_s64, "dWiIi", "n")
BUILTIN(__builtin_neon_vcvtd_n_f64_u64, "dUWiIi", "n")
BUILTIN(__builtin_neon_vcvtd_n_s64_f64, "WidIi", "n")
BUILTIN(__builtin_neon_vcvtd_n_u64_f64, "UWidIi", "n")
BUILTIN(__builtin_neon_vcvtd_s64_f64, "Wid", "n")
BUILTIN(__builtin_neon_vcvtd_u64_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcvtm_s32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtm_s64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtm_u32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtm_u64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtmd_s64_f64, "Wid", "n")
BUILTIN(__builtin_neon_vcvtmd_u64_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcvtmq_s32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtmq_s64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtmq_u32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtmq_u64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtms_s32_f32, "if", "n")
BUILTIN(__builtin_neon_vcvtms_u32_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcvtn_s32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtn_s64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtn_u32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtn_u64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtnd_s64_f64, "Wid", "n")
BUILTIN(__builtin_neon_vcvtnd_u64_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcvtnq_s32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtnq_s64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtnq_u32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtnq_u64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtns_s32_f32, "if", "n")
BUILTIN(__builtin_neon_vcvtns_u32_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcvtp_s32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtp_s64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtp_u32_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtp_u64_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vcvtpd_s64_f64, "Wid", "n")
BUILTIN(__builtin_neon_vcvtpd_u64_f64, "UWid", "n")
BUILTIN(__builtin_neon_vcvtpq_s32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtpq_s64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtpq_u32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtpq_u64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtps_s32_f32, "if", "n")
BUILTIN(__builtin_neon_vcvtps_u32_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcvtq_f32_v, "V4fV16Sci", "n")
BUILTIN(__builtin_neon_vcvtq_f64_v, "V2dV16Sci", "n")
BUILTIN(__builtin_neon_vcvtq_n_f32_v, "V4fV16ScIii", "n")
BUILTIN(__builtin_neon_vcvtq_n_f64_v, "V2dV16ScIii", "n")
BUILTIN(__builtin_neon_vcvtq_n_s32_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vcvtq_n_s64_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vcvtq_n_u32_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vcvtq_n_u64_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vcvtq_s32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtq_s64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtq_u32_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvtq_u64_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vcvts_f32_s32, "fi", "n")
BUILTIN(__builtin_neon_vcvts_f32_u32, "fUi", "n")
BUILTIN(__builtin_neon_vcvts_n_f32_s32, "fiIi", "n")
BUILTIN(__builtin_neon_vcvts_n_f32_u32, "fUiIi", "n")
BUILTIN(__builtin_neon_vcvts_n_s32_f32, "ifIi", "n")
BUILTIN(__builtin_neon_vcvts_n_u32_f32, "UifIi", "n")
BUILTIN(__builtin_neon_vcvts_s32_f32, "if", "n")
BUILTIN(__builtin_neon_vcvts_u32_f32, "Uif", "n")
BUILTIN(__builtin_neon_vcvtx_f32_v, "V2fV16Sci", "n")
BUILTIN(__builtin_neon_vcvtxd_f32_f64, "fd", "n")
BUILTIN(__builtin_neon_vdupb_lane_i8, "ScV8ScIi", "n")
BUILTIN(__builtin_neon_vdupb_laneq_i8, "ScV16ScIi", "n")
BUILTIN(__builtin_neon_vdupd_lane_f64, "dV1dIi", "n")
BUILTIN(__builtin_neon_vdupd_lane_i64, "UWiV1WiIi", "n")
BUILTIN(__builtin_neon_vdupd_laneq_f64, "dV2dIi", "n")
BUILTIN(__builtin_neon_vdupd_laneq_i64, "UWiV2WiIi", "n")
BUILTIN(__builtin_neon_vduph_lane_i16, "UsV4sIi", "n")
BUILTIN(__builtin_neon_vduph_laneq_i16, "UsV8sIi", "n")
BUILTIN(__builtin_neon_vdups_lane_f32, "fV2fIi", "n")
BUILTIN(__builtin_neon_vdups_lane_i32, "UiV2iIi", "n")
BUILTIN(__builtin_neon_vdups_laneq_f32, "fV4fIi", "n")
BUILTIN(__builtin_neon_vdups_laneq_i32, "UiV4iIi", "n")
BUILTIN(__builtin_neon_vext_v, "V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vextq_v, "V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vfma_lane_v, "V8ScV8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vfma_laneq_v, "V8ScV8ScV8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vfma_v, "V8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vfmad_lane_f64, "dddV1dIi", "n")
BUILTIN(__builtin_neon_vfmad_laneq_f64, "dddV2dIi", "n")
BUILTIN(__builtin_neon_vfmaq_lane_v, "V16ScV16ScV16ScV8ScIii", "n")
BUILTIN(__builtin_neon_vfmaq_laneq_v, "V16ScV16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vfmaq_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vfmas_lane_f32, "fffV2fIi", "n")
BUILTIN(__builtin_neon_vfmas_laneq_f32, "fffV4fIi", "n")
BUILTIN(__builtin_neon_vget_lane_f32, "fV2fIi", "n")
BUILTIN(__builtin_neon_vget_lane_f64, "dV1dIi", "n")
BUILTIN(__builtin_neon_vget_lane_i16, "UsV4sIi", "n")
BUILTIN(__builtin_neon_vget_lane_i32, "UiV2iIi", "n")
BUILTIN(__builtin_neon_vget_lane_i64, "UWiV1WiIi", "n")
BUILTIN(__builtin_neon_vget_lane_i8, "ScV8ScIi", "n")
BUILTIN(__builtin_neon_vgetq_lane_f32, "fV4fIi", "n")
BUILTIN(__builtin_neon_vgetq_lane_f64, "dV2dIi", "n")
BUILTIN(__builtin_neon_vgetq_lane_i16, "UsV8sIi", "n")
BUILTIN(__builtin_neon_vgetq_lane_i32, "UiV4iIi", "n")
BUILTIN(__builtin_neon_vgetq_lane_i64, "UWiV2WiIi", "n")
BUILTIN(__builtin_neon_vgetq_lane_i8, "ScV16ScIi", "n")
BUILTIN(__builtin_neon_vhadd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vhaddq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vhsub_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vhsubq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vld1_dup_v, "V8ScvC*i", "n")
BUILTIN(__builtin_neon_vld1_lane_v, "V8ScvC*V8ScIii", "n")
BUILTIN(__builtin_neon_vld1_v, "V8ScvC*i", "n")
BUILTIN(__builtin_neon_vld1_x2_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld1_x3_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld1_x4_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld1q_dup_v, "V16ScvC*i", "n")
BUILTIN(__builtin_neon_vld1q_lane_v, "V16ScvC*V16ScIii", "n")
BUILTIN(__builtin_neon_vld1q_v, "V16ScvC*i", "n")
BUILTIN(__builtin_neon_vld1q_x2_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld1q_x3_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld1q_x4_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld2_dup_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld2_lane_v, "vv*vC*V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vld2_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld2q_dup_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld2q_lane_v, "vv*vC*V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vld2q_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld3_dup_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld3_lane_v, "vv*vC*V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vld3_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld3q_dup_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld3q_lane_v, "vv*vC*V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vld3q_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld4_dup_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld4_lane_v, "vv*vC*V8ScV8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vld4_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld4q_dup_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vld4q_lane_v, "vv*vC*V16ScV16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vld4q_v, "vv*vC*i", "n")
BUILTIN(__builtin_neon_vldrq_p128, "ULLLivC*", "n")
BUILTIN(__builtin_neon_vmax_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vmaxnm_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vmaxnmq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vmaxnmv_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vmaxnmvq_f32, "fV4f", "n")
BUILTIN(__builtin_neon_vmaxnmvq_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vmaxq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vmaxv_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vmaxv_s16, "sV4s", "n")
BUILTIN(__builtin_neon_vmaxv_s32, "iV2i", "n")
BUILTIN(__builtin_neon_vmaxv_s8, "ScV8Sc", "n")
BUILTIN(__builtin_neon_vmaxv_u16, "UsV4Us", "n")
BUILTIN(__builtin_neon_vmaxv_u32, "UiV2Ui", "n")
BUILTIN(__builtin_neon_vmaxv_u8, "ScV8Sc", "n")
BUILTIN(__builtin_neon_vmaxvq_f32, "fV4f", "n")
BUILTIN(__builtin_neon_vmaxvq_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vmaxvq_s16, "sV8s", "n")
BUILTIN(__builtin_neon_vmaxvq_s32, "iV4i", "n")
BUILTIN(__builtin_neon_vmaxvq_s8, "ScV16Sc", "n")
BUILTIN(__builtin_neon_vmaxvq_u16, "UsV8Us", "n")
BUILTIN(__builtin_neon_vmaxvq_u32, "UiV4Ui", "n")
BUILTIN(__builtin_neon_vmaxvq_u8, "ScV16Sc", "n")
BUILTIN(__builtin_neon_vmin_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vminnm_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vminnmq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vminnmv_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vminnmvq_f32, "fV4f", "n")
BUILTIN(__builtin_neon_vminnmvq_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vminq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vminv_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vminv_s16, "sV4s", "n")
BUILTIN(__builtin_neon_vminv_s32, "iV2i", "n")
BUILTIN(__builtin_neon_vminv_s8, "ScV8Sc", "n")
BUILTIN(__builtin_neon_vminv_u16, "UsV4Us", "n")
BUILTIN(__builtin_neon_vminv_u32, "UiV2Ui", "n")
BUILTIN(__builtin_neon_vminv_u8, "ScV8Sc", "n")
BUILTIN(__builtin_neon_vminvq_f32, "fV4f", "n")
BUILTIN(__builtin_neon_vminvq_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vminvq_s16, "sV8s", "n")
BUILTIN(__builtin_neon_vminvq_s32, "iV4i", "n")
BUILTIN(__builtin_neon_vminvq_s8, "ScV16Sc", "n")
BUILTIN(__builtin_neon_vminvq_u16, "UsV8Us", "n")
BUILTIN(__builtin_neon_vminvq_u32, "UiV4Ui", "n")
BUILTIN(__builtin_neon_vminvq_u8, "ScV16Sc", "n")
BUILTIN(__builtin_neon_vmovl_v, "V16ScV8Sci", "n")
BUILTIN(__builtin_neon_vmovn_v, "V8ScV16Sci", "n")
BUILTIN(__builtin_neon_vmul_lane_v, "V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vmul_laneq_v, "V8ScV8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vmul_n_f64, "V1dV1dd", "n")
BUILTIN(__builtin_neon_vmul_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vmull_p64, "ULLLiUWiUWi", "n")
BUILTIN(__builtin_neon_vmull_v, "V16ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vmulq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vmulx_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vmulxd_f64, "ddd", "n")
BUILTIN(__builtin_neon_vmulxq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vmulxs_f32, "fff", "n")
BUILTIN(__builtin_neon_vnegd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vpadal_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpadalq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpadd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpaddd_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vpaddd_s64, "WiV2Wi", "n")
BUILTIN(__builtin_neon_vpaddd_u64, "UWiV2UWi", "n")
BUILTIN(__builtin_neon_vpaddl_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpaddlq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpaddq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpadds_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vpmax_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpmaxnm_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpmaxnmq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpmaxnmqd_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vpmaxnms_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vpmaxq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpmaxqd_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vpmaxs_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vpmin_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpminnm_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vpminnmq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpminnmqd_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vpminnms_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vpminq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vpminqd_f64, "dV2d", "n")
BUILTIN(__builtin_neon_vpmins_f32, "fV2f", "n")
BUILTIN(__builtin_neon_vqabs_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqabsb_s8, "ScSc", "n")
BUILTIN(__builtin_neon_vqabsd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vqabsh_s16, "ss", "n")
BUILTIN(__builtin_neon_vqabsq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqabss_s32, "ii", "n")
BUILTIN(__builtin_neon_vqadd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqaddb_s8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqaddb_u8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqaddd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vqaddd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vqaddh_s16, "sss", "n")
BUILTIN(__builtin_neon_vqaddh_u16, "UsUsUs", "n")
BUILTIN(__builtin_neon_vqaddq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqadds_s32, "iii", "n")
BUILTIN(__builtin_neon_vqadds_u32, "UiUiUi", "n")
BUILTIN(__builtin_neon_vqdmlal_v, "V16ScV16ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqdmlalh_lane_s16, "iisV4sIi", "n")
BUILTIN(__builtin_neon_vqdmlalh_laneq_s16, "iisV8sIi", "n")
BUILTIN(__builtin_neon_vqdmlalh_s16, "iiss", "n")
BUILTIN(__builtin_neon_vqdmlals_lane_s32, "WiWiiV2iIi", "n")
BUILTIN(__builtin_neon_vqdmlals_laneq_s32, "WiWiiV4iIi", "n")
BUILTIN(__builtin_neon_vqdmlals_s32, "WiWiii", "n")
BUILTIN(__builtin_neon_vqdmlsl_v, "V16ScV16ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqdmlslh_lane_s16, "iisV4sIi", "n")
BUILTIN(__builtin_neon_vqdmlslh_laneq_s16, "iisV8sIi", "n")
BUILTIN(__builtin_neon_vqdmlslh_s16, "iiss", "n")
BUILTIN(__builtin_neon_vqdmlsls_lane_s32, "WiWiiV2iIi", "n")
BUILTIN(__builtin_neon_vqdmlsls_laneq_s32, "WiWiiV4iIi", "n")
BUILTIN(__builtin_neon_vqdmlsls_s32, "WiWiii", "n")
BUILTIN(__builtin_neon_vqdmulh_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqdmulhh_s16, "sss", "n")
BUILTIN(__builtin_neon_vqdmulhq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqdmulhs_s32, "iii", "n")
BUILTIN(__builtin_neon_vqdmull_v, "V16ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqdmullh_s16, "iss", "n")
BUILTIN(__builtin_neon_vqdmulls_s32, "Wiii", "n")
BUILTIN(__builtin_neon_vqmovn_v, "V8ScV16Sci", "n")
BUILTIN(__builtin_neon_vqmovnd_s64, "iWi", "n")
BUILTIN(__builtin_neon_vqmovnd_u64, "UiUWi", "n")
BUILTIN(__builtin_neon_vqmovnh_s16, "Scs", "n")
BUILTIN(__builtin_neon_vqmovnh_u16, "ScUs", "n")
BUILTIN(__builtin_neon_vqmovns_s32, "si", "n")
BUILTIN(__builtin_neon_vqmovns_u32, "UsUi", "n")
BUILTIN(__builtin_neon_vqmovun_v, "V8ScV16Sci", "n")
BUILTIN(__builtin_neon_vqmovund_s64, "iWi", "n")
BUILTIN(__builtin_neon_vqmovunh_s16, "Scs", "n")
BUILTIN(__builtin_neon_vqmovuns_s32, "si", "n")
BUILTIN(__builtin_neon_vqneg_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqnegb_s8, "ScSc", "n")
BUILTIN(__builtin_neon_vqnegd_s64, "WiWi", "n")
BUILTIN(__builtin_neon_vqnegh_s16, "ss", "n")
BUILTIN(__builtin_neon_vqnegq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqnegs_s32, "ii", "n")
BUILTIN(__builtin_neon_vqrdmulh_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqrdmulhh_s16, "sss", "n")
BUILTIN(__builtin_neon_vqrdmulhq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqrdmulhs_s32, "iii", "n")
BUILTIN(__builtin_neon_vqrshl_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqrshlb_s8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqrshlb_u8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqrshld_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vqrshld_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vqrshlh_s16, "sss", "n")
BUILTIN(__builtin_neon_vqrshlh_u16, "UsUsUs", "n")
BUILTIN(__builtin_neon_vqrshlq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqrshls_s32, "iii", "n")
BUILTIN(__builtin_neon_vqrshls_u32, "UiUiUi", "n")
BUILTIN(__builtin_neon_vqrshrn_n_v, "V8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vqrshrnd_n_s64, "iWiIi", "n")
BUILTIN(__builtin_neon_vqrshrnd_n_u64, "UiUWiIi", "n")
BUILTIN(__builtin_neon_vqrshrnh_n_s16, "ScsIi", "n")
BUILTIN(__builtin_neon_vqrshrnh_n_u16, "ScUsIi", "n")
BUILTIN(__builtin_neon_vqrshrns_n_s32, "siIi", "n")
BUILTIN(__builtin_neon_vqrshrns_n_u32, "UsUiIi", "n")
BUILTIN(__builtin_neon_vqrshrun_n_v, "V8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vqrshrund_n_s64, "iWiIi", "n")
BUILTIN(__builtin_neon_vqrshrunh_n_s16, "ScsIi", "n")
BUILTIN(__builtin_neon_vqrshruns_n_s32, "siIi", "n")
BUILTIN(__builtin_neon_vqshl_n_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vqshl_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqshlb_n_s8, "ScScIi", "n")
BUILTIN(__builtin_neon_vqshlb_n_u8, "ScScIi", "n")
BUILTIN(__builtin_neon_vqshlb_s8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqshlb_u8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqshld_n_s64, "WiWiIi", "n")
BUILTIN(__builtin_neon_vqshld_n_u64, "UWiUWiIi", "n")
BUILTIN(__builtin_neon_vqshld_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vqshld_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vqshlh_n_s16, "ssIi", "n")
BUILTIN(__builtin_neon_vqshlh_n_u16, "UsUsIi", "n")
BUILTIN(__builtin_neon_vqshlh_s16, "sss", "n")
BUILTIN(__builtin_neon_vqshlh_u16, "UsUsUs", "n")
BUILTIN(__builtin_neon_vqshlq_n_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vqshlq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqshls_n_s32, "iiIi", "n")
BUILTIN(__builtin_neon_vqshls_n_u32, "UiUiIi", "n")
BUILTIN(__builtin_neon_vqshls_s32, "iii", "n")
BUILTIN(__builtin_neon_vqshls_u32, "UiUiUi", "n")
BUILTIN(__builtin_neon_vqshlu_n_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vqshlub_n_s8, "ScScIi", "n")
BUILTIN(__builtin_neon_vqshlud_n_s64, "WiWiIi", "n")
BUILTIN(__builtin_neon_vqshluh_n_s16, "ssIi", "n")
BUILTIN(__builtin_neon_vqshluq_n_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vqshlus_n_s32, "iiIi", "n")
BUILTIN(__builtin_neon_vqshrn_n_v, "V8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vqshrnd_n_s64, "iWiIi", "n")
BUILTIN(__builtin_neon_vqshrnd_n_u64, "UiUWiIi", "n")
BUILTIN(__builtin_neon_vqshrnh_n_s16, "ScsIi", "n")
BUILTIN(__builtin_neon_vqshrnh_n_u16, "ScUsIi", "n")
BUILTIN(__builtin_neon_vqshrns_n_s32, "siIi", "n")
BUILTIN(__builtin_neon_vqshrns_n_u32, "UsUiIi", "n")
BUILTIN(__builtin_neon_vqshrun_n_v, "V8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vqshrund_n_s64, "iWiIi", "n")
BUILTIN(__builtin_neon_vqshrunh_n_s16, "ScsIi", "n")
BUILTIN(__builtin_neon_vqshruns_n_s32, "siIi", "n")
BUILTIN(__builtin_neon_vqsub_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vqsubb_s8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqsubb_u8, "ScScSc", "n")
BUILTIN(__builtin_neon_vqsubd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vqsubd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vqsubh_s16, "sss", "n")
BUILTIN(__builtin_neon_vqsubh_u16, "UsUsUs", "n")
BUILTIN(__builtin_neon_vqsubq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqsubs_s32, "iii", "n")
BUILTIN(__builtin_neon_vqsubs_u32, "UiUiUi", "n")
BUILTIN(__builtin_neon_vqtbl1_v, "V8ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbl1q_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbl2_v, "V8ScV16ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbl2q_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbl3_v, "V8ScV16ScV16ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbl3q_v, "V16ScV16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbl4_v, "V8ScV16ScV16ScV16ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbl4q_v, "V16ScV16ScV16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbx1_v, "V8ScV8ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbx1q_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbx2_v, "V8ScV8ScV16ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbx2q_v, "V16ScV16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbx3_v, "V8ScV8ScV16ScV16ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbx3q_v, "V16ScV16ScV16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vqtbx4_v, "V8ScV8ScV16ScV16ScV16ScV16ScV8Sci", "n")
BUILTIN(__builtin_neon_vqtbx4q_v, "V16ScV16ScV16ScV16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vraddhn_v, "V8ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrbit_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrbitq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrecpe_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrecped_f64, "dd", "n")
BUILTIN(__builtin_neon_vrecpeq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrecpes_f32, "ff", "n")
BUILTIN(__builtin_neon_vrecps_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrecpsd_f64, "ddd", "n")
BUILTIN(__builtin_neon_vrecpsq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrecpss_f32, "fff", "n")
BUILTIN(__builtin_neon_vrecpxd_f64, "dd", "n")
BUILTIN(__builtin_neon_vrecpxs_f32, "ff", "n")
BUILTIN(__builtin_neon_vrhadd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrhaddq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrnd_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrnda_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrndaq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrndi_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrndiq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrndm_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrndmq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrndn_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrndnq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrndp_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrndpq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrndq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrndx_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrndxq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrshl_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrshld_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vrshld_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vrshlq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrshr_n_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vrshrd_n_s64, "WiWiIi", "n")
BUILTIN(__builtin_neon_vrshrd_n_u64, "UWiUWiIi", "n")
BUILTIN(__builtin_neon_vrshrn_n_v, "V8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vrshrq_n_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vrsqrte_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrsqrted_f64, "dd", "n")
BUILTIN(__builtin_neon_vrsqrteq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrsqrtes_f32, "ff", "n")
BUILTIN(__builtin_neon_vrsqrts_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vrsqrtsd_f64, "ddd", "n")
BUILTIN(__builtin_neon_vrsqrtsq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vrsqrtss_f32, "fff", "n")
BUILTIN(__builtin_neon_vrsra_n_v, "V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vrsrad_n_s64, "WiWiWiIi", "n")
BUILTIN(__builtin_neon_vrsrad_n_u64, "UWiUWiUWiIi", "n")
BUILTIN(__builtin_neon_vrsraq_n_v, "V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vrsubhn_v, "V8ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vset_lane_f32, "V2ffV2fIi", "n")
BUILTIN(__builtin_neon_vset_lane_f64, "V1ddV1dIi", "n")
BUILTIN(__builtin_neon_vset_lane_i16, "V4ssV4sIi", "n")
BUILTIN(__builtin_neon_vset_lane_i32, "V2iiV2iIi", "n")
BUILTIN(__builtin_neon_vset_lane_i64, "V1WiWiV1WiIi", "n")
BUILTIN(__builtin_neon_vset_lane_i8, "V8ScScV8ScIi", "n")
BUILTIN(__builtin_neon_vsetq_lane_f32, "V4ffV4fIi", "n")
BUILTIN(__builtin_neon_vsetq_lane_f64, "V2ddV2dIi", "n")
BUILTIN(__builtin_neon_vsetq_lane_i16, "V8ssV8sIi", "n")
BUILTIN(__builtin_neon_vsetq_lane_i32, "V4iiV4iIi", "n")
BUILTIN(__builtin_neon_vsetq_lane_i64, "V2WiWiV2WiIi", "n")
BUILTIN(__builtin_neon_vsetq_lane_i8, "V16ScScV16ScIi", "n")
BUILTIN(__builtin_neon_vsha1cq_u32, "V4iV4UiUiV4Ui", "n")
BUILTIN(__builtin_neon_vsha1h_u32, "UiUi", "n")
BUILTIN(__builtin_neon_vsha1mq_u32, "V4iV4UiUiV4Ui", "n")
BUILTIN(__builtin_neon_vsha1pq_u32, "V4iV4UiUiV4Ui", "n")
BUILTIN(__builtin_neon_vsha1su0q_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsha1su1q_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsha256h2q_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsha256hq_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsha256su0q_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsha256su1q_v, "V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vshl_n_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vshl_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vshld_n_s64, "WiWiIi", "n")
BUILTIN(__builtin_neon_vshld_n_u64, "UWiUWiIi", "n")
BUILTIN(__builtin_neon_vshld_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vshld_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vshll_n_v, "V16ScV8ScIii", "n")
BUILTIN(__builtin_neon_vshlq_n_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vshlq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vshr_n_v, "V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vshrd_n_s64, "WiWiIi", "n")
BUILTIN(__builtin_neon_vshrd_n_u64, "UWiUWiIi", "n")
BUILTIN(__builtin_neon_vshrn_n_v, "V8ScV16ScIii", "n")
BUILTIN(__builtin_neon_vshrq_n_v, "V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vsli_n_v, "V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vslid_n_s64, "WiWiWiIi", "n")
BUILTIN(__builtin_neon_vslid_n_u64, "UWiUWiUWiIi", "n")
BUILTIN(__builtin_neon_vsliq_n_v, "V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vsqadd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vsqaddb_u8, "ScScSc", "n")
BUILTIN(__builtin_neon_vsqaddd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vsqaddh_u16, "UsUsUs", "n")
BUILTIN(__builtin_neon_vsqaddq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsqadds_u32, "UiUiUi", "n")
BUILTIN(__builtin_neon_vsqrt_v, "V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vsqrtq_v, "V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vsra_n_v, "V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vsrad_n_s64, "WiWiWiIi", "n")
BUILTIN(__builtin_neon_vsrad_n_u64, "UWiUWiUWiIi", "n")
BUILTIN(__builtin_neon_vsraq_n_v, "V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vsri_n_v, "V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vsrid_n_s64, "WiWiWiIi", "n")
BUILTIN(__builtin_neon_vsrid_n_u64, "UWiUWiUWiIi", "n")
BUILTIN(__builtin_neon_vsriq_n_v, "V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vst1_lane_v, "vv*V8ScIii", "n")
BUILTIN(__builtin_neon_vst1_v, "vv*V8Sci", "n")
BUILTIN(__builtin_neon_vst1_x2_v, "vv*V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vst1_x3_v, "vv*V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vst1_x4_v, "vv*V8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vst1q_lane_v, "vv*V16ScIii", "n")
BUILTIN(__builtin_neon_vst1q_v, "vv*V16Sci", "n")
BUILTIN(__builtin_neon_vst1q_x2_v, "vv*V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vst1q_x3_v, "vv*V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vst1q_x4_v, "vv*V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vst2_lane_v, "vv*V8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vst2_v, "vv*V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vst2q_lane_v, "vv*V16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vst2q_v, "vv*V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vst3_lane_v, "vv*V8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vst3_v, "vv*V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vst3q_lane_v, "vv*V16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vst3q_v, "vv*V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vst4_lane_v, "vv*V8ScV8ScV8ScV8ScIii", "n")
BUILTIN(__builtin_neon_vst4_v, "vv*V8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vst4q_lane_v, "vv*V16ScV16ScV16ScV16ScIii", "n")
BUILTIN(__builtin_neon_vst4q_v, "vv*V16ScV16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vstrq_p128, "vv*ULLLi", "n")
BUILTIN(__builtin_neon_vsubd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vsubd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vsubhn_v, "V8ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vtbl1_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbl2_v, "V8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbl3_v, "V8ScV8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbl4_v, "V8ScV8ScV8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbx1_v, "V8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbx2_v, "V8ScV8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbx3_v, "V8ScV8ScV8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtbx4_v, "V8ScV8ScV8ScV8ScV8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtrn_v, "vv*V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtrnq_v, "vv*V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vtst_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vtstd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vtstd_u64, "UWiUWiUWi", "n")
BUILTIN(__builtin_neon_vtstq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vuqadd_v, "V8ScV8ScV8Sci", "n")
BUILTIN(__builtin_neon_vuqaddb_s8, "ScScSc", "n")
BUILTIN(__builtin_neon_vuqaddd_s64, "WiWiWi", "n")
BUILTIN(__builtin_neon_vuqaddh_s16, "sss", "n")
BUILTIN(__builtin_neon_vuqaddq_v, "V16ScV16ScV16Sci", "n")
BUILTIN(__builtin_neon_vuqadds_s32, "iii", "n")
BUILTIN(__builtin_neon_vuzp_v, "vv*V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vuzpq_v, "vv*V16ScV16Sci", "n")
BUILTIN(__builtin_neon_vzip_v, "vv*V8ScV8Sci", "n")
BUILTIN(__builtin_neon_vzipq_v, "vv*V16ScV16Sci", "n")
#endif

#ifdef GET_NEON_OVERLOAD_CHECK
case NEON::BI__builtin_neon_vabd_v: mask = 0x70607ULL; break;
case NEON::BI__builtin_neon_vabdq_v: mask = 0x7060700000000ULL; break;
case NEON::BI__builtin_neon_vabs_v: mask = 0x60FULL; break;
case NEON::BI__builtin_neon_vabsq_v: mask = 0x60F00000000ULL; break;
case NEON::BI__builtin_neon_vaddhn_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vaesdq_v: mask = 0x1000000000000ULL; break;
case NEON::BI__builtin_neon_vaeseq_v: mask = 0x1000000000000ULL; break;
case NEON::BI__builtin_neon_vaesimcq_v: mask = 0x1000000000000ULL; break;
case NEON::BI__builtin_neon_vaesmcq_v: mask = 0x1000000000000ULL; break;
case NEON::BI__builtin_neon_vbsl_v: mask = 0xF067FULL; break;
case NEON::BI__builtin_neon_vbslq_v: mask = 0xF067F00000000ULL; break;
case NEON::BI__builtin_neon_vcage_v: mask = 0xC0000ULL; break;
case NEON::BI__builtin_neon_vcageq_v: mask = 0xC000000000000ULL; break;
case NEON::BI__builtin_neon_vcagt_v: mask = 0xC0000ULL; break;
case NEON::BI__builtin_neon_vcagtq_v: mask = 0xC000000000000ULL; break;
case NEON::BI__builtin_neon_vcale_v: mask = 0xC0000ULL; break;
case NEON::BI__builtin_neon_vcaleq_v: mask = 0xC000000000000ULL; break;
case NEON::BI__builtin_neon_vcalt_v: mask = 0xC0000ULL; break;
case NEON::BI__builtin_neon_vcaltq_v: mask = 0xC000000000000ULL; break;
case NEON::BI__builtin_neon_vceqz_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vceqzq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vcgez_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vcgezq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vcgtz_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vcgtzq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vclez_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vclezq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vcls_v: mask = 0x7ULL; break;
case NEON::BI__builtin_neon_vclsq_v: mask = 0x700000000ULL; break;
case NEON::BI__builtin_neon_vcltz_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vcltzq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vclz_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vclzq_v: mask = 0x7000700000000ULL; break;
case NEON::BI__builtin_neon_vcnt_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vcntq_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vcvt_f16_f32: mask = 0x100ULL; break;
case NEON::BI__builtin_neon_vcvt_f32_f16: mask = 0x20000000000ULL; break;
case NEON::BI__builtin_neon_vcvt_f32_f64: mask = 0x200ULL; break;
case NEON::BI__builtin_neon_vcvt_f32_v: mask = 0x40004ULL; break;
case NEON::BI__builtin_neon_vcvt_f64_f32: mask = 0x40000000000ULL; break;
case NEON::BI__builtin_neon_vcvt_f64_v: mask = 0x80008ULL; break;
case NEON::BI__builtin_neon_vcvt_n_f32_v: mask = 0x40004ULL; break;
case NEON::BI__builtin_neon_vcvt_n_f64_v: mask = 0x80008ULL; break;
case NEON::BI__builtin_neon_vcvt_n_s32_v: mask = 0x4ULL; break;
case NEON::BI__builtin_neon_vcvt_n_s64_v: mask = 0x8ULL; break;
case NEON::BI__builtin_neon_vcvt_n_u32_v: mask = 0x40000ULL; break;
case NEON::BI__builtin_neon_vcvt_n_u64_v: mask = 0x80000ULL; break;
case NEON::BI__builtin_neon_vcvt_s32_v: mask = 0x4ULL; break;
case NEON::BI__builtin_neon_vcvt_s64_v: mask = 0x8ULL; break;
case NEON::BI__builtin_neon_vcvt_u32_v: mask = 0x40000ULL; break;
case NEON::BI__builtin_neon_vcvt_u64_v: mask = 0x80000ULL; break;
case NEON::BI__builtin_neon_vcvta_s32_v: mask = 0x4ULL; break;
case NEON::BI__builtin_neon_vcvta_s64_v: mask = 0x8ULL; break;
case NEON::BI__builtin_neon_vcvta_u32_v: mask = 0x40000ULL; break;
case NEON::BI__builtin_neon_vcvta_u64_v: mask = 0x80000ULL; break;
case NEON::BI__builtin_neon_vcvtaq_s32_v: mask = 0x400000000ULL; break;
case NEON::BI__builtin_neon_vcvtaq_s64_v: mask = 0x800000000ULL; break;
case NEON::BI__builtin_neon_vcvtaq_u32_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtaq_u64_v: mask = 0x8000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtm_s32_v: mask = 0x4ULL; break;
case NEON::BI__builtin_neon_vcvtm_s64_v: mask = 0x8ULL; break;
case NEON::BI__builtin_neon_vcvtm_u32_v: mask = 0x40000ULL; break;
case NEON::BI__builtin_neon_vcvtm_u64_v: mask = 0x80000ULL; break;
case NEON::BI__builtin_neon_vcvtmq_s32_v: mask = 0x400000000ULL; break;
case NEON::BI__builtin_neon_vcvtmq_s64_v: mask = 0x800000000ULL; break;
case NEON::BI__builtin_neon_vcvtmq_u32_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtmq_u64_v: mask = 0x8000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtn_s32_v: mask = 0x4ULL; break;
case NEON::BI__builtin_neon_vcvtn_s64_v: mask = 0x8ULL; break;
case NEON::BI__builtin_neon_vcvtn_u32_v: mask = 0x40000ULL; break;
case NEON::BI__builtin_neon_vcvtn_u64_v: mask = 0x80000ULL; break;
case NEON::BI__builtin_neon_vcvtnq_s32_v: mask = 0x400000000ULL; break;
case NEON::BI__builtin_neon_vcvtnq_s64_v: mask = 0x800000000ULL; break;
case NEON::BI__builtin_neon_vcvtnq_u32_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtnq_u64_v: mask = 0x8000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtp_s32_v: mask = 0x4ULL; break;
case NEON::BI__builtin_neon_vcvtp_s64_v: mask = 0x8ULL; break;
case NEON::BI__builtin_neon_vcvtp_u32_v: mask = 0x40000ULL; break;
case NEON::BI__builtin_neon_vcvtp_u64_v: mask = 0x80000ULL; break;
case NEON::BI__builtin_neon_vcvtpq_s32_v: mask = 0x400000000ULL; break;
case NEON::BI__builtin_neon_vcvtpq_s64_v: mask = 0x800000000ULL; break;
case NEON::BI__builtin_neon_vcvtpq_u32_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtpq_u64_v: mask = 0x8000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_f32_v: mask = 0x4000400000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_f64_v: mask = 0x8000800000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_n_f32_v: mask = 0x4000400000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_n_f64_v: mask = 0x8000800000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_n_s32_v: mask = 0x400000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_n_s64_v: mask = 0x800000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_n_u32_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_n_u64_v: mask = 0x8000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_s32_v: mask = 0x400000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_s64_v: mask = 0x800000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_u32_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtq_u64_v: mask = 0x8000000000000ULL; break;
case NEON::BI__builtin_neon_vcvtx_f32_v: mask = 0x40000000000ULL; break;
case NEON::BI__builtin_neon_vext_v: mask = 0xF067FULL; break;
case NEON::BI__builtin_neon_vextq_v: mask = 0xF067F00000000ULL; break;
case NEON::BI__builtin_neon_vfma_lane_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vfma_laneq_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vfma_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vfmaq_lane_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vfmaq_laneq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vfmaq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vhadd_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vhaddq_v: mask = 0x7000700000000ULL; break;
case NEON::BI__builtin_neon_vhsub_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vhsubq_v: mask = 0x7000700000000ULL; break;
case NEON::BI__builtin_neon_vld1_dup_v: mask = 0xF077FULL; break;
case NEON::BI__builtin_neon_vld1_lane_v: mask = 0xF077FULL; break;
case NEON::BI__builtin_neon_vld1_v: mask = 0xF077FULL; PtrArgNum = 0; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1_x2_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1_x3_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1_x4_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1q_dup_v: mask = 0xF077F00000000ULL; break;
case NEON::BI__builtin_neon_vld1q_lane_v: mask = 0xF077F00000000ULL; break;
case NEON::BI__builtin_neon_vld1q_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1q_x2_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1q_x3_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld1q_x4_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld2_dup_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld2_lane_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld2_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld2q_dup_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld2q_lane_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld2q_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld3_dup_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld3_lane_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld3_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld3q_dup_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld3q_lane_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld3q_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld4_dup_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld4_lane_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld4_v: mask = 0xF077FULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld4q_dup_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld4q_lane_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vld4q_v: mask = 0xF077F00000000ULL; PtrArgNum = 1; HasConstPtr = true; break;
case NEON::BI__builtin_neon_vmax_v: mask = 0x70607ULL; break;
case NEON::BI__builtin_neon_vmaxnm_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vmaxnmq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vmaxq_v: mask = 0x7060700000000ULL; break;
case NEON::BI__builtin_neon_vmin_v: mask = 0x70607ULL; break;
case NEON::BI__builtin_neon_vminnm_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vminnmq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vminq_v: mask = 0x7060700000000ULL; break;
case NEON::BI__builtin_neon_vmovl_v: mask = 0xE000E00000000ULL; break;
case NEON::BI__builtin_neon_vmovn_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vmul_lane_v: mask = 0x400ULL; break;
case NEON::BI__builtin_neon_vmul_laneq_v: mask = 0x400ULL; break;
case NEON::BI__builtin_neon_vmul_v: mask = 0x10ULL; break;
case NEON::BI__builtin_neon_vmull_v: mask = 0xE002E00000000ULL; break;
case NEON::BI__builtin_neon_vmulq_v: mask = 0x1000000000ULL; break;
case NEON::BI__builtin_neon_vmulx_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vmulxq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vpadal_v: mask = 0xE000EULL; break;
case NEON::BI__builtin_neon_vpadalq_v: mask = 0xE000E00000000ULL; break;
case NEON::BI__builtin_neon_vpadd_v: mask = 0x70207ULL; break;
case NEON::BI__builtin_neon_vpaddl_v: mask = 0xE000EULL; break;
case NEON::BI__builtin_neon_vpaddlq_v: mask = 0xE000E00000000ULL; break;
case NEON::BI__builtin_neon_vpaddq_v: mask = 0xF060F00000000ULL; break;
case NEON::BI__builtin_neon_vpmax_v: mask = 0x70207ULL; break;
case NEON::BI__builtin_neon_vpmaxnm_v: mask = 0x200ULL; break;
case NEON::BI__builtin_neon_vpmaxnmq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vpmaxq_v: mask = 0x7060700000000ULL; break;
case NEON::BI__builtin_neon_vpmin_v: mask = 0x70207ULL; break;
case NEON::BI__builtin_neon_vpminnm_v: mask = 0x200ULL; break;
case NEON::BI__builtin_neon_vpminnmq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vpminq_v: mask = 0x7060700000000ULL; break;
case NEON::BI__builtin_neon_vqabs_v: mask = 0xFULL; break;
case NEON::BI__builtin_neon_vqabsq_v: mask = 0xF00000000ULL; break;
case NEON::BI__builtin_neon_vqadd_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vqaddq_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vqdmlal_v: mask = 0xC00000000ULL; break;
case NEON::BI__builtin_neon_vqdmlsl_v: mask = 0xC00000000ULL; break;
case NEON::BI__builtin_neon_vqdmulh_v: mask = 0x6ULL; break;
case NEON::BI__builtin_neon_vqdmulhq_v: mask = 0x600000000ULL; break;
case NEON::BI__builtin_neon_vqdmull_v: mask = 0xC00000000ULL; break;
case NEON::BI__builtin_neon_vqmovn_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vqmovun_v: mask = 0x70000ULL; break;
case NEON::BI__builtin_neon_vqneg_v: mask = 0xFULL; break;
case NEON::BI__builtin_neon_vqnegq_v: mask = 0xF00000000ULL; break;
case NEON::BI__builtin_neon_vqrdmulh_v: mask = 0x6ULL; break;
case NEON::BI__builtin_neon_vqrdmulhq_v: mask = 0x600000000ULL; break;
case NEON::BI__builtin_neon_vqrshl_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vqrshlq_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vqrshrn_n_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vqrshrun_n_v: mask = 0x70000ULL; break;
case NEON::BI__builtin_neon_vqshl_n_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vqshl_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vqshlq_n_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vqshlq_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vqshlu_n_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vqshluq_n_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vqshrn_n_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vqshrun_n_v: mask = 0x70000ULL; break;
case NEON::BI__builtin_neon_vqsub_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vqsubq_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vqtbl1_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbl1q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbl2_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbl2q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbl3_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbl3q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbl4_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbl4q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbx1_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbx1q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbx2_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbx2q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbx3_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbx3q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vqtbx4_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vqtbx4q_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vraddhn_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vrbit_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vrbitq_v: mask = 0x1001100000000ULL; break;
case NEON::BI__builtin_neon_vrecpe_v: mask = 0x40600ULL; break;
case NEON::BI__builtin_neon_vrecpeq_v: mask = 0x4060000000000ULL; break;
case NEON::BI__builtin_neon_vrecps_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrecpsq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrhadd_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vrhaddq_v: mask = 0x7000700000000ULL; break;
case NEON::BI__builtin_neon_vrnd_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrnda_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrndaq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrndi_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrndiq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrndm_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrndmq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrndn_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrndnq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrndp_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrndpq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrndq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrndx_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrndxq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrshl_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vrshlq_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vrshr_n_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vrshrn_n_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vrshrq_n_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vrsqrte_v: mask = 0x40600ULL; break;
case NEON::BI__builtin_neon_vrsqrteq_v: mask = 0x4060000000000ULL; break;
case NEON::BI__builtin_neon_vrsqrts_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vrsqrtsq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vrsra_n_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vrsraq_n_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vrsubhn_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vsha1su0q_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vsha1su1q_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vsha256h2q_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vsha256hq_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vsha256su0q_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vsha256su1q_v: mask = 0x4000000000000ULL; break;
case NEON::BI__builtin_neon_vshl_n_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vshl_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vshll_n_v: mask = 0xE000E00000000ULL; break;
case NEON::BI__builtin_neon_vshlq_n_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vshlq_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vshr_n_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vshrn_n_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vshrq_n_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vsli_n_v: mask = 0xF007FULL; break;
case NEON::BI__builtin_neon_vsliq_n_v: mask = 0xF007F00000000ULL; break;
case NEON::BI__builtin_neon_vsqadd_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vsqaddq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vsqrt_v: mask = 0x600ULL; break;
case NEON::BI__builtin_neon_vsqrtq_v: mask = 0x60000000000ULL; break;
case NEON::BI__builtin_neon_vsra_n_v: mask = 0xF000FULL; break;
case NEON::BI__builtin_neon_vsraq_n_v: mask = 0xF000F00000000ULL; break;
case NEON::BI__builtin_neon_vsri_n_v: mask = 0xF007FULL; break;
case NEON::BI__builtin_neon_vsriq_n_v: mask = 0xF007F00000000ULL; break;
case NEON::BI__builtin_neon_vst1_lane_v: mask = 0xF077FULL; break;
case NEON::BI__builtin_neon_vst1_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1_x2_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1_x3_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1_x4_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1q_lane_v: mask = 0xF077F00000000ULL; break;
case NEON::BI__builtin_neon_vst1q_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1q_x2_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1q_x3_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst1q_x4_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst2_lane_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst2_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst2q_lane_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst2q_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst3_lane_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst3_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst3q_lane_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst3q_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst4_lane_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst4_v: mask = 0xF077FULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst4q_lane_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vst4q_v: mask = 0xF077F00000000ULL; PtrArgNum = 0; break;
case NEON::BI__builtin_neon_vsubhn_v: mask = 0x70007ULL; break;
case NEON::BI__builtin_neon_vtbl1_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbl2_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbl3_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbl4_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbx1_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbx2_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbx3_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtbx4_v: mask = 0x10011ULL; break;
case NEON::BI__builtin_neon_vtrn_v: mask = 0x70237ULL; break;
case NEON::BI__builtin_neon_vtrnq_v: mask = 0x7023700000000ULL; break;
case NEON::BI__builtin_neon_vtst_v: mask = 0xF0000ULL; break;
case NEON::BI__builtin_neon_vtstq_v: mask = 0xF000000000000ULL; break;
case NEON::BI__builtin_neon_vuqadd_v: mask = 0xFULL; break;
case NEON::BI__builtin_neon_vuqaddq_v: mask = 0xF00000000ULL; break;
case NEON::BI__builtin_neon_vuzp_v: mask = 0x70237ULL; break;
case NEON::BI__builtin_neon_vuzpq_v: mask = 0x7023700000000ULL; break;
case NEON::BI__builtin_neon_vzip_v: mask = 0x70237ULL; break;
case NEON::BI__builtin_neon_vzipq_v: mask = 0x7023700000000ULL; break;
#endif

#ifdef GET_NEON_IMMEDIATE_CHECK
case NEON::BI__builtin_neon_vcvtq_n_f64_v: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvt_n_f64_v: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvtq_n_s64_v: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvt_n_s64_v: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvtq_n_u64_v: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvt_n_u64_v: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vget_lane_i64: i = 1; u = 0; break;
case NEON::BI__builtin_neon_vgetq_lane_i64: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vgetq_lane_f64: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vget_lane_f64: i = 1; u = 0; break;
case NEON::BI__builtin_neon_vld1_lane_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld1q_lane_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld2_lane_v: i = 4; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld2q_lane_v: i = 4; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld3_lane_v: i = 5; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld3q_lane_v: i = 5; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld4_lane_v: i = 6; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vld4q_lane_v: i = 6; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vcvts_n_s32_f32: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvtd_n_s64_f64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvts_n_u32_f32: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvtd_n_u64_f64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vfmad_lane_f64: i = 3; u = 0; break;
case NEON::BI__builtin_neon_vfmas_lane_f32: i = 3; u = 1; break;
case NEON::BI__builtin_neon_vfmad_laneq_f64: i = 3; u = 1; break;
case NEON::BI__builtin_neon_vfmas_laneq_f32: i = 3; u = 3; break;
case NEON::BI__builtin_neon_vcvts_n_f32_u32: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvts_n_f32_s32: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvtd_n_f64_u64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vcvtd_n_f64_s64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vshld_n_u64: i = 1; u = 63; break;
case NEON::BI__builtin_neon_vshld_n_s64: i = 1; u = 63; break;
case NEON::BI__builtin_neon_vslid_n_u64: i = 2; u = 63; break;
case NEON::BI__builtin_neon_vslid_n_s64: i = 2; u = 63; break;
case NEON::BI__builtin_neon_vqdmlals_lane_s32: i = 3; u = 1; break;
case NEON::BI__builtin_neon_vqdmlalh_lane_s16: i = 3; u = 3; break;
case NEON::BI__builtin_neon_vqdmlals_laneq_s32: i = 3; u = 3; break;
case NEON::BI__builtin_neon_vqdmlalh_laneq_s16: i = 3; u = 7; break;
case NEON::BI__builtin_neon_vqdmlsls_lane_s32: i = 3; u = 1; break;
case NEON::BI__builtin_neon_vqdmlslh_lane_s16: i = 3; u = 3; break;
case NEON::BI__builtin_neon_vqdmlsls_laneq_s32: i = 3; u = 3; break;
case NEON::BI__builtin_neon_vqdmlslh_laneq_s16: i = 3; u = 7; break;
case NEON::BI__builtin_neon_vqrshrns_n_u32: i = 1; l = 1; u = 15; break;
case NEON::BI__builtin_neon_vqrshrnd_n_u64: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vqrshrnh_n_u16: i = 1; l = 1; u = 7; break;
case NEON::BI__builtin_neon_vqrshrns_n_s32: i = 1; l = 1; u = 15; break;
case NEON::BI__builtin_neon_vqrshrnd_n_s64: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vqrshrnh_n_s16: i = 1; l = 1; u = 7; break;
case NEON::BI__builtin_neon_vqrshruns_n_s32: i = 1; l = 1; u = 15; break;
case NEON::BI__builtin_neon_vqrshrund_n_s64: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vqrshrunh_n_s16: i = 1; l = 1; u = 7; break;
case NEON::BI__builtin_neon_vqshlub_n_s8: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vqshlus_n_s32: i = 1; u = 31; break;
case NEON::BI__builtin_neon_vqshlud_n_s64: i = 1; u = 63; break;
case NEON::BI__builtin_neon_vqshluh_n_s16: i = 1; u = 15; break;
case NEON::BI__builtin_neon_vqshlb_n_u8: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vqshls_n_u32: i = 1; u = 31; break;
case NEON::BI__builtin_neon_vqshld_n_u64: i = 1; u = 63; break;
case NEON::BI__builtin_neon_vqshlh_n_u16: i = 1; u = 15; break;
case NEON::BI__builtin_neon_vqshlb_n_s8: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vqshls_n_s32: i = 1; u = 31; break;
case NEON::BI__builtin_neon_vqshld_n_s64: i = 1; u = 63; break;
case NEON::BI__builtin_neon_vqshlh_n_s16: i = 1; u = 15; break;
case NEON::BI__builtin_neon_vqshrns_n_u32: i = 1; l = 1; u = 15; break;
case NEON::BI__builtin_neon_vqshrnd_n_u64: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vqshrnh_n_u16: i = 1; l = 1; u = 7; break;
case NEON::BI__builtin_neon_vqshrns_n_s32: i = 1; l = 1; u = 15; break;
case NEON::BI__builtin_neon_vqshrnd_n_s64: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vqshrnh_n_s16: i = 1; l = 1; u = 7; break;
case NEON::BI__builtin_neon_vqshruns_n_s32: i = 1; l = 1; u = 15; break;
case NEON::BI__builtin_neon_vqshrund_n_s64: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vqshrunh_n_s16: i = 1; l = 1; u = 7; break;
case NEON::BI__builtin_neon_vsrid_n_u64: i = 2; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vsrid_n_s64: i = 2; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vrshrd_n_u64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vrshrd_n_s64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vrsrad_n_u64: i = 2; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vrsrad_n_s64: i = 2; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vshrd_n_u64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vshrd_n_s64: i = 1; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vsrad_n_u64: i = 2; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vsrad_n_s64: i = 2; l = 1; u = 63; break;
case NEON::BI__builtin_neon_vdupb_lane_i8: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vduph_lane_i16: i = 1; u = 3; break;
case NEON::BI__builtin_neon_vdups_lane_i32: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vdupd_lane_i64: i = 1; u = 0; break;
case NEON::BI__builtin_neon_vdupd_lane_f64: i = 1; u = 0; break;
case NEON::BI__builtin_neon_vdups_lane_f32: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vdupb_laneq_i8: i = 1; u = 15; break;
case NEON::BI__builtin_neon_vduph_laneq_i16: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vdups_laneq_i32: i = 1; u = 3; break;
case NEON::BI__builtin_neon_vdupd_laneq_i64: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vdupd_laneq_f64: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vdups_laneq_f32: i = 1; u = 3; break;
case NEON::BI__builtin_neon_vmul_lane_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vmul_laneq_v: i = 2; u = RFT(TV, false, true); break;
case NEON::BI__builtin_neon_vset_lane_i64: i = 2; u = 0; break;
case NEON::BI__builtin_neon_vsetq_lane_i64: i = 2; u = 1; break;
case NEON::BI__builtin_neon_vsetq_lane_f64: i = 2; u = 1; break;
case NEON::BI__builtin_neon_vset_lane_f64: i = 2; u = 0; break;
case NEON::BI__builtin_neon_vsli_n_v: i = 2; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vsliq_n_v: i = 2; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vsri_n_v: i = 2; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vsriq_n_v: i = 2; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vst1_lane_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst1q_lane_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst2_lane_v: i = 3; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst2q_lane_v: i = 3; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst3_lane_v: i = 4; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst3q_lane_v: i = 4; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst4_lane_v: i = 5; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vst4q_lane_v: i = 5; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vcvtq_n_f32_v: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvt_n_f32_v: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvtq_n_s32_v: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvt_n_s32_v: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvtq_n_u32_v: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vcvt_n_u32_v: i = 1; l = 1; u = 31; break;
case NEON::BI__builtin_neon_vext_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vextq_v: i = 2; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vfmaq_lane_v: i = 3; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vfma_lane_v: i = 3; u = RFT(TV, false, false); break;
case NEON::BI__builtin_neon_vfmaq_laneq_v: i = 3; u = RFT(TV, false, true); break;
case NEON::BI__builtin_neon_vfma_laneq_v: i = 3; u = RFT(TV, false, true); break;
case NEON::BI__builtin_neon_vget_lane_i8: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vget_lane_i16: i = 1; u = 3; break;
case NEON::BI__builtin_neon_vgetq_lane_i8: i = 1; u = 15; break;
case NEON::BI__builtin_neon_vgetq_lane_i16: i = 1; u = 7; break;
case NEON::BI__builtin_neon_vgetq_lane_i32: i = 1; u = 3; break;
case NEON::BI__builtin_neon_vgetq_lane_f32: i = 1; u = 3; break;
case NEON::BI__builtin_neon_vget_lane_i32: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vget_lane_f32: i = 1; u = 1; break;
case NEON::BI__builtin_neon_vqrshrn_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqrshrun_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqshluq_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqshlu_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqshlq_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqshl_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqshrn_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vqshrun_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vrshrn_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vrshrq_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vrshr_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vrsraq_n_v: i = 2; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vrsra_n_v: i = 2; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vset_lane_i8: i = 2; u = 7; break;
case NEON::BI__builtin_neon_vset_lane_i16: i = 2; u = 3; break;
case NEON::BI__builtin_neon_vsetq_lane_i8: i = 2; u = 15; break;
case NEON::BI__builtin_neon_vsetq_lane_i16: i = 2; u = 7; break;
case NEON::BI__builtin_neon_vsetq_lane_i32: i = 2; u = 3; break;
case NEON::BI__builtin_neon_vsetq_lane_f32: i = 2; u = 3; break;
case NEON::BI__builtin_neon_vset_lane_i32: i = 2; u = 1; break;
case NEON::BI__builtin_neon_vset_lane_f32: i = 2; u = 1; break;
case NEON::BI__builtin_neon_vshll_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vshlq_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vshl_n_v: i = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vshrn_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vshrq_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vshr_n_v: i = 1; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vsraq_n_v: i = 2; l = 1; u = RFT(TV, true); break;
case NEON::BI__builtin_neon_vsra_n_v: i = 2; l = 1; u = RFT(TV, true); break;
#endif

