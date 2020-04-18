// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/cpp/frame_resource_coordinator.h"
#include "services/resource_coordinator/public/cpp/page_resource_coordinator.h"
#include "services/resource_coordinator/public/cpp/process_resource_coordinator.h"
#include "services/resource_coordinator/public/cpp/system_resource_coordinator.h"
#include "services/resource_coordinator/public/mojom/coordination_unit_provider.mojom.h"
#include "services/resource_coordinator/public/mojom/service_constants.mojom.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_test.h"

namespace resource_coordinator {

class ResourceCoordinatorTest : public service_manager::test::ServiceTest {
 public:
  ResourceCoordinatorTest()
      : service_manager::test::ServiceTest("resource_coordinator_unittests") {}
  ~ResourceCoordinatorTest() override {}

  void GetIDCallback(const CoordinationUnitID& cu_id) {
    loop_->Quit();
  }

  // Given a CU, tests that it works by invoking GetID and waiting for the
  // response. This test will hang (and eventually fail) if the response does
  // not come back from the remote endpoint.
  template <typename CoordinationUnitPtrType>
  void TestCUImpl(CoordinationUnitPtrType cu) {
    base::RunLoop loop;
    loop_ = &loop;
    cu->GetID(base::BindOnce(&ResourceCoordinatorTest::GetIDCallback,
                             base::Unretained(this)));
    loop.Run();
    loop_ = nullptr;
  }

  // Variant that works with mojo interface pointers.
  template <typename CoordinationUnitPtrType>
  void TestCU(CoordinationUnitPtrType& cu) {
    TestCUImpl<CoordinationUnitPtrType&>(cu);
  }

  // Variant that works with pointers to FooResourceCoordinator wrappers.
  template <typename CoordinationUnitPtrType>
  void TestCU(CoordinationUnitPtrType* cu) {
    TestCUImpl<CoordinationUnitPtrType*>(cu);
  }

 protected:
  void SetUp() override {
    service_manager::test::ServiceTest::SetUp();
    connector()->StartService(mojom::kServiceName);
  }

 private:
  base::RunLoop* loop_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ResourceCoordinatorTest);
};

TEST_F(ResourceCoordinatorTest, ResourceCoordinatorInstantiate) {
  // Get the CU provider interface.
  mojom::CoordinationUnitProviderPtr provider;
  connector()->BindInterface(mojom::kServiceName, mojo::MakeRequest(&provider));

  // Create and test a dummy FrameCU.
  CoordinationUnitID frame_id(CoordinationUnitType::kFrame, "");
  mojom::FrameCoordinationUnitPtr frame_cu;
  provider->CreateFrameCoordinationUnit(mojo::MakeRequest(&frame_cu), frame_id);
  TestCU(frame_cu);

  // Create and test a dummy PageCU.
  CoordinationUnitID page_id(CoordinationUnitType::kPage, "");
  mojom::PageCoordinationUnitPtr page_cu;
  provider->CreatePageCoordinationUnit(mojo::MakeRequest(&page_cu), page_id);
  TestCU(page_cu);

  // Create and test a dummy SystemCU.
  mojom::SystemCoordinationUnitPtr system_cu;
  provider->GetSystemCoordinationUnit(mojo::MakeRequest(&system_cu));
  TestCU(system_cu);

  // Create and test a dummy ProcessCU.
  CoordinationUnitID process_id(CoordinationUnitType::kProcess, "");
  mojom::ProcessCoordinationUnitPtr process_cu;
  provider->CreateProcessCoordinationUnit(mojo::MakeRequest(&process_cu),
                                          process_id);
  TestCU(process_cu);

  // Also test the convenience headers for creating and communicating with CUs.
  FrameResourceCoordinator frame_rc(connector());
  TestCU(&frame_rc);
  PageResourceCoordinator page_rc(connector());
  TestCU(&page_rc);
  ProcessResourceCoordinator process_rc(connector());
  TestCU(&process_rc);
  SystemResourceCoordinator system_rc(connector());
  TestCU(&system_rc);
}

}  // namespace resource_coordinator
