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

#ifndef VERSION_H
#define VERSION_H

#include "common.h"

// set version macro if not set yet
#if !defined(VERSION)
#error Version was not set (should be passed to compiler).
#endif

// important strings
extern const char* kApplication;
extern const char* kCopyright;
extern const char* kContact;
extern const char* kWebsite;

// build version.  follows linux kernel style:  an even minor number implies
// a release version, odd implies development version.
extern const char* kVersion;

// application version
extern const char* kAppVersion;

#endif
