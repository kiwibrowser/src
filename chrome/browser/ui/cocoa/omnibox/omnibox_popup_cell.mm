// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_cell.h"

#include <stddef.h>

#include <algorithm>
#include <cmath>

#include "base/i18n/rtl.h"
#include "base/mac/foundation_util.h"
#include "base/mac/objc_release_properties.h"
#include "base/mac/scoped_nsobject.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_popup_view_mac.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"

namespace {

// Extra padding beyond the vertical text padding.
constexpr CGFloat kMaterialExtraVerticalImagePadding = 2.0;

constexpr CGFloat kMaterialTextStartOffset = 27.0;

constexpr CGFloat kMaterialImageXOffset = 6.0;

constexpr CGFloat kDefaultVerticalMargin = 3.0;

constexpr CGFloat kDefaultTextHeight = 19;

// Returns the margin that should appear at the top and bottom of the result.
CGFloat GetVerticalMargin() {
  return base::GetFieldTrialParamByFeatureAsInt(
      omnibox::kUIExperimentVerticalMargin,
      OmniboxFieldTrial::kUIVerticalMarginParam, kDefaultVerticalMargin);
}

// Flips the given |rect| in context of the given |frame|.
NSRect FlipIfRTL(NSRect rect, NSRect frame) {
  DCHECK_LE(NSMinX(frame), NSMinX(rect));
  DCHECK_GE(NSMaxX(frame), NSMaxX(rect));
  if (base::i18n::IsRTL()) {
    NSRect result = rect;
    result.origin.x = NSMinX(frame) + (NSMaxX(frame) - NSMaxX(rect));
    return result;
  }
  return rect;
}

NSColor* SelectedBackgroundColor(BOOL is_dark_theme) {
  return is_dark_theme
             ? skia::SkColorToSRGBNSColor(SkColorSetA(SK_ColorWHITE, 0x14))
             : skia::SkColorToSRGBNSColor(SkColorSetA(SK_ColorBLACK, 0x14));
}

NSColor* HoveredBackgroundColor(BOOL is_dark_theme) {
  return is_dark_theme
             ? skia::SkColorToSRGBNSColor(SkColorSetA(SK_ColorWHITE, 0x0D))
             : [NSColor controlHighlightColor];
}

NSColor* ContentTextColor(BOOL is_dark_theme) {
  return is_dark_theme ? [NSColor whiteColor] : [NSColor blackColor];
}
NSColor* DimTextColor(BOOL is_dark_theme) {
  return is_dark_theme
             ? skia::SkColorToSRGBNSColor(SkColorSetA(SK_ColorWHITE, 0x7F))
             : skia::SkColorToSRGBNSColor(SkColorSetRGB(0x64, 0x64, 0x64));
}
NSColor* InvisibleTextColor() {
  return skia::SkColorToSRGBNSColor(SK_ColorTRANSPARENT);
}
NSColor* PositiveTextColor() {
  return skia::SkColorToSRGBNSColor(SkColorSetRGB(0x3d, 0x94, 0x00));
}
NSColor* NegativeTextColor() {
  return skia::SkColorToSRGBNSColor(SkColorSetRGB(0xdd, 0x4b, 0x39));
}
NSColor* URLTextColor(BOOL is_dark_theme) {
  return is_dark_theme ? skia::SkColorToSRGBNSColor(gfx::kGoogleBlue300)
                       : skia::SkColorToSRGBNSColor(gfx::kGoogleBlue700);
}

NSFont* FieldFont() {
  return OmniboxViewMac::GetNormalFieldFont();
}
NSFont* BoldFieldFont() {
  return OmniboxViewMac::GetBoldFieldFont();
}
NSFont* LargeFont() {
  return OmniboxViewMac::GetLargeFont();
}
NSFont* LargeSuperscriptFont() {
  NSFont* font = OmniboxViewMac::GetLargeFont();
  // Calculate a slightly smaller font. The ratio here is somewhat arbitrary.
  // Proportions from 5/9 to 5/7 all look pretty good.
  CGFloat size = [font pointSize] * 5.0 / 9.0;
  NSFontDescriptor* descriptor = [font fontDescriptor];
  return [NSFont fontWithDescriptor:descriptor size:size];
}

// Sets the writing direction to |direction| for a given |range| of
// |attributedString|.
void SetTextDirectionForRange(NSMutableAttributedString* attributedString,
                              NSWritingDirection direction,
                              NSRange range) {
  [attributedString
      enumerateAttribute:NSParagraphStyleAttributeName
                 inRange:range
                 options:0
              usingBlock:^(id paragraph_style, NSRange range, BOOL* stop) {
                [paragraph_style setBaseWritingDirection:direction];
              }];
}

NSAttributedString* CreateAnswerStringHelper(const base::string16& text,
                                             NSInteger style_type,
                                             bool is_bold,
                                             BOOL is_dark_theme) {
  NSDictionary* answer_style = nil;
  NSFont* answer_font = nil;
  switch (style_type) {
    case SuggestionAnswer::TOP_ALIGNED:
      answer_style = @{
        NSForegroundColorAttributeName : DimTextColor(is_dark_theme),
        NSFontAttributeName : LargeSuperscriptFont(),
        NSSuperscriptAttributeName : @1
      };
      break;
    case SuggestionAnswer::DESCRIPTION_NEGATIVE:
      answer_style = @{
        NSForegroundColorAttributeName : NegativeTextColor(),
        NSFontAttributeName : LargeSuperscriptFont()
      };
      break;
    case SuggestionAnswer::DESCRIPTION_POSITIVE:
      answer_style = @{
        NSForegroundColorAttributeName : PositiveTextColor(),
        NSFontAttributeName : LargeSuperscriptFont()
      };
      break;
    case SuggestionAnswer::PERSONALIZED_SUGGESTION:
      answer_style = @{
        NSForegroundColorAttributeName : ContentTextColor(is_dark_theme),
        NSFontAttributeName : FieldFont()
      };
      break;
    case SuggestionAnswer::ANSWER_TEXT_MEDIUM:
      answer_style = @{
        NSForegroundColorAttributeName : DimTextColor(is_dark_theme),
        NSFontAttributeName : FieldFont()
      };
      break;
    case SuggestionAnswer::ANSWER_TEXT_LARGE:
      answer_style = @{
        NSForegroundColorAttributeName : DimTextColor(is_dark_theme),
        NSFontAttributeName : LargeFont()
      };
      break;
    case SuggestionAnswer::SUGGESTION_SECONDARY_TEXT_SMALL:
      answer_font = FieldFont();
      answer_style = @{
        NSForegroundColorAttributeName : DimTextColor(is_dark_theme),
        NSFontAttributeName : answer_font
      };
      break;
    case SuggestionAnswer::SUGGESTION_SECONDARY_TEXT_MEDIUM:
      answer_font = LargeSuperscriptFont();
      answer_style = @{
        NSForegroundColorAttributeName : DimTextColor(is_dark_theme),
        NSFontAttributeName : answer_font
      };
      break;
    case SuggestionAnswer::SUGGESTION:  // Fall through.
    default:
      answer_style = @{
        NSForegroundColorAttributeName : ContentTextColor(is_dark_theme),
        NSFontAttributeName : FieldFont()
      };
      break;
  }

  if (is_bold) {
    NSMutableDictionary* bold_style = [answer_style mutableCopy];
    // TODO(dschuyler): Account for bolding fonts other than FieldFont.
    // Field font is the only one currently necessary to bold.
    [bold_style setObject:BoldFieldFont() forKey:NSFontAttributeName];
    answer_style = bold_style;
  }

  return [[[NSAttributedString alloc]
      initWithString:base::SysUTF16ToNSString(text)
          attributes:answer_style] autorelease];
}

NSAttributedString* CreateAnswerString(const base::string16& text,
                                       NSInteger style_type,
                                       BOOL is_dark_theme) {
  // TODO(dschuyler): make this better.  Right now this only supports unnested
  // bold tags.  In the future we'll need to flag unexpected tags while adding
  // support for b, i, u, sub, and sup.  We'll also need to support HTML
  // entities (&lt; for '<', etc.).
  const base::string16 begin_tag = base::ASCIIToUTF16("<b>");
  const base::string16 end_tag = base::ASCIIToUTF16("</b>");
  size_t begin = 0;
  base::scoped_nsobject<NSMutableAttributedString> result(
      [[NSMutableAttributedString alloc] init]);
  while (true) {
    size_t end = text.find(begin_tag, begin);
    if (end == base::string16::npos) {
      [result appendAttributedString:CreateAnswerStringHelper(
                                         text.substr(begin), style_type, false,
                                         is_dark_theme)];
      break;
    }
    [result appendAttributedString:CreateAnswerStringHelper(
                                       text.substr(begin, end - begin),
                                       style_type, false, is_dark_theme)];
    begin = end + begin_tag.length();
    end = text.find(end_tag, begin);
    if (end == base::string16::npos)
      break;
    [result appendAttributedString:CreateAnswerStringHelper(
                                       text.substr(begin, end - begin),
                                       style_type, true, is_dark_theme)];
    begin = end + end_tag.length();
  }
  return result.autorelease();
}

NSAttributedString* CreateAnswerLine(const SuggestionAnswer::ImageLine& line,
                                     BOOL is_dark_theme) {
  base::scoped_nsobject<NSMutableAttributedString> answer_string(
      [[NSMutableAttributedString alloc] init]);
  DCHECK(!line.text_fields().empty());
  for (const SuggestionAnswer::TextField& text_field : line.text_fields()) {
    [answer_string appendAttributedString:CreateAnswerString(text_field.text(),
                                                             text_field.type(),
                                                             is_dark_theme)];
  }
  const base::string16 space(base::ASCIIToUTF16(" "));
  const SuggestionAnswer::TextField* text_field = line.additional_text();
  if (text_field) {
    [answer_string
        appendAttributedString:CreateAnswerString(space + text_field->text(),
                                                  text_field->type(),
                                                  is_dark_theme)];
  }
  text_field = line.status_text();
  if (text_field) {
    [answer_string
        appendAttributedString:CreateAnswerString(space + text_field->text(),
                                                  text_field->type(),
                                                  is_dark_theme)];
  }
  base::scoped_nsobject<NSMutableParagraphStyle> style(
      [[NSMutableParagraphStyle alloc] init]);
  [style setTighteningFactorForTruncation:0.0];
  [answer_string addAttribute:NSParagraphStyleAttributeName
                            value:style
                            range:NSMakeRange(0, [answer_string length])];
  return answer_string.autorelease();
}

NSMutableAttributedString* CreateAttributedString(
    const base::string16& text,
    NSColor* text_color,
    NSTextAlignment textAlignment) {
  // Start out with a string using the default style info.
  NSString* s = base::SysUTF16ToNSString(text);
  NSDictionary* attributes = @{
      NSFontAttributeName : FieldFont(),
      NSForegroundColorAttributeName : text_color
  };
  NSMutableAttributedString* attributedString = [[
      [NSMutableAttributedString alloc] initWithString:s
                                            attributes:attributes] autorelease];

  NSMutableParagraphStyle* style =
      [[[NSMutableParagraphStyle alloc] init] autorelease];
  [style setTighteningFactorForTruncation:0.0];
  [style setAlignment:textAlignment];
  if (@available(macOS 10.11, *))
    [style setAllowsDefaultTighteningForTruncation:NO];
  [attributedString addAttribute:NSParagraphStyleAttributeName
                           value:style
                           range:NSMakeRange(0, [attributedString length])];

  return attributedString;
}

NSMutableAttributedString* CreateAttributedString(
    const base::string16& text,
    NSColor* text_color) {
  return CreateAttributedString(text, text_color, NSNaturalTextAlignment);
}

NSAttributedString* CreateClassifiedAttributedString(
    const base::string16& text,
    NSColor* text_color,
    const ACMatchClassifications& classifications,
    BOOL is_dark_theme) {
  NSMutableAttributedString* attributedString =
      CreateAttributedString(text, text_color);
  NSUInteger match_length = [attributedString length];

  // Mark up the runs which differ from the default.
  for (ACMatchClassifications::const_iterator i = classifications.begin();
       i != classifications.end(); ++i) {
    const bool is_last = ((i + 1) == classifications.end());
    const NSUInteger next_offset =
        (is_last ? match_length : static_cast<NSUInteger>((i + 1)->offset));
    const NSUInteger location = static_cast<NSUInteger>(i->offset);
    const NSUInteger length = next_offset - static_cast<NSUInteger>(i->offset);
    // Guard against bad, off-the-end classification ranges.
    if (location >= match_length || length <= 0)
      break;
    const NSRange range =
        NSMakeRange(location, std::min(length, match_length - location));

    if (0 != (i->style & ACMatchClassification::MATCH)) {
      [attributedString addAttribute:NSFontAttributeName
                               value:BoldFieldFont()
                               range:range];
    }

    if (0 != (i->style & ACMatchClassification::URL)) {
      // URLs have their text direction set to to LTR (avoids RTL characters
      // making the URL render from right to left, as per RFC 3987 Section 4.1).
      SetTextDirectionForRange(attributedString, NSWritingDirectionLeftToRight,
                               range);
      [attributedString addAttribute:NSForegroundColorAttributeName
                               value:URLTextColor(is_dark_theme)
                               range:range];
    } else if (0 != (i->style & ACMatchClassification::DIM)) {
      [attributedString addAttribute:NSForegroundColorAttributeName
                               value:DimTextColor(is_dark_theme)
                               range:range];
    } else if (0 != (i->style & ACMatchClassification::INVISIBLE)) {
      [attributedString addAttribute:NSForegroundColorAttributeName
                               value:InvisibleTextColor()
                               range:range];
    }
  }

  return attributedString;
}

}  // namespace

@interface OmniboxPopupCellData ()
@end

@interface OmniboxPopupCell ()
- (CGFloat)drawMatchPart:(NSAttributedString*)attributedString
               withFrame:(NSRect)cellFrame
                  origin:(NSPoint)origin
            withMaxWidth:(int)maxWidth
            forDarkTheme:(BOOL)isDarkTheme
           withHeightCap:(BOOL)hasHeightCap;
- (void)drawMatchWithFrame:(NSRect)cellFrame inView:(NSView*)controlView;
@end

@implementation OmniboxPopupCellData

@synthesize contents = contents_;
@synthesize description = description_;
@synthesize prefix = prefix_;
@synthesize image = image_;
@synthesize answerImage = answerImage_;
@synthesize isContentsRTL = isContentsRTL_;
@synthesize isAnswer = isAnswer_;
@synthesize matchType = matchType_;
@synthesize maxLines = maxLines_;

- (instancetype)initWithMatch:(const AutocompleteMatch&)matchFromModel
                        image:(NSImage*)image
                  answerImage:(NSImage*)answerImage
                 forDarkTheme:(BOOL)isDarkTheme {
  if ((self = [super init])) {
    image_ = [image retain];
    answerImage_ = [answerImage retain];

    AutocompleteMatch match =
        matchFromModel.GetMatchWithContentsAndDescriptionPossiblySwapped();

    isContentsRTL_ =
        (base::i18n::RIGHT_TO_LEFT ==
         base::i18n::GetFirstStrongCharacterDirection(match.contents));
    matchType_ = match.type;

    // Prefix may not have any characters with strong directionality, and may
    // take the UI directionality. But prefix needs to appear in continuation
    // of the contents so we force the directionality.
    NSTextAlignment textAlignment =
        isContentsRTL_ ? NSRightTextAlignment : NSLeftTextAlignment;
    prefix_ = [CreateAttributedString(
        base::UTF8ToUTF16(
            match.GetAdditionalInfo(kACMatchPropertyContentsPrefix)),
        ContentTextColor(isDarkTheme), textAlignment) retain];

    isAnswer_ = !!match.answer;
    if (isAnswer_) {
      contents_ =
          [CreateAnswerLine(match.answer->first_line(), isDarkTheme) retain];
      description_ =
          [CreateAnswerLine(match.answer->second_line(), isDarkTheme) retain];
      maxLines_ = match.answer->second_line().num_text_lines();
    } else {
      contents_ = [CreateClassifiedAttributedString(
          match.contents, ContentTextColor(isDarkTheme), match.contents_class,
          isDarkTheme) retain];
      if (!match.description.empty()) {
        description_ = [CreateClassifiedAttributedString(
            match.description, DimTextColor(isDarkTheme),
            match.description_class, isDarkTheme) retain];
      }
    }
  }
  return self;
}

- (void)dealloc {
  base::mac::ReleaseProperties(self);
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone*)zone {
  return [self retain];
}

- (CGFloat)getMatchContentsWidth {
  return [contents_ size].width;
}

@end

@implementation OmniboxPopupCell

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  OmniboxPopupMatrix* matrix =
      base::mac::ObjCCastStrict<OmniboxPopupMatrix>(controlView);
  BOOL isDarkTheme = [matrix hasDarkTheme];

  if ([self state] == NSOnState || [self isHighlighted]) {
    if ([self state] == NSOnState) {
      [SelectedBackgroundColor(isDarkTheme) set];
    } else {
      [HoveredBackgroundColor(isDarkTheme) set];
    }
    NSRectFillUsingOperation(cellFrame, NSCompositeSourceOver);
  }

  [self drawMatchWithFrame:cellFrame inView:controlView];
}

- (void)drawMatchWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  OmniboxPopupCellData* cellData =
      base::mac::ObjCCastStrict<OmniboxPopupCellData>([self objectValue]);
  OmniboxPopupMatrix* tableView =
      base::mac::ObjCCastStrict<OmniboxPopupMatrix>(controlView);
  CGFloat remainingWidth =
      [OmniboxPopupCell getTextContentAreaWidth:[tableView contentMaxWidth]];
  CGFloat contentsWidth = [cellData getMatchContentsWidth];
  CGFloat separatorWidth = [[tableView separator] size].width;
  CGFloat descriptionWidth =
      [cellData description] ? [[cellData description] size].width : 0;
  int contentsMaxWidth, descriptionMaxWidth;
  OmniboxPopupModel::ComputeMatchMaxWidths(
      ceilf(contentsWidth), ceilf(separatorWidth), ceilf(descriptionWidth),
      ceilf(remainingWidth), [cellData isAnswer],
      !AutocompleteMatch::IsSearchType([cellData matchType]), &contentsMaxWidth,
      &descriptionMaxWidth);

  NSWindow* parentWindow = [[controlView window] parentWindow];
  BOOL isDarkTheme = [parentWindow hasDarkTheme];
  NSRect imageRect = cellFrame;
  imageRect.size = [[cellData image] size];
  imageRect.origin.x += kMaterialImageXOffset + [tableView contentLeftPadding];
  imageRect.origin.y +=
      GetVerticalMargin() + kMaterialExtraVerticalImagePadding;
  [[cellData image] drawInRect:FlipIfRTL(imageRect, cellFrame)
                      fromRect:NSZeroRect
                     operation:NSCompositeSourceOver
                      fraction:1.0
                respectFlipped:YES
                         hints:nil];

  CGFloat left = kMaterialTextStartOffset + [tableView contentLeftPadding];
  NSPoint origin = NSMakePoint(left, GetVerticalMargin());

  origin.x += [self drawMatchPart:[cellData contents]
                        withFrame:cellFrame
                           origin:origin
                     withMaxWidth:contentsMaxWidth
                     forDarkTheme:isDarkTheme
                    withHeightCap:true];

  if (descriptionMaxWidth > 0) {
    if ([cellData isAnswer]) {
      origin = NSMakePoint(
          left, [OmniboxPopupCell getContentTextHeight] - GetVerticalMargin());
      CGFloat imageSize = [tableView answerLineHeight];
      NSRect imageRect =
          NSMakeRect(NSMinX(cellFrame) + origin.x, NSMinY(cellFrame) + origin.y,
                     imageSize, imageSize);
      [[cellData answerImage] drawInRect:FlipIfRTL(imageRect, cellFrame)
                                fromRect:NSZeroRect
                               operation:NSCompositeSourceOver
                                fraction:1.0
                          respectFlipped:YES
                                   hints:nil];
      if ([cellData answerImage]) {
        origin.x += imageSize + kMaterialImageXOffset;

        // Have to nudge the baseline down 1pt in Material Design for the text
        // that follows, so that it's the same as the bottom of the image.
        origin.y += 1;
      }
    } else {
      origin.x += [self drawMatchPart:[tableView separator]
                            withFrame:cellFrame
                               origin:origin
                         withMaxWidth:separatorWidth
                         forDarkTheme:isDarkTheme
                        withHeightCap:true];
    }
    [self drawMatchPart:[cellData description]
              withFrame:cellFrame
                 origin:origin
           withMaxWidth:descriptionMaxWidth
           forDarkTheme:isDarkTheme
          withHeightCap:false];
  }
}

- (CGFloat)drawMatchPart:(NSAttributedString*)attributedString
               withFrame:(NSRect)cellFrame
                  origin:(NSPoint)origin
            withMaxWidth:(int)maxWidth
            forDarkTheme:(BOOL)isDarkTheme
           withHeightCap:(BOOL)hasHeightCap {
  NSRect renderRect = NSIntersectionRect(
      cellFrame, NSOffsetRect(cellFrame, origin.x, origin.y));
  renderRect.size.width =
      std::min(NSWidth(renderRect), static_cast<CGFloat>(maxWidth));
  if (hasHeightCap)
    renderRect.size.height =
        std::min(NSHeight(renderRect), [attributedString size].height);
  if (!NSIsEmptyRect(renderRect)) {
    [attributedString drawWithRect:FlipIfRTL(renderRect, cellFrame)
                           options:NSStringDrawingUsesLineFragmentOrigin |
                                   NSStringDrawingTruncatesLastVisibleLine];
  }
  return NSWidth(renderRect);
}

+ (CGFloat)computeContentsOffset:(const AutocompleteMatch&)match {
  const base::string16& inputText = base::UTF8ToUTF16(
      match.GetAdditionalInfo(kACMatchPropertySuggestionText));
  int contentsStartIndex = 0;
  base::StringToInt(
      match.GetAdditionalInfo(kACMatchPropertyContentsStartIndex),
      &contentsStartIndex);
  // Ignore invalid state.
  if (!base::StartsWith(match.fill_into_edit, inputText,
                        base::CompareCase::SENSITIVE) ||
      !base::EndsWith(match.fill_into_edit, match.contents,
                      base::CompareCase::SENSITIVE) ||
      ((size_t)contentsStartIndex >= inputText.length())) {
    return 0;
  }
  bool isContentsRTL = (base::i18n::RIGHT_TO_LEFT ==
      base::i18n::GetFirstStrongCharacterDirection(match.contents));

  // Color does not matter.
  NSAttributedString* attributedString =
      CreateAttributedString(inputText, DimTextColor(false));
  base::scoped_nsobject<NSTextStorage> textStorage(
      [[NSTextStorage alloc] initWithAttributedString:attributedString]);
  base::scoped_nsobject<NSLayoutManager> layoutManager(
      [[NSLayoutManager alloc] init]);
  base::scoped_nsobject<NSTextContainer> textContainer(
      [[NSTextContainer alloc] init]);
  [layoutManager addTextContainer:textContainer];
  [textStorage addLayoutManager:layoutManager];

  NSUInteger charIndex = static_cast<NSUInteger>(contentsStartIndex);
  NSUInteger glyphIndex =
      [layoutManager glyphIndexForCharacterAtIndex:charIndex];

  // This offset is computed from the left edge of the glyph always from the
  // left edge of the string, irrespective of the directionality of UI or text.
  CGFloat glyphOffset = [layoutManager locationForGlyphAtIndex:glyphIndex].x;

  CGFloat inputWidth = [attributedString size].width;

  // The offset obtained above may need to be corrected because the left-most
  // glyph may not have 0 offset. So we find the offset of left-most glyph, and
  // subtract it from the offset of the glyph we obtained above.
  CGFloat minOffset = glyphOffset;

  // If content is RTL, we are interested in the right-edge of the glyph.
  // Unfortunately the bounding rect computation methods from NSLayoutManager or
  // NSFont don't work correctly with bidirectional text. So we compute the
  // glyph width by finding the closest glyph offset to the right of the glyph
  // we are looking for.
  CGFloat glyphWidth = inputWidth;

  for (NSUInteger i = 0; i < [attributedString length]; i++) {
    if (i == charIndex) continue;
    glyphIndex = [layoutManager glyphIndexForCharacterAtIndex:i];
    CGFloat offset = [layoutManager locationForGlyphAtIndex:glyphIndex].x;
    minOffset = std::min(minOffset, offset);
    if (offset > glyphOffset)
      glyphWidth = std::min(glyphWidth, offset - glyphOffset);
  }
  glyphOffset -= minOffset;
  if (glyphWidth == 0)
    glyphWidth = inputWidth - glyphOffset;
  if (isContentsRTL)
    glyphOffset += glyphWidth;
  return base::i18n::IsRTL() ? (inputWidth - glyphOffset) : glyphOffset;
}

+ (NSAttributedString*)createSeparatorStringForDarkTheme:(BOOL)isDarkTheme {
  base::string16 raw_separator =
      l10n_util::GetStringUTF16(IDS_AUTOCOMPLETE_MATCH_DESCRIPTION_SEPARATOR);
  return CreateAttributedString(raw_separator, DimTextColor(isDarkTheme));
}

+ (CGFloat)getTextContentAreaWidth:(CGFloat)cellContentMaxWidth {
  return cellContentMaxWidth - kMaterialTextStartOffset;
}

+ (CGFloat)getContentTextHeight {
  return kDefaultTextHeight + 2 * GetVerticalMargin();
}

@end
