/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "XScreen.h"

//
// XScreenOpenFailure
//

CString
XScreenOpenFailure::getWhat() const throw()
{
	return format("XScreenOpenFailure", "unable to open screen");
}


//
// XScreenXInputFailure
//

CString
XScreenXInputFailure::getWhat() const throw()
{
	return "";
}


//
// XScreenUnavailable
//

XScreenUnavailable::XScreenUnavailable(double timeUntilRetry) :
	m_timeUntilRetry(timeUntilRetry)
{
	// do nothing
}

XScreenUnavailable::~XScreenUnavailable()
{
	// do nothing
}

double
XScreenUnavailable::getRetryTime() const
{
	return m_timeUntilRetry;
}

CString
XScreenUnavailable::getWhat() const throw()
{
	return format("XScreenUnavailable", "unable to open screen");
}
