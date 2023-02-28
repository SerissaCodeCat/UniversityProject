/****************************************************************************
 *
 * 			vector3d.h: Vector 3d and point representation and manipulation api 
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *      
 */
#ifndef __VECTOR3D_H
#define __VECTOR3D_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<cmath>
#include<iostream>
#include<stdlib.h>
#include<stdio.h>

__BEGIN_YAFRAY

// useful trick found in trimesh2 lib by Szymon Rusinkiewicz
// makes code for vector dot- cross-products much more readable
#define VDOT *
#define VCROSS ^

class YAFRAYCORE_EXPORT vector3d_t
{
	public:
		vector3d_t() { x = y = z = 0; }
		vector3d_t(PFLOAT ix, PFLOAT iy, PFLOAT iz=0) { x=ix;  y=iy;  z=iz; }
		vector3d_t(const vector3d_t &s) { x=s.x;  y=s.y;  z=s.z; }
		void set(PFLOAT ix, PFLOAT iy, PFLOAT iz=0) { x=ix;  y=iy;  z=iz; }
		void normalize();
		// normalizes and returns length
		PFLOAT normLen()
		{
			PFLOAT vl = x*x + y*y + z*z;
			if (vl!=0.0) {
				vl = sqrt(vl);
				const PFLOAT d = 1.0/vl;
				x*=d; y*=d; z*=d;
			}
			return vl;
		}
		// normalizes and returns length squared
		PFLOAT normLenSqr()
		{
			PFLOAT vl = x*x + y*y + z*z;
			if (vl!=0.0) {
				const PFLOAT d = 1.0/sqrt(vl);
				x*=d; y*=d; z*=d;
			}
			return vl;
		}
		PFLOAT length() const;
		bool null()const { return ((x==0) && (y==0) && (z==0)); }
		vector3d_t& operator = (const vector3d_t &s) { x=s.x;  y=s.y;  z=s.z;  return *this;}
		vector3d_t& operator +=(const vector3d_t &s) { x+=s.x;  y+=s.y;  z+=s.z;  return *this;}
		vector3d_t& operator -=(const vector3d_t &s) { x-=s.x;  y-=s.y;  z-=s.z;  return *this;}
		vector3d_t& operator /=(PFLOAT s) { x/=s;  y/=s;  z/=s;  return *this;}
		vector3d_t& operator *=(PFLOAT s) { x*=s;  y*=s;  z*=s;  return *this;}
		PFLOAT operator[] (int i) const{ return (&x)[i]; } //Lynx
		void abs() { x=fabs(x);  y=fabs(y);  z=fabs(z); }
		~vector3d_t() {};
		PFLOAT x,y,z;
};

class YAFRAYCORE_EXPORT point3d_t
{
	public:
		point3d_t() { x = y = z = 0; }
		point3d_t(PFLOAT ix, PFLOAT iy, PFLOAT iz=0) { x=ix;  y=iy;  z=iz; }
		point3d_t(const point3d_t &s) { x=s.x;  y=s.y;  z=s.z; }
		point3d_t(const vector3d_t &v) { x=v.x;  y=v.y;  z=v.z; }
		void set(PFLOAT ix, PFLOAT iy, PFLOAT iz=0) { x=ix;  y=iy;  z=iz; }
		PFLOAT length() const;
		point3d_t& operator= (const point3d_t &s) { x=s.x;  y=s.y;  z=s.z;  return *this; }
		point3d_t& operator *=(PFLOAT s) { x*=s;  y*=s;  z*=s;  return *this;}
		point3d_t& operator +=(PFLOAT s) { x+=s;  y+=s;  z+=s;  return *this;}
		point3d_t& operator +=(const point3d_t &s) { x+=s.x;  y+=s.y;  z+=s.z;  return *this;}
		point3d_t& operator -=(const point3d_t &s) { x-=s.x;  y-=s.y;  z-=s.z;  return *this;}
		PFLOAT operator[] (int i) const{ return (&x)[i]; } //Lynx
		PFLOAT &operator[](int i) { return (&x)[i]; } //Lynx
		~point3d_t() {};
		PFLOAT x,y,z;
};


#define FAST_ANGLE(a,b)  ( (a).x*(b).y - (a).y*(b).x )
#define FAST_SANGLE(a,b) ( (((a).x*(b).y - (a).y*(b).x) >= 0) ? 0 : 1 )
#define SIN(a,b) sqrt(1-((a)*(b))*((a)*(b)))

YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream &out,const vector3d_t &v);
YAFRAYCORE_EXPORT std::ostream & operator << (std::ostream &out,const point3d_t &p);

inline PFLOAT operator * ( const vector3d_t &a,const vector3d_t &b)
{
	return (a.x*b.x+a.y*b.y+a.z*b.z);
}

inline vector3d_t operator * ( PFLOAT f,const vector3d_t &b)
{
	return vector3d_t(f*b.x,f*b.y,f*b.z);
}

inline vector3d_t operator * (const vector3d_t &b,PFLOAT f)
{
	return vector3d_t(f*b.x,f*b.y,f*b.z);
}

inline point3d_t operator * (PFLOAT f,const point3d_t &b)
{
	return point3d_t(f*b.x,f*b.y,f*b.z);
}

inline vector3d_t operator / (const vector3d_t &b,PFLOAT f)
{
	return vector3d_t(b.x/f,b.y/f,b.z/f);
}

inline point3d_t operator / (const point3d_t &b,PFLOAT f)
{
	return point3d_t(b.x/f,b.y/f,b.z/f);
}

inline point3d_t operator * (const point3d_t &b,PFLOAT f)
{
	return point3d_t(b.x*f,b.y*f,b.z*f);
}

inline vector3d_t operator / (PFLOAT f,const vector3d_t &b)
{
	return vector3d_t(b.x/f,b.y/f,b.z/f);
}

inline vector3d_t operator ^ ( const vector3d_t &a,const vector3d_t &b)
{
	return vector3d_t(a.y*b.z-a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

inline vector3d_t  operator - ( const vector3d_t &a,const vector3d_t &b)
{
	return vector3d_t(a.x-b.x,a.y-b.y,a.z-b.z);
}

inline vector3d_t  operator - ( const point3d_t &a,const point3d_t &b)
{
	return vector3d_t(a.x-b.x,a.y-b.y,a.z-b.z);
}

inline point3d_t  operator - ( const point3d_t &a,const vector3d_t &b)
{
	return point3d_t(a.x-b.x,a.y-b.y,a.z-b.z);
}

inline vector3d_t  operator - ( const vector3d_t &b)
{
	return vector3d_t(-b.x,-b.y,-b.z);
}

inline vector3d_t  operator + ( const vector3d_t &a,const vector3d_t &b)
{
	return vector3d_t(a.x+b.x,a.y+b.y,a.z+b.z);
}

inline point3d_t  operator + ( const point3d_t &a,const point3d_t &b)
{
	return point3d_t(a.x+b.x,a.y+b.y,a.z+b.z);
}

inline point3d_t  operator + ( const point3d_t &a,const vector3d_t &b)
{
	return point3d_t(a.x+b.x,a.y+b.y,a.z+b.z);
}

inline bool  operator == ( const point3d_t &a,const point3d_t &b)
{
	return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z));
}

YAFRAYCORE_EXPORT bool  operator == ( const vector3d_t &a,const vector3d_t &b);
YAFRAYCORE_EXPORT bool  operator != ( const vector3d_t &a,const vector3d_t &b);

inline PFLOAT vector3d_t::length()const
{
	return sqrt(x*x+y*y+z*z);
}

inline PFLOAT point3d_t::length()const
{
	return sqrt(x*x+y*y+z*z);
}

inline void vector3d_t::normalize()
{
	PFLOAT len = x*x + y*y + z*z;
	if (len!=0)
	{
		len = 1.0/sqrt(len);
		x *= len;
		y *= len;
		z *= len;
	}
}

inline vector3d_t reflect(const vector3d_t &n,const vector3d_t &v)
{
	const PFLOAT vn = v*n;
	if (vn<0) return -v;
	return 2*vn*n - v;
}


inline vector3d_t toVector(const point3d_t &p)
{
	return vector3d_t(p.x,p.y,p.z);
}

YAFRAYCORE_EXPORT vector3d_t refract(const vector3d_t &n,const vector3d_t &v,PFLOAT IOR);
YAFRAYCORE_EXPORT void fresnel(const vector3d_t & I, const vector3d_t & n, PFLOAT IOR,
						CFLOAT &Kr,CFLOAT &Kt);
YAFRAYCORE_EXPORT void fast_fresnel(const vector3d_t & I, const vector3d_t & n, PFLOAT IORF,
						CFLOAT &Kr,CFLOAT &Kt);

inline void createCS(const vector3d_t &N, vector3d_t &u, vector3d_t &v)
{
	if ((N.x==0) && (N.y==0))
	{
		if (N.z<0)
			u.set(-1, 0, 0);
		else
			u.set(1, 0, 0);
		v.set(0, 1, 0);
	}
	else
	{
		// Note: The root cannot become zero if
		// N.x==0 && N.y==0.
		const PFLOAT d = 1.0/sqrt(N.y*N.y + N.x*N.x);
		u.set(N.y*d, -N.x*d, 0);
		v = N^u;
	}
}

YAFRAYCORE_EXPORT void ShirleyDisk(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v);

extern YAFRAYCORE_EXPORT int myseed;

inline int ourRandomI()
{
	const int a = 7*7*7*7*7;
	const int m = (1<<31)-1;
	const int q = m/a;          // m = aq+r
	const int r = m % a;
	myseed = a * (myseed % q) - r * (myseed/q);
	if (myseed < 0)
		myseed += m;
	return myseed;
}

inline PFLOAT ourRandom()
{
	const int a = 7*7*7*7*7;
	const int m = (1<<31)-1;
	const int q = m/a;          // m = aq+r
	const int r = m % a;
	myseed = a * (myseed % q) - r * (myseed/q);
	if (myseed < 0)
		myseed += m;
	return (PFLOAT)myseed/(PFLOAT)m;
}

inline PFLOAT ourRandom(int &seed)
{
	const int a = 7*7*7*7*7;
	const int m = (1<<31)-1;
	const int q = m/a;          // m = aq+r
	const int r = m % a;
	seed = a * (seed % q) - r * (seed/q);
	if (myseed < 0)
		myseed += m;
	return (PFLOAT)seed/(PFLOAT)m;
}

inline vector3d_t RandomSpherical()
{
	PFLOAT r;
	vector3d_t v(0.0, 0.0, ourRandom());
	if ((r = 1.0 - v.z*v.z)>0.0) {
		PFLOAT a = (2.0*M_PI) * ourRandom();
		r = sqrt(r);
		v.x = r * cos(a);  v.y = r * sin(a);
	}
	else v.z = 1.0;
	return v;
}

YAFRAYCORE_EXPORT vector3d_t randomVectorCone(const vector3d_t &D, const vector3d_t &U, const vector3d_t &V,
						PFLOAT cosang, PFLOAT z1, PFLOAT z2);
YAFRAYCORE_EXPORT vector3d_t randomVectorCone(const vector3d_t &dir, PFLOAT cosangle, PFLOAT r1, PFLOAT r2);
YAFRAYCORE_EXPORT vector3d_t discreteVectorCone(const vector3d_t &dir, PFLOAT cangle, int sample, int square);

__END_YAFRAY

#endif
