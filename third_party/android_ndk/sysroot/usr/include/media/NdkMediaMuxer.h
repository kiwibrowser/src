/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/*
 * This file defines an NDK API.
 * Do not remove methods.
 * Do not change method signatures.
 * Do not change the value of constants.
 * Do not change the size of any of the classes defined in here.
 * Do not reference types that are not part of the NDK.
 * Do not #include files that aren't part of the NDK.
 */

#ifndef _NDK_MEDIA_MUXER_H
#define _NDK_MEDIA_MUXER_H

#include <sys/cdefs.h>
#include <sys/types.h>

#include "NdkMediaCodec.h"
#include "NdkMediaError.h"
#include "NdkMediaFormat.h"

__BEGIN_DECLS

struct AMediaMuxer;
typedef struct AMediaMuxer AMediaMuxer;

typedef enum {
    AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4 = 0,
    AMEDIAMUXER_OUTPUT_FORMAT_WEBM   = 1,
} OutputFormat;

#if __ANDROID_API__ >= 21

/**
 * Create new media muxer
 */
AMediaMuxer* AMediaMuxer_new(int fd, OutputFormat format);

/**
 * Delete a previously created media muxer
 */
media_status_t AMediaMuxer_delete(AMediaMuxer*);

/**
 * Set and store the geodata (latitude and longitude) in the output file.
 * This method should be called before AMediaMuxer_start. The geodata is stored
 * in udta box if the output format is AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4, and is
 * ignored for other output formats.
 * The geodata is stored according to ISO-6709 standard.
 *
 * Both values are specified in degrees.
 * Latitude must be in the range [-90, 90].
 * Longitude must be in the range [-180, 180].
 */
media_status_t AMediaMuxer_setLocation(AMediaMuxer*, float latitude, float longitude);

/**
 * Sets the orientation hint for output video playback.
 * This method should be called before AMediaMuxer_start. Calling this
 * method will not rotate the video frame when muxer is generating the file,
 * but add a composition matrix containing the rotation angle in the output
 * video if the output format is AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4, so that a
 * video player can choose the proper orientation for playback.
 * Note that some video players may choose to ignore the composition matrix
 * during playback.
 * The angle is specified in degrees, clockwise.
 * The supported angles are 0, 90, 180, and 270 degrees.
 */
media_status_t AMediaMuxer_setOrientationHint(AMediaMuxer*, int degrees);

/**
 * Adds a track with the specified format.
 * Returns the index of the new track or a negative value in case of failure,
 * which can be interpreted as a media_status_t.
 */
ssize_t AMediaMuxer_addTrack(AMediaMuxer*, const AMediaFormat* format);

/**
 * Start the muxer. Should be called after AMediaMuxer_addTrack and
 * before AMediaMuxer_writeSampleData.
 */
media_status_t AMediaMuxer_start(AMediaMuxer*);

/**
 * Stops the muxer.
 * Once the muxer stops, it can not be restarted.
 */
media_status_t AMediaMuxer_stop(AMediaMuxer*);

/**
 * Writes an encoded sample into the muxer.
 * The application needs to make sure that the samples are written into
 * the right tracks. Also, it needs to make sure the samples for each track
 * are written in chronological order (e.g. in the order they are provided
 * by the encoder.)
 */
media_status_t AMediaMuxer_writeSampleData(AMediaMuxer *muxer,
        size_t trackIdx, const uint8_t *data, const AMediaCodecBufferInfo *info);

#endif /* __ANDROID_API__ >= 21 */

__END_DECLS

#endif // _NDK_MEDIA_MUXER_H
