/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef IINTERFACE_H
#define IINTERFACE_H

#include "common.h"

//! Base class of interfaces
/*!
This is the base class of all interface classes.  An interface class has
only pure virtual methods.
*/
class IInterface {
public:
	//! Interface destructor does nothing
	virtual ~IInterface() { }
};

#endif
