//--------------------------------------------------------------------------------------
//	Camera.cpp
//
//	The camera class encapsulates the camera's view and projection matrix
//--------------------------------------------------------------------------------------

#include "Defines.h" // General definitions shared by all source files
#include "Camera.h"  // Declaration of this class

///////////////////////////////
// Constructors / Destructors

// Constructor - initialise all camera settings - look at the constructor declaration in the header file to see that there are defaults provided for everything
CCamera::CCamera( D3DXVECTOR3 position, D3DXVECTOR3 rotation, float fov, float nearClip, float farClip )
{
	m_Position = position;
	m_Rotation = rotation;
	UpdateMatrices();

	SetFOV( fov );
	SetNearClip( nearClip );
	SetFarClip( farClip );
}


///////////////////////////////
// Camera Usage

// Update the matrices used for the camera in the rendering pipeline. Treat the camera like a model and create a world matrix for it. Then convert that into
// the view matrix that the rendering pipeline actually uses. Also create the projection matrix, a second matrix that only cameras have
void CCamera::UpdateMatrices()
{
	// Make matrices for position and rotations, then multiply together to get a "camera world matrix"
	D3DXMATRIXA16 matrixXRot, matrixYRot, matrixZRot, matrixTranslation;
	D3DXMatrixRotationX( &matrixXRot, m_Rotation.x );
	D3DXMatrixRotationY( &matrixYRot, m_Rotation.y );
	D3DXMatrixRotationZ( &matrixZRot, m_Rotation.z );
	D3DXMatrixTranslation( &matrixTranslation, m_Position.x, m_Position.y, m_Position.z);
	m_WorldMatrix = matrixZRot * matrixXRot * matrixYRot * matrixTranslation;

	// The rendering pipeline actually needs the inverse of the camera world matrix - called the view matrix. Creating an inverse is easy with DirectX:
	D3DXMatrixInverse( &m_ViewMatrix, NULL, &m_WorldMatrix );

	// Initialize the projection matrix. This determines viewing properties of the camera such as field of view (FOV) and near clip distance
	// One other factor in the projection matrix is the aspect ratio of screen (width/height) - used to adjust FOV between horizontal and vertical
	float aspect = (float)g_ViewportWidth / g_ViewportHeight; 
	D3DXMatrixPerspectiveFovLH( &m_ProjMatrix, m_FOV, 1.33f, m_NearClip, m_FarClip );

	// Combine the view and projection matrix into a single matrix - which can (optionally) be used in the vertex shaders to save one matrix multiply per vertex
	m_ViewProjMatrix = m_ViewMatrix * m_ProjMatrix;
}


// Control the camera's position and rotation using keys provided. Amount of motion performed depends on frame time
void CCamera::Control( float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
                       EKeyCode moveForward, EKeyCode moveBackward, EKeyCode moveLeft, EKeyCode moveRight)
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

	// Local X movement - move in the direction of the X axis, get axis from camera's "world" matrix
	if (KeyHeld( moveRight ))
	{
		m_Position.x += m_WorldMatrix._11 * MoveSpeed * frameTime;
		m_Position.y += m_WorldMatrix._12 * MoveSpeed * frameTime;
		m_Position.z += m_WorldMatrix._13 * MoveSpeed * frameTime;
	}
	if (KeyHeld( moveLeft ))
	{
		m_Position.x -= m_WorldMatrix._11 * MoveSpeed * frameTime;
		m_Position.y -= m_WorldMatrix._12 * MoveSpeed * frameTime;
		m_Position.z -= m_WorldMatrix._13 * MoveSpeed * frameTime;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from view matrix
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
