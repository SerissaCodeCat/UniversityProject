//--------------------------------------------------------------------------------------
//	ShadowMapping.cpp
//
//	Shadow mapping - rendering from the light's point of view to create shadows
//--------------------------------------------------------------------------------------
#include "Linker.h" //linker file to keep all includes in one place.


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

// Models and cameras
CModel* Sphere [10]; 
//CModel* Troll;
//CModel* Crate;
CModel* Floor;
CCamera* MainCamera;


// Textures
ID3D10ShaderResourceView* SphereDiffuseMap = NULL;
ID3D10ShaderResourceView* TrollDiffuseMap = NULL;
ID3D10ShaderResourceView* CrateDiffuseMap = NULL;
ID3D10ShaderResourceView* FloorDiffuseMap = NULL;
ID3D10ShaderResourceView* LightDiffuseMap = NULL;


//**** Shadow Maps ****//
// Very similar data to the render-to-texture (portal) lab

// Width and height of shadow map - controls resolution/quality of shadows
int ShadowMapSize  = 1024.0f;

////////LIGHT MAP to be used in a simmilar way. however because the texture will be a 3D texture as opposed to 2D the ressolution must be lower.
int LightMapSize = 128;
const int NUM_LIGHTS = 2;
struct Photon
	{
		D3DXVECTOR3 pos; //photon possition
		D3DXVECTOR3 origin; // photon origen xyz.
		D3DXVECTOR3 dir; //photon Dirrection
		D3DXVECTOR3 RGB; // photon Power in relevant colours		
		bool flag; //flag in the K/D tree.
	};
const int NumberOfPhotons = 5000;
Photon PhotonMap[3*NumberOfPhotons][NUM_LIGHTS];


// The shadow map textures and the view of it as a depth buffer and shader resource (see code comments)
ID3D10Texture2D*          ShadowMap1Texture = NULL;
ID3D10DepthStencilView*   ShadowMap1DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap1 = NULL;
ID3D10Texture2D*          ShadowMap2Texture = NULL;
ID3D10DepthStencilView*   ShadowMap2DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap2 = NULL;

ID3D10Texture3D*          LightMapTexture = NULL;

//*********************//


//**** Light Data ****//

// Fixed light data
D3DXVECTOR4 BackgroundColour = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );
D3DXVECTOR3 AmbientColour    = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
float SpecularPower = 512.0f;

// Spotlight data - using spotlights in this lab because shadow mapping needs to treat each light as a camera, which is easy with
// spotlights as they are quite camera-like already. Using arrays of data to store lights in this lab for convinience

D3DXVECTOR3 LightColours[NUM_LIGHTS] = { D3DXVECTOR3( 0.8f, 0.8f, 1.0f ) * 8, D3DXVECTOR3( 1.0f, 0.8f, 0.2f ) * 30 }; // Colour * Intensity
CModel*     Lights[NUM_LIGHTS];
float       SpotlightConeAngle = 90.0f; // Spot light cone angle (degrees), like the FOV (field-of-view) of the spot light

// One light will follow an orbit
const float LightOrbitRadius = 100.0f;
const float LightOrbitSpeed  = 0.7f;

//*********************//

// Note: There are move & rotation speed constants in Defines.h


//--------------------------------------------------------------------------------------
// Shader Variables
//--------------------------------------------------------------------------------------
// Variables to connect C++ code to HLSL shaders

// Effects / techniques
ID3D10Effect*          Effect = NULL;
ID3D10EffectTechnique* ShadowMappingTechnique = NULL;
ID3D10EffectTechnique* AdditiveTexTintTechnique = NULL;
ID3D10EffectTechnique* DepthOnlyTechnique = NULL;

// Matrices
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;

// Textures
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
ID3D10EffectShaderResourceVariable* ShadowMap1Var = NULL; // Pass shadow maps to pixel shader to test if pixels are in shadow
ID3D10EffectShaderResourceVariable* ShadowMap2Var = NULL; // Pass shadow maps to pixel shader to test if pixels are in shadow
ID3D10EffectShaderResourceVariable* LightMapVar = NULL; // Pass shadow map to pixel shader to add ambiant light to pixels


// Light effect variables
ID3D10EffectVectorVariable* CameraPosVar = NULL;
ID3D10EffectVectorVariable* Light1PosVar = NULL;
ID3D10EffectVectorVariable* Light1FacingVar = NULL;
ID3D10EffectMatrixVariable* Light1ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* Light1ProjMatrixVar = NULL;
ID3D10EffectScalarVariable* Light1ConeAngleVar = NULL;
ID3D10EffectVectorVariable* Light1ColourVar = NULL;
ID3D10EffectVectorVariable* Light2PosVar = NULL;
ID3D10EffectVectorVariable* Light2FacingVar = NULL;
ID3D10EffectMatrixVariable* Light2ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* Light2ProjMatrixVar = NULL;
ID3D10EffectScalarVariable* Light2ConeAngleVar = NULL;
ID3D10EffectVectorVariable* Light2ColourVar = NULL;
ID3D10EffectVectorVariable* AmbientColourVar = NULL;
ID3D10EffectScalarVariable* SpecularPowerVar = NULL;

// Other 
ID3D10EffectVectorVariable* TintColourVar = NULL;


//--------------------------------------------------------------------------------------
// DirectX Variables
//--------------------------------------------------------------------------------------

// The main D3D interface, this pointer is used to access most D3D functions (and is shared across all cpp files through Defines.h)
ID3D10Device* g_pd3dDevice = NULL;

// Variables used to setup D3D
IDXGISwapChain*         SwapChain = NULL;
ID3D10Texture2D*        DepthStencil = NULL;
//ID3D10Texture3D*		LightMap = NULL;/////////////////////////////////////////////////////////////////////////////////A 3D texture used to allow the photons impact on the scene to be stored 
ID3D10DepthStencilView* DepthStencilView = NULL;
ID3D10RenderTargetView* BackBufferRenderTarget = NULL;

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
void RenderMain( CCamera* camera );
void RenderShadowMap( int lightNum );
void RenderScene();
bool InitWindow( HINSTANCE hInstance, int nCmdShow );
void Emit_Photons_From_Directional_Light(int lightNum);
void CollisionDetect(Photon incident, CModel* Test);
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
	sd.SampleDesc.Quality = 1;
	sd.OutputWindow = HWnd;                            // Target window
	sd.Windowed = TRUE;                                // Whether to render in a window (TRUE) or go fullscreen (FALSE)
	if (FAILED( D3D10CreateDeviceAndSwapChain( NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &sd, &SwapChain, &g_pd3dDevice ) )) return false;


	// Here indicate that the back-buffer can be "viewed" as a render target - rendering to the back buffer is standard behaviour, so this code is standard
	ID3D10Texture2D* pBackBuffer;
	if (FAILED( SwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer ) )) return false;
	hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &BackBufferRenderTarget );
	pBackBuffer->Release();
	if( FAILED( hr ) ) return false;


	// Create a texture (bitmap) to use for a depth buffer for the main viewport
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = g_ViewportWidth;
	descDepth.Height = g_ViewportHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 1;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	if( FAILED( g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &DepthStencil ) )) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	if( FAILED( g_pd3dDevice->CreateDepthStencilView( DepthStencil, NULL, &DepthStencilView ) )) return false;

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

	// Using an array of lights in this lab
	for (int i = 0; i < NUM_LIGHTS; i++) 
	{
		delete Lights[i];
	}
	for (int i = 0; i<10; i++)
	{
		delete Sphere[i];
	}
	delete Floor;
	//delete Crate;
	//delete Troll;
	delete MainCamera;

	if (ShadowMap1)             ShadowMap1->Release();
	if (ShadowMap1DepthView)    ShadowMap1DepthView->Release();
	if (ShadowMap1Texture)      ShadowMap1Texture->Release();
    if (FloorDiffuseMap)		FloorDiffuseMap->Release();
	if (SphereDiffuseMap)		SphereDiffuseMap->Release();
    if (LightDiffuseMap)        LightDiffuseMap->Release();
    if (CrateDiffuseMap)        CrateDiffuseMap->Release();
    if (TrollDiffuseMap)        TrollDiffuseMap->Release();
	if (Effect)                 Effect->Release();
	if (DepthStencilView)       DepthStencilView->Release();
	if (BackBufferRenderTarget) BackBufferRenderTarget->Release();
	if (DepthStencil)           DepthStencil->Release();
	//if (LightMap)				LightMap->Release();
	if (LightMapTexture)		LightMapTexture->Release();
	if (SwapChain)              SwapChain->Release();
	if (g_pd3dDevice)           g_pd3dDevice->Release();
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
	if( FAILED( D3DX10CreateEffectFromFile( L"ShadowMapping.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &Effect, &pErrors, NULL ) ))
	{
		if (pErrors != 0)  MessageBox( NULL, CA2CT(reinterpret_cast<char*>(pErrors->GetBufferPointer())), L"Error", MB_OK ); // Compiler error: display error message
		else               MessageBox( NULL, L"Error loading FX file. Ensure your FX file is in the same folder as this executable.", L"Error", MB_OK );  // No error message - probably file not found
		return false;
	}

	// Now we can select techniques from the compiled effect file
	ShadowMappingTechnique   = Effect->GetTechniqueByName( "ShadowMappingTex" );
	AdditiveTexTintTechnique = Effect->GetTechniqueByName( "AdditiveTexTint" );
	DepthOnlyTechnique       = Effect->GetTechniqueByName( "DepthOnly" );
	
	// Create special variables to allow us to access global variables in the shaders from C++
	WorldMatrixVar    = Effect->GetVariableByName( "WorldMatrix"    )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName( "ViewMatrix"     )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName( "ProjMatrix"     )->AsMatrix();
	ViewProjMatrixVar = Effect->GetVariableByName( "ViewProjMatrix" )->AsMatrix();

	// Access textures variables used in the shader (not referred to as textures - any GPU memory accessed in a shader is a "Shader Resource")
	DiffuseMapVar = Effect->GetVariableByName( "DiffuseMap" )->AsShaderResource();
	ShadowMap1Var = Effect->GetVariableByName( "ShadowMap1" )->AsShaderResource(); // Will pass shadow maps over to pixel shader
	ShadowMap2Var = Effect->GetVariableByName( "ShadowMap2" )->AsShaderResource(); // Will pass shadow maps over to pixel shader

	// Also access shader variables needed for lighting
	CameraPosVar     = Effect->GetVariableByName( "CameraPos"     )->AsVector();
	Light1PosVar     = Effect->GetVariableByName( "Light1Pos"     )->AsVector();
	Light1ColourVar  = Effect->GetVariableByName( "Light1Colour"  )->AsVector();
	Light2PosVar     = Effect->GetVariableByName( "Light2Pos"     )->AsVector();
	Light2ColourVar  = Effect->GetVariableByName( "Light2Colour"  )->AsVector();
	AmbientColourVar = Effect->GetVariableByName( "AmbientColour" )->AsVector();
	SpecularPowerVar = Effect->GetVariableByName( "SpecularPower" )->AsScalar();
	Light1FacingVar  = Effect->GetVariableByName( "Light1Facing"  )->AsVector();
	Light2FacingVar  = Effect->GetVariableByName( "Light2Facing"  )->AsVector();
	Light1ViewMatrixVar = Effect->GetVariableByName( "Light1ViewMatrix"   )->AsMatrix();
	Light2ViewMatrixVar = Effect->GetVariableByName( "Light2ViewMatrix"   )->AsMatrix();
	Light1ProjMatrixVar = Effect->GetVariableByName( "Light1ProjMatrix"   )->AsMatrix();
	Light2ProjMatrixVar = Effect->GetVariableByName( "Light2ProjMatrix"   )->AsMatrix();
	Light1ConeAngleVar  = Effect->GetVariableByName( "Light1CosHalfAngle" )->AsScalar();
	Light2ConeAngleVar  = Effect->GetVariableByName( "Light2CosHalfAngle" )->AsScalar();

	// Other shader variables
	TintColourVar = Effect->GetVariableByName( "TintColour"  )->AsVector();

	return true;
}



//--------------------------------------------------------------------------------------
// Light Helper Functions
//--------------------------------------------------------------------------------------

// Get "camera-like" view matrix for a spot light
D3DXMATRIXA16 CalculateLightViewMatrix( int lightNum )
{
	D3DXMATRIXA16 viewMatrix;
	
	// Get the world matrix of the light model and invert it to get the view matrix (that is more-or-less the definition of a view matrix)
	// We don't always have a physical model for a light, in which case we would need to store this data along with the light colour etc.
	D3DXMATRIXA16 worldMatrix = Lights[lightNum]->GetWorldMatrix();
	D3DXMatrixInverse( &viewMatrix, NULL, &worldMatrix );

	return viewMatrix;
}

// Get "camera-like" projection matrix for a spot light
D3DXMATRIXA16 CalculateLightProjMatrix( int lightNum )
{
	D3DXMATRIXA16 projMatrix;

	// Create a projection matrix for the light. Use the spotlight cone angle as an FOV, just set default values for everything else.
	D3DXMatrixPerspectiveFovLH( &projMatrix, ToRadians( SpotlightConeAngle ), 1, 0.1f, 1000.0f);

	return projMatrix;
}


//--------------------------------------------------------------------------------------
// Scene Setup / Update
//--------------------------------------------------------------------------------------

// Create / load the camera, models and textures for the scene
bool InitScene()
{
	//-----------------
	// Create cameras

	// Main camera is the view shown in the viewport (the screen)
	MainCamera = new CCamera();
	MainCamera->SetPosition( D3DXVECTOR3(15, 30,-70) );
	MainCamera->SetRotation( D3DXVECTOR3(ToRadians(13.0f), ToRadians(0.0f), 0.0f) ); // ToRadians is a new helper function to convert degrees to radians


	//--------------------
	// Load/Create models

	//Troll  = new CModel;
	//Crate  = new CModel;
	Floor = new CModel;

	// Load .X files for each model
	//if (!Troll-> Load( "Troll.x",  ShadowMappingTechnique )) return false;
	//if (!Crate-> Load( "CargoContainer.x", ShadowMappingTechnique )) return false;
	if (!Floor->Load( "Floor.x", ShadowMappingTechnique )) return false;

	// Initial model positions
	//Troll->SetPosition( D3DXVECTOR3(15, 0, 0) );
	//Troll->SetScale( 6.0f );
	//Troll->SetRotation( D3DXVECTOR3(0.0f, ToRadians(215.0f), 0.0f) );
	//Crate->SetPosition( D3DXVECTOR3(40, 0, 30) );
	//Crate->SetScale( 6.0f );
	//Crate->SetRotation( D3DXVECTOR3(0.0f, ToRadians(-20.0f), 0.0f) );

	// Using an array of lights in this lab
	for (int i = 0; i < NUM_LIGHTS; i++) 
	{
		Lights[i] = new CModel;
		if (!Lights[i]->Load( "Light.x", AdditiveTexTintTechnique )) return false;

	}
	for(int i = 0; i<10; i++)
	{
		Sphere[i] = new CModel;
		if (!Sphere[i]->Load( "Sphere.x", ShadowMappingTechnique )) return false;
		Sphere[i]->SetPosition(D3DXVECTOR3(15*i,5*i+10, i*15));
	}

	// The lights are spotlights - so they need to face in a particular direction too (using the model holds the light's positioning data)
	Lights[0]->SetPosition( D3DXVECTOR3(30, 20, 0) );
	Lights[0]->SetScale( 4.0f );
	Lights[0]->FacePoint( Sphere[0]->GetPosition() );
	Lights[1]->SetPosition( D3DXVECTOR3(-20, 30, 20) );
	Lights[1]->SetScale( 8.0f );
	Lights[1]->FacePoint( D3DXVECTOR3( 0, 0, 0 ) );


	//----------------
	// Load textures

	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"TrollDiffuseSpecular.dds", NULL, NULL, &TrollDiffuseMap,  NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"CargoA.dds",               NULL, NULL, &CrateDiffuseMap,  NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"WoodDiffuseSpecular.dds", NULL, NULL, &FloorDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"flare.jpg",                NULL, NULL, &LightDiffuseMap,  NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &SphereDiffuseMap,  NULL ) )) return false;


	////////////////////////
	//**** Shadow Maps ****//

	// Create the shadow map textures, above we used a D3DX... helper function to create basic textures in one line. Here, we need to
	// do things manually as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.Width  = ShadowMapSize; // Size of the shadow map determines quality / resolution of shadows
	texDesc.Height = ShadowMapSize;
	texDesc.MipLevels = 1; // 1 level, means just the main texture, no additional mip-maps. Usually don't use mip-maps when rendering to textures (or we would have to render every level)
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // The shadow map contains a single 32-bit value [tech gotcha: have to say typeless because depth buffer and texture see things slightly differently]
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and will also pass it to shaders
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	if (FAILED(g_pd3dDevice->CreateTexture2D( &texDesc, NULL, &ShadowMap1Texture ) )) return false;

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

	if (FAILED(g_pd3dDevice->CreateTexture2D( &texDesc, NULL, &ShadowMap2Texture ) )) return false;
	if (FAILED(g_pd3dDevice->CreateDepthStencilView( ShadowMap2Texture, &descDSV, &ShadowMap2DepthView ) )) return false;
	if (FAILED( g_pd3dDevice->CreateShaderResourceView( ShadowMap2Texture, &srDesc, &ShadowMap2 ) )) return false;

	//*****************************//

	return true;
}

void CollisionDetect(Photon &incident, CModel* &Test)
{
	float r = 15.0f; //Radius of sphere 
	D3DXVECTOR3 RelativePossition = Test->GetPosition() - incident.pos;
	D3DXVECTOR3 SpherePos = Test->GetPosition();
	float pp = RelativePossition.x*RelativePossition.x + RelativePossition.y*RelativePossition.y + RelativePossition.z*RelativePossition.z - r*r;
	if(pp < 0)
	{
		return;// return and leave the look if the sphere has already been drunk
	}

	
}

void Emit_Photons_From_Directional_Light(int lightNum)
{
	D3D10_TEXTURE3D_DESC LightDesc;
	LightDesc.Width  = LightMapSize; // Size of the shadow map determines quality / resolution of shadows
	LightDesc.Depth = LightMapSize;
	LightDesc.Height = LightMapSize;
	LightDesc.MipLevels = 1;
	LightDesc.Usage = D3D10_USAGE_DEFAULT;
	LightDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and will also pass it to shaders
	
	if (FAILED(g_pd3dDevice->CreateTexture3D( &LightDesc, NULL, &LightMapTexture ) )) return;
	
	float x;
	float y;
	float z;
	float Length;
	D3DXVECTOR3 NormVector;
	D3DXVECTOR3 LightFacing = Lights[lightNum]->GetFacing();
	int Emitted = 0; //number of photons emmited this call
	while (Emitted < NumberOfPhotons)
	{
		do
		{
			x = (rand() % 100+1)/100;
			y = (rand() % 100+1)/100;
			z = (rand() % 100+1)/100;
			Length = sqrt((x * x)+(y * y)+(z*z)); 
			NormVector = D3DXVECTOR3 ((x/Length),(y/Length),(z/Length));
		}while(((LightFacing.x*NormVector.x)+(LightFacing.y*NormVector.y)+(LightFacing.z*NormVector.z))<=0);// uses dot product as simple rejection sampling.
		//if the light photon would be emmited at equal to or greater than 90 degrees to the lights facing dirrection, reject the sample
		//this maintains the lights spot lite functionality
		
		PhotonMap[Emitted*3][lightNum].dir = D3DXVECTOR3(x,y,z);
		PhotonMap[Emitted*3][lightNum].origin = Lights[lightNum]->GetPosition();
		PhotonMap[Emitted*3][lightNum].RGB = LightColours[lightNum];
		//////////////////////////
		//INSERT TRACING CALL HERE
		for(int i = 0; i<10; i++)
		{
			CollisionDetect((PhotonMap[Emitted*3][lightNum]), Sphere[i]);
		}
		Emitted += 1;
	}
}

// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (view matrix, projection matrix) each frame
	// Don't be deceived into thinking that this is a new method to control models - the same code we used previously is used in the CCamera class
	MainCamera->Control( frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
	MainCamera->UpdateMatrices();
	
	// Control troll position and update its world matrix each frame
//	Troll->Control( frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );
//	Troll->UpdateMatrix();

	// Objects that don't move still need a world matrix - could do this in SceneSetup function (which occurs once at the start of the app)
//	Crate->UpdateMatrix();
	Floor->UpdateMatrix();
	for(int i = 0; i<10; i++ )
	{
		Sphere[i]->UpdateMatrix();
	}
	// Update the lights. The orbiting light uses a cheat with the static variable [ask the tutor if you want to know what this is]
	static bool Orbiting = true;
	static float Rotate = 0.0f;
	Lights[0]->SetPosition( Sphere[3]->GetPosition() + D3DXVECTOR3(cos(Rotate)*LightOrbitRadius, 20, sin(Rotate)*LightOrbitRadius) );
	if (Orbiting) Rotate -= LightOrbitSpeed * frameTime;
	Lights[0]->FacePoint( Sphere[0]->GetPosition() ); // The troll is in the spotlight...
	if (KeyHit( Key_1 )) Orbiting = !Orbiting;
	Lights[0]->UpdateMatrix();
	Lights[1]->UpdateMatrix();
}


//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render all the models from the point of view of the given camera
void RenderMain( CCamera* camera )
{
	// Pass the camera's matrices to the vertex shader and position to the vertex shader
	ViewMatrixVar->SetMatrix( camera->GetViewMatrix() );
	ProjMatrixVar->SetMatrix( camera->GetProjectionMatrix() );
	CameraPosVar->SetRawValue( camera->GetPosition(), 0, 12 );

	// Send the shadow map rendered in the function below to the shader
    ShadowMap1Var->SetResource( ShadowMap1 );
    ShadowMap2Var->SetResource( ShadowMap2 );

	/*
	// Render troll
	WorldMatrixVar->SetMatrix( Troll->GetWorldMatrix() ); // Send the troll's world matrix to the shader
    DiffuseMapVar->SetResource( TrollDiffuseMap );        // Send the troll's diffuse/specular map to the shader
	Troll->Render( ShadowMappingTechnique );              // Pass rendering technique to the model class

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix( Crate->GetWorldMatrix() );
    DiffuseMapVar->SetResource( CrateDiffuseMap );
	Crate->Render( ShadowMappingTechnique );
	*/
	for(int i = 0; i<10; i++)
	{
		WorldMatrixVar->SetMatrix( Sphere[i]->GetWorldMatrix() );
		DiffuseMapVar->SetResource( SphereDiffuseMap );
		Sphere[i]->Render( ShadowMappingTechnique );
	}
	WorldMatrixVar->SetMatrix( Floor->GetWorldMatrix() );
    DiffuseMapVar->SetResource( FloorDiffuseMap );
	Floor->Render( ShadowMappingTechnique );

	// Using an array of lights in this lab
	for (int i = 0; i < NUM_LIGHTS; i++) 
	{
		WorldMatrixVar->SetMatrix( (float*)Lights[i]->GetWorldMatrix() );
		DiffuseMapVar->SetResource( LightDiffuseMap );
		TintColourVar->SetRawValue( LightColours[i], 0, 12 ); // Using special shader that tints the light model to match the light colour
		Lights[i]->Render( AdditiveTexTintTechnique );
	}
}

// Render a shadow map for the given light number: render only the models that can obscure other objects, using a special depth-rendering
// shader technique. Assumes the shadow map texture is already set as a render target.
void RenderShadowMap( int lightNum )
{
	//---------------------------------
	// Set "camera" matrices in shader

	// Pass the light's "camera" matrices to the vertex shader - use helper functions above to turn spotlight settings into "camera" matrices
	ViewMatrixVar->SetMatrix( CalculateLightViewMatrix( lightNum ) );
	ProjMatrixVar->SetMatrix( CalculateLightProjMatrix( lightNum ) );


	//-----------------------------------
	// Render each model into shadow map

	/*
	// Render troll - no need to set its texture as shadow maps just render to the depth buffer
	WorldMatrixVar->SetMatrix( Troll->GetWorldMatrix() );
	Troll->Render( DepthOnlyTechnique );  // Use special rendering technique to render depths only

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix( Crate->GetWorldMatrix() );
	Crate->Render( DepthOnlyTechnique );
	*/
	for(int i = 0; i<10; i++ )
	{
		WorldMatrixVar->SetMatrix( Sphere[i]->GetWorldMatrix() );
		Sphere[i]->Render( DepthOnlyTechnique );
	}

	WorldMatrixVar->SetMatrix( Floor->GetWorldMatrix() );
	Floor->Render( DepthOnlyTechnique );
}


// Render everything in the scene
void RenderScene()
{
	//---------------------------
	// Common rendering settings

	// There are some common features all models that we will be rendering, set these once only

	// Pass light information to the vertex shader
	// A great deal of information required for shadow mapping
	Light1PosVar->       SetRawValue( Lights[0]->GetPosition(), 0, 12 );  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light2PosVar->       SetRawValue( Lights[1]->GetPosition(), 0, 12 );
	Light1FacingVar->    SetRawValue( Lights[0]->GetFacing(), 0, 12 );
	Light2FacingVar->    SetRawValue( Lights[1]->GetFacing(), 0, 12 );
	Light1ViewMatrixVar->SetMatrix( CalculateLightViewMatrix( 0 ) );
	Light2ViewMatrixVar->SetMatrix( CalculateLightViewMatrix( 1 ) );
	Light1ProjMatrixVar->SetMatrix( CalculateLightProjMatrix( 0 ) );
	Light2ProjMatrixVar->SetMatrix( CalculateLightProjMatrix( 1 ) );
	Light1ConeAngleVar-> SetFloat( cos( ToRadians(SpotlightConeAngle * 0.5f) ) ); // Shader wants cos(angle/2)
	Light2ConeAngleVar-> SetFloat( cos( ToRadians(SpotlightConeAngle * 0.5f) ) );
	Light1ColourVar->    SetRawValue( LightColours[0], 0, 12 );
	Light2ColourVar->    SetRawValue( LightColours[1], 0, 12 );
	AmbientColourVar->   SetRawValue( AmbientColour, 0, 12 );
	SpecularPowerVar->   SetFloat( SpecularPower );


	//---------------------------
	// Render shadow maps

	// Setup the viewport - defines which part of the shadow map we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = ShadowMapSize;
	vp.Height = ShadowMapSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the shadow map texture to use for rendering as a depth buffer. We will not be rendering any pixel colours
	g_pd3dDevice->OMSetRenderTargets( 0, 0, ShadowMap1DepthView );

	// Clear the shadow map texture (it's a depth buffer)
	g_pd3dDevice->ClearDepthStencilView( ShadowMap1DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Render everything from point of view of light 0 (into the shadow map selected in the lines above)
	RenderShadowMap( 0 );

	g_pd3dDevice->OMSetRenderTargets( 0, 0, ShadowMap2DepthView );
	g_pd3dDevice->ClearDepthStencilView( ShadowMap2DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0 );
	RenderShadowMap( 1 );

	//---------------------------
	// Render main scene

	// Setup the viewport within - defines which part of the back-buffer we will render to (usually all of it)
	vp.Width  = g_ViewportWidth;
	vp.Height = g_ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear the back buffer  and its depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &BackgroundColour[0] );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Render everything from the main camera's point of view
	RenderMain( MainCamera );
//	RenderShadowMap( 0 );


	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
	Emit_Photons_From_Directional_Light(0);
	Emit_Photons_From_Directional_Light(1);
}



////////////////////////////////////////////////////////////////////////////////////////
// Window Setup
////////////////////////////////////////////////////////////////////////////////////////

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
	HWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 10: Shadow Mapping", WS_OVERLAPPEDWINDOW,
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
