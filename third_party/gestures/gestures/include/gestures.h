// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_GESTURES_H__
#define GESTURES_GESTURES_H__

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
#include <string>

#include <memory>

extern "C" {
#endif

// C API:

// external logging interface
#define GESTURES_LOG_ERROR 0
#define GESTURES_LOG_INFO 1

// this function has to be provided by the user of the library.
void gestures_log(int verb, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

typedef double stime_t;  // seconds

enum GestureInterpreterDeviceClass {
  GESTURES_DEVCLASS_UNKNOWN,
  GESTURES_DEVCLASS_MOUSE,
  GESTURES_DEVCLASS_MULTITOUCH_MOUSE,
  GESTURES_DEVCLASS_TOUCHPAD,
  GESTURES_DEVCLASS_TOUCHSCREEN,
};

stime_t StimeFromTimeval(const struct timeval*);
stime_t StimeFromTimespec(const struct timespec*);

struct HardwareProperties {
  float left, top, right, bottom;
  float res_x;  // pixels/mm
  float res_y;  // pixels/mm
  float screen_x_dpi;  // read from X server and passed to library
  float screen_y_dpi;  // read from X server and passed to library
  float orientation_minimum;
  float orientation_maximum;
  unsigned short max_finger_cnt; // Max finger slots in one report
  unsigned short max_touch_cnt;  // Max fingers that can be detected at once
  unsigned supports_t5r2:1;
  unsigned support_semi_mt:1;
  unsigned is_button_pad:1;
  unsigned has_wheel:1;
#ifdef __cplusplus
  std::string String() const;
#endif  // __cplusplus
};

// position is the (x,y) cartesian coord of the finger on the trackpad.
// touch_major/minor are the large/small radii of the ellipse of the touching
// finger. width_major/minor are also radii, but of the finger itself,
// including a little bit that isn't touching. So, width* is generally a tad
// larger than touch*.
// tracking_id: If a finger is the same physical finger across two
// consecutive frames, it must have the same tracking id; if it's a different
// finger, it may (should) have a different tracking id.

// Warp: If a finger has the 'warp' flag set for an axis, it means that while
// the finger may have moved, it should not cause any motion in that direction.
// This may occur is some situations where we thought a finger was in one place,
// but then we realized later it was actually in another place.
// The *_WARP_X/Y_MOVE version is an indication for suppressing unwanted
// cursor movement, while the *_WARP_X/Y_NON_MOVE version is for unwanted
// non-cursor movement (e.g. scrolling)
#define GESTURES_FINGER_WARP_X_NON_MOVE        (1 << 0)
#define GESTURES_FINGER_WARP_Y_NON_MOVE        (1 << 1)
// If a finger has notap set, it shouldn't begin a tap gesture.
#define GESTURES_FINGER_NO_TAP        (1 << 2)
#define GESTURES_FINGER_POSSIBLE_PALM (1 << 3)
#define GESTURES_FINGER_PALM          (1 << 4)
#define GESTURES_FINGER_WARP_X_MOVE   (1 << 5)
#define GESTURES_FINGER_WARP_Y_MOVE   (1 << 6)
// If tap to click movement detection should warp:
#define GESTURES_FINGER_WARP_X_TAP_MOVE   (1 << 7)
#define GESTURES_FINGER_WARP_Y_TAP_MOVE   (1 << 8)
// If a finger is a merged finger or one of close fingers
#define GESTURES_FINGER_MERGE   (1 << 9)
// If a finger is showing a trend of moving (see the TrendClassifyingFilter).
#define GESTURES_FINGER_TREND_INC_X (1 << 10)
#define GESTURES_FINGER_TREND_DEC_X (1 << 11)
#define GESTURES_FINGER_TREND_INC_Y (1 << 12)
#define GESTURES_FINGER_TREND_DEC_Y (1 << 13)
#define GESTURES_FINGER_TREND_INC_PRESSURE (1 << 14)
#define GESTURES_FINGER_TREND_DEC_PRESSURE (1 << 15)
#define GESTURES_FINGER_TREND_INC_TOUCH_MAJOR (1 << 16)
#define GESTURES_FINGER_TREND_DEC_TOUCH_MAJOR (1 << 17)
// If a finger is non-stationary recently (see the StationaryWiggleFilter).
// Compared to the trend flags, this one takes the magnitude of movements
// into account so it might be more useful in some cases. However, it is also
// more prone to abrupt noisy jumps than the trend flags. It also looks at
// a shorter period of time than the trend ones so it may provide faster
// response and lower latency.
#define GESTURES_FINGER_INSTANTANEOUS_MOVING (1 << 18)
// We sometimes use the warp flags only because we want to suppress unwanted
// movements and not that we really have no good idea of the finger position.
// This poses additional difficulty for some classifying logics that relies
// much on the finger position. To maximize the use of any available data,
// we further mark a finger as GESTURES_FINGER_WARP_TELEPORTATION only if we
// indeed have no idea of its position (e.g. due to sensor jumps). For all
// other cases (e.g. click wiggle, plams suppression, stationary wiggle), we
// skip the flag so that we can have the option to use the not-that-accurate
// positions.
#define GESTURES_FINGER_WARP_TELEPORTATION (1 << 19)

#define GESTURES_FINGER_WARP_X    (GESTURES_FINGER_WARP_X_NON_MOVE | \
                                   GESTURES_FINGER_WARP_X_MOVE)
#define GESTURES_FINGER_WARP_Y    (GESTURES_FINGER_WARP_Y_NON_MOVE | \
                                   GESTURES_FINGER_WARP_Y_MOVE)

struct FingerState {
  float touch_major, touch_minor;
  float width_major, width_minor;
  float pressure;
  float orientation;
  float position_x;
  float position_y;
  short tracking_id;
  unsigned flags;
#ifdef __cplusplus
  bool NonFlagsEquals(const FingerState& that) const {
    return touch_major == that.touch_major &&
        touch_minor == that.touch_minor &&
        width_major == that.width_major &&
        width_minor == that.width_minor &&
        pressure == that.pressure &&
        orientation == that.orientation &&
        position_x == that.position_x &&
        position_y == that.position_y &&
        tracking_id == that.tracking_id;
  }
  bool operator==(const FingerState& that) const {
    return NonFlagsEquals(that) && flags == that.flags;
  }
  bool operator!=(const FingerState& that) const { return !(*this == that); }
  static std::string FlagsString(unsigned flags);
  std::string String() const;
#endif
};

#define GESTURES_BUTTON_NONE 0
#define GESTURES_BUTTON_LEFT 1
#define GESTURES_BUTTON_MIDDLE 2
#define GESTURES_BUTTON_RIGHT 4
#define GESTURES_BUTTON_BACK 8
#define GESTURES_BUTTON_FORWARD 16

// One frame of trackpad data
struct HardwareState {
#ifdef __cplusplus
  FingerState* GetFingerState(short tracking_id);
  const FingerState* GetFingerState(short tracking_id) const;
  bool SameFingersAs(const HardwareState& that) const;
  std::string String() const;
  void DeepCopy(const HardwareState& that, unsigned short max_finger_cnt);
#endif  // __cplusplus
  stime_t timestamp;  // 64-bit Wall clock time in microseconds (10^-6 s)
  int buttons_down;  // bit field, use GESTURES_BUTTON_*
  unsigned short finger_cnt;  // Number of valid finger slots
  unsigned short touch_cnt;  // Number of fingers touching pad
  struct FingerState* fingers;
  // For EV_REL events
  float rel_x;
  float rel_y;
  float rel_wheel;
  float rel_hwheel;
};

#define GESTURES_FLING_START 0  // Scroll end/fling begin
#define GESTURES_FLING_TAP_DOWN 1  // Finger touched down/fling end

#define GESTURES_ZOOM_START 0  // Pinch zoom begin
#define GESTURES_ZOOM_UPDATE 1  // Zoom-in/Zoom-out update
#define GESTURES_ZOOM_END 2  // Pinch zoom end

// Gesture sub-structs

// Note about ordinal_* values: Sometimes, UI will want to use unaccelerated
// values for various gestures, so we expose the non-accelerated values in
// the ordinal_* fields.

typedef struct {
  float dx, dy;
  float ordinal_dx, ordinal_dy;
} GestureMove;

typedef struct{
  float dx, dy;
  float ordinal_dx, ordinal_dy;
  // If set, stop_fling means that this scroll should stop flinging, thus
  // if an interpreter suppresses it for any reason (e.g., rounds the size
  // down to 0, thus making it a noop), it will replace it with a Fling
  // TAP_DOWN gesture
  unsigned stop_fling:1;
} GestureScroll;

typedef struct {
  // If a bit is set in both down and up, client should process down first
  unsigned down;  // bit field, use GESTURES_BUTTON_*
  unsigned up;  // bit field, use GESTURES_BUTTON_*
} GestureButtonsChange;

typedef struct {
  // fling velocity (valid when fling_state is GESTURES_FLING_START):
  float vx, vy;
  float ordinal_vx, ordinal_vy;
  unsigned fling_state:1;  // GESTURES_FLING_START or GESTURES_FLING_TAP_DOWN
} GestureFling;

typedef struct {
  float dx, dy;
  float ordinal_dx, ordinal_dy;
} GestureSwipe;

typedef struct {
  float dx, dy;
  float ordinal_dx, ordinal_dy;
} GestureFourFingerSwipe;

typedef struct {
  char __dummy;
  // Remove this when there is a member in this struct. http://crbug.com/341155
  // Currently no members
} GestureFourFingerSwipeLift;

typedef struct {
  char __dummy;
  // Remove this when there is a member in this struct. http://crbug.com/341155
  // Currently no members
} GestureSwipeLift;

typedef struct {
  // Relative pinch factor starting with 1.0 = no pinch
  // <1.0 for outwards pinch
  // >1.0 for inwards pinch
  float dz;
  float ordinal_dz;
  // GESTURES_ZOOM_START, GESTURES_ZOOM_UPDATE, or GESTURES_ZOOM_END
  unsigned zoom_state;
} GesturePinch;

// Metrics types that we care about
enum GestureMetricsType {
  kGestureMetricsTypeNoisyGround = 0,
  kGestureMetricsTypeMouseMovement,
  kGestureMetricsTypeUnknown,
};

typedef struct {
  enum GestureMetricsType type;
  // Optional values for the metrics. 2 are more than enough for now.
  float data[2];
} GestureMetrics;

enum GestureType {
#ifdef GESTURES_INTERNAL
  kGestureTypeNull = -1,  // internal to Gestures library only
#endif  // GESTURES_INTERNAL
  kGestureTypeContactInitiated = 0,
  kGestureTypeMove,
  kGestureTypeScroll,
  kGestureTypeButtonsChange,
  kGestureTypeFling,
  kGestureTypeSwipe,
  kGestureTypePinch,
  kGestureTypeSwipeLift,
  kGestureTypeMetrics,
  kGestureTypeFourFingerSwipe,
  kGestureTypeFourFingerSwipeLift,
};

#ifdef __cplusplus
// Pass these into Gesture() ctor as first arg
extern const GestureMove kGestureMove;
extern const GestureScroll kGestureScroll;
extern const GestureButtonsChange kGestureButtonsChange;
extern const GestureFling kGestureFling;
extern const GestureSwipe kGestureSwipe;
extern const GestureFourFingerSwipe kGestureFourFingerSwipe;
extern const GesturePinch kGesturePinch;
extern const GestureSwipeLift kGestureSwipeLift;
extern const GestureFourFingerSwipeLift kGestureFourFingerSwipeLift;
extern const GestureMetrics kGestureMetrics;
#endif  // __cplusplus

struct Gesture {
#ifdef __cplusplus
  // Create Move/Scroll gesture
#ifdef GESTURES_INTERNAL
  Gesture() : start_time(0), end_time(0), type(kGestureTypeNull) {}
  std::string String() const;
  bool operator==(const Gesture& that) const;
  bool operator!=(const Gesture& that) const { return !(*this == that); };
#endif  // GESTURES_INTERNAL
  Gesture(const GestureMove&, stime_t start, stime_t end, float dx, float dy)
      : start_time(start), end_time(end), type(kGestureTypeMove) {
    details.move.ordinal_dx = details.move.dx = dx;
    details.move.ordinal_dy = details.move.dy = dy;
  }
  Gesture(const GestureScroll&,
          stime_t start, stime_t end, float dx, float dy)
      : start_time(start), end_time(end), type(kGestureTypeScroll) {
    details.scroll.ordinal_dx = details.scroll.dx = dx;
    details.scroll.ordinal_dy = details.scroll.dy = dy;
    details.scroll.stop_fling = 0;
  }
  Gesture(const GestureButtonsChange&,
          stime_t start, stime_t end, unsigned down, unsigned up)
      : start_time(start),
        end_time(end),
        type(kGestureTypeButtonsChange) {
    details.buttons.down = down;
    details.buttons.up = up;
  }
  Gesture(const GestureFling&,
          stime_t start, stime_t end, float vx, float vy, unsigned state)
      : start_time(start), end_time(end), type(kGestureTypeFling) {
    details.fling.ordinal_vx = details.fling.vx = vx;
    details.fling.ordinal_vy = details.fling.vy = vy;
    details.fling.fling_state = state;
  }
  Gesture(const GestureSwipe&,
          stime_t start, stime_t end, float dx, float dy)
      : start_time(start),
        end_time(end),
        type(kGestureTypeSwipe) {
    details.swipe.ordinal_dx = details.swipe.dx = dx;
    details.swipe.ordinal_dy = details.swipe.dy = dy;
  }
  Gesture(const GestureFourFingerSwipe&,
          stime_t start, stime_t end, float dx, float dy)
      : start_time(start),
        end_time(end),
        type(kGestureTypeFourFingerSwipe) {
    details.four_finger_swipe.ordinal_dx = details.four_finger_swipe.dx = dx;
    details.four_finger_swipe.ordinal_dy = details.four_finger_swipe.dy = dy;
  }
  Gesture(const GesturePinch&,
          stime_t start, stime_t end, float dz, unsigned state)
      : start_time(start),
        end_time(end),
        type(kGestureTypePinch) {
    details.pinch.ordinal_dz = details.pinch.dz = dz;
    details.pinch.zoom_state = state;
  }
  Gesture(const GestureSwipeLift&, stime_t start, stime_t end)
      : start_time(start),
        end_time(end),
        type(kGestureTypeSwipeLift) {}
  Gesture(const GestureFourFingerSwipeLift&, stime_t start, stime_t end)
      : start_time(start),
        end_time(end),
        type(kGestureTypeFourFingerSwipeLift) {}
  Gesture(const GestureMetrics&,
          stime_t start, stime_t end, GestureMetricsType m_type,
          float d1, float d2)
      : start_time(start),
        end_time(end),
        type(kGestureTypeMetrics) {
    details.metrics.type = m_type;
    details.metrics.data[0] = d1;
    details.metrics.data[1] = d2;
  }
#endif  // __cplusplus

  stime_t start_time, end_time;
  enum GestureType type;
  union {
    GestureMove move;
    GestureScroll scroll;
    GestureButtonsChange buttons;
    GestureFling fling;
    GestureSwipe swipe;
    GesturePinch pinch;
    GestureSwipeLift swipe_lift;
    GestureMetrics metrics;
    GestureFourFingerSwipe four_finger_swipe;
    GestureFourFingerSwipeLift four_finger_swipe_lift;
  } details;
};

typedef void (*GestureReadyFunction)(void* client_data,
                                     const struct Gesture* gesture);

// Gestures Timer Provider Interface
struct GesturesTimer;
typedef struct GesturesTimer GesturesTimer;

// If this returns < 0, the timer should be freed. If it returns >= 0.0, it
// should be called again after that amount of delay.
typedef stime_t (*GesturesTimerCallback)(stime_t now,
                                         void* callback_data);
// Allocate and return a new timer, or NULL if error.
typedef GesturesTimer* (*GesturesTimerCreate)(void* data);
// Set a timer:
typedef void (*GesturesTimerSet)(void* data,
                                 GesturesTimer* timer,
                                 stime_t delay,
                                 GesturesTimerCallback callback,
                                 void* callback_data);
// Cancel a set timer:
typedef void (*GesturesTimerCancel)(void* data, GesturesTimer* timer);
// Free the timer. Will not be called from within a timer callback.
typedef void (*GesturesTimerFree)(void* data, GesturesTimer* timer);

typedef struct {
  GesturesTimerCreate create_fn;
  GesturesTimerSet set_fn;
  GesturesTimerCancel cancel_fn;
  GesturesTimerFree free_fn;
} GesturesTimerProvider;

// Gestures Property Provider Interface
struct GesturesProp;
typedef struct GesturesProp GesturesProp;

typedef unsigned char GesturesPropBool;

// These functions create a named property of given type.
//   data - data used by PropProvider
//   loc - location of a variable to be updated by PropProvider.
//         Set to NULL to create a ReadOnly property
//   init - initial value for the property.
//          If the PropProvider has an alternate configuration source, it may
//          override this initial value, in which case *loc returns the
//          value from the configuration source.
typedef GesturesProp* (*GesturesPropCreateInt)(void* data, const char* name,
                                               int* loc, size_t count,
                                               const int* init);

typedef GesturesProp* (*GesturesPropCreateShort)(void* data, const char* name,
                                                 short* loc, size_t count,
                                                 const short* init);

typedef GesturesProp* (*GesturesPropCreateBool)(void* data, const char* name,
                                                GesturesPropBool* loc,
                                                size_t count,
                                                const GesturesPropBool* init);

typedef GesturesProp* (*GesturesPropCreateString)(void* data, const char* name,
                                                  const char** loc,
                                                  const char* const init);

typedef GesturesProp* (*GesturesPropCreateReal)(void* data, const char* name,
                                                double* loc, size_t count,
                                                const double* init);

// A function to call just before a property is to be read.
// |handler_data| is a local context pointer that can be used by the handler.
// Handler should return non-zero if it modifies the property's value.
typedef GesturesPropBool (*GesturesPropGetHandler)(void* handler_data);

// A function to call just after a property's value is updated.
// |handler_data| is a local context pointer that can be used by the handler.
typedef void (*GesturesPropSetHandler)(void* handler_data);

// Register handlers to be called when a GesturesProp is accessed.
// The get handler, if not NULL, is called immediately before the property's
// value is to be read.  This gives the library a chance to update its value.
// The set handler, if not NULL, is called immediately after the property's
// value is updated.  This can be used to create a property that is used to
// trigger an action, or to force an update to multiple properties atomically.
// Note: the handlers are called from non-signal/interrupt context
typedef void (*GesturesPropRegisterHandlers)(void* data, GesturesProp* prop,
                                             void* handler_data,
                                             GesturesPropGetHandler getter,
                                             GesturesPropSetHandler setter);

// Free a property.
typedef void (*GesturesPropFree)(void* data, GesturesProp* prop);

typedef struct GesturesPropProvider {
  GesturesPropCreateInt create_int_fn;
  GesturesPropCreateShort create_short_fn;
  GesturesPropCreateBool create_bool_fn;
  GesturesPropCreateString create_string_fn;
  GesturesPropCreateReal create_real_fn;
  GesturesPropRegisterHandlers register_handlers_fn;
  GesturesPropFree free_fn;
} GesturesPropProvider;

#ifdef __cplusplus
// C++ API:

namespace gestures {

class Interpreter;
class PropRegistry;
class LoggingFilterInterpreter;
class Tracer;
class GestureInterpreterConsumer;
class MetricsProperties;

#if __cplusplus >= 201103L

struct GestureInterpreter {
 public:
  explicit GestureInterpreter(int version);
  ~GestureInterpreter();
  void PushHardwareState(HardwareState* hwstate);

  void SetHardwareProperties(const HardwareProperties& hwprops);

  void TimerCallback(stime_t now, stime_t* timeout);

  void set_callback(GestureReadyFunction callback,
                    void* client_data);
  void SetTimerProvider(GesturesTimerProvider* tp, void* data);
  void SetPropProvider(GesturesPropProvider* pp, void* data);

  // Initialize GestureInterpreter based on device configuration.  This must be
  // called after GesturesPropProvider is set and before it accepts any inputs.
  void Initialize(
      GestureInterpreterDeviceClass type=GESTURES_DEVCLASS_TOUCHPAD);

  Interpreter* interpreter() const { return interpreter_.get(); }
  PropRegistry* prop_reg() const { return prop_reg_.get(); }

  std::string EncodeActivityLog();
 private:
  void InitializeTouchpad(void);
  void InitializeTouchpad2(void);
  void InitializeMouse(void);
  void InitializeMultitouchMouse(void);

  GestureReadyFunction callback_;
  void* callback_data_;

  std::unique_ptr<PropRegistry> prop_reg_;
  std::unique_ptr<Tracer> tracer_;
  std::unique_ptr<Interpreter> interpreter_;
  std::unique_ptr<MetricsProperties> mprops_;

  GesturesTimerProvider* timer_provider_;
  void* timer_provider_data_;
  GesturesTimer* interpret_timer_;

  LoggingFilterInterpreter* loggingFilter_;
  std::unique_ptr<GestureInterpreterConsumer> consumer_;
  HardwareProperties hwprops_;

  // Disallow copy & assign;
  GestureInterpreter(const GestureInterpreter&);
  void operator=(const GestureInterpreter&);
};

#else  // __cplusplus >= 201103L

// Must be opaque under C++03 builds, since it has unique_ptr members.
struct GestureInterpreter;

#endif  // __cplusplus >= 201103L


}  // namespace gestures

typedef gestures::GestureInterpreter GestureInterpreter;
#else
struct GestureInterpreter;
typedef struct GestureInterpreter GestureInterpreter;
#endif  // __cplusplus

#define GESTURES_VERSION 1
GestureInterpreter* NewGestureInterpreterImpl(int);
#define NewGestureInterpreter() NewGestureInterpreterImpl(GESTURES_VERSION)

void DeleteGestureInterpreter(GestureInterpreter*);

void GestureInterpreterSetHardwareProperties(GestureInterpreter*,
                                             const struct HardwareProperties*);

void GestureInterpreterPushHardwareState(GestureInterpreter*,
                                         struct HardwareState*);

void GestureInterpreterSetCallback(GestureInterpreter*,
                                   GestureReadyFunction,
                                   void*);

// Gestures will hold a reference to passed provider. Pass NULL to tell
// Gestures to stop holding a reference.
void GestureInterpreterSetTimerProvider(GestureInterpreter*,
                                        GesturesTimerProvider*,
                                        void*);

void GestureInterpreterSetPropProvider(GestureInterpreter*,
                                       GesturesPropProvider*,
                                       void*);

void GestureInterpreterInitialize(GestureInterpreter*,
                                  enum GestureInterpreterDeviceClass);

#ifdef __cplusplus
}
#endif

#endif  // GESTURES_GESTURES_H__
