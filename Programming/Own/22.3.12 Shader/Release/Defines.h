//--------------------------------------------------------------------------------------
//	Defines.h
//
//	General definitions shared across entire project
//--------------------------------------------------------------------------------------

#ifndef DEFINES_H_INCLUDED // Header guard - prevents file being included more than once (would cause errors)
#define DEFINES_H_INCLUDED

#include <Windows.h>
#include <d3d10.h>
#include <d3dx10.h>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Move speed constants (shared between camera and model class)
const float MoveSpeed = 50.0f;
const float RotSpeed = 2.0f;


//-----------------------------------------------------------------------------
// Helper functions and macros
//-----------------------------------------------------------------------------

// Helper macro to release DirectX pointers only if they are not NULL
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } }

// Angular helper functions to convert from degrees to radians and back (D3DX_PI is a double)
inline float ToRadians( float deg ) { return deg * (float)D3DX_PI / 180.0f; }
inline float ToDegrees( float rad ) { return rad * 180.0f / (float)D3DX_PI; }


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Make DirectX render device available to other source files. We declare this
// variable global in one file, then make it "extern" to all others. In general
// this is not good practice - this is a kind of super-global variable. Would
// be better to have a Device class responsible for this data. However, this
// example aims for a minimum of code to help demonstrate the focus topic
extern ID3D10Device* g_pd3dDevice;

// Dimensions of viewport - shared between setup code and camera class (which needs this to create the projection matrix - see code there)
extern int g_ViewportWidth, g_ViewportHeight;


#endif // End of header guard - see top of file
