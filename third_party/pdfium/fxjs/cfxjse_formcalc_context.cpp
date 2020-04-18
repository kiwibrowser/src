// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cfxjse_formcalc_context.h"

#include <time.h>

#include <algorithm>
#include <string>
#include <utility>

#include "core/fxcrt/cfx_decimal.h"
#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_random.h"
#include "core/fxcrt/locale_iface.h"
#include "fxjs/cfxjse_class.h"
#include "fxjs/cfxjse_engine.h"
#include "fxjs/cfxjse_value.h"
#include "fxjs/xfa/cjx_object.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"
#include "xfa/fxfa/cxfa_ffnotify.h"
#include "xfa/fxfa/fm2js/cxfa_fmparser.h"
#include "xfa/fxfa/fm2js/cxfa_fmtojavascriptdepth.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_localevalue.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_timezoneprovider.h"
#include "xfa/fxfa/parser/xfa_utils.h"

namespace {

const double kFinancialPrecision = 0.00000001;

struct XFA_FMHtmlReserveCode {
  uint32_t m_uCode;
  const wchar_t* m_htmlReserve;
};

// Sorted by |m_htmlReserve|.
const XFA_FMHtmlReserveCode kReservesForDecode[] = {
    {198, L"AElig"},   {193, L"Aacute"},   {194, L"Acirc"},
    {192, L"Agrave"},  {913, L"Alpha"},    {197, L"Aring"},
    {195, L"Atilde"},  {196, L"Auml"},     {914, L"Beta"},
    {199, L"Ccedil"},  {935, L"Chi"},      {8225, L"Dagger"},
    {916, L"Delta"},   {208, L"ETH"},      {201, L"Eacute"},
    {202, L"Ecirc"},   {200, L"Egrave"},   {917, L"Epsilon"},
    {919, L"Eta"},     {203, L"Euml"},     {915, L"Gamma"},
    {922, L"Kappa"},   {923, L"Lambda"},   {924, L"Mu"},
    {209, L"Ntilde"},  {925, L"Nu"},       {338, L"OElig"},
    {211, L"Oacute"},  {212, L"Ocirc"},    {210, L"Ograve"},
    {937, L"Omega"},   {927, L"Omicron"},  {216, L"Oslash"},
    {213, L"Otilde"},  {214, L"Ouml"},     {934, L"Phi"},
    {928, L"Pi"},      {936, L"Psi"},      {929, L"Rho"},
    {352, L"Scaron"},  {931, L"Sigma"},    {222, L"THORN"},
    {932, L"Tau"},     {920, L"Theta"},    {218, L"Uacute"},
    {219, L"Ucirc"},   {217, L"Ugrave"},   {933, L"Upsilon"},
    {220, L"Uuml"},    {926, L"Xi"},       {221, L"Yacute"},
    {376, L"Yuml"},    {918, L"Zeta"},     {225, L"aacute"},
    {226, L"acirc"},   {180, L"acute"},    {230, L"aelig"},
    {224, L"agrave"},  {8501, L"alefsym"}, {945, L"alpha"},
    {38, L"amp"},      {8743, L"and"},     {8736, L"ang"},
    {39, L"apos"},     {229, L"aring"},    {8776, L"asymp"},
    {227, L"atilde"},  {228, L"auml"},     {8222, L"bdquo"},
    {946, L"beta"},    {166, L"brvbar"},   {8226, L"bull"},
    {8745, L"cap"},    {231, L"ccedil"},   {184, L"cedil"},
    {162, L"cent"},    {967, L"chi"},      {710, L"circ"},
    {9827, L"clubs"},  {8773, L"cong"},    {169, L"copy"},
    {8629, L"crarr"},  {8746, L"cup"},     {164, L"current"},
    {8659, L"dArr"},   {8224, L"dagger"},  {8595, L"darr"},
    {176, L"deg"},     {948, L"delta"},    {9830, L"diams"},
    {247, L"divide"},  {233, L"eacute"},   {234, L"ecirc"},
    {232, L"egrave"},  {8709, L"empty"},   {8195, L"emsp"},
    {8194, L"ensp"},   {949, L"epsilon"},  {8801, L"equiv"},
    {951, L"eta"},     {240, L"eth"},      {235, L"euml"},
    {8364, L"euro"},   {8707, L"exist"},   {402, L"fnof"},
    {8704, L"forall"}, {189, L"frac12"},   {188, L"frac14"},
    {190, L"frac34"},  {8260, L"frasl"},   {947, L"gamma"},
    {8805, L"ge"},     {62, L"gt"},        {8660, L"hArr"},
    {8596, L"harr"},   {9829, L"hearts"},  {8230, L"hellip"},
    {237, L"iacute"},  {238, L"icirc"},    {161, L"iexcl"},
    {236, L"igrave"},  {8465, L"image"},   {8734, L"infin"},
    {8747, L"int"},    {953, L"iota"},     {191, L"iquest"},
    {8712, L"isin"},   {239, L"iuml"},     {954, L"kappa"},
    {8656, L"lArr"},   {205, L"lacute"},   {955, L"lambda"},
    {9001, L"lang"},   {171, L"laquo"},    {8592, L"larr"},
    {8968, L"lceil"},  {206, L"lcirc"},    {8220, L"ldquo"},
    {8804, L"le"},     {8970, L"lfloor"},  {204, L"lgrave"},
    {921, L"lota"},    {8727, L"lowast"},  {9674, L"loz"},
    {8206, L"lrm"},    {8249, L"lsaquo"},  {8216, L"lsquo"},
    {60, L"lt"},       {207, L"luml"},     {175, L"macr"},
    {8212, L"mdash"},  {181, L"micro"},    {183, L"middot"},
    {8722, L"minus"},  {956, L"mu"},       {8711, L"nabla"},
    {160, L"nbsp"},    {8211, L"ndash"},   {8800, L"ne"},
    {8715, L"ni"},     {172, L"not"},      {8713, L"notin"},
    {8836, L"nsub"},   {241, L"ntilde"},   {957, L"nu"},
    {243, L"oacute"},  {244, L"ocirc"},    {339, L"oelig"},
    {242, L"ograve"},  {8254, L"oline"},   {969, L"omega"},
    {959, L"omicron"}, {8853, L"oplus"},   {8744, L"or"},
    {170, L"ordf"},    {186, L"ordm"},     {248, L"oslash"},
    {245, L"otilde"},  {8855, L"otimes"},  {246, L"ouml"},
    {182, L"para"},    {8706, L"part"},    {8240, L"permil"},
    {8869, L"perp"},   {966, L"phi"},      {960, L"pi"},
    {982, L"piv"},     {177, L"plusmn"},   {8242, L"prime"},
    {8719, L"prod"},   {8733, L"prop"},    {968, L"psi"},
    {163, L"pund"},    {34, L"quot"},      {8658, L"rArr"},
    {8730, L"radic"},  {9002, L"rang"},    {187, L"raquo"},
    {8594, L"rarr"},   {8969, L"rceil"},   {8476, L"real"},
    {174, L"reg"},     {8971, L"rfloor"},  {961, L"rho"},
    {8207, L"rlm"},    {8250, L"rsaquo"},  {8217, L"rsquo"},
    {353, L"saron"},   {8218, L"sbquo"},   {8901, L"sdot"},
    {167, L"sect"},    {173, L"shy"},      {963, L"sigma"},
    {962, L"sigmaf"},  {8764, L"sim"},     {9824, L"spades"},
    {8834, L"sub"},    {8838, L"sube"},    {8721, L"sum"},
    {8835, L"sup"},    {185, L"sup1"},     {178, L"sup2"},
    {179, L"sup3"},    {8839, L"supe"},    {223, L"szlig"},
    {964, L"tau"},     {8221, L"tdquo"},   {8756, L"there4"},
    {952, L"theta"},   {977, L"thetasym"}, {8201, L"thinsp"},
    {254, L"thorn"},   {732, L"tilde"},    {215, L"times"},
    {8482, L"trade"},  {8657, L"uArr"},    {250, L"uacute"},
    {8593, L"uarr"},   {251, L"ucirc"},    {249, L"ugrave"},
    {168, L"uml"},     {978, L"upsih"},    {965, L"upsilon"},
    {252, L"uuml"},    {8472, L"weierp"},  {958, L"xi"},
    {253, L"yacute"},  {165, L"yen"},      {255, L"yuml"},
    {950, L"zeta"},    {8205, L"zwj"},     {8204, L"zwnj"},
};

// Sorted by |m_uCode|.
const XFA_FMHtmlReserveCode kReservesForEncode[] = {
    {34, L"quot"},     {38, L"amp"},      {39, L"apos"},
    {60, L"lt"},       {62, L"gt"},       {160, L"nbsp"},
    {161, L"iexcl"},   {162, L"cent"},    {163, L"pund"},
    {164, L"current"}, {165, L"yen"},     {166, L"brvbar"},
    {167, L"sect"},    {168, L"uml"},     {169, L"copy"},
    {170, L"ordf"},    {171, L"laquo"},   {172, L"not"},
    {173, L"shy"},     {174, L"reg"},     {175, L"macr"},
    {176, L"deg"},     {177, L"plusmn"},  {178, L"sup2"},
    {179, L"sup3"},    {180, L"acute"},   {181, L"micro"},
    {182, L"para"},    {183, L"middot"},  {184, L"cedil"},
    {185, L"sup1"},    {186, L"ordm"},    {187, L"raquo"},
    {188, L"frac14"},  {189, L"frac12"},  {190, L"frac34"},
    {191, L"iquest"},  {192, L"Agrave"},  {193, L"Aacute"},
    {194, L"Acirc"},   {195, L"Atilde"},  {196, L"Auml"},
    {197, L"Aring"},   {198, L"AElig"},   {199, L"Ccedil"},
    {200, L"Egrave"},  {201, L"Eacute"},  {202, L"Ecirc"},
    {203, L"Euml"},    {204, L"lgrave"},  {205, L"lacute"},
    {206, L"lcirc"},   {207, L"luml"},    {208, L"ETH"},
    {209, L"Ntilde"},  {210, L"Ograve"},  {211, L"Oacute"},
    {212, L"Ocirc"},   {213, L"Otilde"},  {214, L"Ouml"},
    {215, L"times"},   {216, L"Oslash"},  {217, L"Ugrave"},
    {218, L"Uacute"},  {219, L"Ucirc"},   {220, L"Uuml"},
    {221, L"Yacute"},  {222, L"THORN"},   {223, L"szlig"},
    {224, L"agrave"},  {225, L"aacute"},  {226, L"acirc"},
    {227, L"atilde"},  {228, L"auml"},    {229, L"aring"},
    {230, L"aelig"},   {231, L"ccedil"},  {232, L"egrave"},
    {233, L"eacute"},  {234, L"ecirc"},   {235, L"euml"},
    {236, L"igrave"},  {237, L"iacute"},  {238, L"icirc"},
    {239, L"iuml"},    {240, L"eth"},     {241, L"ntilde"},
    {242, L"ograve"},  {243, L"oacute"},  {244, L"ocirc"},
    {245, L"otilde"},  {246, L"ouml"},    {247, L"divide"},
    {248, L"oslash"},  {249, L"ugrave"},  {250, L"uacute"},
    {251, L"ucirc"},   {252, L"uuml"},    {253, L"yacute"},
    {254, L"thorn"},   {255, L"yuml"},    {338, L"OElig"},
    {339, L"oelig"},   {352, L"Scaron"},  {353, L"saron"},
    {376, L"Yuml"},    {402, L"fnof"},    {710, L"circ"},
    {732, L"tilde"},   {913, L"Alpha"},   {914, L"Beta"},
    {915, L"Gamma"},   {916, L"Delta"},   {917, L"Epsilon"},
    {918, L"Zeta"},    {919, L"Eta"},     {920, L"Theta"},
    {921, L"lota"},    {922, L"Kappa"},   {923, L"Lambda"},
    {924, L"Mu"},      {925, L"Nu"},      {926, L"Xi"},
    {927, L"Omicron"}, {928, L"Pi"},      {929, L"Rho"},
    {931, L"Sigma"},   {932, L"Tau"},     {933, L"Upsilon"},
    {934, L"Phi"},     {935, L"Chi"},     {936, L"Psi"},
    {937, L"Omega"},   {945, L"alpha"},   {946, L"beta"},
    {947, L"gamma"},   {948, L"delta"},   {949, L"epsilon"},
    {950, L"zeta"},    {951, L"eta"},     {952, L"theta"},
    {953, L"iota"},    {954, L"kappa"},   {955, L"lambda"},
    {956, L"mu"},      {957, L"nu"},      {958, L"xi"},
    {959, L"omicron"}, {960, L"pi"},      {961, L"rho"},
    {962, L"sigmaf"},  {963, L"sigma"},   {964, L"tau"},
    {965, L"upsilon"}, {966, L"phi"},     {967, L"chi"},
    {968, L"psi"},     {969, L"omega"},   {977, L"thetasym"},
    {978, L"upsih"},   {982, L"piv"},     {8194, L"ensp"},
    {8195, L"emsp"},   {8201, L"thinsp"}, {8204, L"zwnj"},
    {8205, L"zwj"},    {8206, L"lrm"},    {8207, L"rlm"},
    {8211, L"ndash"},  {8212, L"mdash"},  {8216, L"lsquo"},
    {8217, L"rsquo"},  {8218, L"sbquo"},  {8220, L"ldquo"},
    {8221, L"tdquo"},  {8222, L"bdquo"},  {8224, L"dagger"},
    {8225, L"Dagger"}, {8226, L"bull"},   {8230, L"hellip"},
    {8240, L"permil"}, {8242, L"prime"},  {8249, L"lsaquo"},
    {8250, L"rsaquo"}, {8254, L"oline"},  {8260, L"frasl"},
    {8364, L"euro"},   {8465, L"image"},  {8472, L"weierp"},
    {8476, L"real"},   {8482, L"trade"},  {8501, L"alefsym"},
    {8592, L"larr"},   {8593, L"uarr"},   {8594, L"rarr"},
    {8595, L"darr"},   {8596, L"harr"},   {8629, L"crarr"},
    {8656, L"lArr"},   {8657, L"uArr"},   {8658, L"rArr"},
    {8659, L"dArr"},   {8660, L"hArr"},   {8704, L"forall"},
    {8706, L"part"},   {8707, L"exist"},  {8709, L"empty"},
    {8711, L"nabla"},  {8712, L"isin"},   {8713, L"notin"},
    {8715, L"ni"},     {8719, L"prod"},   {8721, L"sum"},
    {8722, L"minus"},  {8727, L"lowast"}, {8730, L"radic"},
    {8733, L"prop"},   {8734, L"infin"},  {8736, L"ang"},
    {8743, L"and"},    {8744, L"or"},     {8745, L"cap"},
    {8746, L"cup"},    {8747, L"int"},    {8756, L"there4"},
    {8764, L"sim"},    {8773, L"cong"},   {8776, L"asymp"},
    {8800, L"ne"},     {8801, L"equiv"},  {8804, L"le"},
    {8805, L"ge"},     {8834, L"sub"},    {8835, L"sup"},
    {8836, L"nsub"},   {8838, L"sube"},   {8839, L"supe"},
    {8853, L"oplus"},  {8855, L"otimes"}, {8869, L"perp"},
    {8901, L"sdot"},   {8968, L"lceil"},  {8969, L"rceil"},
    {8970, L"lfloor"}, {8971, L"rfloor"}, {9001, L"lang"},
    {9002, L"rang"},   {9674, L"loz"},    {9824, L"spades"},
    {9827, L"clubs"},  {9829, L"hearts"}, {9830, L"diams"},
};

const FXJSE_FUNCTION_DESCRIPTOR kFormCalcFM2JSFunctions[] = {
    {"Abs", CFXJSE_FormCalcContext::Abs},
    {"Avg", CFXJSE_FormCalcContext::Avg},
    {"Ceil", CFXJSE_FormCalcContext::Ceil},
    {"Count", CFXJSE_FormCalcContext::Count},
    {"Floor", CFXJSE_FormCalcContext::Floor},
    {"Max", CFXJSE_FormCalcContext::Max},
    {"Min", CFXJSE_FormCalcContext::Min},
    {"Mod", CFXJSE_FormCalcContext::Mod},
    {"Round", CFXJSE_FormCalcContext::Round},
    {"Sum", CFXJSE_FormCalcContext::Sum},
    {"Date", CFXJSE_FormCalcContext::Date},
    {"Date2Num", CFXJSE_FormCalcContext::Date2Num},
    {"DateFmt", CFXJSE_FormCalcContext::DateFmt},
    {"IsoDate2Num", CFXJSE_FormCalcContext::IsoDate2Num},
    {"IsoTime2Num", CFXJSE_FormCalcContext::IsoTime2Num},
    {"LocalDateFmt", CFXJSE_FormCalcContext::LocalDateFmt},
    {"LocalTimeFmt", CFXJSE_FormCalcContext::LocalTimeFmt},
    {"Num2Date", CFXJSE_FormCalcContext::Num2Date},
    {"Num2GMTime", CFXJSE_FormCalcContext::Num2GMTime},
    {"Num2Time", CFXJSE_FormCalcContext::Num2Time},
    {"Time", CFXJSE_FormCalcContext::Time},
    {"Time2Num", CFXJSE_FormCalcContext::Time2Num},
    {"TimeFmt", CFXJSE_FormCalcContext::TimeFmt},
    {"Apr", CFXJSE_FormCalcContext::Apr},
    {"Cterm", CFXJSE_FormCalcContext::CTerm},
    {"FV", CFXJSE_FormCalcContext::FV},
    {"Ipmt", CFXJSE_FormCalcContext::IPmt},
    {"NPV", CFXJSE_FormCalcContext::NPV},
    {"Pmt", CFXJSE_FormCalcContext::Pmt},
    {"PPmt", CFXJSE_FormCalcContext::PPmt},
    {"PV", CFXJSE_FormCalcContext::PV},
    {"Rate", CFXJSE_FormCalcContext::Rate},
    {"Term", CFXJSE_FormCalcContext::Term},
    {"Choose", CFXJSE_FormCalcContext::Choose},
    {"Exists", CFXJSE_FormCalcContext::Exists},
    {"HasValue", CFXJSE_FormCalcContext::HasValue},
    {"Oneof", CFXJSE_FormCalcContext::Oneof},
    {"Within", CFXJSE_FormCalcContext::Within},
    {"If", CFXJSE_FormCalcContext::If},
    {"Eval", CFXJSE_FormCalcContext::Eval},
    {"Translate", CFXJSE_FormCalcContext::eval_translation},
    {"Ref", CFXJSE_FormCalcContext::Ref},
    {"UnitType", CFXJSE_FormCalcContext::UnitType},
    {"UnitValue", CFXJSE_FormCalcContext::UnitValue},
    {"At", CFXJSE_FormCalcContext::At},
    {"Concat", CFXJSE_FormCalcContext::Concat},
    {"Decode", CFXJSE_FormCalcContext::Decode},
    {"Encode", CFXJSE_FormCalcContext::Encode},
    {"Format", CFXJSE_FormCalcContext::Format},
    {"Left", CFXJSE_FormCalcContext::Left},
    {"Len", CFXJSE_FormCalcContext::Len},
    {"Lower", CFXJSE_FormCalcContext::Lower},
    {"Ltrim", CFXJSE_FormCalcContext::Ltrim},
    {"Parse", CFXJSE_FormCalcContext::Parse},
    {"Replace", CFXJSE_FormCalcContext::Replace},
    {"Right", CFXJSE_FormCalcContext::Right},
    {"Rtrim", CFXJSE_FormCalcContext::Rtrim},
    {"Space", CFXJSE_FormCalcContext::Space},
    {"Str", CFXJSE_FormCalcContext::Str},
    {"Stuff", CFXJSE_FormCalcContext::Stuff},
    {"Substr", CFXJSE_FormCalcContext::Substr},
    {"Uuid", CFXJSE_FormCalcContext::Uuid},
    {"Upper", CFXJSE_FormCalcContext::Upper},
    {"WordNum", CFXJSE_FormCalcContext::WordNum},
    {"Get", CFXJSE_FormCalcContext::Get},
    {"Post", CFXJSE_FormCalcContext::Post},
    {"Put", CFXJSE_FormCalcContext::Put},
    {"pos_op", CFXJSE_FormCalcContext::positive_operator},
    {"neg_op", CFXJSE_FormCalcContext::negative_operator},
    {"log_or_op", CFXJSE_FormCalcContext::logical_or_operator},
    {"log_and_op", CFXJSE_FormCalcContext::logical_and_operator},
    {"log_not_op", CFXJSE_FormCalcContext::logical_not_operator},
    {"eq_op", CFXJSE_FormCalcContext::equality_operator},
    {"neq_op", CFXJSE_FormCalcContext::notequality_operator},
    {"lt_op", CFXJSE_FormCalcContext::less_operator},
    {"le_op", CFXJSE_FormCalcContext::lessequal_operator},
    {"gt_op", CFXJSE_FormCalcContext::greater_operator},
    {"ge_op", CFXJSE_FormCalcContext::greaterequal_operator},
    {"plus_op", CFXJSE_FormCalcContext::plus_operator},
    {"minus_op", CFXJSE_FormCalcContext::minus_operator},
    {"mul_op", CFXJSE_FormCalcContext::multiple_operator},
    {"div_op", CFXJSE_FormCalcContext::divide_operator},
    {"asgn_val_op", CFXJSE_FormCalcContext::assign_value_operator},
    {"dot_acc", CFXJSE_FormCalcContext::dot_accessor},
    {"dotdot_acc", CFXJSE_FormCalcContext::dotdot_accessor},
    {"concat_obj", CFXJSE_FormCalcContext::concat_fm_object},
    {"is_obj", CFXJSE_FormCalcContext::is_fm_object},
    {"is_ary", CFXJSE_FormCalcContext::is_fm_array},
    {"get_val", CFXJSE_FormCalcContext::get_fm_value},
    {"get_jsobj", CFXJSE_FormCalcContext::get_fm_jsobj},
    {"var_filter", CFXJSE_FormCalcContext::fm_var_filter},
};

const uint8_t kAltTableDate[] = {
    255, 255, 255, 3,   9,   255, 255, 255, 255, 255, 255,
    255, 2,   255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 1,   255, 255, 255, 255, 255, 255, 255, 255,
};
static_assert(FX_ArraySize(kAltTableDate) == L'a' - L'A' + 1,
              "Invalid kAltTableDate size.");

const uint8_t kAltTableTime[] = {
    14,  255, 255, 3,   9,   255, 255, 15,  255, 255, 255,
    255, 6,   255, 255, 255, 255, 255, 7,   255, 255, 255,
    255, 255, 1,   17,  255, 255, 255, 255, 255, 255, 255,
};
static_assert(FX_ArraySize(kAltTableTime) == L'a' - L'A' + 1,
              "Invalid kAltTableTime size.");

void AlternateDateTimeSymbols(WideString& wsPattern,
                              const WideString& wsAltSymbols,
                              const uint8_t* pAltTable) {
  int32_t nLength = wsPattern.GetLength();
  bool bInConstRange = false;
  bool bEscape = false;
  int32_t i = 0;
  while (i < nLength) {
    wchar_t wc = wsPattern[i];
    if (wc == L'\'') {
      bInConstRange = !bInConstRange;
      if (bEscape) {
        i++;
      } else {
        wsPattern.Delete(i);
        nLength--;
      }
      bEscape = !bEscape;
      continue;
    }
    if (!bInConstRange && wc >= L'A' && wc <= L'a') {
      uint8_t nAlt = pAltTable[wc - L'A'];
      if (nAlt != 255)
        wsPattern.SetAt(i, wsAltSymbols[nAlt]);
    }
    i++;
    bEscape = false;
  }
}

bool PatternStringType(const ByteStringView& szPattern, uint32_t& patternType) {
  WideString wsPattern = WideString::FromUTF8(szPattern);
  if (L"datetime" == wsPattern.Left(8)) {
    patternType = XFA_VT_DATETIME;
    return true;
  }
  if (L"date" == wsPattern.Left(4)) {
    auto pos = wsPattern.Find(L"time");
    patternType =
        pos.has_value() && pos.value() != 0 ? XFA_VT_DATETIME : XFA_VT_DATE;
    return true;
  }
  if (L"time" == wsPattern.Left(4)) {
    patternType = XFA_VT_TIME;
    return true;
  }
  if (L"text" == wsPattern.Left(4)) {
    patternType = XFA_VT_TEXT;
    return true;
  }
  if (L"num" == wsPattern.Left(3)) {
    if (L"integer" == wsPattern.Mid(4, 7)) {
      patternType = XFA_VT_INTEGER;
    } else if (L"decimal" == wsPattern.Mid(4, 7)) {
      patternType = XFA_VT_DECIMAL;
    } else if (L"currency" == wsPattern.Mid(4, 8)) {
      patternType = XFA_VT_FLOAT;
    } else if (L"percent" == wsPattern.Mid(4, 7)) {
      patternType = XFA_VT_FLOAT;
    } else {
      patternType = XFA_VT_FLOAT;
    }
    return true;
  }

  patternType = XFA_VT_NULL;
  wsPattern.MakeLower();
  const wchar_t* pData = wsPattern.c_str();
  int32_t iLength = wsPattern.GetLength();
  int32_t iIndex = 0;
  bool bSingleQuotation = false;
  wchar_t patternChar;
  while (iIndex < iLength) {
    patternChar = pData[iIndex];
    if (patternChar == 0x27) {
      bSingleQuotation = !bSingleQuotation;
    } else if (!bSingleQuotation &&
               (patternChar == 'y' || patternChar == 'j')) {
      patternType = XFA_VT_DATE;
      iIndex++;
      wchar_t timePatternChar;
      while (iIndex < iLength) {
        timePatternChar = pData[iIndex];
        if (timePatternChar == 0x27) {
          bSingleQuotation = !bSingleQuotation;
        } else if (!bSingleQuotation && timePatternChar == 't') {
          patternType = XFA_VT_DATETIME;
          break;
        }
        iIndex++;
      }
      break;
    } else if (!bSingleQuotation &&
               (patternChar == 'h' || patternChar == 'k')) {
      patternType = XFA_VT_TIME;
      break;
    } else if (!bSingleQuotation &&
               (patternChar == 'a' || patternChar == 'x' ||
                patternChar == 'o' || patternChar == '0')) {
      patternType = XFA_VT_TEXT;
      if (patternChar == 'x' || patternChar == 'o' || patternChar == '0') {
        break;
      }
    } else if (!bSingleQuotation &&
               (patternChar == 'z' || patternChar == 's' ||
                patternChar == 'e' || patternChar == 'v' ||
                patternChar == '8' || patternChar == ',' ||
                patternChar == '.' || patternChar == '$')) {
      patternType = XFA_VT_FLOAT;
      if (patternChar == 'v' || patternChar == '8' || patternChar == '$') {
        break;
      }
    }
    iIndex++;
  }
  if (patternType == XFA_VT_NULL) {
    patternType = XFA_VT_TEXT | XFA_VT_FLOAT;
  }
  return false;
}

CFXJSE_FormCalcContext* ToJSContext(CFXJSE_Value* pValue,
                                    CFXJSE_Class* pClass) {
  CFXJSE_HostObject* pHostObj = pValue->ToHostObject(pClass);
  if (!pHostObj || pHostObj->type() != CFXJSE_HostObject::kFM2JS)
    return nullptr;
  return static_cast<CFXJSE_FormCalcContext*>(pHostObj);
}

bool IsWhitespace(char c) {
  return c == 0x20 || c == 0x09 || c == 0x0B || c == 0x0C || c == 0x0A ||
         c == 0x0D;
}

LocaleIface* LocaleFromString(CXFA_Document* pDoc,
                              CXFA_LocaleMgr* pMgr,
                              const ByteStringView& szLocale) {
  if (!szLocale.IsEmpty())
    return pMgr->GetLocaleByName(WideString::FromUTF8(szLocale));

  CXFA_Node* pThisNode = ToNode(pDoc->GetScriptContext()->GetThisObject());
  ASSERT(pThisNode);
  return pThisNode->GetLocale();
}

WideString FormatFromString(LocaleIface* pLocale,
                            const ByteStringView& szFormat) {
  if (!szFormat.IsEmpty())
    return WideString::FromUTF8(szFormat);

  return pLocale->GetDatePattern(FX_LOCALEDATETIMESUBCATEGORY_Default);
}

FX_LOCALEDATETIMESUBCATEGORY SubCategoryFromInt(int32_t iStyle) {
  switch (iStyle) {
    case 1:
      return FX_LOCALEDATETIMESUBCATEGORY_Short;
    case 3:
      return FX_LOCALEDATETIMESUBCATEGORY_Long;
    case 4:
      return FX_LOCALEDATETIMESUBCATEGORY_Full;
    case 0:
    case 2:
    default:
      return FX_LOCALEDATETIMESUBCATEGORY_Medium;
  }
}

bool IsPartOfNumber(char ch) {
  return std::isdigit(ch) || ch == '-' || ch == '.';
}

bool IsPartOfNumberW(wchar_t ch) {
  return std::iswdigit(ch) || ch == L'-' || ch == L'.';
}

ByteString GUIDString(bool bSeparator) {
  uint8_t data[16];
  FX_Random_GenerateMT(reinterpret_cast<uint32_t*>(data), 4);
  data[6] = (data[6] & 0x0F) | 0x40;

  ByteString bsStr;
  {
    // Span's lifetime must end before ReleaseBuffer() below.
    pdfium::span<char> pBuf = bsStr.GetBuffer(40);
    size_t out_index = 0;
    for (size_t i = 0; i < 16; ++i, out_index += 2) {
      if (bSeparator && (i == 4 || i == 6 || i == 8 || i == 10))
        pBuf[out_index++] = L'-';

      FXSYS_IntToTwoHexChars(data[i], &pBuf[out_index]);
    }
  }
  bsStr.ReleaseBuffer(bSeparator ? 36 : 32);
  return bsStr;
}

double ByteStringToDouble(const ByteStringView& szStringVal) {
  WideString wsValue = WideString::FromUTF8(szStringVal);
  wsValue.Trim();

  int32_t cc = 0;
  bool bNegative = false;

  const wchar_t* str = wsValue.c_str();
  int32_t len = wsValue.GetLength();
  if (str[0] == '+') {
    cc++;
  } else if (str[0] == '-') {
    bNegative = true;
    cc++;
  }

  int32_t nIntegralLen = 0;
  int64_t nIntegral = 0;
  while (cc < len) {
    if (str[cc] == '.' || str[cc] == 'E' || str[cc] == 'e' ||
        nIntegralLen > 17) {
      break;
    }
    if (!FXSYS_isDecimalDigit(str[cc])) {
      return 0;
    }
    nIntegral = nIntegral * 10 + str[cc] - '0';
    cc++;
    nIntegralLen++;
  }
  nIntegral = bNegative ? -nIntegral : nIntegral;

  int32_t scale = 0;
  double fraction = 0.0;
  uint32_t dwFractional = 0;
  if (cc < len && str[cc] == '.') {
    cc++;
    while (cc < len) {
      fraction += XFA_GetFractionalScale(scale) * (str[cc] - '0');
      scale++;
      cc++;
      if (cc == len)
        break;
      if (scale == XFA_GetMaxFractionalScale() || str[cc] == 'E' ||
          str[cc] == 'e') {
        break;
      }
      if (!FXSYS_isDecimalDigit(str[cc]))
        return 0;
    }
    dwFractional = static_cast<uint32_t>(fraction * 4294967296.0);
  }

  int32_t nExponent = 0;
  bool bExpSign = false;
  if (cc < len && (str[cc] == 'E' || str[cc] == 'e')) {
    cc++;
    if (cc < len) {
      if (str[cc] == '+') {
        cc++;
      } else if (str[cc] == '-') {
        bExpSign = true;
        cc++;
      }
    }
    while (cc < len) {
      if (str[cc] == '.' || !FXSYS_isDecimalDigit(str[cc]))
        return 0;

      nExponent = nExponent * 10 + str[cc] - '0';
      cc++;
    }
    nExponent = bExpSign ? -nExponent : nExponent;
  }

  double dValue = dwFractional / 4294967296.0;
  dValue = nIntegral + (nIntegral >= 0 ? dValue : -dValue);
  if (nExponent != 0)
    dValue *= FXSYS_pow(10, static_cast<float>(nExponent));

  return dValue;
}

}  // namespace

const FXJSE_CLASS_DESCRIPTOR kFormCalcFM2JSDescriptor = {
    "XFA_FM2JS_FormCalcClass",              // name
    kFormCalcFM2JSFunctions,                // methods
    FX_ArraySize(kFormCalcFM2JSFunctions),  // number of methods
    nullptr,                                // dynamic prop type
    nullptr,                                // dynamic prop getter
    nullptr,                                // dynamic prop setter
    nullptr,                                // dynamic prop method call
};

// static
void CFXJSE_FormCalcContext::Abs(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Abs");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double dValue = ValueToDouble(pThis, argOne.get());
  if (dValue < 0)
    dValue = -dValue;

  args.GetReturnValue()->SetDouble(dValue);
}

// static
void CFXJSE_FormCalcContext::Avg(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1) {
    args.GetReturnValue()->SetNull();
    return;
  }

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  uint32_t uCount = 0;
  double dSum = 0.0;
  for (int32_t i = 0; i < argc; i++) {
    std::unique_ptr<CFXJSE_Value> argValue = args.GetValue(i);
    if (argValue->IsNull())
      continue;

    if (!argValue->IsArray()) {
      dSum += ValueToDouble(pThis, argValue.get());
      uCount++;
      continue;
    }

    auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argValue->GetObjectProperty("length", lengthValue.get());
    int32_t iLength = lengthValue->ToInteger();

    if (iLength > 2) {
      auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectPropertyByIdx(1, propertyValue.get());

      auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      if (propertyValue->IsNull()) {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          auto defaultPropValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
          GetObjectDefaultValue(jsObjectValue.get(), defaultPropValue.get());
          if (defaultPropValue->IsNull())
            continue;

          dSum += ValueToDouble(pThis, defaultPropValue.get());
          uCount++;
        }
      } else {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          dSum += ValueToDouble(pThis, newPropertyValue.get());
          uCount++;
        }
      }
    }
  }
  if (uCount == 0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetDouble(dSum / uCount);
}

// static
void CFXJSE_FormCalcContext::Ceil(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Ceil");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argValue = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argValue.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetFloat(ceil(ValueToFloat(pThis, argValue.get())));
}

// static
void CFXJSE_FormCalcContext::Count(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  int32_t iCount = 0;
  for (int32_t i = 0; i < args.GetLength(); i++) {
    std::unique_ptr<CFXJSE_Value> argValue = args.GetValue(i);
    if (argValue->IsNull())
      continue;

    if (argValue->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectProperty("length", lengthValue.get());

      int32_t iLength = lengthValue->ToInteger();
      if (iLength <= 2) {
        pContext->ThrowArgumentMismatchException();
        return;
      }

      auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectPropertyByIdx(1, propertyValue.get());
      argValue->GetObjectPropertyByIdx(2, jsObjectValue.get());
      if (propertyValue->IsNull()) {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
          if (!newPropertyValue->IsNull())
            iCount++;
        }
      } else {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(), newPropertyValue.get());
          iCount += newPropertyValue->IsNull() ? 0 : 1;
        }
      }
    } else if (argValue->IsObject()) {
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      GetObjectDefaultValue(argValue.get(), newPropertyValue.get());
      if (!newPropertyValue->IsNull())
        iCount++;
    } else {
      iCount++;
    }
  }
  args.GetReturnValue()->SetInteger(iCount);
}

// static
void CFXJSE_FormCalcContext::Floor(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Floor");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argValue = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argValue.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetFloat(floor(ValueToFloat(pThis, argValue.get())));
}

// static
void CFXJSE_FormCalcContext::Max(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  uint32_t uCount = 0;
  double dMaxValue = 0.0;
  for (int32_t i = 0; i < args.GetLength(); i++) {
    std::unique_ptr<CFXJSE_Value> argValue = args.GetValue(i);
    if (argValue->IsNull())
      continue;

    if (argValue->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectProperty("length", lengthValue.get());
      int32_t iLength = lengthValue->ToInteger();
      if (iLength <= 2) {
        pContext->ThrowArgumentMismatchException();
        return;
      }

      auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectPropertyByIdx(1, propertyValue.get());
      argValue->GetObjectPropertyByIdx(2, jsObjectValue.get());
      if (propertyValue->IsNull()) {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          uCount++;
          double dValue = ValueToDouble(pThis, newPropertyValue.get());
          dMaxValue = (uCount == 1) ? dValue : std::max(dMaxValue, dValue);
        }
      } else {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          uCount++;
          double dValue = ValueToDouble(pThis, newPropertyValue.get());
          dMaxValue = (uCount == 1) ? dValue : std::max(dMaxValue, dValue);
        }
      }
    } else if (argValue->IsObject()) {
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      GetObjectDefaultValue(argValue.get(), newPropertyValue.get());
      if (newPropertyValue->IsNull())
        continue;

      uCount++;
      double dValue = ValueToDouble(pThis, newPropertyValue.get());
      dMaxValue = (uCount == 1) ? dValue : std::max(dMaxValue, dValue);
    } else {
      uCount++;
      double dValue = ValueToDouble(pThis, argValue.get());
      dMaxValue = (uCount == 1) ? dValue : std::max(dMaxValue, dValue);
    }
  }
  if (uCount == 0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetDouble(dMaxValue);
}

// static
void CFXJSE_FormCalcContext::Min(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  uint32_t uCount = 0;
  double dMinValue = 0.0;
  for (int32_t i = 0; i < args.GetLength(); i++) {
    std::unique_ptr<CFXJSE_Value> argValue = args.GetValue(i);
    if (argValue->IsNull())
      continue;

    if (argValue->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectProperty("length", lengthValue.get());
      int32_t iLength = lengthValue->ToInteger();
      if (iLength <= 2) {
        pContext->ThrowArgumentMismatchException();
        return;
      }

      auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectPropertyByIdx(1, propertyValue.get());
      argValue->GetObjectPropertyByIdx(2, jsObjectValue.get());
      if (propertyValue->IsNull()) {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          uCount++;
          double dValue = ValueToDouble(pThis, newPropertyValue.get());
          dMinValue = uCount == 1 ? dValue : std::min(dMinValue, dValue);
        }
      } else {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          uCount++;
          double dValue = ValueToDouble(pThis, newPropertyValue.get());
          dMinValue = uCount == 1 ? dValue : std::min(dMinValue, dValue);
        }
      }
    } else if (argValue->IsObject()) {
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      GetObjectDefaultValue(argValue.get(), newPropertyValue.get());
      if (newPropertyValue->IsNull())
        continue;

      uCount++;
      double dValue = ValueToDouble(pThis, newPropertyValue.get());
      dMinValue = uCount == 1 ? dValue : std::min(dMinValue, dValue);
    } else {
      uCount++;
      double dValue = ValueToDouble(pThis, argValue.get());
      dMinValue = uCount == 1 ? dValue : std::min(dMinValue, dValue);
    }
  }
  if (uCount == 0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetDouble(dMinValue);
}

// static
void CFXJSE_FormCalcContext::Mod(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 2) {
    pContext->ThrowParamCountMismatchException(L"Mod");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  std::unique_ptr<CFXJSE_Value> argTwo = args.GetValue(1);
  if (argOne->IsNull() || argTwo->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  bool argOneResult;
  double dDividend = ExtractDouble(pThis, argOne.get(), &argOneResult);
  bool argTwoResult;
  double dDivisor = ExtractDouble(pThis, argTwo.get(), &argTwoResult);
  if (!argOneResult || !argTwoResult) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  if (dDivisor == 0.0) {
    pContext->ThrowDivideByZeroException();
    return;
  }

  args.GetReturnValue()->SetDouble(dDividend -
                                   dDivisor * (int32_t)(dDividend / dDivisor));
}

// static
void CFXJSE_FormCalcContext::Round(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 2) {
    pContext->ThrowParamCountMismatchException(L"Round");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  bool dValueRet;
  double dValue = ExtractDouble(pThis, argOne.get(), &dValueRet);
  if (!dValueRet) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  uint8_t uPrecision = 0;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> argTwo = args.GetValue(1);
    if (argTwo->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }

    bool dPrecisionRet;
    double dPrecision = ExtractDouble(pThis, argTwo.get(), &dPrecisionRet);
    if (!dPrecisionRet) {
      pContext->ThrowArgumentMismatchException();
      return;
    }

    uPrecision = static_cast<uint8_t>(pdfium::clamp(dPrecision, 0.0, 12.0));
  }

  CFX_Decimal decimalValue(static_cast<float>(dValue), uPrecision);
  args.GetReturnValue()->SetDouble(decimalValue);
}

// static
void CFXJSE_FormCalcContext::Sum(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc == 0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  uint32_t uCount = 0;
  double dSum = 0.0;
  for (int32_t i = 0; i < argc; i++) {
    std::unique_ptr<CFXJSE_Value> argValue = args.GetValue(i);
    if (argValue->IsNull())
      continue;

    if (argValue->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectProperty("length", lengthValue.get());
      int32_t iLength = lengthValue->ToInteger();
      if (iLength <= 2) {
        pContext->ThrowArgumentMismatchException();
        return;
      }

      auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValue->GetObjectPropertyByIdx(1, propertyValue.get());
      auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      if (propertyValue->IsNull()) {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          dSum += ValueToDouble(pThis, jsObjectValue.get());
          uCount++;
        }
      } else {
        for (int32_t j = 2; j < iLength; j++) {
          argValue->GetObjectPropertyByIdx(j, jsObjectValue.get());
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(), newPropertyValue.get());
          if (newPropertyValue->IsNull())
            continue;

          dSum += ValueToDouble(pThis, newPropertyValue.get());
          uCount++;
        }
      }
    } else if (argValue->IsObject()) {
      auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      GetObjectDefaultValue(argValue.get(), newPropertyValue.get());
      if (newPropertyValue->IsNull())
        continue;

      dSum += ValueToDouble(pThis, argValue.get());
      uCount++;
    } else {
      dSum += ValueToDouble(pThis, argValue.get());
      uCount++;
    }
  }
  if (uCount == 0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetDouble(dSum);
}

// static
void CFXJSE_FormCalcContext::Date(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  if (args.GetLength() != 0) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Date");
    return;
  }

  time_t currentTime;
  time(&currentTime);
  struct tm* pTmStruct = gmtime(&currentTime);

  args.GetReturnValue()->SetInteger(DateString2Num(
      ByteString::Format("%d%02d%02d", pTmStruct->tm_year + 1900,
                         pTmStruct->tm_mon + 1, pTmStruct->tm_mday)
          .AsStringView()));
}

// static
void CFXJSE_FormCalcContext::Date2Num(CFXJSE_Value* pThis,
                                      const ByteStringView& szFuncName,
                                      CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Date2Num");
    return;
  }

  std::unique_ptr<CFXJSE_Value> dateValue = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, dateValue.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString dateString = ValueToUTF8String(dateValue.get());
  ByteString formatString;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> formatValue = GetSimpleValue(pThis, args, 1);
    if (ValueIsNull(pThis, formatValue.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
    formatString = ValueToUTF8String(formatValue.get());
  }

  ByteString localString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> localValue = GetSimpleValue(pThis, args, 2);
    if (ValueIsNull(pThis, localValue.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
    localString = ValueToUTF8String(localValue.get());
  }

  ByteString szIsoDateString =
      Local2IsoDate(pThis, dateString.AsStringView(),
                    formatString.AsStringView(), localString.AsStringView());
  args.GetReturnValue()->SetInteger(
      DateString2Num(szIsoDateString.AsStringView()));
}

// static
void CFXJSE_FormCalcContext::DateFmt(CFXJSE_Value* pThis,
                                     const ByteStringView& szFuncName,
                                     CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Date2Num");
    return;
  }

  int32_t iStyle = 0;
  if (argc > 0) {
    std::unique_ptr<CFXJSE_Value> argStyle = GetSimpleValue(pThis, args, 0);
    if (argStyle->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }

    iStyle = (int32_t)ValueToFloat(pThis, argStyle.get());
    if (iStyle < 0 || iStyle > 4)
      iStyle = 0;
  }

  ByteString szLocal;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> argLocal = GetSimpleValue(pThis, args, 1);
    if (argLocal->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    szLocal = ValueToUTF8String(argLocal.get());
  }

  ByteString formatStr =
      GetStandardDateFormat(pThis, iStyle, szLocal.AsStringView());
  args.GetReturnValue()->SetString(formatStr.AsStringView());
}

// static
void CFXJSE_FormCalcContext::IsoDate2Num(CFXJSE_Value* pThis,
                                         const ByteStringView& szFuncName,
                                         CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)
        ->ThrowParamCountMismatchException(L"IsoDate2Num");
    return;
  }
  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  ByteString szArgString = ValueToUTF8String(argOne.get());
  args.GetReturnValue()->SetInteger(DateString2Num(szArgString.AsStringView()));
}

// static
void CFXJSE_FormCalcContext::IsoTime2Num(CFXJSE_Value* pThis,
                                         const ByteStringView& szFuncName,
                                         CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 1) {
    pContext->ThrowParamCountMismatchException(L"IsoTime2Num");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  CXFA_Document* pDoc = pContext->GetDocument();
  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  ByteString szArgString = ValueToUTF8String(argOne.get());
  auto pos = szArgString.Find('T', 0);
  if (!pos.has_value() || pos.value() == szArgString.GetLength() - 1) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }
  szArgString = szArgString.Right(szArgString.GetLength() - (pos.value() + 1));

  CXFA_LocaleValue timeValue(
      XFA_VT_TIME, WideString::FromUTF8(szArgString.AsStringView()), pMgr);
  if (!timeValue.IsValid()) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }

  CFX_DateTime uniTime = timeValue.GetTime();
  int32_t hour = uniTime.GetHour();
  int32_t min = uniTime.GetMinute();
  int32_t second = uniTime.GetSecond();
  int32_t milSecond = uniTime.GetMillisecond();

  // TODO(dsinclair): See if there is other time conversion code in pdfium and
  //   consolidate.
  int32_t mins = hour * 60 + min;
  mins -= (pMgr->GetDefLocale()->GetTimeZone().tzHour * 60);
  while (mins > 1440)
    mins -= 1440;
  while (mins < 0)
    mins += 1440;
  hour = mins / 60;
  min = mins % 60;

  args.GetReturnValue()->SetInteger(hour * 3600000 + min * 60000 +
                                    second * 1000 + milSecond + 1);
}

// static
void CFXJSE_FormCalcContext::LocalDateFmt(CFXJSE_Value* pThis,
                                          const ByteStringView& szFuncName,
                                          CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc > 2) {
    ToJSContext(pThis, nullptr)
        ->ThrowParamCountMismatchException(L"LocalDateFmt");
    return;
  }

  int32_t iStyle = 0;
  if (argc > 0) {
    std::unique_ptr<CFXJSE_Value> argStyle = GetSimpleValue(pThis, args, 0);
    if (argStyle->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    iStyle = (int32_t)ValueToFloat(pThis, argStyle.get());
    if (iStyle > 4 || iStyle < 0)
      iStyle = 0;
  }

  ByteString szLocal;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> argLocal = GetSimpleValue(pThis, args, 1);
    if (argLocal->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    szLocal = ValueToUTF8String(argLocal.get());
  }

  ByteString formatStr =
      GetLocalDateFormat(pThis, iStyle, szLocal.AsStringView(), false);
  args.GetReturnValue()->SetString(formatStr.AsStringView());
}

// static
void CFXJSE_FormCalcContext::LocalTimeFmt(CFXJSE_Value* pThis,
                                          const ByteStringView& szFuncName,
                                          CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc > 2) {
    ToJSContext(pThis, nullptr)
        ->ThrowParamCountMismatchException(L"LocalTimeFmt");
    return;
  }

  int32_t iStyle = 0;
  if (argc > 0) {
    std::unique_ptr<CFXJSE_Value> argStyle = GetSimpleValue(pThis, args, 0);
    if (argStyle->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    iStyle = (int32_t)ValueToFloat(pThis, argStyle.get());
    if (iStyle > 4 || iStyle < 0)
      iStyle = 0;
  }

  ByteString szLocal;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> argLocal = GetSimpleValue(pThis, args, 1);
    if (argLocal->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    szLocal = ValueToUTF8String(argLocal.get());
  }

  ByteString formatStr =
      GetLocalTimeFormat(pThis, iStyle, szLocal.AsStringView(), false);
  args.GetReturnValue()->SetString(formatStr.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Num2Date(CFXJSE_Value* pThis,
                                      const ByteStringView& szFuncName,
                                      CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Num2Date");
    return;
  }

  std::unique_ptr<CFXJSE_Value> dateValue = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, dateValue.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }
  int32_t dDate = (int32_t)ValueToFloat(pThis, dateValue.get());
  if (dDate < 1) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString formatString;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> formatValue = GetSimpleValue(pThis, args, 1);
    if (ValueIsNull(pThis, formatValue.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
    formatString = ValueToUTF8String(formatValue.get());
  }

  ByteString localString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> localValue = GetSimpleValue(pThis, args, 2);
    if (ValueIsNull(pThis, localValue.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
    localString = ValueToUTF8String(localValue.get());
  }

  int32_t iYear = 1900;
  int32_t iMonth = 1;
  int32_t iDay = 1;
  int32_t i = 0;
  while (dDate > 0) {
    if (iMonth == 2) {
      if ((!((iYear + i) % 4) && ((iYear + i) % 100)) || !((iYear + i) % 400)) {
        if (dDate > 29) {
          ++iMonth;
          if (iMonth > 12) {
            iMonth = 1;
            ++i;
          }
          iDay = 1;
          dDate -= 29;
        } else {
          iDay += static_cast<int32_t>(dDate) - 1;
          dDate = 0;
        }
      } else {
        if (dDate > 28) {
          ++iMonth;
          if (iMonth > 12) {
            iMonth = 1;
            ++i;
          }
          iDay = 1;
          dDate -= 28;
        } else {
          iDay += static_cast<int32_t>(dDate) - 1;
          dDate = 0;
        }
      }
    } else if (iMonth < 8) {
      if ((iMonth % 2 == 0)) {
        if (dDate > 30) {
          ++iMonth;
          if (iMonth > 12) {
            iMonth = 1;
            ++i;
          }
          iDay = 1;
          dDate -= 30;
        } else {
          iDay += static_cast<int32_t>(dDate) - 1;
          dDate = 0;
        }
      } else {
        if (dDate > 31) {
          ++iMonth;
          if (iMonth > 12) {
            iMonth = 1;
            ++i;
          }
          iDay = 1;
          dDate -= 31;
        } else {
          iDay += static_cast<int32_t>(dDate) - 1;
          dDate = 0;
        }
      }
    } else {
      if (iMonth % 2 != 0) {
        if (dDate > 30) {
          ++iMonth;
          if (iMonth > 12) {
            iMonth = 1;
            ++i;
          }
          iDay = 1;
          dDate -= 30;
        } else {
          iDay += static_cast<int32_t>(dDate) - 1;
          dDate = 0;
        }
      } else {
        if (dDate > 31) {
          ++iMonth;
          if (iMonth > 12) {
            iMonth = 1;
            ++i;
          }
          iDay = 1;
          dDate -= 31;
        } else {
          iDay += static_cast<int32_t>(dDate) - 1;
          dDate = 0;
        }
      }
    }
  }

  ByteString szLocalDateString = IsoDate2Local(
      pThis,
      ByteString::Format("%d%02d%02d", iYear + i, iMonth, iDay).AsStringView(),
      formatString.AsStringView(), localString.AsStringView());
  args.GetReturnValue()->SetString(szLocalDateString.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Num2GMTime(CFXJSE_Value* pThis,
                                        const ByteStringView& szFuncName,
                                        CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)
        ->ThrowParamCountMismatchException(L"Num2GMTime");
    return;
  }

  std::unique_ptr<CFXJSE_Value> timeValue = GetSimpleValue(pThis, args, 0);
  if (timeValue->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  int32_t iTime = (int32_t)ValueToFloat(pThis, timeValue.get());
  if (abs(iTime) < 1.0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString formatString;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> formatValue = GetSimpleValue(pThis, args, 1);
    if (formatValue->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    formatString = ValueToUTF8String(formatValue.get());
  }

  ByteString localString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> localValue = GetSimpleValue(pThis, args, 2);
    if (localValue->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    localString = ValueToUTF8String(localValue.get());
  }

  ByteString szGMTTimeString =
      Num2AllTime(pThis, iTime, formatString.AsStringView(),
                  localString.AsStringView(), true);
  args.GetReturnValue()->SetString(szGMTTimeString.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Num2Time(CFXJSE_Value* pThis,
                                      const ByteStringView& szFuncName,
                                      CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Num2Time");
    return;
  }

  std::unique_ptr<CFXJSE_Value> timeValue = GetSimpleValue(pThis, args, 0);
  if (timeValue->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  float fTime = ValueToFloat(pThis, timeValue.get());
  if (fabs(fTime) < 1.0) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString formatString;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> formatValue = GetSimpleValue(pThis, args, 1);
    if (formatValue->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    formatString = ValueToUTF8String(formatValue.get());
  }

  ByteString localString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> localValue = GetSimpleValue(pThis, args, 2);
    if (localValue->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    localString = ValueToUTF8String(localValue.get());
  }

  ByteString szLocalTimeString = Num2AllTime(pThis, static_cast<int32_t>(fTime),
                                             formatString.AsStringView(),
                                             localString.AsStringView(), false);
  args.GetReturnValue()->SetString(szLocalTimeString.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Time(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  if (args.GetLength() != 0) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Time");
    return;
  }

  time_t now;
  time(&now);

  struct tm* pGmt = gmtime(&now);
  args.GetReturnValue()->SetInteger(
      (pGmt->tm_hour * 3600 + pGmt->tm_min * 60 + pGmt->tm_sec) * 1000);
}

// static
void CFXJSE_FormCalcContext::Time2Num(CFXJSE_Value* pThis,
                                      const ByteStringView& szFuncName,
                                      CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Time2Num");
    return;
  }

  ByteString timeString;
  std::unique_ptr<CFXJSE_Value> timeValue = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, timeValue.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }
  timeString = ValueToUTF8String(timeValue.get());

  ByteString formatString;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> formatValue = GetSimpleValue(pThis, args, 1);
    if (ValueIsNull(pThis, formatValue.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
    formatString = ValueToUTF8String(formatValue.get());
  }

  ByteString localString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> localValue = GetSimpleValue(pThis, args, 2);
    if (ValueIsNull(pThis, localValue.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
    localString = ValueToUTF8String(localValue.get());
  }

  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  LocaleIface* pLocale = nullptr;
  if (localString.IsEmpty()) {
    CXFA_Node* pThisNode = ToNode(pDoc->GetScriptContext()->GetThisObject());
    ASSERT(pThisNode);
    pLocale = pThisNode->GetLocale();
  } else {
    pLocale =
        pMgr->GetLocaleByName(WideString::FromUTF8(localString.AsStringView()));
  }

  WideString wsFormat;
  if (formatString.IsEmpty())
    wsFormat = pLocale->GetTimePattern(FX_LOCALEDATETIMESUBCATEGORY_Default);
  else
    wsFormat = WideString::FromUTF8(formatString.AsStringView());

  wsFormat = L"time{" + wsFormat + L"}";
  CXFA_LocaleValue localeValue(XFA_VT_TIME,
                               WideString::FromUTF8(timeString.AsStringView()),
                               wsFormat, pLocale, pMgr);
  if (!localeValue.IsValid()) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }

  CFX_DateTime uniTime = localeValue.GetTime();
  int32_t hour = uniTime.GetHour();
  int32_t min = uniTime.GetMinute();
  int32_t second = uniTime.GetSecond();
  int32_t milSecond = uniTime.GetMillisecond();
  int32_t mins = hour * 60 + min;

  mins -= (CXFA_TimeZoneProvider().GetTimeZone().tzHour * 60);
  while (mins > 1440)
    mins -= 1440;

  while (mins < 0)
    mins += 1440;

  hour = mins / 60;
  min = mins % 60;
  args.GetReturnValue()->SetInteger(hour * 3600000 + min * 60000 +
                                    second * 1000 + milSecond + 1);
}

// static
void CFXJSE_FormCalcContext::TimeFmt(CFXJSE_Value* pThis,
                                     const ByteStringView& szFuncName,
                                     CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"TimeFmt");
    return;
  }

  int32_t iStyle = 0;
  if (argc > 0) {
    std::unique_ptr<CFXJSE_Value> argStyle = GetSimpleValue(pThis, args, 0);
    if (argStyle->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    iStyle = (int32_t)ValueToFloat(pThis, argStyle.get());
    if (iStyle > 4 || iStyle < 0)
      iStyle = 0;
  }

  ByteString szLocal;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> argLocal = GetSimpleValue(pThis, args, 1);
    if (argLocal->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    szLocal = ValueToUTF8String(argLocal.get());
  }

  ByteString formatStr =
      GetStandardTimeFormat(pThis, iStyle, szLocal.AsStringView());
  args.GetReturnValue()->SetString(formatStr.AsStringView());
}

// static
bool CFXJSE_FormCalcContext::IsIsoDateFormat(const char* pData,
                                             int32_t iLength,
                                             int32_t& iStyle,
                                             int32_t& iYear,
                                             int32_t& iMonth,
                                             int32_t& iDay) {
  iYear = 0;
  iMonth = 1;
  iDay = 1;

  if (iLength < 4)
    return false;

  char strYear[5];
  strYear[4] = '\0';
  for (int32_t i = 0; i < 4; ++i) {
    if (!std::isdigit(pData[i]))
      return false;

    strYear[i] = pData[i];
  }
  iYear = FXSYS_atoi(strYear);
  iStyle = 0;
  if (iLength == 4)
    return true;

  iStyle = pData[4] == '-' ? 1 : 0;

  char strTemp[3];
  strTemp[2] = '\0';
  int32_t iPosOff = iStyle == 0 ? 4 : 5;
  if (!std::isdigit(pData[iPosOff]) || !std::isdigit(pData[iPosOff + 1]))
    return false;

  strTemp[0] = pData[iPosOff];
  strTemp[1] = pData[iPosOff + 1];
  iMonth = FXSYS_atoi(strTemp);
  if (iMonth > 12 || iMonth < 1)
    return false;

  if (iStyle == 0) {
    iPosOff += 2;
    if (iLength == 6)
      return true;
  } else {
    iPosOff += 3;
    if (iLength == 7)
      return true;
  }
  if (!std::isdigit(pData[iPosOff]) || !std::isdigit(pData[iPosOff + 1]))
    return false;

  strTemp[0] = pData[iPosOff];
  strTemp[1] = pData[iPosOff + 1];
  iDay = FXSYS_atoi(strTemp);
  if (iPosOff + 2 < iLength)
    return false;

  if ((!(iYear % 4) && (iYear % 100)) || !(iYear % 400)) {
    if (iMonth == 2 && iDay > 29)
      return false;
  } else {
    if (iMonth == 2 && iDay > 28)
      return false;
  }
  if (iMonth != 2) {
    if (iMonth < 8) {
      if (iDay > (iMonth % 2 == 0 ? 30 : 31))
        return false;
    } else if (iDay > (iMonth % 2 == 0 ? 31 : 30)) {
      return false;
    }
  }
  return true;
}

// static
bool CFXJSE_FormCalcContext::IsIsoTimeFormat(const char* pData,
                                             int32_t iLength,
                                             int32_t& iHour,
                                             int32_t& iMinute,
                                             int32_t& iSecond,
                                             int32_t& iMilliSecond,
                                             int32_t& iZoneHour,
                                             int32_t& iZoneMinute) {
  iHour = 0;
  iMinute = 0;
  iSecond = 0;
  iMilliSecond = 0;
  iZoneHour = 0;
  iZoneMinute = 0;
  if (!pData)
    return false;

  char strTemp[3];
  strTemp[2] = '\0';
  int32_t iZone = 0;
  int32_t i = 0;
  while (i < iLength) {
    if (!std::isdigit(pData[i]) && pData[i] != ':') {
      iZone = i;
      break;
    }
    ++i;
  }
  if (i == iLength)
    iZone = iLength;

  int32_t iPos = 0;
  int32_t iIndex = 0;
  while (iIndex < iZone) {
    if (!std::isdigit(pData[iIndex]))
      return false;

    strTemp[0] = pData[iIndex];
    if (!std::isdigit(pData[iIndex + 1]))
      return false;

    strTemp[1] = pData[iIndex + 1];
    if (FXSYS_atoi(strTemp) > 60)
      return false;

    if (pData[2] == ':') {
      if (iPos == 0) {
        iHour = FXSYS_atoi(strTemp);
        ++iPos;
      } else if (iPos == 1) {
        iMinute = FXSYS_atoi(strTemp);
        ++iPos;
      } else {
        iSecond = FXSYS_atoi(strTemp);
      }
      iIndex += 3;
    } else {
      if (iPos == 0) {
        iHour = FXSYS_atoi(strTemp);
        ++iPos;
      } else if (iPos == 1) {
        iMinute = FXSYS_atoi(strTemp);
        ++iPos;
      } else if (iPos == 2) {
        iSecond = FXSYS_atoi(strTemp);
        ++iPos;
      }
      iIndex += 2;
    }
  }

  if (iIndex < iLength && pData[iIndex] == '.') {
    constexpr int kSubSecondLength = 3;
    if (iIndex + kSubSecondLength >= iLength)
      return false;

    ++iIndex;
    char strSec[kSubSecondLength + 1];
    for (int i = 0; i < kSubSecondLength; ++i) {
      char c = pData[iIndex + i];
      if (!std::isdigit(c))
        return false;
      strSec[i] = c;
    }
    strSec[kSubSecondLength] = '\0';

    iMilliSecond = FXSYS_atoi(strSec);
    if (iMilliSecond > 100) {
      iMilliSecond = 0;
      return false;
    }
    iIndex += kSubSecondLength;
  }

  if (iIndex < iLength && FXSYS_towlower(pData[iIndex]) == 'z')
    return true;

  int32_t iSign = 1;
  if (iIndex < iLength) {
    if (pData[iIndex] == '+') {
      ++iIndex;
    } else if (pData[iIndex] == '-') {
      iSign = -1;
      ++iIndex;
    }
  }
  iPos = 0;
  while (iIndex < iLength) {
    if (!std::isdigit(pData[iIndex]))
      return false;

    strTemp[0] = pData[iIndex];
    if (!std::isdigit(pData[iIndex + 1]))
      return false;

    strTemp[1] = pData[iIndex + 1];
    if (FXSYS_atoi(strTemp) > 60)
      return false;

    if (pData[2] == ':') {
      if (iPos == 0) {
        iZoneHour = FXSYS_atoi(strTemp);
      } else if (iPos == 1) {
        iZoneMinute = FXSYS_atoi(strTemp);
      }
      iIndex += 3;
    } else {
      if (!iPos) {
        iZoneHour = FXSYS_atoi(strTemp);
        ++iPos;
      } else if (iPos == 1) {
        iZoneMinute = FXSYS_atoi(strTemp);
        ++iPos;
      }
      iIndex += 2;
    }
  }
  if (iIndex < iLength)
    return false;

  iZoneHour *= iSign;
  return true;
}

// static
bool CFXJSE_FormCalcContext::IsIsoDateTimeFormat(const char* pData,
                                                 int32_t iLength,
                                                 int32_t& iYear,
                                                 int32_t& iMonth,
                                                 int32_t& iDay,
                                                 int32_t& iHour,
                                                 int32_t& iMinute,
                                                 int32_t& iSecond,
                                                 int32_t& iMillionSecond,
                                                 int32_t& iZoneHour,
                                                 int32_t& iZoneMinute) {
  iYear = 0;
  iMonth = 0;
  iDay = 0;
  iHour = 0;
  iMinute = 0;
  iSecond = 0;
  if (!pData)
    return false;

  int32_t iIndex = 0;
  while (pData[iIndex] != 'T' && pData[iIndex] != 't') {
    if (iIndex >= iLength)
      return false;
    ++iIndex;
  }
  if (iIndex != 8 && iIndex != 10)
    return false;

  int32_t iStyle = -1;
  if (!IsIsoDateFormat(pData, iIndex, iStyle, iYear, iMonth, iDay))
    return false;
  if (pData[iIndex] != 'T' && pData[iIndex] != 't')
    return true;

  ++iIndex;
  if (((iLength - iIndex > 13) && (iLength - iIndex < 6)) &&
      (iLength - iIndex != 15)) {
    return true;
  }
  return IsIsoTimeFormat(pData + iIndex, iLength - iIndex, iHour, iMinute,
                         iSecond, iMillionSecond, iZoneHour, iZoneMinute);
}

// static
ByteString CFXJSE_FormCalcContext::Local2IsoDate(
    CFXJSE_Value* pThis,
    const ByteStringView& szDate,
    const ByteStringView& szFormat,
    const ByteStringView& szLocale) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return ByteString();

  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  LocaleIface* pLocale = LocaleFromString(pDoc, pMgr, szLocale);
  if (!pLocale)
    return ByteString();

  WideString wsFormat = FormatFromString(pLocale, szFormat);
  CFX_DateTime dt = CXFA_LocaleValue(XFA_VT_DATE, WideString::FromUTF8(szDate),
                                     wsFormat, pLocale, pMgr)
                        .GetDate();

  return ByteString::Format("%4d-%02d-%02d", dt.GetYear(), dt.GetMonth(),
                            dt.GetDay());
}

// static
ByteString CFXJSE_FormCalcContext::IsoDate2Local(
    CFXJSE_Value* pThis,
    const ByteStringView& szDate,
    const ByteStringView& szFormat,
    const ByteStringView& szLocale) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return ByteString();

  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  LocaleIface* pLocale = LocaleFromString(pDoc, pMgr, szLocale);
  if (!pLocale)
    return ByteString();

  WideString wsFormat = FormatFromString(pLocale, szFormat);
  WideString wsRet;
  CXFA_LocaleValue(XFA_VT_DATE, WideString::FromUTF8(szDate), pMgr)
      .FormatPatterns(wsRet, wsFormat, pLocale, XFA_VALUEPICTURE_Display);
  return wsRet.UTF8Encode();
}

// static
ByteString CFXJSE_FormCalcContext::IsoTime2Local(
    CFXJSE_Value* pThis,
    const ByteStringView& szTime,
    const ByteStringView& szFormat,
    const ByteStringView& szLocale) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return ByteString();

  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  LocaleIface* pLocale = LocaleFromString(pDoc, pMgr, szLocale);
  if (!pLocale)
    return ByteString();

  WideString wsFormat = {
      L"time{", FormatFromString(pLocale, szFormat).AsStringView(), L"}"};
  CXFA_LocaleValue widgetValue(XFA_VT_TIME, WideString::FromUTF8(szTime), pMgr);
  WideString wsRet;
  widgetValue.FormatPatterns(wsRet, wsFormat, pLocale,
                             XFA_VALUEPICTURE_Display);
  return wsRet.UTF8Encode();
}

// static
int32_t CFXJSE_FormCalcContext::DateString2Num(
    const ByteStringView& szDateString) {
  int32_t iLength = szDateString.GetLength();
  int32_t iYear = 0;
  int32_t iMonth = 0;
  int32_t iDay = 0;
  if (iLength <= 10) {
    int32_t iStyle = -1;
    if (!IsIsoDateFormat(szDateString.unterminated_c_str(), iLength, iStyle,
                         iYear, iMonth, iDay)) {
      return 0;
    }
  } else {
    int32_t iHour = 0;
    int32_t iMinute = 0;
    int32_t iSecond = 0;
    int32_t iMilliSecond = 0;
    int32_t iZoneHour = 0;
    int32_t iZoneMinute = 0;
    if (!IsIsoDateTimeFormat(szDateString.unterminated_c_str(), iLength, iYear,
                             iMonth, iDay, iHour, iMinute, iSecond,
                             iMilliSecond, iZoneHour, iZoneMinute)) {
      return 0;
    }
  }

  float dDays = 0;
  int32_t i = 1;
  if (iYear < 1900)
    return 0;

  while (iYear - i >= 1900) {
    dDays +=
        ((!((iYear - i) % 4) && ((iYear - i) % 100)) || !((iYear - i) % 400))
            ? 366
            : 365;
    ++i;
  }
  i = 1;
  while (i < iMonth) {
    if (i == 2)
      dDays += ((!(iYear % 4) && (iYear % 100)) || !(iYear % 400)) ? 29 : 28;
    else if (i <= 7)
      dDays += (i % 2 == 0) ? 30 : 31;
    else
      dDays += (i % 2 == 0) ? 31 : 30;

    ++i;
  }
  i = 0;
  while (iDay - i > 0) {
    dDays += 1;
    ++i;
  }
  return (int32_t)dDays;
}

// static
ByteString CFXJSE_FormCalcContext::GetLocalDateFormat(
    CFXJSE_Value* pThis,
    int32_t iStyle,
    const ByteStringView& szLocale,
    bool bStandard) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return ByteString();

  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  LocaleIface* pLocale = LocaleFromString(pDoc, pMgr, szLocale);
  if (!pLocale)
    return ByteString();

  WideString strRet = pLocale->GetDatePattern(SubCategoryFromInt(iStyle));
  if (!bStandard) {
    AlternateDateTimeSymbols(strRet, pLocale->GetDateTimeSymbols(),
                             kAltTableDate);
  }
  return strRet.UTF8Encode();
}

// static
ByteString CFXJSE_FormCalcContext::GetLocalTimeFormat(
    CFXJSE_Value* pThis,
    int32_t iStyle,
    const ByteStringView& szLocale,
    bool bStandard) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return ByteString();

  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  LocaleIface* pLocale = LocaleFromString(pDoc, pMgr, szLocale);
  if (!pLocale)
    return ByteString();

  WideString strRet = pLocale->GetTimePattern(SubCategoryFromInt(iStyle));
  if (!bStandard) {
    AlternateDateTimeSymbols(strRet, pLocale->GetDateTimeSymbols(),
                             kAltTableTime);
  }
  return strRet.UTF8Encode();
}

// static
ByteString CFXJSE_FormCalcContext::GetStandardDateFormat(
    CFXJSE_Value* pThis,
    int32_t iStyle,
    const ByteStringView& szLocalStr) {
  return GetLocalDateFormat(pThis, iStyle, szLocalStr, true);
}

// static
ByteString CFXJSE_FormCalcContext::GetStandardTimeFormat(
    CFXJSE_Value* pThis,
    int32_t iStyle,
    const ByteStringView& szLocalStr) {
  return GetLocalTimeFormat(pThis, iStyle, szLocalStr, true);
}

// static
ByteString CFXJSE_FormCalcContext::Num2AllTime(CFXJSE_Value* pThis,
                                               int32_t iTime,
                                               const ByteStringView& szFormat,
                                               const ByteStringView& szLocale,
                                               bool bGM) {
  int32_t iHour = 0;
  int32_t iMin = 0;
  int32_t iSec = 0;
  iHour = static_cast<int>(iTime) / 3600000;
  iMin = (static_cast<int>(iTime) - iHour * 3600000) / 60000;
  iSec = (static_cast<int>(iTime) - iHour * 3600000 - iMin * 60000) / 1000;

  if (!bGM) {
    int32_t iZoneHour = 0;
    int32_t iZoneMin = 0;
    int32_t iZoneSec = 0;
    GetLocalTimeZone(iZoneHour, iZoneMin, iZoneSec);
    iHour += iZoneHour;
    iMin += iZoneMin;
    iSec += iZoneSec;
  }

  return IsoTime2Local(
      pThis,
      ByteString::Format("%02d:%02d:%02d", iHour, iMin, iSec).AsStringView(),
      szFormat, szLocale);
}

// static
void CFXJSE_FormCalcContext::GetLocalTimeZone(int32_t& iHour,
                                              int32_t& iMin,
                                              int32_t& iSec) {
  time_t now;
  time(&now);

  struct tm* pGmt = gmtime(&now);
  struct tm* pLocal = localtime(&now);
  iHour = pLocal->tm_hour - pGmt->tm_hour;
  iMin = pLocal->tm_min - pGmt->tm_min;
  iSec = pLocal->tm_sec - pGmt->tm_sec;
}

// static
void CFXJSE_FormCalcContext::Apr(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"Apr");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double nPrincipal = ValueToDouble(pThis, argOne.get());
  double nPayment = ValueToDouble(pThis, argTwo.get());
  double nPeriods = ValueToDouble(pThis, argThree.get());
  if (nPrincipal <= 0 || nPayment <= 0 || nPeriods <= 0) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  double r = 2 * (nPeriods * nPayment - nPrincipal) / (nPeriods * nPrincipal);
  double nTemp = 1;
  for (int32_t i = 0; i < nPeriods; ++i)
    nTemp *= (1 + r);

  double nRet = r * nTemp / (nTemp - 1) - nPayment / nPrincipal;
  while (fabs(nRet) > kFinancialPrecision) {
    double nDerivative =
        ((nTemp + r * nPeriods * (nTemp / (1 + r))) * (nTemp - 1) -
         (r * nTemp * nPeriods * (nTemp / (1 + r)))) /
        ((nTemp - 1) * (nTemp - 1));
    if (nDerivative == 0) {
      args.GetReturnValue()->SetNull();
      return;
    }

    r = r - nRet / nDerivative;
    nTemp = 1;
    for (int32_t i = 0; i < nPeriods; ++i) {
      nTemp *= (1 + r);
    }
    nRet = r * nTemp / (nTemp - 1) - nPayment / nPrincipal;
  }
  args.GetReturnValue()->SetDouble(r * 12);
}

// static
void CFXJSE_FormCalcContext::CTerm(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"CTerm");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float nRate = ValueToFloat(pThis, argOne.get());
  float nFutureValue = ValueToFloat(pThis, argTwo.get());
  float nInitAmount = ValueToFloat(pThis, argThree.get());
  if ((nRate <= 0) || (nFutureValue <= 0) || (nInitAmount <= 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  args.GetReturnValue()->SetFloat(log((float)(nFutureValue / nInitAmount)) /
                                  log((float)(1 + nRate)));
}

// static
void CFXJSE_FormCalcContext::FV(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"FV");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double nAmount = ValueToDouble(pThis, argOne.get());
  double nRate = ValueToDouble(pThis, argTwo.get());
  double nPeriod = ValueToDouble(pThis, argThree.get());
  if ((nRate < 0) || (nPeriod <= 0) || (nAmount <= 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  double dResult = 0;
  if (nRate) {
    double nTemp = 1;
    for (int i = 0; i < nPeriod; ++i) {
      nTemp *= 1 + nRate;
    }
    dResult = nAmount * (nTemp - 1) / nRate;
  } else {
    dResult = nAmount * nPeriod;
  }

  args.GetReturnValue()->SetDouble(dResult);
}

// static
void CFXJSE_FormCalcContext::IPmt(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 5) {
    pContext->ThrowParamCountMismatchException(L"IPmt");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  std::unique_ptr<CFXJSE_Value> argFour = GetSimpleValue(pThis, args, 3);
  std::unique_ptr<CFXJSE_Value> argFive = GetSimpleValue(pThis, args, 4);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get()) || ValueIsNull(pThis, argFour.get()) ||
      ValueIsNull(pThis, argFive.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float nPrincipalAmount = ValueToFloat(pThis, argOne.get());
  float nRate = ValueToFloat(pThis, argTwo.get());
  float nPayment = ValueToFloat(pThis, argThree.get());
  float nFirstMonth = ValueToFloat(pThis, argFour.get());
  float nNumberOfMonths = ValueToFloat(pThis, argFive.get());
  if ((nPrincipalAmount <= 0) || (nRate <= 0) || (nPayment <= 0) ||
      (nFirstMonth < 0) || (nNumberOfMonths < 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  float nRateOfMonth = nRate / 12;
  int32_t iNums =
      (int32_t)((log10((float)(nPayment / nPrincipalAmount)) -
                 log10((float)(nPayment / nPrincipalAmount - nRateOfMonth))) /
                log10((float)(1 + nRateOfMonth)));
  int32_t iEnd = std::min((int32_t)(nFirstMonth + nNumberOfMonths - 1), iNums);

  if (nPayment < nPrincipalAmount * nRateOfMonth) {
    args.GetReturnValue()->SetFloat(0);
    return;
  }

  int32_t i = 0;
  for (i = 0; i < nFirstMonth - 1; ++i)
    nPrincipalAmount -= nPayment - nPrincipalAmount * nRateOfMonth;

  float nSum = 0;
  for (; i < iEnd; ++i) {
    nSum += nPrincipalAmount * nRateOfMonth;
    nPrincipalAmount -= nPayment - nPrincipalAmount * nRateOfMonth;
  }
  args.GetReturnValue()->SetFloat(nSum);
}

// static
void CFXJSE_FormCalcContext::NPV(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  int32_t argc = args.GetLength();
  if (argc < 3) {
    pContext->ThrowParamCountMismatchException(L"NPV");
    return;
  }

  std::vector<std::unique_ptr<CFXJSE_Value>> argValues;
  for (int32_t i = 0; i < argc; i++) {
    argValues.push_back(GetSimpleValue(pThis, args, i));
    if (ValueIsNull(pThis, argValues[i].get())) {
      args.GetReturnValue()->SetNull();
      return;
    }
  }

  double nRate = ValueToDouble(pThis, argValues[0].get());
  if (nRate <= 0) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  std::vector<double> data(argc - 1);
  for (int32_t i = 1; i < argc; i++)
    data.push_back(ValueToDouble(pThis, argValues[i].get()));

  double nSum = 0;
  int32_t iIndex = 0;
  for (int32_t i = 0; i < argc - 1; i++) {
    double nTemp = 1;
    for (int32_t j = 0; j <= i; j++)
      nTemp *= 1 + nRate;

    double nNum = data[iIndex++];
    nSum += nNum / nTemp;
  }
  args.GetReturnValue()->SetDouble(nSum);
}

// static
void CFXJSE_FormCalcContext::Pmt(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"Pmt");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float nPrincipal = ValueToFloat(pThis, argOne.get());
  float nRate = ValueToFloat(pThis, argTwo.get());
  float nPeriods = ValueToFloat(pThis, argThree.get());
  if ((nPrincipal <= 0) || (nRate <= 0) || (nPeriods <= 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  float nTmp = 1 + nRate;
  float nSum = nTmp;
  for (int32_t i = 0; i < nPeriods - 1; ++i)
    nSum *= nTmp;

  args.GetReturnValue()->SetFloat((nPrincipal * nRate * nSum) / (nSum - 1));
}

// static
void CFXJSE_FormCalcContext::PPmt(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 5) {
    pContext->ThrowParamCountMismatchException(L"PPmt");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  std::unique_ptr<CFXJSE_Value> argFour = GetSimpleValue(pThis, args, 3);
  std::unique_ptr<CFXJSE_Value> argFive = GetSimpleValue(pThis, args, 4);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get()) || ValueIsNull(pThis, argFour.get()) ||
      ValueIsNull(pThis, argFive.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float nPrincipalAmount = ValueToFloat(pThis, argOne.get());
  float nRate = ValueToFloat(pThis, argTwo.get());
  float nPayment = ValueToFloat(pThis, argThree.get());
  float nFirstMonth = ValueToFloat(pThis, argFour.get());
  float nNumberOfMonths = ValueToFloat(pThis, argFive.get());
  if ((nPrincipalAmount <= 0) || (nRate <= 0) || (nPayment <= 0) ||
      (nFirstMonth < 0) || (nNumberOfMonths < 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  float nRateOfMonth = nRate / 12;
  int32_t iNums =
      (int32_t)((log10((float)(nPayment / nPrincipalAmount)) -
                 log10((float)(nPayment / nPrincipalAmount - nRateOfMonth))) /
                log10((float)(1 + nRateOfMonth)));
  int32_t iEnd = std::min((int32_t)(nFirstMonth + nNumberOfMonths - 1), iNums);
  if (nPayment < nPrincipalAmount * nRateOfMonth) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  int32_t i = 0;
  for (i = 0; i < nFirstMonth - 1; ++i)
    nPrincipalAmount -= nPayment - nPrincipalAmount * nRateOfMonth;

  float nTemp = 0;
  float nSum = 0;
  for (; i < iEnd; ++i) {
    nTemp = nPayment - nPrincipalAmount * nRateOfMonth;
    nSum += nTemp;
    nPrincipalAmount -= nTemp;
  }
  args.GetReturnValue()->SetFloat(nSum);
}

// static
void CFXJSE_FormCalcContext::PV(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"PV");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double nAmount = ValueToDouble(pThis, argOne.get());
  double nRate = ValueToDouble(pThis, argTwo.get());
  double nPeriod = ValueToDouble(pThis, argThree.get());
  if ((nAmount <= 0) || (nRate < 0) || (nPeriod <= 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  double nTemp = 1;
  for (int32_t i = 0; i < nPeriod; ++i)
    nTemp *= 1 + nRate;

  nTemp = 1 / nTemp;
  args.GetReturnValue()->SetDouble(nAmount * ((1 - nTemp) / nRate));
}

// static
void CFXJSE_FormCalcContext::Rate(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"Rate");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float nFuture = ValueToFloat(pThis, argOne.get());
  float nPresent = ValueToFloat(pThis, argTwo.get());
  float nTotalNumber = ValueToFloat(pThis, argThree.get());
  if ((nFuture <= 0) || (nPresent < 0) || (nTotalNumber <= 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  args.GetReturnValue()->SetFloat(
      FXSYS_pow((float)(nFuture / nPresent), (float)(1 / nTotalNumber)) - 1);
}

// static
void CFXJSE_FormCalcContext::Term(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 3) {
    pContext->ThrowParamCountMismatchException(L"Term");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get()) ||
      ValueIsNull(pThis, argThree.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float nMount = ValueToFloat(pThis, argOne.get());
  float nRate = ValueToFloat(pThis, argTwo.get());
  float nFuture = ValueToFloat(pThis, argThree.get());
  if ((nMount <= 0) || (nRate <= 0) || (nFuture <= 0)) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  args.GetReturnValue()->SetFloat(log((float)(nFuture / nMount * nRate) + 1) /
                                  log((float)(1 + nRate)));
}

// static
void CFXJSE_FormCalcContext::Choose(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  int32_t argc = args.GetLength();
  if (argc < 2) {
    pContext->ThrowParamCountMismatchException(L"Choose");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  int32_t iIndex = (int32_t)ValueToFloat(pThis, argOne.get());
  if (iIndex < 1) {
    args.GetReturnValue()->SetString("");
    return;
  }

  bool bFound = false;
  bool bStopCounterFlags = false;
  int32_t iArgIndex = 1;
  int32_t iValueIndex = 0;
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  while (!bFound && !bStopCounterFlags && (iArgIndex < argc)) {
    std::unique_ptr<CFXJSE_Value> argIndexValue = args.GetValue(iArgIndex);
    if (argIndexValue->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argIndexValue->GetObjectProperty("length", lengthValue.get());
      int32_t iLength = lengthValue->ToInteger();
      if (iLength > 3)
        bStopCounterFlags = true;

      iValueIndex += (iLength - 2);
      if (iValueIndex >= iIndex) {
        auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
        auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
        auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
        argIndexValue->GetObjectPropertyByIdx(1, propertyValue.get());
        argIndexValue->GetObjectPropertyByIdx(
            (iLength - 1) - (iValueIndex - iIndex), jsObjectValue.get());
        if (propertyValue->IsNull()) {
          GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
        } else {
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(), newPropertyValue.get());
        }
        ByteString bsChosen = ValueToUTF8String(newPropertyValue.get());
        args.GetReturnValue()->SetString(bsChosen.AsStringView());
        bFound = true;
      }
    } else {
      iValueIndex++;
      if (iValueIndex == iIndex) {
        ByteString bsChosen = ValueToUTF8String(argIndexValue.get());
        args.GetReturnValue()->SetString(bsChosen.AsStringView());
        bFound = true;
      }
    }
    iArgIndex++;
  }
  if (!bFound)
    args.GetReturnValue()->SetString("");
}

// static
void CFXJSE_FormCalcContext::Exists(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Exists");
    return;
  }
  args.GetReturnValue()->SetInteger(args.GetValue(0)->IsObject());
}

// static
void CFXJSE_FormCalcContext::HasValue(CFXJSE_Value* pThis,
                                      const ByteStringView& szFuncName,
                                      CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"HasValue");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (!argOne->IsString()) {
    args.GetReturnValue()->SetInteger(argOne->IsNumber() ||
                                      argOne->IsBoolean());
    return;
  }

  ByteString valueStr = argOne->ToString();
  valueStr.TrimLeft();
  args.GetReturnValue()->SetInteger(!valueStr.IsEmpty());
}

// static
void CFXJSE_FormCalcContext::Oneof(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  if (args.GetLength() < 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Oneof");
    return;
  }

  bool bFlags = false;
  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::vector<std::unique_ptr<CFXJSE_Value>> parameterValues;
  unfoldArgs(pThis, args, &parameterValues, 1);
  for (const auto& value : parameterValues) {
    if (simpleValueCompare(pThis, argOne.get(), value.get())) {
      bFlags = true;
      break;
    }
  }

  args.GetReturnValue()->SetInteger(bFlags);
}

// static
void CFXJSE_FormCalcContext::Within(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  if (args.GetLength() != 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Within");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetUndefined();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argLow = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> argHigh = GetSimpleValue(pThis, args, 2);
  if (argOne->IsNumber()) {
    float oneNumber = ValueToFloat(pThis, argOne.get());
    float lowNumber = ValueToFloat(pThis, argLow.get());
    float heightNumber = ValueToFloat(pThis, argHigh.get());
    args.GetReturnValue()->SetInteger((oneNumber >= lowNumber) &&
                                      (oneNumber <= heightNumber));
    return;
  }

  ByteString oneString = ValueToUTF8String(argOne.get());
  ByteString lowString = ValueToUTF8String(argLow.get());
  ByteString heightString = ValueToUTF8String(argHigh.get());
  args.GetReturnValue()->SetInteger(
      (oneString.Compare(lowString.AsStringView()) >= 0) &&
      (oneString.Compare(heightString.AsStringView()) <= 0));
}

// static
void CFXJSE_FormCalcContext::If(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args) {
  if (args.GetLength() != 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"If");
    return;
  }

  args.GetReturnValue()->Assign(GetSimpleValue(pThis, args, 0)->ToBoolean()
                                    ? GetSimpleValue(pThis, args, 1).get()
                                    : GetSimpleValue(pThis, args, 2).get());
}

// static
void CFXJSE_FormCalcContext::Eval(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 1) {
    pContext->ThrowParamCountMismatchException(L"Eval");
    return;
  }

  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  std::unique_ptr<CFXJSE_Value> scriptValue = GetSimpleValue(pThis, args, 0);
  ByteString utf8ScriptString = ValueToUTF8String(scriptValue.get());
  if (utf8ScriptString.IsEmpty()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  CFX_WideTextBuf wsJavaScriptBuf;
  if (!CFXJSE_FormCalcContext::Translate(
          WideString::FromUTF8(utf8ScriptString.AsStringView()).AsStringView(),
          &wsJavaScriptBuf)) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Context> pNewContext(
      CFXJSE_Context::Create(pIsolate, nullptr, nullptr, nullptr));

  auto returnValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  pNewContext->ExecuteScript(
      FX_UTF8Encode(wsJavaScriptBuf.AsStringView()).c_str(), returnValue.get());

  args.GetReturnValue()->Assign(returnValue.get());
}

// static
void CFXJSE_FormCalcContext::Ref(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  if (args.GetLength() != 1) {
    pContext->ThrowParamCountMismatchException(L"Ref");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (!argOne->IsArray() && !argOne->IsObject() && !argOne->IsBoolean() &&
      !argOne->IsString() && !argOne->IsNull() && !argOne->IsNumber()) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  if (argOne->IsBoolean() || argOne->IsString() || argOne->IsNumber()) {
    args.GetReturnValue()->Assign(argOne.get());
    return;
  }

  std::vector<std::unique_ptr<CFXJSE_Value>> values;
  for (int32_t i = 0; i < 3; i++)
    values.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

  int intVal = 3;
  if (argOne->IsNull()) {
    // TODO(dsinclair): Why is this 4 when the others are all 3?
    intVal = 4;
    values[2]->SetNull();
  } else if (argOne->IsArray()) {
#ifndef NDEBUG
    auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argOne->GetObjectProperty("length", lengthValue.get());
    ASSERT(lengthValue->ToInteger() >= 3);
#endif

    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argOne->GetObjectPropertyByIdx(1, propertyValue.get());
    argOne->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (!propertyValue->IsNull() || jsObjectValue->IsNull()) {
      pContext->ThrowArgumentMismatchException();
      return;
    }

    values[2]->Assign(jsObjectValue.get());
  } else if (argOne->IsObject()) {
    values[2]->Assign(argOne.get());
  }

  values[0]->SetInteger(intVal);
  values[1]->SetNull();
  args.GetReturnValue()->SetArray(values);
}

// static
void CFXJSE_FormCalcContext::UnitType(CFXJSE_Value* pThis,
                                      const ByteStringView& szFuncName,
                                      CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"UnitType");
    return;
  }

  std::unique_ptr<CFXJSE_Value> unitspanValue = GetSimpleValue(pThis, args, 0);
  if (unitspanValue->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString unitspanString = ValueToUTF8String(unitspanValue.get());
  if (unitspanString.IsEmpty()) {
    args.GetReturnValue()->SetString("in");
    return;
  }

  enum XFA_FM2JS_VALUETYPE_ParserStatus {
    VALUETYPE_START,
    VALUETYPE_HAVEINVALIDCHAR,
    VALUETYPE_HAVEDIGIT,
    VALUETYPE_HAVEDIGITWHITE,
    VALUETYPE_ISCM,
    VALUETYPE_ISMM,
    VALUETYPE_ISPT,
    VALUETYPE_ISMP,
    VALUETYPE_ISIN,
  };
  unitspanString.MakeLower();
  WideString wsTypeString = WideString::FromUTF8(unitspanString.AsStringView());
  const wchar_t* pData = wsTypeString.c_str();
  int32_t u = 0;
  int32_t uLen = wsTypeString.GetLength();
  while (IsWhitespace(pData[u]))
    u++;

  XFA_FM2JS_VALUETYPE_ParserStatus eParserStatus = VALUETYPE_START;
  wchar_t typeChar;
  // TODO(dsinclair): Cleanup this parser, figure out what the various checks
  //    are for.
  while (u < uLen) {
    typeChar = pData[u];
    if (IsWhitespace(typeChar)) {
      if (eParserStatus != VALUETYPE_HAVEDIGIT &&
          eParserStatus != VALUETYPE_HAVEDIGITWHITE) {
        eParserStatus = VALUETYPE_ISIN;
        break;
      }
      eParserStatus = VALUETYPE_HAVEDIGITWHITE;
    } else if (IsPartOfNumberW(typeChar)) {
      if (eParserStatus == VALUETYPE_HAVEDIGITWHITE) {
        eParserStatus = VALUETYPE_ISIN;
        break;
      }
      eParserStatus = VALUETYPE_HAVEDIGIT;
    } else if ((typeChar == 'c' || typeChar == 'p') && (u + 1 < uLen)) {
      wchar_t nextChar = pData[u + 1];
      if ((eParserStatus == VALUETYPE_START ||
           eParserStatus == VALUETYPE_HAVEDIGIT ||
           eParserStatus == VALUETYPE_HAVEDIGITWHITE) &&
          !IsPartOfNumberW(nextChar)) {
        eParserStatus = (typeChar == 'c') ? VALUETYPE_ISCM : VALUETYPE_ISPT;
        break;
      }
      eParserStatus = VALUETYPE_HAVEINVALIDCHAR;
    } else if (typeChar == 'm' && (u + 1 < uLen)) {
      wchar_t nextChar = pData[u + 1];
      if ((eParserStatus == VALUETYPE_START ||
           eParserStatus == VALUETYPE_HAVEDIGIT ||
           eParserStatus == VALUETYPE_HAVEDIGITWHITE) &&
          !IsPartOfNumberW(nextChar)) {
        eParserStatus = VALUETYPE_ISMM;
        if (nextChar == 'p' || ((u + 5 < uLen) && pData[u + 1] == 'i' &&
                                pData[u + 2] == 'l' && pData[u + 3] == 'l' &&
                                pData[u + 4] == 'i' && pData[u + 5] == 'p')) {
          eParserStatus = VALUETYPE_ISMP;
        }
        break;
      }
    } else {
      eParserStatus = VALUETYPE_HAVEINVALIDCHAR;
    }
    u++;
  }
  switch (eParserStatus) {
    case VALUETYPE_ISCM:
      args.GetReturnValue()->SetString("cm");
      break;
    case VALUETYPE_ISMM:
      args.GetReturnValue()->SetString("mm");
      break;
    case VALUETYPE_ISPT:
      args.GetReturnValue()->SetString("pt");
      break;
    case VALUETYPE_ISMP:
      args.GetReturnValue()->SetString("mp");
      break;
    default:
      args.GetReturnValue()->SetString("in");
      break;
  }
}

// static
void CFXJSE_FormCalcContext::UnitValue(CFXJSE_Value* pThis,
                                       const ByteStringView& szFuncName,
                                       CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"UnitValue");
    return;
  }

  std::unique_ptr<CFXJSE_Value> unitspanValue = GetSimpleValue(pThis, args, 0);
  if (unitspanValue->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString unitspanString = ValueToUTF8String(unitspanValue.get());
  const char* pData = unitspanString.c_str();
  if (!pData) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }

  size_t u = 0;
  while (IsWhitespace(pData[u]))
    ++u;

  while (u < unitspanString.GetLength()) {
    if (!IsPartOfNumber(pData[u]))
      break;
    ++u;
  }

  char* pTemp = nullptr;
  double dFirstNumber = strtod(pData, &pTemp);
  while (IsWhitespace(pData[u]))
    ++u;

  size_t uLen = unitspanString.GetLength();
  ByteString strFirstUnit;
  while (u < uLen) {
    if (pData[u] == ' ')
      break;

    strFirstUnit += pData[u];
    ++u;
  }
  strFirstUnit.MakeLower();

  ByteString strUnit;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> unitValue = GetSimpleValue(pThis, args, 1);
    ByteString unitTempString = ValueToUTF8String(unitValue.get());
    const char* pChar = unitTempString.c_str();
    size_t uVal = 0;
    while (IsWhitespace(pChar[uVal]))
      ++uVal;

    while (uVal < unitTempString.GetLength()) {
      if (!std::isdigit(pChar[uVal]) && pChar[uVal] != '.')
        break;
      ++uVal;
    }
    while (IsWhitespace(pChar[uVal]))
      ++uVal;

    size_t uValLen = unitTempString.GetLength();
    while (uVal < uValLen) {
      if (pChar[uVal] == ' ')
        break;

      strUnit += pChar[uVal];
      ++uVal;
    }
    strUnit.MakeLower();
  } else {
    strUnit = strFirstUnit;
  }

  double dResult = 0;
  if (strFirstUnit == "in" || strFirstUnit == "inches") {
    if (strUnit == "mm" || strUnit == "millimeters")
      dResult = dFirstNumber * 25.4;
    else if (strUnit == "cm" || strUnit == "centimeters")
      dResult = dFirstNumber * 2.54;
    else if (strUnit == "pt" || strUnit == "points")
      dResult = dFirstNumber / 72;
    else if (strUnit == "mp" || strUnit == "millipoints")
      dResult = dFirstNumber / 72000;
    else
      dResult = dFirstNumber;
  } else if (strFirstUnit == "mm" || strFirstUnit == "millimeters") {
    if (strUnit == "mm" || strUnit == "millimeters")
      dResult = dFirstNumber;
    else if (strUnit == "cm" || strUnit == "centimeters")
      dResult = dFirstNumber / 10;
    else if (strUnit == "pt" || strUnit == "points")
      dResult = dFirstNumber / 25.4 / 72;
    else if (strUnit == "mp" || strUnit == "millipoints")
      dResult = dFirstNumber / 25.4 / 72000;
    else
      dResult = dFirstNumber / 25.4;
  } else if (strFirstUnit == "cm" || strFirstUnit == "centimeters") {
    if (strUnit == "mm" || strUnit == "millimeters")
      dResult = dFirstNumber * 10;
    else if (strUnit == "cm" || strUnit == "centimeters")
      dResult = dFirstNumber;
    else if (strUnit == "pt" || strUnit == "points")
      dResult = dFirstNumber / 2.54 / 72;
    else if (strUnit == "mp" || strUnit == "millipoints")
      dResult = dFirstNumber / 2.54 / 72000;
    else
      dResult = dFirstNumber / 2.54;
  } else if (strFirstUnit == "pt" || strFirstUnit == "points") {
    if (strUnit == "mm" || strUnit == "millimeters")
      dResult = dFirstNumber / 72 * 25.4;
    else if (strUnit == "cm" || strUnit == "centimeters")
      dResult = dFirstNumber / 72 * 2.54;
    else if (strUnit == "pt" || strUnit == "points")
      dResult = dFirstNumber;
    else if (strUnit == "mp" || strUnit == "millipoints")
      dResult = dFirstNumber * 1000;
    else
      dResult = dFirstNumber / 72;
  } else if (strFirstUnit == "mp" || strFirstUnit == "millipoints") {
    if (strUnit == "mm" || strUnit == "millimeters")
      dResult = dFirstNumber / 72000 * 25.4;
    else if (strUnit == "cm" || strUnit == "centimeters")
      dResult = dFirstNumber / 72000 * 2.54;
    else if (strUnit == "pt" || strUnit == "points")
      dResult = dFirstNumber / 1000;
    else if (strUnit == "mp" || strUnit == "millipoints")
      dResult = dFirstNumber;
    else
      dResult = dFirstNumber / 72000;
  }
  args.GetReturnValue()->SetDouble(dResult);
}

// static
void CFXJSE_FormCalcContext::At(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"At");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString stringTwo = ValueToUTF8String(argTwo.get());
  if (stringTwo.IsEmpty()) {
    args.GetReturnValue()->SetInteger(1);
    return;
  }

  ByteString stringOne = ValueToUTF8String(argOne.get());
  auto pos = stringOne.Find(stringTwo.AsStringView());
  args.GetReturnValue()->SetInteger(pos.has_value() ? pos.value() + 1 : 0);
}

// static
void CFXJSE_FormCalcContext::Concat(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Concat");
    return;
  }

  ByteString resultString;
  bool bAllNull = true;
  for (int32_t i = 0; i < argc; i++) {
    std::unique_ptr<CFXJSE_Value> value = GetSimpleValue(pThis, args, i);
    if (ValueIsNull(pThis, value.get()))
      continue;

    bAllNull = false;
    resultString += ValueToUTF8String(value.get());
  }

  if (bAllNull) {
    args.GetReturnValue()->SetNull();
    return;
  }

  args.GetReturnValue()->SetString(resultString.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Decode(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Decode");
    return;
  }

  if (argc == 1) {
    std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
    if (ValueIsNull(pThis, argOne.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }

    WideString decoded = DecodeURL(
        WideString::FromUTF8(ValueToUTF8String(argOne.get()).AsStringView()));

    args.GetReturnValue()->SetString(
        FX_UTF8Encode(decoded.AsStringView()).AsStringView());
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString toDecodeString = ValueToUTF8String(argOne.get());
  ByteString identifyString = ValueToUTF8String(argTwo.get());
  WideString decoded;

  WideString toDecodeWideString =
      WideString::FromUTF8(toDecodeString.AsStringView());

  if (identifyString.EqualNoCase("html"))
    decoded = DecodeHTML(toDecodeWideString);
  else if (identifyString.EqualNoCase("xml"))
    decoded = DecodeXML(toDecodeWideString);
  else
    decoded = DecodeURL(toDecodeWideString);

  args.GetReturnValue()->SetString(
      FX_UTF8Encode(decoded.AsStringView()).AsStringView());
}

// static
WideString CFXJSE_FormCalcContext::DecodeURL(const WideString& wsURLString) {
  const wchar_t* pData = wsURLString.c_str();
  size_t i = 0;
  CFX_WideTextBuf wsResultBuf;
  while (i < wsURLString.GetLength()) {
    wchar_t ch = pData[i];
    if ('%' != ch) {
      wsResultBuf.AppendChar(ch);
      ++i;
      continue;
    }

    wchar_t chTemp = 0;
    int32_t iCount = 0;
    while (iCount < 2) {
      ++i;
      ch = pData[i];
      if (ch <= '9' && ch >= '0') {
        // TODO(dsinclair): Premultiply and add rather then scale.
        chTemp += (ch - '0') * (!iCount ? 16 : 1);
      } else if (ch <= 'F' && ch >= 'A') {
        chTemp += (ch - 'A' + 10) * (!iCount ? 16 : 1);
      } else if (ch <= 'f' && ch >= 'a') {
        chTemp += (ch - 'a' + 10) * (!iCount ? 16 : 1);
      } else {
        return WideString();
      }
      ++iCount;
    }
    wsResultBuf.AppendChar(chTemp);
    ++i;
  }
  wsResultBuf.AppendChar(0);
  return wsResultBuf.MakeString();
}

// static
WideString CFXJSE_FormCalcContext::DecodeHTML(const WideString& wsHTMLString) {
  wchar_t strString[9];
  size_t iStrIndex = 0;
  size_t iLen = wsHTMLString.GetLength();
  size_t i = 0;
  int32_t iCode = 0;
  const wchar_t* pData = wsHTMLString.c_str();
  CFX_WideTextBuf wsResultBuf;
  while (i < iLen) {
    wchar_t ch = pData[i];
    if (ch != '&') {
      wsResultBuf.AppendChar(ch);
      ++i;
      continue;
    }

    ++i;
    ch = pData[i];
    if (ch == '#') {
      ++i;
      ch = pData[i];
      if (ch != 'x' && ch != 'X') {
        return WideString();
      }

      ++i;
      ch = pData[i];
      if ((ch >= '0' && ch <= '9') || (ch <= 'f' && ch >= 'a') ||
          (ch <= 'F' && ch >= 'A')) {
        while (ch != ';' && i < iLen) {
          if (ch >= '0' && ch <= '9') {
            iCode += ch - '0';
          } else if (ch <= 'f' && ch >= 'a') {
            iCode += ch - 'a' + 10;
          } else if (ch <= 'F' && ch >= 'A') {
            iCode += ch - 'A' + 10;
          } else {
            return WideString();
          }
          ++i;
          // TODO(dsinclair): Postmultiply seems wrong, start at zero
          //   and pre-multiply then can remove the post divide.
          iCode *= 16;
          ch = pData[i];
        }
        iCode /= 16;
      }
    } else {
      while (ch != ';' && i < iLen) {
        strString[iStrIndex++] = ch;
        ++i;
        ch = pData[i];
      }
      strString[iStrIndex] = 0;
    }
    uint32_t iData = 0;
    if (HTMLSTR2Code(strString, &iData)) {
      wsResultBuf.AppendChar((wchar_t)iData);
    } else {
      wsResultBuf.AppendChar(iCode);
    }
    iStrIndex = 0;
    strString[iStrIndex] = 0;
    ++i;
  }
  wsResultBuf.AppendChar(0);

  return wsResultBuf.MakeString();
}

// static
WideString CFXJSE_FormCalcContext::DecodeXML(const WideString& wsXMLString) {
  wchar_t strString[9];
  int32_t iStrIndex = 0;
  int32_t iLen = wsXMLString.GetLength();
  int32_t i = 0;
  int32_t iCode = 0;
  wchar_t ch = 0;
  const wchar_t* pData = wsXMLString.c_str();
  CFX_WideTextBuf wsResultBuf;
  while (i < iLen) {
    ch = pData[i];
    if (ch != '&') {
      wsResultBuf.AppendChar(ch);
      ++i;
      continue;
    }

    // TODO(dsinclair): This is very similar to DecodeHTML, can they be
    //   combined?
    ++i;
    ch = pData[i];
    if (ch == '#') {
      ++i;
      ch = pData[i];
      if (ch != 'x' && ch != 'X') {
        return WideString();
      }

      ++i;
      ch = pData[i];
      if ((ch >= '0' && ch <= '9') || (ch <= 'f' && ch >= 'a') ||
          (ch <= 'F' && ch >= 'A')) {
        while (ch != ';') {
          if (ch >= '0' && ch <= '9') {
            iCode += ch - '0';
          } else if (ch <= 'f' && ch >= 'a') {
            iCode += ch - 'a' + 10;
          } else if (ch <= 'F' && ch >= 'A') {
            iCode += ch - 'A' + 10;
          } else {
            return WideString();
          }
          ++i;
          iCode *= 16;
          ch = pData[i];
        }
        iCode /= 16;
      }
    } else {
      while (ch != ';' && i < iLen) {
        strString[iStrIndex++] = ch;
        ++i;
        ch = pData[i];
      }
      strString[iStrIndex] = 0;
    }

    const wchar_t* const strName[] = {L"quot", L"amp", L"apos", L"lt", L"gt"};
    int32_t iIndex = 0;
    while (iIndex < 5) {
      if (memcmp(strString, strName[iIndex], wcslen(strName[iIndex])) == 0) {
        break;
      }
      ++iIndex;
    }
    switch (iIndex) {
      case 0:
        wsResultBuf.AppendChar('"');
        break;
      case 1:
        wsResultBuf.AppendChar('&');
        break;
      case 2:
        wsResultBuf.AppendChar('\'');
        break;
      case 3:
        wsResultBuf.AppendChar('<');
        break;
      case 4:
        wsResultBuf.AppendChar('>');
        break;
      default:
        wsResultBuf.AppendChar(iCode);
        break;
    }
    iStrIndex = 0;
    strString[iStrIndex] = 0;
    ++i;
    iCode = 0;
  }
  wsResultBuf.AppendChar(0);
  return wsResultBuf.MakeString();
}

// static
void CFXJSE_FormCalcContext::Encode(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Encode");
    return;
  }

  if (argc == 1) {
    std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
    if (ValueIsNull(pThis, argOne.get())) {
      args.GetReturnValue()->SetNull();
      return;
    }

    WideString encoded = EncodeURL(ValueToUTF8String(argOne.get()));
    args.GetReturnValue()->SetString(
        FX_UTF8Encode(encoded.AsStringView()).AsStringView());
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  if (ValueIsNull(pThis, argOne.get()) || ValueIsNull(pThis, argTwo.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString toEncodeString = ValueToUTF8String(argOne.get());
  ByteString identifyString = ValueToUTF8String(argTwo.get());
  WideString encoded;
  if (identifyString.EqualNoCase("html"))
    encoded = EncodeHTML(toEncodeString);
  else if (identifyString.EqualNoCase("xml"))
    encoded = EncodeXML(toEncodeString);
  else
    encoded = EncodeURL(toEncodeString);

  args.GetReturnValue()->SetString(
      FX_UTF8Encode(encoded.AsStringView()).AsStringView());
}

// static
WideString CFXJSE_FormCalcContext::EncodeURL(const ByteString& szURLString) {
  WideString wsURLString = WideString::FromUTF8(szURLString.AsStringView());
  CFX_WideTextBuf wsResultBuf;
  wchar_t strEncode[4];
  strEncode[0] = '%';
  strEncode[3] = 0;
  wchar_t strUnsafe[] = {' ', '<',  '>', '"', '#', '%', '{', '}',
                         '|', '\\', '^', '~', '[', ']', '`'};
  wchar_t strReserved[] = {';', '/', '?', ':', '@', '=', '&'};
  wchar_t strSpecial[] = {'$', '-', '+', '!', '*', '\'', '(', ')', ','};
  const wchar_t* strCode = L"0123456789abcdef";
  for (auto ch : wsURLString) {
    int32_t i = 0;
    int32_t iCount = FX_ArraySize(strUnsafe);
    while (i < iCount) {
      if (ch == strUnsafe[i]) {
        int32_t iIndex = ch / 16;
        strEncode[1] = strCode[iIndex];
        strEncode[2] = strCode[ch - iIndex * 16];
        wsResultBuf << strEncode;
        break;
      }
      ++i;
    }
    if (i < iCount)
      continue;

    i = 0;
    iCount = FX_ArraySize(strReserved);
    while (i < iCount) {
      if (ch == strReserved[i]) {
        int32_t iIndex = ch / 16;
        strEncode[1] = strCode[iIndex];
        strEncode[2] = strCode[ch - iIndex * 16];
        wsResultBuf << strEncode;
        break;
      }
      ++i;
    }
    if (i < iCount)
      continue;

    i = 0;
    iCount = FX_ArraySize(strSpecial);
    while (i < iCount) {
      if (ch == strSpecial[i]) {
        wsResultBuf.AppendChar(ch);
        break;
      }
      ++i;
    }
    if (i < iCount)
      continue;

    if ((ch >= 0x80 && ch <= 0xff) || ch <= 0x1f || ch == 0x7f) {
      int32_t iIndex = ch / 16;
      strEncode[1] = strCode[iIndex];
      strEncode[2] = strCode[ch - iIndex * 16];
      wsResultBuf << strEncode;
    } else if (ch >= 0x20 && ch <= 0x7e) {
      wsResultBuf.AppendChar(ch);
    } else {
      const wchar_t iRadix = 16;
      WideString strTmp;
      while (ch >= iRadix) {
        wchar_t tmp = strCode[ch % iRadix];
        ch /= iRadix;
        strTmp += tmp;
      }
      strTmp += strCode[ch];
      int32_t iLen = strTmp.GetLength();
      if (iLen < 2)
        break;

      int32_t iIndex = 0;
      if (iLen % 2 != 0) {
        strEncode[1] = '0';
        strEncode[2] = strTmp[iLen - 1];
        iIndex = iLen - 2;
      } else {
        strEncode[1] = strTmp[iLen - 1];
        strEncode[2] = strTmp[iLen - 2];
        iIndex = iLen - 3;
      }
      wsResultBuf << strEncode;
      while (iIndex > 0) {
        strEncode[1] = strTmp[iIndex];
        strEncode[2] = strTmp[iIndex - 1];
        iIndex -= 2;
        wsResultBuf << strEncode;
      }
    }
  }
  wsResultBuf.AppendChar(0);
  return wsResultBuf.MakeString();
}

// static
WideString CFXJSE_FormCalcContext::EncodeHTML(const ByteString& szHTMLString) {
  WideString wsHTMLString = WideString::FromUTF8(szHTMLString.AsStringView());
  const wchar_t* strCode = L"0123456789abcdef";
  wchar_t strEncode[9];
  strEncode[0] = '&';
  strEncode[1] = '#';
  strEncode[2] = 'x';
  strEncode[5] = ';';
  strEncode[6] = 0;
  strEncode[7] = ';';
  strEncode[8] = 0;
  CFX_WideTextBuf wsResultBuf;
  int32_t iLen = wsHTMLString.GetLength();
  int32_t i = 0;
  const wchar_t* pData = wsHTMLString.c_str();
  while (i < iLen) {
    uint32_t ch = pData[i];
    WideString htmlReserve;
    if (HTMLCode2STR(ch, &htmlReserve)) {
      wsResultBuf.AppendChar(L'&');
      wsResultBuf << htmlReserve;
      wsResultBuf.AppendChar(L';');
    } else if (ch >= 32 && ch <= 126) {
      wsResultBuf.AppendChar((wchar_t)ch);
    } else if (ch < 256) {
      int32_t iIndex = ch / 16;
      strEncode[3] = strCode[iIndex];
      strEncode[4] = strCode[ch - iIndex * 16];
      strEncode[5] = ';';
      strEncode[6] = 0;
      wsResultBuf << strEncode;
    } else {
      int32_t iBigByte = ch / 256;
      int32_t iLittleByte = ch % 256;
      strEncode[3] = strCode[iBigByte / 16];
      strEncode[4] = strCode[iBigByte % 16];
      strEncode[5] = strCode[iLittleByte / 16];
      strEncode[6] = strCode[iLittleByte % 16];
      wsResultBuf << strEncode;
    }
    ++i;
  }
  wsResultBuf.AppendChar(0);
  return wsResultBuf.MakeString();
}

// static
WideString CFXJSE_FormCalcContext::EncodeXML(const ByteString& szXMLString) {
  WideString wsXMLString = WideString::FromUTF8(szXMLString.AsStringView());
  CFX_WideTextBuf wsResultBuf;
  wchar_t strEncode[9];
  strEncode[0] = '&';
  strEncode[1] = '#';
  strEncode[2] = 'x';
  strEncode[5] = ';';
  strEncode[6] = 0;
  strEncode[7] = ';';
  strEncode[8] = 0;
  const wchar_t* strCode = L"0123456789abcdef";
  for (const auto& ch : wsXMLString) {
    switch (ch) {
      case '"':
        wsResultBuf.AppendChar('&');
        wsResultBuf << WideStringView(L"quot");
        wsResultBuf.AppendChar(';');
        break;
      case '&':
        wsResultBuf.AppendChar('&');
        wsResultBuf << WideStringView(L"amp");
        wsResultBuf.AppendChar(';');
        break;
      case '\'':
        wsResultBuf.AppendChar('&');
        wsResultBuf << WideStringView(L"apos");
        wsResultBuf.AppendChar(';');
        break;
      case '<':
        wsResultBuf.AppendChar('&');
        wsResultBuf << WideStringView(L"lt");
        wsResultBuf.AppendChar(';');
        break;
      case '>':
        wsResultBuf.AppendChar('&');
        wsResultBuf << WideStringView(L"gt");
        wsResultBuf.AppendChar(';');
        break;
      default: {
        if (ch >= 32 && ch <= 126) {
          wsResultBuf.AppendChar(ch);
        } else if (ch < 256) {
          int32_t iIndex = ch / 16;
          strEncode[3] = strCode[iIndex];
          strEncode[4] = strCode[ch - iIndex * 16];
          strEncode[5] = ';';
          strEncode[6] = 0;
          wsResultBuf << strEncode;
        } else {
          int32_t iBigByte = ch / 256;
          int32_t iLittleByte = ch % 256;
          strEncode[3] = strCode[iBigByte / 16];
          strEncode[4] = strCode[iBigByte % 16];
          strEncode[5] = strCode[iLittleByte / 16];
          strEncode[6] = strCode[iLittleByte % 16];
          wsResultBuf << strEncode;
        }
        break;
      }
    }
  }
  wsResultBuf.AppendChar(0);
  return wsResultBuf.MakeString();
}

// static
bool CFXJSE_FormCalcContext::HTMLSTR2Code(const WideStringView& pData,
                                          uint32_t* iCode) {
  auto cmpFunc = [](const XFA_FMHtmlReserveCode& iter,
                    const WideStringView& val) {
    // TODO(tsepez): check usage of c_str() below.
    return wcscmp(val.unterminated_c_str(), iter.m_htmlReserve) > 0;
  };
  const XFA_FMHtmlReserveCode* result =
      std::lower_bound(std::begin(kReservesForDecode),
                       std::end(kReservesForDecode), pData, cmpFunc);
  if (result != std::end(kReservesForEncode) &&
      !wcscmp(pData.unterminated_c_str(), result->m_htmlReserve)) {
    *iCode = result->m_uCode;
    return true;
  }
  return false;
}

// static
bool CFXJSE_FormCalcContext::HTMLCode2STR(uint32_t iCode,
                                          WideString* wsHTMLReserve) {
  auto cmpFunc = [](const XFA_FMHtmlReserveCode iter, const uint32_t& val) {
    return iter.m_uCode < val;
  };
  const XFA_FMHtmlReserveCode* result =
      std::lower_bound(std::begin(kReservesForEncode),
                       std::end(kReservesForEncode), iCode, cmpFunc);
  if (result != std::end(kReservesForEncode) && result->m_uCode == iCode) {
    *wsHTMLReserve = result->m_htmlReserve;
    return true;
  }
  return false;
}

// static
void CFXJSE_FormCalcContext::Format(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() < 2) {
    pContext->ThrowParamCountMismatchException(L"Format");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  ByteString szPattern = ValueToUTF8String(argOne.get());

  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  ByteString szValue = ValueToUTF8String(argTwo.get());

  CXFA_Document* pDoc = pContext->GetDocument();
  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  CXFA_Node* pThisNode = ToNode(pDoc->GetScriptContext()->GetThisObject());
  ASSERT(pThisNode);

  LocaleIface* pLocale = pThisNode->GetLocale();
  uint32_t patternType;
  WideString wsPattern = WideString::FromUTF8(szPattern.AsStringView());
  WideString wsValue = WideString::FromUTF8(szValue.AsStringView());
  if (!PatternStringType(szPattern.AsStringView(), patternType)) {
    switch (patternType) {
      case XFA_VT_DATETIME: {
        auto iTChar = wsPattern.Find(L'T');
        if (!iTChar.has_value()) {
          args.GetReturnValue()->SetString("");
          return;
        }
        WideString wsDatePattern(L"date{");
        wsDatePattern += wsPattern.Left(iTChar.value()) + L"} ";

        WideString wsTimePattern(L"time{");
        wsTimePattern +=
            wsPattern.Right(wsPattern.GetLength() - (iTChar.value() + 1)) +
            L"}";
        wsPattern = wsDatePattern + wsTimePattern;
      } break;
      case XFA_VT_DATE: {
        wsPattern = L"date{" + wsPattern + L"}";
      } break;
      case XFA_VT_TIME: {
        wsPattern = L"time{" + wsPattern + L"}";
      } break;
      case XFA_VT_TEXT: {
        wsPattern = L"text{" + wsPattern + L"}";
      } break;
      case XFA_VT_FLOAT: {
        wsPattern = L"num{" + wsPattern + L"}";
      } break;
      default: {
        WideString wsTestPattern;
        wsTestPattern = L"num{" + wsPattern + L"}";
        CXFA_LocaleValue tempLocaleValue(XFA_VT_FLOAT, wsValue, wsTestPattern,
                                         pLocale, pMgr);
        if (tempLocaleValue.IsValid()) {
          wsPattern = wsTestPattern;
          patternType = XFA_VT_FLOAT;
        } else {
          wsTestPattern = L"text{" + wsPattern + L"}";
          wsPattern = wsTestPattern;
          patternType = XFA_VT_TEXT;
        }
      } break;
    }
  }
  CXFA_LocaleValue localeValue(patternType, wsValue, wsPattern, pLocale, pMgr);
  WideString wsRet;
  if (!localeValue.FormatPatterns(wsRet, wsPattern, pLocale,
                                  XFA_VALUEPICTURE_Display)) {
    args.GetReturnValue()->SetString("");
    return;
  }

  args.GetReturnValue()->SetString(wsRet.UTF8Encode().AsStringView());
}

// static
void CFXJSE_FormCalcContext::Left(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Left");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  if ((ValueIsNull(pThis, argOne.get())) ||
      (ValueIsNull(pThis, argTwo.get()))) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString sourceString = ValueToUTF8String(argOne.get());
  int32_t count = std::max(0, ValueToInteger(pThis, argTwo.get()));
  args.GetReturnValue()->SetString(sourceString.Left(count).AsStringView());
}

// static
void CFXJSE_FormCalcContext::Len(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Len");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString sourceString = ValueToUTF8String(argOne.get());
  args.GetReturnValue()->SetInteger(sourceString.GetLength());
}

// static
void CFXJSE_FormCalcContext::Lower(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Lower");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  CFX_WideTextBuf lowStringBuf;
  ByteString argString = ValueToUTF8String(argOne.get());
  WideString wsArgString = WideString::FromUTF8(argString.AsStringView());
  const wchar_t* pData = wsArgString.c_str();
  size_t i = 0;
  while (i < argString.GetLength()) {
    int32_t ch = pData[i];
    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0xC0 && ch <= 0xDE))
      ch += 32;
    else if (ch == 0x100 || ch == 0x102 || ch == 0x104)
      ch += 1;

    lowStringBuf.AppendChar(ch);
    ++i;
  }
  lowStringBuf.AppendChar(0);

  args.GetReturnValue()->SetString(
      FX_UTF8Encode(lowStringBuf.AsStringView()).AsStringView());
}

// static
void CFXJSE_FormCalcContext::Ltrim(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Ltrim");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString sourceString = ValueToUTF8String(argOne.get());
  sourceString.TrimLeft();
  args.GetReturnValue()->SetString(sourceString.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Parse(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 2) {
    pContext->ThrowParamCountMismatchException(L"Parse");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  if (ValueIsNull(pThis, argTwo.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString szPattern = ValueToUTF8String(argOne.get());
  ByteString szValue = ValueToUTF8String(argTwo.get());
  CXFA_Document* pDoc = pContext->GetDocument();
  CXFA_LocaleMgr* pMgr = pDoc->GetLocalMgr();
  CXFA_Node* pThisNode = ToNode(pDoc->GetScriptContext()->GetThisObject());
  ASSERT(pThisNode);

  LocaleIface* pLocale = pThisNode->GetLocale();
  WideString wsPattern = WideString::FromUTF8(szPattern.AsStringView());
  WideString wsValue = WideString::FromUTF8(szValue.AsStringView());
  uint32_t patternType;
  if (PatternStringType(szPattern.AsStringView(), patternType)) {
    CXFA_LocaleValue localeValue(patternType, wsValue, wsPattern, pLocale,
                                 pMgr);
    if (!localeValue.IsValid()) {
      args.GetReturnValue()->SetString("");
      return;
    }
    args.GetReturnValue()->SetString(
        localeValue.GetValue().UTF8Encode().AsStringView());
    return;
  }

  switch (patternType) {
    case XFA_VT_DATETIME: {
      auto iTChar = wsPattern.Find(L'T');
      if (!iTChar.has_value()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      WideString wsDatePattern(L"date{" + wsPattern.Left(iTChar.value()) +
                               L"} ");
      WideString wsTimePattern(
          L"time{" +
          wsPattern.Right(wsPattern.GetLength() - (iTChar.value() + 1)) + L"}");
      wsPattern = wsDatePattern + wsTimePattern;
      CXFA_LocaleValue localeValue(patternType, wsValue, wsPattern, pLocale,
                                   pMgr);
      if (!localeValue.IsValid()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      args.GetReturnValue()->SetString(
          localeValue.GetValue().UTF8Encode().AsStringView());
      return;
    }
    case XFA_VT_DATE: {
      wsPattern = L"date{" + wsPattern + L"}";
      CXFA_LocaleValue localeValue(patternType, wsValue, wsPattern, pLocale,
                                   pMgr);
      if (!localeValue.IsValid()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      args.GetReturnValue()->SetString(
          localeValue.GetValue().UTF8Encode().AsStringView());
      return;
    }
    case XFA_VT_TIME: {
      wsPattern = L"time{" + wsPattern + L"}";
      CXFA_LocaleValue localeValue(patternType, wsValue, wsPattern, pLocale,
                                   pMgr);
      if (!localeValue.IsValid()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      args.GetReturnValue()->SetString(
          localeValue.GetValue().UTF8Encode().AsStringView());
      return;
    }
    case XFA_VT_TEXT: {
      wsPattern = L"text{" + wsPattern + L"}";
      CXFA_LocaleValue localeValue(XFA_VT_TEXT, wsValue, wsPattern, pLocale,
                                   pMgr);
      if (!localeValue.IsValid()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      args.GetReturnValue()->SetString(
          localeValue.GetValue().UTF8Encode().AsStringView());
      return;
    }
    case XFA_VT_FLOAT: {
      wsPattern = L"num{" + wsPattern + L"}";
      CXFA_LocaleValue localeValue(XFA_VT_FLOAT, wsValue, wsPattern, pLocale,
                                   pMgr);
      if (!localeValue.IsValid()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      args.GetReturnValue()->SetDouble(localeValue.GetDoubleNum());
      return;
    }
    default: {
      WideString wsTestPattern;
      wsTestPattern = L"num{" + wsPattern + L"}";
      CXFA_LocaleValue localeValue(XFA_VT_FLOAT, wsValue, wsTestPattern,
                                   pLocale, pMgr);
      if (localeValue.IsValid()) {
        args.GetReturnValue()->SetDouble(localeValue.GetDoubleNum());
        return;
      }

      wsTestPattern = L"text{" + wsPattern + L"}";
      CXFA_LocaleValue localeValue2(XFA_VT_TEXT, wsValue, wsTestPattern,
                                    pLocale, pMgr);
      if (!localeValue2.IsValid()) {
        args.GetReturnValue()->SetString("");
        return;
      }
      args.GetReturnValue()->SetString(
          localeValue2.GetValue().UTF8Encode().AsStringView());
      return;
    }
  }
}

// static
void CFXJSE_FormCalcContext::Replace(CFXJSE_Value* pThis,
                                     const ByteStringView& szFuncName,
                                     CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 2 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Replace");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  ByteString oneString;
  ByteString twoString;
  if (!ValueIsNull(pThis, argOne.get()) && !ValueIsNull(pThis, argTwo.get())) {
    oneString = ValueToUTF8String(argOne.get());
    twoString = ValueToUTF8String(argTwo.get());
  }

  ByteString threeString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
    threeString = ValueToUTF8String(argThree.get());
  }

  size_t iFindLen = twoString.GetLength();
  std::ostringstream resultString;
  size_t iFindIndex = 0;
  for (size_t u = 0; u < oneString.GetLength(); ++u) {
    char ch = static_cast<char>(oneString[u]);
    if (ch != static_cast<char>(twoString[iFindIndex])) {
      resultString << ch;
      continue;
    }

    size_t iTemp = u + 1;
    ++iFindIndex;
    while (iFindIndex < iFindLen) {
      uint8_t chTemp = oneString[iTemp];
      if (chTemp != twoString[iFindIndex]) {
        iFindIndex = 0;
        break;
      }

      ++iTemp;
      ++iFindIndex;
    }
    if (iFindIndex == iFindLen) {
      resultString << threeString;
      u += iFindLen - 1;
      iFindIndex = 0;
    } else {
      resultString << ch;
    }
  }
  resultString << '\0';
  args.GetReturnValue()->SetString(ByteStringView(resultString.str().c_str()));
}

// static
void CFXJSE_FormCalcContext::Right(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Right");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  if ((ValueIsNull(pThis, argOne.get())) ||
      (ValueIsNull(pThis, argTwo.get()))) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString sourceString = ValueToUTF8String(argOne.get());
  int32_t count = std::max(0, ValueToInteger(pThis, argTwo.get()));
  args.GetReturnValue()->SetString(sourceString.Right(count).AsStringView());
}

// static
void CFXJSE_FormCalcContext::Rtrim(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Rtrim");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  ByteString sourceString = ValueToUTF8String(argOne.get());
  sourceString.TrimRight();
  args.GetReturnValue()->SetString(sourceString.AsStringView());
}

// static
void CFXJSE_FormCalcContext::Space(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Space");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  int32_t count = std::max(0, ValueToInteger(pThis, argOne.get()));
  std::ostringstream spaceString;
  int32_t index = 0;
  while (index < count) {
    spaceString << ' ';
    index++;
  }
  spaceString << '\0';
  args.GetReturnValue()->SetString(ByteStringView(spaceString.str().c_str()));
}

// static
void CFXJSE_FormCalcContext::Str(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Str");
    return;
  }

  std::unique_ptr<CFXJSE_Value> numberValue = GetSimpleValue(pThis, args, 0);
  if (numberValue->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  float fNumber = ValueToFloat(pThis, numberValue.get());

  int32_t iWidth = 10;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> widthValue = GetSimpleValue(pThis, args, 1);
    iWidth = static_cast<int32_t>(ValueToFloat(pThis, widthValue.get()));
  }

  int32_t iPrecision = 0;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> precisionValue =
        GetSimpleValue(pThis, args, 2);
    iPrecision = std::max(
        0, static_cast<int32_t>(ValueToFloat(pThis, precisionValue.get())));
  }

  ByteString formatStr = "%";
  if (iPrecision) {
    formatStr += ".";
    formatStr += ByteString::FormatInteger(iPrecision);
  }
  formatStr += "f";
  ByteString numberString = ByteString::Format(formatStr.c_str(), fNumber);

  const char* pData = numberString.c_str();
  int32_t iLength = numberString.GetLength();
  int32_t u = 0;
  while (u < iLength) {
    if (pData[u] == '.')
      break;

    ++u;
  }

  std::ostringstream resultBuf;
  if (u > iWidth || (iPrecision + u) >= iWidth) {
    int32_t i = 0;
    while (i < iWidth) {
      resultBuf << '*';
      ++i;
    }
    resultBuf << '\0';
    args.GetReturnValue()->SetString(ByteStringView(resultBuf.str().c_str()));
    return;
  }

  if (u == iLength) {
    if (iLength > iWidth) {
      int32_t i = 0;
      while (i < iWidth) {
        resultBuf << '*';
        ++i;
      }
    } else {
      int32_t i = 0;
      while (i < iWidth - iLength) {
        resultBuf << ' ';
        ++i;
      }
      resultBuf << pData;
    }
    args.GetReturnValue()->SetString(ByteStringView(resultBuf.str().c_str()));
    return;
  }

  int32_t iLeavingSpace = iWidth - u - iPrecision;
  if (iPrecision != 0)
    iLeavingSpace--;

  int32_t i = 0;
  while (i < iLeavingSpace) {
    resultBuf << ' ';
    ++i;
  }
  i = 0;
  while (i < u) {
    resultBuf << pData[i];
    ++i;
  }
  if (iPrecision != 0)
    resultBuf << '.';

  u++;
  i = 0;
  while (u < iLength) {
    if (i >= iPrecision)
      break;

    resultBuf << pData[u];
    ++i;
    ++u;
  }
  while (i < iPrecision) {
    resultBuf << '0';
    ++i;
  }
  resultBuf << '\0';
  args.GetReturnValue()->SetString(ByteStringView(resultBuf.str().c_str()));
}

// static
void CFXJSE_FormCalcContext::Stuff(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 3 || argc > 4) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Stuff");
    return;
  }

  ByteString sourceString;
  ByteString insertString;
  int32_t iLength = 0;
  int32_t iStart = 0;
  int32_t iDelete = 0;
  std::unique_ptr<CFXJSE_Value> sourceValue = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> startValue = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> deleteValue = GetSimpleValue(pThis, args, 2);
  if (!sourceValue->IsNull() && !startValue->IsNull() &&
      !deleteValue->IsNull()) {
    sourceString = ValueToUTF8String(sourceValue.get());
    iLength = sourceString.GetLength();
    iStart = pdfium::clamp(
        static_cast<int32_t>(ValueToFloat(pThis, startValue.get())), 1,
        iLength);
    iDelete = std::max(
        0, static_cast<int32_t>(ValueToFloat(pThis, deleteValue.get())));
  }

  if (argc > 3) {
    std::unique_ptr<CFXJSE_Value> insertValue = GetSimpleValue(pThis, args, 3);
    insertString = ValueToUTF8String(insertValue.get());
  }

  iStart -= 1;
  std::ostringstream resultString;
  int32_t i = 0;
  while (i < iStart) {
    resultString << static_cast<char>(sourceString[i]);
    ++i;
  }
  resultString << insertString.AsStringView();
  i = iStart + iDelete;
  while (i < iLength) {
    resultString << static_cast<char>(sourceString[i]);
    ++i;
  }
  resultString << '\0';
  args.GetReturnValue()->SetString(ByteStringView(resultString.str().c_str()));
}

// static
void CFXJSE_FormCalcContext::Substr(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args) {
  if (args.GetLength() != 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Substr");
    return;
  }

  std::unique_ptr<CFXJSE_Value> stringValue = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> startValue = GetSimpleValue(pThis, args, 1);
  std::unique_ptr<CFXJSE_Value> endValue = GetSimpleValue(pThis, args, 2);
  if (ValueIsNull(pThis, stringValue.get()) ||
      (ValueIsNull(pThis, startValue.get())) ||
      (ValueIsNull(pThis, endValue.get()))) {
    args.GetReturnValue()->SetNull();
    return;
  }

  int32_t iStart = 0;
  int32_t iCount = 0;
  ByteString szSourceStr = ValueToUTF8String(stringValue.get());
  int32_t iLength = szSourceStr.GetLength();
  if (iLength == 0) {
    args.GetReturnValue()->SetString("");
    return;
  }

  iStart = pdfium::clamp(
      iLength, 1, static_cast<int32_t>(ValueToFloat(pThis, startValue.get())));
  iCount =
      std::max(0, static_cast<int32_t>(ValueToFloat(pThis, endValue.get())));

  iStart -= 1;
  args.GetReturnValue()->SetString(
      szSourceStr.Mid(iStart, iCount).AsStringView());
}

// static
void CFXJSE_FormCalcContext::Uuid(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 0 || argc > 1) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Uuid");
    return;
  }

  int32_t iNum = 0;
  if (argc > 0) {
    std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
    iNum = static_cast<int32_t>(ValueToFloat(pThis, argOne.get()));
  }
  args.GetReturnValue()->SetString(GUIDString(!!iNum).AsStringView());
}

// static
void CFXJSE_FormCalcContext::Upper(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 2) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"Upper");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (ValueIsNull(pThis, argOne.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  CFX_WideTextBuf upperStringBuf;
  ByteString argString = ValueToUTF8String(argOne.get());
  WideString wsArgString = WideString::FromUTF8(argString.AsStringView());
  const wchar_t* pData = wsArgString.c_str();
  size_t i = 0;
  while (i < wsArgString.GetLength()) {
    int32_t ch = pData[i];
    if ((ch >= 0x61 && ch <= 0x7A) || (ch >= 0xE0 && ch <= 0xFE))
      ch -= 32;
    else if (ch == 0x101 || ch == 0x103 || ch == 0x105)
      ch -= 1;

    upperStringBuf.AppendChar(ch);
    ++i;
  }
  upperStringBuf.AppendChar(0);

  args.GetReturnValue()->SetString(
      FX_UTF8Encode(upperStringBuf.AsStringView()).AsStringView());
}

// static
void CFXJSE_FormCalcContext::WordNum(CFXJSE_Value* pThis,
                                     const ByteStringView& szFuncName,
                                     CFXJSE_Arguments& args) {
  int32_t argc = args.GetLength();
  if (argc < 1 || argc > 3) {
    ToJSContext(pThis, nullptr)->ThrowParamCountMismatchException(L"WordNum");
    return;
  }

  std::unique_ptr<CFXJSE_Value> numberValue = GetSimpleValue(pThis, args, 0);
  if (numberValue->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  float fNumber = ValueToFloat(pThis, numberValue.get());

  int32_t iIdentifier = 0;
  if (argc > 1) {
    std::unique_ptr<CFXJSE_Value> identifierValue =
        GetSimpleValue(pThis, args, 1);
    if (identifierValue->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    iIdentifier =
        static_cast<int32_t>(ValueToFloat(pThis, identifierValue.get()));
  }

  ByteString localeString;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> localeValue = GetSimpleValue(pThis, args, 2);
    if (localeValue->IsNull()) {
      args.GetReturnValue()->SetNull();
      return;
    }
    localeString = ValueToUTF8String(localeValue.get());
  }

  if (fNumber < 0.0f || fNumber > 922337203685477550.0f) {
    args.GetReturnValue()->SetString("*");
    return;
  }

  args.GetReturnValue()->SetString(
      WordUS(ByteString::Format("%.2f", fNumber), iIdentifier).AsStringView());
}

// static
ByteString CFXJSE_FormCalcContext::TrillionUS(const ByteStringView& szData) {
  std::ostringstream strBuf;
  ByteStringView pUnits[] = {"zero", "one", "two",   "three", "four",
                             "five", "six", "seven", "eight", "nine"};
  ByteStringView pCapUnits[] = {"Zero", "One", "Two",   "Three", "Four",
                                "Five", "Six", "Seven", "Eight", "Nine"};
  ByteStringView pTens[] = {"Ten",      "Eleven",  "Twelve",  "Thirteen",
                            "Fourteen", "Fifteen", "Sixteen", "Seventeen",
                            "Eighteen", "Nineteen"};
  ByteStringView pLastTens[] = {"Twenty", "Thirty",  "Forty",  "Fifty",
                                "Sixty",  "Seventy", "Eighty", "Ninety"};
  ByteStringView pComm[] = {" Hundred ", " Thousand ", " Million ", " Billion ",
                            "Trillion"};
  const char* pData = szData.unterminated_c_str();
  int32_t iLength = szData.GetLength();
  int32_t iComm = 0;
  if (iLength > 12)
    iComm = 4;
  else if (iLength > 9)
    iComm = 3;
  else if (iLength > 6)
    iComm = 2;
  else if (iLength > 3)
    iComm = 1;

  int32_t iFirstCount = iLength % 3;
  if (iFirstCount == 0)
    iFirstCount = 3;

  int32_t iIndex = 0;
  if (iFirstCount == 3) {
    if (pData[iIndex] != '0') {
      strBuf << pCapUnits[pData[iIndex] - '0'];
      strBuf << pComm[0];
    }
    if (pData[iIndex + 1] == '0') {
      strBuf << pCapUnits[pData[iIndex + 2] - '0'];
    } else {
      if (pData[iIndex + 1] > '1') {
        strBuf << pLastTens[pData[iIndex + 1] - '2'];
        strBuf << "-";
        strBuf << pUnits[pData[iIndex + 2] - '0'];
      } else if (pData[iIndex + 1] == '1') {
        strBuf << pTens[pData[iIndex + 2] - '0'];
      } else if (pData[iIndex + 1] == '0') {
        strBuf << pCapUnits[pData[iIndex + 2] - '0'];
      }
    }
    iIndex += 3;
  } else if (iFirstCount == 2) {
    if (pData[iIndex] == '0') {
      strBuf << pCapUnits[pData[iIndex + 1] - '0'];
    } else {
      if (pData[iIndex] > '1') {
        strBuf << pLastTens[pData[iIndex] - '2'];
        strBuf << "-";
        strBuf << pUnits[pData[iIndex + 1] - '0'];
      } else if (pData[iIndex] == '1') {
        strBuf << pTens[pData[iIndex + 1] - '0'];
      } else if (pData[iIndex] == '0') {
        strBuf << pCapUnits[pData[iIndex + 1] - '0'];
      }
    }
    iIndex += 2;
  } else if (iFirstCount == 1) {
    strBuf << pCapUnits[pData[iIndex] - '0'];
    iIndex += 1;
  }
  if (iLength > 3 && iFirstCount > 0) {
    strBuf << pComm[iComm];
    --iComm;
  }
  while (iIndex < iLength) {
    if (pData[iIndex] != '0') {
      strBuf << pCapUnits[pData[iIndex] - '0'];
      strBuf << pComm[0];
    }
    if (pData[iIndex + 1] == '0') {
      strBuf << pCapUnits[pData[iIndex + 2] - '0'];
    } else {
      if (pData[iIndex + 1] > '1') {
        strBuf << pLastTens[pData[iIndex + 1] - '2'];
        strBuf << "-";
        strBuf << pUnits[pData[iIndex + 2] - '0'];
      } else if (pData[iIndex + 1] == '1') {
        strBuf << pTens[pData[iIndex + 2] - '0'];
      } else if (pData[iIndex + 1] == '0') {
        strBuf << pCapUnits[pData[iIndex + 2] - '0'];
      }
    }
    if (iIndex < iLength - 3) {
      strBuf << pComm[iComm];
      --iComm;
    }
    iIndex += 3;
  }
  return ByteString(strBuf);
}

// static
ByteString CFXJSE_FormCalcContext::WordUS(const ByteString& szData,
                                          int32_t iStyle) {
  const char* pData = szData.c_str();
  int32_t iLength = szData.GetLength();
  if (iStyle < 0 || iStyle > 2) {
    return ByteString();
  }

  std::ostringstream strBuf;

  int32_t iIndex = 0;
  while (iIndex < iLength) {
    if (pData[iIndex] == '.')
      break;
    ++iIndex;
  }
  int32_t iInteger = iIndex;
  iIndex = 0;
  while (iIndex < iInteger) {
    int32_t iCount = (iInteger - iIndex) % 12;
    if (!iCount && iInteger - iIndex > 0)
      iCount = 12;

    strBuf << TrillionUS(ByteStringView(pData + iIndex, iCount));
    iIndex += iCount;
    if (iIndex < iInteger)
      strBuf << " Trillion ";
  }

  if (iStyle > 0)
    strBuf << " Dollars";

  if (iStyle > 1 && iInteger < iLength) {
    strBuf << " And ";
    iIndex = iInteger + 1;
    while (iIndex < iLength) {
      int32_t iCount = (iLength - iIndex) % 12;
      if (!iCount && iLength - iIndex > 0)
        iCount = 12;

      strBuf << TrillionUS(ByteStringView(pData + iIndex, iCount));
      iIndex += iCount;
      if (iIndex < iLength)
        strBuf << " Trillion ";
    }
    strBuf << " Cents";
  }
  return ByteString(strBuf);
}

// static
void CFXJSE_FormCalcContext::Get(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 1) {
    pContext->ThrowParamCountMismatchException(L"Get");
    return;
  }

  CXFA_Document* pDoc = pContext->GetDocument();
  if (!pDoc)
    return;

  IXFA_AppProvider* pAppProvider = pDoc->GetNotify()->GetAppProvider();
  if (!pAppProvider)
    return;

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  ByteString urlString = ValueToUTF8String(argOne.get());
  RetainPtr<IFX_SeekableReadStream> pFile =
      pAppProvider->DownloadURL(WideString::FromUTF8(urlString.AsStringView()));
  if (!pFile)
    return;

  int32_t size = pFile->GetSize();
  std::vector<uint8_t> dataBuf(size);
  pFile->ReadBlock(dataBuf.data(), size);
  args.GetReturnValue()->SetString(ByteStringView(dataBuf));
}

// static
void CFXJSE_FormCalcContext::Post(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  int32_t argc = args.GetLength();
  if (argc < 2 || argc > 5) {
    pContext->ThrowParamCountMismatchException(L"Post");
    return;
  }

  CXFA_Document* pDoc = pContext->GetDocument();
  if (!pDoc)
    return;

  IXFA_AppProvider* pAppProvider = pDoc->GetNotify()->GetAppProvider();
  if (!pAppProvider)
    return;

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  ByteString bsURL = ValueToUTF8String(argOne.get());

  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  ByteString bsData = ValueToUTF8String(argTwo.get());

  ByteString bsContentType;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
    bsContentType = ValueToUTF8String(argThree.get());
  }

  ByteString bsEncode;
  if (argc > 3) {
    std::unique_ptr<CFXJSE_Value> argFour = GetSimpleValue(pThis, args, 3);
    bsEncode = ValueToUTF8String(argFour.get());
  }

  ByteString bsHeader;
  if (argc > 4) {
    std::unique_ptr<CFXJSE_Value> argFive = GetSimpleValue(pThis, args, 4);
    bsHeader = ValueToUTF8String(argFive.get());
  }

  WideString decodedResponse;
  if (!pAppProvider->PostRequestURL(
          WideString::FromUTF8(bsURL.AsStringView()),
          WideString::FromUTF8(bsData.AsStringView()),
          WideString::FromUTF8(bsContentType.AsStringView()),
          WideString::FromUTF8(bsEncode.AsStringView()),
          WideString::FromUTF8(bsHeader.AsStringView()), decodedResponse)) {
    pContext->ThrowServerDeniedException();
    return;
  }
  args.GetReturnValue()->SetString(decodedResponse.UTF8Encode().AsStringView());
}

// static
void CFXJSE_FormCalcContext::Put(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  int32_t argc = args.GetLength();
  if (argc < 2 || argc > 3) {
    pContext->ThrowParamCountMismatchException(L"Put");
    return;
  }

  CXFA_Document* pDoc = pContext->GetDocument();
  if (!pDoc)
    return;

  IXFA_AppProvider* pAppProvider = pDoc->GetNotify()->GetAppProvider();
  if (!pAppProvider)
    return;

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  ByteString bsURL = ValueToUTF8String(argOne.get());

  std::unique_ptr<CFXJSE_Value> argTwo = GetSimpleValue(pThis, args, 1);
  ByteString bsData = ValueToUTF8String(argTwo.get());

  ByteString bsEncode;
  if (argc > 2) {
    std::unique_ptr<CFXJSE_Value> argThree = GetSimpleValue(pThis, args, 2);
    bsEncode = ValueToUTF8String(argThree.get());
  }

  if (!pAppProvider->PutRequestURL(
          WideString::FromUTF8(bsURL.AsStringView()),
          WideString::FromUTF8(bsData.AsStringView()),
          WideString::FromUTF8(bsEncode.AsStringView()))) {
    pContext->ThrowServerDeniedException();
    return;
  }

  args.GetReturnValue()->SetString("");
}

// static
void CFXJSE_FormCalcContext::assign_value_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 2) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> lValue = args.GetValue(0);
  std::unique_ptr<CFXJSE_Value> rValue = GetSimpleValue(pThis, args, 1);
  if (lValue->IsArray()) {
    v8::Isolate* pIsolate = pContext->GetScriptRuntime();
    auto leftLengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    lValue->GetObjectProperty("length", leftLengthValue.get());
    int32_t iLeftLength = leftLengthValue->ToInteger();
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    lValue->GetObjectPropertyByIdx(1, propertyValue.get());
    if (propertyValue->IsNull()) {
      for (int32_t i = 2; i < iLeftLength; i++) {
        lValue->GetObjectPropertyByIdx(i, jsObjectValue.get());
        if (!SetObjectDefaultValue(jsObjectValue.get(), rValue.get())) {
          pContext->ThrowNoDefaultPropertyException(szFuncName);
          return;
        }
      }
    } else {
      for (int32_t i = 2; i < iLeftLength; i++) {
        lValue->GetObjectPropertyByIdx(i, jsObjectValue.get());
        jsObjectValue->SetObjectProperty(
            propertyValue->ToString().AsStringView(), rValue.get());
      }
    }
  } else if (lValue->IsObject()) {
    if (!SetObjectDefaultValue(lValue.get(), rValue.get())) {
      pContext->ThrowNoDefaultPropertyException(szFuncName);
      return;
    }
  }
  args.GetReturnValue()->Assign(rValue.get());
}

// static
void CFXJSE_FormCalcContext::logical_or_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() && argSecond->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float first = ValueToFloat(pThis, argFirst.get());
  float second = ValueToFloat(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first || second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::logical_and_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() && argSecond->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  float first = ValueToFloat(pThis, argFirst.get());
  float second = ValueToFloat(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first && second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::equality_operator(CFXJSE_Value* pThis,
                                               const ByteStringView& szFuncName,
                                               CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  if (fm_ref_equal(pThis, args)) {
    args.GetReturnValue()->SetInteger(1);
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() || argSecond->IsNull()) {
    args.GetReturnValue()->SetInteger(
        (argFirst->IsNull() && argSecond->IsNull()) ? 1 : 0);
    return;
  }

  if (argFirst->IsString() && argSecond->IsString()) {
    args.GetReturnValue()->SetInteger(argFirst->ToString() ==
                                      argSecond->ToString());
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first == second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::notequality_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  if (fm_ref_equal(pThis, args)) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() || argSecond->IsNull()) {
    args.GetReturnValue()->SetInteger(
        (argFirst->IsNull() && argSecond->IsNull()) ? 0 : 1);
    return;
  }

  if (argFirst->IsString() && argSecond->IsString()) {
    args.GetReturnValue()->SetInteger(argFirst->ToString() !=
                                      argSecond->ToString());
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger(first != second);
}

// static
bool CFXJSE_FormCalcContext::fm_ref_equal(CFXJSE_Value* pThis,
                                          CFXJSE_Arguments& args) {
  std::unique_ptr<CFXJSE_Value> argFirst = args.GetValue(0);
  std::unique_ptr<CFXJSE_Value> argSecond = args.GetValue(1);
  if (!argFirst->IsArray() || !argSecond->IsArray())
    return false;

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  auto firstFlagValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  auto secondFlagValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  argFirst->GetObjectPropertyByIdx(0, firstFlagValue.get());
  argSecond->GetObjectPropertyByIdx(0, secondFlagValue.get());
  if (firstFlagValue->ToInteger() != 3 || secondFlagValue->ToInteger() != 3)
    return false;

  auto firstJSObject = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  auto secondJSObject = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  argFirst->GetObjectPropertyByIdx(2, firstJSObject.get());
  argSecond->GetObjectPropertyByIdx(2, secondJSObject.get());
  if (firstJSObject->IsNull() || secondJSObject->IsNull())
    return false;

  return (firstJSObject->ToHostObject(nullptr) ==
          secondJSObject->ToHostObject(nullptr));
}

// static
void CFXJSE_FormCalcContext::less_operator(CFXJSE_Value* pThis,
                                           const ByteStringView& szFuncName,
                                           CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() || argSecond->IsNull()) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }

  if (argFirst->IsString() && argSecond->IsString()) {
    int result =
        argFirst->ToString().Compare(argSecond->ToString().AsStringView()) < 0;
    args.GetReturnValue()->SetInteger(result);
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first < second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::lessequal_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() || argSecond->IsNull()) {
    args.GetReturnValue()->SetInteger(
        (argFirst->IsNull() && argSecond->IsNull()) ? 1 : 0);
    return;
  }

  if (argFirst->IsString() && argSecond->IsString()) {
    int result =
        argFirst->ToString().Compare(argSecond->ToString().AsStringView()) <= 0;
    args.GetReturnValue()->SetInteger(result);
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first <= second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::greater_operator(CFXJSE_Value* pThis,
                                              const ByteStringView& szFuncName,
                                              CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() || argSecond->IsNull()) {
    args.GetReturnValue()->SetInteger(0);
    return;
  }

  if (argFirst->IsString() && argSecond->IsString()) {
    int result =
        argFirst->ToString().Compare(argSecond->ToString().AsStringView()) > 0;
    args.GetReturnValue()->SetInteger(result);
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first > second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::greaterequal_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() || argSecond->IsNull()) {
    args.GetReturnValue()->SetInteger(
        (argFirst->IsNull() && argSecond->IsNull()) ? 1 : 0);
    return;
  }

  if (argFirst->IsString() && argSecond->IsString()) {
    int result =
        argFirst->ToString().Compare(argSecond->ToString().AsStringView()) >= 0;
    args.GetReturnValue()->SetInteger(result);
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetInteger((first >= second) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::plus_operator(CFXJSE_Value* pThis,
                                           const ByteStringView& szFuncName,
                                           CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = args.GetValue(0);
  std::unique_ptr<CFXJSE_Value> argSecond = args.GetValue(1);
  if (ValueIsNull(pThis, argFirst.get()) &&
      ValueIsNull(pThis, argSecond.get())) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetDouble(first + second);
}

// static
void CFXJSE_FormCalcContext::minus_operator(CFXJSE_Value* pThis,
                                            const ByteStringView& szFuncName,
                                            CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() && argSecond->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetDouble(first - second);
}

// static
void CFXJSE_FormCalcContext::multiple_operator(CFXJSE_Value* pThis,
                                               const ByteStringView& szFuncName,
                                               CFXJSE_Arguments& args) {
  if (args.GetLength() != 2) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() && argSecond->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  double second = ValueToDouble(pThis, argSecond.get());
  args.GetReturnValue()->SetDouble(first * second);
}

// static
void CFXJSE_FormCalcContext::divide_operator(CFXJSE_Value* pThis,
                                             const ByteStringView& szFuncName,
                                             CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 2) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argFirst = GetSimpleValue(pThis, args, 0);
  std::unique_ptr<CFXJSE_Value> argSecond = GetSimpleValue(pThis, args, 1);
  if (argFirst->IsNull() && argSecond->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double second = ValueToDouble(pThis, argSecond.get());
  if (second == 0.0) {
    pContext->ThrowDivideByZeroException();
    return;
  }

  double first = ValueToDouble(pThis, argFirst.get());
  args.GetReturnValue()->SetDouble(first / second);
}

// static
void CFXJSE_FormCalcContext::positive_operator(CFXJSE_Value* pThis,
                                               const ByteStringView& szFuncName,
                                               CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  args.GetReturnValue()->SetDouble(0.0 + ValueToDouble(pThis, argOne.get()));
}

// static
void CFXJSE_FormCalcContext::negative_operator(CFXJSE_Value* pThis,
                                               const ByteStringView& szFuncName,
                                               CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }
  args.GetReturnValue()->SetDouble(0.0 - ValueToDouble(pThis, argOne.get()));
}

// static
void CFXJSE_FormCalcContext::logical_not_operator(
    CFXJSE_Value* pThis,
    const ByteStringView& szFuncName,
    CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  if (argOne->IsNull()) {
    args.GetReturnValue()->SetNull();
    return;
  }

  double first = ValueToDouble(pThis, argOne.get());
  args.GetReturnValue()->SetInteger((first == 0.0) ? 1 : 0);
}

// static
void CFXJSE_FormCalcContext::dot_accessor(CFXJSE_Value* pThis,
                                          const ByteStringView& szFuncName,
                                          CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  int32_t argc = args.GetLength();
  if (argc < 4 || argc > 5) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  bool bIsStar = true;
  int32_t iIndexValue = 0;
  if (argc > 4) {
    bIsStar = false;
    iIndexValue = ValueToInteger(pThis, args.GetValue(4).get());
  }

  ByteString szName = args.GetUTF8String(2);
  ByteString szSomExp = GenerateSomExpression(
      szName.AsStringView(), args.GetInt32(3), iIndexValue, bIsStar);

  std::unique_ptr<CFXJSE_Value> argAccessor = args.GetValue(0);
  if (argAccessor->IsArray()) {
    auto pLengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argAccessor->GetObjectProperty("length", pLengthValue.get());
    int32_t iLength = pLengthValue->ToInteger();
    if (iLength < 3) {
      pContext->ThrowArgumentMismatchException();
      return;
    }

    auto hJSObjValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    std::vector<std::vector<std::unique_ptr<CFXJSE_Value>>> resolveValues(
        iLength - 2);
    bool bAttribute = false;
    int32_t iCounter = 0;
    for (int32_t i = 2; i < iLength; i++) {
      argAccessor->GetObjectPropertyByIdx(i, hJSObjValue.get());

      XFA_RESOLVENODE_RS resolveNodeRS;
      if (ResolveObjects(pThis, hJSObjValue.get(), szSomExp.AsStringView(),
                         &resolveNodeRS, true, szName.IsEmpty())) {
        ParseResolveResult(pThis, resolveNodeRS, hJSObjValue.get(),
                           &resolveValues[i - 2], &bAttribute);
        iCounter += resolveValues[i - 2].size();
      }
    }
    if (iCounter < 1) {
      pContext->ThrowPropertyNotInObjectException(
          WideString::FromUTF8(szName.AsStringView()),
          WideString::FromUTF8(szSomExp.AsStringView()));
      return;
    }

    std::vector<std::unique_ptr<CFXJSE_Value>> values;
    for (int32_t i = 0; i < iCounter + 2; i++)
      values.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

    values[0]->SetInteger(1);
    if (bAttribute)
      values[1]->SetString(szName.AsStringView());
    else
      values[1]->SetNull();

    int32_t iIndex = 2;
    for (int32_t i = 0; i < iLength - 2; i++) {
      for (size_t j = 0; j < resolveValues[i].size(); j++) {
        values[iIndex]->Assign(resolveValues[i][j].get());
        iIndex++;
      }
    }
    args.GetReturnValue()->SetArray(values);
    return;
  }

  XFA_RESOLVENODE_RS resolveNodeRS;
  bool iRet = false;
  ByteString bsAccessorName = args.GetUTF8String(1);
  if (argAccessor->IsObject() ||
      (argAccessor->IsNull() && bsAccessorName.IsEmpty())) {
    iRet = ResolveObjects(pThis, argAccessor.get(), szSomExp.AsStringView(),
                          &resolveNodeRS, true, szName.IsEmpty());
  } else if (!argAccessor->IsObject() && !bsAccessorName.IsEmpty() &&
             GetObjectForName(pThis, argAccessor.get(),
                              bsAccessorName.AsStringView())) {
    iRet = ResolveObjects(pThis, argAccessor.get(), szSomExp.AsStringView(),
                          &resolveNodeRS, true, szName.IsEmpty());
  }
  if (!iRet) {
    pContext->ThrowPropertyNotInObjectException(
        WideString::FromUTF8(szName.AsStringView()),
        WideString::FromUTF8(szSomExp.AsStringView()));
    return;
  }

  std::vector<std::unique_ptr<CFXJSE_Value>> resolveValues;
  bool bAttribute = false;
  ParseResolveResult(pThis, resolveNodeRS, argAccessor.get(), &resolveValues,
                     &bAttribute);

  std::vector<std::unique_ptr<CFXJSE_Value>> values;
  for (size_t i = 0; i < resolveValues.size() + 2; i++)
    values.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

  values[0]->SetInteger(1);
  if (bAttribute)
    values[1]->SetString(szName.AsStringView());
  else
    values[1]->SetNull();

  for (size_t i = 0; i < resolveValues.size(); i++)
    values[i + 2]->Assign(resolveValues[i].get());

  args.GetReturnValue()->SetArray(values);
}

// static
void CFXJSE_FormCalcContext::dotdot_accessor(CFXJSE_Value* pThis,
                                             const ByteStringView& szFuncName,
                                             CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  int32_t argc = args.GetLength();
  if (argc < 4 || argc > 5) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  bool bIsStar = true;
  int32_t iIndexValue = 0;
  if (argc > 4) {
    bIsStar = false;
    iIndexValue = ValueToInteger(pThis, args.GetValue(4).get());
  }

  ByteString szName = args.GetUTF8String(2);
  ByteString szSomExp = GenerateSomExpression(
      szName.AsStringView(), args.GetInt32(3), iIndexValue, bIsStar);

  std::unique_ptr<CFXJSE_Value> argAccessor = args.GetValue(0);
  if (argAccessor->IsArray()) {
    auto pLengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argAccessor->GetObjectProperty("length", pLengthValue.get());
    int32_t iLength = pLengthValue->ToInteger();
    if (iLength < 3) {
      pContext->ThrowArgumentMismatchException();
      return;
    }

    int32_t iCounter = 0;

    std::vector<std::vector<std::unique_ptr<CFXJSE_Value>>> resolveValues(
        iLength - 2);
    auto hJSObjValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    bool bAttribute = false;
    for (int32_t i = 2; i < iLength; i++) {
      argAccessor->GetObjectPropertyByIdx(i, hJSObjValue.get());
      XFA_RESOLVENODE_RS resolveNodeRS;
      if (ResolveObjects(pThis, hJSObjValue.get(), szSomExp.AsStringView(),
                         &resolveNodeRS, false, false)) {
        ParseResolveResult(pThis, resolveNodeRS, hJSObjValue.get(),
                           &resolveValues[i - 2], &bAttribute);
        iCounter += resolveValues[i - 2].size();
      }
    }
    if (iCounter < 1) {
      pContext->ThrowPropertyNotInObjectException(
          WideString::FromUTF8(szName.AsStringView()),
          WideString::FromUTF8(szSomExp.AsStringView()));
      return;
    }

    std::vector<std::unique_ptr<CFXJSE_Value>> values;
    for (int32_t i = 0; i < iCounter + 2; i++)
      values.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

    values[0]->SetInteger(1);
    if (bAttribute)
      values[1]->SetString(szName.AsStringView());
    else
      values[1]->SetNull();

    int32_t iIndex = 2;
    for (int32_t i = 0; i < iLength - 2; i++) {
      for (size_t j = 0; j < resolveValues[i].size(); j++) {
        values[iIndex]->Assign(resolveValues[i][j].get());
        iIndex++;
      }
    }
    args.GetReturnValue()->SetArray(values);
    return;
  }

  XFA_RESOLVENODE_RS resolveNodeRS;
  bool iRet = false;
  ByteString bsAccessorName = args.GetUTF8String(1);
  if (argAccessor->IsObject() ||
      (argAccessor->IsNull() && bsAccessorName.IsEmpty())) {
    iRet = ResolveObjects(pThis, argAccessor.get(), szSomExp.AsStringView(),
                          &resolveNodeRS, false, false);
  } else if (!argAccessor->IsObject() && !bsAccessorName.IsEmpty() &&
             GetObjectForName(pThis, argAccessor.get(),
                              bsAccessorName.AsStringView())) {
    iRet = ResolveObjects(pThis, argAccessor.get(), szSomExp.AsStringView(),
                          &resolveNodeRS, false, false);
  }
  if (!iRet) {
    pContext->ThrowPropertyNotInObjectException(
        WideString::FromUTF8(szName.AsStringView()),
        WideString::FromUTF8(szSomExp.AsStringView()));
    return;
  }

  std::vector<std::unique_ptr<CFXJSE_Value>> resolveValues;
  bool bAttribute = false;
  ParseResolveResult(pThis, resolveNodeRS, argAccessor.get(), &resolveValues,
                     &bAttribute);

  std::vector<std::unique_ptr<CFXJSE_Value>> values;
  for (size_t i = 0; i < resolveValues.size() + 2; i++)
    values.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

  values[0]->SetInteger(1);
  if (bAttribute)
    values[1]->SetString(szName.AsStringView());
  else
    values[1]->SetNull();

  for (size_t i = 0; i < resolveValues.size(); i++)
    values[i + 2]->Assign(resolveValues[i].get());

  args.GetReturnValue()->SetArray(values);
}

// static
void CFXJSE_FormCalcContext::eval_translation(CFXJSE_Value* pThis,
                                              const ByteStringView& szFuncName,
                                              CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 1) {
    pContext->ThrowParamCountMismatchException(L"Eval");
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = GetSimpleValue(pThis, args, 0);
  ByteString argString = ValueToUTF8String(argOne.get());
  if (argString.IsEmpty()) {
    pContext->ThrowArgumentMismatchException();
    return;
  }

  WideString scriptString = WideString::FromUTF8(argString.AsStringView());
  CFX_WideTextBuf wsJavaScriptBuf;
  if (!CFXJSE_FormCalcContext::Translate(scriptString.AsStringView(),
                                         &wsJavaScriptBuf)) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  args.GetReturnValue()->SetString(
      FX_UTF8Encode(wsJavaScriptBuf.AsStringView()).AsStringView());
}

// static
void CFXJSE_FormCalcContext::is_fm_object(CFXJSE_Value* pThis,
                                          const ByteStringView& szFuncName,
                                          CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    args.GetReturnValue()->SetBoolean(false);
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  args.GetReturnValue()->SetBoolean(argOne->IsObject());
}

// static
void CFXJSE_FormCalcContext::is_fm_array(CFXJSE_Value* pThis,
                                         const ByteStringView& szFuncName,
                                         CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    args.GetReturnValue()->SetBoolean(false);
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  args.GetReturnValue()->SetBoolean(argOne->IsArray());
}

// static
void CFXJSE_FormCalcContext::get_fm_value(CFXJSE_Value* pThis,
                                          const ByteStringView& szFuncName,
                                          CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 1) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (argOne->IsArray()) {
    v8::Isolate* pIsolate = pContext->GetScriptRuntime();
    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argOne->GetObjectPropertyByIdx(1, propertyValue.get());
    argOne->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (propertyValue->IsNull()) {
      GetObjectDefaultValue(jsObjectValue.get(), args.GetReturnValue());
      return;
    }

    jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                     args.GetReturnValue());
    return;
  }

  if (argOne->IsObject()) {
    GetObjectDefaultValue(argOne.get(), args.GetReturnValue());
    return;
  }

  args.GetReturnValue()->Assign(argOne.get());
}

// static
void CFXJSE_FormCalcContext::get_fm_jsobj(CFXJSE_Value* pThis,
                                          const ByteStringView& szFuncName,
                                          CFXJSE_Arguments& args) {
  if (args.GetLength() != 1) {
    ToJSContext(pThis, nullptr)->ThrowCompilerErrorException();
    return;
  }

  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (!argOne->IsArray()) {
    args.GetReturnValue()->Assign(argOne.get());
    return;
  }

#ifndef NDEBUG
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  argOne->GetObjectProperty("length", lengthValue.get());
  ASSERT(lengthValue->ToInteger() >= 3);
#endif

  argOne->GetObjectPropertyByIdx(2, args.GetReturnValue());
}

// static
void CFXJSE_FormCalcContext::fm_var_filter(CFXJSE_Value* pThis,
                                           const ByteStringView& szFuncName,
                                           CFXJSE_Arguments& args) {
  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  if (args.GetLength() != 1) {
    pContext->ThrowCompilerErrorException();
    return;
  }

  v8::Isolate* pIsolate = pContext->GetScriptRuntime();
  std::unique_ptr<CFXJSE_Value> argOne = args.GetValue(0);
  if (!argOne->IsArray()) {
    std::unique_ptr<CFXJSE_Value> simpleValue = GetSimpleValue(pThis, args, 0);
    args.GetReturnValue()->Assign(simpleValue.get());
    return;
  }

#ifndef NDEBUG
  auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  argOne->GetObjectProperty("length", lengthValue.get());
  ASSERT(lengthValue->ToInteger() >= 3);
#endif

  auto flagsValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  argOne->GetObjectPropertyByIdx(0, flagsValue.get());
  int32_t iFlags = flagsValue->ToInteger();
  if (iFlags != 3 && iFlags != 4) {
    std::unique_ptr<CFXJSE_Value> simpleValue = GetSimpleValue(pThis, args, 0);
    args.GetReturnValue()->Assign(simpleValue.get());
    return;
  }

  if (iFlags == 4) {
    std::vector<std::unique_ptr<CFXJSE_Value>> values;
    for (int32_t i = 0; i < 3; i++)
      values.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

    values[0]->SetInteger(3);
    values[1]->SetNull();
    values[2]->SetNull();
    args.GetReturnValue()->SetArray(values);
    return;
  }

  auto objectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  argOne->GetObjectPropertyByIdx(2, objectValue.get());
  if (objectValue->IsNull()) {
    pContext->ThrowCompilerErrorException();
    return;
  }
  args.GetReturnValue()->Assign(argOne.get());
}

// static
void CFXJSE_FormCalcContext::concat_fm_object(CFXJSE_Value* pThis,
                                              const ByteStringView& szFuncName,
                                              CFXJSE_Arguments& args) {
  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  uint32_t iLength = 0;
  int32_t argc = args.GetLength();
  std::vector<std::unique_ptr<CFXJSE_Value>> argValues;
  for (int32_t i = 0; i < argc; i++) {
    argValues.push_back(args.GetValue(i));
    if (argValues[i]->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValues[i]->GetObjectProperty("length", lengthValue.get());
      int32_t length = lengthValue->ToInteger();
      iLength = iLength + ((length > 2) ? (length - 2) : 0);
    }
    iLength += 1;
  }

  std::vector<std::unique_ptr<CFXJSE_Value>> returnValues;
  for (int32_t i = 0; i < (int32_t)iLength; i++)
    returnValues.push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

  int32_t index = 0;
  for (int32_t i = 0; i < argc; i++) {
    if (argValues[i]->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argValues[i]->GetObjectProperty("length", lengthValue.get());

      int32_t length = lengthValue->ToInteger();
      for (int32_t j = 2; j < length; j++) {
        argValues[i]->GetObjectPropertyByIdx(j, returnValues[index].get());
        index++;
      }
    }
    returnValues[index]->Assign(argValues[i].get());
    index++;
  }
  args.GetReturnValue()->SetArray(returnValues);
}

// static
std::unique_ptr<CFXJSE_Value> CFXJSE_FormCalcContext::GetSimpleValue(
    CFXJSE_Value* pThis,
    CFXJSE_Arguments& args,
    uint32_t index) {
  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  ASSERT(index < (uint32_t)args.GetLength());

  std::unique_ptr<CFXJSE_Value> argIndex = args.GetValue(index);
  if (!argIndex->IsArray() && !argIndex->IsObject())
    return argIndex;

  if (argIndex->IsArray()) {
    auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argIndex->GetObjectProperty("length", lengthValue.get());
    int32_t iLength = lengthValue->ToInteger();
    auto simpleValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    if (iLength < 3) {
      simpleValue.get()->SetUndefined();
      return simpleValue;
    }

    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    argIndex->GetObjectPropertyByIdx(1, propertyValue.get());
    argIndex->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (propertyValue->IsNull()) {
      GetObjectDefaultValue(jsObjectValue.get(), simpleValue.get());
      return simpleValue;
    }

    jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                     simpleValue.get());
    return simpleValue;
  }

  auto defaultValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  GetObjectDefaultValue(argIndex.get(), defaultValue.get());
  return defaultValue;
}

// static
bool CFXJSE_FormCalcContext::ValueIsNull(CFXJSE_Value* pThis,
                                         CFXJSE_Value* arg) {
  if (!arg || arg->IsNull())
    return true;

  if (!arg->IsArray() && !arg->IsObject())
    return false;

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  if (arg->IsArray()) {
    int32_t iLength = hvalue_get_array_length(pThis, arg);
    if (iLength < 3)
      return true;

    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    arg->GetObjectPropertyByIdx(1, propertyValue.get());
    arg->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (propertyValue->IsNull()) {
      auto defaultValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      GetObjectDefaultValue(jsObjectValue.get(), defaultValue.get());
      return defaultValue->IsNull();
    }

    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                     newPropertyValue.get());
    return newPropertyValue->IsNull();
  }

  auto defaultValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  GetObjectDefaultValue(arg, defaultValue.get());
  return defaultValue->IsNull();
}

// static
int32_t CFXJSE_FormCalcContext::hvalue_get_array_length(CFXJSE_Value* pThis,
                                                        CFXJSE_Value* arg) {
  if (!arg || !arg->IsArray())
    return 0;

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  arg->GetObjectProperty("length", lengthValue.get());
  return lengthValue->ToInteger();
}

// static
bool CFXJSE_FormCalcContext::simpleValueCompare(CFXJSE_Value* pThis,
                                                CFXJSE_Value* firstValue,
                                                CFXJSE_Value* secondValue) {
  if (!firstValue)
    return false;

  if (firstValue->IsString()) {
    ByteString firstString = ValueToUTF8String(firstValue);
    ByteString secondString = ValueToUTF8String(secondValue);
    return firstString == secondString;
  }
  if (firstValue->IsNumber()) {
    float first = ValueToFloat(pThis, firstValue);
    float second = ValueToFloat(pThis, secondValue);
    return first == second;
  }
  if (firstValue->IsBoolean())
    return firstValue->ToBoolean() == secondValue->ToBoolean();

  return firstValue->IsNull() && secondValue && secondValue->IsNull();
}

// static
void CFXJSE_FormCalcContext::unfoldArgs(
    CFXJSE_Value* pThis,
    CFXJSE_Arguments& args,
    std::vector<std::unique_ptr<CFXJSE_Value>>* resultValues,
    int32_t iStart) {
  resultValues->clear();

  int32_t iCount = 0;
  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  int32_t argc = args.GetLength();
  std::vector<std::unique_ptr<CFXJSE_Value>> argsValue;
  for (int32_t i = 0; i < argc - iStart; i++) {
    argsValue.push_back(args.GetValue(i + iStart));
    if (argsValue[i]->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argsValue[i]->GetObjectProperty("length", lengthValue.get());
      int32_t iLength = lengthValue->ToInteger();
      iCount += ((iLength > 2) ? (iLength - 2) : 0);
    } else {
      iCount += 1;
    }
  }

  for (int32_t i = 0; i < iCount; i++)
    resultValues->push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));

  int32_t index = 0;
  for (int32_t i = 0; i < argc - iStart; i++) {
    if (argsValue[i]->IsArray()) {
      auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argsValue[i]->GetObjectProperty("length", lengthValue.get());
      int32_t iLength = lengthValue->ToInteger();
      if (iLength < 3)
        continue;

      auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      argsValue[i]->GetObjectPropertyByIdx(1, propertyValue.get());
      if (propertyValue->IsNull()) {
        for (int32_t j = 2; j < iLength; j++) {
          argsValue[i]->GetObjectPropertyByIdx(j, jsObjectValue.get());
          GetObjectDefaultValue(jsObjectValue.get(),
                                (*resultValues)[index].get());
          index++;
        }
      } else {
        for (int32_t j = 2; j < iLength; j++) {
          argsValue[i]->GetObjectPropertyByIdx(j, jsObjectValue.get());
          jsObjectValue->GetObjectProperty(
              propertyValue->ToString().AsStringView(),
              (*resultValues)[index].get());
          index++;
        }
      }
    } else if (argsValue[i]->IsObject()) {
      GetObjectDefaultValue(argsValue[i].get(), (*resultValues)[index].get());
      index++;
    } else {
      (*resultValues)[index]->Assign(argsValue[i].get());
      index++;
    }
  }
}

// static
void CFXJSE_FormCalcContext::GetObjectDefaultValue(
    CFXJSE_Value* pValue,
    CFXJSE_Value* pDefaultValue) {
  CXFA_Node* pNode = ToNode(CFXJSE_Engine::ToObject(pValue, nullptr));
  if (!pNode) {
    pDefaultValue->SetNull();
    return;
  }
  pNode->JSObject()->Script_Som_DefaultValue(pDefaultValue, false,
                                             XFA_Attribute::Unknown);
}

// static
bool CFXJSE_FormCalcContext::SetObjectDefaultValue(CFXJSE_Value* pValue,
                                                   CFXJSE_Value* hNewValue) {
  CXFA_Node* pNode = ToNode(CFXJSE_Engine::ToObject(pValue, nullptr));
  if (!pNode)
    return false;

  pNode->JSObject()->Script_Som_DefaultValue(hNewValue, true,
                                             XFA_Attribute::Unknown);
  return true;
}

// static
ByteString CFXJSE_FormCalcContext::GenerateSomExpression(
    const ByteStringView& szName,
    int32_t iIndexFlags,
    int32_t iIndexValue,
    bool bIsStar) {
  if (bIsStar)
    return ByteString(szName, "[*]");

  if (iIndexFlags == 0)
    return ByteString(szName);

  if (iIndexFlags == 1 || iIndexValue == 0) {
    return ByteString(szName, "[") + ByteString::FormatInteger(iIndexValue) +
           "]";
  }
  ByteString szSomExp;
  if (iIndexFlags == 2) {
    szSomExp = (iIndexValue < 0) ? (szName + "[-") : (szName + "[+");
    iIndexValue = (iIndexValue < 0) ? (0 - iIndexValue) : iIndexValue;
    szSomExp += ByteString::FormatInteger(iIndexValue);
    szSomExp += "]";
  } else {
    szSomExp = (iIndexValue < 0) ? (szName + "[") : (szName + "[-");
    iIndexValue = (iIndexValue < 0) ? (0 - iIndexValue) : iIndexValue;
    szSomExp += ByteString::FormatInteger(iIndexValue);
    szSomExp += "]";
  }
  return szSomExp;
}

// static
bool CFXJSE_FormCalcContext::GetObjectForName(
    CFXJSE_Value* pThis,
    CFXJSE_Value* accessorValue,
    const ByteStringView& szAccessorName) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return false;

  CFXJSE_Engine* pScriptContext = pDoc->GetScriptContext();
  XFA_RESOLVENODE_RS resolveNodeRS;
  uint32_t dwFlags = XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Properties |
                     XFA_RESOLVENODE_Siblings | XFA_RESOLVENODE_Parent;
  bool iRet = pScriptContext->ResolveObjects(
      pScriptContext->GetThisObject(),
      WideString::FromUTF8(szAccessorName).AsStringView(), &resolveNodeRS,
      dwFlags, nullptr);
  if (iRet && resolveNodeRS.dwFlags == XFA_ResolveNode_RSType_Nodes) {
    accessorValue->Assign(
        pScriptContext->GetJSValueFromMap(resolveNodeRS.objects.front()));
    return true;
  }
  return false;
}

// static
bool CFXJSE_FormCalcContext::ResolveObjects(CFXJSE_Value* pThis,
                                            CFXJSE_Value* pRefValue,
                                            const ByteStringView& bsSomExp,
                                            XFA_RESOLVENODE_RS* resolveNodeRS,
                                            bool bdotAccessor,
                                            bool bHasNoResolveName) {
  CXFA_Document* pDoc = ToJSContext(pThis, nullptr)->GetDocument();
  if (!pDoc)
    return false;

  WideString wsSomExpression = WideString::FromUTF8(bsSomExp);
  CFXJSE_Engine* pScriptContext = pDoc->GetScriptContext();
  CXFA_Object* pNode = nullptr;
  uint32_t dFlags = 0UL;
  if (bdotAccessor) {
    if (pRefValue && pRefValue->IsNull()) {
      pNode = pScriptContext->GetThisObject();
      dFlags = XFA_RESOLVENODE_Siblings | XFA_RESOLVENODE_Parent;
    } else {
      pNode = CFXJSE_Engine::ToObject(pRefValue, nullptr);
      if (!pNode)
        return false;

      if (bHasNoResolveName) {
        WideString wsName;
        if (CXFA_Node* pXFANode = pNode->AsNode()) {
          Optional<WideString> ret =
              pXFANode->JSObject()->TryAttribute(XFA_Attribute::Name, false);
          if (ret)
            wsName = *ret;
        }
        if (wsName.IsEmpty())
          wsName = L"#" + pNode->GetClassName();

        wsSomExpression = wsName + wsSomExpression;
        dFlags = XFA_RESOLVENODE_Siblings;
      } else {
        dFlags = (bsSomExp == "*")
                     ? (XFA_RESOLVENODE_Children)
                     : (XFA_RESOLVENODE_Children | XFA_RESOLVENODE_Attributes |
                        XFA_RESOLVENODE_Properties);
      }
    }
  } else {
    pNode = CFXJSE_Engine::ToObject(pRefValue, nullptr);
    dFlags = XFA_RESOLVENODE_AnyChild;
  }
  return pScriptContext->ResolveObjects(pNode, wsSomExpression.AsStringView(),
                                        resolveNodeRS, dFlags, nullptr);
}

// static
void CFXJSE_FormCalcContext::ParseResolveResult(
    CFXJSE_Value* pThis,
    const XFA_RESOLVENODE_RS& resolveNodeRS,
    CFXJSE_Value* pParentValue,
    std::vector<std::unique_ptr<CFXJSE_Value>>* resultValues,
    bool* bAttribute) {
  ASSERT(bAttribute);

  resultValues->clear();

  CFXJSE_FormCalcContext* pContext = ToJSContext(pThis, nullptr);
  v8::Isolate* pIsolate = pContext->GetScriptRuntime();

  if (resolveNodeRS.dwFlags == XFA_ResolveNode_RSType_Nodes) {
    *bAttribute = false;
    CFXJSE_Engine* pScriptContext = pContext->GetDocument()->GetScriptContext();
    for (CXFA_Object* pObject : resolveNodeRS.objects) {
      resultValues->push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));
      resultValues->back()->Assign(pScriptContext->GetJSValueFromMap(pObject));
    }
    return;
  }

  *bAttribute = true;
  if (resolveNodeRS.pScriptAttribute &&
      resolveNodeRS.pScriptAttribute->eValueType == XFA_ScriptType::Object) {
    for (CXFA_Object* pObject : resolveNodeRS.objects) {
      auto pValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
      CJX_Object* jsObject = pObject->JSObject();
      (jsObject->*(resolveNodeRS.pScriptAttribute->callback))(
          pValue.get(), false, resolveNodeRS.pScriptAttribute->attribute);

      resultValues->push_back(std::move(pValue));
      *bAttribute = false;
    }
  }
  if (!*bAttribute)
    return;
  if (!pParentValue || !pParentValue->IsObject())
    return;

  resultValues->push_back(pdfium::MakeUnique<CFXJSE_Value>(pIsolate));
  resultValues->back()->Assign(pParentValue);
}

// static
int32_t CFXJSE_FormCalcContext::ValueToInteger(CFXJSE_Value* pThis,
                                               CFXJSE_Value* pValue) {
  if (!pValue)
    return 0;

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  if (pValue->IsArray()) {
    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    pValue->GetObjectPropertyByIdx(1, propertyValue.get());
    pValue->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (propertyValue->IsNull()) {
      GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
      return ValueToInteger(pThis, newPropertyValue.get());
    }

    jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                     newPropertyValue.get());
    return ValueToInteger(pThis, newPropertyValue.get());
  }
  if (pValue->IsObject()) {
    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    GetObjectDefaultValue(pValue, newPropertyValue.get());
    return ValueToInteger(pThis, newPropertyValue.get());
  }
  if (pValue->IsString())
    return FXSYS_atoi(pValue->ToString().c_str());
  return pValue->ToInteger();
}

// static
float CFXJSE_FormCalcContext::ValueToFloat(CFXJSE_Value* pThis,
                                           CFXJSE_Value* arg) {
  if (!arg)
    return 0.0f;

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  if (arg->IsArray()) {
    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    arg->GetObjectPropertyByIdx(1, propertyValue.get());
    arg->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (propertyValue->IsNull()) {
      GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
      return ValueToFloat(pThis, newPropertyValue.get());
    }
    jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                     newPropertyValue.get());
    return ValueToFloat(pThis, newPropertyValue.get());
  }
  if (arg->IsObject()) {
    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    GetObjectDefaultValue(arg, newPropertyValue.get());
    return ValueToFloat(pThis, newPropertyValue.get());
  }
  if (arg->IsString()) {
    return static_cast<float>(
        ByteStringToDouble(arg->ToString().AsStringView()));
  }
  if (arg->IsUndefined())
    return 0;

  return arg->ToFloat();
}

// static
double CFXJSE_FormCalcContext::ValueToDouble(CFXJSE_Value* pThis,
                                             CFXJSE_Value* arg) {
  if (!arg)
    return 0;

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  if (arg->IsArray()) {
    auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    arg->GetObjectPropertyByIdx(1, propertyValue.get());
    arg->GetObjectPropertyByIdx(2, jsObjectValue.get());
    if (propertyValue->IsNull()) {
      GetObjectDefaultValue(jsObjectValue.get(), newPropertyValue.get());
      return ValueToDouble(pThis, newPropertyValue.get());
    }
    jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                     newPropertyValue.get());
    return ValueToDouble(pThis, newPropertyValue.get());
  }
  if (arg->IsObject()) {
    auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
    GetObjectDefaultValue(arg, newPropertyValue.get());
    return ValueToDouble(pThis, newPropertyValue.get());
  }
  if (arg->IsString())
    return ByteStringToDouble(arg->ToString().AsStringView());
  if (arg->IsUndefined())
    return 0;
  return arg->ToDouble();
}

// static.
double CFXJSE_FormCalcContext::ExtractDouble(CFXJSE_Value* pThis,
                                             CFXJSE_Value* src,
                                             bool* ret) {
  ASSERT(ret);
  *ret = true;

  if (!src)
    return 0;

  if (!src->IsArray())
    return ValueToDouble(pThis, src);

  v8::Isolate* pIsolate = ToJSContext(pThis, nullptr)->GetScriptRuntime();
  auto lengthValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  src->GetObjectProperty("length", lengthValue.get());
  int32_t iLength = lengthValue->ToInteger();
  if (iLength <= 2) {
    *ret = false;
    return 0.0;
  }

  auto propertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  auto jsObjectValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  src->GetObjectPropertyByIdx(1, propertyValue.get());
  src->GetObjectPropertyByIdx(2, jsObjectValue.get());
  if (propertyValue->IsNull())
    return ValueToDouble(pThis, jsObjectValue.get());

  auto newPropertyValue = pdfium::MakeUnique<CFXJSE_Value>(pIsolate);
  jsObjectValue->GetObjectProperty(propertyValue->ToString().AsStringView(),
                                   newPropertyValue.get());
  return ValueToDouble(pThis, newPropertyValue.get());
}

// static
ByteString CFXJSE_FormCalcContext::ValueToUTF8String(CFXJSE_Value* arg) {
  if (!arg || arg->IsNull() || arg->IsUndefined())
    return ByteString();
  if (arg->IsBoolean())
    return arg->ToBoolean() ? "1" : "0";
  return arg->ToString();
}

// static.
bool CFXJSE_FormCalcContext::Translate(const WideStringView& wsFormcalc,
                                       CFX_WideTextBuf* wsJavascript) {
  if (wsFormcalc.IsEmpty()) {
    wsJavascript->Clear();
    return true;
  }

  CXFA_FMParser parser(wsFormcalc);
  std::unique_ptr<CXFA_FMAST> ast = parser.Parse();
  if (!ast || parser.HasError())
    return false;

  CXFA_FMToJavaScriptDepth::Reset();
  if (!ast->ToJavaScript(wsJavascript))
    return false;

  wsJavascript->AppendChar(0);

  return !CXFA_IsTooBig(wsJavascript);
}

CFXJSE_FormCalcContext::CFXJSE_FormCalcContext(v8::Isolate* pScriptIsolate,
                                               CFXJSE_Context* pScriptContext,
                                               CXFA_Document* pDoc)
    : CFXJSE_HostObject(kFM2JS),
      m_pIsolate(pScriptIsolate),
      m_pFMClass(CFXJSE_Class::Create(pScriptContext,
                                      &kFormCalcFM2JSDescriptor,
                                      false)),
      m_pValue(pdfium::MakeUnique<CFXJSE_Value>(pScriptIsolate)),
      m_pDocument(pDoc) {
  m_pValue.get()->SetObject(this, m_pFMClass);
}

CFXJSE_FormCalcContext::~CFXJSE_FormCalcContext() {}

void CFXJSE_FormCalcContext::GlobalPropertyGetter(CFXJSE_Value* pValue) {
  pValue->Assign(m_pValue.get());
}

void CFXJSE_FormCalcContext::ThrowNoDefaultPropertyException(
    const ByteStringView& name) const {
  // TODO(tsepez): check usage of c_str() below.
  ThrowException(L"%.16S doesn't have a default property.",
                 name.unterminated_c_str());
}

void CFXJSE_FormCalcContext::ThrowCompilerErrorException() const {
  ThrowException(L"Compiler error.");
}

void CFXJSE_FormCalcContext::ThrowDivideByZeroException() const {
  ThrowException(L"Divide by zero.");
}

void CFXJSE_FormCalcContext::ThrowServerDeniedException() const {
  ThrowException(L"Server does not permit operation.");
}

void CFXJSE_FormCalcContext::ThrowPropertyNotInObjectException(
    const WideString& name,
    const WideString& exp) const {
  ThrowException(
      L"An attempt was made to reference property '%.16s' of a non-object "
      L"in SOM expression %.16s.",
      name.c_str(), exp.c_str());
}

void CFXJSE_FormCalcContext::ThrowParamCountMismatchException(
    const WideString& method) const {
  ThrowException(L"Incorrect number of parameters calling method '%.16s'.",
                 method.c_str());
}

void CFXJSE_FormCalcContext::ThrowArgumentMismatchException() const {
  ThrowException(L"Argument mismatch in property or function argument.");
}

void CFXJSE_FormCalcContext::ThrowException(const wchar_t* str, ...) const {
  va_list arg_ptr;
  va_start(arg_ptr, str);
  WideString wsMessage = WideString::FormatV(str, arg_ptr);
  va_end(arg_ptr);

  ASSERT(!wsMessage.IsEmpty());
  FXJSE_ThrowMessage(wsMessage.UTF8Encode().AsStringView());
}
