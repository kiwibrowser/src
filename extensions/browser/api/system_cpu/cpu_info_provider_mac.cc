// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/system_cpu/cpu_info_provider.h"

#include <mach/mach_host.h>

#include "base/mac/scoped_mach_port.h"
#include "base/sys_info.h"

namespace extensions {

bool CpuInfoProvider::QueryCpuTimePerProcessor(
    std::vector<api::system_cpu::ProcessorInfo>* infos) {
  DCHECK(infos);

  natural_t num_of_processors;
  base::mac::ScopedMachSendRight host(mach_host_self());
  mach_msg_type_number_t type;
  processor_cpu_load_info_data_t* cpu_infos;

  if (host_processor_info(host.get(),
                          PROCESSOR_CPU_LOAD_INFO,
                          &num_of_processors,
                          reinterpret_cast<processor_info_array_t*>(&cpu_infos),
                          &type) == KERN_SUCCESS) {
    DCHECK_EQ(num_of_processors,
              static_cast<natural_t>(base::SysInfo::NumberOfProcessors()));
    DCHECK_EQ(num_of_processors, static_cast<natural_t>(infos->size()));

    for (natural_t i = 0; i < num_of_processors; ++i) {
      double user = static_cast<double>(cpu_infos[i].cpu_ticks[CPU_STATE_USER]),
             sys =
                 static_cast<double>(cpu_infos[i].cpu_ticks[CPU_STATE_SYSTEM]),
             nice = static_cast<double>(cpu_infos[i].cpu_ticks[CPU_STATE_NICE]),
             idle = static_cast<double>(cpu_infos[i].cpu_ticks[CPU_STATE_IDLE]);

      infos->at(i).usage.kernel = sys;
      infos->at(i).usage.user = user + nice;
      infos->at(i).usage.idle = idle;
      infos->at(i).usage.total = sys + user + nice + idle;
    }

    vm_deallocate(mach_task_self(),
                  reinterpret_cast<vm_address_t>(cpu_infos),
                  num_of_processors * sizeof(processor_cpu_load_info));

    return true;
  }

  return false;
}

}  // namespace extensions
