/******************************************************************************

 @File         OGLES2DisplacementMap.cpp

 @Title        Displacement Map

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independent

 @Description  Shows how to displace geometry in the vertex shader using a
               texture.

******************************************************************************/
#include <string.h>

#include "PVRShell.h"
#include "OGLES2Tools.h"

/******************************************************************************
 Defines
******************************************************************************/
// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0
#define NORMAL_ARRAY	1
#define TEXCOORD_ARRAY	2

/******************************************************************************
 Consts
******************************************************************************/
// Camera constants. Used for making the projection matrix
const float g_fCameraNear = 4.0f;
const float g_fCameraFar  = 2000.0f;

const float g_fDemoFrameRate = 1.0f / 90.0f;

// The camera to use from the pod file
const int g_ui32Camera = 0;

/******************************************************************************
 Content file names
******************************************************************************/

// Source and binary shaders
const char c_szFragShaderSrcFile[]	= "FragShader.fsh";
const char c_szFragShaderBinFile[]	= "FragShader.fsc";
const char c_szVertShaderSrcFile[]	= "VertShader.vsh";
const char c_szVertShaderBinFile[]	= "VertShader.vsc";

// POD scene files
const char c_szSceneFile[]			= "DisMapScene.pod";
const char c_szDisMapFile[]			= "DisMap.pvr";

/*!****************************************************************************
 Class implementing the PVRShell functions.
******************************************************************************/
class OGLES2DisplacementMap : public PVRShell
{
	// Print3D class used to display text
	CPVRTPrint3D	m_Print3D;

	// 3D Model
	CPVRTModelPOD	m_Scene;

	// OpenGL handles for shaders, textures and VBOs
	GLuint m_uiVertShader;
	GLuint m_uiFragShader;
	GLuint* m_puiVbo;
	GLuint* m_puiIndexVbo;
	GLuint* m_puiTextureIDs;
	GLuint  m_uiDisMapID;

	// Group shader programs and their uniform locations together
	struct
	{
		GLuint uiId;
		GLuint uiMVPMatrixLoc;
		GLuint uiLightDirLoc;
		GLuint uiTexture;
		GLuint uiDisMap;
		GLuint uiDisplacementFactor;
	}
	m_ShaderProgram;

	// Variables to handle the animation in a time-based manner
	unsigned long		m_ulTimePrev;

	// App variables
	PVRTVec4		m_LightDir;
	PVRTMat4		m_View, m_Projection;
	float			m_DisplacementFactor;
	bool			m_bGrow;

public:
	virtual bool InitApplication();
	virtual bool InitView();
	virtual bool ReleaseView();
	virtual bool QuitApplication();
	virtual bool RenderScene();

	OGLES2DisplacementMap();
	bool LoadTextures(CPVRTString* pErrorStr);
	bool LoadShaders(CPVRTString* pErrorStr);
	bool LoadVbos(CPVRTString* pErrorStr);

	void DrawMesh(int i32NodeIndex);
};

/*!****************************************************************************
 @Function		OGLES2DisplacementMap
 @Description	Constructor
******************************************************************************/
OGLES2DisplacementMap::OGLES2DisplacementMap() :    m_puiVbo(0),
													m_puiIndexVbo(0),
													m_puiTextureIDs(0),
													m_ulTimePrev(0),
													m_DisplacementFactor(0),
													m_bGrow(false)
{
}

/*!****************************************************************************
 @Function		LoadTextures
 @Return		bool			true if no error occurred
 @Description	Loads the textures required for this training course
******************************************************************************/
bool OGLES2DisplacementMap::LoadTextures(CPVRTString* pErrorStr)
{
	/*
		Load the textures.
		For a more detailed explanation, see Texturing and IntroducingPVRTools
	*/

	/*
		Initialises an array to lookup the textures
		for each material in the scene.
	*/
	m_puiTextureIDs = new GLuint[m_Scene.nNumMaterial];

	if(!m_puiTextureIDs)
	{
		*pErrorStr = "ERROR: Insufficient memory.";
		return false;
	}

	for(int i = 0; i < (int) m_Scene.nNumMaterial; ++i)
	{
		m_puiTextureIDs[i] = 0;
		SPODMaterial* pMaterial = &m_Scene.pMaterial[i];

		if(pMaterial->nIdxTexDiffuse != -1)
		{
			/*
				Using the tools function PVRTTextureLoadFromPVR load the textures required by the pod file.

				Note: This function only loads .pvr files. You can set the textures in 3D Studio Max to .pvr
				files using the PVRTexTool plug-in for max. Alternatively, the pod material properties can be
				modified in PVRShaman.
			*/

			CPVRTString sTextureName = m_Scene.pTexture[pMaterial->nIdxTexDiffuse].pszName;

			if(PVRTTextureLoadFromPVR(sTextureName.c_str(), &m_puiTextureIDs[i]) != PVR_SUCCESS)
			{
				*pErrorStr = "ERROR: Failed to load " + sTextureName + ".";

				// Check to see if we're trying to load .pvr or not
				CPVRTString sFileExtension = PVRTStringGetFileExtension(sTextureName);

				if(sFileExtension.toLower() == "pvr")
					*pErrorStr += "Note: Can only load pvr files.";

				return false;
			}
		}
	}

	// Load the texture used for the displacement map
	if(PVRTTextureLoadFromPVR(c_szDisMapFile, &m_uiDisMapID) != PVR_SUCCESS)
	{
		*pErrorStr = "ERROR: Failed to load " + CPVRTString(c_szDisMapFile) + ".";
		return false;
	}

	// Define the wrapping to use for the displacement map
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return true;
}

/*!****************************************************************************
 @Function		LoadShaders
 @Output		pErrorStr		A string describing the error on failure
 @Return		bool			true if no error occurred
 @Description	Loads and compiles the shaders and links the shader programs
				required for this training course
******************************************************************************/
bool OGLES2DisplacementMap::LoadShaders(CPVRTString* pErrorStr)
{
	/*
		Load and compile the shaders from files.
		Binary shaders are tried first, source shaders
		are used as fallback.
	*/
	if(PVRTShaderLoadFromFile(
			c_szVertShaderBinFile, c_szVertShaderSrcFile, GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_uiVertShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if (PVRTShaderLoadFromFile(
			c_szFragShaderBinFile, c_szFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiFragShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	/*
		Set up and link the shader program
	*/
	const char* aszAttribs[] = { "inVertex", "inNormal", "inTexCoord" };

	if(PVRTCreateProgram(
			&m_ShaderProgram.uiId, m_uiVertShader, m_uiFragShader, aszAttribs, 3, pErrorStr) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, pErrorStr->c_str());
		return false;
	}

	// Store the location of uniforms for later use
	m_ShaderProgram.uiMVPMatrixLoc	= glGetUniformLocation(m_ShaderProgram.uiId, "MVPMatrix");
	m_ShaderProgram.uiLightDirLoc	= glGetUniformLocation(m_ShaderProgram.uiId, "LightDirection");
	m_ShaderProgram.uiDisplacementFactor = glGetUniformLocation(m_ShaderProgram.uiId, "DisplacementFactor");

	m_ShaderProgram.uiTexture = glGetUniformLocation(m_ShaderProgram.uiId, "sTexture");
	m_ShaderProgram.uiDisMap  = glGetUniformLocation(m_ShaderProgram.uiId, "sDisMap");

	return true;
}

/*!****************************************************************************
 @Function		LoadVbos
 @Description	Loads the mesh data required for this training course into
				vertex buffer objects
******************************************************************************/
bool OGLES2DisplacementMap::LoadVbos(CPVRTString* pErrorStr)
{
	if(!m_Scene.pMesh[0].pInterleaved)
	{
		*pErrorStr = "ERROR: IntroducingPOD requires the pod data to be interleaved. Please re-export with the interleaved option enabled.";
		return false;
	}

	if (!m_puiVbo)      m_puiVbo = new GLuint[m_Scene.nNumMesh];
	if (!m_puiIndexVbo) m_puiIndexVbo = new GLuint[m_Scene.nNumMesh];

	/*
		Load vertex data of all meshes in the scene into VBOs

		The meshes have been exported with the "Interleave Vectors" option,
		so all data is interleaved in the buffer at pMesh->pInterleaved.
		Interleaving data improves the memory access pattern and cache efficiency,
		thus it can be read faster by the hardware.
	*/
	glGenBuffers(m_Scene.nNumMesh, m_puiVbo);
	for (unsigned int i = 0; i < m_Scene.nNumMesh; ++i)
	{
		// Load vertex data into buffer object
		SPODMesh& Mesh = m_Scene.pMesh[i];
		unsigned int uiSize = Mesh.nNumVertex * Mesh.sVertex.nStride;
		glBindBuffer(GL_ARRAY_BUFFER, m_puiVbo[i]);
		glBufferData(GL_ARRAY_BUFFER, uiSize, Mesh.pInterleaved, GL_STATIC_DRAW);

		// Load index data into buffer object if available
		m_puiIndexVbo[i] = 0;
		if (Mesh.sFaces.pData)
		{
			glGenBuffers(1, &m_puiIndexVbo[i]);
			uiSize = PVRTModelPODCountIndices(Mesh) * sizeof(GLshort);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiIndexVbo[i]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiSize, Mesh.sFaces.pData, GL_STATIC_DRAW);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return true;
}

/*!****************************************************************************
 @Function		InitApplication
 @Return		bool		true if no error occurred
 @Description	Code in InitApplication() will be called by PVRShell once per
				run, before the rendering context is created.
				Used to initialize variables that are not dependent on it
				(e.g. external modules, loading meshes, etc.)
				If the rendering context is lost, InitApplication() will
				not be called again.
******************************************************************************/
bool OGLES2DisplacementMap::InitApplication()
{
	// Get and set the read path for content files
	CPVRTResourceFile::SetReadPath((char*)PVRShellGet(prefReadPath));

	// Get and set the load/release functions for loading external files.
	// In the majority of cases the PVRShell will return NULL function pointers implying that
	// nothing special is required to load external files.
	CPVRTResourceFile::SetLoadReleaseFunctions(PVRShellGet(prefLoadFileFunc), PVRShellGet(prefReleaseFileFunc));

	// Load the scene
	if(m_Scene.ReadFromFile(c_szSceneFile) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Couldn't load the .pod file\n");
		return false;
	}

	// The cameras are stored in the file. We check it contains at least one.
	if(m_Scene.nNumCamera == 0)
	{
		PVRShellSet(prefExitMessage, "ERROR: The scene does not contain a camera. Please add one and re-export.\n");
		return false;
	}

	// We also check that the scene contains at least one light
	if(m_Scene.nNumLight == 0)
	{
		PVRShellSet(prefExitMessage, "ERROR: The scene does not contain a light. Please add one and re-export.\n");
		return false;
	}

	return true;
}

/*!****************************************************************************
 @Function		QuitApplication
 @Return		bool		true if no error occurred
 @Description	Code in QuitApplication() will be called by PVRShell once per
				run, just before exiting the program.
				If the rendering context is lost, QuitApplication() will
				not be called.
******************************************************************************/
bool OGLES2DisplacementMap::QuitApplication()
{
	// Free the memory allocated for the scene
	m_Scene.Destroy();

	delete[] m_puiVbo;
	delete[] m_puiIndexVbo;

    return true;
}

/*!****************************************************************************
 @Function		InitView
 @Return		bool		true if no error occurred
 @Description	Code in InitView() will be called by PVRShell upon
				initialization or after a change in the rendering context.
				Used to initialize variables that are dependent on the rendering
				context (e.g. textures, vertex buffers, etc.)
******************************************************************************/
bool OGLES2DisplacementMap::InitView()
{
	CPVRTString ErrorStr;

	/*
		Initialize VBO data
	*/
	if(!LoadVbos(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	/*
		Load textures
	*/
	if(!LoadTextures(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	/*
		Load and compile the shaders & link programs
	*/
	if(!LoadShaders(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	/*
		Initialize Print3D
	*/
	bool bRotate = PVRShellGet(prefIsRotated) && PVRShellGet(prefFullScreen);

	if(m_Print3D.SetTextures(0,PVRShellGet(prefWidth),PVRShellGet(prefHeight), bRotate) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Cannot initialise Print3D\n");
		return false;
	}

	/*
		Set OpenGL ES render states needed for this training course
	*/
	// Enable backface culling and depth test
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);

	// Use a nice bright blue as clear colour
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	//Get the direction of the first light from the scene.
	m_LightDir = m_Scene.GetLightDirection(0);

	// For direction vectors, w should be 0
	m_LightDir.w = 0.0f;


	//	Set up the view and projection matrices from the camera
	PVRTVec3	vFrom, vTo(0.0f), vUp(0.0f, 1.0f, 0.0f);
	float fFOV;

	// Setup the camera

	// Camera nodes are after the mesh and light nodes in the array
	int i32CamID = m_Scene.pNode[m_Scene.nNumMeshNode + m_Scene.nNumLight + g_ui32Camera].nIdx;

	// Get the camera position, target and field of view (fov)
	if(m_Scene.pCamera[i32CamID].nIdxTarget != -1) // Does the camera have a target?
		fFOV = m_Scene.GetCameraPos( vFrom, vTo, g_ui32Camera); // vTo is taken from the target node
	else
		fFOV = m_Scene.GetCamera( vFrom, vTo, vUp, g_ui32Camera); // vTo is calculated from the rotation

	// We can build the model view matrix from the camera position, target and an up vector.
	// For this we usePVRTMat4LookAtRH()
	m_View = PVRTMat4::LookAtRH(vFrom, vTo, vUp);

	// Calculate the projection matrix
	m_Projection = PVRTMat4::PerspectiveFovRH(fFOV, (float)PVRShellGet(prefWidth)/(float)PVRShellGet(prefHeight), g_fCameraNear, g_fCameraFar, PVRTMat4::OGL, bRotate);

	// Initialize variables used for the animation
	m_ulTimePrev = PVRShellGetTime();

	return true;
}

/*!****************************************************************************
 @Function		ReleaseView
 @Return		bool		true if no error occurred
 @Description	Code in ReleaseView() will be called by PVRShell when the
				application quits or before a change in the rendering context.
******************************************************************************/
bool OGLES2DisplacementMap::ReleaseView()
{
	// Deletes the textures
	glDeleteTextures(m_Scene.nNumMaterial, &m_puiTextureIDs[0]);
	glDeleteTextures(1, &m_uiDisMapID);

	// Frees the texture lookup array
	delete[] m_puiTextureIDs;
	m_puiTextureIDs = 0;

	// Delete program and shader objects
	glDeleteProgram(m_ShaderProgram.uiId);

	glDeleteShader(m_uiVertShader);
	glDeleteShader(m_uiFragShader);

	// Delete buffer objects
	glDeleteBuffers(m_Scene.nNumMesh, m_puiVbo);
	glDeleteBuffers(m_Scene.nNumMesh, m_puiIndexVbo);

	// Release Print3D Textures
	m_Print3D.ReleaseTextures();

	return true;
}

/*!****************************************************************************
 @Function		RenderScene
 @Return		bool		true if no error occurred
 @Description	Main rendering loop function of the program. The shell will
				call this function every frame.
				eglSwapBuffers() will be performed by PVRShell automatically.
				PVRShell will also manage important OS events.
				Will also manage relevant OS events. The user has access to
				these events through an abstraction layer provided by PVRShell.
******************************************************************************/
bool OGLES2DisplacementMap::RenderScene()
{
	// Clear the color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//	Calculates the the time since the last frame
	unsigned long ulTime = PVRShellGetTime();
	unsigned long ulDeltaTime = ulTime - m_ulTimePrev;
	m_ulTimePrev = ulTime;

	// Use shader program
	glUseProgram(m_ShaderProgram.uiId);

	// Enable 2D texturing for the first texture.
	glActiveTexture(GL_TEXTURE0);

	// Set the sampler2D variable to the first texture unit
	glUniform1i(m_ShaderProgram.uiTexture, 0);

	// Enable 2D texturing for the second texture.
	glActiveTexture(GL_TEXTURE1);

	// Set the displacement map variable to the second texture unit
	glUniform1i(m_ShaderProgram.uiDisMap, 1);

	// Calculate and set the displacement factor
	if(m_bGrow)
	{
		m_DisplacementFactor += (float)ulDeltaTime * g_fDemoFrameRate;

		if(m_DisplacementFactor > 25.0f)
		{
			m_bGrow = false;
			m_DisplacementFactor = 25.0f;
		}
	}
	else
	{
		m_DisplacementFactor -= (float)ulDeltaTime * g_fDemoFrameRate;

		if(m_DisplacementFactor < 0.0f)
		{
			m_bGrow = true;
			m_DisplacementFactor = 0.0f;
		}
	}

	glUniform1f(m_ShaderProgram.uiDisplacementFactor, m_DisplacementFactor);

	// Bind the displacement map texture
	glBindTexture(GL_TEXTURE_2D, m_uiDisMapID);

	// Now the displacement map texture is bound set the active texture to texture 0
	glActiveTexture(GL_TEXTURE0);

	// Draw the scene

	// Enable the vertex attribute arrays
	glEnableVertexAttribArray(VERTEX_ARRAY);
	glEnableVertexAttribArray(NORMAL_ARRAY);
	glEnableVertexAttribArray(TEXCOORD_ARRAY);

	for(unsigned int i = 0; i < m_Scene.nNumMeshNode; ++i)
	{
		SPODNode& Node = m_Scene.pNode[i];

		// Get the node model matrix
		PVRTMat4 mWorld;
		mWorld = m_Scene.GetWorldMatrix(Node);

		// Pass the model-view-projection matrix (MVP) to the shader to transform the vertices
		PVRTMat4 mModelView, mMVP;
		mModelView = m_View * mWorld;
		mMVP = m_Projection * mModelView;
		glUniformMatrix4fv(m_ShaderProgram.uiMVPMatrixLoc, 1, GL_FALSE, mMVP.f);

		// Pass the light direction in model space to the shader
		PVRTVec4 vLightDir;
		vLightDir = mWorld.inverse() * m_LightDir;

		PVRTVec3 vLightDirModel = *(PVRTVec3*) vLightDir.ptr();
		vLightDirModel.normalize();

		glUniform3fv(m_ShaderProgram.uiLightDirLoc, 1, &vLightDirModel.x);

		// Load the correct texture for the mesh using our texture lookup table
		GLuint uiTex = 0;

		if(Node.nIdxMaterial != -1)
			uiTex = m_puiTextureIDs[Node.nIdxMaterial];

		glBindTexture(GL_TEXTURE_2D, uiTex);

		/*
			Now that the model-view matrix is set and the materials ready,
			call another function to actually draw the mesh.
		*/
		DrawMesh(i);
	}

	// Safely disable the vertex attribute arrays
	glDisableVertexAttribArray(VERTEX_ARRAY);
	glDisableVertexAttribArray(NORMAL_ARRAY);
	glDisableVertexAttribArray(TEXCOORD_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Display the demo name using the tools. For a detailed explanation, see the training course IntroducingPVRTools
	m_Print3D.DisplayDefaultTitle("DisplacementMapping", "", ePVRTPrint3DSDKLogo);
	m_Print3D.Flush();

	return true;
}

/*!****************************************************************************
 @Function		DrawMesh
 @Input			i32NodeIndex		Node index of the mesh to draw
 @Description	Draws a SPODMesh after the model view matrix has been set and
				the material prepared.
******************************************************************************/
void OGLES2DisplacementMap::DrawMesh(int i32NodeIndex)
{
	int i32MeshIndex = m_Scene.pNode[i32NodeIndex].nIdx;
	SPODMesh* pMesh = &m_Scene.pMesh[i32MeshIndex];

	// bind the VBO for the mesh
	glBindBuffer(GL_ARRAY_BUFFER, m_puiVbo[i32MeshIndex]);
	// bind the index buffer, won't hurt if the handle is 0
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiIndexVbo[i32MeshIndex]);

	// Set the vertex attribute offsets
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sVertex.nStride, pMesh->sVertex.pData);
	glVertexAttribPointer(NORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sNormals.nStride, pMesh->sNormals.pData);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, pMesh->psUVW[0].nStride, pMesh->psUVW[0].pData);

	/*
		The geometry can be exported in 4 ways:
		- Indexed Triangle list
		- Non-Indexed Triangle list
		- Indexed Triangle strips
		- Non-Indexed Triangle strips
	*/
	if(pMesh->nNumStrips == 0)
	{
		if(m_puiIndexVbo[i32MeshIndex])
		{
			// Indexed Triangle list
			glDrawElements(GL_TRIANGLES, pMesh->nNumFaces*3, GL_UNSIGNED_SHORT, 0);
		}
		else
		{
			// Non-Indexed Triangle list
			glDrawArrays(GL_TRIANGLES, 0, pMesh->nNumFaces*3);
		}
	}
	else
	{
		int offset = 0;

		for(int i = 0; i < (int)pMesh->nNumStrips; ++i)
		{
			if(m_puiIndexVbo[i32MeshIndex])
			{
				// Indexed Triangle strips
				glDrawElements(GL_TRIANGLE_STRIP, pMesh->pnStripLength[i]+2, GL_UNSIGNED_SHORT, &((GLshort*)0)[offset]);
			}
			else
			{
				// Non-Indexed Triangle strips
				glDrawArrays(GL_TRIANGLE_STRIP, offset, pMesh->pnStripLength[i]+2);
			}
			offset += pMesh->pnStripLength[i]+2;
		}
	}
}

/*!****************************************************************************
 @Function		NewDemo
 @Return		PVRShell*		The demo supplied by the user
 @Description	This function must be implemented by the user of the shell.
				The user should return its PVRShell object defining the
				behaviour of the application.
******************************************************************************/
PVRShell* NewDemo()
{
	return new OGLES2DisplacementMap();
}

/******************************************************************************
 End of file (OGLES2DisplacementMap.cpp)
******************************************************************************/

