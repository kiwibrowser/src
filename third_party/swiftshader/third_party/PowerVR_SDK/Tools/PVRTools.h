/*!****************************************************************************

 @file         PVRTools.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Header file to include a particular API tools header

******************************************************************************/
#ifndef PVRTOOLS_H
#define PVRTOOLS_H

/*****************************************************************************/
/*! @mainpage PVRTools
******************************************************************************

 \tableofcontents 
 
 @section overview Overview
*****************************

PVRTools is a collection of source code to help developers with some common
tasks which are frequently used in 3D programming.
PVRTools supplies code for mathematical operations, matrix handling,
loading 3D models and to optimise geometry.
Sections which are specific to certain APIs contain code for displaying text and loading textures.


 @section fileformats File formats
*****************************
The following file formats are used in PVRTools:

 @subsection PFX_format PFX format
*****************************
PFX (PowerVR Effects) files are used to store graphics effects. As a minimum, a correctly formatted PFX consists of:
 \li One EFFECT block
 \li One VERTEXSHADER block
 \li One FRAGMENTSHADER block
 
It is also possible for PFXs to contain the following:
 \li One TARGET block
 \li Zero or more TEXTURE blocks

By default PFXs are stored in .pfx files. It is possible for multiple PFXs to exist within a single .pfx file, 
each described by a separate effect block; in this instance multiple PFXs may share blocks.
Finally, it is possible for a PFX to reference a TARGET block as an input as if it were a TEXTURE block, 
enabling the simple creation of complex post-processing effects.  For this to function correctly the TARGET 
block render should be completed prior to being read as an input.  If the TARGET block render has not been 
completed prior to being read as an input, the behaviour will vary based on the render target implementation of the platform.

For more information see the <em>PFX File Format Specification</em>.

 @subsection POD_format POD format
*****************************
POD files store data for representing 3D scenes; including geometry information, animations, matrices, materials, skinning data, lights, cameras, and in some instances custom meta-data.  
These files are output by the PVRGeoPOD tool, and are designed for deployment through optimistions such as triangle/vertex sorting and data stripping.

The format is designed to be easily read, for information on the required algorithm and the overall structure of the format see the <em>POD File Format Specification</em>. 

 @subsection PVR_format PVR format
*****************************
PVR files are used as a container to store texture data. PVR files can be exported from PVRTexTool and a number of third party applications.

For more information see the <em>PVR File Format Specification</em>.

 @section files Header files
*****************************

Here is a list of common header files present in PVRTools:

 \li PVRTArray.h: A dynamic, resizable template class.

 \li PVRTBackground.h: Create a textured background.

 \li PVRTBoneBatch.h: Group vertices per bones to allow skinning when the maximum number of bones is limited.

 \li PVRTDecompress.h: Descompress PVRTC texture format.

 \li PVRTError.h: Error codes and tools output debug.

 \li PVRTFixedPoint.h: Fast fixed point mathematical functions.
 
 \li PVRTGlobal.h: Global defines and typedefs.
 
 \li PVRTHash.h: A simple hash class which uses TEA to hash a string or given data into a 32-bit unsigned int.

 \li PVRTMap.h: A dynamic, expanding templated map class.

 \li PVRTMatrix.h: Vector and Matrix functions.
 
 \li PVRTMemoryFileSystem.h: Memory file system for resource files.

 \li PVRTMisc.h: Skybox, line plane intersection code, etc...

 \li PVRTModelPOD.h: Load geometry and animation from a POD file.

 \li PVRTPFXParser.h: Code to parse our PFX file format. Note, not used in fixed function APIs, such as @ref API_OGLES "OpenGL ES 1.x".

 \li PVRTPrint3D.h: Display text/logos on the screen.

 \li PVRTQuaternion.h: Quaternion functions.

 \li PVRTResourceFile.h: The tools code for loading files included using FileWrap.

 \li PVRTShadowVol.h: Tools code for creating shadow volumes.
 
 \li PVRTSkipGraph.h: A "tree-like" structure for storing data which, unlike a tree, can reference any other node.

 \li PVRTString.h: A string class.

 \li PVRTTexture.h: Load textures from resources, BMP or PVR files.

 \li PVRTTrans.h: Transformation and projection functions.

 \li PVRTTriStrip.h: Geometry optimization using strips.

 \li PVRTVector.h: Vector and Matrix functions that are gradually replacing PVRTMatrix.

 \li PVRTVertex.h: Vertex order optimisation for 3D acceleration.

 @section APIs APIs
*****************************
 For information specific to each 3D API, see the list of supported APIs on the <a href="modules.html">Modules</a> page.
 
*/

#if defined(BUILD_OGLES3)
	#include "OGLES3Tools.h"
#elif defined(BUILD_OGLES2)
	#include "OGLES2Tools.h"
#elif defined(BUILD_OGLES)
	#include "OGLESTools.h"
#elif defined(BUILD_OGL)
	#include "OGLTools.h"
#elif defined(BUILD_DX11)
	#include "DX11Tools.h"
#endif

#endif /* PVRTOOLS_H*/

/*****************************************************************************
 End of file (Tools.h)
*****************************************************************************/

