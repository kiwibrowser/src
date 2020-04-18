// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/fontconfig_util_linux.h"

#include <fontconfig/fontconfig.h>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace base {

namespace {

const char kFontsConfTemplate[] = R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>

  <!-- Cache location. -->
  <cachedir>$1</cachedir>

  <!-- GCS-synced fonts. -->
  <dir>$2</dir>

  <!-- Default properties. -->
  <match target="font">
    <edit name="embeddedbitmap" mode="append_last">
      <bool>false</bool>
    </edit>
  </match>

  <!-- TODO(thomasanderson): Figure out why this is necessary. -->
  <match target="pattern">
    <test name="family" compare="eq">
      <string>Tinos</string>
    </test>
    <test name="prgname" compare="eq">
      <string>chromevox_tests</string>
    </test>
    <edit name="hintstyle" mode="assign">
      <const>hintslight</const>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Times</string>
    </test>
    <edit name="family" mode="assign">
      <string>Tinos</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>sans</string>
    </test>
    <edit name="family" mode="assign">
      <string>DejaVu Sans</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>sans serif</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
  </match>

  <!-- Some layout tests specify Helvetica as a family and we need to make sure
       that we don't fallback to Tinos for them -->
  <match target="pattern">
    <test qual="any" name="family">
      <string>Helvetica</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>sans-serif</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>serif</string>
    </test>
    <edit name="family" mode="assign">
      <string>Tinos</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>mono</string>
    </test>
    <edit name="family" mode="assign">
      <string>Cousine</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>monospace</string>
    </test>
    <edit name="family" mode="assign">
      <string>Cousine</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Courier</string>
    </test>
    <edit name="family" mode="assign">
      <string>Cousine</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>cursive</string>
    </test>
    <edit name="family" mode="assign">
      <string>Comic Sans MS</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>fantasy</string>
    </test>
    <edit name="family" mode="assign">
      <string>Impact</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Monaco</string>
    </test>
    <edit name="family" mode="assign">
      <string>Tinos</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Arial</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Courier New</string>
    </test>
    <edit name="family" mode="assign">
      <string>Cousine</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Georgia</string>
    </test>
    <edit name="family" mode="assign">
      <string>Gelasio</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Times New Roman</string>
    </test>
    <edit name="family" mode="assign">
      <string>Tinos</string>
    </edit>
  </match>

  <match target="pattern">
    <test qual="any" name="family">
      <string>Verdana</string>
    </test>
    <!-- NOT metrically compatible! -->
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
  </match>

  <!-- TODO(thomasanderson): Move these configs to be test-specific. -->
  <match target="pattern">
    <test name="family" compare="eq">
      <string>NonAntiAliasedSans</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <edit name="antialias" mode="assign">
      <bool>false</bool>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>SlightHintedGeorgia</string>
    </test>
    <edit name="family" mode="assign">
      <string>Gelasio</string>
    </edit>
    <edit name="hintstyle" mode="assign">
      <const>hintslight</const>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>NonHintedSans</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <!-- These deliberately contradict each other. The 'hinting' preference
         should take priority -->
    <edit name="hintstyle" mode="assign">
      <const>hintfull</const>
    </edit>
   <edit name="hinting" mode="assign">
      <bool>false</bool>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>AutohintedSerif</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <edit name="autohint" mode="assign">
      <bool>true</bool>
    </edit>
    <edit name="hintstyle" mode="assign">
      <const>hintmedium</const>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>HintedSerif</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <edit name="autohint" mode="assign">
      <bool>false</bool>
    </edit>
    <edit name="hintstyle" mode="assign">
      <const>hintmedium</const>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>FullAndAutoHintedSerif</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <edit name="autohint" mode="assign">
      <bool>true</bool>
    </edit>
    <edit name="hintstyle" mode="assign">
      <const>hintfull</const>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>SubpixelEnabledArial</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <edit name="rgba" mode="assign">
      <const>rgb</const>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>SubpixelDisabledArial</string>
    </test>
    <edit name="family" mode="assign">
      <string>Arimo</string>
    </edit>
    <edit name="rgba" mode="assign">
      <const>none</const>
    </edit>
  </match>

  <match target="pattern">
    <!-- FontConfig doesn't currently provide a well-defined way to turn on
         subpixel positioning.  This is just an arbitrary pattern to use after
         turning subpixel positioning on globally to ensure that we don't have
         issues with our style getting cached for other tests. -->
    <test name="family" compare="eq">
      <string>SubpixelPositioning</string>
    </test>
    <edit name="family" mode="assign">
      <string>Tinos</string>
    </edit>
  </match>

  <match target="pattern">
    <!-- See comments above -->
    <test name="family" compare="eq">
      <string>SubpixelPositioningAhem</string>
    </test>
    <edit name="family" mode="assign">
      <string>ahem</string>
    </edit>
  </match>

  <match target="pattern">
    <test name="family" compare="eq">
      <string>SlightHintedTimesNewRoman</string>
    </test>
    <edit name="family" mode="assign">
      <string>Tinos</string>
    </edit>
    <edit name="hintstyle" mode="assign">
      <const>hintslight</const>
    </edit>
  </match>

  <!-- When we encounter a character that the current font doesn't
       support, gfx::GetFallbackFontForChar() returns the first font
       that does have a glyph for the character. The list of fonts is
       sorted by a pattern that includes the current locale, but doesn't
       include a font family (which means that the fallback font depends
       on the locale but not on the current font).

       DejaVu Sans is commonly the only font that supports some
       characters, such as "⇧", and even when other candidates are
       available, DejaVu Sans is commonly first among them, because of
       the way Fontconfig is ordinarily configured. For example, the
       configuration in the Fonconfig source lists DejaVu Sans under the
       sans-serif generic family, and appends sans-serif to patterns
       that don't already include a generic family (such as the pattern
       in gfx::GetFallbackFontForChar()).

       To get the same fallback font in the layout tests, we could
       duplicate this configuration here, or more directly, simply
       append DejaVu Sans to all patterns. -->
  <match target="pattern">
    <edit name="family" mode="append_last">
      <string>DejaVu Sans</string>
    </edit>
  </match>

</fontconfig>
)";

}  // namespace

void SetUpFontconfig() {
  FilePath dir_module;
  PathService::Get(DIR_MODULE, &dir_module);
  FilePath font_cache = dir_module.Append("fontconfig_caches");
  FilePath test_fonts = dir_module.Append("test_fonts");
  std::string fonts_conf = ReplaceStringPlaceholders(
      kFontsConfTemplate, {font_cache.value(), test_fonts.value()}, nullptr);

  FcConfig* config = FcConfigCreate();
  CHECK(config);
#if FC_VERSION >= 21205
  CHECK(FcConfigParseAndLoadFromMemory(
      config, reinterpret_cast<const FcChar8*>(fonts_conf.c_str()), FcTrue));
#else
  FilePath temp;
  CHECK(CreateTemporaryFile(&temp));
  CHECK(WriteFile(temp, fonts_conf.c_str(), fonts_conf.size()));
  CHECK(FcConfigParseAndLoad(
      config, reinterpret_cast<const FcChar8*>(temp.value().c_str()), FcTrue));
  CHECK(DeleteFile(temp, false));
#endif
  CHECK(FcConfigBuildFonts(config));
  CHECK(FcConfigSetCurrent(config));

  // Decrement the reference count for |config|.  It's now owned by fontconfig.
  FcConfigDestroy(config);
}

void TearDownFontconfig() {
  FcFini();
}

bool LoadConfigFileIntoFontconfig(const FilePath& path) {
  // Unlike other FcConfig functions, FcConfigParseAndLoad() doesn't default to
  // the current config when passed NULL. So that's cool.
  if (!FcConfigParseAndLoad(
          FcConfigGetCurrent(),
          reinterpret_cast<const FcChar8*>(path.value().c_str()), FcTrue)) {
    LOG(ERROR) << "Fontconfig failed to load " << path.value();
    return false;
  }
  return true;
}

bool LoadConfigDataIntoFontconfig(const FilePath& temp_dir,
                                  const std::string& data) {
  FilePath path;
  if (!CreateTemporaryFileInDir(temp_dir, &path)) {
    PLOG(ERROR) << "Unable to create temporary file in " << temp_dir.value();
    return false;
  }
  if (WriteFile(path, data.data(), data.size()) !=
      static_cast<int>(data.size())) {
    PLOG(ERROR) << "Unable to write config data to " << path.value();
    return false;
  }
  return LoadConfigFileIntoFontconfig(path);
}

std::string CreateFontconfigEditStanza(const std::string& name,
                                       const std::string& type,
                                       const std::string& value) {
  return StringPrintf(
      "    <edit name=\"%s\" mode=\"assign\">\n"
      "      <%s>%s</%s>\n"
      "    </edit>\n",
      name.c_str(), type.c_str(), value.c_str(), type.c_str());
}

std::string CreateFontconfigTestStanza(const std::string& name,
                                       const std::string& op,
                                       const std::string& type,
                                       const std::string& value) {
  return StringPrintf(
      "    <test name=\"%s\" compare=\"%s\" qual=\"any\">\n"
      "      <%s>%s</%s>\n"
      "    </test>\n",
      name.c_str(), op.c_str(), type.c_str(), value.c_str(), type.c_str());
}

std::string CreateFontconfigAliasStanza(const std::string& original_family,
                                        const std::string& preferred_family) {
  return StringPrintf(
      "  <alias>\n"
      "    <family>%s</family>\n"
      "    <prefer><family>%s</family></prefer>\n"
      "  </alias>\n",
      original_family.c_str(), preferred_family.c_str());
}

}  // namespace base
