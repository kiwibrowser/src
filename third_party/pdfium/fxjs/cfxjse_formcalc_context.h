// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CFXJSE_FORMCALC_CONTEXT_H_
#define FXJS_CFXJSE_FORMCALC_CONTEXT_H_

#include <memory>
#include <vector>

#include "fxjs/cfxjse_arguments.h"
#include "fxjs/cfxjse_context.h"
#include "xfa/fxfa/parser/xfa_resolvenode_rs.h"

class CFX_WideTextBuf;
class CXFA_Document;

class CFXJSE_FormCalcContext : public CFXJSE_HostObject {
 public:
  CFXJSE_FormCalcContext(v8::Isolate* pScriptIsolate,
                         CFXJSE_Context* pScriptContext,
                         CXFA_Document* pDoc);
  ~CFXJSE_FormCalcContext() override;

  static void Abs(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Avg(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Ceil(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Count(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Floor(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Max(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Min(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Mod(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Round(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Sum(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Date(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Date2Num(CFXJSE_Value* pThis,
                       const ByteStringView& szFuncName,
                       CFXJSE_Arguments& args);
  static void DateFmt(CFXJSE_Value* pThis,
                      const ByteStringView& szFuncName,
                      CFXJSE_Arguments& args);
  static void IsoDate2Num(CFXJSE_Value* pThis,
                          const ByteStringView& szFuncName,
                          CFXJSE_Arguments& args);
  static void IsoTime2Num(CFXJSE_Value* pThis,
                          const ByteStringView& szFuncName,
                          CFXJSE_Arguments& args);
  static void LocalDateFmt(CFXJSE_Value* pThis,
                           const ByteStringView& szFuncName,
                           CFXJSE_Arguments& args);
  static void LocalTimeFmt(CFXJSE_Value* pThis,
                           const ByteStringView& szFuncName,
                           CFXJSE_Arguments& args);
  static void Num2Date(CFXJSE_Value* pThis,
                       const ByteStringView& szFuncName,
                       CFXJSE_Arguments& args);
  static void Num2GMTime(CFXJSE_Value* pThis,
                         const ByteStringView& szFuncName,
                         CFXJSE_Arguments& args);
  static void Num2Time(CFXJSE_Value* pThis,
                       const ByteStringView& szFuncName,
                       CFXJSE_Arguments& args);
  static void Time(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Time2Num(CFXJSE_Value* pThis,
                       const ByteStringView& szFuncName,
                       CFXJSE_Arguments& args);
  static void TimeFmt(CFXJSE_Value* pThis,
                      const ByteStringView& szFuncName,
                      CFXJSE_Arguments& args);

  static bool IsIsoDateFormat(const char* pData,
                              int32_t iLength,
                              int32_t& iStyle,
                              int32_t& iYear,
                              int32_t& iMonth,
                              int32_t& iDay);
  static bool IsIsoTimeFormat(const char* pData,
                              int32_t iLength,
                              int32_t& iHour,
                              int32_t& iMinute,
                              int32_t& iSecond,
                              int32_t& iMilliSecond,
                              int32_t& iZoneHour,
                              int32_t& iZoneMinute);
  static bool IsIsoDateTimeFormat(const char* pData,
                                  int32_t iLength,
                                  int32_t& iYear,
                                  int32_t& iMonth,
                                  int32_t& iDay,
                                  int32_t& iHour,
                                  int32_t& iMinute,
                                  int32_t& iSecond,
                                  int32_t& iMillionSecond,
                                  int32_t& iZoneHour,
                                  int32_t& iZoneMinute);
  static ByteString Local2IsoDate(CFXJSE_Value* pThis,
                                  const ByteStringView& szDate,
                                  const ByteStringView& szFormat,
                                  const ByteStringView& szLocale);
  static ByteString IsoDate2Local(CFXJSE_Value* pThis,
                                  const ByteStringView& szDate,
                                  const ByteStringView& szFormat,
                                  const ByteStringView& szLocale);
  static ByteString IsoTime2Local(CFXJSE_Value* pThis,
                                  const ByteStringView& szTime,
                                  const ByteStringView& szFormat,
                                  const ByteStringView& szLocale);
  static int32_t DateString2Num(const ByteStringView& szDateString);
  static ByteString GetLocalDateFormat(CFXJSE_Value* pThis,
                                       int32_t iStyle,
                                       const ByteStringView& szLocalStr,
                                       bool bStandard);
  static ByteString GetLocalTimeFormat(CFXJSE_Value* pThis,
                                       int32_t iStyle,
                                       const ByteStringView& szLocalStr,
                                       bool bStandard);
  static ByteString GetStandardDateFormat(CFXJSE_Value* pThis,
                                          int32_t iStyle,
                                          const ByteStringView& szLocalStr);
  static ByteString GetStandardTimeFormat(CFXJSE_Value* pThis,
                                          int32_t iStyle,
                                          const ByteStringView& szLocalStr);
  static ByteString Num2AllTime(CFXJSE_Value* pThis,
                                int32_t iTime,
                                const ByteStringView& szFormat,
                                const ByteStringView& szLocale,
                                bool bGM);
  static void GetLocalTimeZone(int32_t& iHour, int32_t& iMin, int32_t& iSec);

  static void Apr(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void CTerm(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void FV(CFXJSE_Value* pThis,
                 const ByteStringView& szFuncName,
                 CFXJSE_Arguments& args);
  static void IPmt(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void NPV(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Pmt(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void PPmt(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void PV(CFXJSE_Value* pThis,
                 const ByteStringView& szFuncName,
                 CFXJSE_Arguments& args);
  static void Rate(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Term(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Choose(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static void Exists(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static void HasValue(CFXJSE_Value* pThis,
                       const ByteStringView& szFuncName,
                       CFXJSE_Arguments& args);
  static void Oneof(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Within(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static void If(CFXJSE_Value* pThis,
                 const ByteStringView& szFuncName,
                 CFXJSE_Arguments& args);
  static void Eval(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Ref(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void UnitType(CFXJSE_Value* pThis,
                       const ByteStringView& szFuncName,
                       CFXJSE_Arguments& args);
  static void UnitValue(CFXJSE_Value* pThis,
                        const ByteStringView& szFuncName,
                        CFXJSE_Arguments& args);

  static void At(CFXJSE_Value* pThis,
                 const ByteStringView& szFuncName,
                 CFXJSE_Arguments& args);
  static void Concat(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static void Decode(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static WideString DecodeURL(const WideString& wsURLString);
  static WideString DecodeHTML(const WideString& wsHTMLString);
  static WideString DecodeXML(const WideString& wsXMLString);
  static void Encode(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static WideString EncodeURL(const ByteString& szURLString);
  static WideString EncodeHTML(const ByteString& szHTMLString);
  static WideString EncodeXML(const ByteString& szXMLString);
  static bool HTMLSTR2Code(const WideStringView& pData, uint32_t* iCode);
  static bool HTMLCode2STR(uint32_t iCode, WideString* wsHTMLReserve);
  static void Format(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static void Left(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Len(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Lower(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Ltrim(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Parse(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Replace(CFXJSE_Value* pThis,
                      const ByteStringView& szFuncName,
                      CFXJSE_Arguments& args);
  static void Right(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Rtrim(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Space(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Str(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Stuff(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void Substr(CFXJSE_Value* pThis,
                     const ByteStringView& szFuncName,
                     CFXJSE_Arguments& args);
  static void Uuid(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Upper(CFXJSE_Value* pThis,
                    const ByteStringView& szFuncName,
                    CFXJSE_Arguments& args);
  static void WordNum(CFXJSE_Value* pThis,
                      const ByteStringView& szFuncName,
                      CFXJSE_Arguments& args);
  static ByteString TrillionUS(const ByteStringView& szData);
  static ByteString WordUS(const ByteString& szData, int32_t iStyle);

  static void Get(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void Post(CFXJSE_Value* pThis,
                   const ByteStringView& szFuncName,
                   CFXJSE_Arguments& args);
  static void Put(CFXJSE_Value* pThis,
                  const ByteStringView& szFuncName,
                  CFXJSE_Arguments& args);
  static void assign_value_operator(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args);
  static void logical_or_operator(CFXJSE_Value* pThis,
                                  const ByteStringView& szFuncName,
                                  CFXJSE_Arguments& args);
  static void logical_and_operator(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args);
  static void equality_operator(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args);
  static void notequality_operator(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args);
  static bool fm_ref_equal(CFXJSE_Value* pThis, CFXJSE_Arguments& args);
  static void less_operator(CFXJSE_Value* pThis,
                            const ByteStringView& szFuncName,
                            CFXJSE_Arguments& args);
  static void lessequal_operator(CFXJSE_Value* pThis,
                                 const ByteStringView& szFuncName,
                                 CFXJSE_Arguments& args);
  static void greater_operator(CFXJSE_Value* pThis,
                               const ByteStringView& szFuncName,
                               CFXJSE_Arguments& args);
  static void greaterequal_operator(CFXJSE_Value* pThis,
                                    const ByteStringView& szFuncName,
                                    CFXJSE_Arguments& args);
  static void plus_operator(CFXJSE_Value* pThis,
                            const ByteStringView& szFuncName,
                            CFXJSE_Arguments& args);
  static void minus_operator(CFXJSE_Value* pThis,
                             const ByteStringView& szFuncName,
                             CFXJSE_Arguments& args);
  static void multiple_operator(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args);
  static void divide_operator(CFXJSE_Value* pThis,
                              const ByteStringView& szFuncName,
                              CFXJSE_Arguments& args);
  static void positive_operator(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args);
  static void negative_operator(CFXJSE_Value* pThis,
                                const ByteStringView& szFuncName,
                                CFXJSE_Arguments& args);
  static void logical_not_operator(CFXJSE_Value* pThis,
                                   const ByteStringView& szFuncName,
                                   CFXJSE_Arguments& args);
  static void dot_accessor(CFXJSE_Value* pThis,
                           const ByteStringView& szFuncName,
                           CFXJSE_Arguments& args);
  static void dotdot_accessor(CFXJSE_Value* pThis,
                              const ByteStringView& szFuncName,
                              CFXJSE_Arguments& args);
  static void eval_translation(CFXJSE_Value* pThis,
                               const ByteStringView& szFuncName,
                               CFXJSE_Arguments& args);
  static void is_fm_object(CFXJSE_Value* pThis,
                           const ByteStringView& szFuncName,
                           CFXJSE_Arguments& args);
  static void is_fm_array(CFXJSE_Value* pThis,
                          const ByteStringView& szFuncName,
                          CFXJSE_Arguments& args);
  static void get_fm_value(CFXJSE_Value* pThis,
                           const ByteStringView& szFuncName,
                           CFXJSE_Arguments& args);
  static void get_fm_jsobj(CFXJSE_Value* pThis,
                           const ByteStringView& szFuncName,
                           CFXJSE_Arguments& args);
  static void fm_var_filter(CFXJSE_Value* pThis,
                            const ByteStringView& szFuncName,
                            CFXJSE_Arguments& args);
  static void concat_fm_object(CFXJSE_Value* pThis,
                               const ByteStringView& szFuncName,
                               CFXJSE_Arguments& args);

  static int32_t hvalue_get_array_length(CFXJSE_Value* pThis,
                                         CFXJSE_Value* arg);
  static bool simpleValueCompare(CFXJSE_Value* pThis,
                                 CFXJSE_Value* firstValue,
                                 CFXJSE_Value* secondValue);
  static void unfoldArgs(
      CFXJSE_Value* pThis,
      CFXJSE_Arguments& args,
      std::vector<std::unique_ptr<CFXJSE_Value>>* resultValues,
      int32_t iStart = 0);
  static void GetObjectDefaultValue(CFXJSE_Value* pObjectValue,
                                    CFXJSE_Value* pDefaultValue);
  static bool SetObjectDefaultValue(CFXJSE_Value* pObjectValue,
                                    CFXJSE_Value* pNewValue);
  static ByteString GenerateSomExpression(const ByteStringView& szName,
                                          int32_t iIndexFlags,
                                          int32_t iIndexValue,
                                          bool bIsStar);
  static bool GetObjectForName(CFXJSE_Value* pThis,
                               CFXJSE_Value* accessorValue,
                               const ByteStringView& szAccessorName);
  static bool ResolveObjects(CFXJSE_Value* pThis,
                             CFXJSE_Value* pParentValue,
                             const ByteStringView& bsSomExp,
                             XFA_RESOLVENODE_RS* resolveNodeRS,
                             bool bdotAccessor,
                             bool bHasNoResolveName);
  static void ParseResolveResult(
      CFXJSE_Value* pThis,
      const XFA_RESOLVENODE_RS& resolveNodeRS,
      CFXJSE_Value* pParentValue,
      std::vector<std::unique_ptr<CFXJSE_Value>>* resultValues,
      bool* bAttribute);

  static std::unique_ptr<CFXJSE_Value> GetSimpleValue(CFXJSE_Value* pThis,
                                                      CFXJSE_Arguments& args,
                                                      uint32_t index);
  static bool ValueIsNull(CFXJSE_Value* pThis, CFXJSE_Value* pValue);
  static int32_t ValueToInteger(CFXJSE_Value* pThis, CFXJSE_Value* pValue);
  static float ValueToFloat(CFXJSE_Value* pThis, CFXJSE_Value* pValue);
  static double ValueToDouble(CFXJSE_Value* pThis, CFXJSE_Value* pValue);
  static ByteString ValueToUTF8String(CFXJSE_Value* pValue);
  static double ExtractDouble(CFXJSE_Value* pThis,
                              CFXJSE_Value* src,
                              bool* ret);

  static bool Translate(const WideStringView& wsFormcalc,
                        CFX_WideTextBuf* wsJavascript);

  void GlobalPropertyGetter(CFXJSE_Value* pValue);

 private:
  v8::Isolate* GetScriptRuntime() const { return m_pIsolate; }
  CXFA_Document* GetDocument() const { return m_pDocument.Get(); }

  void ThrowNoDefaultPropertyException(const ByteStringView& name) const;
  void ThrowCompilerErrorException() const;
  void ThrowDivideByZeroException() const;
  void ThrowServerDeniedException() const;
  void ThrowPropertyNotInObjectException(const WideString& name,
                                         const WideString& exp) const;
  void ThrowArgumentMismatchException() const;
  void ThrowParamCountMismatchException(const WideString& method) const;
  void ThrowException(const wchar_t* str, ...) const;

  v8::Isolate* m_pIsolate;
  CFXJSE_Class* m_pFMClass;
  std::unique_ptr<CFXJSE_Value> m_pValue;
  UnownedPtr<CXFA_Document> const m_pDocument;
};

#endif  // FXJS_CFXJSE_FORMCALC_CONTEXT_H_
