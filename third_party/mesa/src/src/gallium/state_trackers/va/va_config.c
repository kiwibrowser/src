/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <va/va.h>
#include <va/va_backend.h>

#include "util/u_debug.h"

#include "va_private.h"

VAStatus
vlVaQueryConfigProfiles(VADriverContextP ctx, VAProfile *profile_list, int *num_profiles)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   int i = 0;

   profile_list[i++] = VAProfileMPEG2Simple;
   *num_profiles = i;

   return VA_STATUS_SUCCESS;
}


VAStatus
vlVaQueryConfigEntrypoints(VADriverContextP ctx, VAProfile profile,
                           VAEntrypoint *entrypoint_list, int *num_entrypoints)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;
		
   VAStatus vaStatus = VA_STATUS_SUCCESS;

   switch (profile) {
   case VAProfileMPEG2Simple:
   case VAProfileMPEG2Main:
      VA_INFO("Using profile %08x\n",profile);
      entrypoint_list[0] = VAEntrypointMoComp;
      *num_entrypoints = 1;
      break;

   case VAProfileH264Baseline:
   case VAProfileH264Main:
   case VAProfileH264High:
      vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
      *num_entrypoints = 0;
      break;

   default:
      VA_ERROR("Unsupported profile %08x\n",profile);
      vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
      *num_entrypoints = 0;
      break;
   }

   return vaStatus;
}

VAStatus
vlVaGetConfigAttributes(VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,
                        VAConfigAttrib *attrib_list, int num_attribs)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaCreateConfig(VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,
                 VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaDestroyConfig(VADriverContextP ctx, VAConfigID config_id)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus
vlVaQueryConfigAttributes(VADriverContextP ctx, VAConfigID config_id, VAProfile *profile,
                          VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list, int *num_attribs)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_ERROR_UNIMPLEMENTED;
}
