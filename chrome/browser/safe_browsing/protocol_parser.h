// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_PROTOCOL_PARSER_H_
#define CHROME_BROWSER_SAFE_BROWSING_PROTOCOL_PARSER_H_

// Parsers and formatters for SafeBrowsing v3.0 protocol:
// https://developers.google.com/safe-browsing/developers_guide_v3
//
// The quoted references are with respect to that document.

#include <stddef.h>

#include <string>
#include <vector>

#include "chrome/browser/safe_browsing/safe_browsing_util.h"
#include "components/safe_browsing/db/util.h"

namespace base {
class TimeDelta;
};

namespace safe_browsing {

// TODO(shess): Maybe the data/len pairs could be productively replaced with
// const base::StringPiece&.

// Parse body of "HTTP Response for Data".  |*next_update_sec| is the minimum
// delay to next update.  |*reset| is set to true if the update requested a
// database reset.  |*chunk_deletes| receives add-del and sub-del requests,
// while |*chunk_urls| receives the list of redirect urls to fetch.  Returns
// |false| if the update could not be decoded properly, in which case all
// results should be discarded.
bool ParseUpdate(const char* chunk_data,
                 size_t chunk_len,
                 size_t* next_update_sec,
                 bool* reset,
                 std::vector<SBChunkDelete>* chunk_deletes,
                 std::vector<ChunkUrl>* chunk_urls);

// Parse body of a redirect response.  |*chunks| receives the parsed chunk data.
// Returns |false| if the data could not be parsed correctly, in which case all
// results should be discarded.
bool ParseChunk(const char* chunk_data,
                size_t chunk_len,
                std::vector<std::unique_ptr<SBChunkData>>* chunks);

// Parse body of "HTTP Response for Full-Length Hashes", returning the list of
// full hashes.  Returns |false| if the data could not be parsed correctly, in
// which case all results should be discarded.
bool ParseGetHash(const char* chunk_data,
                  size_t chunk_len,
                  base::TimeDelta* cache_lifetime,
                  std::vector<SBFullHashResult>* full_hashes);

// Convert prefix hashes into a "HTTP Request for Full-Length Hashes" body.
std::string FormatGetHash(const std::vector<SBPrefix>& prefixes);

// Format the LIST part of "HTTP Request for Data" body.
std::string FormatList(const SBListChunkRanges& list);

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_PROTOCOL_PARSER_H_
