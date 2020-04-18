/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gtest/gtest.h"

#include <fcntl.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/validator/ncvalidate.h"
#include "native_client/src/trusted/validator/validation_cache.h"
#include "native_client/src/trusted/validator/validation_cache_internal.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/validator/rich_file_info.h"
#include "native_client/src/trusted/validator/validation_metadata.h"

#define CONTEXT_MARKER 31
#define QUERY_MARKER 37

#define CODE_SIZE 64

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
# define NOP 0x00
#else  // x86
# define NOP 0x90
#endif


#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
// jr ra
const uint8_t ret[] = { 0x08, 0x00, 0xe0, 0x03 };
#else  // x86
// ret
const uint8_t ret[] = { 0xc3 };
#endif

// Example of an instruction which may get "stubbed out" (replaced with
// HLTs) if the CPU does not support it.
const uint8_t sse41[] =
    { 0x66, 0x0f, 0x3a, 0x0e, 0xd0, 0xc0 };  // pblendw $0xc0,%xmm0,%xmm2

const uint8_t sse41_plus_nontemporal[] =
    { 0x66, 0x0f, 0x3a, 0x0e, 0xd0, 0xc0,  // pblendw $0xc0,%xmm0,%xmm2
      // Example of a non-temporal instruction that is rewritten without
      // being rejected entirely.
#if NACL_BUILD_SUBARCH == 32
      0x66, 0x0f, 0xe7, 0x04, 0x24  // movntdq %xmm0,(%esp)
#else
      0x66, 0x41, 0x0f, 0xe7, 0x07  // movntdq %xmm0,(%r15)
#endif
    };

// Example of a valid JMP to outside the bundle, in a bundle containing an
// instruction that gets stubbed out.
const uint8_t sse41_plus_valid_jmp[] =
    { 0x66, 0x0f, 0x3a, 0x0e, 0xd0, 0xc0,  // pblendw $0xc0,%xmm0,%xmm2
      0xeb, 0x19 };  // jmp to a non-bundle-aligned nop in the next bundle

// Example of an invalid JMP to outside the bundle, in a bundle containing
// an instruction that gets stubbed out.
const uint8_t sse41_plus_invalid_jmp[] =
    { 0x66, 0x0f, 0x3a, 0x0e, 0xd0, 0xc0,  // pblendw $0xc0,%xmm0,%xmm2
      0xeb, 0x39 };  // jmp to non-bundle-aligned address beyond the chunk end

struct MockContext {
  int marker; /* Sanity check that we're getting the right object. */
  int query_result;
  int add_count_expected;
  bool set_validates_expected;
  bool query_destroyed;
};

enum MockQueryState {
  QUERY_CREATED,
  QUERY_GET_CALLED,
  QUERY_SET_CALLED,
  QUERY_DESTROYED
};

struct MockQuery {
  /* Sanity check that we're getting the right object. */
  int marker;
  MockQueryState state;
  int add_count;
  MockContext *context;
};

void *MockCreateQuery(void *handle) {
  MockContext *mcontext = (MockContext *) handle;
  MockQuery *mquery = (MockQuery *) malloc(sizeof(MockQuery));
  EXPECT_EQ(CONTEXT_MARKER, mcontext->marker);
  mquery->marker = QUERY_MARKER;
  mquery->state = QUERY_CREATED;
  mquery->add_count = 0;
  mquery->context = mcontext;
  return mquery;
}

void MockAddData(void *query, const unsigned char *data, size_t length) {
  UNREFERENCED_PARAMETER(data);
  MockQuery *mquery = (MockQuery *) query;
  ASSERT_EQ(QUERY_MARKER, mquery->marker);
  EXPECT_EQ(QUERY_CREATED, mquery->state);
  /* Small data is suspicious. */
  EXPECT_LE((size_t) 2, length);
  mquery->add_count += 1;
}

int MockQueryCodeValidates(void *query) {
  MockQuery *mquery = (MockQuery *) query;
  EXPECT_EQ(QUERY_MARKER, mquery->marker);
  EXPECT_EQ(QUERY_CREATED, mquery->state);
  EXPECT_EQ(mquery->context->add_count_expected, mquery->add_count);
  mquery->state = QUERY_GET_CALLED;
  return mquery->context->query_result;
}

void MockSetCodeValidates(void *query) {
  MockQuery *mquery = (MockQuery *) query;
  ASSERT_EQ(QUERY_MARKER, mquery->marker);
  EXPECT_EQ(QUERY_GET_CALLED, mquery->state);
  EXPECT_EQ(true, mquery->context->set_validates_expected);
  mquery->state = QUERY_SET_CALLED;
}

void MockDestroyQuery(void *query) {
  MockQuery *mquery = (MockQuery *) query;
  ASSERT_EQ(QUERY_MARKER, mquery->marker);
  if (mquery->context->set_validates_expected) {
    EXPECT_EQ(QUERY_SET_CALLED, mquery->state);
  } else {
    EXPECT_EQ(QUERY_GET_CALLED, mquery->state);
  }
  mquery->state = QUERY_DESTROYED;
  mquery->context->query_destroyed = true;
  free(mquery);
}

/* Hint that the validation should use the (fake) cache. */
int MockCachingIsInexpensive(const struct NaClValidationMetadata *metadata) {
  UNREFERENCED_PARAMETER(metadata);
  return 1;
}

class ValidationCachingInterfaceTests : public ::testing::Test {
 protected:
  MockContext context;
  NaClValidationMetadata *metadata_ptr;
  NaClValidationCache cache;
  const struct NaClValidatorInterface *validator;
  NaClCPUFeatures *cpu_features;

  unsigned char code_buffer[CODE_SIZE];

  void SetUp() {
    context.marker = CONTEXT_MARKER;
    context.query_result = 1;
    context.add_count_expected = 4;
    context.set_validates_expected = false;
    context.query_destroyed = false;

    metadata_ptr = NULL;

    cache.handle = &context;
    cache.CreateQuery = MockCreateQuery;
    cache.AddData = MockAddData;
    cache.QueryKnownToValidate = MockQueryCodeValidates;
    cache.SetKnownToValidate = MockSetCodeValidates;
    cache.DestroyQuery = MockDestroyQuery;
    cache.CachingIsInexpensive = MockCachingIsInexpensive;

    validator = NaClCreateValidator();
    cpu_features = (NaClCPUFeatures *) malloc(validator->CPUFeatureSize);
    EXPECT_NE(cpu_features, (NaClCPUFeatures *) NULL);
    validator->SetAllCPUFeatures(cpu_features);

    memset(code_buffer, NOP, sizeof(code_buffer));
  }

  NaClValidationStatus Validate() {
    return validator->Validate(0, code_buffer, CODE_SIZE,
                               FALSE,  /* stubout_mode */
                               0,      /* flags */
                               FALSE,  /* readonly_text */
                               cpu_features,
                               metadata_ptr,
                               &cache);
  }

  void TearDown() {
    free(cpu_features);
  }
};

TEST_F(ValidationCachingInterfaceTests, Sanity) {
  void *query = cache.CreateQuery(cache.handle);
  context.add_count_expected = 2;
  cache.AddData(query, NULL, 6);
  cache.AddData(query, NULL, 128);
  EXPECT_EQ(1, cache.QueryKnownToValidate(query));
  cache.DestroyQuery(query);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, NoCache) {
  const struct NaClValidatorInterface *validator = NaClCreateValidator();
  NaClValidationStatus status = validator->Validate(
      0, code_buffer, CODE_SIZE,
      FALSE, /* stubout_mode */
      0,     /* flags */
      FALSE, /* readonly_text */
      cpu_features,
      NULL, /* metadata */
      NULL);
  EXPECT_EQ(NaClValidationSucceeded, status);
}

TEST_F(ValidationCachingInterfaceTests, CacheHit) {
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, CacheMiss) {
  context.query_result = 0;
  context.set_validates_expected = true;
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
TEST_F(ValidationCachingInterfaceTests, SSE4Allowed) {
  memcpy(code_buffer, sse41, sizeof(sse41));
  context.query_result = 0;
  context.set_validates_expected = true;
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, SSE4Stubout) {
  // Check that the validation is not cached if the validator modifies the
  // the code (by stubbing out instructions).
  memcpy(code_buffer, sse41, sizeof(sse41));
  context.query_result = 0;
  /* TODO(jfb) Use a safe cast here, this test should only run for x86. */
  NaClSetCPUFeatureX86((NaClCPUFeaturesX86 *) cpu_features,
                       NaClCPUFeatureX86_SSE41, 0);
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);

  // Check that the SSE4.1 instruction gets overwritten with HLTs.
  for (size_t index = 0; index < CODE_SIZE; ++index) {
    if (index < sizeof(sse41)) {
      EXPECT_EQ(0xf4 /* HLT */, code_buffer[index]);
    } else {
      EXPECT_EQ(NOP, code_buffer[index]);
    }
  }
}

TEST_F(ValidationCachingInterfaceTests, NonTemporalStubout) {
  // The validation should not be cached if the validator modifies the
  // code.  This test checks for a regression where that property was
  // broken if the code contained a non-temporal instruction.
  memcpy(code_buffer, sse41_plus_nontemporal, sizeof(sse41_plus_nontemporal));
  context.query_result = 0;
  /* TODO(jfb) Use a safe cast here, this test should only run for x86. */
  NaClSetCPUFeatureX86((NaClCPUFeaturesX86 *) cpu_features,
                       NaClCPUFeatureX86_SSE41, 0);
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, RevalidationOfValidJump) {
  // This tests a case where an instruction gets rewritten (replaced with
  // HLTs).  In this case, the validator should revalidate the bundle after
  // modifying it.  This test case checks that the revalidation logic
  // correctly handles JMPs to outside the bundle.
  //
  // This isn't strictly related to validation caching, but it is
  // convenient to reuse its text fixtures.
  memcpy(code_buffer, sse41_plus_valid_jmp, sizeof(sse41_plus_valid_jmp));
  context.query_result = 0;
  /* TODO(jfb) Use a safe cast here, this test should only run for x86. */
  NaClSetCPUFeatureX86((NaClCPUFeaturesX86 *) cpu_features,
                       NaClCPUFeatureX86_SSE41, 0);
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, RevalidationOfInvalidJump) {
  // Like RevalidationOfValidJump, this is another test case for the
  // revalidation logic.  This test checks that we don't allow an invalid
  // jump to outside the code chunk.
  memcpy(code_buffer, sse41_plus_invalid_jmp, sizeof(sse41_plus_invalid_jmp));
  context.query_result = 0;
  /* TODO(jfb) Use a safe cast here, this test should only run for x86. */
  NaClSetCPUFeatureX86((NaClCPUFeaturesX86 *) cpu_features,
                       NaClCPUFeatureX86_SSE41, 0);
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationFailed, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, MultipleStubout) {
  // If a bundle contains multiple instructions that need to be rewritten,
  // check that the revalidation logic handles this correctly.
  memcpy(code_buffer, sse41, sizeof(sse41));
  memcpy(code_buffer + sizeof(sse41), sse41, sizeof(sse41));
  context.query_result = 0;
  /* TODO(jfb) Use a safe cast here, this test should only run for x86. */
  NaClSetCPUFeatureX86((NaClCPUFeaturesX86 *) cpu_features,
                       NaClCPUFeatureX86_SSE41, 0);
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);

  // Check that the SSE4.1 instructions get overwritten with HLTs.
  for (size_t index = 0; index < CODE_SIZE; ++index) {
    if (index < sizeof(sse41) * 2) {
      EXPECT_EQ(0xf4 /* HLT */, code_buffer[index]);
    } else {
      EXPECT_EQ(NOP, code_buffer[index]);
    }
  }
}
#endif

TEST_F(ValidationCachingInterfaceTests, IllegalInst) {
  memcpy(code_buffer, ret, sizeof(ret));
  context.query_result = 0;
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationFailed, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, IllegalCacheHit) {
  memcpy(code_buffer, ret, sizeof(ret));
  NaClValidationStatus status = Validate();
  // Success proves the cache shortcircuted validation.
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

TEST_F(ValidationCachingInterfaceTests, Metadata) {
  NaClValidationMetadata metadata;
  memset(&metadata, 0, sizeof(metadata));
  metadata.identity_type = NaClCodeIdentityFile;
  metadata.file_name = (char *) "foobar";
  metadata.file_name_length = strlen(metadata.file_name);
  metadata.file_size = CODE_SIZE;
  metadata.mtime = 100;
  metadata_ptr = &metadata;
  context.add_count_expected = 12;
  NaClValidationStatus status = Validate();
  EXPECT_EQ(NaClValidationSucceeded, status);
  EXPECT_EQ(true, context.query_destroyed);
}

class ValidationCachingSerializationTests : public ::testing::Test {
 protected:
  struct NaClRichFileInfo info;
  uint8_t *buffer;
  uint32_t buffer_length;
  struct NaClRichFileInfo inp;
  struct NaClRichFileInfo outp;

  void SetUp() {
    buffer = 0;
    buffer_length = 0;
    NaClRichFileInfoCtor(&inp);
    NaClRichFileInfoCtor(&outp);
  }

  void TearDown() {
    free(buffer);
    // Don't free the inp structure, it does not contain malloced memory.
    NaClRichFileInfoDtor(&outp);
  }
};

TEST_F(ValidationCachingSerializationTests, NormalOperationSimple) {
  inp.known_file = 0;
  inp.file_path = "foo";
  inp.file_path_length = 3;
  EXPECT_EQ(0, NaClSerializeNaClDescMetadata(&inp, &buffer, &buffer_length));
  EXPECT_EQ(0, NaClDeserializeNaClDescMetadata(buffer, buffer_length, &outp));

  EXPECT_EQ((uint8_t) 0, outp.known_file);
  EXPECT_EQ((uint32_t) 0, outp.file_path_length);
  EXPECT_EQ(NULL, outp.file_path);
}

TEST_F(ValidationCachingSerializationTests, NormalOperationFull) {
  inp.known_file = 1;
  inp.file_path = "foo";
  inp.file_path_length = 3;

  EXPECT_EQ(0, NaClSerializeNaClDescMetadata(&inp, &buffer, &buffer_length));
  EXPECT_EQ(0, NaClDeserializeNaClDescMetadata(buffer, buffer_length, &outp));

  EXPECT_EQ((uint8_t) 1, outp.known_file);
  EXPECT_EQ((uint32_t) 3, outp.file_path_length);
  EXPECT_EQ(0, memcmp("foo", outp.file_path, outp.file_path_length));
}

TEST_F(ValidationCachingSerializationTests, BadSizeSimple) {
  inp.known_file = 0;
  EXPECT_EQ(0, NaClSerializeNaClDescMetadata(&inp, &buffer, &buffer_length));
  for (uint32_t i = -1; i <= buffer_length + 4; i++) {
    /* The only case that is OK. */
    if (i == buffer_length)
      continue;

    /* Wrong number of bytes, fail. */
    EXPECT_EQ(1, NaClDeserializeNaClDescMetadata(buffer, i, &outp));
  }
}

TEST_F(ValidationCachingSerializationTests, BadSizeFull) {
  inp.known_file = 1;
  inp.file_path = "foo";
  inp.file_path_length = 3;
  EXPECT_EQ(0, NaClSerializeNaClDescMetadata(&inp, &buffer, &buffer_length));
  for (uint32_t i = -1; i <= buffer_length + 4; i++) {
    /* The only case that is OK. */
    if (i == buffer_length)
      continue;

    /* Wrong number of bytes, fail. */
    EXPECT_EQ(1, NaClDeserializeNaClDescMetadata(buffer, i, &outp));
    /* Paranoia. */
    EXPECT_EQ(0, outp.known_file);
    /* Make sure we don't leak on failure. */
    EXPECT_EQ(NULL, outp.file_path);
  }
}

static char *AN_ARBITRARY_FILE_PATH = NULL;

class ValidationCachingFileOriginTests : public ::testing::Test {
 protected:
  struct NaClDesc *desc;

  struct NaClRichFileInfo inp;
  struct NaClRichFileInfo outp;

  void SetUp() {
    struct NaClHostDesc *host_desc = NULL;
    int fd = open(AN_ARBITRARY_FILE_PATH, O_RDONLY);

    desc = NULL;
    NaClRichFileInfoCtor(&inp);
    NaClRichFileInfoCtor(&outp);

    ASSERT_NE(-1, fd);
    host_desc = NaClHostDescPosixMake(fd, NACL_ABI_O_RDONLY);
    desc = (struct NaClDesc *) NaClDescIoDescMake(host_desc);
    ASSERT_NE((struct NaClDesc *) NULL, desc);
  }

  void TearDown() {
    // Don't free the inp structure, it does not contain malloced memory.
    NaClRichFileInfoDtor(&outp);
    NaClDescSafeUnref(desc);
  }
};

TEST_F(ValidationCachingFileOriginTests, None) {
  EXPECT_EQ(1, NaClGetFileOriginInfo(desc, &outp));
}

TEST_F(ValidationCachingFileOriginTests, Simple) {
  inp.known_file = 0;
  inp.file_path = "foobar";
  inp.file_path_length = 6;
  EXPECT_EQ(0, NaClSetFileOriginInfo(desc, &inp));
  EXPECT_EQ(0, NaClGetFileOriginInfo(desc, &outp));

  EXPECT_EQ(0, outp.known_file);
  EXPECT_EQ((uint32_t) 0, outp.file_path_length);
  EXPECT_EQ(NULL, outp.file_path);
}


TEST_F(ValidationCachingFileOriginTests, Full) {
  inp.known_file = 1;
  inp.file_path = "foobar";
  inp.file_path_length = 6;
  EXPECT_EQ(0, NaClSetFileOriginInfo(desc, &inp));
  EXPECT_EQ(0, NaClGetFileOriginInfo(desc, &outp));

  EXPECT_EQ(1, outp.known_file);
  EXPECT_EQ((uint32_t) 6, outp.file_path_length);
  EXPECT_EQ(0, memcmp("foobar", outp.file_path, outp.file_path_length));
}

// Test driver function.
int main(int argc, char *argv[]) {
  // One file we know must exist is this executable.
  AN_ARBITRARY_FILE_PATH = argv[0];
  // The IllegalInst test touches the log mutex deep inside the validator.
  // This causes an SEH exception to be thrown on Windows if the mutex is not
  // initialized.
  // http://code.google.com/p/nativeclient/issues/detail?id=1696
  NaClLogModuleInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
