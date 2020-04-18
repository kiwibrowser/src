// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <map>
#include <utility>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/interfaces/bindings/tests/versioning_test_service.mojom.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_runner.h"

namespace mojo {
namespace test {
namespace versioning {

struct EmployeeInfo {
 public:
  EmployeeInfo() {}

  EmployeePtr employee;
  Array<uint8_t> finger_print;

 private:
  DISALLOW_COPY_AND_ASSIGN(EmployeeInfo);
};

class HumanResourceDatabaseImpl : public HumanResourceDatabase {
 public:
  explicit HumanResourceDatabaseImpl(
      InterfaceRequest<HumanResourceDatabase> request)
      : strong_binding_(this, std::move(request)) {
    // Pretend that there is already some data in the system.
    EmployeeInfo* info = new EmployeeInfo();
    employees_[1] = info;
    info->employee = Employee::New();
    info->employee->employee_id = 1;
    info->employee->name = "Homer Simpson";
    info->employee->department = DEPARTMENT_DEV;
    info->employee->birthday = Date::New();
    info->employee->birthday->year = 1955;
    info->employee->birthday->month = 5;
    info->employee->birthday->day = 12;
    info->finger_print.resize(1024);
    for (uint32_t i = 0; i < 1024; ++i)
      info->finger_print[i] = i;
  }

  ~HumanResourceDatabaseImpl() override {
    for (auto iter = employees_.begin(); iter != employees_.end(); ++iter)
      delete iter->second;
  }

  void AddEmployee(EmployeePtr employee,
                   const AddEmployeeCallback& callback) override {
    uint64_t id = employee->employee_id;
    if (employees_.find(id) == employees_.end())
      employees_[id] = new EmployeeInfo();
    employees_[id]->employee = std::move(employee);
    callback.Run(true);
  }

  void QueryEmployee(uint64_t id,
                     bool retrieve_finger_print,
                     const QueryEmployeeCallback& callback) override {
    if (employees_.find(id) == employees_.end()) {
      callback.Run(nullptr, Array<uint8_t>());
      return;
    }
    callback.Run(employees_[id]->employee.Clone(),
                 retrieve_finger_print ? employees_[id]->finger_print.Clone()
                                       : Array<uint8_t>());
  }

  void AttachFingerPrint(uint64_t id,
                         Array<uint8_t> finger_print,
                         const AttachFingerPrintCallback& callback) override {
    if (employees_.find(id) == employees_.end()) {
      callback.Run(false);
      return;
    }
    employees_[id]->finger_print = std::move(finger_print);
    callback.Run(true);
  }

 private:
  std::map<uint64_t, EmployeeInfo*> employees_;

  StrongBinding<HumanResourceDatabase> strong_binding_;
};

class HumanResourceSystemServer : public service_manager::Service {
 public:
  HumanResourceSystemServer() {
    registry_.AddInterface<HumanResourceDatabase>(
        base::Bind(&HumanResourceSystemServer::Create, base::Unretained(this)));
  }

  // service_manager::Service implementation.
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void Create(HumanResourceDatabaseRequest request) {
    // It will be deleted automatically when the underlying pipe encounters a
    // connection error.
    new HumanResourceDatabaseImpl(std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
};

}  // namespace versioning
}  // namespace test
}  // namespace mojo

MojoResult ServiceMain(MojoHandle request) {
  mojo::ServiceRunner runner(
      new mojo::test::versioning::HumanResourceSystemServer());

  return runner.Run(request);
}
