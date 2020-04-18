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

#ifndef _NDK_MEDIA_EXTRACTOR_H
#define _NDK_MEDIA_EXTRACTOR_H

#include <sys/cdefs.h>
#include <sys/types.h>

#include "NdkMediaCodec.h"
#include "NdkMediaFormat.h"
#include "NdkMediaCrypto.h"

__BEGIN_DECLS

struct AMediaExtractor;
typedef struct AMediaExtractor AMediaExtractor;

#if __ANDROID_API__ >= 21

/**
 * Create new media extractor
 */
AMediaExtractor* AMediaExtractor_new();

/**
 * Delete a previously created media extractor
 */
media_status_t AMediaExtractor_delete(AMediaExtractor*);

/**
 *  Set the file descriptor from which the extractor will read.
 */
media_status_t AMediaExtractor_setDataSourceFd(AMediaExtractor*, int fd, off64_t offset,
        off64_t length);

/**
 * Set the URI from which the extractor will read.
 */
media_status_t AMediaExtractor_setDataSource(AMediaExtractor*, const char *location);
        // TODO support headers

/**
 * Return the number of tracks in the previously specified media file
 */
size_t AMediaExtractor_getTrackCount(AMediaExtractor*);

/**
 * Return the format of the specified track. The caller must free the returned format
 */
AMediaFormat* AMediaExtractor_getTrackFormat(AMediaExtractor*, size_t idx);

/**
 * Select the specified track. Subsequent calls to readSampleData, getSampleTrackIndex and
 * getSampleTime only retrieve information for the subset of tracks selected.
 * Selecting the same track multiple times has no effect, the track is
 * only selected once.
 */
media_status_t AMediaExtractor_selectTrack(AMediaExtractor*, size_t idx);

/**
 * Unselect the specified track. Subsequent calls to readSampleData, getSampleTrackIndex and
 * getSampleTime only retrieve information for the subset of tracks selected..
 */
media_status_t AMediaExtractor_unselectTrack(AMediaExtractor*, size_t idx);

/**
 * Read the current sample.
 */
ssize_t AMediaExtractor_readSampleData(AMediaExtractor*, uint8_t *buffer, size_t capacity);

/**
 * Read the current sample's flags.
 */
uint32_t AMediaExtractor_getSampleFlags(AMediaExtractor*); // see definitions below

/**
 * Returns the track index the current sample originates from (or -1
 * if no more samples are available)
 */
int AMediaExtractor_getSampleTrackIndex(AMediaExtractor*);

/**
 * Returns the current sample's presentation time in microseconds.
 * or -1 if no more samples are available.
 */
int64_t AMediaExtractor_getSampleTime(AMediaExtractor*);

/**
 * Advance to the next sample. Returns false if no more sample data
 * is available (end of stream).
 */
bool AMediaExtractor_advance(AMediaExtractor*);

typedef enum {
    AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC,
    AMEDIAEXTRACTOR_SEEK_NEXT_SYNC,
    AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC
} SeekMode;

/**
 *
 */
media_status_t AMediaExtractor_seekTo(AMediaExtractor*, int64_t seekPosUs, SeekMode mode);

/**
 * mapping of crypto scheme uuid to the scheme specific data for that scheme
 */
typedef struct PsshEntry {
    AMediaUUID uuid;
    size_t datalen;
    void *data;
} PsshEntry;

/**
 * list of crypto schemes and their data
 */
typedef struct PsshInfo {
    size_t numentries;
    PsshEntry entries[0];
} PsshInfo;

/**
 * Get the PSSH info if present.
 */
PsshInfo* AMediaExtractor_getPsshInfo(AMediaExtractor*);


AMediaCodecCryptoInfo *AMediaExtractor_getSampleCryptoInfo(AMediaExtractor *);


enum {
    AMEDIAEXTRACTOR_SAMPLE_FLAG_SYNC = 1,
    AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED = 2,
};

#endif /* __ANDROID_API__ >= 21 */

__END_DECLS

#endif // _NDK_MEDIA_EXTRACTOR_H
