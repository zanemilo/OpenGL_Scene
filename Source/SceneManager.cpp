///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	goldMaterial.ambientStrength = 0.5f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	goldMaterial.specularColor = glm::vec3(10.8f, 10.8f, 10.8f);
	goldMaterial.shininess = 4500.0;
	goldMaterial.tag = "steel";
	m_objectMaterials.push_back(goldMaterial);
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.4f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "filling";
	m_objectMaterials.push_back(cementMaterial);
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wooden";
	m_objectMaterials.push_back(woodMaterial);
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	tileMaterial.ambientStrength = 0.1f;
	tileMaterial.diffuseColor = glm::vec3(0.1f, 0.6f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	tileMaterial.shininess = 4.0;
	tileMaterial.tag = "plastic";
	m_objectMaterials.push_back(tileMaterial);
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.35f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(1.5f, 1.0f, 1.0f);
	glassMaterial.shininess = 850.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	clayMaterial.shininess = 0.2;
	clayMaterial.tag = "cookie";
	m_objectMaterials.push_back(clayMaterial);



}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setVec3Value("lightSources[0].position", 30.0f, 15.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 1.01f, 0.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 37.4f, 30.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.002f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -60.0f, 42.0f, -30.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 4.01f, 1.5f, 4.3f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 37.4f, 3.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 30.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.004f);

	m_pShaderManager->setVec3Value("lightSources[2].position", -2.0f, 50.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 5.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 10.4f, 3.0f, 30.0f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.3f, 0.0f, 0.7f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.2f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.001f);

	m_pShaderManager->setBoolValue("bUseLighting", true);


}

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	if (!CreateGLTexture("textures/cookie.png", "cookie"))
	{
		std::cerr << "Failed to load cookie texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/knife_handle.jpg", "wood"))
	{
		std::cerr << "Failed to load wood texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/filling.png", "filling"))
	{
		std::cerr << "Failed to load filling texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/stainless.jpg", "steel"))
	{
		std::cerr << "Failed to load steel texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/cup.png", "plastic"))
	{
		std::cerr << "Failed to load plastic texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/book.png", "book"))
	{
		std::cerr << "Failed to load book texture!" << std::endl;
	}
	
	if (!CreateGLTexture("textures/paper.png", "paper"))
	{
		std::cerr << "Failed to load paper texture!" << std::endl;
	}



	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// 
	// load the textures for the 3D scene
	LoadSceneTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.09f, 0.0f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wood");
	
	
	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("wooden");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/		//CYLINDER1 Cookie
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.25f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 2.33f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("cookie"); 

	// Disable overlay
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("cookie");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/		//CYLINDER2 Filling
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.7f, 0.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 1.9f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("filling");

	// Disable overlay
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("filling");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/		//CYLINDER3 Cookie
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.25f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 1.8f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("cookie");

	// Disable overlay
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("cookie");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/		//CYLINDER4 Cookie
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.25f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.6f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("cookie");

	// Disable overlay
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("cookie");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/		//CYLINDER5 Filling
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.7f, 0.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("filling");

	// Disable overlay
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("filling");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/		//CYLINDER6 Cookie
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.25f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("cookie");

	// Disable overlay
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("cookie");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/				//BOOKBOTTOM
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 1.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 106.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 1.0f, 1.0f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("book");

	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("wooden");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/				//PAPER
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.5f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 16.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 1.73f, 1.0f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("paper");

	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("filling");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/				//BOOKTOP
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.5f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 106.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.5f, 2.2f, 1.0f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("book");

	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("wooden");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/				//BINDING
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 0.5f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 16.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.62f, 1.7f, 0.1f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("book");

	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("wooden");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/				//Sphere
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.7f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.7f, 1.8f, 2.0f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("steel");

	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("steel");
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/				//CUP
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 5.2f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 6.3f, -4.0f);		// Lowered Plane on Y-Axis for visibility

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plastic");


	// Disable overlay for the plane
	if (m_pShaderManager)
	{
		m_pShaderManager->setBoolValue("bUseOverlay", false);
		m_pShaderManager->setBoolValue("bUseTexture", true); // Enable base texture
	}
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();


}
