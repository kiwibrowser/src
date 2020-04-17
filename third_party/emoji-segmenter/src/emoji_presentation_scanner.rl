/* Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

%%{
  machine emoji_presentation;
  alphtype unsigned char;
  write data noerror nofinal noentry;
}%%

%%{

EMOJI = 0;
EMOJI_TEXT_PRESENTATION = 1;
EMOJI_EMOJI_PRESENTATION = 2;
EMOJI_MODIFIER_BASE = 3;
EMOJI_MODIFIER = 4;
EMOJI_VS_BASE = 5;
REGIONAL_INDICATOR = 6;
KEYCAP_BASE = 7;
COMBINING_ENCLOSING_KEYCAP = 8;
COMBINING_ENCLOSING_CIRCLE_BACKSLASH = 9;
ZWJ = 10;
VS15 = 11;
VS16 = 12;
TAG_BASE = 13;
TAG_SEQUENCE = 14;
TAG_TERM = 15;

any_emoji =  EMOJI_TEXT_PRESENTATION | EMOJI_EMOJI_PRESENTATION |  KEYCAP_BASE |
  EMOJI_MODIFIER_BASE | TAG_BASE | EMOJI;

emoji_combining_enclosing_circle_backslash_sequence = any_emoji
  COMBINING_ENCLOSING_CIRCLE_BACKSLASH;

# This could be sharper than any_emoji by restricting this only to valid
# variation sequences:
# https://www.unicode.org/Public/emoji/11.0/emoji-variation-sequences.txt
# However, implementing
# https://www.unicode.org/reports/tr51/#def_emoji_presentation_sequence is
# sufficient for our purposes here.
emoji_presentation_sequence = any_emoji VS16;

emoji_modifier_sequence = EMOJI_MODIFIER_BASE EMOJI_MODIFIER;

emoji_flag_sequence = REGIONAL_INDICATOR REGIONAL_INDICATOR;

# Here we only allow the valid tag sequences
# https://www.unicode.org/reports/tr51/#valid-emoji-tag-sequences, instead of
# all well-formed ones defined in
# https://www.unicode.org/reports/tr51/#def_emoji_tag_sequence
emoji_tag_sequence = TAG_BASE TAG_SEQUENCE+ TAG_TERM;

emoji_keycap_sequence = KEYCAP_BASE VS16 COMBINING_ENCLOSING_KEYCAP;

emoji_zwj_element =  emoji_presentation_sequence | emoji_modifier_sequence | any_emoji;

emoji_zwj_sequence = emoji_zwj_element ( ZWJ emoji_zwj_element )+;

emoji_presentation = EMOJI_EMOJI_PRESENTATION | TAG_BASE | EMOJI_MODIFIER_BASE |
 emoji_presentation_sequence | emoji_modifier_sequence | emoji_flag_sequence |
 emoji_tag_sequence | emoji_keycap_sequence | emoji_zwj_sequence |
 emoji_combining_enclosing_circle_backslash_sequence;

emoji_run = emoji_presentation;

text_presentation_emoji = any_emoji VS15;
text_run = any;

text_and_emoji_run := |*
# In order to give the the VS15 sequences higher priority than detecting
# emoji sequences they are listed first as scanner token here.
text_presentation_emoji => { *is_emoji = false; return te; };
emoji_run => { *is_emoji = true; return te; };
text_run => { *is_emoji = false; return te; };
*|;

}%%

static emoji_text_iter_t
scan_emoji_presentation (emoji_text_iter_t p,
    const emoji_text_iter_t pe,
    bool* is_emoji)
{
  emoji_text_iter_t ts, te;
  const emoji_text_iter_t eof = pe;

  unsigned act;
  int cs;

  %%{
    write init;
    write exec;
  }%%

  /* Should not be reached. */
  *is_emoji = false;
  return pe;
}
