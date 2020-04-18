// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/alsa/alsa_wrapper.h"

namespace media {

AlsaWrapper::AlsaWrapper() = default;

AlsaWrapper::~AlsaWrapper() = default;

int AlsaWrapper::PcmOpen(snd_pcm_t** handle, const char* name,
                         snd_pcm_stream_t stream, int mode) {
  return snd_pcm_open(handle, name, stream, mode);
}

int AlsaWrapper::DeviceNameHint(int card, const char* iface, void*** hints) {
  return snd_device_name_hint(card, iface, hints);
}

char* AlsaWrapper::DeviceNameGetHint(const void* hint, const char* id) {
  return snd_device_name_get_hint(hint, id);
}

int AlsaWrapper::DeviceNameFreeHint(void** hints) {
  return snd_device_name_free_hint(hints);
}

int AlsaWrapper::CardNext(int* rcard) {
  return snd_card_next(rcard);
}

int AlsaWrapper::PcmClose(snd_pcm_t* handle) {
  return snd_pcm_close(handle);
}

int AlsaWrapper::PcmPrepare(snd_pcm_t* handle) {
  return snd_pcm_prepare(handle);
}

int AlsaWrapper::PcmDrain(snd_pcm_t* handle) {
  return snd_pcm_drain(handle);
}

int AlsaWrapper::PcmDrop(snd_pcm_t* handle) {
  return snd_pcm_drop(handle);
}

int AlsaWrapper::PcmDelay(snd_pcm_t* handle, snd_pcm_sframes_t* delay) {
  return snd_pcm_delay(handle, delay);
}

snd_pcm_sframes_t AlsaWrapper::PcmWritei(snd_pcm_t* handle,
                                         const void* buffer,
                                         snd_pcm_uframes_t size) {
  return snd_pcm_writei(handle, buffer, size);
}

snd_pcm_sframes_t AlsaWrapper::PcmReadi(snd_pcm_t* handle,
                                        void* buffer,
                                        snd_pcm_uframes_t size) {
  return snd_pcm_readi(handle, buffer, size);
}

int AlsaWrapper::PcmRecover(snd_pcm_t* handle, int err, int silent) {
  return snd_pcm_recover(handle, err, silent);
}

const char* AlsaWrapper::PcmName(snd_pcm_t* handle) {
  return snd_pcm_name(handle);
}

int AlsaWrapper::PcmSetParams(snd_pcm_t* handle, snd_pcm_format_t format,
                              snd_pcm_access_t access, unsigned int channels,
                              unsigned int rate, int soft_resample,
                              unsigned int latency) {
  return snd_pcm_set_params(handle,
                            format,
                            access,
                            channels,
                            rate,
                            soft_resample,
                            latency);
}

int AlsaWrapper::PcmGetParams(snd_pcm_t* handle, snd_pcm_uframes_t* buffer_size,
                              snd_pcm_uframes_t* period_size) {
  return snd_pcm_get_params(handle, buffer_size, period_size);
}

snd_pcm_sframes_t AlsaWrapper::PcmAvailUpdate(snd_pcm_t* handle) {
  return snd_pcm_avail_update(handle);
}

snd_pcm_state_t AlsaWrapper::PcmState(snd_pcm_t* handle) {
  return snd_pcm_state(handle);
}

const char* AlsaWrapper::StrError(int errnum) {
  return snd_strerror(errnum);
}

int AlsaWrapper::PcmStart(snd_pcm_t* handle) {
  return snd_pcm_start(handle);
}

int AlsaWrapper::MixerOpen(snd_mixer_t** mixer, int mode) {
  return snd_mixer_open(mixer, mode);
}

int AlsaWrapper::MixerAttach(snd_mixer_t* mixer, const char* name) {
  return snd_mixer_attach(mixer, name);
}

int AlsaWrapper::MixerElementRegister(snd_mixer_t* mixer,
                                      struct snd_mixer_selem_regopt* options,
                                      snd_mixer_class_t** classp) {
  return snd_mixer_selem_register(mixer, options, classp);
}

void AlsaWrapper::MixerFree(snd_mixer_t* mixer) {
  snd_mixer_free(mixer);
}

int AlsaWrapper::MixerDetach(snd_mixer_t* mixer, const char* name) {
  return snd_mixer_detach(mixer, name);
}

int AlsaWrapper::MixerClose(snd_mixer_t* mixer) {
  return snd_mixer_close(mixer);
}

int AlsaWrapper::MixerLoad(snd_mixer_t* mixer) {
  return snd_mixer_load(mixer);
}

snd_mixer_elem_t* AlsaWrapper::MixerFirstElem(snd_mixer_t* mixer) {
  return snd_mixer_first_elem(mixer);
}

snd_mixer_elem_t* AlsaWrapper::MixerNextElem(snd_mixer_elem_t* elem) {
  return snd_mixer_elem_next(elem);
}

int AlsaWrapper::MixerSelemIsActive(snd_mixer_elem_t* elem) {
  return snd_mixer_selem_is_active(elem);
}

const char* AlsaWrapper::MixerSelemName(snd_mixer_elem_t* elem) {
  return snd_mixer_selem_get_name(elem);
}

int AlsaWrapper::MixerSelemSetCaptureVolumeAll(
    snd_mixer_elem_t* elem, long value) {
  return snd_mixer_selem_set_capture_volume_all(elem, value);
}

int AlsaWrapper::MixerSelemGetCaptureVolume(
    snd_mixer_elem_t* elem, snd_mixer_selem_channel_id_t channel, long* value) {
  return snd_mixer_selem_get_capture_volume(elem, channel, value);
}

int AlsaWrapper::MixerSelemHasCaptureVolume(snd_mixer_elem_t* elem) {
  return snd_mixer_selem_has_capture_volume(elem);
}

int AlsaWrapper::MixerSelemGetCaptureVolumeRange(snd_mixer_elem_t* elem,
                                                 long* min, long* max) {
  return snd_mixer_selem_get_capture_volume_range(elem, min, max);
}

void* AlsaWrapper::MixerElemGetCallbackPrivate(const snd_mixer_elem_t* obj) {
  return snd_mixer_elem_get_callback_private(obj);
}

void AlsaWrapper::MixerElemSetCallback(snd_mixer_elem_t* obj,
                                       snd_mixer_elem_callback_t val) {
  snd_mixer_elem_set_callback(obj, val);
}

void AlsaWrapper::MixerElemSetCallbackPrivate(snd_mixer_elem_t* obj,
                                              void* val) {
  snd_mixer_elem_set_callback_private(obj, val);
}

snd_mixer_elem_t* AlsaWrapper::MixerFindSelem(snd_mixer_t* mixer,
                                              const snd_mixer_selem_id_t* id) {
  return snd_mixer_find_selem(mixer, id);
}

int AlsaWrapper::MixerHandleEvents(snd_mixer_t* mixer) {
  return snd_mixer_handle_events(mixer);
}

int AlsaWrapper::MixerPollDescriptors(snd_mixer_t* mixer,
                                      struct pollfd* pfds,
                                      unsigned int space) {
  return snd_mixer_poll_descriptors(mixer, pfds, space);
}

int AlsaWrapper::MixerPollDescriptorsCount(snd_mixer_t* mixer) {
  return snd_mixer_poll_descriptors_count(mixer);
}

int AlsaWrapper::MixerSelemGetPlaybackSwitch(
    snd_mixer_elem_t* elem,
    snd_mixer_selem_channel_id_t channel,
    int* value) {
  return snd_mixer_selem_get_playback_switch(elem, channel, value);
}

int AlsaWrapper::MixerSelemGetPlaybackVolume(
    snd_mixer_elem_t* elem,
    snd_mixer_selem_channel_id_t channel,
    long* value) {
  return snd_mixer_selem_get_playback_volume(elem, channel, value);
}

int AlsaWrapper::MixerSelemGetPlaybackVolumeRange(snd_mixer_elem_t* elem,
                                                  long* min,
                                                  long* max) {
  return snd_mixer_selem_get_playback_volume_range(elem, min, max);
}

int AlsaWrapper::MixerSelemHasPlaybackSwitch(snd_mixer_elem_t* elem) {
  return snd_mixer_selem_has_playback_switch(elem);
}

void AlsaWrapper::MixerSelemIdSetIndex(snd_mixer_selem_id_t* obj,
                                       unsigned int val) {
  snd_mixer_selem_id_set_index(obj, val);
}

void AlsaWrapper::MixerSelemIdSetName(snd_mixer_selem_id_t* obj,
                                      const char* val) {
  snd_mixer_selem_id_set_name(obj, val);
}

int AlsaWrapper::MixerSelemSetPlaybackSwitch(
    snd_mixer_elem_t* elem,
    snd_mixer_selem_channel_id_t channel,
    int value) {
  return snd_mixer_selem_set_playback_switch(elem, channel, value);
}

int AlsaWrapper::MixerSelemSetPlaybackVolumeAll(snd_mixer_elem_t* elem,
                                                long value) {
  return snd_mixer_selem_set_playback_volume_all(elem, value);
}

int AlsaWrapper::MixerSelemIdMalloc(snd_mixer_selem_id_t** ptr) {
  return snd_mixer_selem_id_malloc(ptr);
}

void AlsaWrapper::MixerSelemIdFree(snd_mixer_selem_id_t* obj) {
  snd_mixer_selem_id_free(obj);
}

}  // namespace media
