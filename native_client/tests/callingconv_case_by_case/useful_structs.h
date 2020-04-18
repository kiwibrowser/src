/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_CALLINGCONV_SMALL_USEFUL_STRUCTS_H
#define NATIVE_CLIENT_TESTS_CALLINGCONV_SMALL_USEFUL_STRUCTS_H

#include "native_client/tests/callingconv_case_by_case/utils.h"
/* #include <complex.h> not in newlib... */
#include <float.h>
#include <stdbool.h>
#include <stdint.h>

/*----- Some useful struct definitions -----*/

/* When adding a new type:
 * - remember to add an entry in "for_each_type.inc"
 * - add constants below "kTYPE = ..."
 * - add a check macro "CHECK_TYPE(instance)"
 */

/*--- These should all be less than two eight bytes. ---*/
typedef struct { char x; int32_t y; } CHAR_I32;
typedef struct { int32_t x; int32_t y; } I32_I32;
typedef struct { char x; int64_t y; } CHAR_I64;
typedef struct { int64_t x; int64_t y; } I64_I64;

/* On ARM, returning a 64-bit fundamental type can be done in r0 + r1.
 * However, returning a composite type containing only a single
 * 64-bit fundamental type cannot be done that way and must be done
 * on the stack.  Composite types not larger than 4 bytes can be returned
 * in r0, however.
 * See ARM aapcs document, section 5.4.
 */
typedef struct { int64_t x; } I64_STRUCT;
typedef int64_t I64_NON_STRUCT;
typedef struct { int32_t x; } I32_STRUCT;
typedef int32_t I32_NON_STRUCT;
typedef struct { int16_t x; } I16_STRUCT;
typedef int16_t I16_NON_STRUCT;
typedef struct { double x; } DOUBLE_STRUCT;
typedef double DOUBLE_NON_STRUCT;
typedef struct { float x; } FLOAT_STRUCT;
typedef float FLOAT_NON_STRUCT;

/* Test sharing an eight-byte between an I32 and a FLOAT. */
typedef struct { int32_t x; float y; } I32_FLOAT;
/* Test NOT having both an INT and FLOAT in the same eight-byte / struct. */
typedef struct { float x; float y; } FLOAT_FLOAT;

/* Try embedding some structs (shouldn't really matter) */
typedef struct { struct { int32_t a; char b; } x;
  struct { char a; float b;} y; } STRUCT_STRUCT;

/* Struct containing an empty struct */
typedef struct {} EMPTY;
typedef struct { int* a; EMPTY x; double* b; } PTR_EMPTYSTRUCT_PTR;

/* Not sharing within an eight-byte, but mixing within two eight-bytes */
typedef struct { int64_t x; double y; } I64_DOUBLE;
/* And not-sharing even within two eight-bytes */
typedef struct { double x; double y; } DOUBLE_DOUBLE;

/* Check bool also. If there was a way to check _Bool at the same time,
 * that would be good...
 */
typedef struct { char x; bool y; int32_t z; bool w; } CHAR_BOOL_I32_BOOL;

/* This normally only uses one register, since the alignment padding does not
 * need to be serialized through registers. */
typedef struct { int32_t x __attribute__((aligned(16))); } I32_ALIGN16;
/* This looks the same in bitcode, but user_data != padding. */
typedef struct { int32_t x; char user_data[12]; } I32_CHAR12;

/* Other array tests. */
typedef struct { int32_t x[4]; } ARRAY_I32_4;
typedef struct { float x[4]; } ARRAY_FLOAT_4;


/* These ones should be classified as MEMORY since they are bigger
   than two eight-bytes. */
typedef struct { double x; double y; double z; double w;}
  DOUBLE_DOUBLE_DOUBLE;
/* This one is too big because of alignment, not content. */
typedef struct { char x; int64_t y; int32_t z;} CHAR_I64_I32;
/* This fellow is also aligned far too much. */
typedef struct { int32_t x; char y __attribute__((aligned(32))); }
  I32_CHAR_ALIGN32;

/* Bitfields are normally allowed to straddle an 8-byte boundary. */
typedef struct {
  int32_t x:31;
  int32_t y:31;
  int32_t z:31;
} __attribute__((packed)) BITFIELD_STRADDLE;

/* Non bitfields are normally NOT allowed to straddle an 8-byte boundary. */
typedef struct {
  int16_t x;
  int64_t y;
} __attribute__((packed)) NONBITFIELD_STRADDLE;

/* TODO(jvoung): add some bitfield type to test ordering of bitfields,
 * not just the straddle property. */

/*-- Test some Unions --*/

/* Order of declaration should not matter (though clang defaults to picking
 * the first widest type for its representation). These would normally be
 * considered INTEGER class.
 */
typedef union { int64_t x; double y; } U_I64_DOUBLE;
typedef union { double x; int64_t y; } U_DOUBLE_I64;
typedef union { double x; int16_t y[4]; } U_DOUBLE_ARRAY_I16_4;
typedef union { int16_t x[4]; double y; } U_ARRAY_I16_4_DOUBLE;

/* Since one of the union members straddle and is a non-bitfield, the
 * whole thing would have been treated that way.
 */
typedef union {
  BITFIELD_STRADDLE x;
  NONBITFIELD_STRADDLE y;
} U_STRADDLE_BF_NONBF;

/* Example which threw off LLVM when using first class aggregates.
 * Access to the int16_t gets truncated.
 */
typedef union {
  int16_t x;
  struct { char a; int b; } y;
} U_I16_STRUCT;

/*--- Test some Enums ---*/

typedef enum {
  E1 = 1, E2, E3, E4
} ENUM1;

typedef enum {
  E1_PACKED8 = 1, E2_PACKED8, E3_PACKED8, E4_PACKED8
} __attribute__((__packed__)) ENUM1_PACKED8;

typedef enum {
  E1_PACKED16 = 1 << 9, E2_PACKED16, E3_PACKED16, E4_PACKED16
} __attribute__((__packed__)) ENUM1_PACKED16;

/* This will likely actually be 32, since there is no 24-bit data type */
typedef enum {
  E1_PACKED24 = 1 << 17, E2_PACKED24, E3_PACKED24, E4_PACKED24
} __attribute__((__packed__)) ENUM1_PACKED24;

typedef enum {
  E1_PACKED32 = 1 << 25, E2_PACKED32, E3_PACKED32, E4_PACKED32
} __attribute__((__packed__)) ENUM1_PACKED32;

/*--- Trivial C++ classes vs Non-Trivial C++ classes ---*/

class CLASS_I32_I32 {
 public:
  int32_t x;
  int32_t y;

  /* no harm if there's a function */
  int32_t foo(void) {
    return 0;
  }

  /* or static members */
  static int32_t static_mem;
};

class CLASS_DOUBLE_DOUBLE {
 public:
  double x;
  double y;
};

class NONTRIV_CLASS_I32_I32 {
 public:
  NONTRIV_CLASS_I32_I32(int32_t x_ = 0, int32_t y_ = 0) : x(x_), y(y_) { }
  NONTRIV_CLASS_I32_I32(const NONTRIV_CLASS_I32_I32 &other) :
      x(other.x), y(other.y) {
    fprintf(stderr, "I32_I32 Copy constructor, x: %i y: %i!\n", x, y);
  }
  virtual ~NONTRIV_CLASS_I32_I32() {
    fprintf(stderr, "I32_I32 destructor!\n");
  }
  virtual void foo(void) {
    fprintf(stderr, "foo!\n");
  }
  int32_t x;
  int32_t y;
};

class NONTRIV_CLASS_BASE1 {
 public:
  NONTRIV_CLASS_BASE1(int a, int b) : a(a), b(b) {
  }
  NONTRIV_CLASS_BASE1(const NONTRIV_CLASS_BASE1 &other)
      : a(other.a), b(other.b) {
    fprintf(stderr, "CLASS_BASE1 Copy constructor!\n");
  }
  virtual int bar(void) {
    return a + b;
  }
  int a;
  int b;
};

class NONTRIV_CLASS_DOUBLE_DOUBLE {
 public:
  NONTRIV_CLASS_DOUBLE_DOUBLE(double x_ = 0.0, double y_ = 0.0) :
      x(x_), y(y_) { }
  NONTRIV_CLASS_DOUBLE_DOUBLE(const NONTRIV_CLASS_DOUBLE_DOUBLE &other) :
      x(other.x), y(other.y) {
    fprintf(stderr, "DOUBLE_DOUBLE Copy constructor!\n");
  }
  virtual ~NONTRIV_CLASS_DOUBLE_DOUBLE() {
    fprintf(stderr, "DOUBLE_DOUBLE destructor!\n");
  }
  virtual double foo(void) {
    return KDOUBLE1;
  }
  double x;
  double y;
};

class NONTRIV_CLASS_DOUBLE_DOUBLE2
    : public NONTRIV_CLASS_BASE1,
      public NONTRIV_CLASS_DOUBLE_DOUBLE {
 public:
  NONTRIV_CLASS_DOUBLE_DOUBLE2(double x_ = 0.0, double y_ = 0.0) :
      NONTRIV_CLASS_BASE1(1, 2), NONTRIV_CLASS_DOUBLE_DOUBLE(x_, y_) { }

  virtual double foo(void) {
    return KDOUBLE2;
  }
};

/* A union member cannot be of a class type that has a nontrivial
 * constructor. */
typedef union {
  CLASS_I32_I32 x;
  int32_t y;
  CLASS_DOUBLE_DOUBLE z;
} U_CLASS_INT_CLASS;


/*--- test member pointers and member function pointers ---*/

typedef double NONTRIV_CLASS_DOUBLE_DOUBLE::*pointer_to_member;
typedef double (NONTRIV_CLASS_DOUBLE_DOUBLE::*pointer_to_member_func) (void);

typedef struct {
  pointer_to_member x;
  pointer_to_member y;
} MEMBER_PTRS;

typedef struct {
  pointer_to_member_func x;
  pointer_to_member_func y;
} MEMBER_FUN_PTRS;

/* Test the case where a pointer_to_member_func might straddle an 8-byte. */
typedef struct {
  int32_t x;
  pointer_to_member_func y;
} I32_MEMBER_FUN_PTR;
typedef struct {
  int32_t x;
  pointer_to_member_func y;
  int32_t z;
} I32_MEMBER_FUN_PTR_I32;

/*--- test vectors ---*/

/* TODO(jvoung): Make sure this works with ARM too... */

/* Define the types here instead of using xmmintrin.h, etc. */

/* SSE1 (and NEON?)  */
typedef int v4si __attribute__ ((__vector_size__(16)));
typedef float v4sf __attribute__ ((__vector_size__(16)));
typedef float __m128 __attribute__((__vector_size__(16)));

/* SSE2 ... */
typedef double __v2df __attribute__ ((__vector_size__ (16)));
typedef double __m128d __attribute__((__vector_size__(16)));

typedef int64_t __v2di __attribute__ ((__vector_size__ (16)));
typedef int64_t __m128i __attribute__((__vector_size__(16)));

typedef int16_t __v8hi __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));

/* avxintrin (AVX) -- Not actually testing this since we might
 * not have bots that support it.
 */
typedef double __v4df __attribute__ ((__vector_size__ (32)));
typedef float __v8sf __attribute__ ((__vector_size__ (32)));
typedef int64_t __v4di __attribute__ ((__vector_size__ (32)));
typedef int __v8si __attribute__ ((__vector_size__ (32)));
typedef int16_t __v16hi __attribute__ ((__vector_size__ (32)));
typedef char __v32qi __attribute__ ((__vector_size__ (32)));

typedef float __m256 __attribute__ ((__vector_size__ (32)));
typedef double __m256d __attribute__((__vector_size__(32)));
typedef int64_t __m256i __attribute__((__vector_size__(32)));


/* Check complex double vs struct {double, double} (should be same) */
/* complex.h does not appear to be in newlib...
   typedef union { double complex x; int32_t y; } U_COMPLEX_I32;
   typedef union { struct { double a; double b; } x; int32_t y; }
   U_DOUBLE_DOUBLE_I32;
*/

/* TODO(jvoung): handle this separately, since these are extensions and
 * generate warnings / errors unless -pedantic is filtered.
 */

/* Flexible Arrays */
/* typedef struct {
   int32_t len;
   char flex[];
   } I32_CHARFLEX2; */


/*----- Constant definitions for the above types, along with CHECKs ------*/

/* For each TYPE, these must look like:
   - kTYPE
   - CHECK_TYPE
*/

/* no kEMPTY, since that is like void */

/* Hack: make static const so that each module can include this
   and not get a multiple definition error. */

/* Would be nicer if these were auto-generated, but for now keep it simple
 * and allow modifying each individual case. Each data type has a different
 * format for initialization and checking.
 */

static const CHAR_I32 kCHAR_I32 = { KCHAR1, KI321 };
#define CHECK_CHAR_I32(s)                       \
  ASSERT_EQ(s.x, KCHAR1, "(CHECK_CHAR_I32, x)") \
  ASSERT_EQ(s.y, KI321, "(CHECK_CHAR_I32, y)")

static const I32_I32 kI32_I32 = { KI322, KI321 };
#define CHECK_I32_I32(s)                        \
  ASSERT_EQ(s.x, KI322, "(CHECK_I32_I32, x)")   \
  ASSERT_EQ(s.y, KI321, "(CHECK_I32_I32, y)")

static const CHAR_I64 kCHAR_I64 = { KCHAR1, KI641 };
#define CHECK_CHAR_I64(s)                       \
  ASSERT_EQ(s.x, KCHAR1, "(CHECK_CHAR_I64, x)") \
  ASSERT_EQ(s.y, KI641, "(CHECK_CHAR_I64, y)")

static const I64_I64 kI64_I64 = { KI642, KI641 };
#define CHECK_I64_I64(s)                        \
  ASSERT_EQ(s.x, KI642, "(CHECK_I64_I64, x)")   \
  ASSERT_EQ(s.y, KI641, "(CHECK_I64_I64, y)")

static const I64_STRUCT kI64_STRUCT = { KI641 };
#define CHECK_I64_STRUCT(s)                     \
  ASSERT_EQ(s.x, KI641, "(CHECK_I64_STRUCT)")

static const I64_NON_STRUCT kI64_NON_STRUCT = KI641;
#define CHECK_I64_NON_STRUCT(x)                 \
  ASSERT_EQ(x, KI641, "(CHECK_I64_NON_STRUCT)")

static const I32_STRUCT kI32_STRUCT = { KI321 };
#define CHECK_I32_STRUCT(s)                     \
  ASSERT_EQ(s.x, KI321, "(CHECK_I32_STRUCT)")

static const I32_NON_STRUCT kI32_NON_STRUCT = KI321;
#define CHECK_I32_NON_STRUCT(x)                 \
  ASSERT_EQ(x, KI321, "(CHECK_I32_NON_STRUCT)")

static const I16_STRUCT kI16_STRUCT = { KI161 };
#define CHECK_I16_STRUCT(s)                     \
  ASSERT_EQ(s.x, KI161, "(CHECK_I16_STRUCT)")

static const I16_NON_STRUCT kI16_NON_STRUCT = KI161;
#define CHECK_I16_NON_STRUCT(x)                 \
  ASSERT_EQ(x, KI161, "(CHECK_I16_NON_STRUCT)")

static const DOUBLE_STRUCT kDOUBLE_STRUCT = { KDOUBLE1 };
#define CHECK_DOUBLE_STRUCT(s)                     \
  ASSERT_EQ(s.x, KDOUBLE1, "(CHECK_DOUBLE_STRUCT)")

static const DOUBLE_NON_STRUCT kDOUBLE_NON_STRUCT = KDOUBLE1;
#define CHECK_DOUBLE_NON_STRUCT(x)                 \
  ASSERT_EQ(x, KDOUBLE1, "(CHECK_DOUBLE_NON_STRUCT)")

static const FLOAT_STRUCT kFLOAT_STRUCT = { KFLOAT1 };
#define CHECK_FLOAT_STRUCT(s)                     \
  ASSERT_EQ(s.x, KFLOAT1, "(CHECK_FLOAT_STRUCT)")

static const FLOAT_NON_STRUCT kFLOAT_NON_STRUCT = KFLOAT1;
#define CHECK_FLOAT_NON_STRUCT(x)                 \
  ASSERT_EQ(x, KFLOAT1, "(CHECK_FLOAT_NON_STRUCT)")

static const I32_FLOAT kI32_FLOAT = { KI321, KFLOAT1 };
#define CHECK_I32_FLOAT(s)                          \
  ASSERT_EQ(s.x, KI321, "(CHECK_I32_FLOAT, x)")     \
  ASSERT_EQ(s.y, KFLOAT1, "(CHECK_I32_FLOAT, y)")

static const FLOAT_FLOAT kFLOAT_FLOAT = { KFLOAT2, KFLOAT1 };
#define CHECK_FLOAT_FLOAT(s)                        \
  ASSERT_EQ(s.x, KFLOAT2, "(CHECK_FLOAT_FLOAT, x)") \
  ASSERT_EQ(s.y, KFLOAT1, "(CHECK_FLOAT_FLOAT, y)")

static const STRUCT_STRUCT kSTRUCT_STRUCT = { { KI321, KCHAR1 },
                                              { KCHAR2, KFLOAT1 } };
#define CHECK_STRUCT_STRUCT(s)                              \
  ASSERT_EQ(s.x.a, KI321, "(CHECK_STRUCT_STRUCT, x.a")      \
  ASSERT_EQ(s.x.b, KCHAR1, "(CHECK_STRUCT_STRUCT, x.b")     \
  ASSERT_EQ(s.y.a, KCHAR2, "(CHECK_STRUCT_STRUCT, y.a")     \
  ASSERT_EQ(s.y.b, KFLOAT1, "(CHECK_STRUCT_STRUCT, y.b")

static const EMPTY kEMPTY = {};
static const PTR_EMPTYSTRUCT_PTR kPTR_EMPTYSTRUCT_PTR = { (int*) KPTR1,
                                                          kEMPTY,
                                                          (double*) KPTR2 };

#define CHECK_PTR_EMPTYSTRUCT_PTR(s) \
    ASSERT_EQ(s.a, KPTR1, "(CHECK_PTR_EMPTYSTRUCT_PTR, a)") \
    ASSERT_EQ(s.b, KPTR2, "(CHECK_PTR_EMPTYSTRUCT_PTR, b)")

static const I64_DOUBLE kI64_DOUBLE = { KI641, KDOUBLE1 };
#define CHECK_I64_DOUBLE(s)                         \
  ASSERT_EQ(s.x, KI641, "(CHECK_I64_DOUBLE, x)")    \
  ASSERT_EQ(s.y, KDOUBLE1, "(CHECK_I64_DOUBLE, y)")

static const DOUBLE_DOUBLE kDOUBLE_DOUBLE = { KDOUBLE2, KDOUBLE1 };
#define CHECK_DOUBLE_DOUBLE(s)                          \
  ASSERT_EQ(s.x, KDOUBLE2, "(CHECK_DOUBLE_DOUBLE, x)")  \
  ASSERT_EQ(s.y, KDOUBLE1, "(CHECK_DOUBLE_DOUBLE, y)")

static const CHAR_BOOL_I32_BOOL kCHAR_BOOL_I32_BOOL = {
  KCHAR1, KBOOL1, KI321, KBOOL2 };
#define CHECK_CHAR_BOOL_I32_BOOL(s)                         \
  ASSERT_EQ(s.x, KCHAR1, "(CHECK_CHAR_BOOL_I32_BOOL, x)")   \
  ASSERT_EQ(s.y, KBOOL1, "(CHECK_CHAR_BOOL_I32_BOOL, y)")   \
  ASSERT_EQ(s.z, KI321, "(CHECK_CHAR_BOOL_I32_BOOL, z)")    \
  ASSERT_EQ(s.w, KBOOL2, "(CHECK_CHAR_BOOL_I32_BOOL, w)")

static const I32_ALIGN16 kI32_ALIGN16 = { KI321 };
#define CHECK_I32_ALIGN16(s)                    \
  ASSERT_EQ(s.x, KI321, "(CHECK_I32_ALIGN16")

static const I32_CHAR12 kI32_CHAR12 = { KI321,
                                        { KCHAR1, KCHAR2, KCHAR3, KCHAR4,
                                          KCHAR1, KCHAR2, KCHAR3, KCHAR4,
                                          KCHAR1, KCHAR2, KCHAR3, KCHAR4} };
#define CHECK_I32_CHAR12(s)                                         \
  ASSERT_EQ(s.x, KI321, "(CHECK_I32_CHAR12, x)")                    \
  ASSERT_EQ(s.user_data[0], KCHAR1, "(CHECK_I32_CHAR12, [0])")      \
  ASSERT_EQ(s.user_data[1], KCHAR2, "(CHECK_I32_CHAR12, [1])")      \
  ASSERT_EQ(s.user_data[2], KCHAR3, "(CHECK_I32_CHAR12, [2])")      \
  ASSERT_EQ(s.user_data[3], KCHAR4, "(CHECK_I32_CHAR12, [3])")      \
  ASSERT_EQ(s.user_data[4], KCHAR1, "(CHECK_I32_CHAR12, [4])")      \
  ASSERT_EQ(s.user_data[5], KCHAR2, "(CHECK_I32_CHAR12, [5])")      \
  ASSERT_EQ(s.user_data[6], KCHAR3, "(CHECK_I32_CHAR12, [6])")      \
  ASSERT_EQ(s.user_data[7], KCHAR4, "(CHECK_I32_CHAR12, [7])")      \
  ASSERT_EQ(s.user_data[8], KCHAR1, "(CHECK_I32_CHAR12, [8])")      \
  ASSERT_EQ(s.user_data[9], KCHAR2, "(CHECK_I32_CHAR12, [9])")      \
  ASSERT_EQ(s.user_data[10], KCHAR3, "(CHECK_I32_CHAR12, [10])")    \
  ASSERT_EQ(s.user_data[11], KCHAR4, "(CHECK_I32_CHAR12, [11])")

static const ARRAY_I32_4 kARRAY_I32_4 = { { KI324, KI323, KI322, KI321 } };
#define CHECK_ARRAY_I32_4(s)                          \
  ASSERT_EQ(s.x[0], KI324, "(CHECK_ARRAY_I32_4, x[0])")  \
  ASSERT_EQ(s.x[1], KI323, "(CHECK_ARRAY_I32_4, x[1])")  \
  ASSERT_EQ(s.x[2], KI322, "(CHECK_ARRAY_I32_4, x[2])")  \
  ASSERT_EQ(s.x[3], KI321, "(CHECK_ARRAY_I32_4, x[3])")

static const ARRAY_FLOAT_4 kARRAY_FLOAT_4 = {
  { KFLOAT4, KFLOAT3, KFLOAT2, KFLOAT1 } };
#define CHECK_ARRAY_FLOAT_4(s)                          \
  ASSERT_EQ(s.x[0], KFLOAT4, "(CHECK_ARRAY_FLOAT_4, x[0])")  \
  ASSERT_EQ(s.x[1], KFLOAT3, "(CHECK_ARRAY_FLOAT_4, x[1])")  \
  ASSERT_EQ(s.x[2], KFLOAT2, "(CHECK_ARRAY_FLOAT_4, x[2])")  \
  ASSERT_EQ(s.x[3], KFLOAT1, "(CHECK_ARRAY_FLOAT_4, x[3])")

static const DOUBLE_DOUBLE_DOUBLE kDOUBLE_DOUBLE_DOUBLE = {
  KDOUBLE3, KDOUBLE2, KDOUBLE1 };
#define CHECK_DOUBLE_DOUBLE_DOUBLE(s)                           \
  ASSERT_EQ(s.x, KDOUBLE3, "(CHECK_DOUBLE_DOUBLE_DOUBLE, x)")   \
  ASSERT_EQ(s.y, KDOUBLE2, "(CHECK_DOUBLE_DOUBLE_DOUBLE, y)")   \
  ASSERT_EQ(s.z, KDOUBLE1, "(CHECK_DOUBLE_DOUBLE_DOUBLE, z)")

static const CHAR_I64_I32 kCHAR_I64_I32 = {
  KCHAR1, KI641, KI321 };
#define CHECK_CHAR_I64_I32(s)                       \
  ASSERT_EQ(s.x, KCHAR1, "(CHECK_CHAR_I64_I32, x)") \
  ASSERT_EQ(s.y, KI641, "(CHECK_CHAR_I64_I32, y)")  \
  ASSERT_EQ(s.z, KI321, "(CHECK_CHAR_I64_I32, z)")

static const I32_CHAR_ALIGN32 kI32_CHAR_ALIGN32 = {
  KI321, KCHAR1 };
#define CHECK_I32_CHAR_ALIGN32(s)                       \
  ASSERT_EQ(s.x, KI321, "(CHECK_I32_CHAR_ALIGN32, x)") \
  ASSERT_EQ(s.y, KCHAR1, "(CHECK_I32_CHAR_ALIGN32, y)")

static const BITFIELD_STRADDLE kBITFIELD_STRADDLE = {
  (KI321 >> 1), (KI322 >> 1), (KI323 >> 1) };
#define CHECK_BITFIELD_STRADDLE(_s_)                                \
  ASSERT_EQ(_s_.x, (KI321 >> 1), "(CHECK_BITFIELD_STRADDLE, x)")    \
  ASSERT_EQ(_s_.y, (KI322 >> 1), "(CHECK_BITFIELD_STRADDLE, y)")    \
  ASSERT_EQ(_s_.z, (KI323 >> 1), "(CHECK_BITFIELD_STRADDLE, z)")

static const NONBITFIELD_STRADDLE kNONBITFIELD_STRADDLE = {
  KI161, KI641 };
#define CHECK_NONBITFIELD_STRADDLE(s)                           \
  ASSERT_EQ(s.x, KI161, "(CHECK_NONBITFIELD_STRADDLE, x)")    \
  ASSERT_EQ(s.y, KI641, "(CHECK_NONBITFIELD_STRADDLE, y)")


/*--- Unions ---*/

static const U_I64_DOUBLE kU_I64_DOUBLE = { KI641 };
#define CHECK_U_I64_DOUBLE(s)                       \
  ASSERT_EQ(s.x, KI641, "(CHECK_U_I64_DOUBLE, x)")

static const U_DOUBLE_I64 kU_DOUBLE_I64 = { KDOUBLE1 };
#define CHECK_U_DOUBLE_I64(s)                           \
  ASSERT_EQ(s.x, KDOUBLE1, "(CHECK_U_DOUBLE_I64, x)")

static const U_DOUBLE_ARRAY_I16_4 kU_DOUBLE_ARRAY_I16_4 = { KDOUBLE1 };
#define CHECK_U_DOUBLE_ARRAY_I16_4(s)                           \
  ASSERT_EQ(s.x, KDOUBLE1, "(CHECK_U_DOUBLE_ARRAY_I16_4, x)")

static const U_ARRAY_I16_4_DOUBLE kU_ARRAY_I16_4_DOUBLE = {
  { KI164, KI163, KI162, KI161 } };
#define CHECK_U_ARRAY_I16_4_DOUBLE(s)                           \
  ASSERT_EQ(s.x[0], KI164, "(CHECK_U_ARRAY_I16_4_DOUBLE, x[0])")    \
  ASSERT_EQ(s.x[1], KI163, "(CHECK_U_ARRAY_I16_4_DOUBLE, x[1])")    \
  ASSERT_EQ(s.x[2], KI162, "(CHECK_U_ARRAY_I16_4_DOUBLE, x[2])")    \
  ASSERT_EQ(s.x[3], KI161, "(CHECK_U_ARRAY_I16_4_DOUBLE, x[3])")

static const U_STRADDLE_BF_NONBF kU_STRADDLE_BF_NONBF = {
  { (KI321 >> 1), (KI322 >> 1), (KI323 >> 1) } };
#define CHECK_U_STRADDLE_BF_NONBF(_s_)          \
  CHECK_BITFIELD_STRADDLE(_s_.x)

static const U_I16_STRUCT kU_I16_STRUCT = { KI161 };
#define CHECK_U_I16_STRUCT(s)                         \
  ASSERT_EQ(s.x, KI161, "(CHECK_U_I16_STRUCT, x)")

/*--- Enums ---*/

static const ENUM1 kENUM1 = E4;
#define CHECK_ENUM1(s)                          \
  ASSERT_EQ(s, kENUM1, "(CHECK_ENUM1)")

static const ENUM1_PACKED8 kENUM1_PACKED8 = E4_PACKED8;
#define CHECK_ENUM1_PACKED8(s)                                  \
  fprintf(stderr, "Size of packed enum: %u\n", sizeof(s));      \
  ASSERT_EQ(s, kENUM1_PACKED8 , "(CHECK_ENUM1_PACKED8)")

static const ENUM1_PACKED16 kENUM1_PACKED16 = E4_PACKED16;
#define CHECK_ENUM1_PACKED16(s)                                 \
  fprintf(stderr, "Size of packed enum: %u\n", sizeof(s));      \
  ASSERT_EQ(s, kENUM1_PACKED16, "(CHECK_ENUM1_PACKED16)")

static const ENUM1_PACKED24 kENUM1_PACKED24 = E4_PACKED24;
#define CHECK_ENUM1_PACKED24(s)                                 \
  fprintf(stderr, "Size of packed enum: %u\n", sizeof(s));      \
  ASSERT_EQ(s, kENUM1_PACKED24, "(CHECK_ENUM1_PACKED24)")

static const ENUM1_PACKED32 kENUM1_PACKED32 = E4_PACKED32;
#define CHECK_ENUM1_PACKED32(s)                                 \
  fprintf(stderr, "Size of packed enum: %u\n", sizeof(s));      \
  ASSERT_EQ(s, kENUM1_PACKED32, "(CHECK_ENUM1_PACKED32)")

/*--- Trivial C++ classes vs Non-Trivial C++ classes ---*/

static const CLASS_I32_I32 kCLASS_I32_I32 = {KI322, KI321};
#define CHECK_CLASS_I32_I32(s)                      \
  ASSERT_EQ(s.x, KI322, "(CHECK_CLASS_I32_I32, x)") \
  ASSERT_EQ(s.y, KI321, "(CHECK_CLASS_I32_I32, y)")

static const CLASS_DOUBLE_DOUBLE kCLASS_DOUBLE_DOUBLE = {KDOUBLE2, KDOUBLE1};
#define CHECK_CLASS_DOUBLE_DOUBLE(s)                            \
  ASSERT_EQ(s.x, KDOUBLE2, "(CHECK_CLASS_DOUBLE_DOUBLE, x)")    \
  ASSERT_EQ(s.y, KDOUBLE1, "(CHECK_CLASS_DOUBLE_DOUBLE, y)")

static const NONTRIV_CLASS_I32_I32 kNONTRIV_CLASS_I32_I32(KI322, KI321);
#define CHECK_NONTRIV_CLASS_I32_I32(s)                      \
  ASSERT_EQ(s.x, KI322, "(CHECK_NONTRIV_CLASS_I32_I32, x)") \
  ASSERT_EQ(s.y, KI321, "(CHECK_NONTRIV_CLASS_I32_I32, y)")

static const NONTRIV_CLASS_DOUBLE_DOUBLE
kNONTRIV_CLASS_DOUBLE_DOUBLE(KDOUBLE2, KDOUBLE1);
#define CHECK_NONTRIV_CLASS_DOUBLE_DOUBLE(s)                            \
  ASSERT_EQ(s.x, KDOUBLE2, "(CHECK_NONTRIV_CLASS_DOUBLE_DOUBLE, x)")    \
  ASSERT_EQ(s.y, KDOUBLE1, "(CHECK_NONTRIV_CLASS_DOUBLE_DOUBLE, y)")

static const U_CLASS_INT_CLASS kU_CLASS_INT_CLASS = { {KI322, KI321 } };
#define CHECK_U_CLASS_INT_CLASS(s)                            \
  ASSERT_EQ(s.x.x, KI322, "(CHECK_U_CLASS_INT_CLASS, x.x)")   \
  ASSERT_EQ(s.x.y, KI321, "(CHECK_U_CLASS_INT_CLASS, x.y)")


/*--- Member pointers and member function pointers. ---*/

static const MEMBER_PTRS kMEMBER_PTRS = {
  &NONTRIV_CLASS_DOUBLE_DOUBLE::x,
  &NONTRIV_CLASS_DOUBLE_DOUBLE::y
};
#define CHECK_MEMBER_PTRS(s)                            \
  do {                                                  \
    NONTRIV_CLASS_DOUBLE_DOUBLE some_instance(0, 0);    \
    some_instance.*(s.x) = KDOUBLE2;                    \
    some_instance.*(s.y) = KDOUBLE1;                    \
    CHECK_NONTRIV_CLASS_DOUBLE_DOUBLE(some_instance);   \
  } while (0)

static const MEMBER_FUN_PTRS kMEMBER_FUN_PTRS = {
  &NONTRIV_CLASS_DOUBLE_DOUBLE::foo,
  &NONTRIV_CLASS_DOUBLE_DOUBLE::foo
};
#define CHECK_MEMBER_FUN_PTRS(s)                            \
  do {                                                      \
    NONTRIV_CLASS_DOUBLE_DOUBLE some_instance(0.0, 0.0);    \
    NONTRIV_CLASS_DOUBLE_DOUBLE2 some_instance2(0.0, 0.0);  \
    ASSERT_EQ((some_instance.*(s.x))(), KDOUBLE1,           \
              "(CHECK_MEMBER_FUN_PTRS, instance1, x)");     \
    ASSERT_EQ((some_instance.*(s.y))(), KDOUBLE1,           \
              "(CHECK_MEMBER_FUN_PTRS, instance1, y)");     \
    ASSERT_EQ((some_instance2.*(s.x))(), KDOUBLE2,          \
              "(CHECK_MEMBER_FUN_PTRS, instance2, x)");     \
    ASSERT_EQ((some_instance2.*(s.y))(), KDOUBLE2,          \
              "(CHECK_MEMBER_FUN_PTRS, instance2, y)");     \
  } while (0)

static const I32_MEMBER_FUN_PTR kI32_MEMBER_FUN_PTR = {
  KI321,
  &NONTRIV_CLASS_DOUBLE_DOUBLE::foo,
};
#define CHECK_I32_MEMBER_FUN_PTR(s)                         \
  do {                                                      \
    ASSERT_EQ(s.x, KI321, "(CHECK_I32_MEMBER_FUN_PTR, x)"); \
    NONTRIV_CLASS_DOUBLE_DOUBLE2 some_instance(0.0, 0.0);   \
    ASSERT_EQ((some_instance.*(s.y))(), KDOUBLE2,           \
              "(CHECK_I32_MEMBER_FUN_PTR, instance, y)");   \
  } while (0)

static const I32_MEMBER_FUN_PTR_I32 kI32_MEMBER_FUN_PTR_I32 = {
  KI322,
  &NONTRIV_CLASS_DOUBLE_DOUBLE::foo,
  KI321
};
#define CHECK_I32_MEMBER_FUN_PTR_I32(s)                         \
  do {                                                          \
    ASSERT_EQ(s.x, KI322, "(CHECK_I32_MEMBER_FUN_PTR_I32, x)"); \
    NONTRIV_CLASS_DOUBLE_DOUBLE2 some_instance(0.0, 0.0);       \
    ASSERT_EQ((some_instance.*(s.y))(), KDOUBLE2,               \
              "(CHECK_I32_MEMBER_FUN_PTR_I32, instance, y)");   \
    ASSERT_EQ(s.z, KI321, "(CHECK_I32_MEMBER_FUN_PTR_I32, z)"); \
  } while (0)

/*--- Test vectors ---*/

#define MAKE_PUNNER(type, elem_type, num_elems)     \
  typedef union {                                   \
    type v;                                         \
    elem_type elems[num_elems];                     \
  } Vec_##type;

static const __m128 k__m128 = { KFLOAT1, KFLOAT2, KFLOAT3, KFLOAT4 };
MAKE_PUNNER(__m128, float, 4)
#define CHECK___m128(s)                                                 \
  do {                                                                  \
    Vec___m128 local_copy;                                              \
    local_copy.v = s;                                                   \
    ASSERT_EQ(local_copy.elems[0], KFLOAT1,                             \
              "(CHECK___m128, [0])");                                   \
    ASSERT_EQ(local_copy.elems[1], KFLOAT2,                             \
              "(CHECK___m128, [1])");                                   \
    ASSERT_EQ(local_copy.elems[2], KFLOAT3,                             \
              "(CHECK___m128, [2])");                                   \
    ASSERT_EQ(local_copy.elems[3], KFLOAT4,                             \
              "(CHECK___m128, [3])");                                   \
  } while (0)

#endif  /* NATIVE_CLIENT_TESTS_CALLINGCONV_SMALL_USEFUL_STRUCTS_H */
