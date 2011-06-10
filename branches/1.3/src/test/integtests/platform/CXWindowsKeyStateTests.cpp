/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define TEST_ENV
#include "Global.h"

#include "CMockKeyMap.h"
#include "CMockEventQueue.h"
#include "CXWindowsKeyState.h"
#include "CLog.h"
#include <errno.h>

#define XK_LATIN1
#define XK_MISCELLANY
#include "X11/keysymdef.h"

#if HAVE_XKB_EXTENSION
#	include <X11/XKBlib.h>
#endif

class CXWindowsKeyStateTests : public ::testing::Test
{
protected:
	CXWindowsKeyStateTests() :
		m_display(NULL)
	{
	}

	~CXWindowsKeyStateTests()
	{
		if (m_display != NULL) {
			LOG((CLOG_DEBUG "closing display"));
			XCloseDisplay(m_display);
		}
	}

	virtual void
	SetUp()
	{
		// open the display only once for the entire test suite
		if (this->m_display == NULL) {
			LOG((CLOG_DEBUG "opening display"));
			this->m_display = XOpenDisplay(NULL);

			ASSERT_TRUE(this->m_display != NULL) << "unable to open display: " << errno;
		}
	}

	virtual void
	TearDown()
	{
	}

	Display* m_display;
};

TEST_F(CXWindowsKeyStateTests, setActiveGroup_pollAndSet_groupIsZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.setActiveGroup(CXWindowsKeyState::kGroupPollAndSet);

	ASSERT_EQ(0, keyState.m_group);
}

TEST_F(CXWindowsKeyStateTests, setActiveGroup_poll_groupIsNotSet)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.setActiveGroup(CXWindowsKeyState::kGroupPoll);

	ASSERT_LE(-1, keyState.m_group);
}

TEST_F(CXWindowsKeyStateTests, setActiveGroup_customGroup_groupWasSet)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.setActiveGroup(1);

	ASSERT_EQ(1, keyState.m_group);
}

TEST_F(CXWindowsKeyStateTests, mapModifiersFromX_zeroState_zeroMask)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	int mask = keyState.mapModifiersFromX(0);

	ASSERT_EQ(0, mask);
}

TEST_F(CXWindowsKeyStateTests, mapModifiersToX_zeroMask_resultIsTrue)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	unsigned int modifiers = 0;
	bool result = keyState.mapModifiersToX(0, modifiers);

	ASSERT_TRUE(result);
}

TEST_F(CXWindowsKeyStateTests, fakeCtrlAltDel_default_returnsFalse)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	bool result = keyState.fakeCtrlAltDel();

	ASSERT_FALSE(result);
}

TEST_F(CXWindowsKeyStateTests, pollActiveModifiers_defaultState_returnsZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	KeyModifierMask actual = keyState.pollActiveModifiers();

	ASSERT_EQ(0, actual);
}

TEST_F(CXWindowsKeyStateTests, pollActiveModifiers_shiftKeyPressed_shiftInMask)
{
	// NOTE: This test is sensitive to which keys you are holding down, try
	//		 not to be pressing any key when running it.
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	// set fake modifier mapping
	std::fill(
		keyState.m_modifierFromX.begin(), keyState.m_modifierFromX.end(), 0);
	keyState.m_modifierFromX[ShiftMapIndex] = KeyModifierShift;

	KeyCode key = XKeysymToKeycode(m_display, XK_Shift_L);

	// fake shift key down without using synergy
	XTestFakeKeyEvent(m_display, key, true, CurrentTime);
	KeyModifierMask modDown = keyState.pollActiveModifiers();

	// fake shift key up without using synergy
	XTestFakeKeyEvent(m_display, key, false, CurrentTime);
	KeyModifierMask modUp = keyState.pollActiveModifiers();

	EXPECT_TRUE((modDown & KeyModifierShift) == KeyModifierShift)
		<< "shift key not in mask";

	EXPECT_TRUE((modUp & KeyModifierShift) == 0)
		<< "shift key never released - although this might not be a failure - "
		"try running this test again making sure no keys are being held down";
}

TEST_F(CXWindowsKeyStateTests, pollActiveGroup_defaultState_returnsZero)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	SInt32 actual = keyState.pollActiveGroup();

	ASSERT_EQ(0, actual);
}

TEST_F(CXWindowsKeyStateTests, pollActiveGroup_positiveGroup_returnsGroup)
{
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	keyState.m_group = 3;

	SInt32 actual = keyState.pollActiveGroup();

	ASSERT_EQ(3, actual);
}

TEST_F(CXWindowsKeyStateTests, pollActiveGroup_xkb_areEqual)
{
#if HAVE_XKB_EXTENSION
	CMockKeyMap keyMap;
	CMockEventQueue eventQueue;
	CXWindowsKeyState keyState(
		m_display, true, (IEventQueue&)keyMap, (CKeyMap&)eventQueue);

	// reset the group
	keyState.m_group = -1;

	XkbStateRec state;

	// compare pollActiveGroup() with XkbGetState()
	if (XkbGetState(m_display, XkbUseCoreKbd, &state)) {
		SInt32 actual = keyState.pollActiveGroup();

		ASSERT_EQ(state.group, actual);
	}
	else {
		// TODO: Investigate on XkbGetState() failing every time
		FAIL() << "XkbGetState() returned an error: " << errno;
	}
#else
	SUCCEED() << "Xkb extension not installed";
#endif
}
