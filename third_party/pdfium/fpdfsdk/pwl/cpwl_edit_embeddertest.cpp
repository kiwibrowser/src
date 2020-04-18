// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fpdfsdk/cpdfsdk_annot.h"
#include "fpdfsdk/cpdfsdk_annotiterator.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/formfiller/cffl_formfiller.h"
#include "fpdfsdk/formfiller/cffl_interactiveformfiller.h"
#include "fpdfsdk/pwl/cpwl_wnd.h"
#include "testing/embedder_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class CPWLEditEmbeddertest : public EmbedderTest {
 protected:
  void SetUp() override {
    EmbedderTest::SetUp();
    CreateAndInitializeFormPDF();
  }

  void TearDown() override {
    UnloadPage(GetPage());
    EmbedderTest::TearDown();
  }

  void CreateAndInitializeFormPDF() {
    EXPECT_TRUE(OpenDocument("text_form_multiple.pdf"));
    m_page = LoadPage(0);
    ASSERT_TRUE(m_page);

    m_pFormFillEnv = static_cast<CPDFSDK_FormFillEnvironment*>(form_handle());
    CPDFSDK_AnnotIterator iter(m_pFormFillEnv->GetPageView(0),
                               CPDF_Annot::Subtype::WIDGET);
    // Normal text field.
    m_pAnnot = iter.GetFirstAnnot();
    ASSERT_TRUE(m_pAnnot);
    ASSERT_EQ(CPDF_Annot::Subtype::WIDGET, m_pAnnot->GetAnnotSubtype());

    // Read-only text field.
    CPDFSDK_Annot* pAnnotReadOnly = iter.GetNextAnnot(m_pAnnot);

    // Pre-filled text field with char limit of 10.
    m_pAnnotCharLimit = iter.GetNextAnnot(pAnnotReadOnly);
    ASSERT_TRUE(m_pAnnotCharLimit);
    ASSERT_EQ(CPDF_Annot::Subtype::WIDGET,
              m_pAnnotCharLimit->GetAnnotSubtype());
    CPDFSDK_Annot* pLastAnnot = iter.GetLastAnnot();
    ASSERT_EQ(m_pAnnotCharLimit, pLastAnnot);
  }

  void FormFillerAndWindowSetup(CPDFSDK_Annot* pAnnotTextField) {
    CFFL_InteractiveFormFiller* pInteractiveFormFiller =
        m_pFormFillEnv->GetInteractiveFormFiller();
    {
      CPDFSDK_Annot::ObservedPtr pObserved(pAnnotTextField);
      EXPECT_TRUE(pInteractiveFormFiller->OnSetFocus(&pObserved, 0));
    }

    m_pFormFiller =
        pInteractiveFormFiller->GetFormFiller(pAnnotTextField, false);
    ASSERT_TRUE(m_pFormFiller);

    CPWL_Wnd* pWindow =
        m_pFormFiller->GetPDFWindow(m_pFormFillEnv->GetPageView(0), false);
    ASSERT_TRUE(pWindow);
    ASSERT_EQ(PWL_CLASSNAME_EDIT, pWindow->GetClassName());

    m_pEdit = static_cast<CPWL_Edit*>(pWindow);
  }

  void TypeTextIntoTextField(int num_chars) {
    // Type text starting with 'A' to as many chars as specified by |num_chars|.
    for (int i = 0; i < num_chars; ++i) {
      EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), i + 'A', 0));
    }
  }

  FPDF_PAGE GetPage() { return m_page; }
  CPWL_Edit* GetCPWLEdit() { return m_pEdit; }
  CFFL_FormFiller* GetCFFLFormFiller() { return m_pFormFiller; }
  CPDFSDK_Annot* GetCPDFSDKAnnot() { return m_pAnnot; }
  CPDFSDK_Annot* GetCPDFSDKAnnotCharLimit() { return m_pAnnotCharLimit; }

 private:
  FPDF_PAGE m_page;
  CPWL_Edit* m_pEdit;
  CFFL_FormFiller* m_pFormFiller;
  CPDFSDK_Annot* m_pAnnot;
  CPDFSDK_Annot* m_pAnnotCharLimit;
  CPDFSDK_FormFillEnvironment* m_pFormFillEnv;
};

TEST_F(CPWLEditEmbeddertest, TypeText) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  EXPECT_TRUE(GetCPWLEdit()->GetText().IsEmpty());
  EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), 'a', 0));
  EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), 'b', 0));
  EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), 'c', 0));

  EXPECT_STREQ(L"abc", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, GetSelectedTextEmptyAndBasic) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  // Attempt to set selection before text has been typed to test that
  // selection is identified as empty.
  //
  // Select from character index [0, 3) within form text field.
  GetCPWLEdit()->SetSelection(0, 3);
  EXPECT_TRUE(GetCPWLEdit()->GetSelectedText().IsEmpty());

  EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), 'a', 0));
  EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), 'b', 0));
  EXPECT_TRUE(GetCFFLFormFiller()->OnChar(GetCPDFSDKAnnot(), 'c', 0));
  GetCPWLEdit()->SetSelection(0, 2);

  EXPECT_STREQ(L"ab", GetCPWLEdit()->GetSelectedText().c_str());
}

TEST_F(CPWLEditEmbeddertest, GetSelectedTextFragments) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(50);

  GetCPWLEdit()->SetSelection(0, 0);
  EXPECT_TRUE(GetCPWLEdit()->GetSelectedText().IsEmpty());

  GetCPWLEdit()->SetSelection(0, 1);
  EXPECT_STREQ(L"A", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->SetSelection(0, -1);
  EXPECT_STREQ(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqr",
               GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->SetSelection(-8, -1);
  EXPECT_TRUE(GetCPWLEdit()->GetSelectedText().IsEmpty());

  GetCPWLEdit()->SetSelection(23, 12);
  EXPECT_STREQ(L"MNOPQRSTUVW", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->SetSelection(12, 23);
  EXPECT_STREQ(L"MNOPQRSTUVW", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->SetSelection(49, 50);
  EXPECT_STREQ(L"r", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->SetSelection(49, 55);
  EXPECT_STREQ(L"r", GetCPWLEdit()->GetSelectedText().c_str());
}

TEST_F(CPWLEditEmbeddertest, DeleteEntireTextSelection) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(50);

  GetCPWLEdit()->SetSelection(0, -1);
  EXPECT_STREQ(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqr",
               GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->ReplaceSelection(L"");
  EXPECT_TRUE(GetCPWLEdit()->GetText().IsEmpty());
}

TEST_F(CPWLEditEmbeddertest, DeleteTextSelectionMiddle) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(50);

  GetCPWLEdit()->SetSelection(12, 23);
  EXPECT_STREQ(L"MNOPQRSTUVW", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->ReplaceSelection(L"");
  EXPECT_STREQ(L"ABCDEFGHIJKLXYZ[\\]^_`abcdefghijklmnopqr",
               GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, DeleteTextSelectionLeft) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(50);

  GetCPWLEdit()->SetSelection(0, 5);
  EXPECT_STREQ(L"ABCDE", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->ReplaceSelection(L"");
  EXPECT_STREQ(L"FGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqr",
               GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, DeleteTextSelectionRight) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(50);

  GetCPWLEdit()->SetSelection(45, 50);
  EXPECT_STREQ(L"nopqr", GetCPWLEdit()->GetSelectedText().c_str());

  GetCPWLEdit()->ReplaceSelection(L"");
  EXPECT_STREQ(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm",
               GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, DeleteEmptyTextSelection) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(50);

  GetCPWLEdit()->ReplaceSelection(L"");
  EXPECT_STREQ(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqr",
               GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInEmptyTextField) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"Hello", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInPopulatedTextFieldLeft) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  // Move cursor to beginning of text field.
  EXPECT_TRUE(
      GetCFFLFormFiller()->OnKeyDown(GetCPDFSDKAnnot(), FWL_VKEY_Home, 0));

  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"HelloABCDEFGHIJ", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInPopulatedTextFieldMiddle) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  // Move cursor to middle of text field.
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(
        GetCFFLFormFiller()->OnKeyDown(GetCPDFSDKAnnot(), FWL_VKEY_Left, 0));
  }

  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"ABCDEHelloFGHIJ", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInPopulatedTextFieldRight) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"ABCDEFGHIJHello", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedTextFieldWhole) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  GetCPWLEdit()->SetSelection(0, -1);
  EXPECT_STREQ(L"ABCDEFGHIJ", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"Hello", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedTextFieldLeft) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  GetCPWLEdit()->SetSelection(0, 5);
  EXPECT_STREQ(L"ABCDE", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"HelloFGHIJ", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedTextFieldMiddle) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  GetCPWLEdit()->SetSelection(2, 7);
  EXPECT_STREQ(L"CDEFG", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"ABHelloHIJ", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedTextFieldRight) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  TypeTextIntoTextField(10);

  GetCPWLEdit()->SetSelection(5, 10);
  EXPECT_STREQ(L"FGHIJ", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hello");
  EXPECT_STREQ(L"ABCDEHello", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInEmptyCharLimitTextFieldOverflow) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->SetSelection(0, -1);
  EXPECT_STREQ(L"Elephant", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"");

  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"Hippopotam", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInEmptyCharLimitTextFieldFit) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->SetSelection(0, -1);
  EXPECT_STREQ(L"Elephant", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"");

  GetCPWLEdit()->ReplaceSelection(L"Zebra");
  EXPECT_STREQ(L"Zebra", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInPopulatedCharLimitTextFieldLeft) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"HiElephant", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInPopulatedCharLimitTextFieldMiddle) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  // Move cursor to middle of text field.
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(GetCFFLFormFiller()->OnKeyDown(GetCPDFSDKAnnotCharLimit(),
                                               FWL_VKEY_Right, 0));
  }

  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"ElephHiant", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, InsertTextInPopulatedCharLimitTextFieldRight) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  // Move cursor to end of text field.
  EXPECT_TRUE(GetCFFLFormFiller()->OnKeyDown(GetCPDFSDKAnnotCharLimit(),
                                             FWL_VKEY_End, 0));

  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"ElephantHi", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedCharLimitTextFieldWhole) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->SetSelection(0, -1);
  EXPECT_STREQ(L"Elephant", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"Hippopotam", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedCharLimitTextFieldLeft) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->SetSelection(0, 4);
  EXPECT_STREQ(L"Elep", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"Hippophant", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedCharLimitTextFieldMiddle) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->SetSelection(2, 6);
  EXPECT_STREQ(L"epha", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"ElHippopnt", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest,
       InsertTextAndReplaceSelectionInPopulatedCharLimitTextFieldRight) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnotCharLimit());
  GetCPWLEdit()->SetSelection(4, 8);
  EXPECT_STREQ(L"hant", GetCPWLEdit()->GetSelectedText().c_str());
  GetCPWLEdit()->ReplaceSelection(L"Hippopotamus");
  EXPECT_STREQ(L"ElepHippop", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithEndCarriageFeed) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\r");
  EXPECT_STREQ(L"Foo", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithEndNewline) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\n");
  EXPECT_STREQ(L"Foo", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithEndCarriageFeedAndNewLine) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\r\n");
  EXPECT_STREQ(L"Foo", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithEndNewLineAndCarriageFeed) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\n\r");
  EXPECT_STREQ(L"Foo", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithBodyCarriageFeed) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\rBar");
  EXPECT_STREQ(L"FooBar", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithBodyNewline) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\nBar");
  EXPECT_STREQ(L"FooBar", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithBodyCarriageFeedAndNewLine) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\r\nBar");
  EXPECT_STREQ(L"FooBar", GetCPWLEdit()->GetText().c_str());
}

TEST_F(CPWLEditEmbeddertest, SetTextWithBodyNewLineAndCarriageFeed) {
  FormFillerAndWindowSetup(GetCPDFSDKAnnot());
  GetCPWLEdit()->SetText(L"Foo\n\rBar");
  EXPECT_STREQ(L"FooBar", GetCPWLEdit()->GetText().c_str());
}
