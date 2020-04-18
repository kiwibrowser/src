// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Closure definitions of Mojo service objects. Note that these
 *     definitions are bound only after chrome.mojoPrivate.requireAsync resolves
 *     in mojo.js.
 */


var mojo = {};



// Core Mojo.

/**
 * @param {!Object} interfaceType
 * @param {!Object} impl
 * @param {mojo.InterfaceRequest=} request
 * @constructor
 */
mojo.Binding = function(interfaceType, impl, request) {};


/**
 * Closes the message pipe. The bound object will no longer receive messages.
 */
mojo.Binding.prototype.close = function() {};


/**
 * Binds to the given request.
 * @param {!mojo.InterfaceRequest} request
 */
mojo.Binding.prototype.bind = function(request) {};


/** @param {function()} callback */
mojo.Binding.prototype.setConnectionErrorHandler = function(callback) {};


/**
 * Creates an interface ptr and bind it to this instance.
 * @return {?} The interface ptr.
 */
mojo.Binding.prototype.createInterfacePtrAndBind = function() {};



/** @constructor */
mojo.InterfaceRequest = function() {};


/**
 * Closes the message pipe. The object can no longer be bound to an
 * implementation.
 */
mojo.InterfaceRequest.prototype.close = function() {};



/** @constructor */
mojo.InterfacePtrController = function() {};


/**
 * Closes the message pipe. Messages can no longer be sent with this object.
 */
mojo.InterfacePtrController.prototype.reset = function() {};


/** @param {function()} callback */
mojo.InterfacePtrController.prototype.setConnectionErrorHandler = function(
    callback) {};



/**
 * @param {!Object} interfacePtr
 */
mojo.makeRequest = function(interfacePtr) {};


// Mojom structs.

/**
 * @constructor
 * @struct
 */
mojo.IPAddress = function() {};



/** @type {!Array<number>} */
mojo.IPAddress.prototype.address;


/** @type {!Array<number>} */
mojo.IPAddress.prototype.address_bytes;



/**
 * @constructor
 * @struct
 */
mojo.IPEndPoint = function() {};


/** @type {mojo.IPAddress} */
mojo.IPEndPoint.prototype.address;


/** @type {number} */
mojo.IPEndPoint.prototype.port;



/** @constructor */
mojo.Url = function() {};


/** @type {string} */
mojo.Url.prototype.url;



/**
 * @constructor
 * @struct
 */
mojo.DialMediaSink = function() {};


/** @type {mojo.IPAddress} */
mojo.DialMediaSink.prototype.ip_address;


/** @type {string} */
mojo.DialMediaSink.prototype.model_name;


/** @type {mojo.Url} */
mojo.DialMediaSink.prototype.app_url;



/**
 * @constructor
 * @struct
 */
mojo.CastMediaSink = function() {};



/** @type {mojo.IPAddress} */
mojo.CastMediaSink.prototype.ip_address;


/** @type {mojo.IPEndPoint} */
mojo.CastMediaSink.prototype.ip_endpoint;


/** @type {string} */
mojo.CastMediaSink.prototype.model_name;


/** @type {number} */
mojo.CastMediaSink.prototype.capabilities;


/** @type {number} */
mojo.CastMediaSink.prototype.cast_channel_id;



/**
 * @constructor
 * @struct
 */
mojo.SinkExtraData = function() {};


/** @type {mojo.DialMediaSink} */
mojo.SinkExtraData.prototype.dial_media_sink;


/** @type {mojo.CastMediaSink} */
mojo.SinkExtraData.prototype.cast_media_sink;



/**
 * @constructor
 * @struct
 */
mojo.Sink = function() {};


/** @constructor */
mojo.SinkIconType = function() {};
mojo.SinkIconType.CAST = 0;
mojo.SinkIconType.CAST_AUDIO_GROUP = 1;
mojo.SinkIconType.CAST_AUDIO = 2;
mojo.SinkIconType.MEETING = 3;
mojo.SinkIconType.HANGOUT = 4;
mojo.SinkIconType.EDUCATION = 5;
mojo.SinkIconType.GENERIC = 6;


/** @type {string} */
mojo.Sink.prototype.sink_id;


/** @type {string} */
mojo.Sink.prototype.name;


/** @type {?string} */
mojo.Sink.prototype.description;


/** @type {?string} */
mojo.Sink.prototype.domain;


/** @type {?mojo.SinkIconType} */
mojo.Sink.prototype.icon_type;


/** @type {?mojo.SinkExtraData} */
mojo.Sink.prototype.extra_data;



/**
 * @param {!Object} values An object mapping from property names to values.
 * @constructor
 * @struct
 */
mojo.TimeDelta = function(values) {};


/** @type {number} */
mojo.TimeDelta.prototype.microseconds;



/**
 * @param {!Object} values An object mapping from property names to values.
 * @constructor
 * @struct
 */
mojo.MediaStatus = function(values) {};


/** @constructor */
mojo.MediaStatus.PlayState = function() {};
/** @type {!mojo.MediaStatus.PlayState} */
mojo.MediaStatus.PlayState.PLAYING;
/** @type {!mojo.MediaStatus.PlayState} */
mojo.MediaStatus.PlayState.PAUSED;
/** @type {!mojo.MediaStatus.PlayState} */
mojo.MediaStatus.PlayState.BUFFERING;


/** @type {?string} */
mojo.MediaStatus.prototype.title;


/** @type {?string} */
mojo.MediaStatus.prototype.description;


/** @type {boolean} */
mojo.MediaStatus.prototype.can_play_pause;


/** @type {boolean} */
mojo.MediaStatus.prototype.can_mute;


/** @type {boolean} */
mojo.MediaStatus.prototype.can_set_volume;


/** @type {boolean} */
mojo.MediaStatus.prototype.can_seek;


/** @type {?mojo.MediaStatus.PlayState} */
mojo.MediaStatus.prototype.play_state;


/** @type {boolean} */
mojo.MediaStatus.prototype.is_muted;


/** @type {number} */
mojo.MediaStatus.prototype.volume;


/** @type {?mojo.TimeDelta} */
mojo.MediaStatus.prototype.duration;


/** @type {?mojo.TimeDelta} */
mojo.MediaStatus.prototype.current_time;


/** @type {mojo.HangoutsMediaStatusExtraData} */
mojo.MediaStatus.prototype.hangouts_extra_data;



/**
 * @param {!Object} values
 * @constructor
 * @struct
 */
mojo.HangoutsMediaStatusExtraData = function(values) {};


/** @type {boolean} */
mojo.HangoutsMediaStatusExtraData.prototype.local_present;



/**
 * @param {!Object} values An object mapping from property names to values.
 * @constructor
 * @struct
 */
mojo.Origin = function(values) {};


/** @type {string} */
mojo.Origin.prototype.scheme;


/** @type {string} */
mojo.Origin.prototype.host;


/** @type {number} */
mojo.Origin.prototype.port;


/** @type {string} */
mojo.Origin.prototype.suborigin;


/** @type {boolean} */
mojo.Origin.prototype.unique;



/** @constructor */
mojo.RouteControllerType = function() {};
/** @type {mojo.RouteControllerType} */
mojo.RouteControllerType.kNone;
/** @type {mojo.RouteControllerType} */
mojo.RouteControllerType.kGeneric;
/** @type {mojo.RouteControllerType} */
mojo.RouteControllerType.kHangouts;
/** @type {mojo.RouteControllerType} */
mojo.RouteControllerType.kMirroring;


// Mojom interfaces.

/**
 * @constructor
 * @struct
 */
mojo.MediaStatusObserverPtr = function() {};


/** @param {!mojo.MediaStatus} status */
mojo.MediaStatusObserverPtr.prototype.onMediaStatusUpdated = function(status) {
};


/** @type {!mojo.InterfacePtrController} */
mojo.MediaStatusObserverPtr.prototype.ptr;



/** @type {!Object} */
mojo.MediaController;


/** @type {!Object} */
mojo.HangoutsMediaRouteController;


/** @constructor */
mojo.MediaRouteProviderConfig = function() {};


/** @type {boolean} */
mojo.MediaRouteProviderConfig.prototype.enable_dial_discovery;


/** @type {boolean} */
mojo.MediaRouteProviderConfig.prototype.enable_dial_sink_query;


/** @type {boolean} */
mojo.MediaRouteProviderConfig.prototype.enable_cast_discovery;


/** @type {boolean} */
mojo.MediaRouteProviderConfig.prototype.enable_cast_sink_query;



/** @constructor */
mojo.RemotingStopReason = function() {};
/** @type {!mojo.RemotingStopReason} */
mojo.RemotingStopReason.ROUTE_TERMINATED;
/** @type {!mojo.RemotingStopReason} */
mojo.RemotingStopReason.USER_DISABLED;



/** @constructor */
mojo.RemotingSinkFeature = function() {};



/** @constructor */
mojo.RemotingSinkAudioCapability = function() {};
/** @type {!mojo.RemotingSinkAudioCapability} */
mojo.RemotingSinkAudioCapability.CODEC_BASELINE_SET;
/** @type {!mojo.RemotingSinkAudioCapability} */
mojo.RemotingSinkAudioCapability.CODEC_AAC;
/** @type {!mojo.RemotingSinkAudioCapability} */
mojo.RemotingSinkAudioCapability.CODEC_OPUS;



/** @constructor */
mojo.RemotingSinkVideoCapability = function() {};
/** @type {!mojo.RemotingSinkVideoCapability} */
mojo.RemotingSinkVideoCapability.SUPPORT_4K;
/** @type {!mojo.RemotingSinkVideoCapability} */
mojo.RemotingSinkVideoCapability.CODEC_BASELINE_SET;
/** @type {!mojo.RemotingSinkVideoCapability} */
mojo.RemotingSinkVideoCapability.CODEC_H264;
/** @type {!mojo.RemotingSinkVideoCapability} */
mojo.RemotingSinkVideoCapability.CODEC_VP8;
/** @type {!mojo.RemotingSinkVideoCapability} */
mojo.RemotingSinkVideoCapability.CODEC_VP9;
/** @type {!mojo.RemotingSinkVideoCapability} */
mojo.RemotingSinkVideoCapability.CODEC_HEVC;



/**
 * @constructor
 * @struct
 */
mojo.RemotingSinkMetadata = function() {};


/** @type {!Array<mojo.RemotingSinkFeature>} */
mojo.RemotingSinkMetadata.prototype.features;


/** @type {!Array<mojo.RemotingSinkAudioCapability>} */
mojo.RemotingSinkMetadata.prototype.audio_capabilities;


/** @type {!Array<mojo.RemotingSinkVideoCapability>} */
mojo.RemotingSinkMetadata.prototype.video_capabilities;


/** @type {!string} */
mojo.RemotingSinkMetadata.prototype.friendly_name;



/** @type {!Object} */
mojo.MirrorServiceRemoter;



/**
 * @constructor
 */
mojo.MirrorServiceRemoterPtr = function() {};


/** @type {!mojo.InterfacePtrController} */
mojo.MirrorServiceRemoterPtr.prototype.ptr;



/**
 * @constructor
 */
mojo.MirrorServiceRemotingSourcePtr = function() {};


/** @param {!mojo.RemotingSinkMetadata} capabilities */
mojo.MirrorServiceRemotingSourcePtr.prototype.onSinkAvailable = function(
    capabilities) {};

/** @param {!Uint8Array} message */
mojo.MirrorServiceRemotingSourcePtr.prototype.onMessageFromSink = function(
    message) {};


/** @param {!mojo.RemotingStopReason} reason */
mojo.MirrorServiceRemotingSourcePtr.prototype.onStopped = function(reason) {};


/** Notifies the remoting source when streaming error occurs. */
mojo.MirrorServiceRemotingSourcePtr.prototype.onError = function() {};


/** @type {!mojo.InterfacePtrController} */
mojo.MirrorServiceRemotingSourcePtr.prototype.ptr;
