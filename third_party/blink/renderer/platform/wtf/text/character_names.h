/*
 * Copyright (C) 2007, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_TEXT_CHARACTER_NAMES_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_TEXT_CHARACTER_NAMES_H_

#include "third_party/blink/renderer/platform/wtf/text/unicode.h"

namespace WTF {
namespace Unicode {

// Names here are taken from the Unicode standard.

// Most of these are UChar constants, not UChar32, which makes them
// more convenient for WebCore code that mostly uses UTF-16.

const UChar kActivateArabicFormShapingCharacter = 0x206D;
const UChar kActivateSymmetricSwappingCharacter = 0x206B;
const UChar32 kAegeanWordSeparatorLineCharacter = 0x10100;
const UChar32 kAegeanWordSeparatorDotCharacter = 0x10101;
const UChar kArabicLetterMarkCharacter = 0x061C;
const UChar kBlackCircleCharacter = 0x25CF;
const UChar kBlackSquareCharacter = 0x25A0;
const UChar kBlackUpPointingTriangleCharacter = 0x25B2;
const UChar kBulletCharacter = 0x2022;
const UChar kBullseyeCharacter = 0x25CE;
const UChar32 kCancelTag = 0xE007F;
const UChar kCarriageReturnCharacter = 0x000D;
const UChar kCombiningEnclosingCircleBackslashCharacter = 0x20E0;
const UChar kCombiningEnclosingKeycapCharacter = 0x20E3;
const UChar kDeleteCharacter = 0x007F;
const UChar kEthiopicPrefaceColonCharacter = 0x1366;
const UChar kEthiopicWordspaceCharacter = 0x1361;
const UChar kHeavyBlackHeartCharacter = 0x2764;
const UChar32 kEyeCharacter = 0x1F441;
const UChar32 kBoyCharacter = 0x1F466;
const UChar32 kGirlCharacter = 0x1F467;
const UChar32 kManCharacter = 0x1F468;
const UChar32 kWomanCharacter = 0x1F469;
const UChar32 kKissMarkCharacter = 0x1F48B;
const UChar32 kFamilyCharacter = 0x1F46A;
const UChar kFemaleSignCharacter = 0x2640;
const UChar kFirstStrongIsolateCharacter = 0x2068;
const UChar kFisheyeCharacter = 0x25C9;
const UChar kFullstopCharacter = 0x002E;
const UChar kHebrewPunctuationGereshCharacter = 0x05F3;
const UChar kHebrewPunctuationGershayimCharacter = 0x05F4;
const UChar kHiraganaLetterSmallACharacter = 0x3041;
const UChar kHorizontalEllipsisCharacter = 0x2026;
const UChar kHyphenCharacter = 0x2010;
const UChar kHyphenMinusCharacter = 0x002D;
const UChar kIdeographicCommaCharacter = 0x3001;
const UChar kIdeographicFullStopCharacter = 0x3002;
#if defined(USING_SYSTEM_ICU)
const UChar ideographicSpaceCharacter = 0x3000;
#endif
const UChar kInhibitArabicFormShapingCharacter = 0x206C;
const UChar kInhibitSymmetricSwappingCharacter = 0x206A;
const UChar kLatinCapitalLetterIWithDotAbove = 0x0130;
const UChar kLatinSmallLetterDotlessI = 0x0131;
const UChar kLeftDoubleQuotationMarkCharacter = 0x201C;
const UChar kLeftSingleQuotationMarkCharacter = 0x2018;
const UChar32 kLeftSpeechBubbleCharacter = 0x1F5E8;
const UChar kLeftToRightEmbedCharacter = 0x202A;
const UChar kLeftToRightIsolateCharacter = 0x2066;
const UChar kLeftToRightMarkCharacter = 0x200E;
const UChar kLeftToRightOverrideCharacter = 0x202D;
const UChar kLineSeparator = 0x2028;
const UChar kLineTabulationCharacter = 0x000B;
const UChar kLowLineCharacter = 0x005F;
const UChar kMaleSignCharacter = 0x2642;
const UChar kMinusSignCharacter = 0x2212;
const UChar kNewlineCharacter = 0x000A;
const UChar kFormFeedCharacter = 0x000C;
const UChar kNationalDigitShapesCharacter = 0x206E;
const UChar kNominalDigitShapesCharacter = 0x206F;
const UChar kNoBreakSpaceCharacter = 0x00A0;
const UChar kObjectReplacementCharacter = 0xFFFC;
const UChar kParagraphSeparator = 0x2029;
const UChar kPopDirectionalFormattingCharacter = 0x202C;
const UChar kPopDirectionalIsolateCharacter = 0x2069;
const UChar32 kRainbowCharacter = 0x1F308;
const UChar kReplacementCharacter = 0xFFFD;
const UChar kReverseSolidusCharacter = 0x005C;
const UChar kRightDoubleQuotationMarkCharacter = 0x201D;
const UChar kRightSingleQuotationMarkCharacter = 0x2019;
const UChar kRightToLeftEmbedCharacter = 0x202B;
const UChar kRightToLeftIsolateCharacter = 0x2067;
const UChar kRightToLeftMarkCharacter = 0x200F;
const UChar kRightToLeftOverrideCharacter = 0x202E;
const UChar kSesameDotCharacter = 0xFE45;
const UChar kSmallLetterSharpSCharacter = 0x00DF;
const UChar kSolidusCharacter = 0x002F;
const UChar kSoftHyphenCharacter = 0x00AD;
const UChar kSpaceCharacter = 0x0020;
const UChar kStaffOfAesculapiusCharacter = 0x2695;
const UChar kTabulationCharacter = 0x0009;
const UChar32 kTagDigitZero = 0xE0030;
const UChar32 kTagDigitNine = 0xE0039;
const UChar32 kTagLatinSmallLetterA = 0xE0061;
const UChar32 kTagLatinSmallLetterZ = 0xE007A;
const UChar kTibetanMarkIntersyllabicTshegCharacter = 0x0F0B;
const UChar kTibetanMarkDelimiterTshegBstarCharacter = 0x0F0C;
const UChar32 kUgariticWordDividerCharacter = 0x1039F;
const UChar kVariationSelector15Character = 0xFE0E;
const UChar kVariationSelector16Character = 0xFE0F;
const UChar32 kWavingWhiteFlagCharacter = 0x1F3F3;
const UChar kWhiteBulletCharacter = 0x25E6;
const UChar kWhiteCircleCharacter = 0x25CB;
const UChar kWhiteSesameDotCharacter = 0xFE46;
const UChar kWhiteUpPointingTriangleCharacter = 0x25B3;
const UChar kYenSignCharacter = 0x00A5;
const UChar kZeroWidthJoinerCharacter = 0x200D;
const UChar kZeroWidthNonJoinerCharacter = 0x200C;
const UChar kZeroWidthSpaceCharacter = 0x200B;
const UChar kZeroWidthNoBreakSpaceCharacter = 0xFEFF;
const UChar32 kMaxCodepoint = 0x10ffff;

}  // namespace Unicode
}  // namespace WTF

using WTF::Unicode::kAegeanWordSeparatorLineCharacter;
using WTF::Unicode::kAegeanWordSeparatorDotCharacter;
using WTF::Unicode::kActivateArabicFormShapingCharacter;
using WTF::Unicode::kActivateSymmetricSwappingCharacter;
using WTF::Unicode::kArabicLetterMarkCharacter;
using WTF::Unicode::kBlackCircleCharacter;
using WTF::Unicode::kBlackSquareCharacter;
using WTF::Unicode::kBlackUpPointingTriangleCharacter;
using WTF::Unicode::kBulletCharacter;
using WTF::Unicode::kBullseyeCharacter;
using WTF::Unicode::kCancelTag;
using WTF::Unicode::kCarriageReturnCharacter;
using WTF::Unicode::kCombiningEnclosingKeycapCharacter;
using WTF::Unicode::kCombiningEnclosingCircleBackslashCharacter;
using WTF::Unicode::kEthiopicPrefaceColonCharacter;
using WTF::Unicode::kEthiopicWordspaceCharacter;
using WTF::Unicode::kEyeCharacter;
using WTF::Unicode::kFamilyCharacter;
using WTF::Unicode::kFemaleSignCharacter;
using WTF::Unicode::kFirstStrongIsolateCharacter;
using WTF::Unicode::kFisheyeCharacter;
using WTF::Unicode::kFormFeedCharacter;
using WTF::Unicode::kFullstopCharacter;
using WTF::Unicode::kHebrewPunctuationGereshCharacter;
using WTF::Unicode::kHebrewPunctuationGershayimCharacter;
using WTF::Unicode::kHiraganaLetterSmallACharacter;
using WTF::Unicode::kHorizontalEllipsisCharacter;
using WTF::Unicode::kHyphenCharacter;
using WTF::Unicode::kHyphenMinusCharacter;
using WTF::Unicode::kIdeographicCommaCharacter;
using WTF::Unicode::kIdeographicFullStopCharacter;
#if defined(USING_SYSTEM_ICU)
using WTF::Unicode::ideographicSpaceCharacter;
#endif
using WTF::Unicode::kInhibitArabicFormShapingCharacter;
using WTF::Unicode::kInhibitSymmetricSwappingCharacter;
using WTF::Unicode::kLatinCapitalLetterIWithDotAbove;
using WTF::Unicode::kLatinSmallLetterDotlessI;
using WTF::Unicode::kLeftDoubleQuotationMarkCharacter;
using WTF::Unicode::kLeftSingleQuotationMarkCharacter;
using WTF::Unicode::kLeftSpeechBubbleCharacter;
using WTF::Unicode::kLeftToRightEmbedCharacter;
using WTF::Unicode::kLeftToRightIsolateCharacter;
using WTF::Unicode::kLeftToRightMarkCharacter;
using WTF::Unicode::kLeftToRightOverrideCharacter;
using WTF::Unicode::kLineSeparator;
using WTF::Unicode::kLowLineCharacter;
using WTF::Unicode::kMaleSignCharacter;
using WTF::Unicode::kMinusSignCharacter;
using WTF::Unicode::kNewlineCharacter;
using WTF::Unicode::kNationalDigitShapesCharacter;
using WTF::Unicode::kNominalDigitShapesCharacter;
using WTF::Unicode::kNoBreakSpaceCharacter;
using WTF::Unicode::kObjectReplacementCharacter;
using WTF::Unicode::kParagraphSeparator;
using WTF::Unicode::kPopDirectionalFormattingCharacter;
using WTF::Unicode::kPopDirectionalIsolateCharacter;
using WTF::Unicode::kRainbowCharacter;
using WTF::Unicode::kReplacementCharacter;
using WTF::Unicode::kReverseSolidusCharacter;
using WTF::Unicode::kRightDoubleQuotationMarkCharacter;
using WTF::Unicode::kRightSingleQuotationMarkCharacter;
using WTF::Unicode::kRightToLeftEmbedCharacter;
using WTF::Unicode::kRightToLeftIsolateCharacter;
using WTF::Unicode::kRightToLeftMarkCharacter;
using WTF::Unicode::kRightToLeftOverrideCharacter;
using WTF::Unicode::kSesameDotCharacter;
using WTF::Unicode::kSmallLetterSharpSCharacter;
using WTF::Unicode::kSoftHyphenCharacter;
using WTF::Unicode::kSolidusCharacter;
using WTF::Unicode::kSpaceCharacter;
using WTF::Unicode::kStaffOfAesculapiusCharacter;
using WTF::Unicode::kTabulationCharacter;
using WTF::Unicode::kTagDigitZero;
using WTF::Unicode::kTagDigitNine;
using WTF::Unicode::kTagLatinSmallLetterA;
using WTF::Unicode::kTagLatinSmallLetterZ;
using WTF::Unicode::kTibetanMarkIntersyllabicTshegCharacter;
using WTF::Unicode::kTibetanMarkDelimiterTshegBstarCharacter;
using WTF::Unicode::kUgariticWordDividerCharacter;
using WTF::Unicode::kVariationSelector15Character;
using WTF::Unicode::kVariationSelector16Character;
using WTF::Unicode::kWavingWhiteFlagCharacter;
using WTF::Unicode::kWhiteBulletCharacter;
using WTF::Unicode::kWhiteCircleCharacter;
using WTF::Unicode::kWhiteSesameDotCharacter;
using WTF::Unicode::kWhiteUpPointingTriangleCharacter;
using WTF::Unicode::kYenSignCharacter;
using WTF::Unicode::kZeroWidthJoinerCharacter;
using WTF::Unicode::kZeroWidthNonJoinerCharacter;
using WTF::Unicode::kZeroWidthSpaceCharacter;
using WTF::Unicode::kZeroWidthNoBreakSpaceCharacter;
using WTF::Unicode::kMaxCodepoint;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_TEXT_CHARACTER_NAMES_H_
