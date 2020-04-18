// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GrallocAndroid.hpp"

#include <cutils/log.h>

GrallocModule *GrallocModule::getInstance()
{
	static GrallocModule instance;
	return &instance;
}

GrallocModule::GrallocModule()
{
	const hw_module_t *module = nullptr;
	hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);

	m_major_version = (module->module_api_version >> 8) & 0xff;
	switch(m_major_version)
	{
	case 0:
		m_module = reinterpret_cast<const gralloc_module_t*>(module);
		break;
	case 1:
#ifdef HAVE_GRALLOC1
		gralloc1_open(module, &m_gralloc1_device);
		m_gralloc1_lock = (GRALLOC1_PFN_LOCK) m_gralloc1_device->getFunction(m_gralloc1_device, GRALLOC1_FUNCTION_LOCK);
		m_gralloc1_unlock = (GRALLOC1_PFN_UNLOCK)m_gralloc1_device->getFunction(m_gralloc1_device, GRALLOC1_FUNCTION_UNLOCK);
		break;
#endif
	default:
		ALOGE("unknown gralloc major version (%d)", m_major_version);
		break;
	}
}
