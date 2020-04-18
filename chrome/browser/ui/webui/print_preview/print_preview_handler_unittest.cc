// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/print_preview_handler.h"

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/flat_set.h"
#include "base/json/json_writer.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/icu_test_util.h"
#include "base/values.h"
#include "chrome/browser/printing/print_test_utils.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/browser/ui/webui/print_preview/printer_handler.h"
#include "chrome/test/base/testing_profile.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_web_ui.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace printing {

namespace {

const char kDummyInitiatorName[] = "TestInitiator";
const char kTestData[] = "abc";

// Array of all printing::PrinterType values.
const printing::PrinterType kAllTypes[] = {
    printing::kPrivetPrinter, printing::kExtensionPrinter,
    printing::kPdfPrinter, printing::kLocalPrinter};

struct PrinterInfo {
  std::string id;
  bool is_default;
  base::Value basic_info = base::Value(base::Value::Type::DICTIONARY);
  base::Value capabilities = base::Value(base::Value::Type::DICTIONARY);
};

PrinterInfo GetSimplePrinterInfo(const std::string& name, bool is_default) {
  PrinterInfo simple_printer;
  simple_printer.id = name;
  simple_printer.is_default = is_default;
  simple_printer.basic_info.SetKey("printer_name",
                                   base::Value(simple_printer.id));
  simple_printer.basic_info.SetKey("printer_description",
                                   base::Value("Printer for test"));
  simple_printer.basic_info.SetKey("printer_status", base::Value(1));
  base::Value cdd(base::Value::Type::DICTIONARY);
  base::Value capabilities(base::Value::Type::DICTIONARY);
  simple_printer.capabilities.SetKey("printer",
                                     simple_printer.basic_info.Clone());
  simple_printer.capabilities.SetKey("capabilities", cdd.Clone());
  return simple_printer;
}

PrinterInfo GetEmptyPrinterInfo() {
  PrinterInfo empty_printer;
  empty_printer.id = "EmptyPrinter";
  empty_printer.is_default = false;
  empty_printer.basic_info.SetKey("printer_name",
                                  base::Value(empty_printer.id));
  empty_printer.basic_info.SetKey("printer_description",
                                  base::Value("Printer with no capabilities"));
  empty_printer.basic_info.SetKey("printer_status", base::Value(0));
  empty_printer.capabilities.SetKey("printer",
                                    empty_printer.basic_info.Clone());
  return empty_printer;
}

base::Value GetPrintPreviewTicket(bool is_pdf) {
  base::Value print_ticket = GetPrintTicket(kLocalPrinter, false);

  // Make some modifications to match a preview print ticket.
  print_ticket.SetKey(kSettingPageRange, base::Value());
  print_ticket.SetKey(kIsFirstRequest, base::Value(true));
  print_ticket.SetKey(kPreviewRequestID, base::Value(0));
  print_ticket.SetKey(kSettingPreviewModifiable, base::Value(is_pdf));
  print_ticket.RemoveKey(kSettingPageWidth);
  print_ticket.RemoveKey(kSettingPageHeight);
  print_ticket.RemoveKey(kSettingShowSystemDialog);

  return print_ticket;
}

std::unique_ptr<base::ListValue> ConstructPreviewArgs(
    base::StringPiece callback_id,
    const base::Value& print_ticket) {
  base::Value args(base::Value::Type::LIST);
  args.GetList().emplace_back(callback_id);
  std::string json;
  base::JSONWriter::Write(print_ticket, &json);
  args.GetList().emplace_back(json);
  return base::ListValue::From(base::Value::ToUniquePtrValue(std::move(args)));
}

class TestPrinterHandler : public PrinterHandler {
 public:
  explicit TestPrinterHandler(const std::vector<PrinterInfo>& printers) {
    SetPrinters(printers);
  }

  ~TestPrinterHandler() override {}

  void Reset() override {}

  void GetDefaultPrinter(DefaultPrinterCallback cb) override {
    std::move(cb).Run(default_printer_);
  }

  void StartGetPrinters(const AddedPrintersCallback& added_printers_callback,
                        GetPrintersDoneCallback done_callback) override {
    if (!printers_.empty())
      added_printers_callback.Run(printers_);
    std::move(done_callback).Run();
  }

  void StartGetCapability(const std::string& destination_id,
                          GetCapabilityCallback callback) override {
    std::move(callback).Run(
        base::DictionaryValue::From(std::make_unique<base::Value>(
            printer_capabilities_[destination_id]->Clone())));
  }

  void StartGrantPrinterAccess(const std::string& printer_id,
                               GetPrinterInfoCallback callback) override {}

  void StartPrint(const std::string& destination_id,
                  const std::string& capability,
                  const base::string16& job_title,
                  const std::string& ticket_json,
                  const gfx::Size& page_size,
                  const scoped_refptr<base::RefCountedMemory>& print_data,
                  PrintCallback callback) override {
    std::move(callback).Run(base::Value());
  }

  void SetPrinters(const std::vector<PrinterInfo>& printers) {
    base::Value::ListStorage printer_list;
    for (const auto& printer : printers) {
      if (printer.is_default)
        default_printer_ = printer.id;
      printer_list.push_back(printer.basic_info.Clone());
      printer_capabilities_[printer.id] = base::DictionaryValue::From(
          std::make_unique<base::Value>(printer.capabilities.Clone()));
    }
    printers_ = base::ListValue(printer_list);
  }

 private:
  std::string default_printer_;
  base::ListValue printers_;
  std::map<std::string, std::unique_ptr<base::DictionaryValue>>
      printer_capabilities_;

  DISALLOW_COPY_AND_ASSIGN(TestPrinterHandler);
};

class FakePrintPreviewUI : public PrintPreviewUI {
 public:
  FakePrintPreviewUI(content::WebUI* web_ui,
                     std::unique_ptr<PrintPreviewHandler> handler)
      : PrintPreviewUI(web_ui, std::move(handler)) {}

  ~FakePrintPreviewUI() override {}

  void GetPrintPreviewDataForIndex(
      int index,
      scoped_refptr<base::RefCountedMemory>* data) const override {
    *data = base::MakeRefCounted<base::RefCountedStaticMemory>(
        reinterpret_cast<const unsigned char*>(kTestData),
        sizeof(kTestData) - 1);
  }

  void OnPrintPreviewRequest(int request_id) override {}
  void OnCancelPendingPreviewRequest() override {}
  void OnHidePreviewDialog() override {}
  void OnClosePrintPreviewDialog() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FakePrintPreviewUI);
};

class TestPrintPreviewHandler : public PrintPreviewHandler {
 public:
  TestPrintPreviewHandler(std::unique_ptr<PrinterHandler> printer_handler,
                          content::WebContents* initiator)
      : bad_messages_(0),
        test_printer_handler_(std::move(printer_handler)),
        initiator_(initiator) {}

  PrinterHandler* GetPrinterHandler(PrinterType printer_type) override {
    called_for_type_.insert(printer_type);
    return test_printer_handler_.get();
  }

  void RegisterForGaiaCookieChanges() override {}
  void UnregisterForGaiaCookieChanges() override {}

  void BadMessageReceived() override { bad_messages_++; }

  content::WebContents* GetInitiator() const override { return initiator_; }

  bool CalledOnlyForType(PrinterType printer_type) {
    return (called_for_type_.size() == 1 &&
            *called_for_type_.begin() == printer_type);
  }

  bool NotCalled() { return called_for_type_.empty(); }

  void reset_calls() { called_for_type_.clear(); }

  int bad_messages() { return bad_messages_; }

 private:
  int bad_messages_;
  base::flat_set<PrinterType> called_for_type_;
  std::unique_ptr<PrinterHandler> test_printer_handler_;
  content::WebContents* const initiator_;

  DISALLOW_COPY_AND_ASSIGN(TestPrintPreviewHandler);
};

}  // namespace

}  // namespace printing

class PrintPreviewHandlerTest : public testing::Test {
 public:
  PrintPreviewHandlerTest() {
    TestingProfile::Builder builder;
    profile_ = builder.Build();
    initiator_web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));
    content::WebContents* initiator = initiator_web_contents_.get();
    preview_web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));
    printing::PrintViewManager::CreateForWebContents(initiator);
    printing::PrintViewManager::FromWebContents(initiator)->PrintPreviewNow(
        initiator->GetMainFrame(), false);
    web_ui_ = std::make_unique<content::TestWebUI>();
    web_ui_->set_web_contents(preview_web_contents_.get());

    printers_.push_back(
        printing::GetSimplePrinterInfo(printing::kDummyPrinterName, true));
    auto printer_handler =
        std::make_unique<printing::TestPrinterHandler>(printers_);
    printer_handler_ = printer_handler.get();

    auto preview_handler = std::make_unique<printing::TestPrintPreviewHandler>(
        std::move(printer_handler), initiator);
    preview_handler->set_web_ui(web_ui());
    handler_ = preview_handler.get();

    auto preview_ui = std::make_unique<printing::FakePrintPreviewUI>(
        web_ui(), std::move(preview_handler));
    preview_ui->SetInitiatorTitle(
        base::ASCIIToUTF16(printing::kDummyInitiatorName));
    web_ui()->SetController(preview_ui.release());
  }

  ~PrintPreviewHandlerTest() override {
    printing::PrintViewManager::FromWebContents(initiator_web_contents_.get())
        ->PrintPreviewDone();
  }

  void Initialize() {
    // Set locale since the delimeters we check in VerifyInitialSettings()
    // depend on it.
    base::test::ScopedRestoreICUDefaultLocale scoped_locale("en");

    // Sending this message will enable javascript, so it must always be called
    // before any other messages are sent.
    base::Value args(base::Value::Type::LIST);
    args.GetList().emplace_back("test-callback-id-0");
    std::unique_ptr<base::ListValue> list_args =
        base::ListValue::From(base::Value::ToUniquePtrValue(std::move(args)));
    handler()->HandleGetInitialSettings(list_args.get());

    // In response to get initial settings, the initial settings are sent back
    // and a use-cloud-print event is dispatched.
    ASSERT_EQ(2u, web_ui()->call_data().size());
  }

  void AssertWebUIEventFired(const content::TestWebUI::CallData& data,
                             const std::string& event_id) {
    EXPECT_EQ("cr.webUIListenerCallback", data.function_name());
    std::string event_fired;
    ASSERT_TRUE(data.arg1()->GetAsString(&event_fired));
    EXPECT_EQ(event_id, event_fired);
  }

  void CheckWebUIResponse(const content::TestWebUI::CallData& data,
                          const std::string& callback_id_in,
                          bool expect_success) {
    EXPECT_EQ("cr.webUIResponse", data.function_name());
    std::string callback_id;
    ASSERT_TRUE(data.arg1()->GetAsString(&callback_id));
    EXPECT_EQ(callback_id_in, callback_id);
    bool success = false;
    ASSERT_TRUE(data.arg2()->GetAsBoolean(&success));
    EXPECT_EQ(expect_success, success);
  }

  // Validates the initial settings structure in the response matches the
  // print_preview.NativeInitialSettings type in
  // chrome/browser/resources/print_preview/native_layer.js. Checks that
  // |default_printer_name| is the printer name returned and that
  // |initiator_title| is the initiator title returned and validates that
  // delimeters are correct for "en" locale (set in Initialize()). Assumes
  // "test-callback-id-0" was used as the callback id.
  void ValidateInitialSettings(const content::TestWebUI::CallData& data,
                               const std::string& default_printer_name,
                               const std::string& initiator_title) {
    CheckWebUIResponse(data, "test-callback-id-0", true);
    const base::Value* settings = data.arg3();
    ASSERT_TRUE(settings->FindKeyOfType("isInKioskAutoPrintMode",
                                        base::Value::Type::BOOLEAN));
    ASSERT_TRUE(settings->FindKeyOfType("isInAppKioskMode",
                                        base::Value::Type::BOOLEAN));

    const base::Value* thousands_delimeter = settings->FindKeyOfType(
        "thousandsDelimeter", base::Value::Type::STRING);
    ASSERT_TRUE(thousands_delimeter);
    EXPECT_EQ(",", thousands_delimeter->GetString());
    const base::Value* decimal_delimeter =
        settings->FindKeyOfType("decimalDelimeter", base::Value::Type::STRING);
    ASSERT_TRUE(decimal_delimeter);
    EXPECT_EQ(".", decimal_delimeter->GetString());

    ASSERT_TRUE(
        settings->FindKeyOfType("unitType", base::Value::Type::INTEGER));
    ASSERT_TRUE(settings->FindKeyOfType("previewModifiable",
                                        base::Value::Type::BOOLEAN));
    const base::Value* title =
        settings->FindKeyOfType("documentTitle", base::Value::Type::STRING);
    ASSERT_TRUE(title);
    EXPECT_EQ(initiator_title, title->GetString());
    ASSERT_TRUE(settings->FindKeyOfType("documentHasSelection",
                                        base::Value::Type::BOOLEAN));
    ASSERT_TRUE(settings->FindKeyOfType("shouldPrintSelectionOnly",
                                        base::Value::Type::BOOLEAN));
    const base::Value* printer =
        settings->FindKeyOfType("printerName", base::Value::Type::STRING);
    ASSERT_TRUE(printer);
    EXPECT_EQ(default_printer_name, printer->GetString());
  }

  IPC::TestSink& initiator_sink() {
    content::RenderFrameHost* rfh = initiator_web_contents_->GetMainFrame();
    auto* rph = static_cast<content::MockRenderProcessHost*>(rfh->GetProcess());
    return rph->sink();
  }

  IPC::TestSink& preview_sink() {
    content::RenderFrameHost* rfh = preview_web_contents_->GetMainFrame();
    auto* rph = static_cast<content::MockRenderProcessHost*>(rfh->GetProcess());
    return rph->sink();
  }

  base::DictionaryValue VerifyPreviewMessage() {
    // Verify that the preview was requested from the renderer
    EXPECT_TRUE(
        initiator_sink().GetUniqueMessageMatching(PrintMsg_PrintPreview::ID));
    const IPC::Message* msg =
        initiator_sink().GetFirstMessageMatching(PrintMsg_PrintPreview::ID);
    EXPECT_TRUE(msg);
    PrintMsg_PrintPreview::Param param;
    PrintMsg_PrintPreview::Read(msg, &param);
    return std::move(std::get<0>(param));
  }

  const Profile* profile() { return profile_.get(); }
  content::TestWebUI* web_ui() { return web_ui_.get(); }
  printing::TestPrintPreviewHandler* handler() { return handler_; }
  printing::TestPrinterHandler* printer_handler() { return printer_handler_; }
  std::vector<printing::PrinterInfo>& printers() { return printers_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<content::TestWebUI> web_ui_;
  content::RenderViewHostTestEnabler rvh_test_enabler_;
  std::unique_ptr<content::WebContents> preview_web_contents_;
  std::unique_ptr<content::WebContents> initiator_web_contents_;
  std::vector<printing::PrinterInfo> printers_;
  printing::TestPrinterHandler* printer_handler_;
  printing::TestPrintPreviewHandler* handler_;

  DISALLOW_COPY_AND_ASSIGN(PrintPreviewHandlerTest);
};

TEST_F(PrintPreviewHandlerTest, InitialSettings) {
  Initialize();

  // Verify initial settings were sent.
  ValidateInitialSettings(*web_ui()->call_data().back(),
                          printing::kDummyPrinterName,
                          printing::kDummyInitiatorName);

  // Check that the use-cloud-print event got sent
  AssertWebUIEventFired(*web_ui()->call_data().front(), "use-cloud-print");
}

TEST_F(PrintPreviewHandlerTest, GetPrinters) {
  Initialize();

  // Check all three printer types that implement
  // PrinterHandler::StartGetPrinters().
  const printing::PrinterType types[] = {printing::kPrivetPrinter,
                                         printing::kExtensionPrinter,
                                         printing::kLocalPrinter};
  for (size_t i = 0; i < arraysize(types); i++) {
    printing::PrinterType type = types[i];
    handler()->reset_calls();
    base::Value args(base::Value::Type::LIST);
    std::string callback_id_in =
        "test-callback-id-" + base::UintToString(i + 1);
    args.GetList().emplace_back(callback_id_in);
    args.GetList().emplace_back(type);
    std::unique_ptr<base::ListValue> list_args =
        base::ListValue::From(base::Value::ToUniquePtrValue(std::move(args)));
    handler()->HandleGetPrinters(list_args.get());
    EXPECT_TRUE(handler()->CalledOnlyForType(type));

    // Start with 2 calls from initial settings, then add 2 more for each loop
    // iteration (one for printers-added, and one for the response).
    ASSERT_EQ(2u + 2 * (i + 1), web_ui()->call_data().size());

    // Validate printers-added
    const content::TestWebUI::CallData& add_data =
        *web_ui()->call_data()[web_ui()->call_data().size() - 2];
    AssertWebUIEventFired(add_data, "printers-added");
    int type_out;
    ASSERT_TRUE(add_data.arg2()->GetAsInteger(&type_out));
    EXPECT_EQ(type, type_out);
    ASSERT_TRUE(add_data.arg3());
    const base::Value::ListStorage& printer_list = add_data.arg3()->GetList();
    ASSERT_EQ(printer_list.size(), 1u);
    EXPECT_TRUE(printer_list[0].FindKeyOfType("printer_name",
                                              base::Value::Type::STRING));

    // Verify getPrinters promise was resolved successfully.
    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    CheckWebUIResponse(data, callback_id_in, true);
  }
}

TEST_F(PrintPreviewHandlerTest, GetPrinterCapabilities) {
  // Add an empty printer to the handler.
  printers().push_back(printing::GetEmptyPrinterInfo());
  printer_handler()->SetPrinters(printers());

  // Initial settings first to enable javascript.
  Initialize();

  // Check all four printer types that implement
  // PrinterHandler::StartGetCapability().
  for (size_t i = 0; i < arraysize(printing::kAllTypes); i++) {
    printing::PrinterType type = printing::kAllTypes[i];
    handler()->reset_calls();
    base::Value args(base::Value::Type::LIST);
    std::string callback_id_in =
        "test-callback-id-" + base::UintToString(i + 1);
    args.GetList().emplace_back(callback_id_in);
    args.GetList().emplace_back(printing::kDummyPrinterName);
    args.GetList().emplace_back(type);
    std::unique_ptr<base::ListValue> list_args =
        base::ListValue::From(base::Value::ToUniquePtrValue(std::move(args)));
    handler()->HandleGetPrinterCapabilities(list_args.get());
    EXPECT_TRUE(handler()->CalledOnlyForType(type));

    // Start with 2 calls from initial settings, then add 1 more for each loop
    // iteration.
    ASSERT_EQ(2u + (i + 1), web_ui()->call_data().size());

    // Verify that the printer capabilities promise was resolved correctly.
    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    CheckWebUIResponse(data, callback_id_in, true);
    const base::Value* settings = data.arg3();
    ASSERT_TRUE(settings);
    EXPECT_TRUE(settings->FindKeyOfType(printing::kSettingCapabilities,
                                        base::Value::Type::DICTIONARY));
  }

  // Run through the loop again, this time with a printer that has no
  // capabilities.
  for (size_t i = 0; i < arraysize(printing::kAllTypes); i++) {
    printing::PrinterType type = printing::kAllTypes[i];
    handler()->reset_calls();
    base::Value args(base::Value::Type::LIST);
    std::string callback_id_in =
        "test-callback-id-" +
        base::UintToString(i + arraysize(printing::kAllTypes) + 1);
    args.GetList().emplace_back(callback_id_in);
    args.GetList().emplace_back("EmptyPrinter");
    args.GetList().emplace_back(type);
    std::unique_ptr<base::ListValue> list_args =
        base::ListValue::From(base::Value::ToUniquePtrValue(std::move(args)));
    handler()->HandleGetPrinterCapabilities(list_args.get());
    EXPECT_TRUE(handler()->CalledOnlyForType(type));

    // Start with 2 calls from initial settings plus
    // arraysize(printing::kAllTypes) from first loop, then add 1 more for each
    // loop iteration.
    ASSERT_EQ(2u + arraysize(printing::kAllTypes) + (i + 1),
              web_ui()->call_data().size());

    // Verify printer capabilities promise was rejected.
    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    CheckWebUIResponse(data, callback_id_in, false);
  }
}

TEST_F(PrintPreviewHandlerTest, Print) {
  Initialize();

  // All four printer types can print, as well as cloud printers.
  for (size_t i = 0; i <= arraysize(printing::kAllTypes); i++) {
    // Also check cloud print. Use dummy type value of Privet (will be ignored).
    bool cloud = i == arraysize(printing::kAllTypes);
    printing::PrinterType type =
        cloud ? printing::kPrivetPrinter : printing::kAllTypes[i];
    handler()->reset_calls();
    base::Value args(base::Value::Type::LIST);
    std::string callback_id_in =
        "test-callback-id-" + base::UintToString(i + 1);
    args.GetList().emplace_back(callback_id_in);
    base::Value print_ticket = printing::GetPrintTicket(type, cloud);
    std::string json;
    base::JSONWriter::Write(print_ticket, &json);
    args.GetList().emplace_back(json);
    std::unique_ptr<base::ListValue> list_args =
        base::ListValue::From(base::Value::ToUniquePtrValue(std::move(args)));
    handler()->HandlePrint(list_args.get());

    // Verify correct PrinterHandler was called or that no handler was requested
    // for cloud printers.
    if (cloud) {
      EXPECT_TRUE(handler()->NotCalled());
    } else {
      EXPECT_TRUE(handler()->CalledOnlyForType(type));
    }

    // Verify print promise was resolved successfully.
    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    CheckWebUIResponse(data, callback_id_in, true);

    // For cloud print, should also get the encoded data back as a string.
    if (cloud) {
      std::string print_data;
      ASSERT_TRUE(data.arg3()->GetAsString(&print_data));
      std::string expected_data;
      base::Base64Encode(printing::kTestData, &expected_data);
      EXPECT_EQ(print_data, expected_data);
    }
  }
}

TEST_F(PrintPreviewHandlerTest, GetPreview) {
  Initialize();

  base::Value print_ticket = printing::GetPrintPreviewTicket(false);
  std::unique_ptr<base::ListValue> list_args =
      printing::ConstructPreviewArgs("test-callback-id-1", print_ticket);
  handler()->HandleGetPreview(list_args.get());

  // Verify that the preview was requested from the renderer with the
  // appropriate settings.
  base::DictionaryValue preview_params = VerifyPreviewMessage();
  bool preview_id_found = false;
  for (const auto& it : preview_params.DictItems()) {
    if (it.first == printing::kPreviewUIID) {  // This is added by the handler.
      preview_id_found = true;
      continue;
    }
    base::Value* value_in = print_ticket.FindKey(it.first);
    ASSERT_TRUE(value_in);
    EXPECT_EQ(*value_in, it.second);
  }
  EXPECT_TRUE(preview_id_found);
}

TEST_F(PrintPreviewHandlerTest, SendPreviewUpdates) {
  Initialize();

  const char callback_id_in[] = "test-callback-id-1";
  base::Value print_ticket = printing::GetPrintPreviewTicket(false);
  std::unique_ptr<base::ListValue> list_args =
      printing::ConstructPreviewArgs(callback_id_in, print_ticket);
  handler()->HandleGetPreview(list_args.get());
  base::DictionaryValue preview_params = VerifyPreviewMessage();

  // Read the preview UI ID and request ID
  const base::Value* request_value =
      preview_params.FindKey(printing::kPreviewRequestID);
  ASSERT_TRUE(request_value);
  ASSERT_TRUE(request_value->is_int());
  int preview_request_id = request_value->GetInt();

  const base::Value* ui_value = preview_params.FindKey(printing::kPreviewUIID);
  ASSERT_TRUE(ui_value);
  ASSERT_TRUE(ui_value->is_int());
  int preview_ui_id = ui_value->GetInt();

  // Simulate renderer responses: PageLayoutReady, PageCountReady,
  // PagePreviewReady, and OnPrintPreviewReady will be called in that order.
  base::DictionaryValue layout;
  layout.SetKey(printing::kSettingMarginTop, base::Value(34.0));
  layout.SetKey(printing::kSettingMarginLeft, base::Value(34.0));
  layout.SetKey(printing::kSettingMarginBottom, base::Value(34.0));
  layout.SetKey(printing::kSettingMarginRight, base::Value(34.0));
  layout.SetKey(printing::kSettingContentWidth, base::Value(544.0));
  layout.SetKey(printing::kSettingContentHeight, base::Value(700.0));
  layout.SetKey(printing::kSettingPrintableAreaX, base::Value(17));
  layout.SetKey(printing::kSettingPrintableAreaY, base::Value(17));
  layout.SetKey(printing::kSettingPrintableAreaWidth, base::Value(578));
  layout.SetKey(printing::kSettingPrintableAreaHeight, base::Value(734));
  handler()->SendPageLayoutReady(layout, false);

  // Verify that page-layout-ready webUI event was fired.
  AssertWebUIEventFired(*web_ui()->call_data().back(), "page-layout-ready");

  // 1 page document. Modifiable so send default 100 scaling.
  handler()->SendPageCountReady(1, preview_request_id, 100);
  AssertWebUIEventFired(*web_ui()->call_data().back(), "page-count-ready");

  // Page at index 0 is ready.
  handler()->SendPagePreviewReady(0, preview_ui_id, preview_request_id);
  AssertWebUIEventFired(*web_ui()->call_data().back(), "page-preview-ready");

  // Print preview is ready.
  handler()->OnPrintPreviewReady(preview_ui_id, preview_request_id);
  CheckWebUIResponse(*web_ui()->call_data().back(), callback_id_in, true);

  // Renderer responses have been as expected.
  EXPECT_EQ(handler()->bad_messages(), 0);

  // None of these should work since there has been no new preview request.
  // Check that there are no new web UI messages sent.
  size_t message_count = web_ui()->call_data().size();
  handler()->SendPageLayoutReady(base::DictionaryValue(), false);
  EXPECT_EQ(message_count, web_ui()->call_data().size());
  handler()->SendPageCountReady(1, 0, -1);
  EXPECT_EQ(message_count, web_ui()->call_data().size());
  handler()->OnPrintPreviewReady(0, 0);
  EXPECT_EQ(message_count, web_ui()->call_data().size());

  // Handler should have tried to kill the renderer for each of these.
  EXPECT_EQ(handler()->bad_messages(), 3);
}
