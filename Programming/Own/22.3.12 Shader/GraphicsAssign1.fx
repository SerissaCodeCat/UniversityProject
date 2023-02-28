//--------------------------------------------------------------------------------------
// File: GraphicsAssign1.fx
//
//	Shaders Graphics Assignment
//	Add further models using different shader techniques
//	See assignment specification for details
//--------------------------------------------------------------------------------------


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

struct VS_LIGHTING_OUTPUT
{
    float4 ProjPos			 : SV_POSITION;  
	float4 WorldPos			 :POSITION;
	float4 WorldNormal       :NORMAL;
    float2 UV				 : TEXCOORD0;
};

struct VS_BASIC_OUTPUT
{
    float4 ProjPos       : SV_POSITION;  
    float3 DiffuseLight  : COLOR0;
    float3 SpecularLight : COLOR1;
    float2 UV            : TEXCOORD0;
};






//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// All these variables are created & manipulated in the C++ code and passed into the shader here

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;
float4x4 ViewProjMatrix;
//////////////////////////////////////////////////////////////////////////////////////////
///////////////////TEXTURE VARIABLES//////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
float3		LightPos;
float3		LightFacing;
float4x4	LightViewMatrix;
float4x4	LightProjMatrix;
float		LightCosHalfAngle;
float3		LightColour;

float3		Light1Pos;
float3		Light1Facing;
float4x4	Light1ViewMatrix;
float4x4	Light1ProjMatrix;
float		Light1CosHalfAngle;
float3		Light1Colour;

float3 AmbientColour;
float  SpecularPower;
float  Tint;
float3 CameraPos;
float3 ModelColour;

Texture2D DiffuseMap;
Texture2D ShadowMap;
Texture2D ShadowMap1;

SamplerState Point
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
    MaxLOD = 0;
};

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

// Basic vertex shader to transform 3D model vertices to 2D and pass UVs to the pixel shader
//
VS_BASIC_OUTPUT BasicTransform( VS_BASIC_INPUT vIn )
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
};

VS_LIGHTING_OUTPUT VertexLightingTex( VS_BASIC_INPUT vIn )
{
	VS_LIGHTING_OUTPUT vOut;

	float4 modelPos = float4(vIn.Pos, 1.0f); 
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	float4 modelNormal = float4(vIn.Normal, 0.0f); 
	float4 worldNormal = mul( modelNormal, WorldMatrix );

	// Calculate direction of light and camera
	float3 CameraDir = normalize(CameraPos - worldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)
	float3 LightDir = normalize(LightPos - worldPos.xyz);   // Same for light
	float3 Light1Dir = normalize(Light1Pos - worldPos.xyz); // Same for 2ndlight
	
	// Calculate lighting on this vertex or pixel
	
	float3 halfway = normalize(LightDir + CameraDir);
	
	vOut.WorldPos = worldPos;
	vOut.WorldNormal = worldNormal;
	vOut.UV = vIn.UV;


	return vOut;
}






//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


// A pixel shader that just outputs a single fixed colour
//
float4 OneColour( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	return float4( ModelColour, 1.0 ); // Set alpha channel to 1.0 (opaque)
}
float4 DiffuseTex( VS_BASIC_OUTPUT vOut ) : SV_Target 
{
	// Extract a colour for this pixel from the diffuse map (texture). Although there is no lighting in this lab, we will stick
	// to the conventional names for textures, with the diffuse map being the main texture representing the look of a surface
	float3 diffuseMapColour = DiffuseMap.Sample(Tri, vOut.UV).rgb;
	
	// Combine mesh colour and texture colour for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = diffuseMapColour;
	combinedColour.a = 1.0f; // No alpha processing in this shader

	return combinedColour;
}
float4 DiffuseTexTint( VS_BASIC_OUTPUT vOut ) : SV_Target 
{
	// Extract a colour for this pixel from the diffuse map (texture). Although there is no lighting in this lab, we will stick
	// to the conventional names for textures, with the diffuse map being the main texture representing the look of a surface
	
	float3 diffuseMapColour = DiffuseMap.Sample(Tri, vOut.UV).rgb;
	diffuseMapColour.r += Tint;
	diffuseMapColour.g -= Tint;
	diffuseMapColour.b = 0;

	
	// Combine mesh colour and texture colour for final pixel colour
	float4 combinedColour;	
	combinedColour.rgb = diffuseMapColour;
	combinedColour.a = 1.0f; // No alpha processing in this shader

	return combinedColour;
}

float4 PixelLitDiffuseMap( VS_LIGHTING_OUTPUT vOut ) : SV_Target
{  

	// Extract diffuse material colour for this pixel from a texture )
	float4 DiffuseMaterial = DiffuseMap.Sample( Tri, vOut.UV ).rgba;
	
	float3 SpecularMaterial = DiffuseMaterial[3];
	//float4 WorldNormal = normalize(WorldNormal); 

	// Calculate direction of light and camera
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera
	float3 LightDir = normalize(LightPos - vOut.WorldPos.xyz);   // light
	//float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);   // 2ndlight
	
	// Calculate lighting on this vertex (or pixel)
	
	float3 halfway = normalize((LightDir + CameraDir)/2);
	//float3 halfway1 = normalize((Light1Dir + CameraDir)/2);

	// Combine colours (lighting, textures) for final pixel colour
	float3 DiffuseLight = AmbientColour + LightColour * max( dot(vOut.WorldNormal.xyz, LightDir), 0 );
	//float3 DiffuseLight1 = AmbientColour + Light1Colour * max( dot(vOut.WorldNormal.xyz, Light1Dir), 0 );
	float3 SpecularLight = LightColour * pow( max( dot(vOut.WorldNormal.xyz, halfway), 0 ), SpecularPower );
	//float3 SpecularLight1 = Light1Colour * pow( max( dot(vOut.WorldNormal.xyz, halfway1), 0 ), SpecularPower );

	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * (DiffuseLight/*+DiffuseLight1*/) + SpecularMaterial * (SpecularLight/*+SpecularLight1*/);
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

float4 AlphaLitDiffuseMap( VS_LIGHTING_OUTPUT vOut ) : SV_Target
{  

	// Extract diffuse material colour for this pixel from a texture )
	float4 DiffuseMaterial = DiffuseMap.Sample( Tri, vOut.UV ).rgba;
	
	float3 SpecularMaterial = DiffuseMaterial[3];
	//float4 WorldNormal = normalize(WorldNormal); 

	// Calculate direction of light and camera
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera
	float3 LightDir = normalize(LightPos - vOut.WorldPos.xyz);   // light
	//float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);   // 2ndlight
	
	// Calculate lighting on this vertex (or pixel)
	
	float3 halfway = normalize(LightDir + CameraDir);
	//float3 halfway1 = normalize(Light1Dir + CameraDir);

	// Combine colours (lighting, textures) for final pixel colour
	float3 DiffuseLight = AmbientColour + LightColour * max( dot(vOut.WorldNormal.xyz, LightDir), 0 );
	//float3 DiffuseLight1 = AmbientColour + Light1Colour * max( dot(vOut.WorldNormal.xyz, Light1Dir), 0 );
	float3 SpecularLight = LightColour * pow( max( dot(vOut.WorldNormal.xyz, halfway), 0 ), SpecularPower );
	//float3 SpecularLight1 = Light1Colour * pow( max( dot(vOut.WorldNormal.xyz, halfway1), 0 ), SpecularPower );

	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * (DiffuseLight/*+DiffuseLight1*/) + SpecularMaterial * (SpecularLight/*+SpecularLight1*/);
	combinedColour.a = DiffuseMaterial[3]; 

	return combinedColour;
}



///////////////////////////////////////////////////////////////////////////////////////
//////////////////////STATES///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


// Culling States
RasterizerState CullNone  // Cull none of the polygons, i.e. show both sides
{
	CullMode = None;
};

RasterizerState CullBack  // Cull back side of polygon - normal behaviour, only show front of polygons
{
	CullMode = Back;
};

RasterizerState CullFront  // Cull front side of polygon - normal behaviour, only show back of polygons
{
	CullMode = Front;
};



// Depth States
// Manipulate how the depth buffer is used to correct sorting errors with transparent polygons.

DepthStencilState DepthWritesOff // Don't write to the depth buffer - polygons rendered will not obscure other polygons
{
	DepthWriteMask = ZERO;
};
DepthStencilState DepthWritesOn  // Write to the depth buffer - normal behaviour 
{
	DepthWriteMask = ALL;
};


/////////////////////
// Blending States

// Blending states determine how the source pixels (those from the polygon we are rendering) blend with the destination
// pixels (what has already been rendered to the back buffer). This is where we can enable additive, multiplicative or 
// other blending modes

BlendState NoBlending
{
    BlendEnable[0] = FALSE;
};

BlendState AdditiveBlending
{
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
};
BlendState MultiplicativeBlending
{
    BlendEnable[0] = TRUE;
    SrcBlend = DEST_COLOR;
    DestBlend = ZERO;
    BlendOp = ADD;
};
BlendState AlphaBlending
{
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
};




//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

// Render models unlit in a single colour
technique10 PlainColour
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, OneColour() ) );

		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState( CullBack ); 
        SetDepthStencilState( DepthWritesOn, 0 );
	}
}
technique10 Textured
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, DiffuseTex() ) );

		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState( CullBack ); 
        SetDepthStencilState( DepthWritesOn, 0 );
	}
}
technique10 AddTextured
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, DiffuseTexTint() ) );

		//Blending and culling
		SetRasterizerState( CullBack );
		SetBlendState( AdditiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthWritesOn, 0 );

	}
}

technique10 VertexLit
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VertexLightingTex() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, PixelLitDiffuseMap() ) );

		//Blending and culling
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullNone ); 
		SetDepthStencilState( DepthWritesOn, 0 );
	}
}

technique10 SpecularAlphaLit
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VertexLightingTex() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, AlphaLitDiffuseMap() ) );

		//Blending and culling
		SetBlendState( AlphaBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullNone ); 
		SetDepthStencilState( DepthWritesOff, 0 );
	}
}
technique10 ShadowMappingTechnique
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VertexLightingTex() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, AlphaLitDiffuseMap() ) );

		//Blending and culling
		SetBlendState( AlphaBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullNone ); 
		SetDepthStencilState( DepthWritesOff, 0 );
	}
}
technique10 DepthOnlyTechnique
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( NULL );

		//Blending and culling
		SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetRasterizerState( CullBack ); 
		SetDepthStencilState( DepthWritesOn, 0 );
	}
}

