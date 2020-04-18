/******************************************************************************

 @File         OGLES2ChameleonMan.cpp

 @Title        OGLES2ChameleonMan

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independent

 @Description  Shows how to perform skinning combined with Dot3 lighting

******************************************************************************/
#include "PVRShell.h"
#include "OGLES2Tools.h"

/******************************************************************************
 Constants
******************************************************************************/

// Camera constants used to generate the projection matrix
const float g_fCameraNear	= 4.0f;
const float g_fCameraFar	= 30000.0f;

const float g_fDemoFrameRate = 0.02f;

/******************************************************************************
 shader attributes
******************************************************************************/

// Skinned

// vertex attributes
enum EVertexAttrib {
	VERTEX_ARRAY, NORMAL_ARRAY, TANGENT_ARRAY, BINORMAL_ARRAY, TEXCOORD_ARRAY, BONEWEIGHT_ARRAY, BONEINDEX_ARRAY, eNumAttribs };
const char* g_aszAttribNames[] = {
	"inVertex", "inNormal", "inTangent", "inBiNormal", "inTexCoord", "inBoneWeight", "inBoneIndex" };

// shader uniforms
enum ESkinnnedUniform {
	eViewProj, eLightPos, eBoneCount, eBoneMatrices, eBoneMatricesIT, ebUseDot3, eNumSkinnedUniforms };
const char* g_aszSkinnedUniformNames[] = {
	"ViewProjMatrix", "LightPos", "BoneCount", "BoneMatrixArray[0]", "BoneMatrixArrayIT[0]", "bUseDot3" };

// Default

// vertex attributes
enum EDefaultVertexAttrib {
	DEFAULT_VERTEX_ARRAY, DEFAULT_TEXCOORD_ARRAY, eNumDefaultAttribs };
const char* g_aszDefaultAttribNames[] = {
	"inVertex", "inTexCoord"};

// shader uniforms
enum EDefaultUniform {
	eDefaultMVPMatrix, eDefaultUOffset, eNumDefaultUniforms };
const char* g_aszDefaultUniformNames[] = {
	"MVPMatrix", "fUOffset" };

/******************************************************************************
 Content file names
******************************************************************************/

// Source and binary shaders
const char c_szSkinnedFragShaderSrcFile[]	= "SkinnedFragShader.fsh";
const char c_szSkinnedFragShaderBinFile[]	= "SkinnedFragShader.fsc";
const char c_szSkinnedVertShaderSrcFile[]	= "SkinnedVertShader.vsh";
const char c_szSkinnedVertShaderBinFile[]	= "SkinnedVertShader.vsc";
const char c_szDefaultFragShaderSrcFile[]	= "DefaultFragShader.fsh";
const char c_szDefaultFragShaderBinFile[]	= "DefaultFragShader.fsc";
const char c_szDefaultVertShaderSrcFile[]	= "DefaultVertShader.vsh";
const char c_szDefaultVertShaderBinFile[]	= "DefaultVertShader.vsc";

// Base Textures
const char c_szFinalChameleonManHeadBodyTexFile[]	= "FinalChameleonManHeadBody.pvr";
const char c_szFinalChameleonManLegsTexFile[]		= "FinalChameleonManLegs.pvr";
const char c_szLampTexFile[]						= "lamp.pvr";
const char c_szChameleonBeltTexFile[]				= "ChameleonBelt.pvr";

const char c_szSkylineTexFile[]						= "skyline.pvr";
const char c_szWallDiffuseBakedTexFile[]			= "Wall_diffuse_baked.pvr";

// Tangent Space BumpMap Textures
const char c_szTang_space_BodyMapTexFile[]			= "Tang_space_BodyMap.pvr";
const char c_szTang_space_LegsMapTexFile[]			= "Tang_space_LegsMap.pvr";
const char c_szTang_space_BeltMapTexFile[]			= "Tang_space_BeltMap.pvr";

// POD scene files
const char c_szSceneFile[] = "ChameleonScene.pod";

/****************************************************************************
 ** Enums                                                                 **
 ****************************************************************************/
enum EMeshes
{
	eBody,
	eLegs,
	eBelt,
	eWall,
	eBackground,
	eLights
};

/****************************************************************************
** Structures
****************************************************************************/

/*!****************************************************************************
 Class implementing the PVRShell functions.
******************************************************************************/
class OGLES2ChameleonMan : public PVRShell
{
	// Print3D class used to display text
	CPVRTPrint3D	m_Print3D;

	// 3D Model
	CPVRTModelPOD	m_Scene;

	// Model transformation variables
	float	m_fWallPos;
	float	m_fBackgroundPos;
	float	m_fLightPos;

	// OpenGL handles for shaders and VBOs
	GLuint	m_uiSkinnedVertShader;
	GLuint	m_uiDefaultVertShader;
	GLuint	m_uiSkinnedFragShader;
	GLuint	m_uiDefaultFragShader;
	GLuint*	m_puiVbo;
	GLuint*	m_puiIndexVbo;

	// Texture IDs
	GLuint m_ui32TexHeadBody;
	GLuint m_ui32TexLegs;
	GLuint m_ui32TexBeltNormalMap;
	GLuint m_ui32TexHeadNormalMap;
	GLuint m_ui32TexLegsNormalMap;
	GLuint m_ui32TexSkyLine;
	GLuint m_ui32TexWall;
	GLuint m_ui32TexLamp;
	GLuint m_ui32TexBelt;

	// Group shader programs and their uniform locations together
	struct
	{
		GLuint uiId;
		GLuint auiLoc[eNumSkinnedUniforms];
	}
	m_SkinnedShaderProgram;

	struct
	{
		GLuint uiId;
		GLuint auiLoc[eNumDefaultUniforms];
	}
	m_DefaultShaderProgram;

	bool m_bEnableDOT3;

	// Variables to handle the animation in a time-based manner
	unsigned long m_iTimePrev;
	float	m_fFrame;

public:
	OGLES2ChameleonMan() :	m_fWallPos(0),
							m_fBackgroundPos(0),
							m_fLightPos(0),
							m_puiVbo(0),
							m_puiIndexVbo(0),
							m_bEnableDOT3(true),
							m_iTimePrev(0),
							m_fFrame(0)
	{
	}

	virtual bool InitApplication();
	virtual bool InitView();
	virtual bool ReleaseView();
	virtual bool QuitApplication();
	virtual bool RenderScene();

	bool LoadTextures(CPVRTString* pErrorStr);
	bool LoadShaders(CPVRTString* pErrorStr);
	void LoadVbos();

	void DrawSkinnedMesh(int i32NodeIndex);
};

/*!****************************************************************************
 @Function		LoadTextures
 @Output		pErrorStr		A string describing the error on failure
 @Return		bool			true if no error occured
 @Description	Loads the textures required for this training course
******************************************************************************/
bool OGLES2ChameleonMan::LoadTextures(CPVRTString* const pErrorStr)
{
	// Load Textures
	if(PVRTTextureLoadFromPVR(c_szFinalChameleonManHeadBodyTexFile, &m_ui32TexHeadBody) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load texture for Upper Body.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szFinalChameleonManLegsTexFile, &m_ui32TexLegs) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load texture for Legs.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szTang_space_BodyMapTexFile, &m_ui32TexHeadNormalMap) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load normalmap texture for Upper Body.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szTang_space_LegsMapTexFile, &m_ui32TexLegsNormalMap) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load normalmap texture for Legs.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szTang_space_BeltMapTexFile, &m_ui32TexBeltNormalMap) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load normalmap texture for Belt.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szSkylineTexFile, &m_ui32TexSkyLine) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load texture for SkyLine.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szWallDiffuseBakedTexFile, &m_ui32TexWall) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load texture for Wall.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szLampTexFile, &m_ui32TexLamp) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load texture for Lamps.\n");
		return false;
	}

	if(PVRTTextureLoadFromPVR(c_szChameleonBeltTexFile, &m_ui32TexBelt) != PVR_SUCCESS)
	{
        *pErrorStr = CPVRTString("ERROR: Failed to load texture for Belt.\n");
		return false;
	}

	return true;
}

/*!****************************************************************************
 @Function		LoadShaders
 @Output		pErrorStr		A string describing the error on failure
 @Return		bool			true if no error occured
 @Description	Loads and compiles the shaders and links the shader programs
				required for this training course
******************************************************************************/
bool OGLES2ChameleonMan::LoadShaders(CPVRTString* pErrorStr)
{
	int i;

	/*
		Load and compile the shaders from files.
		Binary shaders are tried first, source shaders
		are used as fallback.
	*/


	// Create the skinned program
	if(PVRTShaderLoadFromFile(
			c_szSkinnedVertShaderBinFile, c_szSkinnedVertShaderSrcFile, GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_uiSkinnedVertShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if(PVRTShaderLoadFromFile(
			c_szSkinnedFragShaderBinFile, c_szSkinnedFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiSkinnedFragShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if(PVRTCreateProgram(&m_SkinnedShaderProgram.uiId, m_uiSkinnedVertShader, m_uiSkinnedFragShader, g_aszAttribNames, eNumAttribs, pErrorStr) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, pErrorStr->c_str());
		return false;
	}

	// Store the location of uniforms for later use
	for(i = 0; i < eNumSkinnedUniforms; ++i)
	{
		m_SkinnedShaderProgram.auiLoc[i] = glGetUniformLocation(m_SkinnedShaderProgram.uiId, g_aszSkinnedUniformNames[i]);
	}

	glUniform1i(m_SkinnedShaderProgram.auiLoc[ebUseDot3], m_bEnableDOT3);

	// Set the sampler2D uniforms to corresponding texture units
	glUniform1i(glGetUniformLocation(m_SkinnedShaderProgram.uiId, "sTexture"), 0);
	glUniform1i(glGetUniformLocation(m_SkinnedShaderProgram.uiId, "sNormalMap"), 1);

	// Create the non-skinned program
	if(PVRTShaderLoadFromFile(
			c_szDefaultVertShaderBinFile, c_szDefaultVertShaderSrcFile, GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_uiDefaultVertShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if(PVRTShaderLoadFromFile(
			c_szDefaultFragShaderBinFile, c_szDefaultFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiDefaultFragShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if(PVRTCreateProgram(&m_DefaultShaderProgram.uiId, m_uiDefaultVertShader, m_uiDefaultFragShader, g_aszDefaultAttribNames, eNumDefaultAttribs, pErrorStr) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, pErrorStr->c_str());
		return false;
	}

	// Store the location of uniforms for later use
	for(i = 0; i < eNumDefaultUniforms; ++i)
	{
		m_DefaultShaderProgram.auiLoc[i] = glGetUniformLocation(m_DefaultShaderProgram.uiId, g_aszDefaultUniformNames[i]);
	}

	// Set the sampler2D uniforms to corresponding texture units
	glUniform1i(glGetUniformLocation(m_DefaultShaderProgram.uiId, "sTexture"), 0);

	return true;
}

/*!****************************************************************************
 @Function		LoadVbos
 @Description	Loads the mesh data required for this training course into
				vertex buffer objects
******************************************************************************/
void OGLES2ChameleonMan::LoadVbos()
{
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
}

/*!****************************************************************************
 @Function		InitApplication
 @Return		bool		true if no error occured
 @Description	Code in InitApplication() will be called by PVRShell once per
				run, before the rendering context is created.
				Used to initialize variables that are not dependant on it
				(e.g. external modules, loading meshes, etc.)
				If the rendering context is lost, InitApplication() will
				not be called again.
******************************************************************************/
bool OGLES2ChameleonMan::InitApplication()
{
	// Get and set the read path for content files
	CPVRTResourceFile::SetReadPath((char*)PVRShellGet(prefReadPath));

	// Get and set the load/release functions for loading external files.
	// In the majority of cases the PVRShell will return NULL function pointers implying that
	// nothing special is required to load external files.
	CPVRTResourceFile::SetLoadReleaseFunctions(PVRShellGet(prefLoadFileFunc), PVRShellGet(prefReleaseFileFunc));

	// Load the scene
	if (m_Scene.ReadFromFile(c_szSceneFile) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Couldn't load the .pod file\n");
		return false;
	}

	// The cameras are stored in the file. We check it contains at least one.
	if (m_Scene.nNumCamera == 0)
	{
		PVRShellSet(prefExitMessage, "ERROR: The scene does not contain a camera\n");
		return false;
	}

	// Check the scene contains at least one light
	if (m_Scene.nNumLight == 0)
	{
		PVRShellSet(prefExitMessage, "ERROR: The scene does not contain a light\n");
		return false;
	}
	return true;
}

/*!****************************************************************************
 @Function		QuitApplication
 @Return		bool		true if no error occured
 @Description	Code in QuitApplication() will be called by PVRShell once per
				run, just before exiting the program.
				If the rendering context is lost, QuitApplication() will
				not be called.
******************************************************************************/
bool OGLES2ChameleonMan::QuitApplication()
{
	// Free the memory allocated for the scene
	m_Scene.Destroy();

	delete [] m_puiVbo;
	delete [] m_puiIndexVbo;

	return true;
}

/*!****************************************************************************
 @Function		InitView
 @Return		bool		true if no error occured
 @Description	Code in InitView() will be called by PVRShell upon
				initialization or after a change in the rendering context.
				Used to initialize variables that are dependant on the rendering
				context (e.g. textures, vertex buffers, etc.)
******************************************************************************/
bool OGLES2ChameleonMan::InitView()
{
	CPVRTString ErrorStr;

	/*
		Initialize VBO data
	*/
	LoadVbos();

	/*
		Load textures
	*/
	if (!LoadTextures(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	/*
		Load and compile the shaders & link programs
	*/
	if (!LoadShaders(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	/*
		Initialize Print3D
	*/

	// Is the screen rotated?
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

	// Use black as our clear colour
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Initialise variables used for the animation
	m_iTimePrev = PVRShellGetTime();

	return true;
}

/*!****************************************************************************
 @Function		ReleaseView
 @Return		bool		true if no error occured
 @Description	Code in ReleaseView() will be called by PVRShell when the
				application quits or before a change in the rendering context.
******************************************************************************/
bool OGLES2ChameleonMan::ReleaseView()
{
	// Delete textures
	glDeleteTextures(1, &m_ui32TexLegs);
	glDeleteTextures(1, &m_ui32TexBeltNormalMap);
	glDeleteTextures(1, &m_ui32TexHeadNormalMap);
	glDeleteTextures(1, &m_ui32TexLegsNormalMap);
	glDeleteTextures(1, &m_ui32TexSkyLine);
	glDeleteTextures(1, &m_ui32TexWall);
	glDeleteTextures(1, &m_ui32TexLamp);
	glDeleteTextures(1, &m_ui32TexBelt);

	// Delete program and shader objects
	glDeleteProgram(m_SkinnedShaderProgram.uiId);
	glDeleteProgram(m_DefaultShaderProgram.uiId);

	glDeleteShader(m_uiSkinnedVertShader);
	glDeleteShader(m_uiDefaultVertShader);
	glDeleteShader(m_uiSkinnedFragShader);
	glDeleteShader(m_uiDefaultFragShader);

	// Delete buffer objects
	glDeleteBuffers(m_Scene.nNumMesh, m_puiVbo);
	glDeleteBuffers(m_Scene.nNumMesh, m_puiIndexVbo);

	// Release Print3D Textures
	m_Print3D.ReleaseTextures();

	return true;
}

/*!****************************************************************************
 @Function		RenderScene
 @Return		bool		true if no error occured
 @Description	Main rendering loop function of the program. The shell will
				call this function every frame.
				eglSwapBuffers() will be performed by PVRShell automatically.
				PVRShell will also manage important OS events.
				Will also manage relevent OS events. The user has access to
				these events through an abstraction layer provided by PVRShell.
******************************************************************************/
bool OGLES2ChameleonMan::RenderScene()
{
	// Clear the color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use shader program
	glUseProgram(m_SkinnedShaderProgram.uiId);

	if(PVRShellIsKeyPressed(PVRShellKeyNameACTION1))
	{
		m_bEnableDOT3 = !m_bEnableDOT3;
		glUniform1i(m_SkinnedShaderProgram.auiLoc[ebUseDot3], m_bEnableDOT3);
	}

	/*
		Calculates the frame number to animate in a time-based manner.
		Uses the shell function PVRShellGetTime() to get the time in milliseconds.
	*/
	unsigned long iTime = PVRShellGetTime();

	if(iTime > m_iTimePrev)
	{
		float fDelta = (float) (iTime - m_iTimePrev);
		m_fFrame += fDelta * g_fDemoFrameRate;

		// Increment the counters to make sure our animation works
		m_fLightPos	+= fDelta * 0.0034f;
		m_fWallPos	+= fDelta * 0.00027f;
		m_fBackgroundPos += fDelta * -0.000027f;

		// Wrap the Animation back to the Start
		if(m_fLightPos >= PVRT_TWO_PI)
			m_fLightPos -= PVRT_TWO_PI;

		if(m_fWallPos >= PVRT_TWO_PI)
			m_fWallPos -= PVRT_TWO_PI;

		if(m_fBackgroundPos <= 0)
			m_fBackgroundPos += 1.0f;

		if(m_fFrame > m_Scene.nNumFrame - 1)
			m_fFrame = 0;
	}

	m_iTimePrev	= iTime;

	// Set the scene animation to the current frame
	m_Scene.SetFrame(m_fFrame);

	// Set up camera
	PVRTVec3	vFrom, vTo, vUp(0.0f, 1.0f, 0.0f);
	PVRTMat4 mView, mProjection;
	PVRTVec3	LightPos;
	float fFOV;
	int i;

	bool bRotate = PVRShellGet(prefIsRotated) && PVRShellGet(prefFullScreen);

	// Get the camera position, target and field of view (fov)
	if(m_Scene.pCamera[0].nIdxTarget != -1) // Does the camera have a target?
		fFOV = m_Scene.GetCameraPos( vFrom, vTo, 0); // vTo is taken from the target node
	else
		fFOV = m_Scene.GetCamera( vFrom, vTo, vUp, 0); // vTo is calculated from the rotation

	fFOV *= bRotate ? (float)PVRShellGet(prefWidth)/(float)PVRShellGet(prefHeight) : (float)PVRShellGet(prefHeight)/(float)PVRShellGet(prefWidth);

	/*
		We can build the model view matrix from the camera position, target and an up vector.
		For this we use PVRTMat4::LookAtRH().
	*/
	mView = PVRTMat4::LookAtRH(vFrom, vTo, vUp);

	// Calculate the projection matrix
	mProjection = PVRTMat4::PerspectiveFovRH(fFOV,  (float)PVRShellGet(prefWidth)/(float)PVRShellGet(prefHeight), g_fCameraNear, g_fCameraFar, PVRTMat4::OGL, bRotate);

	// Update Light Position and related VGP Program constant
	LightPos.x = 200.0f;
	LightPos.y = 350.0f;
	LightPos.z = 200.0f * PVRTABS(sin((PVRT_PI / 4.0f) + m_fLightPos));

	glUniform3fv(m_SkinnedShaderProgram.auiLoc[eLightPos], 1, LightPos.ptr());

	// Set up the View * Projection Matrix
	PVRTMat4 mViewProjection;

	mViewProjection = mProjection * mView;
	glUniformMatrix4fv(m_SkinnedShaderProgram.auiLoc[eViewProj], 1, GL_FALSE, mViewProjection.ptr());

	// Enable the vertex attribute arrays
	for(i = 0; i < eNumAttribs; ++i) glEnableVertexAttribArray(i);

	// Draw skinned meshes
	for(unsigned int i32NodeIndex = 0; i32NodeIndex < 3; ++i32NodeIndex)
	{
		// Bind correct texture
		switch(i32NodeIndex)
		{
			case eBody:
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_ui32TexHeadNormalMap);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_ui32TexHeadBody);
				break;
			case eLegs:
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_ui32TexLegsNormalMap);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_ui32TexLegs);
				break;
			default:
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_ui32TexBeltNormalMap);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_ui32TexBelt);
				break;
		}

		DrawSkinnedMesh(i32NodeIndex);
	}

	// Safely disable the vertex attribute arrays
	for(i = 0; i < eNumAttribs; ++i) glDisableVertexAttribArray(i);

	// Draw non-skinned meshes
	glUseProgram(m_DefaultShaderProgram.uiId);

	// Enable the vertex attribute arrays
	for(i = 0; i < eNumDefaultAttribs; ++i) glEnableVertexAttribArray(i);

	for(unsigned int i32NodeIndex = 3; i32NodeIndex < m_Scene.nNumMeshNode; ++i32NodeIndex)
	{
		SPODNode& Node = m_Scene.pNode[i32NodeIndex];
		SPODMesh& Mesh = m_Scene.pMesh[Node.nIdx];

		// bind the VBO for the mesh
		glBindBuffer(GL_ARRAY_BUFFER, m_puiVbo[Node.nIdx]);

		// bind the index buffer, won't hurt if the handle is 0
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiIndexVbo[Node.nIdx]);

		// Get the node model matrix
		PVRTMat4 mWorld;
		mWorld = m_Scene.GetWorldMatrix(Node);

		// Setup the appropriate texture and transformation (if needed)
		switch(i32NodeIndex)
		{
			case eWall:
				glBindTexture(GL_TEXTURE_2D, m_ui32TexWall);

				// Rotate the wall mesh which is circular
				mWorld *= PVRTMat4::RotationY(m_fWallPos);

				glUniform1f(m_DefaultShaderProgram.auiLoc[eDefaultUOffset], 0);

				break;
			case eBackground:
				glBindTexture(GL_TEXTURE_2D, m_ui32TexSkyLine);

				glUniform1f(m_DefaultShaderProgram.auiLoc[eDefaultUOffset], m_fBackgroundPos);
				break;
			case eLights:
				{
					glBindTexture(GL_TEXTURE_2D, m_ui32TexLamp);

					PVRTMat4 mWallWorld = m_Scene.GetWorldMatrix(m_Scene.pNode[eWall]);
					mWorld = mWallWorld * PVRTMat4::RotationY(m_fWallPos) * mWallWorld.inverse() * mWorld;

					glUniform1f(m_DefaultShaderProgram.auiLoc[eDefaultUOffset], 0);
				}
				break;
			default:
			break;
		};

		// Set up shader uniforms
		PVRTMat4 mModelViewProj;
		mModelViewProj = mViewProjection * mWorld;
		glUniformMatrix4fv(m_DefaultShaderProgram.auiLoc[eDefaultMVPMatrix], 1, GL_FALSE, mModelViewProj.ptr());

		// Set the vertex attribute offsets
		glVertexAttribPointer(DEFAULT_VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, Mesh.sVertex.nStride,  Mesh.sVertex.pData);
		glVertexAttribPointer(DEFAULT_TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, Mesh.psUVW[0].nStride, Mesh.psUVW[0].pData);

		// Indexed Triangle list
		glDrawElements(GL_TRIANGLES, Mesh.nNumFaces*3, GL_UNSIGNED_SHORT, 0);
	}

	// Safely disable the vertex attribute arrays
	for(i = 0; i < eNumAttribs; ++i) glDisableVertexAttribArray(i);

	// unbind the VBOs
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Display the demo name using the tools. For a detailed explanation, see the training course IntroducingPVRTools
	const char * pDescription;

	if(m_bEnableDOT3)
		pDescription = "Skinning with DOT3 Per Pixel Lighting";
	else
		pDescription = "Skinning with Vertex Lighting";

	m_Print3D.DisplayDefaultTitle("Chameleon Man", pDescription, ePVRTPrint3DSDKLogo);
	m_Print3D.Flush();

	return true;
}

/*!****************************************************************************
 @Function		DrawSkinnedMesh
 @Input			i32NodeIndex		Node index of the mesh to draw
 @Description	Draws a SPODMesh after the model view matrix has been set and
				the meterial prepared.
******************************************************************************/
void OGLES2ChameleonMan::DrawSkinnedMesh(int i32NodeIndex)
{
	SPODNode& Node = m_Scene.pNode[i32NodeIndex];
	SPODMesh& Mesh = m_Scene.pMesh[Node.nIdx];

	// bind the VBO for the mesh
	glBindBuffer(GL_ARRAY_BUFFER, m_puiVbo[Node.nIdx]);
	// bind the index buffer, won't hurt if the handle is 0
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiIndexVbo[Node.nIdx]);

	// Set the vertex attribute offsets
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, Mesh.sVertex.nStride,  Mesh.sVertex.pData);
	glVertexAttribPointer(NORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, Mesh.sNormals.nStride, Mesh.sNormals.pData);
	glVertexAttribPointer(TANGENT_ARRAY, 3, GL_FLOAT, GL_FALSE, Mesh.sTangents.nStride, Mesh.sTangents.pData);
	glVertexAttribPointer(BINORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, Mesh.sBinormals.nStride, Mesh.sBinormals.pData);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, Mesh.psUVW[0].nStride, Mesh.psUVW[0].pData);
	glVertexAttribPointer(BONEINDEX_ARRAY, Mesh.sBoneIdx.n, GL_UNSIGNED_BYTE, GL_FALSE, Mesh.sBoneIdx.nStride, Mesh.sBoneIdx.pData);
	glVertexAttribPointer(BONEWEIGHT_ARRAY, Mesh.sBoneWeight.n, GL_UNSIGNED_BYTE, GL_TRUE, Mesh.sBoneWeight.nStride, Mesh.sBoneWeight.pData);

	for(int i32Batch = 0; i32Batch < Mesh.sBoneBatches.nBatchCnt; ++i32Batch)
	{
		/*
			If the current mesh has bone index and weight data then we need to
			set up some additional variables in the shaders.
		*/

		// Set the number of bones that will influence each vertex in the mesh
		glUniform1i(m_SkinnedShaderProgram.auiLoc[eBoneCount], Mesh.sBoneIdx.n);

		// Go through the bones for the current bone batch
		PVRTMat4 amBoneWorld[8];
		PVRTMat3 afBoneWorldIT[8], mBoneIT;

		int i32Count = Mesh.sBoneBatches.pnBatchBoneCnt[i32Batch];

		for(int i = 0; i < i32Count; ++i)
		{
			// Get the Node of the bone
			int i32NodeID = Mesh.sBoneBatches.pnBatches[i32Batch * Mesh.sBoneBatches.nBatchBoneMax + i];

			// Get the World transformation matrix for this bone and combine it with our app defined
			// transformation matrix
			amBoneWorld[i] = m_Scene.GetBoneWorldMatrix(Node, m_Scene.pNode[i32NodeID]);

			// Calculate the inverse transpose of the 3x3 rotation/scale part for correct lighting
			afBoneWorldIT[i] = PVRTMat3(amBoneWorld[i]).inverse().transpose();
		}

		glUniformMatrix4fv(m_SkinnedShaderProgram.auiLoc[eBoneMatrices], i32Count, GL_FALSE, amBoneWorld[0].ptr());
		glUniformMatrix3fv(m_SkinnedShaderProgram.auiLoc[eBoneMatricesIT], i32Count, GL_FALSE, afBoneWorldIT[0].ptr());

		/*
			As we are using bone batching we don't want to draw all the faces contained within pMesh, we only want
			to draw the ones that are in the current batch. To do this we pass to the drawMesh function the offset
			to the start of the current batch of triangles (Mesh.sBoneBatches.pnBatchOffset[i32Batch]) and the
			total number of triangles to draw (i32Tris)
		*/
		int i32Tris;
		if(i32Batch+1 < Mesh.sBoneBatches.nBatchCnt)
			i32Tris = Mesh.sBoneBatches.pnBatchOffset[i32Batch+1] - Mesh.sBoneBatches.pnBatchOffset[i32Batch];
		else
			i32Tris = Mesh.nNumFaces - Mesh.sBoneBatches.pnBatchOffset[i32Batch];

		// Draw the mesh
		size_t offset = sizeof(GLushort) * 3 * Mesh.sBoneBatches.pnBatchOffset[i32Batch];
		glDrawElements(GL_TRIANGLES, i32Tris * 3, GL_UNSIGNED_SHORT, (void*) offset);
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
	return new OGLES2ChameleonMan();
}

/******************************************************************************
 End of file (OGLES2ChameleonMan.cpp)
******************************************************************************/

