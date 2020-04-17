# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])  # FreeType License, zlib

# We can't just glob *.c, since Freetype has .c files that include other .c
# files all over the place.  In order to simplify the process of listing them
# manually, we patch out the non-TrueType drivers from the configuration file,
# since the only font we ship is TrueType.
sources = [
    "src/autofit/afangles.c",
    "src/autofit/afblue.c",
    "src/autofit/afcjk.c",
    "src/autofit/afdummy.c",
    "src/autofit/afglobal.c",
    "src/autofit/afhints.c",
    "src/autofit/afindic.c",
    "src/autofit/aflatin.c",
    "src/autofit/afloader.c",
    "src/autofit/afmodule.c",
    "src/autofit/afpic.c",
    "src/autofit/afranges.c",
    "src/autofit/afshaper.c",
    "src/autofit/afwarp.c",
    "src/base/basepic.c",
    "src/base/ftadvanc.c",
    "src/base/ftapi.c",
    "src/base/ftbbox.c",
    "src/base/ftbdf.c",
    "src/base/ftbitmap.c",
    "src/base/ftcalc.c",
    "src/base/ftcid.c",
    "src/base/ftdbgmem.c",
    "src/base/ftdebug.c",
    "src/base/ftfntfmt.c",
    "src/base/ftfstype.c",
    "src/base/ftgasp.c",
    "src/base/ftgloadr.c",
    "src/base/ftglyph.c",
    "src/base/ftgxval.c",
    "src/base/fthash.c",
    "src/base/ftinit.c",
    "src/base/ftlcdfil.c",
    "src/base/ftmm.c",
    "src/base/ftobjs.c",
    "src/base/ftotval.c",
    "src/base/ftoutln.c",
    "src/base/ftpatent.c",
    "src/base/ftpfr.c",
    "src/base/ftpic.c",
    "src/base/ftrfork.c",
    "src/base/ftsnames.c",
    "src/base/ftstream.c",
    "src/base/ftstroke.c",
    "src/base/ftsynth.c",
    "src/base/ftsystem.c",
    "src/base/fttrigon.c",
    "src/base/fttype1.c",
    "src/base/ftutil.c",
    "src/base/ftwinfnt.c",
    "src/gzip/ftgzip.c",
    "src/raster/ftraster.c",
    "src/raster/ftrend1.c",
    "src/raster/rastpic.c",
    "src/sfnt/pngshim.c",
    "src/sfnt/sfdriver.c",
    "src/sfnt/sfntpic.c",
    "src/sfnt/sfobjs.c",
    "src/sfnt/ttbdf.c",
    "src/sfnt/ttcmap.c",
    "src/sfnt/ttkern.c",
    "src/sfnt/ttload.c",
    "src/sfnt/ttmtx.c",
    "src/sfnt/ttpost.c",
    "src/sfnt/ttsbit.c",
    "src/smooth/ftgrays.c",
    "src/smooth/ftsmooth.c",
    "src/smooth/ftspic.c",
    "src/truetype/ttdriver.c",
    "src/truetype/ttgload.c",
    "src/truetype/ttgxvar.c",
    "src/truetype/ttinterp.c",
    "src/truetype/ttobjs.c",
    "src/truetype/ttpic.c",
    "src/truetype/ttpload.c",
    "src/truetype/ttsubpix.c",
]

cc_library(
    name = "freetype",
    srcs = sources,
    hdrs = glob([
        "src/**/*.h",
        "include/**/*.h",
    ]),
    copts = ["-DFT2_BUILD_LIBRARY", "-UDEBUG"],
    includes = ["include"],
    textual_hdrs = glob(["src/**/*.c"]),
    visibility = ["//visibility:public"],
)
