// Copyright 2013 Google Inc. All Rights Reserved.

/**
 * @fileoverview This file defines the input tool, included IME and virtual
 *     keyboard.
 *
 * @author wuyingbing@google.com (Yingbing Wu)
 */

goog.provide('i18n.input.lang.InputTool');

goog.require('goog.array');
goog.require('goog.object');
goog.require('goog.string');
goog.require('i18n.input.common.GlobalSettings');
goog.require('i18n.input.lang.InputToolCode');
goog.require('i18n.input.lang.InputToolType');

goog.scope(function() {
var GlobalSettings = i18n.input.common.GlobalSettings;
var InputToolCode = i18n.input.lang.InputToolCode;
var InputToolType = i18n.input.lang.InputToolType;



/**
 * The input tool class is used to define Input Tool. Don't call the method
 * directly, use InputTool.get instead.
 *
 * @param {!InputToolCode} inputToolCode The input tool code
 *     value.
 * @constructor
 */
i18n.input.lang.InputTool = function(inputToolCode) {
  /**
   * The unique code of input tools.
   *
   * @type {!InputToolCode}
   */
  this.code = inputToolCode;

  /**
   * The input tools type value.
   *
   * @type {?InputToolType}
   */
  this.type = null;

  /**
   * The target language code.
   *
   * @type {string}
   */
  this.languageCode = 'en';

  /**
   * The source language code.
   *
   * @type {string}
   */
  this.sourceLanguageCode = 'en';

  /**
   * Keyboard layout code. Only valid if type is KBD.
   * @type {string}
   */
  this.layoutCode;

  // Parses input tool code.
  this.parseInputToolCode_();
};
var InputTool = i18n.input.lang.InputTool;


/**
 * The array of rtl keyboards' input tool codes.
 *
 * @type {!Array.<string>}
 */
InputTool.RtlKeyboards = [
  InputToolCode.KEYBOARD_ARABIC,
  InputToolCode.KEYBOARD_DARI,
  InputToolCode.KEYBOARD_HEBREW,
  InputToolCode.KEYBOARD_PASHTO,
  InputToolCode.KEYBOARD_PERSIAN,
  InputToolCode.KEYBOARD_SOUTHERN_UZBEK,
  InputToolCode.KEYBOARD_UIGHUR,
  InputToolCode.KEYBOARD_URDU,
  InputToolCode.KEYBOARD_YIDDISH];


/**
 * The array of rtl ime' input tool codes.
 *
 * @type {!Array.<string>}
 */
InputTool.RtlIMEs = [
  InputToolCode.INPUTMETHOD_TRANSLITERATION_ARABIC,
  InputToolCode.INPUTMETHOD_TRANSLITERATION_HEBREW,
  InputToolCode.INPUTMETHOD_TRANSLITERATION_PERSIAN,
  InputToolCode.INPUTMETHOD_TRANSLITERATION_URDU];


/**
 * The mapping from 3-letter language codes to 2-letter language codes.
 *
 * @type {!Object.<string, string>}
 */
InputTool.LanguageCodeThreeTwoMap = goog.object.create(
    'arm', 'hy',
    'bel', 'be',
    'bul', 'bg',
    'cat', 'ca',
    'cze', 'cs',
    'dan', 'da',
    'eng', 'en',
    'est', 'et',
    'fao', 'fo',
    'fin', 'fi',
    'fra', 'fr',
    'geo', 'ka',
    'ger', 'de',
    'gre', 'el',
    'heb', 'he',
    'hun', 'hu',
    'ice', 'is',
    'ind', 'id',
    'ita', 'it',
    'jpn', 'ja',
    'kaz', 'kk',
    'lav', 'lv',
    'lit', 'lt',
    'mlt', 'mt',
    'mon', 'mn',
    'msa', 'ms',
    'nld', 'nl',
    // The new specification is "nb", but NACL uses "no".
    'nob', 'no',
    'pol', 'pl',
    'por', 'pt',
    'rum', 'ro',
    'rus', 'ru',
    'scr', 'hr',
    'slo', 'sk',
    'slv', 'sl',
    'spa', 'es',
    'srp', 'sr',
    'swe', 'sv',
    'tur', 'tr',
    'ukr', 'uk');


/**
 * The special XKB id to language code mapping.
 *
 * @private {!Object.<string, string>}
 */
InputTool.XkbId2Language_ = {
  // NACL treads "pt-BR", "pt-PT" the same with "pt".
  'xkb:us:intl:por': 'pt',
  'xkb:br::por': 'pt',
  'xkb:pt::por': 'pt'
};


/**
 * The input tool code and instance mapping.
 *
 * @type {!Object.<string, InputTool>}
 * @private
 */
InputTool.instances_ = {};


/**
 * Gets an input tool by code.
 *
 * @param {!string} inputToolCode The input tool code value.
 * @return {InputTool} The input tool.
 */
InputTool.get = function(inputToolCode) {
  if (!inputToolCode) {
    return null;
  }

  // The code isn't BCP47 pattern, transfers it from old pattern.
  if (!goog.object.contains(InputToolCode, inputToolCode)) {
    inputToolCode = InputTool.parseToBCP47_(inputToolCode);
  }

  // Allow BCP47 code 'fa_t_k0_und' to 'fa-t-k0-und'.
  inputToolCode = inputToolCode.replace(/_/g, '-');

  // Adds '-und' to keep compatible with previous codes.
  if (!goog.object.contains(InputToolCode, inputToolCode)) {
    inputToolCode = InputTool.parseToBCP47_(
        inputToolCode + '-und');
  }

  if (InputTool.instances_[inputToolCode]) {
    return InputTool.instances_[inputToolCode];
  }

  // If the input tool code is valid.
  if (goog.object.contains(InputToolCode, inputToolCode)) {
    InputTool.instances_[inputToolCode] =
        new InputTool(
        /** @type {InputToolCode} */ (inputToolCode));
    return InputTool.instances_[inputToolCode];
  }

  return null;
};


/**
 * Language codes whose BCP47 code has a rule like:
 * Has 'und-latn', then adding 'phone' at last, otherwise, 'inscript' at last.
 *
 * @type {!Array.<string>}
 * @private
 */
InputTool.PHONETIC_INSCRIPT_LANGS_ = [
  'bn', 'gu', 'pa', 'kn', 'ml', 'or', 'sa', 'ta', 'te', 'ne'
];


/**
 * Special previous old code mapping to BCP47 code.
 *
 * @type {!Object.<string, string>}
 * @private
 * @const
 */
InputTool.BCP47_SPECIAL_ = {
  'im_pinyin_zh_hans': InputToolCode.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED,
  'im_pinyin_zh_hant': InputToolCode.INPUTMETHOD_PINYIN_CHINESE_TRADITIONAL,
  'im_t13n_ja': InputToolCode.INPUTMETHOD_TRANSLITERATION_JAPANESE,
  'im_t13n_ja-Hira': InputToolCode.INPUTMETHOD_TRANSLITERATION_HIRAGANA,
  'im_wubi_zh_hans': InputToolCode.INPUTMETHOD_WUBI_CHINESE_SIMPLIFIED,
  'im_zhuyin_zh_hant': InputToolCode.INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL,
  'vkd_bg_phone': InputToolCode.KEYBOARD_BULGARIAN_PHONETIC,
  'vkd_chr_phone': InputToolCode.KEYBOARD_CHEROKEE_PHONETIC,
  'vkd_cs_qwertz': InputToolCode.KEYBOARD_CZECH_QWERTZ,
  'vkd_deva_phone': InputToolCode.KEYBOARD_DEVANAGARI_PHONETIC,
  'vkd_en_dvorak': InputToolCode.KEYBOARD_ENGLISH_DVORAK,
  'vkd_es_es': InputToolCode.KEYBOARD_SPANISH,
  'vkd_ethi': InputToolCode.KEYBOARD_ETHIOPIC,
  'vkd_gu_phone': InputToolCode.KEYBOARD_GUJARATI_PHONETIC,
  'vkd_guru_inscript': InputToolCode.KEYBOARD_GURMUKHI_INSCRIPT,
  'vkd_guru_phone': InputToolCode.KEYBOARD_GURMUKHI_PHONETIC,
  'vkd_hu_101': InputToolCode.KEYBOARD_HUNGARIAN_101,
  'vkd_hy_east': InputToolCode.KEYBOARD_ARMENIAN_EASTERN,
  'vkd_hy_west': InputToolCode.KEYBOARD_ARMENIAN_WESTERN,
  'vkd_ka_qwerty': InputToolCode.KEYBOARD_GEORGIAN_QWERTY,
  'vkd_ka_typewriter': InputToolCode.KEYBOARD_GEORGIAN_TYPEWRITER,
  'vkd_ro_sr13392_primary': InputToolCode.KEYBOARD_ROMANIAN_SR13392_PRIMARY,
  'vkd_ro_sr13392_secondary': InputToolCode.KEYBOARD_ROMANIAN_SR13392_SECONDARY,
  'vkd_ru_phone': InputToolCode.KEYBOARD_RUSSIAN_PHONETIC,
  'vkd_ru_phone_aatseel': InputToolCode.KEYBOARD_RUSSIAN_PHONETIC_AATSEEL,
  'vkd_ru_phone_yazhert': InputToolCode.KEYBOARD_RUSSIAN_PHONETIC_YAZHERT,
  'vkd_sk_qwerty': InputToolCode.KEYBOARD_SLOVAK_QWERTY,
  'vkd_ta_itrans': InputToolCode.KEYBOARD_TAMIL_ITRANS,
  'vkd_ta_tamil99': InputToolCode.KEYBOARD_TAMIL_99,
  'vkd_ta_typewriter': InputToolCode.KEYBOARD_TAMIL_TYPEWRITER,
  'vkd_th_pattajoti': InputToolCode.KEYBOARD_THAI_PATTAJOTI,
  'vkd_th_tis': InputToolCode.KEYBOARD_THAI_TIS,
  'vkd_tr_f': InputToolCode.KEYBOARD_TURKISH_F,
  'vkd_tr_q': InputToolCode.KEYBOARD_TURKISH_Q,
  'vkd_uk_101': InputToolCode.KEYBOARD_UKRAINIAN_101,
  'vkd_us_intl': InputToolCode.KEYBOARD_FRENCH_INTL,
  'vkd_uz_cyrl_phone': InputToolCode.KEYBOARD_UZBEK_CYRILLIC_PHONETIC,
  'vkd_uz_cyrl_type': InputToolCode.KEYBOARD_UZBEK_CYRILLIC_TYPEWRITTER,
  'vkd_vi_tcvn': InputToolCode.KEYBOARD_VIETNAMESE_TCVN,
  'vkd_vi_telex': InputToolCode.KEYBOARD_VIETNAMESE_TELEX
};


/**
 * BCP47 code maps to previous code.
 *
 * @type {!Object.<string, string>}
 * @private
 */
InputTool.BCP47_SPECIAL_REVERSE_ = goog.object.transpose(
    InputTool.BCP47_SPECIAL_);


/**
 * Special keyboard layout code mapping. Multiple Input Tools map to the same
 * layout.
 *
 * key: Input Tool code.
 * value: layout code.
 *
 * @private {!Object.<string, string>}
 */
InputTool.SpecialLayoutCodes_ = goog.object.create(
    InputToolCode.KEYBOARD_DUTCH_INTL, 'us_intl',
    InputToolCode.KEYBOARD_FRENCH_INTL, 'us_intl',
    InputToolCode.KEYBOARD_GERMAN_INTL, 'us_intl',
    InputToolCode.KEYBOARD_HAITIAN, 'fr',
    InputToolCode.KEYBOARD_INDONESIAN, 'latn_002',
    InputToolCode.KEYBOARD_IRISH, 'latn_002',
    InputToolCode.KEYBOARD_ITALIAN_INTL, 'us_intl',
    InputToolCode.KEYBOARD_JAVANESE, 'latn_002',
    InputToolCode.KEYBOARD_MARATHI, 'deva_phone',
    InputToolCode.KEYBOARD_MARATHI_INSCRIPT, 'hi',
    InputToolCode.KEYBOARD_MALAY, 'latn_002',
    InputToolCode.KEYBOARD_PORTUGUESE_BRAZIL_INTL, 'us_intl',
    InputToolCode.KEYBOARD_PORTUGUESE_PORTUGAL_INTL, 'us_intl',
    InputToolCode.KEYBOARD_SANSKRIT_INSCRIPT, 'hi',
    InputToolCode.KEYBOARD_SPANISH_INTL, 'us_intl',
    InputToolCode.KEYBOARD_SWAHILI, 'latn_002',
    InputToolCode.KEYBOARD_TAGALOG, 'latn_002',
    InputToolCode.KEYBOARD_TIGRINYA, 'ethi',
    InputToolCode.KEYBOARD_WELSH, 'latn_002');


/**
 * Parses previous old code to BCP 47 code.
 *
 * @param {string} itCode Previous old input tool code format.
 * @return {string} BCP 47 code.
 * @private
 */
InputTool.parseToBCP47_ = function(itCode) {
  if (InputTool.BCP47_SPECIAL_[itCode]) {
    return InputTool.BCP47_SPECIAL_[itCode];
  }

  if (itCode == 'vkd_iw') {
    return InputToolCode.KEYBOARD_HEBREW;
  }

  if (itCode == 'im_t13n_iw') {
    return InputToolCode.INPUTMETHOD_TRANSLITERATION_HEBREW;
  }

  // Types 'legacy' to 'lagacy' by mistake, correct it.
  // Can't put 'tr' + '-t-k0-lagacy' into BCP47_SPECIAL_ map, becasue we have
  // to split 'tr-t-k0-lagacy' but JS grammar wasn't allow to
  // use 'tr' + '-t-k0-lagacy' as key.
  if (itCode == 'tr' + '-t-k0-lagacy') {
    return InputToolCode.KEYBOARD_TURKISH_F;
  }

  var parts = itCode.split('_');
  var code = '';
  if (goog.string.startsWith(itCode, 'im_t13n')) {
    // Example: 'im_t13n_hi'.
    code = parts[2] + '-t-i0-und';
  } else if (goog.string.startsWith(itCode, 'vkd_')) {
    // Special codes for keyboard.
    if (parts.length == 2) {
      // Example: 'vkd_sq'
      code = parts[1] + '-t-k0-und';
    } else {
      if (goog.array.contains(
          InputTool.PHONETIC_INSCRIPT_LANGS_, parts[1])) {
        if (parts[2] == 'inscript') {
          code = parts[1] + '-t-k0-und';
        } else {
          code = parts[1] + '-t-und-latn-k0-und';
        }
      } else {
        code = parts[1] + '-t-k0-' + parts[2];
        if (!goog.object.contains(InputToolCode, code)) {
          code = parts[1] + '-' + parts[2] + '-t-k0-und';
        }
      }
    }
  }
  return goog.object.contains(InputToolCode, code) ? code : itCode;
};


/**
 * Gets the input tools by parameters. Keep compatible with previous language
 * code pair. Not support to get input tool by keyboard layout.
 *
 * @param {!InputToolType} type The input tool type.
 * @param {!string} code It's the target language code if type is input method.
 * @return {InputTool} The input tool.
 */
InputTool.getInputTool = function(type, code) {
  // Makes compatible input tool code with previous language code version.
  if (type == InputToolType.IME) {
    if (code == 'zh' || code == 'zh-Hans') {
      return InputTool.get(
          InputToolCode.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED);
    } else if (code == 'zh-Hant') {
      return InputTool.get(
          InputToolCode.INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL);
    } else if (code == 'ja') {
      return InputTool.get(
          InputToolCode.INPUTMETHOD_TRANSLITERATION_JAPANESE);
    } else {
      return InputTool.get(code + '-t-i0-und');
    }
  } else if (type == InputToolType.KBD) {
    return InputTool.get('vkd_' + code);
  }
  return null;
};


/**
 * Parses BCP47 codes to the virtual keyboard layout.
 *
 * @private
 */
InputTool.prototype.parseLayoutCode_ = function() {
  if (InputTool.SpecialLayoutCodes_[this.code]) {
    this.layoutCode = InputTool.SpecialLayoutCodes_[this.code];
  } else if (InputTool.BCP47_SPECIAL_REVERSE_[this.code]) {
    // Removes prefix 'vkd_';
    this.layoutCode = InputTool.
        BCP47_SPECIAL_REVERSE_[this.code].slice(4);
  } else {
    var parts = this.code.split('-t-');
    var countryCode = parts[0];
    var inputToolType = parts[1];
    countryCode = countryCode.replace(/-/g, '_');
    if (countryCode == 'en_us') {
      countryCode = 'us';
    }

    if (goog.array.contains(
        InputTool.PHONETIC_INSCRIPT_LANGS_, countryCode) &&
        (inputToolType == 'und-latn-k0-und' || inputToolType == 'k0-und')) {
      // If it's virtual keyboard having the inscript/phonetic rule.
      this.layoutCode = countryCode +
          (inputToolType == 'k0-und' ? '_inscript' : '_phone');
    } else if (inputToolType == 'k0-und') {
      this.layoutCode = countryCode;
    } else {
      var matches = inputToolType.match(/k0-(.*)/);
      if (matches[1]) {
        this.layoutCode = countryCode + '_' + matches[1].replace(
            'qwerty', 'phone').replace('-', '_');
      }
    }
  }
};


/**
 * Parses the input tool code.
 * TODO(wuyingbing): We will introduce new code pattern, and then write a new
 * parsing method.
 *
 * @private
 */
InputTool.prototype.parseInputToolCode_ = function() {
  // Sets the input tool type.
  if (this.code.indexOf('-i0') >= 0) {
    this.type = InputToolType.IME;
    if (goog.string.endsWith(this.code, '-handwrit')) {
      this.type = InputToolType.HWT;
    } else if (goog.string.endsWith(this.code, '-voice')) {
      this.type = InputToolType.VOICE;
    }
  } else if (this.code.indexOf('-k0') >= 0) {
    this.type = InputToolType.KBD;
  } else if (goog.string.startsWith(this.code, 'xkb')) {
    this.type = InputToolType.XKB;
  }

  // Sets target language code.
  var codes = this.code.split(/-t|-i0|-k0|:/);

  if (codes[0] == 'yue-hant') {
    codes[0] = 'zh-Hant';
  }
  switch (this.code) {
    // Currently most of systems doesn't support 'yue-hant', so hack it to
    // 'zh-hant';
    case InputToolCode.INPUTMETHOD_CANTONESE_TRADITIONAL:
      codes[0] = 'zh-Hant';
      break;
    case InputToolCode.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED:
    case InputToolCode.INPUTMETHOD_WUBI_CHINESE_SIMPLIFIED:
      codes[0] = 'zh-Hans';
      break;
  }
  if (this.type == InputToolType.XKB) {
    if (InputTool.XkbId2Language_[this.code]) {
      this.languageCode = InputTool.XkbId2Language_[this.code];
    } else {
      this.languageCode = this.formatLanguageCode_(codes[codes.length - 1]);
    }
  } else {
    this.languageCode = this.formatLanguageCode_(codes[0]);
    // Sets source language target.
    if (codes[1]) {
      this.sourceLanguageCode = this.formatLanguageCode_(codes[1]);
    }
  }

  if (this.type == InputToolType.KBD) {
    this.parseLayoutCode_();
  }
};


/** @override */
InputTool.prototype.toString = function() {
  return this.code;
};


/**
 * Gets the input tool's direction.
 *
 * @return {string} The direction string - 'rtl' or 'ltr'.
 */
InputTool.prototype.getDirection = function() {
  return this.isRightToLeft() ? 'rtl' : 'ltr';
};


/**
 * Gets the input tool's direction.
 *
 * @return {boolean} Whether is rtl direction of the input tool.
 */
InputTool.prototype.isRightToLeft = function() {
  return goog.array.contains(InputTool.RtlIMEs, this.code) ||
      goog.array.contains(InputTool.RtlKeyboards, this.code);
};


/**
 * Gets whether has status bar.
 *
 * @return {boolean} Whether has status bar.
 */
InputTool.prototype.hasStatusBar = function() {
  // Don't show status bar in moblie device.
  if (!GlobalSettings.mobile && this.type == InputToolType.IME) {
    return /^(zh|yue)/.test(this.code);
  }
  return false;
};


/**
 * Format language to standard language code.
 *
 * @param {string} code The language code.
 * @return {string} The standard language code.
 * @private
 */
InputTool.prototype.formatLanguageCode_ = function(code) {
  // Hack 'und-ethi' to 'et'. The major population use 'ethi' script in
  // Ethiopia country. So we set 'et' as language code.
  if (code == 'und-ethi') {
    return 'et';
  }

  var parts = code.split('-');
  var retCode;
  if (parts.length == 2) {
    if (parts[1].length == 2) {
      retCode = parts[0] + '-' + parts[1].toUpperCase();
    } else {
      retCode = parts[0] + '-' + parts[1].charAt(0).toUpperCase() +
          parts[1].slice(1);
    }
  } else {
    if (goog.object.containsKey(InputTool.LanguageCodeThreeTwoMap, parts[0])) {
      retCode = InputTool.LanguageCodeThreeTwoMap[parts[0]];
    } else {
      retCode = parts[0];
    }
  }
  return retCode;
};


/**
 * Returns whether the input tool is transliteration or not.
 *
 * @return {boolean} .
 */
InputTool.prototype.isTransliteration = function() {
  var reg = new RegExp('^(am|ar|bn|el|gu|he|hi|kn|ml|mr|ne|or|fa|pa|ru|sa|' +
      'sr|si|ta|te|ti|ur|uk|be|bg)');
  return this.type == InputToolType.IME && reg.test(this.code);
};


/**
 * Returns whether the input tool is Latin suggestion or not.
 *
 * @return {boolean} .
 */
InputTool.prototype.isLatin = function() {
  return this.type == InputToolType.IME &&
      /^(en|fr|de|it|es|nl|pt|tr|sv|da|fi|no)/.test(this.code);
};
});  // goog.scope

