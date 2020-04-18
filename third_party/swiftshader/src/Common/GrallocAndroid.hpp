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

#ifndef GRALLOC_ANDROID
#define GRALLOC_ANDROID

#include <hardware/gralloc.h>
#include <cutils/log.h>

#ifdef HAVE_GRALLOC1
#include <hardware/gralloc1.h>
#include <sync/sync.h>
#endif

#include <unistd.h> // for close()

class GrallocModule
{
public:
	static GrallocModule *getInstance();
	int lock(buffer_handle_t handle, int usage, int left, int top, int width, int height, void **vaddr)
	{
		switch(m_major_version)
		{
		case 0:
			{
				return m_module->lock(m_module, handle, usage, left, top, width, height, vaddr);
			}
		case 1:
#ifdef HAVE_GRALLOC1
			{
				gralloc1_rect_t outRect{};
				outRect.left = left;
				outRect.top = top;
				outRect.width = width;
				outRect.height = height;
				return m_gralloc1_lock(m_gralloc1_device, handle, usage, usage, &outRect, vaddr, -1);
			}
#endif
		default:
			{
				ALOGE("no gralloc module to lock");
				return -1;
			}
		}
	}

	int unlock(buffer_handle_t handle)
	{
		switch(m_major_version)
		{
		case 0:
			{
				return m_module->unlock(m_module, handle);
			}
		case 1:
#ifdef HAVE_GRALLOC1
			{
				int32_t fenceFd = -1;
				int error = m_gralloc1_unlock(m_gralloc1_device, handle, &fenceFd);
				if (!error)
				{
					sync_wait(fenceFd, -1);
					close(fenceFd);
				}
				return error;
			}
#endif
		default:
			{
				ALOGE("no gralloc module to unlock");
				return -1;
			}
		}
	}

private:
	GrallocModule();
	uint8_t m_major_version;
	const gralloc_module_t *m_module;
#ifdef HAVE_GRALLOC1
	gralloc1_device_t *m_gralloc1_device = nullptr;
	GRALLOC1_PFN_LOCK m_gralloc1_lock = nullptr;
	GRALLOC1_PFN_UNLOCK m_gralloc1_unlock = nullptr;
#endif
};

#endif  // GRALLOC_ANDROID
