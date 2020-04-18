// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/power_ui.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/chromeos/power/power_data_collector.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace chromeos {

namespace {

const char kStringsJsFile[] = "strings.js";

const char kRequestBatteryChargeDataCallback[] = "requestBatteryChargeData";
const char kOnRequestBatteryChargeDataFunction[] =
    "powerUI.showBatteryChargeData";

const char kRequestCpuIdleDataCallback[] = "requestCpuIdleData";
const char kOnRequestCpuIdleDataFunction[] =
    "powerUI.showCpuIdleData";

const char kRequestCpuFreqDataCallback[] = "requestCpuFreqData";
const char kOnRequestCpuFreqDataFunction[] =
    "powerUI.showCpuFreqData";

class PowerMessageHandler : public content::WebUIMessageHandler {
 public:
  PowerMessageHandler();
  ~PowerMessageHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void OnGetBatteryChargeData(const base::ListValue* value);
  void OnGetCpuIdleData(const base::ListValue* value);
  void OnGetCpuFreqData(const base::ListValue* value);
  void GetJsStateOccupancyData(
      const std::vector<CpuDataCollector::StateOccupancySampleDeque>& data,
      const std::vector<std::string>& state_names,
      base::ListValue* js_data);
  void GetJsSystemResumedData(base::ListValue* value);
};

PowerMessageHandler::PowerMessageHandler() {
}

PowerMessageHandler::~PowerMessageHandler() {
}

void PowerMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      kRequestBatteryChargeDataCallback,
      base::BindRepeating(&PowerMessageHandler::OnGetBatteryChargeData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestCpuIdleDataCallback,
      base::BindRepeating(&PowerMessageHandler::OnGetCpuIdleData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestCpuFreqDataCallback,
      base::BindRepeating(&PowerMessageHandler::OnGetCpuFreqData,
                          base::Unretained(this)));
}

void PowerMessageHandler::OnGetBatteryChargeData(const base::ListValue* value) {
  const base::circular_deque<PowerDataCollector::PowerSupplySample>&
      power_supply = PowerDataCollector::Get()->power_supply_data();
  base::ListValue js_power_supply_data;
  for (size_t i = 0; i < power_supply.size(); ++i) {
    const PowerDataCollector::PowerSupplySample& sample = power_supply[i];
    std::unique_ptr<base::DictionaryValue> element(new base::DictionaryValue);
    element->SetDouble("batteryPercent", sample.battery_percent);
    element->SetDouble("batteryDischargeRate", sample.battery_discharge_rate);
    element->SetBoolean("externalPower", sample.external_power);
    element->SetDouble("time", sample.time.ToJsTime());

    js_power_supply_data.Append(std::move(element));
  }

  base::ListValue js_system_resumed_data;
  GetJsSystemResumedData(&js_system_resumed_data);

  web_ui()->CallJavascriptFunctionUnsafe(kOnRequestBatteryChargeDataFunction,
                                         js_power_supply_data,
                                         js_system_resumed_data);
}

void PowerMessageHandler::OnGetCpuIdleData(const base::ListValue* value) {
  const CpuDataCollector& cpu_data_collector =
      PowerDataCollector::Get()->cpu_data_collector();

  const std::vector<CpuDataCollector::StateOccupancySampleDeque>& idle_data =
      cpu_data_collector.cpu_idle_state_data();
  const std::vector<std::string>& idle_state_names =
      cpu_data_collector.cpu_idle_state_names();
  base::ListValue js_idle_data;
  GetJsStateOccupancyData(idle_data, idle_state_names, &js_idle_data);

  base::ListValue js_system_resumed_data;
  GetJsSystemResumedData(&js_system_resumed_data);

  web_ui()->CallJavascriptFunctionUnsafe(kOnRequestCpuIdleDataFunction,
                                         js_idle_data, js_system_resumed_data);
}

void PowerMessageHandler::OnGetCpuFreqData(const base::ListValue* value) {
  const CpuDataCollector& cpu_data_collector =
      PowerDataCollector::Get()->cpu_data_collector();

  const std::vector<CpuDataCollector::StateOccupancySampleDeque>& freq_data =
      cpu_data_collector.cpu_freq_state_data();
  const std::vector<std::string>& freq_state_names =
      cpu_data_collector.cpu_freq_state_names();
  base::ListValue js_freq_data;
  GetJsStateOccupancyData(freq_data, freq_state_names, &js_freq_data);

  base::ListValue js_system_resumed_data;
  GetJsSystemResumedData(&js_system_resumed_data);

  web_ui()->CallJavascriptFunctionUnsafe(kOnRequestCpuFreqDataFunction,
                                         js_freq_data, js_system_resumed_data);
}

void PowerMessageHandler::GetJsSystemResumedData(base::ListValue *data) {
  DCHECK(data);

  const base::circular_deque<PowerDataCollector::SystemResumedSample>&
      system_resumed = PowerDataCollector::Get()->system_resumed_data();
  for (size_t i = 0; i < system_resumed.size(); ++i) {
    const PowerDataCollector::SystemResumedSample& sample = system_resumed[i];
    std::unique_ptr<base::DictionaryValue> element(new base::DictionaryValue);
    element->SetDouble("sleepDuration",
                       sample.sleep_duration.InMillisecondsF());
    element->SetDouble("time", sample.time.ToJsTime());

    data->Append(std::move(element));
  }
}

void PowerMessageHandler::GetJsStateOccupancyData(
    const std::vector<CpuDataCollector::StateOccupancySampleDeque>& data,
    const std::vector<std::string>& state_names,
    base::ListValue *js_data) {
  for (unsigned int cpu = 0; cpu < data.size(); ++cpu) {
    const CpuDataCollector::StateOccupancySampleDeque& sample_deque = data[cpu];
    std::unique_ptr<base::ListValue> js_sample_list(new base::ListValue);
    for (unsigned int i = 0; i < sample_deque.size(); ++i) {
      const CpuDataCollector::StateOccupancySample& sample = sample_deque[i];
      std::unique_ptr<base::DictionaryValue> js_sample(
          new base::DictionaryValue);
      js_sample->SetDouble("time", sample.time.ToJsTime());
      js_sample->SetBoolean("cpuOnline", sample.cpu_online);

      std::unique_ptr<base::DictionaryValue> state_dict(
          new base::DictionaryValue);
      for (size_t index = 0; index < sample.time_in_state.size(); ++index) {
        state_dict->SetDouble(state_names[index],
                              sample.time_in_state[index].InMillisecondsF());
      }
      js_sample->Set("timeInState", std::move(state_dict));

      js_sample_list->Append(std::move(js_sample));
    }
    js_data->Append(std::move(js_sample_list));
  }
}

}  // namespace

PowerUI::PowerUI(content::WebUI* web_ui) : content::WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<PowerMessageHandler>());

  content::WebUIDataSource* html =
      content::WebUIDataSource::Create(chrome::kChromeUIPowerHost);

  html->AddLocalizedString("titleText", IDS_ABOUT_POWER_TITLE);
  html->AddLocalizedString("showButton", IDS_ABOUT_POWER_SHOW_BUTTON);
  html->AddLocalizedString("hideButton", IDS_ABOUT_POWER_HIDE_BUTTON);
  html->AddLocalizedString("reloadButton", IDS_ABOUT_POWER_RELOAD_BUTTON);
  html->AddLocalizedString("notEnoughDataAvailableYet",
                           IDS_ABOUT_POWER_NOT_ENOUGH_DATA);
  html->AddLocalizedString("systemSuspended",
                           IDS_ABOUT_POWER_SYSTEM_SUSPENDED);
  html->AddLocalizedString("invalidData", IDS_ABOUT_POWER_INVALID);
  html->AddLocalizedString("offlineText", IDS_ABOUT_POWER_OFFLINE);

  html->AddLocalizedString("batteryChargeSectionTitle",
                           IDS_ABOUT_POWER_BATTERY_CHARGE_SECTION_TITLE);
  html->AddLocalizedString("batteryChargePercentageHeader",
                           IDS_ABOUT_POWER_BATTERY_CHARGE_PERCENTAGE_HEADER);
  html->AddLocalizedString("batteryDischargeRateHeader",
                           IDS_ABOUT_POWER_BATTERY_DISCHARGE_RATE_HEADER);
  html->AddLocalizedString("dischargeRateLegendText",
                           IDS_ABOUT_POWER_DISCHARGE_RATE_LEGEND_TEXT);
  html->AddLocalizedString("movingAverageLegendText",
                           IDS_ABOUT_POWER_MOVING_AVERAGE_LEGEND_TEXT);
  html->AddLocalizedString("binnedAverageLegendText",
                           IDS_ABOUT_POWER_BINNED_AVERAGE_LEGEND_TEXT);
  html->AddLocalizedString("averageOverText",
                           IDS_ABOUT_POWER_AVERAGE_OVER_TEXT);
  html->AddLocalizedString("samplesText",
                           IDS_ABOUT_POWER_AVERAGE_SAMPLES_TEXT);

  html->AddLocalizedString("cpuIdleSectionTitle",
                           IDS_ABOUT_POWER_CPU_IDLE_SECTION_TITLE);
  html->AddLocalizedString("idleStateOccupancyPercentageHeader",
                           IDS_ABOUT_POWER_CPU_IDLE_STATE_OCCUPANCY_PERCENTAGE);

  html->AddLocalizedString("cpuFreqSectionTitle",
                           IDS_ABOUT_POWER_CPU_FREQ_SECTION_TITLE);
  html->AddLocalizedString("frequencyStateOccupancyPercentageHeader",
                           IDS_ABOUT_POWER_CPU_FREQ_STATE_OCCUPANCY_PERCENTAGE);

  html->SetJsonPath(kStringsJsFile);

  html->AddResourcePath("power.css", IDR_ABOUT_POWER_CSS);
  html->AddResourcePath("power.js", IDR_ABOUT_POWER_JS);
  html->SetDefaultResource(IDR_ABOUT_POWER_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html);
}

PowerUI::~PowerUI() {
}

}  // namespace chromeos
