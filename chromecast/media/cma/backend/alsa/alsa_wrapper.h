// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_ALSA_ALSA_WRAPPER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_ALSA_ALSA_WRAPPER_H_

#include "base/macros.h"
#include "media/audio/alsa/alsa_wrapper.h"

namespace chromecast {
namespace media {

extern const int kAlsaTstampTypeMonotonicRaw;

// Extends the Chromium AlsaWrapper, adding additional functions that we use.
class AlsaWrapper : public ::media::AlsaWrapper {
 public:
  AlsaWrapper();
  ~AlsaWrapper() override;

  virtual int PcmPause(snd_pcm_t* handle, int enable);

  virtual int PcmStatusMalloc(snd_pcm_status_t** ptr);
  virtual void PcmStatusFree(snd_pcm_status_t* obj);
  virtual int PcmStatus(snd_pcm_t* handle, snd_pcm_status_t* status);
  virtual snd_pcm_sframes_t PcmStatusGetDelay(const snd_pcm_status_t* obj);
  virtual snd_pcm_uframes_t PcmStatusGetAvail(const snd_pcm_status_t* obj);
  virtual void PcmStatusGetHtstamp(const snd_pcm_status_t* obj,
                                   snd_htimestamp_t* ptr);
  virtual snd_pcm_state_t PcmStatusGetState(const snd_pcm_status_t* obj);
  virtual ssize_t PcmFormatSize(snd_pcm_format_t format, size_t samples);
  virtual int PcmHwParamsMalloc(snd_pcm_hw_params_t** ptr);
  virtual void PcmHwParamsFree(snd_pcm_hw_params_t* obj);
  virtual int PcmHwParamsCurrent(snd_pcm_t* handle,
                                 snd_pcm_hw_params_t* params);
  virtual int PcmHwParamsCanPause(const snd_pcm_hw_params_t* params);
  virtual int PcmHwParamsAny(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
  virtual int PcmHwParamsSetRateResample(snd_pcm_t* handle,
                                         snd_pcm_hw_params_t* params,
                                         bool val);
  virtual int PcmHwParamsSetAccess(snd_pcm_t* handle,
                                   snd_pcm_hw_params_t* params,
                                   snd_pcm_access_t access);
  virtual int PcmHwParamsSetFormat(snd_pcm_t* handle,
                                   snd_pcm_hw_params_t* params,
                                   snd_pcm_format_t format);
  virtual int PcmHwParamsSetChannels(snd_pcm_t* handle,
                                     snd_pcm_hw_params_t* params,
                                     unsigned int num_channels);
  virtual int PcmHwParamsSetRateNear(snd_pcm_t* handle,
                                     snd_pcm_hw_params_t* params,
                                     unsigned int* rate,
                                     int* dir);
  virtual int PcmHwParamsSetBufferSizeNear(snd_pcm_t* handle,
                                           snd_pcm_hw_params_t* params,
                                           snd_pcm_uframes_t* val);
  virtual int PcmHwParamsSetPeriodSizeNear(snd_pcm_t* handle,
                                           snd_pcm_hw_params_t* params,
                                           snd_pcm_uframes_t* val,
                                           int* dir);
  virtual int PcmHwParamsTestFormat(snd_pcm_t* handle,
                                    snd_pcm_hw_params_t* params,
                                    snd_pcm_format_t format);
  virtual int PcmHwParamsTestRate(snd_pcm_t* handle,
                                  snd_pcm_hw_params_t* params,
                                  unsigned int rate,
                                  int dir);
  virtual int PcmHwParams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);

  virtual int PcmSwParamsMalloc(snd_pcm_sw_params_t** params);
  virtual void PcmSwParamsFree(snd_pcm_sw_params_t* params);
  virtual int PcmSwParamsCurrent(snd_pcm_t* handle,
                                 snd_pcm_sw_params_t* params);
  virtual int PcmSwParamsSetStartThreshold(snd_pcm_t* handle,
                                           snd_pcm_sw_params_t* params,
                                           snd_pcm_uframes_t val);
  virtual int PcmSwParamsSetAvailMin(snd_pcm_t* handle,
                                     snd_pcm_sw_params_t* params,
                                     snd_pcm_uframes_t val);
  virtual int PcmSwParamsSetTstampMode(snd_pcm_t* handle,
                                       snd_pcm_sw_params_t* obj,
                                       snd_pcm_tstamp_t val);
  virtual int PcmSwParamsSetTstampType(snd_pcm_t* handle,
                                       snd_pcm_sw_params_t* obj,
                                       int val);
  virtual int PcmSwParams(snd_pcm_t* handle, snd_pcm_sw_params_t* obj);

 private:
  DISALLOW_COPY_AND_ASSIGN(AlsaWrapper);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_ALSA_ALSA_WRAPPER_H_
