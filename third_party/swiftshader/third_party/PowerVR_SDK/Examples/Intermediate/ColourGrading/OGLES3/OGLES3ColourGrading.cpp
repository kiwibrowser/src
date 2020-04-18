/******************************************************************************
 
 @File         OGLES3ColourGrading.cpp
 
 @Title        Colour grading
 
 @Version
 
 @Copyright    Copyright (c) Imagination Technologies Limited.
 
 @Platform     Independent
 
 @Description  Demonstrates how to colour grade your render.
 
 ******************************************************************************/
#include "PVRShell.h"
#include "OGLES3Tools.h"

/******************************************************************************
 Constants
 ******************************************************************************/
const char* const c_pszMaskTexture = "MaskTexture.pvr";
const char* const c_pszBackgroundTexture = "Background.pvr";

// Our colour lookup tables
const char* const c_pszLUTs[] = 
{ 
	"identity.pvr",
	"bw.pvr",
	"cooler.pvr",
	"warmer.pvr",
	"sepia.pvr",
	"inverted.pvr",
	"highcontrast.pvr",
	"bluewhitegradient.pvr"
};

// Shader source
const char* const c_szFragShaderSrcFile	= "FragShader.fsh";
const char* const c_szVertShaderSrcFile	= "VertShader.vsh";
const char* const c_szSceneFragShaderSrcFile	= "SceneFragShader.fsh";
const char* const c_szSceneVertShaderSrcFile	= "SceneVertShader.vsh";
const char* const c_szBackgroundFragShaderSrcFile = "BackgroundFragShader.fsh";

// POD scene files
const char c_szSceneFile[] = "Mask.pod";

// Camera constants. Used for making the projection matrix
const float CAM_FOV  = PVRT_PI / 6;
const float CAM_NEAR = 4.0f;
const float CAM_FAR = 5000.0f;

// Index to bind the attributes to vertex shaders
const int VERTEX_ARRAY   = 0;
const int TEXCOORD_ARRAY = 1;
const int NORMAL_ARRAY   = 2;

// Look up table enumeration
enum ELUTs
{
	eIdentity,
	eBW,
	eCooler,
	eWarmer,
	eSepia,
	eInverted,
	eHighContrast,
	eBlueWhiteGradient,
	eLast,

	// The range to cycle through
	eA = eBW,
	eB = eBlueWhiteGradient
};

const char* const c_pszLUTNames[] =
{
	"Identity",
	"Black and white",
	"Cooler",
	"Warmer",
	"Sepia",
	"Inverted",
	"High Contrast",
	"Blue White Gradient"
};

/*!****************************************************************************
 Class implementing the PVRShell functions.
 ******************************************************************************/
class OGLES3ColourGrading : public PVRShell
{
	// Print3D object
	CPVRTPrint3D			m_Print3D;
	
	// Texture handle
	GLuint					m_uiMaskTexture;
	GLuint					m_uiBackgroundTexture;
	GLuint					m_uiLUTs[eLast];
	int						m_iCurrentLUT;

	// VBO handle
	GLuint					m_ui32FullScreenRectVBO;
	
	// Stride for vertex data
	unsigned int			m_ui32VertexStride;
	
	// 3D Model
	CPVRTModelPOD	m_Mask;
	GLuint* m_puiMaskVBO;
	GLuint* m_puiMaskIBO;

	GLuint m_ui32BackgroundVBO;

	// Projection and view matrices
	PVRTMat4 m_mViewProjection;

	// Shaders
	GLuint m_uiPostVertShader;
	GLuint m_uiPostFragShader;

	struct
	{
		GLuint uiId;
	}
	m_PostShaderProgram;

	GLuint m_uiBackgroundFragShader;

	struct
	{
		GLuint uiId;
	}
	m_BackgroundShaderProgram;

	GLuint m_uiSceneVertShader;
	GLuint m_uiSceneFragShader;

	struct
	{
		GLuint uiId;
		GLuint uiMVPMatrixLoc;
		GLuint uiLightDirLoc;
		GLuint uiMaterialBiasLoc;
		GLuint uiMaterialScaleLoc;
	}
	m_SceneShaderProgram;

	// Render contexts, etc
	GLint	  m_i32OriginalFbo;

	// Texture IDs used by the app
	GLuint	m_uiTextureToRenderTo;

	// Handle for our FBO and the depth buffer that it requires
	GLuint m_uiFBO;

	// Handle for our multi-sampled FBO and the depth buffer that it requires
	GLuint m_uiFBOMultisampled;
	GLuint m_uiDepthBufferMultisampled;
	GLuint m_uiColourBufferMultisampled;	

	// Start time
	unsigned long m_ulStartTime;

public:
	// PVRShell functions
	virtual bool InitApplication();
	virtual bool InitView();
	virtual bool ReleaseView();
	virtual bool QuitApplication();
	virtual bool RenderScene();

private:
	bool LoadShaders(CPVRTString& ErrorStr);
	bool CreateFBO();
	void LoadVbos(const bool bRotated);
	void DrawMesh(const int i32NodeIndex);
};


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
bool OGLES3ColourGrading::InitApplication()
{
	// Get and set the read path for content files
	CPVRTResourceFile::SetReadPath((char*)PVRShellGet(prefReadPath));
	
	// Get and set the load/release functions for loading external files.
	// In the majority of cases the PVRShell will return NULL function pointers implying that
	// nothing special is required to load external files.
	CPVRTResourceFile::SetLoadReleaseFunctions(PVRShellGet(prefLoadFileFunc), PVRShellGet(prefReleaseFileFunc));

	// Load the scene
	if(m_Mask.ReadFromFile(c_szSceneFile) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Couldn't load the .pod file\n");
		return false;
	}

	// Initialise some variables
	m_puiMaskVBO = m_puiMaskIBO = 0;
	m_iCurrentLUT = eA;
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
bool OGLES3ColourGrading::QuitApplication()
{
	// Free the memory allocated for the scene
	m_Mask.Destroy();

	delete[] m_puiMaskVBO;
	m_puiMaskVBO = 0;

	delete[] m_puiMaskIBO;
	m_puiMaskIBO = 0;
    return true;
}

/*!****************************************************************************
 @Function		LoadEffects
 @Output		ErrorStr	A description of an error, if one occurs.
 @Return		bool		true if no error occurred
 @Description	Loads and parses the bundled PFX and generates the various
                effect objects.
 ******************************************************************************/
bool OGLES3ColourGrading::LoadShaders(CPVRTString& ErrorStr)
{
	// Load and compile the shaders from files.
	if(PVRTShaderLoadFromFile(NULL, c_szVertShaderSrcFile, GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_uiPostVertShader, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if(PVRTShaderLoadFromFile(NULL, c_szFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiPostFragShader, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	// Setup and link the shader program
	const char* aszAttribs[] = { "inVertex", "inTexCoord" };
	if(PVRTCreateProgram(&m_PostShaderProgram.uiId, m_uiPostVertShader, m_uiPostFragShader, aszAttribs, 2, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	// Set the sampler variables to their respective texture unit
	glUniform1i(glGetUniformLocation(m_PostShaderProgram.uiId, "sTexture"), 0);
	glUniform1i(glGetUniformLocation(m_PostShaderProgram.uiId, "sColourLUT"), 1);

	// Background shader

	if(PVRTShaderLoadFromFile(NULL, c_szBackgroundFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiBackgroundFragShader, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	// Set up and link the shader program re-using the vertex shader from the main shader program
	const char* aszBackgroundAttribs[] = { "inVertex", "inTexCoord" };
	if(PVRTCreateProgram(&m_BackgroundShaderProgram.uiId, m_uiPostVertShader, m_uiBackgroundFragShader, aszBackgroundAttribs, 2, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	// Set the sampler2D variable to the first texture unit
	glUniform1i(glGetUniformLocation(m_BackgroundShaderProgram.uiId, "sTexture"), 0);

	// Scene shaders - Used for rendering the mask
	if(PVRTShaderLoadFromFile(NULL, c_szSceneVertShaderSrcFile, GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_uiSceneVertShader, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if(PVRTShaderLoadFromFile(NULL, c_szSceneFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiSceneFragShader, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	// Setup and link the shader program
	const char* aszSceneAttribs[] = { "inVertex", "inTexCoord", "inNormal" };
	if (PVRTCreateProgram(&m_SceneShaderProgram.uiId, m_uiSceneVertShader, m_uiSceneFragShader, aszSceneAttribs, 3, &ErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	// Set the sampler2D variable to the first texture unit
	glUniform1i(glGetUniformLocation(m_SceneShaderProgram.uiId, "sTexture"), 0);

	// Store the location of uniforms for later use
	m_SceneShaderProgram.uiMVPMatrixLoc		= glGetUniformLocation(m_SceneShaderProgram.uiId, "MVPMatrix");
	m_SceneShaderProgram.uiLightDirLoc		= glGetUniformLocation(m_SceneShaderProgram.uiId, "LightDirection");
	m_SceneShaderProgram.uiMaterialBiasLoc	= glGetUniformLocation(m_SceneShaderProgram.uiId, "MaterialBias");
	m_SceneShaderProgram.uiMaterialScaleLoc	= glGetUniformLocation(m_SceneShaderProgram.uiId, "MaterialScale");

	// Set default shader material uniforms
	float fSpecularConcentration = 0.6f;	// a value from 0 to 1 (wider, concentrated)
	float fSpecularIntensity = 0.3f;		// a value from 0 to 1

	// Specular bias
	glUniform1f(m_SceneShaderProgram.uiMaterialBiasLoc, fSpecularConcentration);
	// Specular intensity scale
	glUniform1f(m_SceneShaderProgram.uiMaterialScaleLoc, fSpecularIntensity / (1.0f - fSpecularConcentration));

	return true;
}

/*!****************************************************************************
 @Function		LoadVbos
 @Description	Loads the mesh data required for this training course into
				vertex buffer objects
******************************************************************************/
void OGLES3ColourGrading::LoadVbos(const bool bRotated)
{
	if(!m_puiMaskVBO)      
		m_puiMaskVBO = new GLuint[m_Mask.nNumMesh];

	if(!m_puiMaskIBO) 
		m_puiMaskIBO = new GLuint[m_Mask.nNumMesh];

	/*
		Load vertex data of all meshes in the scene into VBOs

		The meshes have been exported with the "Interleave Vectors" option,
		so all data is interleaved in the buffer at pMesh->pInterleaved.
		Interleaving data improves the memory access pattern and cache efficiency,
		thus it can be read faster by the hardware.
	*/
	glGenBuffers(m_Mask.nNumMesh, m_puiMaskVBO);
	for(unsigned int i = 0; i < m_Mask.nNumMesh; ++i)
	{
		// Load vertex data into buffer object
		SPODMesh& Mesh = m_Mask.pMesh[i];
		unsigned int uiSize = Mesh.nNumVertex * Mesh.sVertex.nStride;
		glBindBuffer(GL_ARRAY_BUFFER, m_puiMaskVBO[i]);
		glBufferData(GL_ARRAY_BUFFER, uiSize, Mesh.pInterleaved, GL_STATIC_DRAW);

		// Load index data into buffer object if available
		m_puiMaskIBO[i] = 0;
		if (Mesh.sFaces.pData)
		{
			glGenBuffers(1, &m_puiMaskIBO[i]);
			uiSize = PVRTModelPODCountIndices(Mesh) * sizeof(GLshort);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiMaskIBO[i]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiSize, Mesh.sFaces.pData, GL_STATIC_DRAW);
		}
	}

	// Create VBO for the fullscreen rect that we'll be rendering our FBO to

	// Interleaved vertex data
	GLfloat afVertices[] = {
		// Left quad
		-1.0f,  1.0f, 0.0f,	1.0f,	// Pos
		0.0f,  1.0f,			    // UVs

		-1.0f, -1.0f, 0.0f,	1.0f,
		0.0f,  0.0f,

		0.0f,  1.0f, 0.0f,	1.0f,
		0.5f,  1.0f,

		0.0f, -1.0f, 0.0f,	1.0f,
		0.5f,  0.0f,

		1.0f,  1.0f, 0.0f,	1.0f,
		1.0f,  1.0f,

		1.0f, -1.0f, 0.0f,	1.0f,
		1.0f,  0.0f,
	};

	if(bRotated) // If we're rotated then pre-process the fullscreen rect's geometry to compensate
	{
		for(unsigned int i = 0; i < 6; ++i)
		{
			float fTmp = afVertices[i * 6 + 1];
			afVertices[i * 6 + 1] = afVertices[i * 6];
			afVertices[i * 6] = fTmp;

			fTmp = afVertices[i * 6 + 5];
			afVertices[i * 6 + 5] = afVertices[i * 6 + 4];
			afVertices[i * 6 + 4] = fTmp;
		}
	}

	glGenBuffers(1, &m_ui32FullScreenRectVBO);
	m_ui32VertexStride = 6 * sizeof(GLfloat); // 4 floats for the pos, 2 for the UVs

	// Bind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_ui32FullScreenRectVBO);

	// Set the buffer's data
	glBufferData(GL_ARRAY_BUFFER, 6 * m_ui32VertexStride, afVertices, GL_STATIC_DRAW);

	// Create the VBO for the background
	GLfloat afBackgroundVertices[] = {
		// Left quad
		-1.0f,  1.0f, 0.0f,	1.0f,	// Pos
		0.0f,  1.0f,			    // UVs

		-1.0f, -1.0f, 0.0f,	1.0f,
		0.0f,  0.0f,

		1.0f,  1.0f, 0.0f,	1.0f,
		1.0f,  1.0f,

		1.0f, -1.0f, 0.0f,	1.0f,
		1.0f,  0.0f,
	};

	if(bRotated) // If we're rotated then pre-process the background geometry
	{
		for(unsigned int i = 0; i < 4; ++i)
		{
			float fTmp = afBackgroundVertices[i * 6 + 1];
			afBackgroundVertices[i * 6 + 1] = -afBackgroundVertices[i * 6];
			afBackgroundVertices[i * 6] = -fTmp;
		}
	}

	glGenBuffers(1, &m_ui32BackgroundVBO);

	// Bind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_ui32BackgroundVBO);

	// Set the buffer's data
	glBufferData(GL_ARRAY_BUFFER, 4 * m_ui32VertexStride, afBackgroundVertices, GL_STATIC_DRAW);

	// Unbind our buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool OGLES3ColourGrading::CreateFBO()
{
	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };

	// Query the max amount of samples that are supported, we are going to use the max
	GLint samples;
	glGetIntegerv(GL_MAX_SAMPLES, &samples);

	// Get the currently bound frame buffer object. On most platforms this just gives 0.
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_i32OriginalFbo);

	// Create a texture for rendering to
	glGenTextures(1, &m_uiTextureToRenderTo);
	glBindTexture(GL_TEXTURE_2D, m_uiTextureToRenderTo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, PVRShellGet(prefWidth), PVRShellGet(prefHeight), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the object that will allow us to render to the aforementioned texture
	glGenFramebuffers(1, &m_uiFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_uiFBO);
	
	glDrawBuffers(1, drawBuffers);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	// Attach the texture to the FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_uiTextureToRenderTo, 0);

	// Check that our FBO creation was successful
	GLuint uStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(uStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		PVRShellSet(prefExitMessage, "ERROR: Failed to initialise FBO");
		return false;
	}

	// Create and initialize the multi-sampled FBO.

	// Create the object that will allow us to render to the aforementioned texture
	glGenFramebuffers(1, &m_uiFBOMultisampled);
	glBindFramebuffer(GL_FRAMEBUFFER, m_uiFBOMultisampled);
	
	glDrawBuffers(1, drawBuffers);
	glReadBuffer(GL_COLOR_ATTACHMENT0);			

	// Generate and bind a render buffer which will become a multisampled depth buffer shared between our two FBOs
	glGenRenderbuffers(1, &m_uiDepthBufferMultisampled);
	glBindRenderbuffer(GL_RENDERBUFFER, m_uiDepthBufferMultisampled);	
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, PVRShellGet(prefWidth), PVRShellGet(prefHeight));

	glGenRenderbuffers(1, &m_uiColourBufferMultisampled);
	glBindRenderbuffer(GL_RENDERBUFFER, m_uiColourBufferMultisampled);	
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGB8, PVRShellGet(prefWidth), PVRShellGet(prefHeight));
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Attach the multisampled depth buffer we created earlier to our FBO.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_uiDepthBufferMultisampled);

	// Attach the multisampled colour renderbuffer to the FBO
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_uiColourBufferMultisampled);

	// Check that our FBO creation was successful
	uStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(uStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		PVRShellSet(prefExitMessage, "ERROR: Failed to initialise multisampled FBO");
		return false;
	}		

	// Unbind the frame buffer object so rendering returns back to the backbuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_i32OriginalFbo);

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
bool OGLES3ColourGrading::InitView()
{
	// Initialize the textures used by Print3D.
	bool bRotate = PVRShellGet(prefIsRotated) && PVRShellGet(prefFullScreen);
	
	if(m_Print3D.SetTextures(0, PVRShellGet(prefWidth), PVRShellGet(prefHeight), bRotate) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Cannot initialise Print3D\n");
		return false;
	}	
		
	// Create the texture
	if(PVRTTextureLoadFromPVR(c_pszMaskTexture, &m_uiMaskTexture) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Failed to load mask texture\n");
		return false;
	}
	
	if(PVRTTextureLoadFromPVR(c_pszBackgroundTexture, &m_uiBackgroundTexture) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Failed to load background texture\n");
		return false;
	}
	
	// Load our 3D texture look up tables
	for(unsigned int i = 0; i < eLast; ++i)
	{
		if(PVRTTextureLoadFromPVR(c_pszLUTs[i], &m_uiLUTs[i]) != PVR_SUCCESS)
		{
			PVRShellSet(prefExitMessage, "ERROR: Failed to load a 3D texture\n");
			return false;
		}

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// Load the effects
	CPVRTString ErrorStr;
	if(!LoadShaders(ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}
	
	// Create FBOs
	if(!CreateFBO())
	{
		PVRShellSet(prefExitMessage, "Failed to create FBO");
		return false;
	}

	// Initialise VBO data
	LoadVbos(bRotate);

	// Calculate the projection and view matrices
	float fAspect = PVRShellGet(prefWidth) / (float)PVRShellGet(prefHeight);
	m_mViewProjection = PVRTMat4::PerspectiveFovRH(CAM_FOV, fAspect, CAM_NEAR, CAM_FAR, PVRTMat4::OGL, bRotate);
	m_mViewProjection *= PVRTMat4::LookAtRH(PVRTVec3(0.f, 0.f, 150.f), PVRTVec3(0.f), PVRTVec3(0.f, 1.f, 0.f));

	// Enable backface culling and depth test
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Set the clear colour
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// Store initial time
	m_ulStartTime = PVRShellGetTime();

	return true;
}

/*!****************************************************************************
 @Function		ReleaseView
 @Return		bool		true if no error occurred
 @Description	Code in ReleaseView() will be called by PVRShell when the
                application quits or before a change in the rendering context.
 ******************************************************************************/
bool OGLES3ColourGrading::ReleaseView()
{
	// Frees the texture
	glDeleteTextures(1, &m_uiMaskTexture);
	glDeleteTextures(1, &m_uiBackgroundTexture);
	glDeleteTextures(eLast, m_uiLUTs);
	glDeleteTextures(1, &m_uiTextureToRenderTo);

	// Release Vertex buffer object.
	glDeleteBuffers(1, &m_ui32FullScreenRectVBO);
	glDeleteBuffers(1, &m_ui32BackgroundVBO);

	// Release effects
	glDeleteShader(m_uiPostVertShader);
	glDeleteShader(m_uiPostFragShader);
	glDeleteShader(m_uiBackgroundFragShader);
	glDeleteShader(m_uiSceneVertShader);
	glDeleteShader(m_uiSceneFragShader);

	glDeleteProgram(m_PostShaderProgram.uiId);
	glDeleteProgram(m_BackgroundShaderProgram.uiId);
	glDeleteProgram(m_SceneShaderProgram.uiId);

	// Tidy up the FBOs and renderbuffers

	// Delete frame buffer objects
	glDeleteFramebuffers(1, &m_uiFBO);
	glDeleteFramebuffers(1, &m_uiFBOMultisampled);

	// Delete our depth buffer
	glDeleteRenderbuffers(1, &m_uiDepthBufferMultisampled);
	glDeleteRenderbuffers(1, &m_uiColourBufferMultisampled);

	// Delete buffer objects
	glDeleteBuffers(m_Mask.nNumMesh, m_puiMaskVBO);
	glDeleteBuffers(m_Mask.nNumMesh, m_puiMaskIBO);

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
bool OGLES3ColourGrading::RenderScene()
{
	// Clears the colour buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	unsigned long ulTime = PVRShellGetTime() - m_ulStartTime;

	// Process input to switch between tone mapping operators
	if(PVRShellIsKeyPressed(PVRShellKeyNameRIGHT))
	{
		++m_iCurrentLUT;

		if(m_iCurrentLUT > eB)
			m_iCurrentLUT = eA;
	}
	else if(PVRShellIsKeyPressed(PVRShellKeyNameLEFT))
	{
		--m_iCurrentLUT;

		if(m_iCurrentLUT < eA)
			m_iCurrentLUT = eB;
	}
	
	// Render to our texture
	{
		// Bind our FBO
		glBindFramebuffer(GL_FRAMEBUFFER, m_uiFBOMultisampled);

		// Clear the colour and depth buffer of our FBO surface
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		// Bind the VBO
		glBindBuffer(GL_ARRAY_BUFFER, m_ui32BackgroundVBO);

		// Use shader program
		glUseProgram(m_BackgroundShaderProgram.uiId);

		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(VERTEX_ARRAY);
		glEnableVertexAttribArray(TEXCOORD_ARRAY);

		// Set the vertex attribute offsets
		glVertexAttribPointer(VERTEX_ARRAY, 4, GL_FLOAT, GL_FALSE, m_ui32VertexStride, 0);
		glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, m_ui32VertexStride, (void*) (4 * sizeof(GLfloat)));

		// Bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_uiBackgroundTexture);

		// Draw a screen-aligned quad.

		// Draws a non-indexed triangle array
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Safely disable the vertex attribute arrays
		glDisableVertexAttribArray(VERTEX_ARRAY);
		glDisableVertexAttribArray(TEXCOORD_ARRAY);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		// Use shader program
		glUseProgram(m_SceneShaderProgram.uiId);

		// Rotate the model matrix
		PVRTMat4 mModel = PVRTMat4::RotationY(ulTime * 0.0015f);

		// Calculate model view projection matrix
		PVRTMat4 mMVP = m_mViewProjection * mModel;

		// Feeds Projection Model View matrix to the shaders
		glUniformMatrix4fv(m_SceneShaderProgram.uiMVPMatrixLoc, 1, GL_FALSE, mMVP.ptr());

		PVRTVec3 vMsLightDir = (PVRTVec3(1, 1, 1) * PVRTMat3(mModel)).normalized();
		glUniform3fv(m_SceneShaderProgram.uiLightDirLoc, 1, vMsLightDir.ptr());

		glBindTexture(GL_TEXTURE_2D, m_uiMaskTexture);

		// Now that the uniforms are set, call another function to actually draw the mesh.
		DrawMesh(0);

		// Unbind the VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//	Give the drivers a hint that we don't want the depth and stencil information stored for future use.
		const GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
		glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, attachments);	

		// Blit and resolve the multisampled render buffer to the non-multisampled FBO
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_uiFBOMultisampled);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_uiFBO);
		glBlitFramebuffer(0, 0, PVRShellGet(prefWidth), PVRShellGet(prefHeight), 0, 0, PVRShellGet(prefWidth), PVRShellGet(prefHeight), GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// We are done with rendering to our FBO so switch back to the back buffer.
		glBindFramebuffer(GL_FRAMEBUFFER, m_i32OriginalFbo);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Use shader program
	glUseProgram(m_PostShaderProgram.uiId);
	
	// Bind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_ui32FullScreenRectVBO);
	
	// Enable the vertex attribute arrays
	glEnableVertexAttribArray(VERTEX_ARRAY);
	glEnableVertexAttribArray(TEXCOORD_ARRAY);

	// Set the vertex attribute offsets
	glVertexAttribPointer(VERTEX_ARRAY, 4, GL_FLOAT, GL_FALSE, m_ui32VertexStride, 0);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, m_ui32VertexStride, (void*) (4 * sizeof(GLfloat)));
	
	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_uiTextureToRenderTo);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, m_uiLUTs[m_iCurrentLUT]);

	// Draw a screen-aligned quad.

	// Draw the left-hand side that shows the scene with the colour grading applied
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	// Draw the right-hande side showing the scene how it looks without
	glBindTexture(GL_TEXTURE_3D, m_uiLUTs[eIdentity]);
	glDrawArrays(GL_TRIANGLE_STRIP, 2, 4);

	// Safely disable the vertex attribute arrays
	glDisableVertexAttribArray(VERTEX_ARRAY);
	glDisableVertexAttribArray(TEXCOORD_ARRAY);
	
	// Unbind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Render title
	m_Print3D.DisplayDefaultTitle("Colour grading using 3D textures", c_pszLUTNames[m_iCurrentLUT], ePVRTPrint3DSDKLogo);
	m_Print3D.Flush();
	
	return true;
}

/*!****************************************************************************
 @Function		DrawMesh
 @Input			i32NodeIndex		Node index of the mesh to draw
 @Description	Draws a SPODMesh after the model view matrix has been set and
				the material prepared.
******************************************************************************/
void OGLES3ColourGrading::DrawMesh(const int i32NodeIndex)
{
	int i32MeshIndex = m_Mask.pNode[i32NodeIndex].nIdx;
	SPODMesh* pMesh = &m_Mask.pMesh[i32MeshIndex];

	// bind the VBO for the mesh
	glBindBuffer(GL_ARRAY_BUFFER, m_puiMaskVBO[i32MeshIndex]);
	// bind the index buffer, won't hurt if the handle is 0
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiMaskIBO[i32MeshIndex]);

	// Enable the vertex attribute arrays
	glEnableVertexAttribArray(VERTEX_ARRAY);
	glEnableVertexAttribArray(NORMAL_ARRAY);
	glEnableVertexAttribArray(TEXCOORD_ARRAY);

	// Set the vertex attribute offsets
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sVertex.nStride, pMesh->sVertex.pData);
	glVertexAttribPointer(NORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sNormals.nStride, pMesh->sNormals.pData);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, pMesh->psUVW[0].nStride, pMesh->psUVW[0].pData);

	// Indexed Triangle list
	glDrawElements(GL_TRIANGLES, pMesh->nNumFaces*3, GL_UNSIGNED_SHORT, 0);
	
	// Safely disable the vertex attribute arrays
	glDisableVertexAttribArray(VERTEX_ARRAY);
	glDisableVertexAttribArray(NORMAL_ARRAY);
	glDisableVertexAttribArray(TEXCOORD_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
	return new OGLES3ColourGrading();
}

/******************************************************************************
 End of file (OGLES3ColourGrading.cpp)
 ******************************************************************************/

