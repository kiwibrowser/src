/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

//
// checked_cast.h
//
// A template system, intended to be a drop-in replacement for static_cast<>,
// which performs compile-time and (if necessary) runtime evaluation of
// integral type casts to detect and handle arithmetic overflow errors.
//
#ifndef NATIVE_CLIENT_SRC_INCLUDE_CHECKED_CAST_H_
#define NATIVE_CLIENT_SRC_INCLUDE_CHECKED_CAST_H_ 1

#include "native_client/src/include/build_config.h"

// Windows defines std::min and std::max in a different header
// than gcc prior to Visual Studio 2013.
#if NACL_WINDOWS
#include <xutility>
#endif

#include <algorithm>
#include <limits>

// TODO(ilewis): remove reference to base as soon as we can get COMPILE_ASSERT
//                from another source.
#include "native_client/src/shared/platform/nacl_log.h"

namespace nacl {

  //
  // Function to determine whether an information-preserving cast
  // is possible. In other words, if this function returns true, then
  // the input value has an exact representation in the target type.
  //
  template<typename target_t, typename source_t>
  bool can_cast(const source_t& input);

  //
  // Function to safely cast from one type to another, with a customizable
  // policy determining what happens if the cast cannot complete without
  // losing information.
  //
  template <
    typename target_t,
    typename source_t,
    template<typename, typename> class trunc_policy>
  target_t checked_cast(const source_t& input);

  //
  // Convenience wrappers for specializations of checked_cast with
  // different truncation policies
  //
  template<typename T, typename S>
  T assert_cast(const S& input);

  template<typename T, typename S>
  T saturate_cast(const S& input);


  //
  // Helper function prototypes
  //
  //
  namespace CheckedCast {
    //
    // OnTruncate* functions: policy functions that define
    // what to do if a checked cast can't be made without
    // truncation that loses data.
    //
    template <typename target_t, typename source_t>
    struct TruncationPolicySaturate {
      static target_t OnTruncate(const source_t& input);
    };

    template <typename target_t, typename source_t>
    struct TruncationPolicyAbort {
      static target_t OnTruncate(const source_t& input);
    };

    namespace detail {
//-----------------------------------------------------------------------------
// RuntimeHelpers
//
// Ugly nested templates--necessary because GCC is ever vigilant about type
// safety, and has no way to temporarily disable certain warnings
//
//-----------------------------------------------------------------------------

      //
      // TrivialityChecker template
      // Makes decisions about whether a cast is trivial (does not require
      // the value of the input to be checked).
      //
      // The specializations are necessary to avoid running comparisons that
      // are either invalid or always true, since these comparisons trigger
      // warnings.
      //
      template<typename tlimits, typename slimits, bool IsCompileTimeTrivial>
      struct TrivialityChecker {
        static bool IsRuntimeTrivial() { return false; }
      };

      template<typename tlimits, typename slimits>
      struct TrivialityChecker<tlimits, slimits, true> {
        static bool IsRuntimeTrivial() {
          // Split into variables to bypass const value warning
          bool tlimits_lequ_min_slimits = tlimits::min() <= slimits::min();
          bool tlimits_gequ_max_slimits = tlimits::max() >= slimits::max();
          bool trivial = tlimits_lequ_min_slimits && tlimits_gequ_max_slimits;
          return trivial;
        }
      };

      //
      // Casting versions of std::min and std::max.
      // These should only be called from code that has checked to make
      // sure the cast is valid, otherwise compiler warnings will be
      // triggered.
      //
      template<typename A, typename B>
      static B cast_min(const A& a, const B& b) {
        return (static_cast<B>(a) < b) ? static_cast<B>(a) : b;
      }

      template<typename A, typename B>
      static B cast_max(const A& a, const B& b) {
        return (static_cast<B>(a) > b) ? static_cast<B>(a) : b;
      }

      //
      // Template specializations for determining valid ranges for
      // any given typecast. Specialized for different combinations
      // of signed/unsigned types.
      //
      template<typename target_t,
               typename source_t,
               bool SignednessDiffers,
               bool TargetIsSigned>
      struct RangeDetail {
        typedef std::numeric_limits<target_t> tlimits;
        typedef std::numeric_limits<source_t> slimits;

        static source_t OverlapMin() {
          return cast_max(tlimits::min(), slimits::min());
        }
        static source_t OverlapMax() {
          return cast_min(tlimits::max(), slimits::max());
        }
      };

      // signed target / unsigned source
      template<typename target_t, typename source_t>
      struct RangeDetail<target_t, source_t, true, true> {
        typedef std::numeric_limits<target_t> tlimits;
        typedef std::numeric_limits<source_t> slimits;

        static source_t OverlapMin() {
          if (tlimits::min() >= 0) {
            return cast_max(tlimits::min(), slimits::min());
          } else {
            return slimits::min();
          }
        }
        static source_t OverlapMax() {
          if (tlimits::max() >= 0) {
            return cast_min(tlimits::max(), slimits::max());
          } else {
            return slimits::min();
          }
        }
      };

      // unsigned target / signed source
      template<typename target_t, typename source_t>
      struct RangeDetail<target_t, source_t, true, false> {
        typedef std::numeric_limits<target_t> tlimits;
        typedef std::numeric_limits<source_t> slimits;

        static source_t OverlapMin() {
          if (slimits::min() >= 0) {
            return cast_max(tlimits::min(), slimits::min());
          } else if (slimits::max() >= 0) {
            return cast_min(tlimits::min(), slimits::max());
          } else {
            return slimits::min();
          }
        }
        static source_t OverlapMax() {
          if (slimits::max() >= 0) {
            return cast_min(tlimits::max(), slimits::max());
          } else {
            return slimits::min();
          }
        }
      };

      //
      // Wrapper for RangeDetail. Prevents RangeDetail objects
      // from being instantiated for unsupported types.
      //
      template<typename target_t, typename source_t, bool IsSupported>
      struct RangeHelper {
        typedef std::numeric_limits<target_t> tlimits;
        typedef std::numeric_limits<source_t> slimits;

        static bool OverlapExists() { return false; }

        // The return values from OverlapMin() and OverlapMax() are
        // arbitrary if OverlapExists() returns false, so we'll
        // just return the minimum source value for both.
        static source_t OverlapMin() { return slimits::min(); }
        static source_t OverlapMax() { return slimits::min(); }
      };

      template<typename target_t, typename source_t>
      struct RangeHelper<target_t, source_t, true> {
        typedef std::numeric_limits<target_t> tlimits;
        typedef std::numeric_limits<source_t> slimits;

        typedef RangeDetail<target_t,
          source_t,
          (tlimits::is_signed != slimits::is_signed),
          tlimits::is_signed> detail_t;

        static bool OverlapExists() {
          return detail_t::OverlapMin() < detail_t::OverlapMax();
        }
        static source_t OverlapMin() { return detail_t::OverlapMin(); }
        static source_t OverlapMax() { return detail_t::OverlapMax(); }
      };

      //
      // CastInfo
      //
      // Maintains information about how to cast between
      // two types.
      //
      template<typename target_t, typename source_t>
      struct CastInfo {
        typedef std::numeric_limits<target_t> tlimits;
        typedef std::numeric_limits<source_t> slimits;

        static const bool kSupported = tlimits::is_specialized
          && slimits::is_specialized
          && tlimits::is_integer
          && slimits::is_integer
          && tlimits::is_bounded
          && slimits::is_bounded;

        static const bool kIdenticalSignedness =
          (tlimits::is_signed == slimits::is_signed);

        // "digits" in numeric_limits refers to binary
        // digits. (Decimal digits are stored in a field
        // called digits10.)
        static const bool kSufficientBits =
          (tlimits::digits >= slimits::digits);

        static const bool kCompileTimeTrivial = kSupported
          && kIdenticalSignedness
          && kSufficientBits;

        typedef TrivialityChecker<tlimits,
                                  slimits,
                                  kCompileTimeTrivial> trivial_t;
        typedef RangeHelper<target_t, source_t, kSupported> range_t;

        // Can the cast be done without runtime checks--i.e. are
        // all possible input values also valid output values?
        static bool RuntimeTrivial() {
          return trivial_t::IsRuntimeTrivial();
        }

        // Are there any valid input values for which a cast would succeed?
        static bool RuntimePossible() {
          return range_t::OverlapExists();
        }

        // Is the given source value a valid target value?
        static bool RuntimeRangeCheck(const source_t& src) {
          return (range_t::OverlapExists()
                  && src <= range_t::OverlapMax()
                  && src >= range_t::OverlapMin());
        }

        // Range of source values which are also valid target values.
        static source_t RuntimeRangeMin() { return range_t::OverlapMin(); }
        static source_t RuntimeRangeMax() { return range_t::OverlapMax(); }
      };
    }  // namespace detail
  }  // namespace CheckedCast
}  // namespace nacl


//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// checked_cast(const source_t& input)
//    An augmented replacement for static_cast. Does range checking,
//    overflow validation. Includes a policy which allows the caller to
//    specify what action to take if it's determined that the cast
//    would lose data.
//
// Template parameters:
//    target_t:     the type on the left hand side of the cast
//    source_t:     the type on the right hand side of the cast
//    trunc_policy: type of a policy object that will be called
//                if the cast would result in a loss of data. The
//                type must implement a method named OnTruncate which
//                is compatible with the following signature:
//                target_t (target_t& output, const source_t& input).
//                It is the responsibility of this function to
//                convert the value of 'input' and store the result
//                of the conversion in out parameter 'output'. It is
//                permissible for the function to abort or log warnings
//                if that is the desired behavior.
//
// Function parameters:
//    input:    the value to convert.
//
// usage:
//    // naive truncation handler for sample purposes ONLY!!
//    template <typename T, typename S>
//    void naive_trunc(T& o, const S& i) {
//      // this only gets called if checked_cast can't do a safe automatic
//      // conversion. In real code you'd want to do something cool like
//      // clamp the incoming value or abort the program. For this sample
//      // we just return a c-style cast, which makes the outcome roughly
//      // equivalent to static_cast<>.
//      o = (T)i;
//    }
//
//    void main() {
//      uint64_t foo = 0xffff0000ffff0000;
//      uint32_t bar = checked_cast<uint32_t>(foo, naive_trunc);
//    }
//
template <
    typename target_t,
    typename source_t,
    template<typename, typename> class trunc_policy>
target_t nacl::checked_cast(const source_t& input) {
  target_t output;

  //
  // Runtime checks--these should compile out for all basic types
  //
  if (nacl::can_cast<target_t>(input)) {
    output = static_cast<target_t>(input);
  } else {
    output = trunc_policy<target_t, source_t>::OnTruncate(input);
  }

  return output;
}

//-----------------------------------------------------------------------------
// can_cast(const source_t& input)
//  Returns true if checked_cast will return without invoking trunc_policy.
//-----------------------------------------------------------------------------
template <typename target_t, typename source_t>
bool nacl::can_cast(const source_t& input) {
  typedef CheckedCast::detail::CastInfo<target_t, source_t> info;

  bool result;

  //
  // Runtime checks--these should compile out for all basic types
  //
  result = info::RuntimeTrivial()
    || (info::RuntimePossible()
    && info::RuntimeRangeCheck(input));

  return result;
}


//
// Convenience wrappers for specializations of checked_cast
//

//-----------------------------------------------------------------------------
// checked_cast_fatal(const S& input)
// Calls checked_cast; on truncation, log error and abort
//-----------------------------------------------------------------------------
template<typename T, typename S>
T nacl::assert_cast(const S& input) {
  return checked_cast<T, S, CheckedCast::TruncationPolicyAbort>(input);
}
//-----------------------------------------------------------------------------
// saturate_cast(const S& input)
// Calls checked_cast; on truncation, saturates the input to the minimum
// or maximum of the output.
//-----------------------------------------------------------------------------
template<typename T, typename S>
T nacl::saturate_cast(const S& input) {
  return
    checked_cast<T, S, CheckedCast::TruncationPolicySaturate>(input);
}

//-----------------------------------------------------------------------------
// CheckedCastDetail::OnTruncationSaturate
// Implements the Saturate truncation policy.
//-----------------------------------------------------------------------------
template <typename target_t, typename source_t>
target_t nacl
    ::CheckedCast
    ::TruncationPolicySaturate<target_t, source_t>
    ::OnTruncate(const source_t& input) {
  typedef detail::CastInfo<target_t, source_t> info;

  source_t clamped = input;
  bool valid = info::RuntimePossible();

  if (!valid) {
    NaClLog(LOG_FATAL, "Checked cast: type ranges do not overlap");
  }

  clamped = std::max(clamped, info::RuntimeRangeMin());
  clamped = std::min(clamped, info::RuntimeRangeMax());

  target_t output = static_cast<target_t>(clamped);

  return output;
}



//-----------------------------------------------------------------------------
// CheckedCastDetail::OnTruncationAbort
// Implements the Abort truncation policy.
//-----------------------------------------------------------------------------
template <typename target_t, typename source_t>
target_t nacl
    ::CheckedCast
    ::TruncationPolicyAbort<target_t, source_t>
    ::OnTruncate(const source_t&) {
  NaClLog(LOG_FATAL, "Arithmetic overflow");

  // Unreachable, assuming that LOG_FATAL really is fatal
  return 0;
}


#endif  /* NATIVE_CLIENT_SRC_INCLUDE_CHECKED_CAST_H_ */
