/****************************************************************************
 *
 * 			output.h: Generic output module api 
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
#ifndef __COUTPUT_H
#define __COUTPUT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "color.h"

__BEGIN_YAFRAY
class YAFRAYCORE_EXPORT colorOutput_t
{
	public:
		virtual ~colorOutput_t() {};
		virtual bool putPixel(int x, int y,const color_t &c, 
				CFLOAT alpha=0,PFLOAT depth=0)=0;
		virtual void flush()=0;
};

__END_YAFRAY
#endif
