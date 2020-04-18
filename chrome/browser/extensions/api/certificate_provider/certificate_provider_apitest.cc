// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/certificate_provider/certificate_provider_service.h"
#include "chrome/browser/chromeos/certificate_provider/certificate_provider_service_factory.h"
#include "chrome/browser/extensions/api/certificate_provider/certificate_provider_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "crypto/rsa_private_key.h"
#include "extensions/common/extension.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/mem.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

using testing::Return;
using testing::_;

namespace {

void IgnoreResult(const base::Closure& callback, const base::Value* value) {
  callback.Run();
}

void StoreBool(bool* result,
               const base::Closure& callback,
               const base::Value* value) {
  value->GetAsBoolean(result);
  callback.Run();
}

void StoreString(std::string* result,
                 const base::Closure& callback,
                 const base::Value* value) {
  value->GetAsString(result);
  callback.Run();
}

void StoreDigest(std::vector<uint8_t>* digest,
                 const base::Closure& callback,
                 const base::Value* value) {
  ASSERT_TRUE(value->is_blob()) << "Unexpected value in StoreDigest";
  digest->assign(value->GetBlob().begin(), value->GetBlob().end());
  callback.Run();
}

bool RsaSign(const std::vector<uint8_t>& digest,
             crypto::RSAPrivateKey* key,
             std::vector<uint8_t>* signature) {
  RSA* rsa_key = EVP_PKEY_get0_RSA(key->key());
  if (!rsa_key)
    return false;

  unsigned len = 0;
  signature->resize(RSA_size(rsa_key));
  if (!RSA_sign(NID_sha1, digest.data(), digest.size(), signature->data(), &len,
                rsa_key)) {
    signature->clear();
    return false;
  }
  signature->resize(len);
  return true;
}

// Create a string that if evaluated in JavaScript returns a Uint8Array with
// |bytes| as content.
std::string JsUint8Array(const std::vector<uint8_t>& bytes) {
  std::string res = "new Uint8Array([";
  for (const uint8_t byte : bytes) {
    res += base::UintToString(byte);
    res += ", ";
  }
  res += "])";
  return res;
}

// Enters the code in the ShowPinDialog window and pushes the OK event.
void EnterCode(chromeos::CertificateProviderService* service,
               const base::string16& code) {
  chromeos::RequestPinView* view =
      service->pin_dialog_manager()->active_view_for_testing();
  view->textfield_for_testing()->SetText(code);
  view->Accept();
  base::RunLoop().RunUntilIdle();
}

// Enters the valid code for extensions from local example folders, in the
// ShowPinDialog window and waits for the window to close. The extension code
// is expected to send "Success" message after the validation and request to
// stopPinRequest is done.
void EnterCorrectPin(chromeos::CertificateProviderService* service) {
  ExtensionTestMessageListener listener("Success", false);
  EnterCode(service, base::ASCIIToUTF16("1234"));
  ASSERT_TRUE(listener.WaitUntilSatisfied());
}

// Enters an invalid code for extensions from local example folders, in the
// ShowPinDialog window and waits for the window to update with the error. The
// extension code is expected to send "Invalid PIN" message after the validation
// and the new requestPin (with the error) is done.
void EnterWrongPin(chromeos::CertificateProviderService* service) {
  ExtensionTestMessageListener listener("Invalid PIN", false);
  EnterCode(service, base::ASCIIToUTF16("567"));
  ASSERT_TRUE(listener.WaitUntilSatisfied());

  // Check that we have an error message displayed.
  chromeos::RequestPinView* view =
      service->pin_dialog_manager()->active_view_for_testing();
  EXPECT_EQ(SK_ColorRED, view->error_label_for_testing()->enabled_color());
}

class CertificateProviderApiTest : public extensions::ExtensionApiTest {
 public:
  CertificateProviderApiTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);

    extensions::ExtensionApiTest::SetUpInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    extensions::ExtensionApiTest::SetUpOnMainThread();
    // Set up the AutoSelectCertificateForUrls policy to avoid the client
    // certificate selection dialog.
    const std::string autoselect_pattern =
        "{\"pattern\": \"*\", \"filter\": {\"ISSUER\": {\"CN\": \"root\"}}}";

    std::unique_ptr<base::ListValue> autoselect_policy(new base::ListValue);
    autoselect_policy->AppendString(autoselect_pattern);

    policy::PolicyMap policy;
    policy.Set(policy::key::kAutoSelectCertificateForUrls,
               policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
               policy::POLICY_SOURCE_CLOUD, std::move(autoselect_policy),
               nullptr);
    provider_.UpdateChromePolicy(policy);

    content::RunAllPendingInMessageLoop();
  }

 protected:
  policy::MockConfigurationPolicyProvider provider_;
};

class CertificateProviderRequestPinTest : public CertificateProviderApiTest {
 public:
  // Loads certificate_provider extension from |folder| and |file_name|.
  // Returns the CertificateProviderService object from browser context.
  chromeos::CertificateProviderService* LoadRequestPinExtension(
      const std::string& folder,
      const std::string& file_name) {
    const base::FilePath extension_path =
        test_data_dir_.AppendASCII("certificate_provider/" + folder);
    const extensions::Extension* const extension =
        LoadExtension(extension_path);
    chromeos::CertificateProviderService* service =
        chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
            profile());
    service->pin_dialog_manager()->AddSignRequestId(extension->id(), 123);
    ui_test_utils::NavigateToURL(browser(),
                                 extension->GetResourceURL(file_name));
    return service;
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(CertificateProviderApiTest, Basic) {
  // Start an HTTPS test server that requests a client certificate.
  net::SpawnedTestServer::SSLOptions ssl_options;
  ssl_options.request_client_certificate = true;
  net::SpawnedTestServer https_server(net::SpawnedTestServer::TYPE_HTTPS,
                                      ssl_options, base::FilePath());
  ASSERT_TRUE(https_server.Start());

  extensions::ResultCatcher catcher;

  const base::FilePath extension_path =
      test_data_dir_.AppendASCII("certificate_provider");
  const extensions::Extension* const extension = LoadExtension(extension_path);
  ui_test_utils::NavigateToURL(browser(),
                               extension->GetResourceURL("basic.html"));

  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();
  VLOG(1) << "Extension registered. Navigate to the test https page.";

  content::WebContents* const extension_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  content::TestNavigationObserver navigation_observer(
      nullptr /* no WebContents */);
  navigation_observer.StartWatchingNewWebContents();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), https_server.GetURL("client-cert"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_NONE);

  content::WebContents* const https_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  VLOG(1) << "Wait for the extension to respond to the certificates request.";
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  VLOG(1) << "Wait for the extension to receive the sign request.";
  ASSERT_TRUE(catcher.GetNextResult()) << catcher.message();

  VLOG(1) << "Fetch the digest from the sign request.";
  std::vector<uint8_t> request_digest;
  {
    base::RunLoop run_loop;
    extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("signDigestRequest.digest;"),
        base::Bind(&StoreDigest, &request_digest, run_loop.QuitClosure()));
    run_loop.Run();
  }

  VLOG(1) << "Sign the digest using the private key.";
  std::string key_pk8;
  base::ReadFileToString(extension_path.AppendASCII("l1_leaf.pk8"), &key_pk8);

  const uint8_t* const key_pk8_begin =
      reinterpret_cast<const uint8_t*>(key_pk8.data());
  std::unique_ptr<crypto::RSAPrivateKey> key(
      crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(
          std::vector<uint8_t>(key_pk8_begin, key_pk8_begin + key_pk8.size())));
  ASSERT_TRUE(key);

  std::vector<uint8_t> signature;
  EXPECT_TRUE(RsaSign(request_digest, key.get(), &signature));

  VLOG(1) << "Inject the signature back to the extension and let it reply.";
  {
    base::RunLoop run_loop;
    const std::string code =
        "replyWithSignature(" + JsUint8Array(signature) + ");";
    extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16(code),
        base::Bind(&IgnoreResult, run_loop.QuitClosure()));
    run_loop.Run();
  }

  VLOG(1) << "Wait for the https navigation to finish.";
  navigation_observer.Wait();

  VLOG(1) << "Check whether the server acknowledged that a client certificate "
             "was presented.";
  {
    base::RunLoop run_loop;
    std::string https_reply;
    https_contents->GetMainFrame()->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.body.textContent;"),
        base::Bind(&StoreString, &https_reply, run_loop.QuitClosure()));
    run_loop.Run();
    // Expect the server to return the fingerprint of the client cert that we
    // presented, which should be the fingerprint of 'l1_leaf.der'.
    // The fingerprint can be calculated independently using:
    // openssl x509 -inform DER -noout -fingerprint -in
    //   chrome/test/data/extensions/api_test/certificate_provider/l1_leaf.der
    ASSERT_EQ(
        "got client cert with fingerprint: "
        "2ab3f55e06eb8b36a741fe285a769da45edb2695",
        https_reply);
  }

  // Replying to the same signature request a second time must fail.
  {
    base::RunLoop run_loop;
    const std::string code = "replyWithSignatureSecondTime();";
    bool result = false;
    extension_contents->GetMainFrame()->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16(code),
        base::Bind(&StoreBool, &result, run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_TRUE(result);
  }
}

// User enters the correct PIN.
IN_PROC_BROWSER_TEST_F(CertificateProviderRequestPinTest, ShowPinDialogAccept) {
  chromeos::CertificateProviderService* service =
      LoadRequestPinExtension("request_pin", "basic.html");

  // Enter the valid PIN.
  EnterCorrectPin(service);

  // The view should be set to nullptr when the window is closed.
  EXPECT_EQ(service->pin_dialog_manager()->active_view_for_testing(), nullptr);
}

// User closes the dialog kMaxClosedDialogsPer10Mins times, and the extension
// should be blocked from showing it again.
IN_PROC_BROWSER_TEST_F(CertificateProviderRequestPinTest, ShowPinDialogClose) {
  chromeos::CertificateProviderService* service =
      LoadRequestPinExtension("request_pin", "basic.html");

  views::Widget* window =
      service->pin_dialog_manager()->active_window_for_testing();
  for (int i = 0;
       i < extensions::api::certificate_provider::kMaxClosedDialogsPer10Mins;
       i++) {
    ExtensionTestMessageListener listener("User closed the dialog", false);
    window->Close();
    ASSERT_TRUE(listener.WaitUntilSatisfied());
    window = service->pin_dialog_manager()->active_window_for_testing();
  }

  ExtensionTestMessageListener close_listener("User closed the dialog", true);
  window->Close();
  ASSERT_TRUE(close_listener.WaitUntilSatisfied());
  close_listener.Reply("GetLastError");
  ExtensionTestMessageListener last_error_listener(
      "This request exceeds the MAX_PIN_DIALOGS_CLOSED_PER_10_MINUTES quota.",
      false);
  ASSERT_TRUE(last_error_listener.WaitUntilSatisfied());
  EXPECT_EQ(service->pin_dialog_manager()->active_view_for_testing(), nullptr);
}

// User enters a wrong PIN first and a correct PIN on the second try.
IN_PROC_BROWSER_TEST_F(CertificateProviderRequestPinTest,
                       ShowPinDialogWrongPin) {
  chromeos::CertificateProviderService* service =
      LoadRequestPinExtension("request_pin", "basic.html");
  EnterWrongPin(service);

  // The window should be active.
  EXPECT_EQ(
      service->pin_dialog_manager()->active_window_for_testing()->IsVisible(),
      true);
  EXPECT_NE(service->pin_dialog_manager()->active_view_for_testing(), nullptr);

  // Enter the valid PIN.
  EnterCorrectPin(service);

  // The view should be set to nullptr when the window is closed.
  EXPECT_EQ(service->pin_dialog_manager()->active_view_for_testing(), nullptr);
}

// User enters wrong PIN three times.
IN_PROC_BROWSER_TEST_F(CertificateProviderRequestPinTest,
                       ShowPinDialogWrongPinThreeTimes) {
  chromeos::CertificateProviderService* service =
      LoadRequestPinExtension("request_pin", "basic.html");
  for (int i = 0; i < 3; i++) {
    EnterWrongPin(service);
  }

  chromeos::RequestPinView* view =
      service->pin_dialog_manager()->active_view_for_testing();

  // The textfield has to be disabled, as extension does not allow input now.
  EXPECT_EQ(view->textfield_for_testing()->enabled(), false);

  // Close the dialog.
  ExtensionTestMessageListener listener("No attempt left", false);
  service->pin_dialog_manager()->active_window_for_testing()->Close();
  ASSERT_TRUE(listener.WaitUntilSatisfied());
  EXPECT_EQ(service->pin_dialog_manager()->active_view_for_testing(), nullptr);
}

// User closes the dialog while the extension is processing the request.
IN_PROC_BROWSER_TEST_F(CertificateProviderRequestPinTest,
                       ShowPinDialogCloseWhileProcessing) {
  chromeos::CertificateProviderService* service =
      LoadRequestPinExtension("request_pin", "basic_lock.html");

  EnterCode(service, base::ASCIIToUTF16("123"));
  service->pin_dialog_manager()->active_window_for_testing()->Close();
  base::RunLoop().RunUntilIdle();

  // The view should be set to nullptr when the window is closed.
  EXPECT_EQ(service->pin_dialog_manager()->active_view_for_testing(), nullptr);
}
