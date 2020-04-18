/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

//
// checked_cast_debug.h
//
// Additional truncation policies for checked_cast. The policies in this header
// emit more diagnostic information and thus impose a higher runtime cost.
//
#ifndef NATIVE_CLIENT_SRC_INCLUDE_CHECKED_CAST_DEBUG_H_
#define NATIVE_CLIENT_SRC_INCLUDE_CHECKED_CAST_DEBUG_H_ 1


#include "checked_cast.h"


// CPPLint complains about strstream, but in this case its use is justified:
// http://www.corp.google.com/eng/doc/cppguide.xml#Streams states "use streams
// only for logging" and logging is the use case here. (In addition, since
// we're logging data whose type is templatized, printf isn't a particularly
// attractive option.)
#include <strstream>

#include "native_client/src/include/nacl_string.h"

namespace nacl {

  template<typename T, typename S>
  INLINE T checked_cast_log_fatal(const S& input);

  namespace CheckedCast {

    template <typename target_t, typename source_t>
    struct TruncationPolicyLogFatal {
      static target_t OnTruncate(const source_t& input);
    };

  }  // namespace CheckedCastDetail
}  // namespace nacl

//-----------------------------------------------------------------------------
// nacl::checked_cast_log_fatal
// wrapper for checked_cast using the LogFatal trunctation policy.
//-----------------------------------------------------------------------------
template<typename T, typename S>
INLINE T nacl::assert_cast(const S& input) {
  return checked_cast<T, S, CheckedCast::TruncationPolicyLogFatal>(input);
}

//-----------------------------------------------------------------------------
// CheckedCastDetail::OnTruncationAbort
// Implements the Abort truncation policy, plus a bonus informational log
// message.
//-----------------------------------------------------------------------------
template <typename target_t, typename source_t>
INLINE target_t nacl
::CheckedCast
::TruncationPolicyLogFatal<target_t, source_t>
::OnTruncate(const source_t& input) {
  typedef std::numeric_limits<target_t> target_limits;
  typedef std::numeric_limits<source_t> source_limits;

  // Log the error
  //
  // Since we don't know the types of target_t and source_t until
  // template instantiation time, it's not trivial to write a correct
  // printf format string for them. Using std::strstream sidesteps
  // that issue, at least for common types.
  //
  std::ostrstream stm;
  stm << "Overflow converting value " << input << ". "
    << "Valid range for destination type is ("
    << target_limits::min() << ", "
    << target_limits::max() << ")";

  // Transfer the strstream to a nacl::string. This makes sure
  // it's null-terminated.
  nacl::string strLog(stm.str(), stm.pcount());
  NaClLog(LOG_FATAL, "%s", strLog.c_str());

  // Unreachable, assuming that LOG_FATAL really is fatal
  return 0;
}

#endif  // NATIVE_CLIENT_SRC_INCLUDE_CHECKED_CAST_DEBUG_H_
