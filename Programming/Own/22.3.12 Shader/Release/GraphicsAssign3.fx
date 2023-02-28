//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// Standard input geometry data, more complex techniques (e.g. normal mapping) may need more
struct VS_BASIC_INPUT
{
    float3 Pos    : POSITION;
	float3 Normal : NORMAL;
	float2 UV     : TEXCOORD0;
};

// a remade output format to accomodate lighting//
struct VS_LIGHTING_OUTPUT
{
	float4 ProjPos      : SV_POSITION;
	float3 DiffuseLight :COLOR0;
	float3 SpecularLight:COLOR1;
	float2 UV           :TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// All these variables are created & manipulated in the C++ code and passed into the shader here

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;
//////////////////////////////////////////////////////////////////////////////////////////
///////////////////TEXTURE VARIABLES//////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
float3 LightPos;
float3 LightColour;
float3 AmbientColour;
float  SpecularPower;
float3 CameraPos;
float3 ModelColour;
Texture2D DiffuseMap;

SamplerState Tri
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
    MaxLOD = 0;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------




VS_BASIC_OUTPUT LitTransform( VS_BASIC_INPUT vIn )
{
	VS_LIGHTING_OUTPUT vOut;
	
	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
	float4 worldNormal = mul( modelNormal, WorldMatrix );
	worldNormal = normalize(worldNormal); 

	// Calculate direction of light and camera
	float3 CameraDir = normalize(CameraPos - worldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)
	float3 LightDir = normalize(LightPos - worldPos.xyz);   // Same for light
	
	// Calculate lighting on this vertex (or pixel) - equations from lecture
	vOut.DiffuseLight = AmbientColour + LightColour * max( dot(worldNormal.xyz, LightDir), 0 );
	float3 halfway = normalize(LightDir + CameraDir);
	vOut.SpecularLight = LightColour * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );

	
	// Pass texture coordinates (UVs) on to the pixel shader
	vOut.UV = vIn.UV;

	return VLOut;
}





//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


float4 DiffuseTex( VS_BASIC_OUTPUT vOut ) : SV_Target 
{
	// Extract a colour for this pixel from the diffuse map (texture). Although there is no lighting in this lab, we will stick
	// to the conventional names for textures, with the diffuse map being the main texture representing the look of a surface
	float3 diffuseMapColour = DiffuseMap.Sample(Tri, vOut.UV).rgb;
	
	// Combine mesh colour and texture colour for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = diffuseMapColour;
	combinedColour.a = 1.0f;

	return combinedColour;
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

technique10 LitTextured
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, LitTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, DiffuseTex() ) );
	}
}
