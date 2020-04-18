/*
 * Copyright (C) 2006 The Android Open Source Project
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

#ifndef SKIA_CONFIG_SKUSERCONFIG_H_
#define SKIA_CONFIG_SKUSERCONFIG_H_

/*  SkTypes.h, the root of the public header files, does the following trick:

    #include <SkPreConfig.h>
    #include <SkUserConfig.h>
    #include <SkPostConfig.h>

    SkPreConfig.h runs first, and it is responsible for initializing certain
    skia defines.

    SkPostConfig.h runs last, and its job is to just check that the final
    defines are consistent (i.e. that we don't have mutually conflicting
    defines).

    SkUserConfig.h (this file) runs in the middle. It gets to change or augment
    the list of flags initially set in preconfig, and then postconfig checks
    that everything still makes sense.

    Below are optional defines that add, subtract, or change default behavior
    in Skia. Your port can locally edit this file to enable/disable flags as
    you choose, or these can be delared on your command line (i.e. -Dfoo).

    By default, this include file will always default to having all of the flags
    commented out, so including it will have no effect.
*/

///////////////////////////////////////////////////////////////////////////////

/*  Skia has lots of debug-only code. Often this is just null checks or other
    parameter checking, but sometimes it can be quite intrusive (e.g. check that
    each 32bit pixel is in premultiplied form). This code can be very useful
    during development, but will slow things down in a shipping product.

    By default, these mutually exclusive flags are defined in SkPreConfig.h,
    based on the presence or absence of NDEBUG, but that decision can be changed
    here.
 */
//#define SK_DEBUG
//#define SK_RELEASE

#ifdef DCHECK_ALWAYS_ON
    #undef SK_RELEASE
    #define SK_DEBUG
#endif

/*  If, in debugging mode, Skia needs to stop (presumably to invoke a debugger)
    it will call SK_CRASH(). If this is not defined it, it is defined in
    SkPostConfig.h to write to an illegal address
 */
//#define SK_CRASH() *(int *)(uintptr_t)0 = 0


/*  preconfig will have attempted to determine the endianness of the system,
    but you can change these mutually exclusive flags here.
 */
//#define SK_CPU_BENDIAN
//#define SK_CPU_LENDIAN

/*  Define this to provide font subsetter for font subsetting when generating
    PDF documents.
 */
#define SK_PDF_USE_SFNTLY

/*  To write debug messages to a console, skia will call SkDebugf(...) following
    printf conventions (e.g. const char* format, ...). If you want to redirect
    this to something other than printf, define yours here
 */
//#define SkDebugf(...)  MyFunction(__VA_ARGS__)


/*  If SK_DEBUG is defined, then you can optionally define SK_SUPPORT_UNITTEST
    which will run additional self-tests at startup. These can take a long time,
    so this flag is optional.
 */
#ifdef SK_DEBUG
#define SK_SUPPORT_UNITTEST
#endif

/* If your system embeds skia and has complex event logging, define this
   symbol to name a file that maps the following macros to your system's
   equivalents:
       SK_TRACE_EVENT0(event)
       SK_TRACE_EVENT1(event, name1, value1)
       SK_TRACE_EVENT2(event, name1, value1, name2, value2)
   src/utils/SkDebugTrace.h has a trivial implementation that writes to
   the debug output stream. If SK_USER_TRACE_INCLUDE_FILE is not defined,
   SkTrace.h will define the above three macros to do nothing.
*/
#undef SK_USER_TRACE_INCLUDE_FILE

// ===== Begin Chrome-specific definitions =====

#ifdef SK_DEBUG
#define SK_REF_CNT_MIXIN_INCLUDE "sk_ref_cnt_ext_debug.h"
#else
#define SK_REF_CNT_MIXIN_INCLUDE "sk_ref_cnt_ext_release.h"
#endif

#define SK_MSCALAR_IS_FLOAT
#undef SK_MSCALAR_IS_DOUBLE

#define GR_MAX_OFFSCREEN_AA_DIM     512

// Log the file and line number for assertions.
#define SkDebugf(...) SkDebugf_FileLine(__FILE__, __LINE__, false, __VA_ARGS__)
SK_API void SkDebugf_FileLine(const char* file, int line, bool fatal,
                              const char* format, ...);

// Marking the debug print as "fatal" will cause a debug break, so we don't need
// a separate crash call here.
#define SK_DEBUGBREAK(cond) do { if (!(cond)) { \
    SkDebugf_FileLine(__FILE__, __LINE__, true, \
    "%s:%d: failed assertion \"%s\"\n", \
    __FILE__, __LINE__, #cond); } } while (false)

#if !defined(ANDROID)   // On Android, we use the skia default settings.
#define SK_A32_SHIFT    24
#define SK_R32_SHIFT    16
#define SK_G32_SHIFT    8
#define SK_B32_SHIFT    0
#endif

#if defined(SK_BUILD_FOR_MAC)

#define SK_CPU_LENDIAN
#undef  SK_CPU_BENDIAN

#elif defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_ANDROID)

// Prefer FreeType's emboldening algorithm to Skia's
// TODO: skia used to just use hairline, but has improved since then, so
// we should revisit this choice...
#define SK_USE_FREETYPE_EMBOLDEN

#if defined(SK_BUILD_FOR_UNIX) && defined(SK_CPU_BENDIAN)
// Above we set the order for ARGB channels in registers. I suspect that, on
// big endian machines, you can keep this the same and everything will work.
// The in-memory order will be different, of course, but as long as everything
// is reading memory as words rather than bytes, it will all work. However, if
// you find that colours are messed up I thought that I would leave a helpful
// locator for you. Also see the comments in
// base/gfx/bitmap_platform_device_linux.h
#error Read the comment at this location
#endif

#endif

// The default crash macro writes to badbeef which can cause some strange
// problems. Instead, pipe this through to the logging function as a fatal
// assertion.
#define SK_CRASH() SkDebugf_FileLine(__FILE__, __LINE__, true, "SK_CRASH")

// These flags are no longer defined in Skia, but we have them (temporarily)
// until we update our call-sites (typically these are for API changes).
//
// Remove these as we update our sites.
//

// Workaround for poor anisotropic mipmap quality,
// pending Skia ripmap support.
// (https://bugs.chromium.org/p/skia/issues/detail?id=4863)
#ifndef    SK_SUPPORT_LEGACY_ANISOTROPIC_MIPMAP_SCALE
#   define SK_SUPPORT_LEGACY_ANISOTROPIC_MIPMAP_SCALE
#endif

// Remove this after we fixed all the issues related to the new SDF algorithm
// (https://codereview.chromium.org/1643143002)
#ifndef SK_USE_LEGACY_DISTANCE_FIELDS
#define SK_USE_LEGACY_DISTANCE_FIELDS
#endif

// Skia is enabling this feature soon. Chrome probably does
// not want it for M64
#ifndef SK_DISABLE_EXPLICIT_GPU_RESOURCE_ALLOCATION
#define SK_DISABLE_EXPLICIT_GPU_RESOURCE_ALLOCATION
#endif

// Skia is enabling this feature soon. Chrome probably does
// not want it for M64
#ifndef SK_DISABLE_RENDER_TARGET_SORTING
#define SK_DISABLE_RENDER_TARGET_SORTING
#endif

#ifndef SK_SUPPORT_LEGACY_TILED_BITMAPS
#define SK_SUPPORT_LEGACY_TILED_BITMAPS
#endif

// The matrix image filter imperceptibly alters the following two layout tests:
//   fast/css/transformed-mask.html
//   fast/reflections/opacity-reflection-transform.html
// and changes the following cc_unittests:
//   LayerTreeHostCommonTest.VisibleRectWithScalingClippingAndFilters
//   LayerTreeHostCommonTest.VisibleRectWithClippingAndFilters
// Landing the fix in Skia behind this flag will allow those all to be updated
// together in Chrome (along with the removal of this flag).
#ifndef SK_IGNORE_MATRIX_IMAGE_FILTER_FIX
#define SK_IGNORE_MATRIX_IMAGE_FILTER_FIX
#endif

// remove after rebaselining svg layout tests
#ifndef SK_SUPPORT_LEGACY_SVG_ARC_TO
#define SK_SUPPORT_LEGACY_SVG_ARC_TO
#endif

#ifndef SK_SUPPORT_LEGACY_YUV_COLORSPACE
#define SK_SUPPORT_LEGACY_YUV_COLORSPACE
#endif

// Max. verb count for paths rendered by the edge-AA tessellating path renderer.
#define GR_AA_TESSELLATOR_MAX_VERB_COUNT 10

#define SK_DISABLE_LOWP_BILERP_CLAMP_CLAMP_STAGE


#ifndef SK_SUPPORT_LEGACY_THREADED_DAA_BUGS
#define SK_SUPPORT_LEGACY_THREADED_DAA_BUGS
#endif

#define SK_DONT_OPTIMIZE_SRC_PROFILES_FOR_SPEED
#define SK_DONT_OPTIMIZE_DST_PROFILES_FOR_SPEED

///////////////////////// Imported from BUILD.gn and skia_common.gypi

/* In some places Skia can use static initializers for global initialization,
 *  or fall back to lazy runtime initialization. Chrome always wants the latter.
 */
#define SK_ALLOW_STATIC_GLOBAL_INITIALIZERS 0

/* Restrict formats for Skia font matching to SFNT type fonts. */
#define SK_FONT_CONFIG_INTERFACE_ONLY_ALLOW_SFNT_FONTS

#define SK_IGNORE_BLURRED_RRECT_OPT
#define SK_USE_DISCARDABLE_SCALEDIMAGECACHE

#define SK_ATTR_DEPRECATED          SK_NOTHING_ARG1
#define GR_GL_CUSTOM_SETUP_HEADER   "GrGLConfig_chrome.h"

// mtklein's fiddling with Src / SrcOver.  Will rebaseline these only once when done.
#define SK_SUPPORT_LEGACY_X86_BLITS

// ===== End Chrome-specific definitions =====

#endif
