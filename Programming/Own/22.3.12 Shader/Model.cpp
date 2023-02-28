//--------------------------------------------------------------------------------------
//	Model.cpp
//
//	The model class collects together a model's geometry (vertex and index data) and
//	also manages it's positioning with a world matrix
//--------------------------------------------------------------------------------------

#include "Defines.h" // General definitions shared by all source files
#include "Model.h"   // Declaration of this class

#include "CImportXFile.h"    // Class to load meshes (taken from a full graphics engine)

///////////////////////////////
// Constructors / Destructors

// Constructor - initialise all camera settings - look at the constructor declaration in the header file to see that there are defaults provided for everything
CModel::CModel( D3DXVECTOR3 position, D3DXVECTOR3 rotation, float scale )
{
	m_Position = position;
	m_Rotation = rotation;
	SetScale( scale );
	UpdateMatrix();

	// Good practice to ensure all private data is sensibly initialised
	m_VertexBuffer = NULL;
	m_NumVertices = 0;
	m_VertexSize = 0;
	m_VertexLayout = NULL;

	m_IndexBuffer = NULL;
	m_NumIndices = 0;

	m_HasGeometry = false;
}

// Model destructor
CModel::~CModel()
{
	ReleaseResources();
}

// Release resources used by model
void CModel::ReleaseResources()
{
	// Release resources
	SAFE_RELEASE( m_IndexBuffer );  // Using a DirectX helper macro to simplify code here - look it up in Defines.h
	SAFE_RELEASE( m_VertexBuffer );
	SAFE_RELEASE( m_VertexLayout );
	m_HasGeometry = false;
}


/////////////////////////////
// Model Loading

// The loading and parsing of ".X" files is supported using a class taken from another application. We will not look at the process (more to do with parsing than graphics). Ultimately
// we end up with arrays of data exactly as we have previously manually typed in

// Load the model geometry from a file. This function only reads the geometry using the first material in the file, so multi-material
// models will load but will have parts missing. May optionally request for tangents to be created for the model (for normal or parallax mapping)
// We need to pass an example technique that the model will use to help DirectX understand how to connect this data with the vertex shaders
// Returns true if the load was successful
bool CModel::Load( const string& fileName, ID3D10EffectTechnique* exampleTechnique, bool tangents /*= false*/ ) // The commented out bit is the default parameter (can't write it here, only in the declaration)
{
	// Release any existing geometry in this object
	ReleaseResources();

	// Use CImportXFile class (from another application) to load the given file. The import code is wrapped in the namespace 'gen'
	gen::CImportXFile mesh;
	if (mesh.ImportFile( fileName.c_str() ) != gen::kSuccess)
	{
		return false;
	}

	// Get first sub-mesh from loaded file
	gen::SSubMesh subMesh;
	if (mesh.GetSubMesh( 0, &subMesh, tangents ) != gen::kSuccess)
	{
		return false;
	}


	// Create vertex element list & layout. We need a vertex layout to say what data we have per vertex in this model (e.g. position, normal, uv, etc.)
	// In previous projects the element list was a manually typed in array as we knew what data we would provide. However, as we can load models with
	// different vertex data this time we need flexible code. The array is built up one element at a time: ask the import class if it loaded normals, 
	// if so then add a normal line to the array, then ask if it loaded UVS...etc
	unsigned int numElts = 0;
	unsigned int offset = 0;
	// Position is always required
	m_VertexElts[numElts].SemanticName = "POSITION";   // Semantic in HLSL (what is this data for)
	m_VertexElts[numElts].SemanticIndex = 0;           // Index to add to semantic (a count for this kind of data, when using multiple of the same type, e.g. TEXCOORD0, TEXCOORD1)
	m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT; // Type of data - this one will be a float3 in the shader. Most data communicated as though it were colours
	m_VertexElts[numElts].AlignedByteOffset = offset;  // Offset of element from start of vertex data (e.g. if we have position (float3), uv (float2) then normal, the normal's offset is 5 floats = 5*4 = 20)
	m_VertexElts[numElts].InputSlot = 0;               // For when using multiple vertex buffers (e.g. instancing - an advanced topic)
	m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA; // Use this value for most cases (only changed for instancing)
	m_VertexElts[numElts].InstanceDataStepRate = 0;                     // --"--
	offset += 12;
	++numElts;
	// Repeat for each kind of vertex data
	if (subMesh.hasNormals)
	{
		m_VertexElts[numElts].SemanticName = "NORMAL";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh.hasTangents)
	{
		m_VertexElts[numElts].SemanticName = "TANGENT";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh.hasTextureCoords)
	{
		m_VertexElts[numElts].SemanticName = "TEXCOORD";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 8;
		++numElts;
	}
	if (subMesh.hasVertexColours)
	{
		m_VertexElts[numElts].SemanticName = "COLOR";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R8G8B8A8_UNORM; // A RGBA colour with 1 byte (0-255) per component
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 4;
		++numElts;
	}
	m_VertexSize = offset;

	// Given the vertex element list, pass it to DirectX to create a vertex layout. We also need to pass an example of a technique that will
	// render this model. We will only be able to render this model with techniques that have the same vertex input as the example we use here
	D3D10_PASS_DESC PassDesc;
	exampleTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	g_pd3dDevice->CreateInputLayout( m_VertexElts, numElts, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_VertexLayout );


	// Create the vertex buffer and fill it with the loaded vertex data
	m_NumVertices = subMesh.numVertices;
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT; // Not a dynamic buffer
	bufferDesc.ByteWidth = m_NumVertices * m_VertexSize; // Buffer size
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData; // Initial data
	initData.pSysMem = subMesh.vertices;   
	if (FAILED( g_pd3dDevice->CreateBuffer( &bufferDesc, &initData, &m_VertexBuffer )))
	{
		return false;
	}


	// Create the index buffer - assuming 2-byte (WORD) index data
	m_NumIndices = static_cast<unsigned int>(subMesh.numFaces) * 3;
	bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_NumIndices * sizeof(WORD);
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	initData.pSysMem = subMesh.faces;   
	if (FAILED( g_pd3dDevice->CreateBuffer( &bufferDesc, &initData, &m_IndexBuffer )))
	{
		return false;
	}

	m_HasGeometry = true;
	return true;
}


/////////////////////////////
// Model Usage

// Update the world matrix of the model from its position, rotation and scaling
void CModel::UpdateMatrix()
{
	// Make a matrix for position and scaling, and one each for X, Y & Z rotations
	D3DXMATRIX matrixXRot, matrixYRot, matrixZRot, matrixTranslation, matrixScaling;
	D3DXMatrixRotationX( &matrixXRot, m_Rotation.x );
	D3DXMatrixRotationY( &matrixYRot, m_Rotation.y );
	D3DXMatrixRotationZ( &matrixZRot, m_Rotation.z );
	D3DXMatrixTranslation( &matrixTranslation, m_Position.x, m_Position.y, m_Position.z );
	D3DXMatrixScaling( &matrixScaling, m_Scale.x, m_Scale.y, m_Scale.z );

	// Multiply above matrices together to get the effect of them all combined - this makes the world matrix for the rendering pipeline
	// Order of multiplication is important, get slightly different control mechanism depending on order
	m_WorldMatrix = matrixScaling * matrixZRot * matrixXRot * matrixYRot * matrixTranslation;
}


// Control the model's position and rotation using keys provided. Amount of motion performed depends on frame time
void CModel::Control( float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
                      EKeyCode turnCW, EKeyCode turnCCW, EKeyCode moveForward, EKeyCode moveBackward )
{
	if (KeyHeld( turnDown ))
	{
		m_Rotation.x += RotSpeed * frameTime;
	}
	if (KeyHeld( turnUp ))
	{
		m_Rotation.x -= RotSpeed * frameTime;
	}
	if (KeyHeld( turnRight ))
	{
		m_Rotation.y += RotSpeed * frameTime;
	}
	if (KeyHeld( turnLeft ))
	{
		m_Rotation.y -= RotSpeed * frameTime;
	}
	if (KeyHeld( turnCW ))
	{
		m_Rotation.z += RotSpeed * frameTime;
	}
	if (KeyHeld( turnCCW ))
	{
		m_Rotation.z -= RotSpeed * frameTime;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from world matrix
	if (KeyHeld( moveForward ))
	{
		m_Position.x += m_WorldMatrix._31 * MoveSpeed * frameTime;
		m_Position.y += m_WorldMatrix._32 * MoveSpeed * frameTime;
		m_Position.z += m_WorldMatrix._33 * MoveSpeed * frameTime;
	}
	if (KeyHeld( moveBackward ))
	{
		m_Position.x -= m_WorldMatrix._31 * MoveSpeed * frameTime;
		m_Position.y -= m_WorldMatrix._32 * MoveSpeed * frameTime;
		m_Position.z -= m_WorldMatrix._33 * MoveSpeed * frameTime;
	}
}


// Render the model with the given technique. Assumes any shader variables for the technique have already been set up (e.g. matrices and textures)
void CModel::Render( ID3D10EffectTechnique* technique )
{
	// Don't render if no geometry
	if (!m_HasGeometry)
	{
		return;
	}

	// Select vertex and index buffer - assuming all data will be as triangle lists
	UINT offset = 0;
	g_pd3dDevice->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &m_VertexSize, &offset );
	g_pd3dDevice->IASetInputLayout( m_VertexLayout );
	g_pd3dDevice->IASetIndexBuffer( m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	g_pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Render the model. All the data and shader variables are prepared, now select the technique to use and draw.
	// The loop is for advanced techniques that need multiple passes - we will only use techniques with one pass
	D3D10_TECHNIQUE_DESC techDesc;
	technique->GetDesc( &techDesc );
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		technique->GetPassByIndex( p )->Apply( 0 );
		g_pd3dDevice->DrawIndexed( m_NumIndices, 0, 0 );
	}
	g_pd3dDevice->DrawIndexed( m_NumIndices, 0, 0 );
}
