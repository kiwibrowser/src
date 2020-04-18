// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_VALIDATION_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_VALIDATION_H_

#include <set>
#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "base/strings/string_piece_forward.h"
#include "components/autofill/core/browser/field_types.h"

namespace base {
class Time;
}  // namespace base

namespace autofill {

class CreditCard;
class AutofillProfile;

// Constants for the length of a CVC.
static const size_t GENERAL_CVC_LENGTH = 3;
static const size_t AMEX_CVC_LENGTH = 4;

// Used to express the completion status of a credit card.
typedef uint32_t CreditCardCompletionStatus;
static const CreditCardCompletionStatus CREDIT_CARD_COMPLETE = 0;
static const CreditCardCompletionStatus CREDIT_CARD_EXPIRED = 1 << 0;
static const CreditCardCompletionStatus CREDIT_CARD_NO_CARDHOLDER = 1 << 1;
static const CreditCardCompletionStatus CREDIT_CARD_NO_NUMBER = 1 << 2;
static const CreditCardCompletionStatus CREDIT_CARD_NO_BILLING_ADDRESS = 1 << 3;

// Returns true if |year| and |month| describe a date later than |now|.
// |year| must have 4 digits.
bool IsValidCreditCardExpirationDate(int year,
                                     int month,
                                     const base::Time& now);

// Returns true if |text| looks like a valid credit card number.
// Uses the Luhn formula to validate the number.
bool IsValidCreditCardNumber(const base::string16& text);

// Returns true if |number| has correct length according to card network.
bool HasCorrectLength(const base::string16& number);

// Returns true if |number| passes the validation by Luhn formula.
bool PassesLuhnCheck(base::string16& number);

// Returns true if |code| looks like a valid credit card security code
// for the given credit card type.
bool IsValidCreditCardSecurityCode(const base::string16& code,
                                   const base::StringPiece card_type);

// Returns true if |text| is a supported card type and a valid credit card
// number. |error_message| can't be null and will be filled with the appropriate
// error message.
bool IsValidCreditCardNumberForBasicCardNetworks(
    const base::string16& text,
    const std::set<std::string>& supported_basic_card_networks,
    base::string16* error_message);

// Returns the credit card's completion status. If equal to
// CREDIT_CARD_COMPLETE, then the card is ready to be used for Payment Request.
CreditCardCompletionStatus GetCompletionStatusForCard(
    const CreditCard& credit_card,
    const std::string& app_locale,
    const std::vector<AutofillProfile*> billing_addresses);

// Return the message to be displayed to the user, indicating what's missing
// to make the credit card complete for payment. If more than one thing is
// missing, the message will be a generic "more information required".
base::string16 GetCompletionMessageForCard(CreditCardCompletionStatus status);

// Returns the title string for a card edit dialog. The title string will
// mention what needs to be added/fixed to make the card valid if it is not
// valid. Otherwise, it will be "Edit card".
base::string16 GetEditDialogTitleForCard(CreditCardCompletionStatus status);

// Returns true if |text| looks like a valid e-mail address.
bool IsValidEmailAddress(const base::string16& text);

// Returns true if |text| is a valid US state name or abbreviation.  It is case
// insensitive.  Valid for US states only.
bool IsValidState(const base::string16& text);

// Returns whether the number contained in |text| is possible phone number,
// either in international format, or in the national format associated with
// |country_code|. Callers should cache the result as the parsing is expensive.
bool IsPossiblePhoneNumber(const base::string16& text,
                           const std::string& country_code);

// Returns true if |text| looks like a valid zip code.
// Valid for US zip codes only.
bool IsValidZip(const base::string16& text);

// Returns true if |text| looks like an SSN, with or without separators.
bool IsSSN(const base::string16& text);

// Returns whether |value| is valid for the given |type|. If not null,
// |error_message| is populated when the function returns false.
bool IsValidForType(const base::string16& value,
                    ServerFieldType type,
                    base::string16* error_message);

// Returns the expected CVC length based on the |card_type|.
size_t GetCvcLengthForCardType(const base::StringPiece card_type);

// Returns true if |value| appears to be a UPI Virtual Payment Address.
// https://upipayments.co.in/virtual-payment-address-vpa/
bool IsUPIVirtualPaymentAddress(const base::string16& value);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_VALIDATION_H_
