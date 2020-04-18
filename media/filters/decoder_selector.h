// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_DECODER_SELECTOR_H_
#define MEDIA_FILTERS_DECODER_SELECTOR_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/base/demuxer_stream.h"
#include "media/base/pipeline_status.h"
#include "media/filters/decoder_stream_traits.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class CdmContext;
class DecryptingDemuxerStream;
class MediaLog;

// DecoderSelector (creates if necessary and) initializes the proper
// Decoder for a given DemuxerStream. If the given DemuxerStream is
// encrypted, a DecryptingDemuxerStream may also be created.
// The template parameter |StreamType| is the type of stream we will be
// selecting a decoder for.
template<DemuxerStream::Type StreamType>
class MEDIA_EXPORT DecoderSelector {
 public:
  typedef DecoderStreamTraits<StreamType> StreamTraits;
  typedef typename StreamTraits::DecoderType Decoder;
  typedef typename StreamTraits::DecoderConfigType DecoderConfig;

  // Callback to create a list of decoders to select from.
  // TODO(xhwang): Use a DecoderFactory to create decoders one by one as needed,
  // instead of creating a list of decoders all at once.
  using CreateDecodersCB =
      base::RepeatingCallback<std::vector<std::unique_ptr<Decoder>>()>;

  // Indicates completion of Decoder selection.
  // - First parameter: The initialized Decoder. If it's set to NULL, then
  // Decoder initialization failed.
  // - Second parameter: The initialized DecryptingDemuxerStream. If it's not
  // NULL, then a DecryptingDemuxerStream is created and initialized to do
  // decryption for the initialized Decoder.
  // Note: The caller owns selected Decoder and DecryptingDemuxerStream.
  // The caller should call DecryptingDemuxerStream::Reset() before
  // calling Decoder::Reset() to release any pending decryption or read.
  using SelectDecoderCB =
      base::Callback<void(std::unique_ptr<Decoder>,
                          std::unique_ptr<DecryptingDemuxerStream>)>;

  DecoderSelector(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      CreateDecodersCB create_decoders_cb,
      MediaLog* media_log);

  // Aborts pending Decoder selection and fires |select_decoder_cb| with
  // null and null immediately if it's pending.
  ~DecoderSelector();

  // Initializes and selects the first Decoder that can decode the |stream|.
  // The selected Decoder (and DecryptingDemuxerStream) is returned via
  // the |select_decoder_cb|.
  // Notes:
  // 1. This must not be called again before |select_decoder_cb| is run.
  // 2. |create_decoders_cb| will be called to create a list of candidate
  //    decoders to select from.
  // 3. The |blacklisted_decoder| will be skipped in the decoder selection
  //    process, unless DecryptingDemuxerStream is chosen. This is because
  //    DecryptingDemuxerStream updates the |config_|, and the blacklist should
  //    only be applied to the original |stream| config.
  // 4. All decoders that are not selected will be deleted upon returning
  //    |select_decoder_cb|.
  // 5. |cdm_context| is optional. If |cdm_context| is null, no CDM will be
  //    available to perform decryption.
  void SelectDecoder(StreamTraits* traits,
                     DemuxerStream* stream,
                     CdmContext* cdm_context,
                     const std::string& blacklisted_decoder,
                     const SelectDecoderCB& select_decoder_cb,
                     const typename Decoder::OutputCB& output_cb,
                     const base::Closure& waiting_for_decryption_key_cb);

 private:
  void InitializeDecoder();
  void DecoderInitDone(bool success);
  void ReturnNullDecoder();
  void InitializeDecryptingDemuxerStream();
  void DecryptingDemuxerStreamInitDone(PipelineStatus status);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  CreateDecodersCB create_decoders_cb_;
  MediaLog* media_log_;

  StreamTraits* traits_;

  // Could be the |stream| passed in SelectDecoder, or |decrypted_stream_| when
  // a DecryptingDemuxerStream is selected.
  DemuxerStream* input_stream_ = nullptr;

  CdmContext* cdm_context_;
  std::string blacklisted_decoder_;
  SelectDecoderCB select_decoder_cb_;
  typename Decoder::OutputCB output_cb_;
  base::Closure waiting_for_decryption_key_cb_;

  std::vector<std::unique_ptr<Decoder>> decoders_;

  std::unique_ptr<Decoder> decoder_;
  std::unique_ptr<DecryptingDemuxerStream> decrypted_stream_;

  // Config of the |input_stream| used to initialize decoders.
  DecoderConfig config_;

  // Indicates if we tried to initialize |decrypted_stream_|.
  bool tried_decrypting_demuxer_stream_ = false;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<DecoderSelector> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DecoderSelector);
};

typedef DecoderSelector<DemuxerStream::VIDEO> VideoDecoderSelector;
typedef DecoderSelector<DemuxerStream::AUDIO> AudioDecoderSelector;

}  // namespace media

#endif  // MEDIA_FILTERS_DECODER_SELECTOR_H_
