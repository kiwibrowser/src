// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef PUBLIC_FPDF_TRANSFORMPAGE_H_
#define PUBLIC_FPDF_TRANSFORMPAGE_H_

// NOLINTNEXTLINE(build/include)
#include "fpdfview.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* FPDF_PAGEARCSAVER;
typedef void* FPDF_PAGEARCLOADER;

/**
*  Set "MediaBox" entry to the page dictionary.
* @param[in] page   - Handle to a page.
* @param[in] left   - The left of the rectangle.
* @param[in] bottom - The bottom of the rectangle.
* @param[in] right  - The right of the rectangle.
* @param[in] top    - The top of the rectangle.
* @retval None.
*/
FPDF_EXPORT void FPDF_CALLCONV FPDFPage_SetMediaBox(FPDF_PAGE page,
                                                    float left,
                                                    float bottom,
                                                    float right,
                                                    float top);

/**
*  Set "CropBox" entry to the page dictionary.
* @param[in] page   - Handle to a page.
* @param[in] left   - The left of the rectangle.
* @param[in] bottom - The bottom of the rectangle.
* @param[in] right  - The right of the rectangle.
* @param[in] top    - The top of the rectangle.
* @retval None.
*/
FPDF_EXPORT void FPDF_CALLCONV FPDFPage_SetCropBox(FPDF_PAGE page,
                                                   float left,
                                                   float bottom,
                                                   float right,
                                                   float top);

/**  Get "MediaBox" entry from the page dictionary.
* @param[in] page   - Handle to a page.
* @param[in] left   - Pointer to a double value receiving the left of the
* rectangle.
* @param[in] bottom - Pointer to a double value receiving the bottom of the
* rectangle.
* @param[in] right  - Pointer to a double value receiving the right of the
* rectangle.
* @param[in] top    - Pointer to a double value receiving the top of the
* rectangle.
* @retval True if success,else fail.
*/
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV FPDFPage_GetMediaBox(FPDF_PAGE page,
                                                         float* left,
                                                         float* bottom,
                                                         float* right,
                                                         float* top);

/**  Get "CropBox" entry from the page dictionary.
* @param[in] page   - Handle to a page.
* @param[in] left   - Pointer to a double value receiving the left of the
* rectangle.
* @param[in] bottom - Pointer to a double value receiving the bottom of the
* rectangle.
* @param[in] right  - Pointer to a double value receiving the right of the
* rectangle.
* @param[in] top    - Pointer to a double value receiving the top of the
* rectangle.
* @retval True if success,else fail.
*/
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV FPDFPage_GetCropBox(FPDF_PAGE page,
                                                        float* left,
                                                        float* bottom,
                                                        float* right,
                                                        float* top);

/**
 * Apply transforms to |page|.
 *
 * If |matrix| is provided it will be applied to transform the page.
 * If |clipRect| is provided it will be used to clip the resulting page.
 * If neither |matrix| or |clipRect| are provided this method returns |false|.
 * Returns |true| if transforms are applied.
 *
 * @param[in] page        - Page handle.
 * @param[in] matrix      - Transform matrix.
 * @param[in] clipRect    - Clipping rectangle.
 * @Note. This function will transform the whole page, and would take effect to
 * all the objects in the page.
 */
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFPage_TransFormWithClip(FPDF_PAGE page,
                           FS_MATRIX* matrix,
                           FS_RECTF* clipRect);

/**
* Transform (scale, rotate, shear, move) the clip path of page object.
* @param[in] page_object - Handle to a page object. Returned by
* FPDFPageObj_NewImageObj.
* @param[in] a  - The coefficient "a" of the matrix.
* @param[in] b  - The coefficient "b" of the matrix.
* @param[in] c  - The coefficient "c" of the matrix.
* @param[in] d  - The coefficient "d" of the matrix.
* @param[in] e  - The coefficient "e" of the matrix.
* @param[in] f  - The coefficient "f" of the matrix.
* @retval None.
*/
FPDF_EXPORT void FPDF_CALLCONV
FPDFPageObj_TransformClipPath(FPDF_PAGEOBJECT page_object,
                              double a,
                              double b,
                              double c,
                              double d,
                              double e,
                              double f);

/**
* Create a new clip path, with a rectangle inserted.
*
* @param[in] left   - The left of the clip box.
* @param[in] bottom - The bottom of the clip box.
* @param[in] right  - The right of the clip box.
* @param[in] top    - The top of the clip box.
* @retval a handle to the clip path.
*/
FPDF_EXPORT FPDF_CLIPPATH FPDF_CALLCONV FPDF_CreateClipPath(float left,
                                                            float bottom,
                                                            float right,
                                                            float top);

/**
* Destroy the clip path.
*
* @param[in] clipPath - A handle to the clip path.
* Destroy the clip path.
* @retval None.
*/
FPDF_EXPORT void FPDF_CALLCONV FPDF_DestroyClipPath(FPDF_CLIPPATH clipPath);

/**
* Clip the page content, the page content that outside the clipping region
* become invisible.
*
* @param[in] page        - A page handle.
* @param[in] clipPath    - A handle to the clip path.
* @Note. A clip path will be inserted before the page content stream or content
* array. In this way, the page content will be clipped
* by this clip path.
*/
FPDF_EXPORT void FPDF_CALLCONV FPDFPage_InsertClipPath(FPDF_PAGE page,
                                                       FPDF_CLIPPATH clipPath);

#ifdef __cplusplus
}
#endif

#endif  // PUBLIC_FPDF_TRANSFORMPAGE_H_
