// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/alsa/alsa_wrapper.h"

#include "chromecast/media/cma/backend/audio_buildflags.h"

namespace chromecast {
namespace media {

#if BUILDFLAG(ALSA_MONOTONIC_RAW_TSTAMPS)
const int kAlsaTstampTypeMonotonicRaw =
    static_cast<int>(SND_PCM_TSTAMP_TYPE_MONOTONIC_RAW);
#else
const int kAlsaTstampTypeMonotonicRaw = 0;
#endif  // BUILDFLAG(ALSA_MONOTONIC_RAW_TSTAMPS)

AlsaWrapper::AlsaWrapper() {
}

AlsaWrapper::~AlsaWrapper() {
}

int AlsaWrapper::PcmPause(snd_pcm_t* handle, int enable) {
  return snd_pcm_pause(handle, enable);
}

int AlsaWrapper::PcmStatusMalloc(snd_pcm_status_t** ptr) {
  return snd_pcm_status_malloc(ptr);
}

void AlsaWrapper::PcmStatusFree(snd_pcm_status_t* obj) {
  snd_pcm_status_free(obj);
}

int AlsaWrapper::PcmStatus(snd_pcm_t* handle, snd_pcm_status_t* status) {
  return snd_pcm_status(handle, status);
}

snd_pcm_sframes_t AlsaWrapper::PcmStatusGetDelay(const snd_pcm_status_t* obj) {
  return snd_pcm_status_get_delay(obj);
}

snd_pcm_uframes_t AlsaWrapper::PcmStatusGetAvail(const snd_pcm_status_t* obj) {
  return snd_pcm_status_get_avail(obj);
}

ssize_t AlsaWrapper::PcmFormatSize(snd_pcm_format_t format, size_t samples) {
  return snd_pcm_format_size(format, samples);
}

void AlsaWrapper::PcmStatusGetHtstamp(const snd_pcm_status_t* obj,
                                      snd_htimestamp_t* ptr) {
  snd_pcm_status_get_htstamp(obj, ptr);
}

snd_pcm_state_t AlsaWrapper::PcmStatusGetState(const snd_pcm_status_t* obj) {
  return snd_pcm_status_get_state(obj);
}

int AlsaWrapper::PcmHwParamsMalloc(snd_pcm_hw_params_t** ptr) {
  return snd_pcm_hw_params_malloc(ptr);
}

void AlsaWrapper::PcmHwParamsFree(snd_pcm_hw_params_t* obj) {
  snd_pcm_hw_params_free(obj);
}

int AlsaWrapper::PcmHwParamsCurrent(snd_pcm_t* handle,
                                    snd_pcm_hw_params_t* params) {
  return snd_pcm_hw_params_current(handle, params);
}

int AlsaWrapper::PcmHwParamsCanPause(const snd_pcm_hw_params_t* params) {
  return snd_pcm_hw_params_can_pause(params);
}

int AlsaWrapper::PcmHwParamsSetRateResample(snd_pcm_t* handle,
                                            snd_pcm_hw_params_t* params,
                                            bool val) {
  return snd_pcm_hw_params_set_rate_resample(handle, params, val);
}

int AlsaWrapper::PcmHwParamsSetAccess(snd_pcm_t* handle,
                                      snd_pcm_hw_params_t* params,
                                      snd_pcm_access_t access) {
  return snd_pcm_hw_params_set_access(handle, params, access);
}

int AlsaWrapper::PcmHwParamsSetFormat(snd_pcm_t* handle,
                                      snd_pcm_hw_params_t* params,
                                      snd_pcm_format_t format) {
  return snd_pcm_hw_params_set_format(handle, params, format);
}

int AlsaWrapper::PcmHwParamsSetChannels(snd_pcm_t* handle,
                                        snd_pcm_hw_params_t* params,
                                        unsigned int num_channels) {
  return snd_pcm_hw_params_set_channels(handle, params, num_channels);
}

int AlsaWrapper::PcmHwParamsSetRateNear(snd_pcm_t* handle,
                                        snd_pcm_hw_params_t* params,
                                        unsigned int* rate,
                                        int* dir) {
  return snd_pcm_hw_params_set_rate_near(handle, params, rate, dir);
}

int AlsaWrapper::PcmHwParamsSetBufferSizeNear(snd_pcm_t* handle,
                                              snd_pcm_hw_params_t* params,
                                              snd_pcm_uframes_t* val) {
  return snd_pcm_hw_params_set_buffer_size_near(handle, params, val);
}

int AlsaWrapper::PcmHwParamsSetPeriodSizeNear(snd_pcm_t* handle,
                                              snd_pcm_hw_params_t* params,
                                              snd_pcm_uframes_t* val,
                                              int* dir) {
  return snd_pcm_hw_params_set_period_size_near(handle, params, val, dir);
}

int AlsaWrapper::PcmHwParamsTestFormat(snd_pcm_t* handle,
                                       snd_pcm_hw_params_t* params,
                                       snd_pcm_format_t format) {
  return snd_pcm_hw_params_test_format(handle, params, format);
}

int AlsaWrapper::PcmHwParamsTestRate(snd_pcm_t* handle,
                                     snd_pcm_hw_params_t* params,
                                     unsigned int rate,
                                     int dir) {
  return snd_pcm_hw_params_test_rate(handle, params, rate, dir);
}

int AlsaWrapper::PcmHwParamsAny(snd_pcm_t* handle,
                                snd_pcm_hw_params_t* params) {
  return snd_pcm_hw_params_any(handle, params);
}

int AlsaWrapper::PcmHwParams(snd_pcm_t* handle, snd_pcm_hw_params_t* params) {
  return snd_pcm_hw_params(handle, params);
}

int AlsaWrapper::PcmSwParamsMalloc(snd_pcm_sw_params_t** params) {
  return snd_pcm_sw_params_malloc(params);
}

void AlsaWrapper::PcmSwParamsFree(snd_pcm_sw_params_t* params) {
  snd_pcm_sw_params_free(params);
}

int AlsaWrapper::PcmSwParamsCurrent(snd_pcm_t* handle,
                                    snd_pcm_sw_params_t* params) {
  return snd_pcm_sw_params_current(handle, params);
}

int AlsaWrapper::PcmSwParamsSetStartThreshold(snd_pcm_t* handle,
                                              snd_pcm_sw_params_t* params,
                                              snd_pcm_uframes_t val) {
  return snd_pcm_sw_params_set_start_threshold(handle, params, val);
}

int AlsaWrapper::PcmSwParamsSetAvailMin(snd_pcm_t* handle,
                                        snd_pcm_sw_params_t* params,
                                        snd_pcm_uframes_t val) {
  return snd_pcm_sw_params_set_avail_min(handle, params, val);
}

int AlsaWrapper::PcmSwParamsSetTstampMode(snd_pcm_t* handle,
                                          snd_pcm_sw_params_t* obj,
                                          snd_pcm_tstamp_t val) {
  return snd_pcm_sw_params_set_tstamp_mode(handle, obj, val);
}

int AlsaWrapper::PcmSwParamsSetTstampType(snd_pcm_t* handle,
                                          snd_pcm_sw_params_t* obj,
                                          int val) {
#if BUILDFLAG(ALSA_MONOTONIC_RAW_TSTAMPS)
  return snd_pcm_sw_params_set_tstamp_type(
      handle, obj, static_cast<snd_pcm_tstamp_type_t>(val));
#else
  return 0;
#endif  // BUILDFLAG(ALSA_MONOTONIC_RAW_TSTAMPS)
}

int AlsaWrapper::PcmSwParams(snd_pcm_t* handle, snd_pcm_sw_params_t* params) {
  return snd_pcm_sw_params(handle, params);
}

}  // namespace media
}  // namespace chromecast
