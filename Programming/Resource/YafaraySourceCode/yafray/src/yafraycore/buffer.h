/****************************************************************************
 *
 * 			buffer.h: Buffers (color and float) api
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Est�vez
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

#ifndef __BUFFER_H
#define __BUFFER_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include <cstdio>
#include <iostream>
#include "color.h"

__BEGIN_YAFRAY

template<typename T1, unsigned char T2>
class YAFRAYCORE_EXPORT gBuf_t
{
	public :
		gBuf_t(int x,int y)
		{
			data = new T1 [x*y*T2];
			if (data==NULL)
			{
				std::cerr << "Error allocating memory in cBuffer\n";
				exit(1);
			}
			mx = x;
			my = y;
		}

		gBuf_t() { data=NULL; }
		~gBuf_t() { if (data) delete[] data; }

		void set(int x,int y)
		{
			if (data) delete[] data;
			data = new T1 [x*y*T2];
			if(data==NULL)
			{
				std::cerr << "Error allocating memory in cBuffer\n";
				exit(1);
			}
			mx = x;
			my = y;
		}

		T1 * operator()(int x,int y) { return &data[(y*mx+x)*T2]; }

		gBuf_t & operator = (const gBuf_t &source)
		{
			if ((mx!=source.mx) || (my!=source.my)) std::cerr << "Error, trying to assign buffers of a different size\n";
			if ((data == NULL) || (source.data == NULL)) std::cerr << "Assigning unallocated buffers\n";
			int total = mx*my*T2;
			for(int i=0;i<total;++i)
				data[i] = source.data[i];
			return *this;
		}

		int resx() const { return mx; };
		int resy() const { return my; };
	protected :
		T1 *data;
		int mx, my;
};

typedef gBuf_t<unsigned char, 4> cBuffer_t;
typedef gBuf_t<float, 4> fcBuffer_t;	//float RGBA buffer

/*
class cBuffer_t
{
	public :
		cBuffer_t(int x,int y);
		cBuffer_t() {data=NULL;};
		~cBuffer_t();

		void set(int x,int y);
		unsigned char * operator()(int x,int y) {return &(data[(y*mx+x)*3]);};
		cBuffer_t & operator = (const cBuffer_t &source);
		int resx() const {return mx;};
		int resy() const {return my;};
	protected :
		unsigned char *data;
		int mx,my;
};
*/

class YAFRAYCORE_EXPORT fBuffer_t
{
	public :
		fBuffer_t(int x,int y);
		fBuffer_t() {data=NULL;};
		~fBuffer_t();

		void set(int x,int y);
		GFLOAT & operator()(int x,int y) {return data[y*mx+x];};
		const GFLOAT & operator()(int x,int y)const {return data[y*mx+x];};
		fBuffer_t & operator = (const fBuffer_t &source);
		const GFLOAT *buffer(int x,int y)const {return &data[y*mx+x];};
		GFLOAT *buffer(int x,int y) {return &data[y*mx+x];};
		int resx() const {return mx;};
		int resy() const {return my;};
	protected :
		GFLOAT *data;
		int mx,my;
};

template<class T>
class Buffer_t
{
	public :
		Buffer_t(int x,int y);
		Buffer_t() {data=NULL;};
		~Buffer_t();

		void set(int x,int y);
		T & operator()(int x,int y) {return data[y*mx+x];};
		T * buffer(int x, int y) {return &data[y*mx+x];};
		const T & operator()(int x,int y)const {return data[y*mx+x];};
		Buffer_t<T> & operator = (const Buffer_t<T> &source);
		int resx() const {return mx;};
		int resy() const {return my;};
	protected :
		T *data;
		int mx,my;
};

template<class T>
Buffer_t<T>::Buffer_t(int x, int y)
{
	data=new T [x*y];
	if(data==NULL)
	{
		std::cout<<"Error allocating memory in cBuffer\n";
		exit(1);
	}
	mx=x;
	my=y;
}

template<class T>
Buffer_t<T>::~Buffer_t()
{
	if(data!=NULL)
		delete [] data;
}

template<class T>
void Buffer_t<T>::set(int x, int y)
{
	if(data!=NULL)
		delete [] data;
	data=new T [x*y];
	if(data==NULL)
	{
		std::cout<<"Error allocating memory in cBuffer\n";
		exit(1);
	}
	mx=x;
	my=y;
}

template<class T>
Buffer_t<T> & Buffer_t<T>::operator = (const Buffer_t<T> &source)
{
	if( (mx!=source.mx) || (my!=source.my) )
	{
		std::cout<<"Error, trying to assign  buffers of a diferent size\n";
	}
	if( (data == NULL) || (source.data == NULL) )
	{
		std::cout<<"Assigning unallocated buffers\n";
	}
	int total=mx*my;
	for(int i=0;i<total;++i) data[i]=source.data[i];
	
	return *this;
	
}

__END_YAFRAY

#endif
