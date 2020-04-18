/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_BASE_AUDIO_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_BASE_AUDIO_CONTEXT_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_decode_error_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_decode_success_callback.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"
#include "third_party/blink/renderer/core/html/media/autoplay_policy.h"
#include "third_party/blink/renderer/core/typed_arrays/array_buffer_view_helpers.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/webaudio/async_audio_decoder.h"
#include "third_party/blink/renderer/modules/webaudio/audio_destination_node.h"
#include "third_party/blink/renderer/modules/webaudio/deferred_task_handler.h"
#include "third_party/blink/renderer/modules/webaudio/iir_filter_node.h"
#include "third_party/blink/renderer/platform/audio/audio_bus.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class AnalyserNode;
class AudioBuffer;
class AudioBufferSourceNode;
class AudioContextOptions;
class AudioListener;
class AudioWorklet;
class BiquadFilterNode;
class ChannelMergerNode;
class ChannelSplitterNode;
class ConstantSourceNode;
class ConvolverNode;
class DelayNode;
class Document;
class DynamicsCompressorNode;
class ExceptionState;
class GainNode;
class HTMLMediaElement;
class IIRFilterNode;
class MediaElementAudioSourceNode;
class MediaStream;
class MediaStreamAudioDestinationNode;
class MediaStreamAudioSourceNode;
class OscillatorNode;
class PannerNode;
class PeriodicWave;
class PeriodicWaveConstraints;
class ScriptProcessorNode;
class ScriptPromiseResolver;
class ScriptState;
class SecurityOrigin;
class StereoPannerNode;
class WaveShaperNode;
class WorkerThread;

// BaseAudioContext is the cornerstone of the web audio API and all AudioNodes
// are created from it.  For thread safety between the audio thread and the main
// thread, it has a rendering graph locking mechanism.

class MODULES_EXPORT BaseAudioContext
    : public EventTargetWithInlineData,
      public ActiveScriptWrappable<BaseAudioContext>,
      public PausableObject {
  USING_GARBAGE_COLLECTED_MIXIN(BaseAudioContext);
  DEFINE_WRAPPERTYPEINFO();

 public:
  // The state of an audio context.  On creation, the state is Suspended. The
  // state is Running if audio is being processed (audio graph is being pulled
  // for data). The state is Closed if the audio context has been closed.  The
  // valid transitions are from Suspended to either Running or Closed; Running
  // to Suspended or Closed. Once Closed, there are no valid transitions.
  enum AudioContextState { kSuspended, kRunning, kClosed };

  // Create an AudioContext for rendering to the audio hardware.
  static BaseAudioContext* Create(Document&,
                                  const AudioContextOptions&,
                                  ExceptionState&);

  ~BaseAudioContext() override;

  void Trace(blink::Visitor*) override;

  // Is the destination node initialized and ready to handle audio?
  bool IsDestinationInitialized() const {
    AudioDestinationNode* dest = destination();
    return dest ? dest->GetAudioDestinationHandler().IsInitialized() : false;
  }

  // Document notification
  void ContextDestroyed(ExecutionContext*) final;
  bool HasPendingActivity() const override;

  // Cannnot be called from the audio thread.
  AudioDestinationNode* destination() const;

  size_t CurrentSampleFrame() const {
    // TODO: What is the correct value for the current frame if the destination
    // node has gone away?  0 is a valid frame.
    return destination_node_ ? destination_node_->GetAudioDestinationHandler()
                                   .CurrentSampleFrame()
                             : 0;
  }

  double currentTime() const {
    // TODO: What is the correct value for the current time if the destination
    // node has gone away? 0 is a valid time.
    return destination_node_
               ? destination_node_->GetAudioDestinationHandler().CurrentTime()
               : 0;
  }

  float sampleRate() const {
    return destination_node_
               ? destination_node_->GetAudioDestinationHandler().SampleRate()
               : ClosedContextSampleRate();
  }

  float FramesPerBuffer() const {
    return destination_node_ ? destination_node_->GetAudioDestinationHandler()
                                   .FramesPerBuffer()
                             : 0;
  }

  size_t CallbackBufferSize() const {
    return destination_node_ ? destination_node_->Handler().CallbackBufferSize()
                             : 0;
  }

  String state() const;
  AudioContextState ContextState() const { return context_state_; }
  void ThrowExceptionForClosedState(ExceptionState&);

  AudioBuffer* createBuffer(unsigned number_of_channels,
                            size_t number_of_frames,
                            float sample_rate,
                            ExceptionState&);

  // Asynchronous audio file data decoding.
  ScriptPromise decodeAudioData(ScriptState*,
                                DOMArrayBuffer* audio_data,
                                V8DecodeSuccessCallback*,
                                V8DecodeErrorCallback*,
                                ExceptionState&);

  ScriptPromise decodeAudioData(ScriptState*,
                                DOMArrayBuffer* audio_data,
                                ExceptionState&);

  ScriptPromise decodeAudioData(ScriptState*,
                                DOMArrayBuffer* audio_data,
                                V8DecodeSuccessCallback*,
                                ExceptionState&);

  // Handles the promise and callbacks when |decodeAudioData| is finished
  // decoding.
  void HandleDecodeAudioData(
      AudioBuffer*,
      ScriptPromiseResolver*,
      V8PersistentCallbackFunction<V8DecodeSuccessCallback>*,
      V8PersistentCallbackFunction<V8DecodeErrorCallback>*);

  AudioListener* listener() { return listener_; }

  virtual bool HasRealtimeConstraint() = 0;

  // The AudioNode create methods are called on the main thread (from
  // JavaScript).
  AudioBufferSourceNode* createBufferSource(ExceptionState&);
  ConstantSourceNode* createConstantSource(ExceptionState&);
  MediaElementAudioSourceNode* createMediaElementSource(HTMLMediaElement*,
                                                        ExceptionState&);
  MediaStreamAudioSourceNode* createMediaStreamSource(MediaStream*,
                                                      ExceptionState&);
  MediaStreamAudioDestinationNode* createMediaStreamDestination(
      ExceptionState&);
  GainNode* createGain(ExceptionState&);
  BiquadFilterNode* createBiquadFilter(ExceptionState&);
  WaveShaperNode* createWaveShaper(ExceptionState&);
  DelayNode* createDelay(ExceptionState&);
  DelayNode* createDelay(double max_delay_time, ExceptionState&);
  PannerNode* createPanner(ExceptionState&);
  ConvolverNode* createConvolver(ExceptionState&);
  DynamicsCompressorNode* createDynamicsCompressor(ExceptionState&);
  AnalyserNode* createAnalyser(ExceptionState&);
  ScriptProcessorNode* createScriptProcessor(ExceptionState&);
  ScriptProcessorNode* createScriptProcessor(size_t buffer_size,
                                             ExceptionState&);
  ScriptProcessorNode* createScriptProcessor(size_t buffer_size,
                                             size_t number_of_input_channels,
                                             ExceptionState&);
  ScriptProcessorNode* createScriptProcessor(size_t buffer_size,
                                             size_t number_of_input_channels,
                                             size_t number_of_output_channels,
                                             ExceptionState&);
  StereoPannerNode* createStereoPanner(ExceptionState&);
  ChannelSplitterNode* createChannelSplitter(ExceptionState&);
  ChannelSplitterNode* createChannelSplitter(size_t number_of_outputs,
                                             ExceptionState&);
  ChannelMergerNode* createChannelMerger(ExceptionState&);
  ChannelMergerNode* createChannelMerger(size_t number_of_inputs,
                                         ExceptionState&);
  OscillatorNode* createOscillator(ExceptionState&);
  PeriodicWave* createPeriodicWave(const Vector<float>& real,
                                   const Vector<float>& imag,
                                   ExceptionState&);
  PeriodicWave* createPeriodicWave(const Vector<float>& real,
                                   const Vector<float>& imag,
                                   const PeriodicWaveConstraints&,
                                   ExceptionState&);

  // Suspend
  virtual ScriptPromise suspendContext(ScriptState*) = 0;

  // Resume
  virtual ScriptPromise resumeContext(ScriptState*) = 0;

  // IIRFilter
  IIRFilterNode* createIIRFilter(Vector<double> feedforward_coef,
                                 Vector<double> feedback_coef,
                                 ExceptionState&);

  // When a source node has started processing and needs to be protected,
  // this method tells the context to protect the node.
  //
  // The context itself keeps a reference to all source nodes.  The source
  // nodes, then reference all nodes they're connected to.  In turn, these
  // nodes reference all nodes they're connected to.  All nodes are ultimately
  // connected to the AudioDestinationNode.  When the context release a source
  // node, it will be deactivated from the rendering graph along with all
  // other nodes it is uniquely connected to.
  void NotifySourceNodeStartedProcessing(AudioNode*);
  // When a source node has no more processing to do (has finished playing),
  // this method tells the context to release the corresponding node.
  void NotifySourceNodeFinishedProcessing(AudioHandler*);

  // Called at the start of each render quantum.
  void HandlePreRenderTasks(const AudioIOPosition& output_position);

  // Called at the end of each render quantum.
  void HandlePostRenderTasks();

  // Keeps track of the number of connections made.
  void IncrementConnectionCount() {
    DCHECK(IsMainThread());
    connection_count_++;
  }

  unsigned ConnectionCount() const { return connection_count_; }

  DeferredTaskHandler& GetDeferredTaskHandler() const {
    return *deferred_task_handler_;
  }
  //
  // Thread Safety and Graph Locking:
  //
  // The following functions call corresponding functions of
  // DeferredTaskHandler.
  bool IsAudioThread() const {
    return GetDeferredTaskHandler().IsAudioThread();
  }
  void lock() { GetDeferredTaskHandler().lock(); }
  bool TryLock() { return GetDeferredTaskHandler().TryLock(); }
  void unlock() { GetDeferredTaskHandler().unlock(); }

  // Returns true if this thread owns the context's lock.
  bool IsGraphOwner() { return GetDeferredTaskHandler().IsGraphOwner(); }

  using GraphAutoLocker = DeferredTaskHandler::GraphAutoLocker;

  // Returns the maximum numuber of channels we can support.
  static unsigned MaxNumberOfChannels() { return kMaxNumberOfChannels; }

  // EventTarget
  const AtomicString& InterfaceName() const final;
  ExecutionContext* GetExecutionContext() const final;

  DEFINE_ATTRIBUTE_EVENT_LISTENER(statechange);

  // Start the AudioContext. `isAllowedToStart()` MUST be called
  // before.  This does NOT set the context state to running.  The
  // caller must set the state AFTER calling startRendering.
  void StartRendering();

  void NotifyStateChange();

  // A context is considered closed if:
  //  - closeContext() has been called.
  //  - it has been stopped by its execution context.
  virtual bool IsContextClosed() const { return is_cleared_; }

  // Get the security origin for this audio context.
  const SecurityOrigin* GetSecurityOrigin() const;

  // Get the PeriodicWave for the specified oscillator type.  The table is
  // initialized internally if necessary.
  PeriodicWave* GetPeriodicWave(int type);

  // For metrics purpose, records when start() is called on a
  // AudioScheduledSourceHandler or a AudioBufferSourceHandler without a user
  // gesture while the AudioContext requires a user gesture.
  void MaybeRecordStartAttempt();

  // AudioWorklet IDL
  AudioWorklet* audioWorklet() const;

  // Callback from AudioWorklet, invoked when the associated
  // AudioWorkletGlobalScope is created and the worklet operation is ready after
  // the first script evaluation.
  void NotifyWorkletIsReady();

  // Update the information in AudioWorkletGlobalScope synchronously on the
  // worklet rendering thread. Must be called from the rendering thread.
  // Does nothing when the worklet global scope does not exist.
  void UpdateWorkletGlobalScopeOnRenderingThread();

  // Returns true if the URL would taint the origin so that we shouldn't be
  // allowing media to played through webaudio.
  // TODO(crbug.com/845913): This should really be on an AudioContext.  Move
  // this when we move the media stuff from BaseAudioContext to AudioContext, as
  // requried by the spec.
  bool WouldTaintOrigin(const KURL& url) const;

 protected:
  enum ContextType { kRealtimeContext, kOfflineContext };

  explicit BaseAudioContext(Document*, enum ContextType);

  void Initialize();
  void Uninitialize();

  void SetContextState(AudioContextState);

  virtual void DidClose() {}

  // Tries to handle AudioBufferSourceNodes that were started but became
  // disconnected or was never connected. Because these never get pulled
  // anymore, they will stay around forever. So if we can, try to stop them so
  // they can be collected.
  void HandleStoppableSourceNodes();

  Member<AudioDestinationNode> destination_node_;

  // FIXME(dominicc): Move m_resumeResolvers to AudioContext, because only
  // it creates these Promises.
  // Vector of promises created by resume(). It takes time to handle them, so we
  // collect all of the promises here until they can be resolved or rejected.
  HeapVector<Member<ScriptPromiseResolver>> resume_resolvers_;

  void SetClosedContextSampleRate(float new_sample_rate) {
    closed_context_sample_rate_ = new_sample_rate;
  }
  float ClosedContextSampleRate() const { return closed_context_sample_rate_; }

  void RejectPendingDecodeAudioDataResolvers();

  // If any, unlock user gesture requirements if a user gesture is being
  // processed.
  void MaybeUnlockUserGesture();

  // Returns whether the AudioContext is allowed to start rendering.
  bool IsAllowedToStart() const;

  AudioIOPosition OutputPosition();

 private:
  friend class BaseAudioContextAutoplayTest;
  friend class DISABLED_BaseAudioContextAutoplayTest;

  // Do not change the order of this enum, it is used for metrics.
  enum AutoplayStatus {
    // The AudioContext failed to activate because of user gesture requirements.
    kAutoplayStatusFailed = 0,
    // Same as AutoplayStatusFailed but start() on a node was called with a user
    // gesture.
    kAutoplayStatusFailedWithStart = 1,
    // The AudioContext had user gesture requirements and was able to activate
    // with a user gesture.
    kAutoplayStatusSucceeded = 2,

    // Keep at the end.
    kAutoplayStatusCount
  };

  bool is_cleared_;
  void Clear();

  // When the context goes away, there might still be some sources which
  // haven't finished playing.  Make sure to release them here.
  void ReleaseActiveSourceNodes();

  // Returns the Document wich wich the instance is associated.
  Document* GetDocument() const;

  // Returns the AutoplayPolicy currently applying to this instance.
  AutoplayPolicy::Type GetAutoplayPolicy() const;

  // Returns whether the autoplay requirements are fulfilled.
  bool AreAutoplayRequirementsFulfilled() const;

  // Listener for the PannerNodes
  Member<AudioListener> listener_;

  // Accessed by audio thread and main thread, coordinated using
  // the associated mutex.
  //
  // These raw pointers are safe because AudioSourceNodes in
  // active_source_nodes_ own them.
  Mutex finished_source_handlers_mutex_;
  Vector<AudioHandler*> finished_source_handlers_;

  // List of source nodes. This is either accessed when the graph lock is
  // held, or on the main thread when the audio thread has finished.
  // Oilpan: This Vector holds connection references. We must call
  // AudioHandler::makeConnection when we add an AudioNode to this, and must
  // call AudioHandler::breakConnection() when we remove an AudioNode from
  // this.
  HeapVector<Member<AudioNode>> active_source_nodes_;

  // Called by the audio thread to handle Promises for resume() and suspend(),
  // posting a main thread task to perform the actual resolving, if needed.
  //
  // TODO(dominicc): Move to AudioContext because only it creates
  // these Promises.
  void ResolvePromisesForUnpause();

  // The audio thread relies on the main thread to perform some operations
  // over the objects that it owns and controls; |ScheduleMainThreadCleanup()|
  // posts the task to initiate those.
  //
  // That is, we combine all those sub-tasks into one task action for
  // convenience and performance, |PerformCleanupOnMainThread()|. It handles
  // promise resolving, stopping and finishing up of audio source nodes etc.
  // Actions that should happen, but can happen asynchronously to the
  // audio thread making rendering progress.
  void ScheduleMainThreadCleanup();
  void PerformCleanupOnMainThread();

  // When the context is going away, reject any pending script promise
  // resolvers.
  virtual void RejectPendingResolvers();

  // Record the current autoplay status and clear it.
  void RecordAutoplayStatus();

  // True if we're in the process of resolving promises for resume().  Resolving
  // can take some time and the audio context process loop is very fast, so we
  // don't want to call resolve an excessive number of times.
  bool is_resolving_resume_promises_;

  // Set to |true| by the audio thread when it posts a main-thread task to
  // perform delayed state sync'ing updates that needs to be done on the main
  // thread. Cleared by the main thread task once it has run.
  bool has_posted_cleanup_task_;

  // Whether a user gesture is required to start this AudioContext.
  bool user_gesture_required_;

  unsigned connection_count_;

  // Graph locking.
  scoped_refptr<DeferredTaskHandler> deferred_task_handler_;

  // The state of the BaseAudioContext.
  AudioContextState context_state_;

  AsyncAudioDecoder audio_decoder_;

  // When a context is closed, the sample rate is cleared.  But decodeAudioData
  // can be called after the context has been closed and it needs the sample
  // rate.  When the context is closed, the sample rate is saved here.
  float closed_context_sample_rate_;

  // Vector of promises created by decodeAudioData.  This keeps the resolvers
  // alive until decodeAudioData finishes decoding and can tell the main thread
  // to resolve them.
  HeapHashSet<Member<ScriptPromiseResolver>> decode_audio_resolvers_;

  // PeriodicWave's for the builtin oscillator types.  These only depend on the
  // sample rate. so they can be shared with all OscillatorNodes in the context.
  // To conserve memory, these are lazily initialized on first use.
  Member<PeriodicWave> periodic_wave_sine_;
  Member<PeriodicWave> periodic_wave_square_;
  Member<PeriodicWave> periodic_wave_sawtooth_;
  Member<PeriodicWave> periodic_wave_triangle_;

  // This is considering 32 is large enough for multiple channels audio.
  // It is somewhat arbitrary and could be increased if necessary.
  enum { kMaxNumberOfChannels = 32 };

  base::Optional<AutoplayStatus> autoplay_status_;
  AudioIOPosition output_position_;

  Member<AudioWorklet> audio_worklet_;

  // In order to update some information (e.g. current frame) in
  // AudioWorkletGlobalScope *synchronously*, the context needs to keep the
  // reference to the WorkerThread associated with the AudioWorkletGlobalScope.
  // This cannot be nullptr once it is assigned from AudioWorkletThread until
  // the BaseAudioContext goes away.
  WorkerThread* audio_worklet_thread_ = nullptr;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_BASE_AUDIO_CONTEXT_H_
