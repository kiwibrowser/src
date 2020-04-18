/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Lobodzinski <mark@lunarg.com>
 *
 */

#include <string.h>
#include <string>
#include <vector>
#include <map>
#include "vulkan/vulkan.h"
#include "vk_layer_config.h"
#include "vk_layer_utils.h"

struct VULKAN_FORMAT_INFO {
    size_t size;
    uint32_t channel_count;
    VkFormatCompatibilityClass format_class;
};

// Set up data structure with number of bytes and number of channels for each Vulkan format
const std::map<VkFormat, VULKAN_FORMAT_INFO> vk_format_table = {
    {VK_FORMAT_UNDEFINED,                   {0, 0, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT }},
    {VK_FORMAT_R4G4_UNORM_PACK8,            {1, 2, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R4G4B4A4_UNORM_PACK16,       {2, 4, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_B4G4R4A4_UNORM_PACK16,       {2, 4, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R5G6B5_UNORM_PACK16,         {2, 3, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_B5G6R5_UNORM_PACK16,         {2, 3, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R5G5B5A1_UNORM_PACK16,       {2, 4, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_B5G5R5A1_UNORM_PACK16,       {2, 4, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_A1R5G5B5_UNORM_PACK16,       {2, 4, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8_UNORM,                    {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8_SNORM,                    {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8_USCALED,                  {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8_SSCALED,                  {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8_UINT,                     {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8_SINT,                     {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8_SRGB,                     {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_8_BIT}},
    {VK_FORMAT_R8G8_UNORM,                  {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8_SNORM,                  {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8_USCALED,                {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8_SSCALED,                {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8_UINT,                   {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8_SINT,                   {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8_SRGB,                   {2, 2, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R8G8B8_UNORM,                {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8_SNORM,                {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8_USCALED,              {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8_SSCALED,              {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8_UINT,                 {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8_SINT,                 {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8_SRGB,                 {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_UNORM,                {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_SNORM,                {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_USCALED,              {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_SSCALED,              {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_UINT,                 {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_SINT,                 {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_B8G8R8_SRGB,                 {3, 3, VK_FORMAT_COMPATIBILITY_CLASS_24_BIT}},
    {VK_FORMAT_R8G8B8A8_UNORM,              {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R8G8B8A8_SNORM,              {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R8G8B8A8_USCALED,            {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R8G8B8A8_SSCALED,            {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R8G8B8A8_UINT,               {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R8G8B8A8_SINT,               {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R8G8B8A8_SRGB,               {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_UNORM,              {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_SNORM,              {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_USCALED,            {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_SSCALED,            {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_UINT,               {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_SINT,               {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_B8G8R8A8_SRGB,               {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_UNORM_PACK32,       {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_SNORM_PACK32,       {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_USCALED_PACK32,     {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_SSCALED_PACK32,     {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_UINT_PACK32,        {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_SINT_PACK32,        {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A8B8G8R8_SRGB_PACK32,        {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2R10G10B10_UNORM_PACK32,    {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2R10G10B10_SNORM_PACK32,    {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2R10G10B10_USCALED_PACK32,  {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2R10G10B10_SSCALED_PACK32,  {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2R10G10B10_UINT_PACK32,     {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2R10G10B10_SINT_PACK32,     {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32,    {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2B10G10R10_SNORM_PACK32,    {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2B10G10R10_USCALED_PACK32,  {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2B10G10R10_SSCALED_PACK32,  {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2B10G10R10_UINT_PACK32,     {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_A2B10G10R10_SINT_PACK32,     {4, 4, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16_UNORM,                   {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16_SNORM,                   {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16_USCALED,                 {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16_SSCALED,                 {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16_UINT,                    {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16_SINT,                    {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16_SFLOAT,                  {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_16_BIT}},
    {VK_FORMAT_R16G16_UNORM,                {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16_SNORM,                {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16_USCALED,              {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16_SSCALED,              {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16_UINT,                 {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16_SINT,                 {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16_SFLOAT,               {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R16G16B16_UNORM,             {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16_SNORM,             {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16_USCALED,           {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16_SSCALED,           {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16_UINT,              {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16_SINT,              {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16_SFLOAT,            {6, 3, VK_FORMAT_COMPATIBILITY_CLASS_48_BIT}},
    {VK_FORMAT_R16G16B16A16_UNORM,          {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R16G16B16A16_SNORM,          {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R16G16B16A16_USCALED,        {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R16G16B16A16_SSCALED,        {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R16G16B16A16_UINT,           {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R16G16B16A16_SINT,           {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R16G16B16A16_SFLOAT,         {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R32_UINT,                    {4, 1, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R32_SINT,                    {4, 1, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R32_SFLOAT,                  {4, 1, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_R32G32_UINT,                 {8, 2, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R32G32_SINT,                 {8, 2, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R32G32_SFLOAT,               {8, 2, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R32G32B32_UINT,              {12, 3, VK_FORMAT_COMPATIBILITY_CLASS_96_BIT}},
    {VK_FORMAT_R32G32B32_SINT,              {12, 3, VK_FORMAT_COMPATIBILITY_CLASS_96_BIT}},
    {VK_FORMAT_R32G32B32_SFLOAT,            {12, 3, VK_FORMAT_COMPATIBILITY_CLASS_96_BIT}},
    {VK_FORMAT_R32G32B32A32_UINT,           {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_128_BIT}},
    {VK_FORMAT_R32G32B32A32_SINT,           {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_128_BIT}},
    {VK_FORMAT_R32G32B32A32_SFLOAT,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_128_BIT}},
    {VK_FORMAT_R64_UINT,                    {8, 1, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R64_SINT,                    {8, 1, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R64_SFLOAT,                  {8, 1, VK_FORMAT_COMPATIBILITY_CLASS_64_BIT}},
    {VK_FORMAT_R64G64_UINT,                 {16, 2, VK_FORMAT_COMPATIBILITY_CLASS_128_BIT}},
    {VK_FORMAT_R64G64_SINT,                 {16, 2, VK_FORMAT_COMPATIBILITY_CLASS_128_BIT}},
    {VK_FORMAT_R64G64_SFLOAT,               {16, 2, VK_FORMAT_COMPATIBILITY_CLASS_128_BIT}},
    {VK_FORMAT_R64G64B64_UINT,              {24, 3, VK_FORMAT_COMPATIBILITY_CLASS_192_BIT}},
    {VK_FORMAT_R64G64B64_SINT,              {24, 3, VK_FORMAT_COMPATIBILITY_CLASS_192_BIT}},
    {VK_FORMAT_R64G64B64_SFLOAT,            {24, 3, VK_FORMAT_COMPATIBILITY_CLASS_192_BIT}},
    {VK_FORMAT_R64G64B64A64_UINT,           {32, 4, VK_FORMAT_COMPATIBILITY_CLASS_256_BIT}},
    {VK_FORMAT_R64G64B64A64_SINT,           {32, 4, VK_FORMAT_COMPATIBILITY_CLASS_256_BIT}},
    {VK_FORMAT_R64G64B64A64_SFLOAT,         {32, 4, VK_FORMAT_COMPATIBILITY_CLASS_256_BIT}},
    {VK_FORMAT_B10G11R11_UFLOAT_PACK32,     {4, 3, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,      {4, 3, VK_FORMAT_COMPATIBILITY_CLASS_32_BIT}},
    {VK_FORMAT_D16_UNORM,                   {2, 1, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_X8_D24_UNORM_PACK32,         {3, 1, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_D32_SFLOAT,                  {4, 1, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_S8_UINT,                     {1, 1, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_D16_UNORM_S8_UINT,           {3, 2, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_D24_UNORM_S8_UINT,           {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_D32_SFLOAT_S8_UINT,          {4, 2, VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT}},
    {VK_FORMAT_BC1_RGB_UNORM_BLOCK,         {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGB_BIT}},
    {VK_FORMAT_BC1_RGB_SRGB_BLOCK,          {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGB_BIT}},
    {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,        {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGBA_BIT}},
    {VK_FORMAT_BC1_RGBA_SRGB_BLOCK,         {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGBA_BIT}},
    {VK_FORMAT_BC2_UNORM_BLOCK,             {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC2_BIT}},
    {VK_FORMAT_BC2_SRGB_BLOCK,              {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC2_BIT}},
    {VK_FORMAT_BC3_UNORM_BLOCK,             {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC3_BIT}},
    {VK_FORMAT_BC3_SRGB_BLOCK,              {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC3_BIT}},
    {VK_FORMAT_BC4_UNORM_BLOCK,             {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC4_BIT}},
    {VK_FORMAT_BC4_SNORM_BLOCK,             {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC4_BIT}},
    {VK_FORMAT_BC5_UNORM_BLOCK,             {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC5_BIT}},
    {VK_FORMAT_BC5_SNORM_BLOCK,             {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC5_BIT}},
    {VK_FORMAT_BC6H_UFLOAT_BLOCK,           {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC6H_BIT}},
    {VK_FORMAT_BC6H_SFLOAT_BLOCK,           {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC6H_BIT}},
    {VK_FORMAT_BC7_UNORM_BLOCK,             {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC7_BIT}},
    {VK_FORMAT_BC7_SRGB_BLOCK,              {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_BC7_BIT}},
    {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     {8, 3, VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGB_BIT}},
    {VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,      {8, 3, VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGB_BIT}},
    {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,   {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGBA_BIT}},
    {VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,    {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGBA_BIT}},
    {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ETC2_EAC_RGBA_BIT}},
    {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,    {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ETC2_EAC_RGBA_BIT}},
    {VK_FORMAT_EAC_R11_UNORM_BLOCK,         {8, 1, VK_FORMAT_COMPATIBILITY_CLASS_EAC_R_BIT}},
    {VK_FORMAT_EAC_R11_SNORM_BLOCK,         {8, 1, VK_FORMAT_COMPATIBILITY_CLASS_EAC_R_BIT}},
    {VK_FORMAT_EAC_R11G11_UNORM_BLOCK,      {16, 2, VK_FORMAT_COMPATIBILITY_CLASS_EAC_RG_BIT}},
    {VK_FORMAT_EAC_R11G11_SNORM_BLOCK,      {16, 2, VK_FORMAT_COMPATIBILITY_CLASS_EAC_RG_BIT}},
    {VK_FORMAT_ASTC_4x4_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_4X4_BIT}},
    {VK_FORMAT_ASTC_4x4_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_4X4_BIT}},
    {VK_FORMAT_ASTC_5x4_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X4_BIT}},
    {VK_FORMAT_ASTC_5x4_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X4_BIT}},
    {VK_FORMAT_ASTC_5x5_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X5_BIT}},
    {VK_FORMAT_ASTC_5x5_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X5_BIT}},
    {VK_FORMAT_ASTC_6x5_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X5_BIT}},
    {VK_FORMAT_ASTC_6x5_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X5_BIT}},
    {VK_FORMAT_ASTC_6x6_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X6_BIT}},
    {VK_FORMAT_ASTC_6x6_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X6_BIT}},
    {VK_FORMAT_ASTC_8x5_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X5_BIT}},
    {VK_FORMAT_ASTC_8x5_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X5_BIT}},
    {VK_FORMAT_ASTC_8x6_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X6_BIT}},
    {VK_FORMAT_ASTC_8x6_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X6_BIT}},
    {VK_FORMAT_ASTC_8x8_UNORM_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X8_BIT}},
    {VK_FORMAT_ASTC_8x8_SRGB_BLOCK,         {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X8_BIT}},
    {VK_FORMAT_ASTC_10x5_UNORM_BLOCK,       {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X5_BIT}},
    {VK_FORMAT_ASTC_10x5_SRGB_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X5_BIT}},
    {VK_FORMAT_ASTC_10x6_UNORM_BLOCK,       {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X6_BIT}},
    {VK_FORMAT_ASTC_10x6_SRGB_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X6_BIT}},
    {VK_FORMAT_ASTC_10x8_UNORM_BLOCK,       {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X8_BIT}},
    {VK_FORMAT_ASTC_10x8_SRGB_BLOCK,        {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X8_BIT}},
    {VK_FORMAT_ASTC_10x10_UNORM_BLOCK,      {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X10_BIT}},
    {VK_FORMAT_ASTC_10x10_SRGB_BLOCK,       {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X10_BIT}},
    {VK_FORMAT_ASTC_12x10_UNORM_BLOCK,      {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X10_BIT}},
    {VK_FORMAT_ASTC_12x10_SRGB_BLOCK,       {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X10_BIT}},
    {VK_FORMAT_ASTC_12x12_UNORM_BLOCK,      {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X12_BIT}},
    {VK_FORMAT_ASTC_12x12_SRGB_BLOCK,       {16, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X12_BIT}},
    {VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X6_BIT }},
    {VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X6_BIT}},
    {VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X8_BIT}},
    {VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X8_BIT}},
    {VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,  {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X10_BIT}},
    {VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,  {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X10_BIT}},
    {VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,  {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X10_BIT}},
    {VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,  {8, 4, VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X10_BIT}},
};

// Return true if format is a depth or stencil format
VK_LAYER_EXPORT bool vk_format_is_depth_or_stencil(VkFormat format) {
    return (vk_format_is_depth_and_stencil(format) || vk_format_is_depth_only(format) || vk_format_is_stencil_only(format));
}

// Return true if format contains depth and stencil information
VK_LAYER_EXPORT bool vk_format_is_depth_and_stencil(VkFormat format) {
    bool is_ds = false;

    switch (format) {
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        is_ds = true;
        break;
    default:
        break;
    }
    return is_ds;
}

// Return true if format is a stencil-only format
VK_LAYER_EXPORT bool vk_format_is_stencil_only(VkFormat format) { return (format == VK_FORMAT_S8_UINT); }

// Return true if format is a depth-only format
VK_LAYER_EXPORT bool vk_format_is_depth_only(VkFormat format) {
    bool is_depth = false;

    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
        is_depth = true;
        break;
    default:
        break;
    }

    return is_depth;
}

// Return true if format is of time UNORM
VK_LAYER_EXPORT bool vk_format_is_norm(VkFormat format) {
    bool is_norm = false;

    switch (format) {
    case VK_FORMAT_R4G4_UNORM_PACK8:
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        is_norm = true;
        break;
    default:
        break;
    }

    return is_norm;
};

// Return true if format is an integer format
VK_LAYER_EXPORT bool vk_format_is_int(VkFormat format) { return (vk_format_is_sint(format) || vk_format_is_uint(format)); }

// Return true if format is an unsigned integer format
VK_LAYER_EXPORT bool vk_format_is_uint(VkFormat format) {
    bool is_uint = false;

    switch (format) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64A64_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        is_uint = true;
        break;
    default:
        break;
    }

    return is_uint;
}

// Return true if format is a signed integer format
VK_LAYER_EXPORT bool vk_format_is_sint(VkFormat format) {
    bool is_sint = false;

    switch (format) {
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64A64_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        is_sint = true;
        break;
    default:
        break;
    }

    return is_sint;
}

// Return true if format is a floating-point format
VK_LAYER_EXPORT bool vk_format_is_float(VkFormat format) {
    bool is_float = false;

    switch (format) {
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R64_SFLOAT:
    case VK_FORMAT_R64G64_SFLOAT:
    case VK_FORMAT_R64G64B64_SFLOAT:
    case VK_FORMAT_R64G64B64A64_SFLOAT:
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        is_float = true;
        break;
    default:
        break;
    }

    return is_float;
}

// Return true if format is in the SRGB colorspace
VK_LAYER_EXPORT bool vk_format_is_srgb(VkFormat format) {
    bool is_srgb = false;

    switch (format) {
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
        is_srgb = true;
        break;
    default:
        break;
    }

    return is_srgb;
}

// Return true if format is compressed
VK_LAYER_EXPORT bool vk_format_is_compressed(VkFormat format) {
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return true;
    default:
        return false;
    }
}

// Return compressed block sizes for block compressed formats
VK_LAYER_EXPORT VkExtent2D vk_format_compressed_block_size(VkFormat format) {
    VkExtent2D block_size = { 1, 1 };
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        block_size = { 4, 4 };
        break;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        block_size = { 5, 4 };
        break;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        block_size = { 5, 5 };
        break;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        block_size = { 6, 5 };
        break;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        block_size = { 6, 6 };
        break;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        block_size = { 8, 5 };
        break;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        block_size = { 8, 6 };
        break;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        block_size = { 8, 8 };
        break;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        block_size = { 10, 5 };
        break;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        block_size = { 10, 6 };
        break;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        block_size = { 10, 8 };
        break;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        block_size = { 10, 10 };
        break;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        block_size = { 12, 10 };
        break;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        block_size = { 12, 12 };
        break;
    default:
        break;
    }
    return block_size;
}

// Return format class of the specified format
VK_LAYER_EXPORT VkFormatCompatibilityClass vk_format_get_compatibility_class(VkFormat format) {
    auto item = vk_format_table.find(format);
    if (item != vk_format_table.end()) {
        return item->second.format_class;
    }
    return VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT;
}

// Return size, in bytes, of a pixel of the specified format
VK_LAYER_EXPORT size_t vk_format_get_size(VkFormat format) {
    auto item = vk_format_table.find(format);
    if (item != vk_format_table.end()) {
        return item->second.size;
    }
    return 0;
}

// Return the number of channels for a given format
unsigned int vk_format_get_channel_count(VkFormat format) {
    auto item = vk_format_table.find(format);
    if (item != vk_format_table.end()) {
        return item->second.channel_count;
    }
    return 0;
}

// Perform a zero-tolerant modulo operation
VK_LAYER_EXPORT VkDeviceSize vk_safe_modulo(VkDeviceSize dividend, VkDeviceSize divisor) {
    VkDeviceSize result = 0;
    if (divisor != 0) {
        result = dividend % divisor;
    }
    return result;
}

static const uint8_t UTF8_ONE_BYTE_CODE = 0xC0;
static const uint8_t UTF8_ONE_BYTE_MASK = 0xE0;
static const uint8_t UTF8_TWO_BYTE_CODE = 0xE0;
static const uint8_t UTF8_TWO_BYTE_MASK = 0xF0;
static const uint8_t UTF8_THREE_BYTE_CODE = 0xF0;
static const uint8_t UTF8_THREE_BYTE_MASK = 0xF8;
static const uint8_t UTF8_DATA_BYTE_CODE = 0x80;
static const uint8_t UTF8_DATA_BYTE_MASK = 0xC0;

VK_LAYER_EXPORT VkStringErrorFlags vk_string_validate(const int max_length, const char *utf8) {
    VkStringErrorFlags result = VK_STRING_ERROR_NONE;
    int num_char_bytes = 0;
    int i, j;

    for (i = 0; i < max_length; i++) {
        if (utf8[i] == 0) {
            break;
        } else if ((utf8[i] >= 0xa) && (utf8[i] < 0x7f)) {
            num_char_bytes = 0;
        } else if ((utf8[i] & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_CODE) {
            num_char_bytes = 1;
        } else if ((utf8[i] & UTF8_TWO_BYTE_MASK) == UTF8_TWO_BYTE_CODE) {
            num_char_bytes = 2;
        } else if ((utf8[i] & UTF8_THREE_BYTE_MASK) == UTF8_THREE_BYTE_CODE) {
            num_char_bytes = 3;
        } else {
            result = VK_STRING_ERROR_BAD_DATA;
        }

        // Validate the following num_char_bytes of data
        for (j = 0; (j < num_char_bytes) && (i < max_length); j++) {
            if (++i == max_length) {
                result |= VK_STRING_ERROR_LENGTH;
                break;
            }
            if ((utf8[i] & UTF8_DATA_BYTE_MASK) != UTF8_DATA_BYTE_CODE) {
                result |= VK_STRING_ERROR_BAD_DATA;
            }
        }
    }
    return result;
}

// Utility function for finding a text string in another string
VK_LAYER_EXPORT bool white_list(const char *item, const char *list) {
    std::string candidate(item);
    std::string white_list(list);
    return (white_list.find(candidate) != std::string::npos);
}

// Debug callbacks get created in three ways:
//   o  Application-defined debug callbacks
//   o  Through settings in a vk_layer_settings.txt file
//   o  By default, if neither an app-defined debug callback nor a vk_layer_settings.txt file is present
//
// At layer initialization time, default logging callbacks are created to output layer error messages.
// If a vk_layer_settings.txt file is present its settings will override any default settings.
//
// If a vk_layer_settings.txt file is present and an application defines a debug callback, both callbacks
// will be active.  If no vk_layer_settings.txt file is present, creating an application-defined debug
// callback will cause the default callbacks to be unregisterd and removed.
VK_LAYER_EXPORT void layer_debug_actions(debug_report_data *report_data, std::vector<VkDebugReportCallbackEXT> &logging_callback,
                                         const VkAllocationCallbacks *pAllocator, const char *layer_identifier) {

    VkDebugReportCallbackEXT callback = VK_NULL_HANDLE;

    std::string report_flags_key = layer_identifier;
    std::string debug_action_key = layer_identifier;
    std::string log_filename_key = layer_identifier;
    report_flags_key.append(".report_flags");
    debug_action_key.append(".debug_action");
    log_filename_key.append(".log_filename");

    // Initialize layer options
    VkDebugReportFlagsEXT report_flags = GetLayerOptionFlags(report_flags_key, report_flags_option_definitions, 0);
    VkLayerDbgActionFlags debug_action = GetLayerOptionFlags(debug_action_key, debug_actions_option_definitions, 0);
    // Flag as default if these settings are not from a vk_layer_settings.txt file
    bool default_layer_callback = (debug_action & VK_DBG_LAYER_ACTION_DEFAULT) ? true : false;

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG) {
        const char *log_filename = getLayerOption(log_filename_key.c_str());
        FILE *log_output = getLayerLogOutput(log_filename, layer_identifier);
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = log_callback;
        dbgCreateInfo.pUserData = (void *)log_output;
        layer_create_msg_callback(report_data, default_layer_callback, &dbgCreateInfo, pAllocator, &callback);
        logging_callback.push_back(callback);
    }

    callback = VK_NULL_HANDLE;

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = win32_debug_output_msg;
        dbgCreateInfo.pUserData = NULL;
        layer_create_msg_callback(report_data, default_layer_callback, &dbgCreateInfo, pAllocator, &callback);
        logging_callback.push_back(callback);
    }
}
