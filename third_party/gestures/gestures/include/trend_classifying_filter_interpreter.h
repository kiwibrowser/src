// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/map.h"
#include "gestures/include/memory_manager.h"
#include "gestures/include/list.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/set.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_TREND_CLASSIFYING_FILTER_INTERPRETER_H_
#define GESTURES_TREND_CLASSIFYING_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter checks whether there is a statistically significant
// evidence showing that a finger is non-stationary (i.e. moving along a
// direction). We utilizes a variation of the Kendall's Tau test to
// achieve the purpose. For the interested, please may refer to the following
// links for more details:
//
// [1] http://en.wikipedia.org/wiki/Kendall_tau_rank_correlation_coefficient
// [2] http://www.stats.uwo.ca/faculty/aim/vita/pdf/McLeod95.pdf
//
// The Kendall's Tau test is a method to measure the degree of association
// between two random variables. Specifically, it tries to assess if one's
// value has a tendency to change when the other one changes. Given the
// trajectory on the (X, Y) plane of a finger over some period of time, one may
// view the problem of deciding stationary fingers as to assess whether there
// is any association between either X and time or Y and time. The advantage of
// Kendall's test is that it is non-parametric, i.e. it uses the ranking of
// data instead of the data values themselves. In this way, it enjoys better
// robustness to outliers, random noises as well as non-linear relationships.
//
// Given a time-series (t1, d1), (t2, d2) .... (tn, dn), the Kendall's Tau
// coefficient is defined as:
//
//         (# of concordant pairs) - (# of discordant pairs)
// Tau = -----------------------------------------------------              (1)
//                       n * (n - 1) / 2
//
// , where n is the total number of samples and concordant/discordant pairs are
// [(ti, di), (tj, dj)] whose ti < tj and di < dj or di > dj. The above
// definition is only valid when there is no ties in the sample. In the case
// that ties are present, as in our case, various adjustments need to be made.
//
// The denominator part in the formula (1) is sometimes called the Kendall's
// score or simply the Kendall's S-statistic. Study has shown that its
// distribution approximately follows the normal distribution when n is large
// (e.g. > 6) no matter whether there are ties or not. The detailed proof could
// be found in [2]. Here, we simply give the expression for the variance of S in
// the case that there may only be ties in one variable (our application) as
// follows:
//
//          n * (n - 1) * (2n + 5)       2
// Var(S) = ---------------------- - Σ (--- * C(ui, 3) + C(ui, 2))          (2)
//                   18              i   3
//
// , where ui is the number of samples in the i-th ties group and C(x, y) is
// the binomial coefficient notation.
//
// For each finger, we maintain a buffer of past coordinates and compare the
// normalized Z-value S/sqrt(Var(S)) to a pre-set threshold Zα as in standard
// statistical hypothesis tests. If the Z-value is outside of the region
// [-Zα, Zα], we can conclude that the instance is too rare to happen simply by
// chance and we reject the null hypothesis that there is no association
// between two random variables. We use a two-tailed test here because the
// finger could move in either the positive or the negative direction. The
// interpreter then marks each identified finger with the flags
// GESTURES_FINGER_TREND_*_X or GESTURES_FINGER_TREND_*_Y respectively which
// would be further used in the ImmediateInterpreter to identify resting
// thumbs.

class TrendClassifyingFilterInterpreter: public FilterInterpreter {

public:
  TrendClassifyingFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                                    Tracer* tracer);
  virtual ~TrendClassifyingFilterInterpreter() {}

protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

private:
  struct KState {
    KState() { Init(); }
    KState(const FingerState& fs) { Init(fs); }

    // Init functions called by ctors. We don't use constructors directly since
    // we have to init classes by ourselves for MemoryManaged types.
    void Init();
    void Init(const FingerState& fs);

    // Element struct for tracking one finger property (e.g. x, y, pressure).
    struct KAxis {
      KAxis(): val(0.0), sum(0), ties(0), score(0), var(0.0) {}

      void Init() {
        val = 0.0;
        sum = 0, ties = 0;
        score = 0;
        var = 0.0;
      }

      // The data value to track of the finger at a given timestamp
      float val;

      // Temp values to speed up the S and Var(S) computation. For more detail,
      // please look at the comment for UpdateKTValuePair.
      int sum, ties;

      // The S and Var(S) computed for this timestamp
      int score;
      double var;
    };
    static const size_t n_axes_ = 6;
    KAxis axes_[n_axes_];

    KAxis* XAxis() { return &axes_[0]; }
    KAxis* DxAxis() { return &axes_[1]; }
    KAxis* YAxis() { return &axes_[2]; }
    KAxis* DyAxis() { return &axes_[3]; }
    KAxis* PressureAxis() { return &axes_[4]; }
    KAxis* TouchMajorAxis() { return &axes_[5]; }

    static bool IsDelta(int idx) { return (idx == 1 || idx == 3); }
    static unsigned IncFlag(int idx) {
      static const unsigned flags[n_axes_] = {
          GESTURES_FINGER_TREND_INC_X,
          GESTURES_FINGER_TREND_INC_X,
          GESTURES_FINGER_TREND_INC_Y,
          GESTURES_FINGER_TREND_INC_Y,
          GESTURES_FINGER_TREND_INC_PRESSURE,
          GESTURES_FINGER_TREND_INC_TOUCH_MAJOR };
      return flags[idx];
    }
    static unsigned DecFlag(int idx) {
      static const unsigned flags[n_axes_] = {
          GESTURES_FINGER_TREND_DEC_X,
          GESTURES_FINGER_TREND_DEC_X,
          GESTURES_FINGER_TREND_DEC_Y,
          GESTURES_FINGER_TREND_DEC_Y,
          GESTURES_FINGER_TREND_DEC_PRESSURE,
          GESTURES_FINGER_TREND_DEC_TOUCH_MAJOR };
      return flags[idx];
    }

    KState* next_;
    KState* prev_;
  };

  typedef MemoryManagedList<KState> FingerHistory;

  // Trend types for internal use
  enum TrendType {
    TREND_NONE,
    TREND_INCREASING,
    TREND_DECREASING
  };

  // Detect moving fingers and append the GESTURES_FINGER_TREND_* flags
  void UpdateFingerState(const HardwareState& hwstate);

  // Push new finger data into the buffer and update values
  void AddNewStateToBuffer(FingerHistory* history, const FingerState& fs);

  // Assess statistical significance with a classic two-tail hypothesis test
  TrendType RunKTTest(const KState::KAxis* current, const size_t n_samples);

  // Given a time-series (t1, d1), (t2, d2) .... (tn, dn), a naive
  // implementation to compute the Kendall's S-statistic as in (1) would take
  // O(n^2) time, which might be too much even for a moderate size of n. To
  // speed it up, we save temp values sum_i and ties_i for each item and
  // incrementally update them upon each new data point's arrival. For each
  // item (ti, di), sum_i and ties_i stand for:
  //
  // sum_i = (# of concordant pairs w.r.t. (ti, di)) -
  //      (# of discordant pairs w.r.t. (ti, di))
  // ties_i = (# of other equal value items where dj == di)
  //
  // within the range (ti, di) .... (tn, dn).
  //
  // The S-statistic, C(ui, 3) and C(ui, 2) in (2) can then be computed as
  //
  // S = Σsum_i , C(ui, 2) =      Σ ties_j
  //     i                 j∈i-th ties group
  //
  // C(ui, 3) =      Σ (ties_j * (ties_j - 1) / 2)
  //          j∈i-th ties group
  //
  // The sum_i and ties_i can be updated in O(1) time for each item, thus
  // reducing the total time complexity to compute S and Var(S) from O(n^2) to
  // O(n).
  inline void UpdateKTValuePair(KState::KAxis* past, KState::KAxis* current,
      int* t_n2_sum, int* t_n3_sum) {
    if (past->val < current->val)
      past->sum++;
    else if (past->val > current->val)
      past->sum--;
    else
      past->ties++;
    current->score += past->sum, (*t_n2_sum) += past->ties;
    (*t_n3_sum) += ((past->ties * (past->ties - 1)) >> 1);
  }

  // Compute the variance of the Kendall's S-statistic according to (2)
  double ComputeKTVariance(const int tie_n2, const int tie_n3,
      const size_t n_samples);

  // Append flag based on the test result
  void InterpretTestResult(const TrendType trend_type,
                           const unsigned flag_increasing,
                           const unsigned flag_decreasing,
                           unsigned* flags);

  // memory managers to prevent malloc during interrupt calls
  MemoryManager<KState> kstate_mm_;
  MemoryManager<FingerHistory> history_mm_;

  // A map to store each finger's past coordinates and calculation
  // intermediates
  typedef map<short, FingerHistory*, kMaxFingers> FingerHistoryMap;
  FingerHistoryMap histories_;

  // Flag to turn on/off the trend classifying filter
  BoolProperty trend_classifying_filter_enable_;

  // Flag to turn on/off the support for second order movements. Our test can
  // capture any kind of monotonically increasing/decreasing trends, but it
  // may fail on functions that contain extrema (e.g. a parabolic curve where
  // the finger first moves toward the -X direction and then goes toward the
  // +X). We can run the same test on the 1-st order differences of data to
  // handle second order functions. Higher order functions are not considered
  // here.
  BoolProperty second_order_enable_;

  // Minimum number of samples required (must be >= 6 for a
  // meaningful result)
  IntProperty min_num_of_samples_;

  // Number of samples desired
  IntProperty num_of_samples_;

  // The critical z-value for the hypothesis testing. For a test statistic that
  // follows the normal distribution, we can compute the probability as the
  // area under the probability density function. A two-sided test may be
  // illustrated as follows:
  //
  //                     ..|..
  //                   ..xx|xx..    Two-tailed test
  //                  |xxxx|xxxx|
  //                 .|xxxx|xxxx|.
  //               .. |xxxx|xxxx| ..
  //           ....   |xxxx|xxxx|   ....
  //  -------------------------------------------
  //  -∞             -Zα   0   +Zα             +∞
  //
  // Given the desired false positive rate (α), the threshold Zα is computed
  // so that the area in [-Zα, Zα] is 1-α and the areas in [-∞, -Zα] and
  // [Zα, +∞] are α/2 respectively. If the z-value of test statistic locates
  // outside of the region [-Zα, Zα], we may conclude that the instance is rare
  // enough to demonstrate statistical significance.
  //
  // We provide a few sample Zα values for different α's here for reference.
  // A custom Zα value can be readily computed by most math packages (e.g.
  // the Python scipy). To compute Zα for a specific α (assume scipy was
  // installed):
  //
  // $ python
  // >>> from scipy.stats import norm
  // >>> norm.interval(1 - 0.01) # This computes the interval for α = 0.01
  // (-2.5758293035489004, 2.5758293035489004)
  // >>> norm.interval(1 - 0.05) # This computes the interval for α = 0.05
  // (-1.959963984540054, 1.959963984540054)
  //
  // The default value is chosen to correspond to the case where α = 0.01.
  //
  //  α      |   two-sided test z-value
  // ----------------------------------
  // 0.001   |   3.2905267314919255
  // 0.005   |   2.8070337683438109
  // 0.01    |   2.5758293035489004
  // 0.02    |   2.3263478740408408
  // 0.05    |   1.959963984540054
  // 0.10    |   1.6448536269514722
  // 0.20    |   1.2815515655446004
  DoubleProperty z_threshold_;
};

}

#endif
