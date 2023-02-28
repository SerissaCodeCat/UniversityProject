//--------------------------------------------------------------------------------------
//	Camera.h
//
//	The camera class encapsulates the camera's view and projection matrix
//--------------------------------------------------------------------------------------

#ifndef CAMERA_H_INCLUDED // Header guard - prevents file being included more than once (would cause errors)
#define CAMERA_H_INCLUDED

#include "Input.h"

//-----------------------------------------------------------------------------
// DirectX Camera Class Defintition
//-----------------------------------------------------------------------------

class CCamera
{
/////////////////////////////
// Private member variables
private:

	// Postition and rotations for the camera (rarely scale cameras)
	D3DXVECTOR3 m_Position;
	D3DXVECTOR3 m_Rotation;

	// Camera settings: field of view, near and far clip plane distances. Note that the FOV angle is measured in radians (radians = degrees * PI/180)
	float m_FOV;
	float m_NearClip;
	float m_FarClip;

	// Current view, projection and combined view-projection matrices (DirectX matrix type)
	D3DXMATRIXA16 m_WorldMatrix;    // Easiest to treat the camera like a model and give it a "world" matrix...
	D3DXMATRIXA16 m_ViewMatrix;     // ... the view matrix used in the pipeline is the inverse of its world matrix
	D3DXMATRIXA16 m_ProjMatrix;     // Projection matrix to set field of view and near/far clip distances
	D3DXMATRIXA16 m_ViewProjMatrix; // Combine (multiply) the view and projection matrices together - saves a matrix multiply in the shader (optional optimisation)


/////////////////////////////
// Public member functions
public:

	///////////////////////////////
	// Constructors / Destructors

	// Constructor - initialise all settings, sensible defaults provided for everything.
	CCamera( D3DXVECTOR3 position = D3DXVECTOR3(0,0,0), D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0), float fov = D3DX_PI/4, float nearClip = 0.1f, float farClip = 10000.0f );


	/////////////////////////////
	// Data access

	// Getters
	D3DXVECTOR3 GetPosition()
	{
		return m_Position;
	}
	D3DXVECTOR3 GetRotation()
	{
		return m_Rotation;
	}

	D3DXMATRIXA16 GetViewMatrix()
	{
		return m_ViewMatrix;
	}
	D3DXMATRIXA16 GetProjectionMatrix()
	{
		return m_ProjMatrix;
	}
	D3DXMATRIXA16 GetViewProjectionMatrix()
	{
		return m_ViewProjMatrix;
	}

	float GetFOV()
	{
		return m_FOV;
	}
	float GetNearClip()
	{
		return m_NearClip;
	}
	float GetFarClip()
	{
		return m_FarClip;
	}


	// Setters
	void SetPosition( D3DXVECTOR3 position )
	{
		m_Position = position;
	}
	void SetRotation( D3DXVECTOR3 rotation )
	{
		m_Rotation = rotation;
	}
	void SetFOV( float fov )
	{
		m_FOV = fov;
	}
	void SetNearClip( float nearClip )
	{
		m_NearClip = nearClip;
	}
	void SetFarClip( float farClip )
	{
		m_FarClip = farClip;
	}


	/////////////////////////////
	// Camera Usage

	// Update the matrices used for the camera in the rendering pipeline
	void UpdateMatrices();

	// Control the camera's position and rotation using keys provided
	void Control( float frameTime, EKeyCode turnUp, EKeyCode turnDown, EKeyCode turnLeft, EKeyCode turnRight,  
	              EKeyCode moveForward, EKeyCode moveBackward, EKeyCode moveLeft, EKeyCode moveRight);
};


#endif // End of header guard - see top of file
