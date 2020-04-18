// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mach/mach.h>

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/perf/perf_test.h"

namespace {

// This test spawns a new browser and counts the number of open Mach ports in
// the browser process. It navigates tabs and closes them, repeatedly measuring
// the number of open ports. This is used to protect against leaking Mach ports,
// which was the source of <http://crbug.com/105513>.
class MachPortsTest : public InProcessBrowserTest {
 public:
  MachPortsTest() {
    embedded_test_server()->ServeFilesFromSourceDirectory(
        "data/mach_ports/moz");
  }

  void SetUpOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDown() override {
    std::string ports;
    for (int port : port_counts_)
      base::StringAppendF(&ports, "%d,", port);
    perf_test::PrintResultList("mach_ports", "", "", ports, "ports", true);

    InProcessBrowserTest::TearDown();
  }

  // Gets the browser's current number of Mach ports and records it.
  void RecordPortCount() {
    mach_port_name_array_t names;
    mach_msg_type_number_t names_count = 0;
    mach_port_type_array_t types;
    mach_msg_type_number_t types_count = 0;

    mach_port_t self = mach_task_self();

    // A friendlier interface would allow NULL buffers to only get the counts.
    kern_return_t kr = mach_port_names(self,
                                       &names, &names_count,
                                       &types, &types_count);
    ASSERT_EQ(KERN_SUCCESS, kr) << "Failed to get mach_port_names(): "
                                << mach_error_string(kr);
    ASSERT_EQ(names_count, types_count);  // Documented kernel invariant.

    port_counts_.push_back(names_count);

    vm_deallocate(self, reinterpret_cast<vm_address_t>(names),
        names_count * sizeof(mach_port_name_array_t));
    vm_deallocate(self, reinterpret_cast<vm_address_t>(types),
        types_count * sizeof(mach_port_type_array_t));
  }

  // Adds a tab from the page cycler data at the specified domain.
  void AddTab(const std::string& domain) {
    AddTabAtIndex(
        0, embedded_test_server()->GetURL("/" + domain + "/").Resolve("?skip"),
        ui::PAGE_TRANSITION_TYPED);
  }

 private:
  std::vector<int> port_counts_;
};

IN_PROC_BROWSER_TEST_F(MachPortsTest, GetCounts) {
  browser()->window()->Show();

  // Record startup number.
  RecordPortCount();

  // Create a browser and open a few tabs.
  AddTab("www.google.com");
  RecordPortCount();

  AddTab("www.cnn.com");
  RecordPortCount();

  AddTab("www.nytimes.com");
  RecordPortCount();

  TabStripModel* tab_strip_model = browser()->tab_strip_model();
  int tab_count = tab_strip_model->count();
  ASSERT_EQ(4, tab_count);  // Also count about:blank.

  // Close each tab, recording the number of ports after each. Do not close the
  // last tab, which is about:blank.
  for (int i = 0; i < tab_count - 1; ++i) {
    EXPECT_TRUE(
        tab_strip_model->CloseWebContentsAt(0, TabStripModel::CLOSE_NONE));
    RecordPortCount();
  }
}

}  // namespace
