//--------------------------------------------------------------------------------------
// File: ShadowMapping.fx
//
// Shadow mapping - rendering from the light's point of view to create shadows
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// This structure describes the vertex data to be sent into the vertex shader
struct VS_INPUT
{
    float3 Pos    : POSITION;
    float3 Normal : NORMAL;
	float2 UV     : TEXCOORD0;
};


// The vertex shader processes the input geometry above and outputs data to be used by the rest of the pipeline
struct VS_LIGHTING_OUTPUT
{
    float4 ProjPos       : SV_POSITION;  // 2D "projected" position for vertex (required output for vertex shader)
	float3 WorldPos      : POSITION;
	float3 WorldNormal   : NORMAL;
    float2 UV            : TEXCOORD0;
};


// More basic techniques don't deal with lighting. So the vertex shader output is different in those cases
struct VS_BASIC_OUTPUT
{
    float4 ProjPos       : SV_POSITION;  // 2D "projected" position for vertex (required output for vertex shader)
    float2 UV            : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////STRUCTURE FOR PHOTONS
/*
struct Photon
{
	float3 Origen	:POSITION;
	float3 Direction;
	int Bounce_Number;
	float3 Colour;
	float3 Dist;
};
*/

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// All these variables are created & manipulated in the C++ code and passed into the shader here

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;

// Spotlight information - a great deal of information needed for shadow mapping
float3   Light1Pos;
float3   Light1Facing;
float4x4 Light1ViewMatrix;
float4x4 Light1ProjMatrix;
float    Light1CosHalfAngle;
float3   Light1Colour;

float3   Light2Pos;
float3   Light2Facing;
float4x4 Light2ViewMatrix;
float4x4 Light2ProjMatrix;
float    Light2CosHalfAngle;
float3   Light2Colour;

// Other data relating to lighting
float3 AmbientColour;
float  SpecularPower;
float3 CameraPos; // needed for specular lighting

// Variable used to tint each light model to show the colour that it emits
float3 TintColour;


// Diffuse texture map and shadow map
Texture2D DiffuseMap;
Texture2D ShadowMap1;
Texture2D ShadowMap2;
Texture3D LightMap;

// Samplers to use with the above texture maps. Specifies texture filtering and addressing mode to use when accessing texture pixels
SamplerState TrilinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};
SamplerState PointClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

// The vertex shader will process each of the vertices in the model, typically transforming/projecting them into 2D at a minimum.
// This vertex shader passes on the vertex position and normal to the pixel shader for per-pixel lighting
//
VS_LIGHTING_OUTPUT LightingTransformTex( VS_INPUT vIn )
{
	VS_LIGHTING_OUTPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );

	// Transform the vertex normal from model space into world space (almost same as first lines of code above)
	float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
	vOut.WorldNormal = mul( modelNormal, WorldMatrix ).xyz;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;

	return vOut;
}


// Basic vertex shader to transform 3D model vertices to 2D and pass UVs to the pixel shader
//
VS_BASIC_OUTPUT BasicTransform( VS_INPUT vIn )
{
	VS_BASIC_OUTPUT vOut;
	
	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	
	// Pass texture coordinates (UVs) on to the pixel shader
	vOut.UV = vIn.UV;

	return vOut;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------

// The pixel shader determines colour for each pixel in the rendered polygons, given the data passed on from the vertex shader
// This shader expects vertex position, normal and UVs from the vertex shader. It per-pixel lighting with shadow mapping and diffuse/specular maps
//
float4 ShadowMapTex( VS_LIGHTING_OUTPUT vOut ) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Slight adjustment to calculated depth of pixels so they don't shadow themselves
	const float DepthAdjust = 0.0005f;

	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	float3 worldNormal = normalize(vOut.WorldNormal); 

	///////////////////////
	// Calculate lighting

	// Calculate direction of camera
	float3 cameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)

	//----------
	// LIGHT 1

	// Start with no light contribution from this light
	float3 diffuseLight1 = 0;
	float3 specularLight1 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 light1ViewPos = mul( float4(vOut.WorldPos, 1.0f), Light1ViewMatrix ); 
	float4 light1ProjPos = mul( light1ViewPos, Light1ProjMatrix );

	// Get direction from pixel to light
	float3 light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);

	// Check if pixel is within light cone
	if (dot( light1Dir, -Light1Facing ) > Light1CosHalfAngle)
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * light1ProjPos.xy / light1ProjPos.w + float2( 0.5f, 0.5f );
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = light1ProjPos.z / light1ProjPos.w;// - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		
		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap1.Sample( PointClamp, shadowUV ).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 light1Dist = length(Light1Pos - vOut.WorldPos.xyz); 
			diffuseLight1 = Light1Colour * max( dot(worldNormal.xyz, light1Dir), 0 ) / light1Dist;
			float3 halfway = normalize(light1Dir + cameraDir);
			specularLight1 = diffuseLight1 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );
		}
	}


	//----------
	// LIGHT 2

	// Same process as above for light 2

	// Start with no light contribution from this light
	float3 diffuseLight2 = 0;
	float3 specularLight2 = 0;

	// Get 2D position of pixel *as seen from light* using light's "camera" matrices
	float4 light2ViewPos = mul( float4(vOut.WorldPos, 1.0f), Light2ViewMatrix );
	float4 light2ProjPos = mul( light2ViewPos, Light2ProjMatrix );

	// Check if pixel is within light cone
	float3 light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
	if (dot( light2Dir, -Light2Facing ) > Light2CosHalfAngle)
	{
		// Get texture coordinates for shadow map
		float2 shadowUV = 0.5f * light2ProjPos.xy / light2ProjPos.w + float2( 0.5f, 0.5f );
		shadowUV.y = 1.0f - shadowUV.y;

		// See if this pixel is visible to this light by testing the shadow map
		float depthFromLight = light2ProjPos.z / light2ProjPos.w;// - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		if (depthFromLight < ShadowMap2.Sample( PointClamp, shadowUV ).r)
		{
			float3 light2Dist = length(Light2Pos - vOut.WorldPos.xyz); 
			diffuseLight2 = Light2Colour * max( dot(worldNormal.xyz, light2Dir), 0 ) / light2Dist;
			float3 halfway = normalize(light2Dir + cameraDir);
			specularLight2 = diffuseLight2 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );
		}
	}

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 diffuseLight = AmbientColour + diffuseLight1 + diffuseLight2;
	float3 specularLight = specularLight1 + specularLight2;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMaterial = DiffuseMap.Sample( TrilinearWrap, vOut.UV );
	
	// Get specular material colour from texture alpha
	float3 specularMaterial = diffuseMaterial.a;

	
	////////////////////
	// Combine colours 
	
	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = diffuseMaterial * diffuseLight + specularMaterial * specularLight;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

float4 LightShadowMapTex( VS_LIGHTING_OUTPUT vOut ) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Slight adjustment to calculated depth of pixels so they don't shadow themselves
	const float DepthAdjust = 0.0005f;

	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	float3 worldNormal = normalize(vOut.WorldNormal); 

	///////////////////////
	// Calculate lighting

	// Calculate direction of camera
	float3 cameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)

	//----------
	// LIGHT 1

	// Start with no light contribution from this light
	float3 diffuseLight1 = 0;
	float3 specularLight1 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 light1ViewPos = mul( float4(vOut.WorldPos, 1.0f), Light1ViewMatrix ); 
	float4 light1ProjPos = mul( light1ViewPos, Light1ProjMatrix );

	// Get direction from pixel to light
	float3 light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);

	// Check if pixel is within light cone
	if (dot( light1Dir, -Light1Facing ) > Light1CosHalfAngle)
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * light1ProjPos.xy / light1ProjPos.w + float2( 0.5f, 0.5f );
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = light1ProjPos.z / light1ProjPos.w;// - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		
		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap1.Sample( PointClamp, shadowUV ).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 light1Dist = length(Light1Pos - vOut.WorldPos.xyz); 
			diffuseLight1 = Light1Colour * max( dot(worldNormal.xyz, light1Dir), 0 ) / light1Dist;
			float3 halfway = normalize(light1Dir + cameraDir);
			specularLight1 = diffuseLight1 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );
		}
	}


	//----------
	// LIGHT 2

	// Same process as above for light 2

	// Start with no light contribution from this light
	float3 diffuseLight2 = 0;
	float3 specularLight2 = 0;

	// Get 2D position of pixel *as seen from light* using light's "camera" matrices
	float4 light2ViewPos = mul( float4(vOut.WorldPos, 1.0f), Light2ViewMatrix );
	float4 light2ProjPos = mul( light2ViewPos, Light2ProjMatrix );

	// Check if pixel is within light cone
	float3 light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
	if (dot( light2Dir, -Light2Facing ) > Light2CosHalfAngle)
	{
		// Get texture coordinates for shadow map
		float2 shadowUV = 0.5f * light2ProjPos.xy / light2ProjPos.w + float2( 0.5f, 0.5f );
		shadowUV.y = 1.0f - shadowUV.y;

		// See if this pixel is visible to this light by testing the shadow map
		float depthFromLight = light2ProjPos.z / light2ProjPos.w;// - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		if (depthFromLight < ShadowMap2.Sample( PointClamp, shadowUV ).r)
		{
			float3 light2Dist = length(Light2Pos - vOut.WorldPos.xyz); 
			diffuseLight2 = Light2Colour * max( dot(worldNormal.xyz, light2Dir), 0 ) / light2Dist;
			float3 halfway = normalize(light2Dir + cameraDir);
			specularLight2 = diffuseLight2 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// aquire the colour of the related pixel in the light map here. this can then be used in place of Ambiant Colour below//////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 diffuseLight = AmbientColour + diffuseLight1 + diffuseLight2;
	float3 specularLight = specularLight1 + specularLight2;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMaterial = DiffuseMap.Sample( TrilinearWrap, vOut.UV );
	
	// Get specular material colour from texture alpha
	float3 specularMaterial = diffuseMaterial.a;

	
	////////////////////
	// Combine colours 
	
	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = diffuseMaterial * diffuseLight + specularMaterial * specularLight;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////Initial attempt at photon bouncing algorythm, failed due to shaders only consiudering one object at a time, 
/*float4 GlobalIllumination( VS_LIGHTING_OUTPUT vOut ) : SV_Target
{
	float3 light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);

	// Check if pixel is within light cone
	if (dot( light1Dir, -Light1Facing ) > Light1CosHalfAngle)
	{ 
		Photon.Origen = Light1Pos;
		Photon.Direction = light1Dir;
		Photon.Colour = Light1Colour;
		Photon.Bounce = 0;
		Photon.Dist = length(Photon.Origen - vOut.WorldPos.xyz);
		while(Photon.Bounce < 1)
		{
			Photon.Origen = worldNormal.xyz;
			Photon.Direction = normalize(reflect(Photon.Direction, worldNormal.xyz));
			Photon.Colour = (Photon.Colour * max( dot(worldNormal.xyz, Photon.Direction), 0 ) / Photon.Dist;
			Photon.Bounce += 1; 
			combinedColour.rgb = diffuseMaterial * Photon.Colour;
			combinedColour.a = 1.0f;
		} 
	}
}*/


// A pixel shader that just tints a (diffuse) texture with a fixed colour
//
float4 TintDiffuseMap( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMapColour = DiffuseMap.Sample( TrilinearWrap, vOut.UV );

	// Tint by global colour (set from C++)
	diffuseMapColour.rgb *= TintColour / 10;

	return diffuseMapColour;
}

// Shader used when rendering the shadow map depths. In fact a pixel shader isn't needed, we are
// only writing to the depth buffer. However, needed to display what's in a shadow map (which we
// do as one of the exercises).
float4 PixelDepth( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	// Output the value that would go in the depth puffer to the pixel colour (greyscale)
	return vOut.ProjPos.z / vOut.ProjPos.w;
}


//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

// States are needed to switch between additive blending for the lights and no blending for other models

RasterizerState CullNone  // Cull none of the polygons, i.e. show both sides
{
	CullMode = None;
};
RasterizerState CullBack  // Cull back side of polygon - normal behaviour, only show front of polygons
{
	CullMode = Back;
};
RasterizerState CullFront  // Cull front side of polygon - unusual behaviour, show inside of model
{
	CullMode = Front;
};


DepthStencilState DepthWritesOff // Don't write to the depth buffer - polygons rendered will not obscure other polygons
{
	DepthWriteMask = ZERO;
};
DepthStencilState DepthWritesOn  // Write to the depth buffer - normal behaviour 
{
	DepthWriteMask = ALL;
};


BlendState NoBlending // Switch off blending - pixels will be opaque
{
    BlendEnable[0] = FALSE;
};

BlendState AdditiveBlending // Additive blending is used for lighting effects
{
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
};


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

// Shadow mapping with textures
technique10 ShadowMappingTex
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, LightingTransformTex() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, ShadowMapTex() ) );

		// Switch off blending states
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullBack ); 
		SetDepthStencilState( DepthWritesOn, 0 );
	}
}


// Additive blended texture. No lighting, but uses a global colour tint. Used for light models
technique10 AdditiveTexTint
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, TintDiffuseMap() ) );

		// States required for additive blending
		SetBlendState( AdditiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullNone ); 
		SetDepthStencilState( DepthWritesOff, 0 );
     }
}


// Rendering a shadow map. Only outputs the depth of each pixel
technique10 DepthOnly
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, PixelDepth() ) );

		// Switch off blending states
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullFront ); 
		SetDepthStencilState( DepthWritesOn, 0 );
     }
}
