NULL =

ACLOCAL_AMFLAGS = -I m4

TESTS = \
  cff_type2_charstring_test \
  layout_common_table_test \
  tests/test_bad_fonts.sh \
  $(NULL)

bin_PROGRAMS = \
  ots-sanitize \
  $(NULL)

if HAVE_SYSTIME
bin_PROGRAMS += ots-perf
endif

if HAVE_FREETYPE
bin_PROGRAMS += \
  ots-idempotent \
  ots-validator-checker \
  ots-side-by-side \
  $(NULL)

TESTS += tests/test_good_fonts.sh
else
if HAVE_CORETEXT
bin_PROGRAMS += \
  ots-idempotent \
  ots-validator-checker \
  $(NULL)

TESTS += tests/test_good_fonts.sh
else
if HAVE_WIN32
bin_PROGRAMS += \
  ots-idempotent \
  $(NULL)

TESTS += tests/test_good_fonts.sh
endif # HAVE_WIN32
endif # HAVE_CORETEXT
endif # HAVE_FREETYPE

noinst_PROGRAMS = ots-fuzzer

if HAVE_FSANITIZE
noinst_PROGRAMS += ots-testfuzz
TESTS += tests/test_fuzzed_fonts.sh
endif

lib_LIBRARIES = \
  libots.a \
  libbrotli.a \
  libwoff2.a \
  libz.a \
  $(NULL)

if OTS_GRAPHITE
lib_LIBRARIES += \
  liblz4.a \
  $(NULL)
endif

check_PROGRAMS = \
  cff_type2_charstring_test \
  layout_common_table_test \
  $(NULL)

check_LIBRARIES = \
  libgtest.a \
  $(NULL)


libots_a_SOURCES = \
  src/cff.cc \
  src/cff.h \
  src/cff_type2_charstring.cc \
  src/cff_type2_charstring.h \
  src/cmap.cc \
  src/cmap.h \
  src/cvt.cc \
  src/cvt.h \
  src/fpgm.cc \
  src/fpgm.h \
  src/gasp.cc \
  src/gasp.h \
  src/gdef.cc \
  src/gdef.h \
  src/glyf.cc \
  src/glyf.h \
  src/gpos.cc \
  src/gpos.h \
  src/gsub.cc \
  src/gsub.h \
  src/hdmx.cc \
  src/hdmx.h \
  src/head.cc \
  src/head.h \
  src/hhea.cc \
  src/hhea.h \
  src/hmtx.cc \
  src/hmtx.h \
  src/kern.cc \
  src/kern.h \
  src/layout.cc \
  src/layout.h \
  src/loca.cc \
  src/loca.h \
  src/ltsh.cc \
  src/ltsh.h \
  src/maxp.cc \
  src/maxp.h \
  src/math.cc \
  src/math_.h \
  src/metrics.cc \
  src/metrics.h \
  src/name.cc \
  src/name.h \
  src/os2.cc \
  src/os2.h \
  src/ots.cc \
  src/ots.h \
  src/post.cc \
  src/post.h \
  src/prep.cc \
  src/prep.h \
  src/vdmx.cc \
  src/vdmx.h \
  src/vhea.cc \
  src/vhea.h \
  src/vmtx.cc \
  src/vmtx.h \
  src/vorg.cc \
  src/vorg.h \
  $(NULL)

if OTS_GRAPHITE
libots_a_SOURCES += \
  src/feat.cc \
  src/feat.h \
  src/glat.cc \
  src/glat.h \
  src/gloc.cc \
  src/gloc.h \
  src/graphite.h \
  src/sile.h \
  src/sile.cc \
  src/silf.h \
  src/silf.cc \
  src/sill.h \
  src/sill.cc \
  $(NULL)
endif

libots_a_CPPFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/third_party/woff2/include \
  -I$(top_srcdir)/third_party/zlib \
  $(NULL)

if OTS_GRAPHITE
libots_a_CPPFLAGS += \
  -I$(top_srcdir)/third_party/lz4/lib \
  $(NULL)
endif

pkginclude_HEADERS = \
  include/ots-memory-stream.h \
  include/opentype-sanitiser.h \
  $(NULL)


libbrotli_a_SOURCES = \
  third_party/brotli/c/common/dictionary.c \
  third_party/brotli/c/common/dictionary.h \
  third_party/brotli/c/common/constants.h \
  third_party/brotli/c/common/version.h \
  third_party/brotli/c/dec/bit_reader.c \
  third_party/brotli/c/dec/bit_reader.h \
  third_party/brotli/c/dec/context.h \
  third_party/brotli/c/dec/decode.c \
  third_party/brotli/c/dec/huffman.c \
  third_party/brotli/c/dec/huffman.h \
  third_party/brotli/c/dec/port.h \
  third_party/brotli/c/dec/prefix.h \
  third_party/brotli/c/dec/state.c \
  third_party/brotli/c/dec/state.h \
  third_party/brotli/c/dec/transform.h \
  third_party/brotli/c/include/brotli/decode.h \
  third_party/brotli/c/include/brotli/port.h \
  third_party/brotli/c/include/brotli/types.h \
  $(NULL)

libbrotli_a_CPPFLAGS = \
  -I$(top_srcdir)/third_party/brotli/c/include \
  $(NULL)


liblz4_a_SOURCES = \
  third_party/lz4/lib/lz4.h \
  third_party/lz4/lib/lz4.c \
  $(NULL)


libz_a_SOURCES = \
  third_party/zlib/adler32.c \
  third_party/zlib/compress.c \
  third_party/zlib/crc32.c \
  third_party/zlib/crc32.h \
  third_party/zlib/deflate.c \
  third_party/zlib/deflate.h \
  third_party/zlib/gzclose.c \
  third_party/zlib/gzguts.h \
  third_party/zlib/gzlib.c \
  third_party/zlib/gzread.c \
  third_party/zlib/gzwrite.c \
  third_party/zlib/infback.c \
  third_party/zlib/inffast.c \
  third_party/zlib/inffast.h \
  third_party/zlib/inffixed.h \
  third_party/zlib/inflate.c \
  third_party/zlib/inflate.h \
  third_party/zlib/inftrees.c \
  third_party/zlib/inftrees.h \
  third_party/zlib/trees.c \
  third_party/zlib/trees.h \
  third_party/zlib/uncompr.c \
  third_party/zlib/zconf.h \
  third_party/zlib/zlib.h \
  third_party/zlib/zutil.c \
  third_party/zlib/zutil.h \
  $(NULL)


libwoff2_a_SOURCES = \
  third_party/woff2/include/woff2/decode.h \
  third_party/woff2/include/woff2/output.h \
  third_party/woff2/src/buffer.h \
  third_party/woff2/src/port.h \
  third_party/woff2/src/round.h \
  third_party/woff2/src/store_bytes.h \
  third_party/woff2/src/table_tags.cc \
  third_party/woff2/src/table_tags.h \
  third_party/woff2/src/variable_length.cc \
  third_party/woff2/src/variable_length.h \
  third_party/woff2/src/woff2_common.cc \
  third_party/woff2/src/woff2_common.h \
  third_party/woff2/src/woff2_dec.cc \
  third_party/woff2/src/woff2_out.cc \
  $(NULL)

libwoff2_a_CPPFLAGS = \
  -I$(top_srcdir)/third_party/brotli/c/include \
  -I$(top_srcdir)/third_party/woff2/include \
  $(NULL)


ots_sanitize_SOURCES = \
  util/ots-sanitize.cc \
  util/test-context.h \
  $(NULL)


ots_perf_SOURCES = \
  util/ots-perf.cc \
  $(NULL)


ots_idempotent_SOURCES = \
  util/ots-idempotent.cc \
  util/test-context.h \
  $(NULL)

ots_idempotent_CXXFLAGS = \
  $(CORETEXT_CFLAGS) \
  $(FREETYPE_CFLAGS) \
  $(NULL)

ots_idempotent_LDADD = \
  $(LDADD) \
  $(CORETEXT_LIBS) \
  $(FREETYPE_LIBS) \
  $(NULL)


ots_validator_checker_SOURCES = \
  util/ots-validator-checker.cc \
  $(NULL)

ots_validator_checker_CXXFLAGS = \
  $(CORETEXT_CFLAGS) \
  $(FREETYPE_CFLAGS) \
  $(NULL)

ots_validator_checker_LDADD = \
  $(LDADD) \
  $(CORETEXT_LIBS) \
  $(FREETYPE_LIBS) \
  $(NULL)


ots_side_by_side_SOURCES = \
  util/ots-side-by-side.cc \
  $(NULL)

ots_side_by_side_CXXFLAGS = \
  $(FREETYPE_CFLAGS) \
  $(NULL)

ots_side_by_side_LDADD = \
  $(LDADD) \
  $(FREETYPE_LIBS) \
  $(NULL)


# Used by OSS-Fuzz, should not directly use -fsanitize
ots_fuzzer_SOURCES = \
  util/ots-fuzzer.cc \
  $(NULL)

ots_fuzzer_CPPFLAGS = \
  -I$(top_srcdir)/src \
  $(AM_CPPFLAGS) \
  $(NULL)


ots_testfuzz_SOURCES = \
  util/ots-fuzzer.cc \
  $(libbrotli_a_SOURCES) \
  $(libots_a_SOURCES) \
  $(libwoff2_a_SOURCES) \
  $(libz_a_SOURCES) \
  $(NULL)

if OTS_GRAPHITE
ots_testfuzz_SOURCES += \
  $(liblz4_a_SOURCES) \
  $(NULL)
endif

ots_testfuzz_CPPFLAGS = \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/src \
  -I$(top_srcdir)/third_party/brotli/c/include \
  -I$(top_srcdir)/third_party/woff2/include \
  -I$(top_srcdir)/third_party/zlib \
  $(AM_CPPFLAGS) \
  $(NULL)

if OTS_GRAPHITE
ots_testfuzz_CPPFLAGS += \
  -I$(top_srcdir)/third_party/lz4/lib \
  $(NULL)
endif

ots_testfuzz_CXXFLAGS = \
  $(ASAN_CFLAGS) \
  $(UBSAN_CFLAGS) \
  $(NULL)


cff_type2_charstring_test_SOURCES = \
  third_party/googletest/googletest/src/gtest_main.cc \
  tests/cff_type2_charstring_test.cc \
  $(NULL)

cff_type2_charstring_test_CPPFLAGS = \
  -I$(top_srcdir)/src \
  $(GTEST_CPPFLAGS) \
  $(AM_CPPFLAGS) \
  $(NULL)

cff_type2_charstring_test_CXXFLAGS = \
  $(PTHREAD_CFLAGS) \
  $(NULL)

cff_type2_charstring_test_LDADD = \
  libgtest.a \
  $(LDADD) \
  $(PTHREAD_LIBS) \
  $(NULL)


layout_common_table_test_SOURCES = \
  third_party/googletest/googletest/src/gtest_main.cc \
  tests/layout_common_table_test.cc \
  $(NULL)

layout_common_table_test_CPPFLAGS = \
  -I$(top_srcdir)/src \
  $(GTEST_CPPFLAGS) \
  $(AM_CPPFLAGS) \
  $(NULL)

layout_common_table_test_CXXFLAGS = \
  $(PTHREAD_CFLAGS) \
  $(NULL)

layout_common_table_test_LDADD = \
  libgtest.a \
  $(LDADD) \
  $(PTHREAD_LIBS) \
  $(NULL)


libgtest_a_SOURCES = \
  third_party/googletest/googletest/include/gtest/gtest-death-test.h \
  third_party/googletest/googletest/include/gtest/gtest.h \
  third_party/googletest/googletest/include/gtest/gtest-message.h \
  third_party/googletest/googletest/include/gtest/gtest-param-test.h \
  third_party/googletest/googletest/include/gtest/gtest_pred_impl.h \
  third_party/googletest/googletest/include/gtest/gtest-printers.h \
  third_party/googletest/googletest/include/gtest/gtest_prod.h \
  third_party/googletest/googletest/include/gtest/gtest-spi.h \
  third_party/googletest/googletest/include/gtest/gtest-test-part.h \
  third_party/googletest/googletest/include/gtest/gtest-typed-test.h \
  third_party/googletest/googletest/include/gtest/internal/custom/gtest.h \
  third_party/googletest/googletest/include/gtest/internal/custom/gtest-port.h \
  third_party/googletest/googletest/include/gtest/internal/custom/gtest-printers.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-death-test-internal.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-filepath.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-internal.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-linked_ptr.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-param-util-generated.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-param-util.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-port-arch.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-port.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-string.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-tuple.h \
  third_party/googletest/googletest/include/gtest/internal/gtest-type-util.h \
  third_party/googletest/googletest/src/gtest.cc \
  third_party/googletest/googletest/src/gtest-death-test.cc \
  third_party/googletest/googletest/src/gtest-filepath.cc \
  third_party/googletest/googletest/src/gtest-internal-inl.h \
  third_party/googletest/googletest/src/gtest-port.cc \
  third_party/googletest/googletest/src/gtest-printers.cc \
  third_party/googletest/googletest/src/gtest-test-part.cc \
  third_party/googletest/googletest/src/gtest-typed-test.cc \
  $(NULL)

libgtest_a_CPPFLAGS = \
  $(AM_CPPFLAGS) \
  $(GTEST_CPPFLAGS) \
  $(NULL)

libgtest_a_CXXFLAGS = \
  $(PTHREAD_CFLAGS) \
  $(NULL)

libgtest_a_LIBADD = \
  $(PTHREAD_LIBS) \
  $(NULL)

GTEST_CPPFLAGS += \
  -I$(top_srcdir)/third_party/googletest/googletest/include \
  -I$(top_srcdir)/third_party/googletest/googletest/ \
  $(NULL)

if HAVE_WIN32
GTEST_CPPFLAGS += -DGTEST_OS_WINDOWS
endif


AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(NULL)

LDADD = \
  libots.a \
  libwoff2.a \
  libbrotli.a \
  libz.a \
  $(NULL)

if OTS_GRAPHITE
LDADD += \
  liblz4.a \
  $(NULL)
endif

if HAVE_WIN32
LDADD += -lgdi32
endif

AM_TESTS_ENVIRONMENT = \
  EXEEXT="$(EXEEXT)"; \
  export EXEEXT; \
  top_srcdir="$(top_srcdir)"; \
  export top_srcdir; \
  top_builddir="$(top_builddir)"; \
  export top_builddir; \
  $(NULL)


EXTRA_DIST = \
  LICENSE \
  docs/DesignDoc.md \
  docs/HowToFix.md \
  docs/HowToTest.md \
  tests/BLACKLIST.txt \
  tests/test_fuzzed_fonts.sh \
  tests/test_bad_fonts.sh \
  tests/test_good_fonts.sh \
  third_party/brotli/LICENSE \
  third_party/lz4/LICENSE \
  third_party/googletest/googletest/LICENSE \
  third_party/woff2/LICENSE \
  third_party/zlib/README \
  tests/fonts/bad/0014256514b220c525e98840b0d6ba736a85acbd.ttf \
  tests/fonts/bad/003a9baf5665abc42fae0d7027718294601385b9.ttf \
  tests/fonts/bad/00ac7a910785ea3a30655fe386d4cb02b39719aa.otf \
  tests/fonts/bad/013d9956e40d1ea194c4d7817fbf220d6be9c33b.ttf \
  tests/fonts/bad/01592f6b2f515888a44dc6311c481a57edf79644.ttf \
  tests/fonts/bad/024ad17b23e4298f1d80246b63d63d8e54c76e3d.ttf \
  tests/fonts/bad/0349e0140ce7dc311f52413dea38b8d64df5cd0c.ttf \
  tests/fonts/bad/035341aa98a4342f18cf59d7332a77794c5a8923.ttf \
  tests/fonts/bad/03b667c2ab94eba7e078bc91cd7145e04f352cd5.ttf \
  tests/fonts/bad/07165649b488b700585197b1d1083f94ac61d525.ttf \
  tests/fonts/bad/078400f5718f84841fc43a04fd28d946ddad5e37.otf \
  tests/fonts/bad/080a26ad508861558f5f69836881bb0b8e8842c2.ttf \
  tests/fonts/bad/08b3b69027fc071fa71910cc7857833b7e5f4534.ttf \
  tests/fonts/bad/092da87de7e293efee8be43f531b5bf0612693a5.ttf \
  tests/fonts/bad/0a228b7430d18283b6564822c06e8521dac93407.ttf \
  tests/fonts/bad/0a85ab417c6726d99235a6f9aaaf4879f711f01b.ttf \
  tests/fonts/bad/0c60702ee5003855555fee931a2da7daa917a1ef.ttf \
  tests/fonts/bad/0e4a70a10a82f7f4d63865006e81a445de26d3d1.ttf \
  tests/fonts/bad/0f4f4a8e4a247694370ff11ae8b129395eca85fd.ttf \
  tests/fonts/bad/0fce4352e04a156a1cb8d4bc344cd2be1dfe7177.otf \
  tests/fonts/bad/103ad354e517994c447b9c5c6914a290517cb5ed.otf \
  tests/fonts/bad/103b47d389cb700e76893677f51f7b74d09d08d5.ttf \
  tests/fonts/bad/13124b99e25a379efbdd1c3820b1484f842e1ad5.ttf \
  tests/fonts/bad/173c4c23291c983fead3d734afd8a4de504f508e.otf \
  tests/fonts/bad/17cbd36aad32fe96dfb6dc49ceaaed54553c9189.ttf \
  tests/fonts/bad/1ccbe1de6cfd3c183f067d4a2549f81405740a1d.ttf \
  tests/fonts/bad/1de19636832bb7ebb45680fb09b44227f19a96ce.ttf \
  tests/fonts/bad/23b2a3316b3797bf9a61acdf36cf63d10ab3342e.woff \
  tests/fonts/bad/251a4ffb5418b336217a9e7958941192b5a20137.otf \
  tests/fonts/bad/28ce9a4dc67328856213bb1da09d55e4cf561c68.ttf \
  tests/fonts/bad/2d0bf4e52a64aa8bda2fdd9597e1bb4fbf33eb2b.woff2 \
  tests/fonts/bad/2d80771036e065b9cd582b769d0388a0de90f84f.ttf \
  tests/fonts/bad/2edb1d50d2b8f4ccf8b7d56e7f354dd86be081f9.ttf \
  tests/fonts/bad/335e7e5a354010624679dfb52609652c4a7f6fc8.ttf \
  tests/fonts/bad/33df9c8b531928c90a3afebf6b15828dd0cad929.otf \
  tests/fonts/bad/33f430d6661b687d4449d0f0878da477a50f41c1.ttf \
  tests/fonts/bad/345cfba1a71738b238de912c04370623f153f72f.ttf \
  tests/fonts/bad/35159513f8d8422ff1a880a039ad68c857660cdd.ttf \
  tests/fonts/bad/3b7eaf30cc528463aa344305b5ab9528e1bc3c15.ttf \
  tests/fonts/bad/3b9bcfd57f7f18dd4c45ba51e77b23b3895faa9f.ttc \
  tests/fonts/bad/3ba5737414924ed17800ff60ff21a524a513e111.ttf \
  tests/fonts/bad/3bfa96a443c19de63f28554b748bf84e57ee51c3.otf \
  tests/fonts/bad/410761bc1115173bad3f6b0d3d72103e1dc32f67.otf \
  tests/fonts/bad/42e947856c0ee90faf78051ffc41e527a8ee8be9.woff2 \
  tests/fonts/bad/457227a3d4dbf6dd062d251b2ae0c8b31f14c9b8.otf \
  tests/fonts/bad/494f40cf28ec4c2753aa907941fe48b7d15f6b04.ttf \
  tests/fonts/bad/4c8e7bcd6b657941c4e846f6e48bb714fb3faf92.otf \
  tests/fonts/bad/4d29ba6a49fcba83c9a0cb91835b8e5e9fbe2e0b.ttf \
  tests/fonts/bad/4d9ba653319ad56fa13a724e6d2c357ff948c2ea.ttf \
  tests/fonts/bad/52053c2de32349ee09bbb6a4e755c2b3e85b9821.woff \
  tests/fonts/bad/55c321d6932942d22555ff9b985e905eb472d4d1.ttf \
  tests/fonts/bad/5733cd9ca76aead50df6240b6b42d466e78240b2.woff \
  tests/fonts/bad/58b7bd1ebd3396d226c5a777098d115a2fc9f56f.ttf \
  tests/fonts/bad/58c575a08b375e51c9776275c5f877396ddf552b.otf \
  tests/fonts/bad/5c67809a0d4b9a16d9eba881282f920153b761eb.otf \
  tests/fonts/bad/65932a3c0a5ebcbc1ac55267e98506e9dedf41a5.ttf \
  tests/fonts/bad/66803a29a4e2dc941921d82abf376857583b49db.ttf \
  tests/fonts/bad/68bfc9f9233e34db2c5ecb5fb966bf205f92079a.otf \
  tests/fonts/bad/692df3e4b35e0b83cc8991a19ae29e434e705124.ttf \
  tests/fonts/bad/698e25666a75ea6121065b5fced5ccee62b65ca4.otf \
  tests/fonts/bad/6d2cc2870fabdc4fa686b66accd404759349e4a8.ttf \
  tests/fonts/bad/7304e91492c3ba14030446c92f084f8fb031e8e8.ttf \
  tests/fonts/bad/7522970cb3bb80a698c206cfa61f8418767c1704.ttf \
  tests/fonts/bad/7c8eefff708b374fffbce135ba28a4565766e560.otf \
  tests/fonts/bad/7ee3cdda8571ff1e51aa081825f72cd0a545e052.ttf \
  tests/fonts/bad/81942d3ea419539b69990ba98f824a8a46dcb951.ttf \
  tests/fonts/bad/85d903c71a429ed98a012e742a700cbe2fef005c.ttf \
  tests/fonts/bad/8a97d860fcbd1294be09f2d0aebb764f2c12f69c.woff \
  tests/fonts/bad/8edb1c6072ff63478456cc93601b77b0eb3432e7.otf \
  tests/fonts/bad/90d60863109aab420257ee10577f2673cb91b3e7.ttf \
  tests/fonts/bad/92520e16995b11b01f56b3834f200416f656161d.ttf \
  tests/fonts/bad/9682ce841ae16214afca9b5e584162611f88dfff.ttf \
  tests/fonts/bad/985de319c172b29bd9e651b093271a8939c35f13.ttf \
  tests/fonts/bad/a53afc75de22a49c7eebf925b6f54e6dcf208900.ttf \
  tests/fonts/bad/a80d45906af055b269d6e81eeae9e91d5c26ef27.ttf \
  tests/fonts/bad/a9a801b5149ecc0c9fe30db4beec2c511b0e3f71.ttf \
  tests/fonts/bad/aae49b8143752111464bfee4e06514a648b4365e.woff2 \
  tests/fonts/bad/af434603052be497e74415f2f160df0d6989aded.ttf \
  tests/fonts/bad/af7109c13a67bf110892429dfe658a8f62670ce1.ttf \
  tests/fonts/bad/b49c3750db6a1538283586cbf1115e973383350c.ttf \
  tests/fonts/bad/b9a7339a1ff85286437d93e59df48cc35e5610d6.ttf \
  tests/fonts/bad/b9ea4b9b671307bc0f9745dd684e1e4f6e48191d.otf \
  tests/fonts/bad/bd5ee794425c2809f262b4bc6e3c0b33008fca73.otf \
  tests/fonts/bad/c22404e0f69eaca5bbfbecd13d2941df5748a8e3.ttf \
  tests/fonts/bad/c69c27b17d332fbf33dbc8f25baa0a0a461293a2.otf \
  tests/fonts/bad/ca5b8d77bb1f1a094cc0032e83487f3cde33a9d6.ttf \
  tests/fonts/bad/cb5656f950377ad5de2c7a6df0a659fbb7065ffe.ttf \
  tests/fonts/bad/cbc1d3e183f8138f94b788baa397413d7863eb07.ttf \
  tests/fonts/bad/cf6934b8e2d100b495e219d1b079fa2c34133d08.ttf \
  tests/fonts/bad/d262af9b0b7836f7395ae8b1a4a37e38fa0ae6c3.ttf \
  tests/fonts/bad/dbcd1243ae657ee8934a699d474be2d0ba79da94.ttf \
  tests/fonts/bad/dcc4f3316f90bc8a4c05e0086d2b49fddd3a059b.otf \
  tests/fonts/bad/e071a5082117ad5a64dc3db1bfcd0a31d6db93ae.ttf \
  tests/fonts/bad/ea8c4b1d5178ae184ffd0346f12fd426850729cc.otf \
  tests/fonts/bad/eaeb6d903b14ee184f887aa8a0a81b917e252da9.otf \
  tests/fonts/bad/eb44137aa49ccb9ea7aad127a8fdc6e155f20565.ttf \
  tests/fonts/bad/ed550c912a8c5febfdad9ca2500b3284ecfac0d7.ttf \
  tests/fonts/bad/f1dba4340ce94f5359fa4434debc7efcfd1b521f.ttf \
  tests/fonts/fuzzing/011facefb10ee4f813117eae60bb5940a280ae30.woff2 \
  tests/fonts/fuzzing/0509e80afb379d16560e9e47bdd7d888bebdebc6.ttf \
  tests/fonts/fuzzing/051d92f8bc6ff724511b296c27623f824de256e9.ttf \
  tests/fonts/fuzzing/07f054357ff8638bac3711b422a1e31180bba863.ttf \
  tests/fonts/fuzzing/11340844a1190f140c78a0a964d49b5985253ea0.woff2 \
  tests/fonts/fuzzing/15dfc433a135a658b9f4b1a861b5cdd9658ccbb9.ttf \
  tests/fonts/fuzzing/1679504907c14c2f27119135bd5da57f8713dd05.woff \
  tests/fonts/fuzzing/16a8e02ff1373539ca818da4f70e9b8573563f29.woff \
  tests/fonts/fuzzing/17ebbcdaf31a8f80c5efa07f4dc11ee40de7ee7f.woff \
  tests/fonts/fuzzing/17f60353ae7621b585e8d98a546bbf8479649535.woff2 \
  tests/fonts/fuzzing/181a03beec3e4aa1814fb24a3b8ca5f930629955.woff \
  tests/fonts/fuzzing/191826b9643e3f124d865d617ae609db6a2ce203.ttf \
  tests/fonts/fuzzing/1a6f1687b7a221f9f2c834b0b360d3c8463b6daf.ttf \
  tests/fonts/fuzzing/1acb1be6d45ca77a1b734e3892b823a34634d38e.woff2 \
  tests/fonts/fuzzing/1c04a16f32a39c26c851b7fc014d2e8d298ba2b8.ttf \
  tests/fonts/fuzzing/1c2c3fc37b2d4c3cb2ef726c6cdaaabd4b7f3eb9.ttf \
  tests/fonts/fuzzing/1c2fb74c1b2aa173262734c1f616148f1648cfd6.ttf \
  tests/fonts/fuzzing/1d5090bfad0eeb11c88882085d4a195f73926327.woff2 \
  tests/fonts/fuzzing/1e665cdc2796a76d8fd46232566795aa890991b1.woff2 \
  tests/fonts/fuzzing/2028193bd133228575af7000c60fa6f3e46a4bf0.woff2 \
  tests/fonts/fuzzing/205edd09bd3d141cc9580f650109556cc28b22cb.otf \
  tests/fonts/fuzzing/205edd09bd3d141cc9580f650109556cc28b22cb.ttf \
  tests/fonts/fuzzing/2069628b7b2ff2f8367f46c366f010c4f5a6295e.woff \
  tests/fonts/fuzzing/217a934cfe15c548b572c203dceb2befdf026462.ttf \
  tests/fonts/fuzzing/226bc2deab3846f1a682085f70c67d0421014144.ttf \
  tests/fonts/fuzzing/2393b84159bd84138529dd136ba6bf45a7addd79.woff \
  tests/fonts/fuzzing/270b89df543a7e48e206a2d830c0e10e5265c630.ttf \
  tests/fonts/fuzzing/27539eee6487a75cc1ddf8c80b8e3f863ee540c3.woff \
  tests/fonts/fuzzing/294c88607d6ebf0bd78f2c8e234e03e2d9b186d5.woff2 \
  tests/fonts/fuzzing/298c9e1d955f10f6f72c6915c3c6ff9bf9695cec.ttf \
  tests/fonts/fuzzing/29ca0fe494750bb483ed291cffa89d55279d3ac9.woff2 \
  tests/fonts/fuzzing/2a124fa3e39a294280d406c4790398726bdc66ca.woff \
  tests/fonts/fuzzing/2abab3950432f64f17882a6517a9b7d2fb2dafe7.woff \
  tests/fonts/fuzzing/3493e92eaded2661cadde752a39f9d58b11f0326.ttf \
  tests/fonts/fuzzing/3511ff5c1647150595846ac414c595cccac34f18.ttf \
  tests/fonts/fuzzing/361ce9ae6e20175595cac0cb82addcb184e20953.woff2 \
  tests/fonts/fuzzing/3684bc52f9aa06e9e6de67d80b626d44ca2bccf5.woff \
  tests/fonts/fuzzing/37033cc5cf37bb223d7355153016b6ccece93b28.ttf \
  tests/fonts/fuzzing/375d6ae32a3cbe52fbf81a4e5777e3377675d5a3.ttf \
  tests/fonts/fuzzing/39e15202b9fa9700438dbae6343ad8b21b359a68.woff \
  tests/fonts/fuzzing/3b1f374416818644fa5cdb1d592659e502506888.woff2 \
  tests/fonts/fuzzing/3e66a8cf6c20ccb36bce728f5d818f18145499c7.woff \
  tests/fonts/fuzzing/3fef5bc4d1567fea803c815a0d53d6d7bfa9bdf7.woff \
  tests/fonts/fuzzing/41c14459f0f134a81fc5f4051b861bae8503c87b.woff \
  tests/fonts/fuzzing/43979b90b2dd929723cf4fe1715990bcb9c9a56b.otf \
  tests/fonts/fuzzing/43979b90b2dd929723cf4fe1715990bcb9c9a56b.ttf \
  tests/fonts/fuzzing/43ef465752be9af900745f72fe29cb853a1401a5.ttf \
  tests/fonts/fuzzing/442a5b09fc1d847eb353b9e219e26067df8b9fb8.woff2 \
  tests/fonts/fuzzing/44a458acc6abfe78166e99c5ec9ec0fcf46182af.woff \
  tests/fonts/fuzzing/4502415a27007e8f37af2d6663ccf909a45abba4.woff2 \
  tests/fonts/fuzzing/45855bc8d46332b39c4ab9e2ee1a26b1f896da6b.ttf \
  tests/fonts/fuzzing/49c9f7485c1392fa09a1b801bc2ffea79275f22e.ttf \
  tests/fonts/fuzzing/4af3c3b1bf2882e84f25b30bc4aedae2a0b5f98a.woff2 \
  tests/fonts/fuzzing/4cce528e99f600ed9c25a2b69e32eb94a03b4ae8.ttf \
  tests/fonts/fuzzing/4d707d06afca5573a717fa3a9e825863c35ca786.ttf \
  tests/fonts/fuzzing/4fcaec74137a83b5304d7d5f830b81c9abed73e5.woff2 \
  tests/fonts/fuzzing/5028afb650b1bb718ed2131e872fbcce57828fff.ttf \
  tests/fonts/fuzzing/52a846621453e288b657f84ef423ff3f832b6c0c.woff \
  tests/fonts/fuzzing/52b6e52e7382c7c7e5ce839cc5df0cd3ae133add.ttf \
  tests/fonts/fuzzing/558661aa659912f4d30ecd27bd09835171a8e2b0.ttf \
  tests/fonts/fuzzing/5614290438dcf671fcc1f2858ac9127e45f500bb.woff2 \
  tests/fonts/fuzzing/56bf9590c3f7587a632539b74a7aa0a04dbaadd3.woff \
  tests/fonts/fuzzing/56cfd0e18d07f41c38e9598545a6d369127fc6f9.ttf \
  tests/fonts/fuzzing/57a9d9f83020155cbb1d2be1f43d82388cbecc88.ttf \
  tests/fonts/fuzzing/5a5daf5eb5a4db77a2baa3ad9c7a6ed6e0655fa8.ttf \
  tests/fonts/fuzzing/5ac36fb372dd26e0e1142c82c95b0e3807ae8136.woff \
  tests/fonts/fuzzing/5dfad7735c6a67085f1b90d4d497e32907db4c78.ttf \
  tests/fonts/fuzzing/5dfcc2260fa1a08c72655c18885e50260703c863.woff \
  tests/fonts/fuzzing/60adec4d200651dbf91e7c373789bbc2a2c4b46b.woff \
  tests/fonts/fuzzing/641bd9db850193064d17575053ae2bf8ec149ddc.ttf \
  tests/fonts/fuzzing/6466d38c62e73a39202435a4f73bf5d6acbb73c0.ttf \
  tests/fonts/fuzzing/6830389bba9787db7da796f364aa47b951e658ae.woff2 \
  tests/fonts/fuzzing/6991b13ce889466be6de3f66e891de2bc0f117ee.ttf \
  tests/fonts/fuzzing/6c1506d68d00e95a065d97e30964857e0d91c5a1.woff \
  tests/fonts/fuzzing/6e43399b43162dedd7054e00ae97e7b0a71bf72d.woff2 \
  tests/fonts/fuzzing/6eb6ce1f3930892d42a24d9fe955585c40a59cc0.woff2 \
  tests/fonts/fuzzing/6ff0fbead4462d9f229167b4e6839eceb8465058.ttf \
  tests/fonts/fuzzing/7042436c3b20a4426fc9a2dba76f68b2b27423af.woff2 \
  tests/fonts/fuzzing/706c5d7b625f207bc0d874c67237aad6f1e9cd6f.ttf \
  tests/fonts/fuzzing/71eca05729f14607e81e9476dce804fe3bafc086.woff \
  tests/fonts/fuzzing/72106114384cb564d4691bd3ea38960b0148f00b.woff \
  tests/fonts/fuzzing/72566065770241b8c31bd35201ad62102962a5aa.woff \
  tests/fonts/fuzzing/73bf759cd86e12788cd3571daffedeb391fad078.woff2 \
  tests/fonts/fuzzing/757ebd573617a24aa9dfbf0b885c54875c6fe06b.otf \
  tests/fonts/fuzzing/757ebd573617a24aa9dfbf0b885c54875c6fe06b.ttf \
  tests/fonts/fuzzing/79e3983181b2bef104af453fdc98f4a335d48bd5.woff2 \
  tests/fonts/fuzzing/7a37dc4d5bf018456aea291cee06daf004c0221c.ttf \
  tests/fonts/fuzzing/7e14e7883ed152baa158b80e207b66114c823a8b.otf \
  tests/fonts/fuzzing/7e14e7883ed152baa158b80e207b66114c823a8b.ttf \
  tests/fonts/fuzzing/7ef276fc886ea502a03b9b0e5c8b547d5dc2b61c.ttf \
  tests/fonts/fuzzing/8099955657a54e9ee38a6ba1d6f950ce58e3cc25.ttf \
  tests/fonts/fuzzing/813c2f8e5512187fd982417a7fb4286728e6f4a8.ttf \
  tests/fonts/fuzzing/814e2e2e76682ab9a518493b475b23565993a4df.woff2 \
  tests/fonts/fuzzing/81e64d72d3dd87b1d36744927d3842492f1c1001.woff2 \
  tests/fonts/fuzzing/8240789f6d12d4cfc4b5e8e6f246c3701bcf861f.otf \
  tests/fonts/fuzzing/8240789f6d12d4cfc4b5e8e6f246c3701bcf861f.ttf \
  tests/fonts/fuzzing/82fae690a786b2d96af7d4845206b528381fa109.woff2 \
  tests/fonts/fuzzing/8454d22037f892e76614e1645d066689a0200e61.ttf \
  tests/fonts/fuzzing/85414f2552b654585b7a8d13dcc3e8fd9f7970a3.ttf \
  tests/fonts/fuzzing/884801ab66022b60168010261438a4ed983ea9a7.woff \
  tests/fonts/fuzzing/8a8dc0703e7a9d14427ceadcea275dfda6a7f844.woff2 \
  tests/fonts/fuzzing/8a9fea2a7384f2116e5b84a9b31f83be7850ce21.ttf \
  tests/fonts/fuzzing/8faaf7e0d92db057ff26cf5af44a44e873c054cb.woff2 \
  tests/fonts/fuzzing/908810fd6350c87a62c2dc4bb623e54ddd8aa042.woff \
  tests/fonts/fuzzing/9192ffc3ddba8dc44a21ccac20bf743593642241.woff2 \
  tests/fonts/fuzzing/93ef7dcd6968068b41515a381aa1b04535312b01.woff \
  tests/fonts/fuzzing/94895e7495f726fa316ca3f2a03c98b86dc20560.ttc \
  tests/fonts/fuzzing/9883c9ae58c5dd598ab2fdc142e2dacbc0ad1cb3.woff2 \
  tests/fonts/fuzzing/995ca3cf99525e11fcdfeb5ba9f45a6cc602fcfb.woff \
  tests/fonts/fuzzing/99e0f8817ca25d074ff4456de4296c4397e23563.woff \
  tests/fonts/fuzzing/99e872a7478a9f7f7f2a183d623f2a5ebaff72ad.woff \
  tests/fonts/fuzzing/9a6305f950f8e3960618b78fca6ba7d7abf3b231.ttf \
  tests/fonts/fuzzing/9c3c16ff5fb0e0adef3344f011b5bbb1469f2fd4.woff \
  tests/fonts/fuzzing/9cb9bb35205cb4a7588a5a2b8e35fc4be856fc95.woff \
  tests/fonts/fuzzing/9f553001b12ed154a54de011828fd78138c66113.woff2 \
  tests/fonts/fuzzing/a0ca212e069702964aba80ab9c28bcb8aabae782.woff2 \
  tests/fonts/fuzzing/a34a7b00f22ffb5fd7eef6933b81c7e71bc2cdfb.ttf \
  tests/fonts/fuzzing/a34a9191d9376bda419836effeef7e75c1386016.ttf \
  tests/fonts/fuzzing/a5ea7e8e3e00a6f147a8a77b56e6e88c5e4fe30f.woff \
  tests/fonts/fuzzing/a69118c2c2ada48ff803d9149daa54c9ebdae30e.ttf \
  tests/fonts/fuzzing/a919b33197965846f21074b24e30250d67277bce.ttf \
  tests/fonts/fuzzing/a98e908e2ed21b22228ea59ebcc0f05034c86f2e.ttf \
  tests/fonts/fuzzing/ad24c3e446b9be316db8387adb0c7e2667388d3c.woff \
  tests/fonts/fuzzing/b2093e804590557247aa7bc2070757c26344c565.woff \
  tests/fonts/fuzzing/b3fe94051b5612817c31fd406172d6d98b823cc8.ttf \
  tests/fonts/fuzzing/b6acef662e0beb8d5fcf5b61c6b0ca69537b7402.ttf \
  tests/fonts/fuzzing/b9e2aaa0d75fcef6971ec3a96d806ba4a6b31fe2.otf \
  tests/fonts/fuzzing/b9e2aaa0d75fcef6971ec3a96d806ba4a6b31fe2.ttf \
  tests/fonts/fuzzing/bac8d8c1ad2b8a50bf10853c9aeca4fca6c3bf3e.woff2 \
  tests/fonts/fuzzing/bb0c53752e85c3d28973ebc913287b8987d3dfe8.ttf \
  tests/fonts/fuzzing/bb29ce50df2bdba2d10726427c6b7609bf460e04.ttf \
  tests/fonts/fuzzing/bb9473d2403488714043bcfb946c9f78b86ad627.ttf \
  tests/fonts/fuzzing/bfec49121cbe81f898476c5bcc6d188b489f1d7e.woff \
  tests/fonts/fuzzing/c077be3830a3fe49277220e2515791cc9a2e5060.woff \
  tests/fonts/fuzzing/c455eacf5e313dd37ad87d279f6eba6c3114310f.woff2 \
  tests/fonts/fuzzing/c4e48b0886ef460f532fb49f00047ec92c432ec0.ttf \
  tests/fonts/fuzzing/c64aff5250ea7360a18154e58678ab14a8e6fee9.woff \
  tests/fonts/fuzzing/c8fda8de3123a35984f9f0811c4481fe3a8d3641.woff2 \
  tests/fonts/fuzzing/cc5f3d2d717fb6bd4dfae1c16d48a2cb8e12233b.ttf \
  tests/fonts/fuzzing/ccfc10b31bad1fb3ae508a332c7914c6db037188.woff2 \
  tests/fonts/fuzzing/cff4306f450b3b433adca6872ff1c928a6ede2c6.woff \
  tests/fonts/fuzzing/d32823ca92b0443465c07625325bb64a7d78f47d.woff2 \
  tests/fonts/fuzzing/d343af630ddff2ed15ae72d9bb4949bb384a8e03.woff2 \
  tests/fonts/fuzzing/d3ee8b9422e66a4dc827a15a65667bd5eab9ce5f.woff \
  tests/fonts/fuzzing/d4acbc69d72d4ed528367718f022708070ec9635.woff2 \
  tests/fonts/fuzzing/d4e4a9508c6b9e73c514b8af27b56918f45c3f9e.ttf \
  tests/fonts/fuzzing/d4eaf020145c14e2cd76c49353da1fdff8e4f62f.woff \
  tests/fonts/fuzzing/d629e7fedc0b350222d7987345fe61613fa3929a.ttf \
  tests/fonts/fuzzing/d69ecb7001e46df61c33e2ec74fdcdd32e018db3.ttc \
  tests/fonts/fuzzing/d77f5dd2f02db900f5d568349dc6ed58683a00a9.woff \
  tests/fonts/fuzzing/d8df0f95c805bf3986671a5f39d4e1ce1ef278ce.woff2 \
  tests/fonts/fuzzing/dc2b2ffc9fc5318fea924276dc8298f2b06f26ab.woff2 \
  tests/fonts/fuzzing/dcacf960143aecbc692471b2a658c8dc42c9613e.woff2 \
  tests/fonts/fuzzing/df768b9c257e0c9c35786c47cae15c46571d56be.ttf \
  tests/fonts/fuzzing/dfba97547bf1cc337702d52311dee7029bc51db8.woff \
  tests/fonts/fuzzing/e207635780b42f898d58654b65098763e340f5c7.ttf \
  tests/fonts/fuzzing/e31aa13e4a7b565c68e4822d2ef7d16069f8ffab.woff2 \
  tests/fonts/fuzzing/e88c339237f52d21e01c55f01b9c1b4cc14a0467.ttf \
  tests/fonts/fuzzing/e9e765985a2beed45d8c4c4666d94a2acede9d48.woff2 \
  tests/fonts/fuzzing/ee39587d13b2afa5499cc79e45780aa79293bbd4.ttf \
  tests/fonts/fuzzing/ef86fe710cfea877bbe0dbb6946a1f88d0661031.ttf \
  tests/fonts/fuzzing/f22416c692720a7d46fadf4af99f4c9e094f00b9.ttf \
  tests/fonts/fuzzing/f293a7cc0c9fad957df4904e8ce5e366e7cab93c.woff \
  tests/fonts/fuzzing/f37d5b05405c0861da62bdad2b00fcaa7d2923f9.woff2 \
  tests/fonts/fuzzing/f499fbc23865022234775c43503bba2e63978fe1.ttf \
  tests/fonts/fuzzing/f4bcb76e745d6390bdf0447f2128db19686c432d.woff \
  tests/fonts/fuzzing/f518eb6f6b5eec2946c9fbbbde44e45d46f5e2ac.ttf \
  tests/fonts/fuzzing/f9b1dd4dcb515e757789a22cb4241107746fd3d0.ttf \
  tests/fonts/fuzzing/fab39d60d758cb586db5a504f218442cd1395725.ttf \
  tests/fonts/fuzzing/fbb6c84c9e1fe0c39e152fbe845e51fd81f6748e.ttf \
  tests/fonts/fuzzing/fc4263389f1a48dd3d1c793dff6b1e0cf8bccd33.woff \
  tests/fonts/fuzzing/fcdcffbdf1c4c97c05308d7600e4c283eb47dbca.ttf \
  tests/fonts/fuzzing/ffa0f5d2d9025486d8469d8b1fdd983e7632499b.ttf \
  tests/fonts/good/029c6e2623f5b255ba10e2e3b0f17711ad8aa966.ttf \
  tests/fonts/good/0c4afb23b983bbab65c39869b81ca1dfb90c0258.ttf \
  tests/fonts/good/0e16ec5ab94d3992bba42a9177b159113ede1485.ttf \
  tests/fonts/good/0fc088827bbe36bfb3fb6c3d8b59f66cb234dfb3.ttf \
  tests/fonts/good/1232d0423fe3bb731faa3da008281ca030d3fe0a.woff \
  tests/fonts/good/126e13890b4c36319166a07bb5f4301132e6dcee.ttf \
  tests/fonts/good/15e79042604b100fa6f93d817112b8f4921f6a4b.ttf \
  tests/fonts/good/19d60aa144bb703f0c7535e3b34e926e0cae954d.ttf \
  tests/fonts/good/24834cb0e118b8a80c05209d996963cf05121f43.ttf \
  tests/fonts/good/2f806821ba44e69a532fbcc9d5f46a5aa20fa905.ttf \
  tests/fonts/good/322aa2ac0a3916d3a5cb1e7789ca355de0a6bc76.ttf \
  tests/fonts/good/3b4a0f922a35acba59502ba042f35cafbff1865f.ttf \
  tests/fonts/good/3eb899fa22c7a7e1294f50204804869fddfda8c9.ttf \
  tests/fonts/good/3ee1ab163f0029bdd8f90b79f2c0e798bc26957b.ttf \
  tests/fonts/good/49a9db4b6c08dadd1e8aeff4cee63dc454c5cb7c.ttf \
  tests/fonts/good/4fa1f484ba76ad1951727d8e4a80d57d9b08204b.ttf \
  tests/fonts/good/56e62c1aa63b75291bd6c3bd04f29ee36590341f.ttf \
  tests/fonts/good/6a20e627475700e4b889926fcc423420d6df3033.ttf \
  tests/fonts/good/6a46e85c50dce18db3a5739456933f29442e410e.ttf \
  tests/fonts/good/6ae5423c8ca52f5e20fc4be14d10ae0cd3857be5.ttf \
  tests/fonts/good/6bed910d67d00b0e705b657747af2b83f2edc1c5.ttf \
  tests/fonts/good/6c26e8ccc29afe595364bf649455d10dc0e39861.ttf \
  tests/fonts/good/759f24b9ab9a97980c5bb5649d9f5032fb9e41b0.ttf \
  tests/fonts/good/83de2fc102dbe5e1738710655787d736e51f5b56.ttf \
  tests/fonts/good/877ba38762b32f50468250d8bb659a2af252f44f.ttf \
  tests/fonts/good/9227eefacd215fee911b7c4f935e0bad9bde5772.ttf \
  tests/fonts/good/923963eece37a2b494c7f2b75db4ac24635784bf.ttf \
  tests/fonts/good/942fab6918c0dc9bf59f62ab15e3fd38736bbe8d.ttf \
  tests/fonts/good/94eec826e16eb8d695b13690bb1e704948b98bf9.ttf \
  tests/fonts/good/965fe4ee4b2f4d090ea4cb90507c3eddcf597e1c.ttf \
  tests/fonts/good/9dc7e9523203d97914edf64aff494a1a18762782.ttf \
  tests/fonts/good/a29b6af1965f9cac3b4ada03258bcce6a155b168.ttf \
  tests/fonts/good/b258998a804fe4780216ebd43248436e2d015f03.ttf \
  tests/fonts/good/b3590210e2baa7fb6c58af1ce98d383d8bf47683.ttf \
  tests/fonts/good/b4a99e4fdd041256e354d4ada89bb2900908e78b.ttf \
  tests/fonts/good/b8a18a5eec94d4ab749fbd825e6fa198607704a2.ttf \
  tests/fonts/good/b98ec6ce1630f86eb34d18666002676498618717.ttf \
  tests/fonts/good/baaef388d8b2a70bec6024534974f417119304be.ttf \
  tests/fonts/good/bf5c8e9f890bfbf757589943b18534e6097eeaad.ttf \
  tests/fonts/good/c026cbcefd9e0c9995208ef613c27b48b548ba6f.ttf \
  tests/fonts/good/c1fd802800359304594785d0ea5fb1eadb14666c.ttf \
  tests/fonts/good/c7999e53cde30047922a2f6adb5eb64f4aac2d54.ttf \
  tests/fonts/good/c7d76de613012ce941785c387eb6570d905bc6a7.ttf \
  tests/fonts/good/d0ff0af970457d3a599c5f82f375ceb1faaade3c.ttf \
  tests/fonts/good/d391cf982a114ea43871890da047df4a7b5a087b.ttf \
  tests/fonts/good/d3ae20d9f21e321f0813094dd6970468f187b669.ttf \
  tests/fonts/good/d427ef152e9e6a8b1865ae00c22b9a282d008073.ttf \
  tests/fonts/good/d8ab8092b46ae0e06c310bdd063b9fd65d04fe93.ttf \
  tests/fonts/good/da1c5e82a0257f9998ff45185ffc153fde98fb12.ttf \
  tests/fonts/good/db4b768546934de921667761967706f4f527a75a.ttf \
  tests/fonts/good/e6e8ab2987f8c1f9bf6a8a3f9e58dbd62195c4e6.ttf \
  tests/fonts/good/e9e065f23e8c5c1559cdb76a7a350b857ce2713e.ttf \
  tests/fonts/good/f172aa07083478097b7ba62e485c9ffefcd0d623.ttf \
  tests/fonts/good/f60191da5f12f78d23ec32c6fdf51314ce3bb8c0.ttf \
  tests/fonts/good/fd62f786684b29020b46c40ae4cacfbd044fc7ab.ttf \
  $(NULL)


MAINTAINERCLEANFILES = \
  $(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
  $(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
  $(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
  $(NULL)

-include $(top_srcdir)/git.mk
