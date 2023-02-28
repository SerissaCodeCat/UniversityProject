//--------------------------------------------------------------------------------------
//	GraphicsAssign1.cpp
//
//	Shaders Graphics Assignment
//	Add further models using different shader techniques
//	See assignment specification for details
//--------------------------------------------------------------------------------------

//***|  INFO  |****************************************************************
// Lights:
//   The initial project shows models for a couple of point lights, but the
//   initial shaders don't actually apply any lighting. Part of the assignment
//   is to add a shader that uses this information to actually light a model.
//   Refer to lab work to determine how best to do this.
// 
// Textures:
//   The models in the initial scene have textures loaded but the initial
//   technique/shaders doesn't show them. Part of the assignment is to add 
//   techniques to show a textured model
//
// Shaders:
//   The initial shaders render plain coloured objects with no lighting:
//   - The vertex shader performs basic transformation of 3D vertices to 2D
//   - The pixel shader colours every pixel the same colour
//   A few shader variables are set up to communicate between C++ and HLSL
//   You will usually need to add further variables to add new techniques
//*****************************************************************************

#include <windows.h>
#include <d3d10.h>
#include <D3DX10.h>
#include <atlbase.h>
#include<deque> //added in to support a changing number of lights.
#include "resource.h"
#include "Defines.h" // General definitions shared by all source files
#include "Model.h"   // Model class - encapsulates working with vertex/index data and world matrix
#include "Camera.h"  // Camera class - encapsulates the camera's view and projection matrix
#include "CTimer.h"  // Timer class - not DirectX
#include "Input.h"   // Input functions - not DirectX



//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

// Models and cameras encapsulated in classes for flexibity and convenience
// The CModel class collects together geometry and world matrix, and provides functions to control the model and render it
// The CCamera class handles the view and projections matrice, and provides functions to control the camera
CModel* Cube;
CModel* Floor;
CModel* Sphere;
CModel* Teapot;
CModel* Troll;
CCamera* Camera;
CModel* Bush;


// Textures - no texture class yet so using DirectX variables
ID3D10ShaderResourceView* CubeDiffuseMap	= NULL;
ID3D10ShaderResourceView* FloorDiffuseMap	= NULL;
ID3D10ShaderResourceView* SphereDiffuseMap	= NULL;
ID3D10ShaderResourceView* TeapotDiffuseMap	= NULL;
ID3D10ShaderResourceView* TrollDiffuseMap	= NULL;
ID3D10ShaderResourceView* BushDiffuseMap	= NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///SHADOW MAPS
ID3D10Texture2D*          ShadowMap1Texture = NULL;
ID3D10DepthStencilView*   ShadowMap1DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap1 = NULL;

ID3D10Texture2D*          ShadowMap2Texture = NULL;
ID3D10DepthStencilView*   ShadowMap2DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap2 = NULL;

int ShadowMapSize = 256;
float LightConeAngle = 90.0f;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Light data - stored manually as there is no light class
const int NumLights = 2; 
CModel* Light[NumLights];
D3DXVECTOR4 BackgroundColour = D3DXVECTOR4( 0.2f, 0.2f, 0.3f, 1.0f );
D3DXVECTOR3 LightColour[NumLights] = { D3DXVECTOR3( 0.8f, 0.8f, 1.0f ) * 8, D3DXVECTOR3( 1.0f, 0.8f, 0.2f ) * 30 };
D3DXVECTOR3 AmbientColour = D3DXVECTOR3( 0.00f, 0.00f, 0.00f );
D3DXVECTOR3 Black( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3 Blue( 0.0f, 0.0f, 1.0f );
float SpecularPower = 256.0f;


float Tint;
float light1Red;
float light1Green;
float light1Blue;
// Display models where the lights are. One of the lights will follow an orbit


const float LightOrbitRadius = 20.0f;
const float LightOrbitSpeed  = 0.5f;

// Note: There are move & rotation speed constants in Defines.h



//--------------------------------------------------------------------------------------
// Shader Variables
//--------------------------------------------------------------------------------------
// Variables to connect C++ code to HLSL shaders

// Effects / techniques
ID3D10Effect*          Effect = NULL;
ID3D10EffectTechnique* PlainColourTechnique = NULL;
ID3D10EffectTechnique* TexturedTechnique = NULL;
ID3D10EffectTechnique* AddTexturedTechnique = NULL;
ID3D10EffectTechnique* VertexLitTechnique = NULL;
ID3D10EffectTechnique* SpecularAlphaLitTechnique = NULL;
//ID3D10EffectTechnique* ShadowMappingTechnique = NULL;
ID3D10EffectTechnique* DepthOnlyTechnique = NULL;



// Matrices
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;

// Textures
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ADDED FOR SHADOW MAPS
ID3D10EffectShaderResourceVariable* ShadowMap1Var = NULL; // Pass shadow maps to pixel shader to test if pixels are in shadow
ID3D10EffectShaderResourceVariable* ShadowMap2Var = NULL; // Pass shadow maps to pixel shader to test if pixels are in shadow
ID3D10Texture2D* DepthStencil = NULL;
ID3D10DepthStencilView* DepthStencilView = NULL;
ID3D10RenderTargetView* BackBufferRenderTarget = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Light Effect variables
ID3D10EffectVectorVariable* g_pCameraPosVar = NULL;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///Added additional variables for shadow mapping
ID3D10EffectVectorVariable* CameraPosVar = NULL;

ID3D10EffectVectorVariable* g_pLightPosVar = NULL;
ID3D10EffectVectorVariable* g_pLightFacingVar = NULL;
ID3D10EffectMatrixVariable* g_pLightViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* g_pLightProjMatrixVar = NULL;
ID3D10EffectScalarVariable* g_pLightConeAngleVar = NULL;
ID3D10EffectVectorVariable* g_pLightColourVar = NULL;

ID3D10EffectVectorVariable* g_pLight1PosVar = NULL;
ID3D10EffectVectorVariable* g_pLight1FacingVar = NULL;
ID3D10EffectMatrixVariable* g_pLight1ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* g_pLight1ProjMatrixVar = NULL;
ID3D10EffectScalarVariable* g_pLight1ConeAngleVar = NULL;
ID3D10EffectVectorVariable* g_pLight1ColourVar = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


ID3D10EffectVectorVariable* g_pAmbientColourVar = NULL;
ID3D10EffectScalarVariable* g_pSpecularPowerVar = NULL;
ID3D10EffectScalarVariable* g_pTintVar = NULL;

// Miscellaneous
ID3D10EffectVectorVariable* ModelColourVar = NULL;
D3DXMATRIX CubeWorldMatrix;
D3DXMATRIX FloorWorldMatrix;
/*
D3DXVECTOR3 CameraPosition( -3.0f, 5.0f, -10.0f );
D3DXVECTOR3 CameraRotation( 0.2f, 0.2f, 0.0f );
*/
D3DXMATRIX CameraWorldMatrix;
D3DXMATRIX ViewMatrix;
D3DXMATRIX ProjMatrix;
D3DXVECTOR3 LightPosition;
D3DXVECTOR3 Light1Position;
D3DXMATRIX LightWorldMatrix;

//--------------------------------------------------------------------------------------
// DirectX Variables
//--------------------------------------------------------------------------------------

// The main D3D interface, this pointer is used to access most D3D functions (and is shared across all cpp files through Defines.h)
ID3D10Device* g_pd3dDevice = NULL;

// Variables used to setup D3D
IDXGISwapChain*         SwapChain = NULL;
//ID3D10Texture2D*        DepthStencil = NULL;
//ID3D10DepthStencilView* DepthStencilView = NULL;
ID3D10RenderTargetView* RenderTargetView = NULL;

// Variables used to setup the Window
HINSTANCE HInst = NULL;
HWND      HWnd = NULL;
int       g_ViewportWidth;
int       g_ViewportHeight;



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------

// Declare functions in advance so we can use them before their definition
bool InitDevice();
void ReleaseResources();
bool LoadEffectFile();
bool InitScene();
void UpdateScene( float frameTime );
void RenderShadows();
void RenderMain(CCamera* Camera);
void RenderShadowMap(int LightNum);
void RenderScene();
bool InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );



//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
bool InitDevice()
{
	// Many DirectX functions return a "HRESULT" variable to indicate success or failure. Microsoft code often uses
	// the FAILED macro to test this variable, you'll see it throughout the code - it's fairly self explanatory.
	HRESULT hr = S_OK;


	////////////////////////////////
	// Initialise Direct3D

	// Calculate the visible area the window we are using - the "client rectangle" refered to in the first function is the 
	// size of the interior of the window, i.e. excluding the frame and title
	RECT rc;
	GetClientRect( HWnd, &rc );
	g_ViewportWidth = rc.right - rc.left;
	g_ViewportHeight = rc.bottom - rc.top;


	// Create a Direct3D device (i.e. initialise D3D), and create a swap-chain (create a back buffer to render to)
	DXGI_SWAP_CHAIN_DESC sd;         // Structure to contain all the information needed
	ZeroMemory( &sd, sizeof( sd ) ); // Clear the structure to 0 - common Microsoft practice, not really good style
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_ViewportWidth;             // Target window size
	sd.BufferDesc.Height = g_ViewportHeight;           // --"--
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Pixel format of target window
	sd.BufferDesc.RefreshRate.Numerator = 60;          // Refresh rate of monitor
	sd.BufferDesc.RefreshRate.Denominator = 1;         // --"--
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.OutputWindow = HWnd;                          // Target window
	sd.Windowed = TRUE;                                // Whether to render in a window (TRUE) or go fullscreen (FALSE)
	hr = D3D10CreateDeviceAndSwapChain( NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_DEBUG,
										D3D10_SDK_VERSION, &sd, &SwapChain, &g_pd3dDevice );
	if( FAILED( hr ) ) return false;


	// Specify the render target as the back-buffer - this is an advanced topic. This code almost always occurs in the standard D3D setup
	ID3D10Texture2D* pBackBuffer;
	hr = SwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );
	if( FAILED( hr ) ) return false;
	hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &RenderTargetView );
	pBackBuffer->Release();
	if( FAILED( hr ) ) return false;


	// Create a texture (bitmap) to use for a depth buffer
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = g_ViewportWidth;
	descDepth.Height = g_ViewportHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &DepthStencil );
	if( FAILED( hr ) ) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView( DepthStencil, &descDSV, &DepthStencilView );
	if( FAILED( hr ) ) return false;

	// Select the back buffer and depth buffer to use for rendering now
	g_pd3dDevice->OMSetRenderTargets( 1, &RenderTargetView, DepthStencilView );


	// Setup the viewport - defines which part of the window we will render to, almost always the whole window
	

	return true;
}


// Release the memory held by all objects created
void ReleaseResources()
{
	// The D3D setup and preparation of the geometry created several objects that use up memory (e.g. textures, vertex/index buffers etc.)
	// Each object that allocates memory (or hardware resources) needs to be "released" when we exit the program
	// There is similar code in every D3D program, but the list of objects that need to be released depends on what was created
	// Test each variable to see if it exists before deletion
	if( g_pd3dDevice )     g_pd3dDevice->ClearState();

	for (int i = 0; i<NumLights; i++)
	{
		delete Light[i];
	}
	delete Floor;
	delete Cube;
	delete Camera;
	delete Troll;
	delete Bush;
	delete Teapot;
	delete Sphere;


    if( FloorDiffuseMap )  FloorDiffuseMap->Release();
    if( CubeDiffuseMap )   CubeDiffuseMap->Release();
	if( TrollDiffuseMap )  TrollDiffuseMap->Release();
	if( BushDiffuseMap )  BushDiffuseMap->Release();
	if( TeapotDiffuseMap)  TeapotDiffuseMap->Release();
	if( SphereDiffuseMap)  SphereDiffuseMap->Release();
	if( Effect )           Effect->Release();
	if( DepthStencilView ) DepthStencilView->Release();
	if( RenderTargetView ) RenderTargetView->Release();
	if( DepthStencil )     DepthStencil->Release();
	if( SwapChain )        SwapChain->Release();
	if( g_pd3dDevice )     g_pd3dDevice->Release();
	
}



//--------------------------------------------------------------------------------------
// Load and compile Effect file (.fx file containing shaders)
//--------------------------------------------------------------------------------------
// An effect file contains a set of "Techniques". A technique is a combination of vertex, geometry and pixel shaders (and some states) used for
// rendering in a particular way. We load the effect file at runtime (it's written in HLSL and has the extension ".fx"). The effect code is compiled
// *at runtime* into low-level GPU language. When rendering a particular model we specify which technique from the effect file that it will use

bool LoadEffectFile()
{
	ID3D10Blob* pErrors; // This strangely typed variable collects any errors when compiling the effect file
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // These "flags" are used to set the compiler options

	// Load and compile the effect file
	HRESULT hr = D3DX10CreateEffectFromFile( L"GraphicsAssign1.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &Effect, &pErrors, NULL );
	if( FAILED( hr ) )
	{
		if (pErrors != 0)  MessageBox( NULL, CA2CT(reinterpret_cast<char*>(pErrors->GetBufferPointer())), L"Error", MB_OK ); // Compiler error: display error message
		else               MessageBox( NULL, L"Error loading FX file. Ensure your FX file is in the same folder as this executable.", L"Error", MB_OK );  // No error message - probably file not found
		return false;
	}

	
	// Now we can select techniques from the compiled effect file
	PlainColourTechnique			= Effect->GetTechniqueByName( "PlainColour" );
	TexturedTechnique				= Effect->GetTechniqueByName( "Textured" );
	AddTexturedTechnique			= Effect->GetTechniqueByName("AddTextured");
	VertexLitTechnique				= Effect->GetTechniqueByName("VertexLit");
	SpecularAlphaLitTechnique		= Effect->GetTechniqueByName("SpecularAlphaLit");
	DepthOnlyTechnique				= Effect->GetTechniqueByName("DepthOnly");/////////////MAKE DEPTH ONLY
	

	// Create special variables to allow us to access global variables in the shaders from C++
	WorldMatrixVar    = Effect->GetVariableByName( "WorldMatrix" )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName( "ViewMatrix"  )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName( "ProjMatrix"  )->AsMatrix();
	DiffuseMapVar	  = Effect->GetVariableByName( "DiffuseMap" )->AsShaderResource();
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Added for shadows
	ShadowMap1Var = Effect->GetVariableByName( "ShadowMap" )->AsShaderResource(); // Will pass shadow maps over to pixel shader
	ShadowMap2Var = Effect->GetVariableByName( "ShadowMap1" )->AsShaderResource(); // Will pass shadow maps over to pixel shader
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ModelColourVar	  = Effect->GetVariableByName( "ModelColour"  )->AsVector();

	// Also access shader variables needed for lighting
	g_pCameraPosVar     = Effect->GetVariableByName( "CameraPos"     )->AsVector();
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///Lights Redone to include matricies and vectors needed for shadow mapping
	
	g_pLightPosVar			 = Effect->GetVariableByName( "LightPos"      )->AsVector();
	g_pLightFacingVar		 = Effect->GetVariableByName( "LightFacing"  )->AsVector();
	g_pLightColourVar		 = Effect->GetVariableByName( "LightColour"   )->AsVector();
	g_pLightViewMatrixVar	 = Effect->GetVariableByName( "LightViewMatrix"   )->AsMatrix();
	g_pLightProjMatrixVar	 = Effect->GetVariableByName( "LightProjMatrix"   )->AsMatrix();
	g_pLightConeAngleVar	 = Effect->GetVariableByName( "LightCosHalfAngle" )->AsScalar();

	g_pLight1PosVar			 = Effect->GetVariableByName( "Light1Pos"      )->AsVector();
	g_pLight1FacingVar		 = Effect->GetVariableByName( "Light1Facing"  )->AsVector();
	g_pLight1ColourVar		 = Effect->GetVariableByName( "Light1Colour"   )->AsVector();	
	g_pLight1ViewMatrixVar	 = Effect->GetVariableByName( "Light1ViewMatrix"   )->AsMatrix();
	g_pLight1ProjMatrixVar	 = Effect->GetVariableByName( "Light1ProjMatrix"   )->AsMatrix();
	g_pLight1ConeAngleVar	 = Effect->GetVariableByName( "Light1CosHalfAngle" )->AsScalar();
	
	g_pAmbientColourVar = Effect->GetVariableByName( "AmbientColour" )->AsVector();
	g_pSpecularPowerVar = Effect->GetVariableByName( "SpecularPower" )->AsScalar();
	g_pTintVar		    = Effect->GetVariableByName( "Tint" )->AsScalar();
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	return true;
}



//--------------------------------------------------------------------------------------
// Scene Setup / Update / Rendering
//--------------------------------------------------------------------------------------

// Create / load the camera, models and textures for the scene
bool InitScene()
{
	//////////////////
	// Create camera

	Camera = new CCamera();
	Camera->SetPosition( D3DXVECTOR3(-15, 20,-40) );
	Camera->SetRotation( D3DXVECTOR3(ToRadians(13.0f), ToRadians(18.0f), 0.0f) ); // ToRadians is a new helper function to convert degrees to radians


	///////////////////////
	// Load/Create models

	Cube	= new CModel;
	Floor	= new CModel;
	Troll	= new CModel;
	Bush	= new CModel;
	Teapot	= new CModel;
	Sphere	= new CModel;

	// The model class can load ".X" files. It encapsulates (i.e. hides away from this code) the file loading/parsing and creation of vertex/index buffers
	// We must pass an example technique used for each model. We can then only render models with techniques that uses matching vertex input data
	if (!Cube->  Load( "Cube.x",  PlainColourTechnique )) return false;
	if (!Floor-> Load( "Floor.x", PlainColourTechnique )) return false;
	if (!Teapot->Load( "Teapot.x", PlainColourTechnique)) return false; 
	if (!Troll->Load( "Troll.x", PlainColourTechnique,true)) return false;
	if (!Bush->Load( "Bush1.x", PlainColourTechnique,true)) return false;
	if (!Sphere->Load( "Sphere.x", PlainColourTechnique)) return false;
	
	/////////////////////////////////////////////////////////////////////////////////
	for (int i = 0; i<NumLights; i++)
	{
		Light[i] = new CModel;
		if (!Light[i]->Load( "Light.x", PlainColourTechnique )) return false;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Initial positions
	
	Cube->SetPosition( D3DXVECTOR3( 0, 5, 0) );
	Troll->SetPosition(  D3DXVECTOR3(20, 0, 0) );
	Bush->SetPosition(  D3DXVECTOR3(25, 0, 0) );
	Teapot->SetPosition(  D3DXVECTOR3(30, 0, 20) );
	for(int i = 0; i < NumLights; i++)
	{
		Light[i]->SetPosition( D3DXVECTOR3(10*i, 10*i, 0) );
		Light[i]->SetScale( 0.1f ); // Nice if size of light reflects its brightness
	}
	Sphere->SetPosition(  D3DXVECTOR3(-40, 10, 0) );


	//////////////////
	// Load textures

	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &CubeDiffuseMap,  NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"WoodDiffuseSpecular.dds",  NULL, NULL, &FloorDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"Troll1DiffuseSpecular.dds",  NULL, NULL, &TrollDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"BushDiffuseAlpha.dds",  NULL, NULL, &BushDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StoneDiffuseSpecular.dds",  NULL, NULL, &TeapotDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StoneDiffuseSpecular.dds",  NULL, NULL, &SphereDiffuseMap, NULL ) )) return false;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//load and format SHADOW MAP textures using DX10 texture API

	D3D10_TEXTURE2D_DESC shadowMapDesc;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL/*indicates a render target*/ | D3D10_BIND_SHADER_RESOURCE;//indicates it is to be passed to the shader after creation.
	shadowMapDesc.CPUAccessFlags = 0; // 0 indicates the CPU has no need to ether access or write to this texture.
	shadowMapDesc.Format = DXGI_FORMAT_R32_TYPELESS; //A single-component, 32-bit typeless format that supports 32 bits for the red channel. Typeless so depth buffer and textures dont conflict
	shadowMapDesc.Height = ShadowMapSize;
	shadowMapDesc.Width = ShadowMapSize;
	shadowMapDesc.MipLevels = 1; // only a single layer to the texture.
	shadowMapDesc.MiscFlags = 0;
	shadowMapDesc.SampleDesc.Count = 1; // unable to find information on this, seems to be related to Anti-Aliasing
	shadowMapDesc.SampleDesc.Quality=0; // also seems related to AA
	shadowMapDesc.Usage = D3D10_USAGE_DEFAULT; //default DX10 Usage
	if (FAILED(g_pd3dDevice->CreateTexture2D( &shadowMapDesc, NULL, &ShadowMap1Texture ) )) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D32_FLOAT; // See "tech gotcha" above
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	if (FAILED(g_pd3dDevice->CreateDepthStencilView( ShadowMap1Texture, &descDSV, &ShadowMap1DepthView ) )) return false;

	// We also need to send this texture (a GPU memory resource) to the shaders. To do that we must create a shader-resource "view"	
	D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = DXGI_FORMAT_R32_FLOAT; // See "tech gotcha" above
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED( g_pd3dDevice->CreateShaderResourceView( ShadowMap1Texture, &srDesc, &ShadowMap1 ) )) return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///functions to calculate the values needed for working with the lights
D3DXMATRIXA16 LightViewMatrixCalc(int lightNum)
{
	D3DXMATRIXA16 viewMatrix;

	D3DXMATRIXA16 worldMatrix = Light[lightNum]->GetWorldMatrix();
	D3DXMatrixInverse(&viewMatrix, NULL, &worldMatrix);//view matrix is the inverse of the world matrix.

	return viewMatrix;

}

D3DXMATRIXA16 LightProjMatrixCalc (int lightNum)
{
	D3DXMATRIXA16 projMatrix;

	D3DXMatrixPerspectiveFovLH( &projMatrix, ToRadians( LightConeAngle ), 1, 0.1f, 1000.0f);

	return projMatrix;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderShadowMap(int LightNum)
{

	// Pass the light's "camera" matrices to the vertex shader - use helper functions above to turn spotlight settings into "camera" matrices
	ViewMatrixVar->SetMatrix( LightViewMatrixCalc( LightNum ) );
	ProjMatrixVar->SetMatrix( LightProjMatrixCalc( LightNum ) );


	//-----------------------------------
	// Render each model into shadow map

	// Render troll - no need to set its texture as shadow maps just render to the depth buffer
	WorldMatrixVar->SetMatrix( Troll->GetWorldMatrix() );
	Troll->Render( DepthOnlyTechnique );  // Use special rendering technique to render depths only

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix( Troll->GetWorldMatrix() );
	Troll->Render( DepthOnlyTechnique );

	WorldMatrixVar->SetMatrix( Cube->GetWorldMatrix() );
	Cube->Render( DepthOnlyTechnique );

	WorldMatrixVar->SetMatrix( Teapot->GetWorldMatrix() );
	Teapot->Render( DepthOnlyTechnique );

	WorldMatrixVar->SetMatrix( Bush->GetWorldMatrix() );
	Bush->Render( DepthOnlyTechnique );

	WorldMatrixVar->SetMatrix( Floor->GetWorldMatrix() );
	Floor->Render( DepthOnlyTechnique );
}
void RenderMain (CCamera *camera)
{
	// Pass the camera's matrices to the vertex shader and position to the vertex shader
	ViewMatrixVar->SetMatrix( camera->GetViewMatrix() );
	ProjMatrixVar->SetMatrix( camera->GetProjectionMatrix() );
	CameraPosVar->SetRawValue( camera->GetPosition(), 0, 12 );


	// Send the shadow maps rendered in the function below to the shader
    ShadowMap1Var->SetResource( ShadowMap1 );
  

	// Render troll
	WorldMatrixVar->SetMatrix( Troll->GetWorldMatrix() ); // Send the troll's world matrix to the shader
    DiffuseMapVar->SetResource( TrollDiffuseMap );        // Send the troll's diffuse/specular map to the shader
	Troll->Render( SpecularAlphaLitTechnique );              // Pass rendering technique to the model class

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix( Cube->GetWorldMatrix() );
    DiffuseMapVar->SetResource( CubeDiffuseMap );
	Cube->Render( SpecularAlphaLitTechnique );

	WorldMatrixVar->SetMatrix( Teapot->GetWorldMatrix() );
    DiffuseMapVar->SetResource( TeapotDiffuseMap );
	Teapot->Render( SpecularAlphaLitTechnique );

	WorldMatrixVar->SetMatrix( Floor->GetWorldMatrix() );
    DiffuseMapVar->SetResource( FloorDiffuseMap );
	Floor->Render( SpecularAlphaLitTechnique );

	// Using an array of lights in this lab
	for (int i = 0; i < NumLights; i++) 
	{
		WorldMatrixVar->SetMatrix( (float*)Light[i]->GetWorldMatrix() );
		g_pTintVar->SetRawValue( LightColour[i], 0, 12 ); // Using special shader that tints the light model to match the light colour
		Light[i]->Render( PlainColourTechnique );
	}
}
// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (view matrix, projection matrix) each frame
	// Don't be deceived into thinking that this is a new method to control models - the same code we used previously is in the camera class
	Camera->Control( frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
	Camera->UpdateMatrices();

	
	// Control cube position and update its world matrix each frame
	Cube->Control( frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );
	Cube->UpdateMatrix();

	// Update the orbiting light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float Rotate = 0.0f;
	Light[0]->SetPosition( Light[0]->GetPosition() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 0, sin(Rotate)*LightOrbitRadius) );
	Light[1]->SetPosition( Light[1]->GetPosition() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 0, sin(Rotate)*LightOrbitRadius) );
	Rotate -= LightOrbitSpeed * frameTime;
	//Light[0]->UpdateMatrix();
	//pulsing light 1 one and off
	static float fade = 0.6f;
	Tint -= fade * frameTime;
	//Light1Colour.x += fade * frameTime;
	//Light2Colour.x += fade * frameTime;
	//Light2Colour.z -= fade * frameTime;
	//if(Light1Colour.x < 0.0f || Light1Colour.x > 1.0f)
	//{
	//	fade = -fade;
	//}
	//Light1Colour.y=Light1Colour.z=Light1Colour.x;
	// same technique used to change light 2 from green to yellow
	
	//Light2Colour.y = 0.0f;
	// Second light doesn't move, but do need to make sure its matrix has been calculated - could do this in InitScene instead
	for(int i = 0; i < NumLights; i++)
	{
		Light[i]->UpdateMatrix();
	}
	Troll->UpdateMatrix();
	Bush->UpdateMatrix();
	Teapot->UpdateMatrix();
	Sphere->UpdateMatrix();
}


// Render everything in the scene
void RenderScene()
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///d//Moved Camera pos/rot retrieval to here, original submission had overlooked this and specular highlights where only being
	////Calculated from the camera's original possition.
	D3DXVECTOR3 CameraPosition = Camera->GetPosition();
	D3DXVECTOR3 CameraRotation = Camera->GetRotation();
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//taken dirrectly from shadowMapping
	g_pLightPosVar->			SetRawValue( Light[0]->GetPosition(), 0, 12 );  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	g_pLightFacingVar->		SetRawValue( Light[0]->GetFacing(), 0, 12 );
	g_pLightViewMatrixVar->	SetMatrix( LightViewMatrixCalc( 0 ) );
	g_pLightProjMatrixVar->	SetMatrix( LightProjMatrixCalc( 0 ) );
	g_pLightConeAngleVar->		SetFloat( cos( ToRadians(LightConeAngle * 0.5f) ) ); // Shader wants cos(angle/2)
	g_pLightColourVar->		SetRawValue( LightColour[0], 0, 12 );

	g_pLight1PosVar->			SetRawValue( Light[1]->GetPosition(), 0, 12 );  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	g_pLight1FacingVar->		SetRawValue( Light[1]->GetFacing(), 0, 12 );
	g_pLight1ViewMatrixVar->	SetMatrix( LightViewMatrixCalc( 1 ) );
	g_pLight1ProjMatrixVar->	SetMatrix( LightProjMatrixCalc( 1 ) );
	g_pLight1ConeAngleVar->		SetFloat( cos( ToRadians(LightConeAngle * 0.5f) ) ); // Shader wants cos(angle/2)
	g_pLight1ColourVar->		SetRawValue( LightColour[1], 0, 12 );
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	g_pAmbientColourVar->SetRawValue( AmbientColour, 0, 12 );
	g_pSpecularPowerVar->SetFloat( SpecularPower );
	g_pCameraPosVar->SetRawValue( CameraPosition, 0, 12 );
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Render Shadow Maps

	D3D10_VIEWPORT ViewPort;
	ViewPort.Width = ShadowMapSize;
	ViewPort.Height = ShadowMapSize;
	ViewPort.MaxDepth = 1.0f;
	ViewPort.MinDepth = 0.0f;
	ViewPort.TopLeftX = 0;
	ViewPort.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &ViewPort );
	g_pd3dDevice->OMSetRenderTargets( 0, 0, DepthStencilView );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );
	RenderShadowMap( 0 );

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///Render Main
	
	ViewPort.Width  = g_ViewportWidth;
	ViewPort.Height = g_ViewportHeight;
	ViewPort.MinDepth = 0.0f;
	ViewPort.MaxDepth = 1.0f;
	ViewPort.TopLeftX = 0;
	ViewPort.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &ViewPort );

	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear the back buffer  and its depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &BackgroundColour[0] );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	RenderMain(Camera);
	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
}



//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	// Initialise everything in turn
	if( !InitWindow( hInstance, nCmdShow) )
	{
		return 0;
	}
	if( !InitDevice() || !LoadEffectFile() || !InitScene() )
	{
		ReleaseResources(); // Cleanup DirectX on failure
		return 0;
	}

	// Initialise simple input functions (in Input.h/.cpp, not part of DirectX)
	InitInput();

	// Initialise a timer class (in CTimer.h/.cpp, not part of DirectX). It's like a stopwatch - start it counting now
	CTimer Timer;
	Timer.Start();

	// Main message loop. The window will stay in this loop until it is closed
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		// First check to see if there are any messages that need to be processed for the window (window resizing, minimizing, whatever), if there are then deal with them
		// If not then the window is idle and the D3D rendering occurs. This is in a loop. So the window is rendered over and over, as fast as possible as long as we are
		// not manipulating the window in some way
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			// Deal with messages
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else // Otherwise render & update
		{
			RenderScene();

			// Get the time passed since the last frame (since the last time this line was reached) - used so the rendering and update can be
			// synchronised to real time and won't be dependent on machine speed
			float frameTime = Timer.GetLapTime();
			UpdateScene( frameTime );

			// Allow user to quit with escape key
			if (KeyHit( Key_Escape )) 
			{
				DestroyWindow( HWnd );
			}
		}
	}

	// Release all the resources we've created before leaving
	ReleaseResources();

	return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
bool InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
	if( !RegisterClassEx( &wcex ) )	return false;

	// Create window
	HInst = hInstance;
	RECT rc = { 0, 0, 960, 720 };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	HWnd = CreateWindow( L"TutorialWindowClass", L"CO2409 - Graphics Assignment - Shaders", WS_OVERLAPPEDWINDOW,
	                     CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL );
	if( !HWnd )	return false;

	ShowWindow( HWnd, nCmdShow );

	return true;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch( message )
	{
		case WM_PAINT:
			hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		// These windows messages (WM_KEYXXXX) can be used to get keyboard input to the window
		// This application has added some simple functions (not DirectX) to process these messages (all in Input.cpp/h)
		case WM_KEYDOWN:
			KeyDownEvent( static_cast<EKeyState>(wParam) );
			break;

		case WM_KEYUP:
			KeyUpEvent( static_cast<EKeyState>(wParam) );
			break;
		
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}
