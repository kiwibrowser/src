/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_DATE_TIME_FIELD_ELEMENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_DATE_TIME_FIELD_ELEMENTS_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/html/forms/date_time_numeric_field_element.h"
#include "third_party/blink/renderer/core/html/forms/date_time_symbolic_field_element.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class DateTimeAMPMFieldElement final : public DateTimeSymbolicFieldElement {
 public:
  static DateTimeAMPMFieldElement* Create(Document&,
                                          FieldOwner&,
                                          const Vector<String>&);

 private:
  DateTimeAMPMFieldElement(Document&, FieldOwner&, const Vector<String>&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeAMPMFieldElement);
};

class DateTimeDayFieldElement final : public DateTimeNumericFieldElement {
 public:
  static DateTimeDayFieldElement* Create(Document&,
                                         FieldOwner&,
                                         const String& placeholder,
                                         const Range&);

 private:
  DateTimeDayFieldElement(Document&,
                          FieldOwner&,
                          const String& placeholder,
                          const Range&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeDayFieldElement);
};

class DateTimeHourFieldElementBase : public DateTimeNumericFieldElement {
 protected:
  DateTimeHourFieldElementBase(Document&,
                               FieldOwner&,
                               const Range&,
                               const Range& hard_limits,
                               const Step&);
  void Initialize();

 private:
  // DateTimeFieldElement functions.
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeHourFieldElementBase);
};

class DateTimeHour11FieldElement final : public DateTimeHourFieldElementBase {
 public:
  static DateTimeHour11FieldElement* Create(Document&,
                                            FieldOwner&,
                                            const Range&,
                                            const Step&);

 private:
  DateTimeHour11FieldElement(Document&,
                             FieldOwner&,
                             const Range& hour23_range,
                             const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsInteger(int, EventBehavior = kDispatchNoEvent) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeHour11FieldElement);
};

class DateTimeHour12FieldElement final : public DateTimeHourFieldElementBase {
 public:
  static DateTimeHour12FieldElement* Create(Document&,
                                            FieldOwner&,
                                            const Range&,
                                            const Step&);

 private:
  DateTimeHour12FieldElement(Document&,
                             FieldOwner&,
                             const Range& hour23_range,
                             const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsInteger(int, EventBehavior = kDispatchNoEvent) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeHour12FieldElement);
};

class DateTimeHour23FieldElement final : public DateTimeHourFieldElementBase {
 public:
  static DateTimeHour23FieldElement* Create(Document&,
                                            FieldOwner&,
                                            const Range&,
                                            const Step&);

 private:
  DateTimeHour23FieldElement(Document&,
                             FieldOwner&,
                             const Range& hour23_range,
                             const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsInteger(int, EventBehavior = kDispatchNoEvent) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeHour23FieldElement);
};

class DateTimeHour24FieldElement final : public DateTimeHourFieldElementBase {
 public:
  static DateTimeHour24FieldElement* Create(Document&,
                                            FieldOwner&,
                                            const Range&,
                                            const Step&);

 private:
  DateTimeHour24FieldElement(Document&,
                             FieldOwner&,
                             const Range& hour23_range,
                             const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsInteger(int, EventBehavior = kDispatchNoEvent) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeHour24FieldElement);
};

class DateTimeMillisecondFieldElement final
    : public DateTimeNumericFieldElement {
 public:
  static DateTimeMillisecondFieldElement* Create(Document&,
                                                 FieldOwner&,
                                                 const Range&,
                                                 const Step&);

 private:
  DateTimeMillisecondFieldElement(Document&,
                                  FieldOwner&,
                                  const Range&,
                                  const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeMillisecondFieldElement);
};

class DateTimeMinuteFieldElement final : public DateTimeNumericFieldElement {
 public:
  static DateTimeMinuteFieldElement* Create(Document&,
                                            FieldOwner&,
                                            const Range&,
                                            const Step&);

 private:
  DateTimeMinuteFieldElement(Document&, FieldOwner&, const Range&, const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeMinuteFieldElement);
};

class DateTimeMonthFieldElement final : public DateTimeNumericFieldElement {
 public:
  static DateTimeMonthFieldElement* Create(Document&,
                                           FieldOwner&,
                                           const String& placeholder,
                                           const Range&);

 private:
  DateTimeMonthFieldElement(Document&,
                            FieldOwner&,
                            const String& placeholder,
                            const Range&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeMonthFieldElement);
};

class DateTimeSecondFieldElement final : public DateTimeNumericFieldElement {
 public:
  static DateTimeSecondFieldElement* Create(Document&,
                                            FieldOwner&,
                                            const Range&,
                                            const Step&);

 private:
  DateTimeSecondFieldElement(Document&, FieldOwner&, const Range&, const Step&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeSecondFieldElement);
};

class DateTimeSymbolicMonthFieldElement final
    : public DateTimeSymbolicFieldElement {
 public:
  static DateTimeSymbolicMonthFieldElement* Create(Document&,
                                                   FieldOwner&,
                                                   const Vector<String>&,
                                                   int minimum,
                                                   int maximum);

 private:
  DateTimeSymbolicMonthFieldElement(Document&,
                                    FieldOwner&,
                                    const Vector<String>&,
                                    int minimum,
                                    int maximum);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeSymbolicMonthFieldElement);
};

class DateTimeWeekFieldElement final : public DateTimeNumericFieldElement {
 public:
  static DateTimeWeekFieldElement* Create(Document&, FieldOwner&, const Range&);

 private:
  DateTimeWeekFieldElement(Document&, FieldOwner&, const Range&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  DISALLOW_COPY_AND_ASSIGN(DateTimeWeekFieldElement);
};

class DateTimeYearFieldElement final : public DateTimeNumericFieldElement {
 public:
  struct Parameters {
    STACK_ALLOCATED();
    int minimum_year;
    int maximum_year;
    bool min_is_specified;
    bool max_is_specified;
    String placeholder;

    Parameters()
        : minimum_year(-1),
          maximum_year(-1),
          min_is_specified(false),
          max_is_specified(false) {}
  };

  static DateTimeYearFieldElement* Create(Document&,
                                          FieldOwner&,
                                          const Parameters&);

 private:
  DateTimeYearFieldElement(Document&, FieldOwner&, const Parameters&);

  // DateTimeFieldElement functions.
  void PopulateDateTimeFieldsState(DateTimeFieldsState&) override;
  void SetValueAsDate(const DateComponents&) override;
  void SetValueAsDateTimeFieldsState(const DateTimeFieldsState&) override;

  // DateTimeNumericFieldElement functions.
  int DefaultValueForStepDown() const override;
  int DefaultValueForStepUp() const override;

  bool min_is_specified_;
  bool max_is_specified_;

  DISALLOW_COPY_AND_ASSIGN(DateTimeYearFieldElement);
};

}  // namespace blink

#endif
