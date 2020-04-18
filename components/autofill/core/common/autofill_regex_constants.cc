// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains UTF8 strings that we want as char arrays.  To avoid
// different compilers, we use a script to convert the UTF8 strings into
// numeric literals (\x##).

#include "components/autofill/core/common/autofill_regex_constants.h"

namespace autofill {

/////////////////////////////////////////////////////////////////////////////
// address_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kAttentionIgnoredRe[] = "attention|attn";
const char kRegionIgnoredRe[] =
    "province|region|other"
    "|provincia"       // es
    "|bairro|suburb";  // pt-BR, pt-PT
const char kAddressNameIgnoredRe[] = "address.*nickname|address.*label";
const char kCompanyRe[] =
    "company|business|organization|organisation"
    "|firma|firmenname"    // de-DE
    "|empresa"             // es
    "|societe|société"     // fr-FR
    "|ragione.?sociale"    // it-IT
    "|会社"                // ja-JP
    "|название.?компании"  // ru
    "|单位|公司"           // zh-CN
    "|회사|직장";          // ko-KR
const char kAddressLine1Re[] =
    "^address$|address[_-]?line(one)?|address1|addr1|street"
    "|(?:shipping|billing)address$"
    "|strasse|straße|hausnummer|housenumber"  // de-DE
    "|house.?name"                            // en-GB
    "|direccion|dirección"                    // es
    "|adresse"                                // fr-FR
    "|indirizzo"                              // it-IT
    "|^住所$|住所1"                           // ja-JP
    "|morada|endereço"                        // pt-BR, pt-PT
    "|Адрес"                                  // ru
    "|地址"                                   // zh-CN
    "|^주소.?$|주소.?1";                      // ko-KR
const char kAddressLine1LabelRe[] =
    "address"
    "|adresse"    // fr-FR
    "|indirizzo"  // it-IT
    "|住所"       // ja-JP
    "|地址"       // zh-CN
    "|주소";      // ko-KR
const char kAddressLine2Re[] =
    "address[_-]?line(2|two)|address2|addr2|street|suite|unit"
    "|adresszusatz|ergänzende.?angaben"        // de-DE
    "|direccion2|colonia|adicional"            // es
    "|addresssuppl|complementnom|appartement"  // fr-FR
    "|indirizzo2"                              // it-IT
    "|住所2"                                   // ja-JP
    "|complemento|addrcomplement"              // pt-BR, pt-PT
    "|Улица"                                   // ru
    "|地址2"                                   // zh-CN
    "|주소.?2";                                // ko-KR
const char kAddressLine2LabelRe[] =
    "address|line"
    "|adresse"    // fr-FR
    "|indirizzo"  // it-IT
    "|地址"       // zh-CN
    "|주소";      // ko-KR
const char kAddressLinesExtraRe[] =
    "address.*line[3-9]|address[3-9]|addr[3-9]|street|line[3-9]"
    "|municipio"           // es
    "|batiment|residence"  // fr-FR
    "|indirizzo[3-9]";     // it-IT
const char kAddressLookupRe[] = "lookup";
const char kCountryRe[] =
    "country|countries"
    "|país|pais"   // es
    "|国"          // ja-JP
    "|国家"        // zh-CN
    "|국가|나라";  // ko-KR
const char kCountryLocationRe[] = "location";
const char kZipCodeRe[] =
    "zip|postal|post.*code|pcode"
    "|pin.?code"                // en-IN
    "|postleitzahl"             // de-DE
    "|\\bcp\\b"                 // es
    "|\\bcdp\\b"                // fr-FR
    "|\\bcap\\b"                // it-IT
    "|郵便番号"                 // ja-JP
    "|codigo|codpos|\\bcep\\b"  // pt-BR, pt-PT
    "|Почтовый.?Индекс"         // ru
    "|邮政编码|邮编"            // zh-CN
    "|郵遞區號"                 // zh-TW
    "|우편.?번호";              // ko-KR
const char kZip4Re[] =
    "zip|^-$|post2"
    "|codpos2";  // pt-BR, pt-PT
const char kCityRe[] =
    "city|town"
    "|\\bort\\b|stadt"                       // de-DE
    "|suburb"                                // en-AU
    "|ciudad|provincia|localidad|poblacion"  // es
    "|ville|commune"                         // fr-FR
    "|localita"                              // it-IT
    "|市区町村"                              // ja-JP
    "|cidade"                                // pt-BR, pt-PT
    "|Город"                                 // ru
    "|市"                                    // zh-CN
    "|分區"                                  // zh-TW
    "|^시[^도·・]|시[·・]?군[·・]?구";       // ko-KR
const char kStateRe[] =
    "(?<!united )state|county|region|province"
    "|land"                 // de-DE
    "|county|principality"  // en-UK
    "|都道府県"             // ja-JP
    "|estado|provincia"     // pt-BR, pt-PT
    "|область"              // ru
    "|省"                   // zh-CN
    "|地區"                 // zh-TW
    "|^시[·・]?도";         // ko-KR

const char kSearchTermRe[] =
    "search"
    "|suche.*"              // de-DE
    "|搜索"                 // zh-CN zh-TW
    "|探す|検索"            // ja-JP to search
    "|recherch.*"           // fr-FR
    "|busca"                // pt-BR, pt-PT
    "|جستجو"                // fa
    "|искать|найти|поиск";  // ru

/////////////////////////////////////////////////////////////////////////////
// credit_card_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kNameOnCardRe[] =
    "card.?(?:holder|owner)|name.*(\\b)?on(\\b)?.*card"
    "|(?:card|cc).?name|cc.?full.?name"
    "|karteninhaber"                   // de-DE
    "|nombre.*tarjeta"                 // es
    "|nom.*carte"                      // fr-FR
    "|nome.*cart"                      // it-IT
    "|名前"                            // ja-JP
    "|Имя.*карты"                      // ru
    "|信用卡开户名|开户名|持卡人姓名"  // zh-CN
    "|持卡人姓名";                     // zh-TW
const char kNameOnCardContextualRe[] = "name";
const char kCardNumberRe[] =
    "(add)?(?:card|cc|acct).?(?:number|#|no|num|field)"
    "|(?<!telefon|haus)nummer"  // de-DE
    "|credito|numero|número"    // es
    "|numéro"                   // fr-FR
    "|カード番号"               // ja-JP
    "|Номер.*карты"             // ru
    "|信用卡号|信用卡号码"      // zh-CN
    "|信用卡卡號"               // zh-TW
    "|카드";                    // ko-KR
const char kCardCvcRe[] =
    "verification|card.?identification|security.?code|card.?code"
    "|security.?value"
    "|security.?number|card.?pin|c-v-v"
    "|(cvn|cvv|cvc|csc|cvd|cid|ccv)(field)?"
    "|\\bcid\\b";

// "Expiration date" is the most common label here, but some pages have
// "Expires", "exp. date" or "exp. month" and "exp. year".  We also look
// for the field names ccmonth and ccyear, which appear on at least 4 of
// our test pages.

// On at least one page (The China Shop2.html) we find only the labels
// "month" and "year".  So for now we match these words directly; we'll
// see if this turns out to be too general.

// Toolbar Bug 51451: indeed, simply matching "month" is too general for
//   https://rps.fidelity.com/ftgw/rps/RtlCust/CreatePIN/Init.
// Instead, we match only words beginning with "month".
const char kExpirationMonthRe[] =
    "expir|exp.*mo|exp.*date|ccmonth|cardmonth|addmonth"
    "|gueltig|gültig|monat"  // de-DE
    "|fecha"                 // es
    "|date.*exp"             // fr-FR
    "|scadenza"              // it-IT
    "|有効期限"              // ja-JP
    "|validade"              // pt-BR, pt-PT
    "|Срок действия карты"   // ru
    "|月";                   // zh-CN
const char kExpirationYearRe[] =
    "exp|^/|(add)?year"
    "|ablaufdatum|gueltig|gültig|jahr"  // de-DE
    "|fecha"                            // es
    "|scadenza"                         // it-IT
    "|有効期限"                         // ja-JP
    "|validade"                         // pt-BR, pt-PT
    "|Срок действия карты"              // ru
    "|年|有效期";                       // zh-CN

// Used to match a expiration date field with a two digit year.
// The following conditions must be met:
//  - Exactly two adjacent y's.
//  - (optional) Exactly two adjacent m's before the y's.
//    - (optional) Separated by white-space and/or a dash or slash.
//  - (optional) Prepended with some text similar to "Expiration Date".
// Tested in components/autofill/core/common/autofill_regexes_unittest.cc
const char kExpirationDate2DigitYearRe[] =
    "(?:exp.*date[^y\\n\\r]*|mm\\s*[-/]?\\s*)yy(?:[^y]|$)";
// Used to match a expiration date field with a four digit year.
// Same requirements as |kExpirationDate2DigitYearRe| except:
//  - Exactly four adjacent y's.
// Tested in components/autofill/core/common/autofill_regexes_unittest.cc
const char kExpirationDate4DigitYearRe[] =
    "(?:exp.*date[^y\\n\\r]*|mm\\s*[-/]?\\s*)yyyy(?:[^y]|$)";
// Used to match expiration date fields that do not specify a year length.
const char kExpirationDateRe[] =
    "expir|exp.*date|^expfield$"
    "|gueltig|gültig"        // de-DE
    "|fecha"                 // es
    "|date.*exp"             // fr-FR
    "|scadenza"              // it-IT
    "|有効期限"              // ja-JP
    "|validade"              // pt-BR, pt-PT
    "|Срок действия карты";  // ru
const char kGiftCardRe[] = "gift.?card";
const char kDebitGiftCardRe[] =
    "(?:visa|mastercard|discover|amex|american express).*gift.?card";
const char kDebitCardRe[] = "debit.*card";

/////////////////////////////////////////////////////////////////////////////
// email_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kEmailRe[] =
    "e.?mail"
    "|courriel"                                    // fr
    "|メールアドレス"                              // ja-JP
    "|Электронной.?Почты"                          // ru
    "|邮件|邮箱"                                   // zh-CN
    "|電郵地址"                                    // zh-TW
    "|(?:이메일|전자.?우편|[Ee]-?mail)(.?주소)?";  // ko-KR

/////////////////////////////////////////////////////////////////////////////
// name_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kNameIgnoredRe[] =
    "user.?name|user.?id|nickname|maiden name|title|prefix|suffix"
    "|vollständiger.?name"              // de-DE
    "|用户名"                           // zh-CN
    "|(?:사용자.?)?아이디|사용자.?ID";  // ko-KR
const char kNameRe[] =
    "^name|full.?name|your.?name|customer.?name|bill.?name|ship.?name"
    "|name.*first.*last|firstandlastname"
    "|nombre.*y.*apellidos"  // es
    "|^nom"                  // fr-FR
    "|お名前|氏名"           // ja-JP
    "|^nome"                 // pt-BR, pt-PT
    "|姓名"                  // zh-CN
    "|성명";                 // ko-KR
const char kNameSpecificRe[] =
    "^name"
    "|^nom"    // fr-FR
    "|^nome";  // pt-BR, pt-PT
const char kFirstNameRe[] =
    "first.*name|initials|fname|first$|given.*name"
    "|vorname"                 // de-DE
    "|nombre"                  // es
    "|forename|prénom|prenom"  // fr-FR
    "|名"                      // ja-JP
    "|nome"                    // pt-BR, pt-PT
    "|Имя"                     // ru
    "|이름";                   // ko-KR
const char kMiddleInitialRe[] = "middle.*initial|m\\.i\\.|mi$|\\bmi\\b";
const char kMiddleNameRe[] =
    "middle.*name|mname|middle$"
    "|apellido.?materno|lastlastname";  // es
const char kLastNameRe[] =
    "last.*name|lname|surname|last$|secondname|family.*name"
    "|nachname"                            // de-DE
    "|apellido"                            // es
    "|famille|^nom"                        // fr-FR
    "|cognome"                             // it-IT
    "|姓"                                  // ja-JP
    "|morada|apelidos|surename|sobrenome"  // pt-BR, pt-PT
    "|Фамилия"                             // ru
    "|\\b성(?:[^명]|\\b)";                 // ko-KR

/////////////////////////////////////////////////////////////////////////////
// phone_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kPhoneRe[] =
    "phone|mobile|contact.?number"
    "|telefonnummer"                                // de-DE
    "|telefono|teléfono"                            // es
    "|telfixe"                                      // fr-FR
    "|電話"                                         // ja-JP
    "|telefone|telemovel"                           // pt-BR, pt-PT
    "|телефон"                                      // ru
    "|电话"                                         // zh-CN
    "|(?:전화|핸드폰|휴대폰|휴대전화)(?:.?번호)?";  // ko-KR
const char kCountryCodeRe[] = "country.*code|ccode|_cc";
const char kAreaCodeNotextRe[] = "^\\($";
const char kAreaCodeRe[] =
    "area.*code|acode|area"
    "|지역.?번호";  // ko-KR
const char kPhonePrefixSeparatorRe[] = "^-$|^\\)$";
const char kPhoneSuffixSeparatorRe[] = "^-$";
const char kPhonePrefixRe[] =
    "prefix|exchange"
    "|preselection"  // fr-FR
    "|ddd";          // pt-BR, pt-PT
const char kPhoneSuffixRe[] = "suffix";
const char kPhoneExtensionRe[] =
    "\\bext|ext\\b|extension"
    "|ramal";  // pt-BR, pt-PT

/////////////////////////////////////////////////////////////////////////////
// validation.cc
/////////////////////////////////////////////////////////////////////////////
const char kUPIVirtualPaymentAddressRe[] =
    "^\\w+@("
    "airtel|"       // My Airtel-Recharge, Bill, Bank
    "allbank|"      // Allahabad Bank UPI
    "andb|"         // Andhra Bank ONE
    "axisbank|"     // Axis Pay
    "axisgo|"       // Ola
    "barodampay|"   // Baroda MPay
    "boi|"          // BHIM BOI UPI
    "centralbank|"  // Cent UPI
    "cnrb|"         // Canara Bank UPI - Empower
    "csbpay|"       // CSB UPI
    "dbs|"          // digibank by DBS
    "dcb|"          // DCB Bank
    "denabank|"     // Dena Bank E-UPI
    "fbl|"          // Cointab
    "federal|"      // Lotza
    "hdfcbank|"     // HDFC Bank MobileBanking
    "hsbc|"         // HSBC Simply Pay
    "icici|"        // Pockets- ICICI Bank
    "idbi|"         // PayWiz
    "idfcbank|"     // IDFC Bank UPI App
    "indianbank|"   // Indian Bank UPI
    "indus|"        // Indus Pay
    "iob|"          // IOB UPI
    "jkb|"          // BHIM JK Bank UPI
    "jsb|"          // JetPay UPI
    "kaypay|"       // KayPay
    "kbl|"          // KBL Smartz
    "kotak|"        // kotak Mahindra Bank
    "kvb|"          // KVB Upay
    "lvb|"          // LVB UPay
    "mahb|"         // MAHAUPI
    "obc|"          // Oriental BHIM UPI
    "okicici|"      // Tez
    "okhdfcbank|"   // Tez
    "okaxis|"       // Tez
    "paytm|"        // Paytm
    "pingpay|"      // Samsung Pay
    "pnb|"          // PNB UPI
    "pockets|"      // Pockets- ICICI Bank
    "psb|"          // PSB UPI App
    "rbl|"          // RBL Pay
    "sbi|"          // SBI Pay
    "scb|"          // Standard Chartered
    "sib|"          // SIB M-Pay (UPI Pay)
    "syndicate|"    // Synd UPI
    "tjsb|"         // TranZapp
    "ubi|"          // United UPI
    "uboi|"         // Union Bank UPI
    "uco|"          // UCO UPI
    "unionbank|"    // Union Bank UPI
    "united|"       // United UPI
    "upi|"          // BHIM Bharat Interface for Money
    "utbi|"         // United UPI
    "vijb|"         // Vijaya UPI App
    "ybl|"          // Yes Pay
    "yesbank"       // NuPay
    ")$";

/////////////////////////////////////////////////////////////////////////////
// form_structure.cc
/////////////////////////////////////////////////////////////////////////////
const char kUrlSearchActionRe[] = "/search(/|((\\w*\\.\\w+)?$))";

}  // namespace autofill
