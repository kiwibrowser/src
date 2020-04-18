# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autofill_task.autofill_task import AutofillTask

# pylint: disable=g-multiple-import
# pylint: disable=unused-import
from autofill_task.actions import (SetContext, Open, Click, Type, Wait, Select,
                                   ByID, ByClassName, ByCssSelector, Screenshot,
                                   ByXPath, ValidateFields, TriggerAutofill)

from autofill_task.generator import Generator


class TestNeweggComGuestCheckout(AutofillTask):
  def _create_script(self):
    self.script = [
        Open('http://www.newegg.com/Product/Product.aspx?Item=N82E16823126097'),
        Click(ByXPath('//*[@id="landingpage-cart"]/div/div[2]/button['
                      'contains(., \'ADD TO CART\')]')),
        Click(ByXPath('//a[contains(., \'View Shopping Cart\')]'), True),
        Click(ByXPath('//a[contains(., \'Secure Checkout\')]')),
        Click(ByXPath('//a[contains(., \'CONTINUE AS A GUEST\')]'), True),
        TriggerAutofill(ByXPath('//*[@id="SFirstName"]'), 'NAME_FIRST'),
        ValidateFields([
            (ByXPath('//*[@id="SFirstName"]'), 'NAME_FIRST'),
            (ByXPath('//*[@id="SLastName"]'), 'NAME_LAST'),
            (ByXPath('//*[@id="SAddress1"]'), 'ADDRESS_HOME_LINE1'),
            (ByXPath('//*[@id="SAddress2"]'), 'ADDRESS_HOME_LINE2'),
            (ByXPath('//*[@id="SCity"]'), 'ADDRESS_HOME_CITY'),
            (ByXPath('//*[@id="SState_Option_USA"]'), 'ADDRESS_HOME_STATE',
             'CA'),
            (ByXPath('//*[@id="SZip"]'), 'ADDRESS_HOME_ZIP', '94035-____'),
            (ByXPath('//*[@id="ShippingPhone"]'), 'PHONE_HOME_CITY_AND_NUMBER',
             '(650) 670-1234 x__________'),
            (ByXPath('//*[@id="email"]'), 'EMAIL_ADDRESS'),
        ]),
    ]


class TestGamestopCom(AutofillTask):
  def _create_script(self):
    self.script = [
        Open('http://www.gamestop.com/ps4/consoles/playstation-4-500gb-system-'
             'white/118544'),
        # First redirects you to the canadian site if run internationally
        Open('http://www.gamestop.com/ps4/consoles/playstation-4-500gb-system-'
             'white/118544'),
        Click(ByXPath('//*[@id="mainContentPlaceHolder_dynamicContent_ctl00_'
                      'RepeaterRightColumnLayouts_RightColumnPlaceHolder_0_'
                      'ctl00_0_ctl00_0_StandardPlaceHolder_2_ctl00_2_'
                      'rptBuyBoxes_2_lnkAddToCart_0"]')),
        Click(ByXPath('//*[@id="checkoutButton"]')),
        Click(ByXPath('//*[@id="cartcheckoutbtn"]')),
        Click(ByXPath('//*[@id="buyasguest"]')),
        TriggerAutofill(ByXPath('//*[@id="ShipTo_FirstName"]'), 'NAME_FIRST'),
        ValidateFields([
            (ByXPath('//*[@id="ShipTo_CountryCode"]'), 'ADDRESS_HOME_COUNTRY',
             'US'),
            (ByXPath('//*[@id="ShipTo_FirstName"]'), 'NAME_FIRST'),
            (ByXPath('//*[@id="ShipTo_LastName"]'), 'NAME_LAST'),
            (ByXPath('//*[@id="ShipTo_Line1"]'), 'ADDRESS_HOME_LINE1'),
            (ByXPath('//*[@id="ShipTo_Line2"]'), 'ADDRESS_HOME_LINE2'),
            (ByXPath('//*[@id="ShipTo_City"]'), 'ADDRESS_HOME_CITY'),
            (ByXPath('//*[@id="USStates"]'), 'ADDRESS_HOME_STATE', 'CA'),
            (ByXPath('//*[@id="ShipTo_PostalCode"]'), 'ADDRESS_HOME_ZIP'),
            (ByXPath('//*[@id="ShipTo_PhoneNumber"]'),
             'PHONE_HOME_CITY_AND_NUMBER'),
            (ByXPath('//*[@id="ShipTo_EmailAddress"]'), 'EMAIL_ADDRESS'),
        ])
    ]


class TestLowesCom(AutofillTask):
  def _create_script(self):
    self.script = [
        Open('http://www.lowes.com/pd/Weber-Original-Kettle-22-in-Black-'
             'Porcelain-Enameled-Kettle-Charcoal-Grill/3055249'),
        Type(ByXPath('//*[@id="zipcode-input"]'),
             self.profile_data('ADDRESS_HOME_ZIP'), True),
        Click(ByXPath('//button[contains(., \'Ok\')]'), True),
        Click(ByXPath('//*[@id="storeList"]/li[1]/div/div[2]/button['
                      'contains(., \'Shop this store\')]'), True),
        Wait(3),
        Click(ByXPath('//button[contains(., \'Add To Cart\')]')),
        Click(ByXPath('//a[contains(., \'View Cart\')]')),
        Click(ByXPath('//*[@id="LDshipModeId_1"]')),
        Click(ByXPath('//*[@id="ShopCartForm"]/div[2]/div[2]/a[contains(.,'
                      ' \'Start Secure Checkout\')]')),
        Click(ByXPath('//*[@id="login-container"]/div[2]/div/div/div/a['
                      'contains(., \'Check Out\')]')),
        TriggerAutofill(ByXPath('//*[@id="fname"]'), 'NAME_FIRST'),
        ValidateFields([
            (ByXPath('//*[@id="fname"]'), 'NAME_FIRST'),
            (ByXPath('//*[@id="lname"]'), 'NAME_LAST'),
            (ByXPath('//*[@id="company-name"]'), 'COMPANY_NAME'),
            (ByXPath('//*[@id="address-1"]'), 'ADDRESS_HOME_LINE1'),
            (ByXPath('//*[@id="address-2"]'), 'ADDRESS_HOME_LINE2'),
            (ByXPath('//*[@id="city"]'), 'ADDRESS_HOME_CITY'),
            (ByXPath('//*[@id="state"]'), 'ADDRESS_HOME_STATE', 'CA'),
            (ByXPath('//*[@id="zip"]'), 'ADDRESS_HOME_ZIP'),
        ]),
        Click(ByXPath('//*[@id="revpay_com_order"]')),
        Wait(1), # Buttons with the same xPath exists on both pages
        Click(ByXPath('//*[@id="revpay_com_order"]')),
        Wait(1), # Buttons with the same xPath exists on both pages
        TriggerAutofill(ByXPath('//*[@name="cardNumber"]'),
                        'CREDIT_CARD_NUMBER'),
        Type(ByXPath('//*[@id="s-code"]'),
             self.profile_data('CREDIT_CARD_VERIFICATION_CODE')),
        Type(ByXPath('//*[@id="billing-address-phone1"]'),
             self.profile_data('PHONE_HOME_CITY_AND_NUMBER')),
        Type(ByXPath('//*[@id="billingEmailAddress"]'),
             self.profile_data('EMAIL_ADDRESS')),
        ValidateFields([
            (ByXPath('//*[@id="checkout-card-type"]'), 'CREDIT_CARD_TYPE'),
            (ByXPath('//*[@name="cardNumber"]'), 'CREDIT_CARD_NUMBER'),
            (ByXPath('//*[@id="s-code"]'), 'CREDIT_CARD_VERIFICATION_CODE'),
            (ByXPath('//*[@id="expiration-month"]'), 'CREDIT_CARD_EXP_MONTH'),
            (ByXPath('//*[@id="expiration-year"]'),
             'CREDIT_CARD_EXP_4_DIGIT_YEAR'),
            (ByXPath('//*[@id="billing-address-phone1"]'),
             'PHONE_HOME_CITY_AND_NUMBER', '(650) 670-1234'),
            (ByXPath('//*[@id="billingEmailAddress"]'), 'EMAIL_ADDRESS'),
        ]),
        Click(ByXPath('//*[@id="revpay_com_order"]'))
    ]
