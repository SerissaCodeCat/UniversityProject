/****************************************************************************
 *
 * 			vector3d.cc: Vector 3d and point manipulation implementation 
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

#include "vector3d.h"
#include "matrix4.h"
//#include<cmath>


__BEGIN_YAFRAY

std::ostream & operator << (std::ostream &out,const vector3d_t &v)
{
	out<<"("<<v.x<<","<<v.y<<","<<v.z<<")";
	return out;
}


std::ostream & operator << (std::ostream &out,const point3d_t &p)
{
	out<<"("<<p.x<<","<<p.y<<","<<p.z<<")";
	return out;
}


bool  operator == ( const vector3d_t &a,const vector3d_t &b)
{
	if(a.x!=b.x) return false;
	if(a.y!=b.y) return false;
	if(a.z!=b.z) return false;
	return true;
}

bool  operator != ( const vector3d_t &a,const vector3d_t &b)
{
	if(a.x!=b.x) return true;
	if(a.y!=b.y) return true;
	if(a.z!=b.z) return true;
	return false;
}

vector3d_t refract(const vector3d_t &n,const vector3d_t &v,PFLOAT IOR)
{
	vector3d_t N=n,I,T;
	PFLOAT eta=IOR;
	I=-v;
	if((v*n)<0)
	{
		N=-n;
		eta=IOR;
	}
	else
	{
		N=n;
		eta=1.0/IOR;
	}
	PFLOAT IdotN = v*N;
	PFLOAT k = 1 - eta*eta*(1 - IdotN*IdotN);
	T= (k < 0) ? vector3d_t(0,0,0) : (eta*I + (eta*IdotN - sqrt(k))*N);
	T.normalize();
	return T;
}

void fresnel(const vector3d_t & I, const vector3d_t & n, PFLOAT IOR,
						CFLOAT &Kr,CFLOAT &Kt)
{
	PFLOAT eta;
	vector3d_t N;

	if((I*n)<0)
	{
		//eta=1.0/IOR;
		eta=IOR;
		N=-n;
	}
	else
	{
		eta=IOR;
		N=n;
	}
	PFLOAT c=I*N;
	PFLOAT g=eta*eta+c*c-1;
	if(g<=0)
		g=0;
	else
		g=sqrt(g);
	PFLOAT aux=c*(g+c);

	Kr=( ( 0.5*(g-c)*(g-c) )/( (g+c)*(g+c) ) ) *
		   ( 1+ ((aux-1)*(aux-1))/( (aux+1)*(aux+1) ) );
	if(Kr<1.0)
		Kt=1-Kr;
	else
		Kt=0;
}


// 'Faster' Schlick fresnel approximation,
void fast_fresnel(const vector3d_t & I, const vector3d_t & n, PFLOAT IORF,
		CFLOAT &Kr, CFLOAT &Kt)
{
	PFLOAT t = 1 - (I*n);
	//t = (t<0)?0:((t>1)?1:t);
	PFLOAT t2 = t*t;
	Kr = IORF + (1 - IORF) * t2*t2*t;
	Kt = 1-Kr;
}

// P.Shirley's concentric disk algorithm, maps square to disk
void ShirleyDisk(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v)
{
	PFLOAT phi=0, r=0, a=2*r1-1, b=2*r2-1;
	if (a>-b) {
		if (a>b) {	// Reg.1
			r = a;
			phi = M_PI_4 * (b/a);
		}
		else {			// Reg.2
			r = b;
			phi = M_PI_4 * (2 - a/b);
		}
	}
	else {
		if (a<b) {	// Reg.3
			r = -a;
			phi = M_PI_4 * (4 + b/a);
		}
		else {			// Reg.4
			r = -b;
			if (b!=0)
				phi = M_PI_4 * (6 - a/b);
			else
				phi = 0;
		}
	}
	u = r * cos(phi);
	v = r * sin(phi);
}



YAFRAYCORE_EXPORT int myseed=123212;

vector3d_t randomVectorCone(const vector3d_t &D,
				const vector3d_t &U, const vector3d_t &V,
				PFLOAT cosang, PFLOAT z1, PFLOAT z2)
{
  PFLOAT t1=2.0*M_PI*z1, t2=1.0-(1.0-cosang)*z2;
  return (U*cos(t1) + V*sin(t1))*sqrt(1.0-t2*t2) + D*t2;
}

vector3d_t randomVectorCone(const vector3d_t &dir, PFLOAT cangle, PFLOAT r1, PFLOAT r2)
{
	vector3d_t u, v;
	createCS(dir, u, v);
	return randomVectorCone(dir, u, v, cangle, r1, r2);
}

vector3d_t discreteVectorCone(const vector3d_t &dir, PFLOAT cangle, int sample, int square)
{
	PFLOAT r1=(PFLOAT)(sample / square)/(PFLOAT)square;
	PFLOAT r2=(PFLOAT)(sample % square)/(PFLOAT)square;
	PFLOAT tt = 2*M_PI * r1;
	PFLOAT ss = std::acos(1.0 - (1.0 - cangle)*r2);
	vector3d_t	vx(std::cos(ss),std::sin(ss)*cos(tt),std::sin(ss)*std::sin(tt));
	vector3d_t	i(1,0,0),c;
	matrix4x4_t M(1);
	if((fabs(dir.y)>0.0) || (fabs(dir.z)>0.0))
	{
		M[0][0]=dir.x;
		M[1][0]=dir.y;
		M[2][0]=dir.z;
		c=i^dir;
		c.normalize();
		M[0][1]=c.x;
		M[1][1]=c.y;
		M[2][1]=c.z;
		c=dir^c;
		c.normalize();
		M[0][2]=c.x;
		M[1][2]=c.y;
		M[2][2]=c.z;
	}
	else if(dir.x<0.0) M[0][0]=-1.0;
	return M*vx;
}

__END_YAFRAY
