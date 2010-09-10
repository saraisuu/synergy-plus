/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#include "CMSWindowsKeyState.h"
#include "CMSWindowsDesks.h"
#include "CThread.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "CStringUtil.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "CArchMiscWindows.h"

// extended mouse buttons
#if !defined(VK_XBUTTON1)
#define VK_XBUTTON1				0x05
#define VK_XBUTTON2				0x06
#endif

//
// CMSWindowsKeyState
//

// map virtual keys to synergy key enumeration
const KeyID				CMSWindowsKeyState::s_virtualKey[] =
{
	/* 0x000 */ { kKeyNone },		// reserved
	/* 0x001 */ { kKeyNone },		// VK_LBUTTON
	/* 0x002 */ { kKeyNone },		// VK_RBUTTON
	/* 0x003 */ { kKeyNone },		// VK_CANCEL
	/* 0x004 */ { kKeyNone },		// VK_MBUTTON
	/* 0x005 */ { kKeyNone },		// VK_XBUTTON1
	/* 0x006 */ { kKeyNone },		// VK_XBUTTON2
	/* 0x007 */ { kKeyNone },		// undefined
	/* 0x008 */ { kKeyBackSpace },	// VK_BACK
	/* 0x009 */ { kKeyTab },		// VK_TAB
	/* 0x00a */ { kKeyNone },		// undefined
	/* 0x00b */ { kKeyNone },		// undefined
	/* 0x00c */ { kKeyClear },		// VK_CLEAR
	/* 0x00d */ { kKeyReturn },		// VK_RETURN
	/* 0x00e */ { kKeyNone },		// undefined
	/* 0x00f */ { kKeyNone },		// undefined
	/* 0x010 */ { kKeyShift_L },	// VK_SHIFT
	/* 0x011 */ { kKeyControl_L },	// VK_CONTROL
	/* 0x012 */ { kKeyAlt_L },		// VK_MENU
	/* 0x013 */ { kKeyPause },		// VK_PAUSE
	/* 0x014 */ { kKeyCapsLock },	// VK_CAPITAL
	/* 0x015 */ { kKeyNone },		// VK_KANA			
	/* 0x016 */ { kKeyNone },		// VK_HANGUL		
	/* 0x017 */ { kKeyNone },		// VK_JUNJA			
	/* 0x018 */ { kKeyNone },		// VK_FINAL			
	/* 0x019 */ { kKeyZenkaku },	// VK_KANJI			
	/* 0x01a */ { kKeyNone },		// undefined
	/* 0x01b */ { kKeyEscape },		// VK_ESCAPE
	/* 0x01c */ { kKeyHenkan },		// VK_CONVERT		
	/* 0x01d */ { kKeyNone },		// VK_NONCONVERT	
	/* 0x01e */ { kKeyNone },		// VK_ACCEPT		
	/* 0x01f */ { kKeyNone },		// VK_MODECHANGE	
	/* 0x020 */ { kKeyNone },		// VK_SPACE
	/* 0x021 */ { kKeyKP_PageUp },	// VK_PRIOR
	/* 0x022 */ { kKeyKP_PageDown },// VK_NEXT
	/* 0x023 */ { kKeyKP_End },		// VK_END
	/* 0x024 */ { kKeyKP_Home },	// VK_HOME
	/* 0x025 */ { kKeyKP_Left },	// VK_LEFT
	/* 0x026 */ { kKeyKP_Up },		// VK_UP
	/* 0x027 */ { kKeyKP_Right },	// VK_RIGHT
	/* 0x028 */ { kKeyKP_Down },	// VK_DOWN
	/* 0x029 */ { kKeySelect },		// VK_SELECT
	/* 0x02a */ { kKeyNone },		// VK_PRINT
	/* 0x02b */ { kKeyExecute },	// VK_EXECUTE
	/* 0x02c */ { kKeyPrint },		// VK_SNAPSHOT
	/* 0x02d */ { kKeyKP_Insert },	// VK_INSERT
	/* 0x02e */ { kKeyKP_Delete },	// VK_DELETE
	/* 0x02f */ { kKeyHelp },		// VK_HELP
	/* 0x030 */ { kKeyNone },		// VK_0
	/* 0x031 */ { kKeyNone },		// VK_1
	/* 0x032 */ { kKeyNone },		// VK_2
	/* 0x033 */ { kKeyNone },		// VK_3
	/* 0x034 */ { kKeyNone },		// VK_4
	/* 0x035 */ { kKeyNone },		// VK_5
	/* 0x036 */ { kKeyNone },		// VK_6
	/* 0x037 */ { kKeyNone },		// VK_7
	/* 0x038 */ { kKeyNone },		// VK_8
	/* 0x039 */ { kKeyNone },		// VK_9
	/* 0x03a */ { kKeyNone },		// undefined
	/* 0x03b */ { kKeyNone },		// undefined
	/* 0x03c */ { kKeyNone },		// undefined
	/* 0x03d */ { kKeyNone },		// undefined
	/* 0x03e */ { kKeyNone },		// undefined
	/* 0x03f */ { kKeyNone },		// undefined
	/* 0x040 */ { kKeyNone },		// undefined
	/* 0x041 */ { kKeyNone },		// VK_A
	/* 0x042 */ { kKeyNone },		// VK_B
	/* 0x043 */ { kKeyNone },		// VK_C
	/* 0x044 */ { kKeyNone },		// VK_D
	/* 0x045 */ { kKeyNone },		// VK_E
	/* 0x046 */ { kKeyNone },		// VK_F
	/* 0x047 */ { kKeyNone },		// VK_G
	/* 0x048 */ { kKeyNone },		// VK_H
	/* 0x049 */ { kKeyNone },		// VK_I
	/* 0x04a */ { kKeyNone },		// VK_J
	/* 0x04b */ { kKeyNone },		// VK_K
	/* 0x04c */ { kKeyNone },		// VK_L
	/* 0x04d */ { kKeyNone },		// VK_M
	/* 0x04e */ { kKeyNone },		// VK_N
	/* 0x04f */ { kKeyNone },		// VK_O
	/* 0x050 */ { kKeyNone },		// VK_P
	/* 0x051 */ { kKeyNone },		// VK_Q
	/* 0x052 */ { kKeyNone },		// VK_R
	/* 0x053 */ { kKeyNone },		// VK_S
	/* 0x054 */ { kKeyNone },		// VK_T
	/* 0x055 */ { kKeyNone },		// VK_U
	/* 0x056 */ { kKeyNone },		// VK_V
	/* 0x057 */ { kKeyNone },		// VK_W
	/* 0x058 */ { kKeyNone },		// VK_X
	/* 0x059 */ { kKeyNone },		// VK_Y
	/* 0x05a */ { kKeyNone },		// VK_Z
	/* 0x05b */ { kKeySuper_L },	// VK_LWIN
	/* 0x05c */ { kKeySuper_R },	// VK_RWIN
	/* 0x05d */ { kKeyMenu },		// VK_APPS
	/* 0x05e */ { kKeyNone },		// undefined
	/* 0x05f */ { kKeySleep },		// VK_SLEEP
	/* 0x060 */ { kKeyKP_0 },		// VK_NUMPAD0
	/* 0x061 */ { kKeyKP_1 },		// VK_NUMPAD1
	/* 0x062 */ { kKeyKP_2 },		// VK_NUMPAD2
	/* 0x063 */ { kKeyKP_3 },		// VK_NUMPAD3
	/* 0x064 */ { kKeyKP_4 },		// VK_NUMPAD4
	/* 0x065 */ { kKeyKP_5 },		// VK_NUMPAD5
	/* 0x066 */ { kKeyKP_6 },		// VK_NUMPAD6
	/* 0x067 */ { kKeyKP_7 },		// VK_NUMPAD7
	/* 0x068 */ { kKeyKP_8 },		// VK_NUMPAD8
	/* 0x069 */ { kKeyKP_9 },		// VK_NUMPAD9
	/* 0x06a */ { kKeyKP_Multiply },// VK_MULTIPLY
	/* 0x06b */ { kKeyKP_Add },		// VK_ADD
	/* 0x06c */ { kKeyKP_Separator },// VK_SEPARATOR
	/* 0x06d */ { kKeyKP_Subtract },// VK_SUBTRACT
	/* 0x06e */ { kKeyKP_Decimal },	// VK_DECIMAL
	/* 0x06f */ { kKeyNone },		// VK_DIVIDE
	/* 0x070 */ { kKeyF1 },			// VK_F1
	/* 0x071 */ { kKeyF2 },			// VK_F2
	/* 0x072 */ { kKeyF3 },			// VK_F3
	/* 0x073 */ { kKeyF4 },			// VK_F4
	/* 0x074 */ { kKeyF5 },			// VK_F5
	/* 0x075 */ { kKeyF6 },			// VK_F6
	/* 0x076 */ { kKeyF7 },			// VK_F7
	/* 0x077 */ { kKeyF8 },			// VK_F8
	/* 0x078 */ { kKeyF9 },			// VK_F9
	/* 0x079 */ { kKeyF10 },		// VK_F10
	/* 0x07a */ { kKeyF11 },		// VK_F11
	/* 0x07b */ { kKeyF12 },		// VK_F12
	/* 0x07c */ { kKeyF13 },		// VK_F13
	/* 0x07d */ { kKeyF14 },		// VK_F14
	/* 0x07e */ { kKeyF15 },		// VK_F15
	/* 0x07f */ { kKeyF16 },		// VK_F16
	/* 0x080 */ { kKeyF17 },		// VK_F17
	/* 0x081 */ { kKeyF18 },		// VK_F18
	/* 0x082 */ { kKeyF19 },		// VK_F19
	/* 0x083 */ { kKeyF20 },		// VK_F20
	/* 0x084 */ { kKeyF21 },		// VK_F21
	/* 0x085 */ { kKeyF22 },		// VK_F22
	/* 0x086 */ { kKeyF23 },		// VK_F23
	/* 0x087 */ { kKeyF24 },		// VK_F24
	/* 0x088 */ { kKeyNone },		// unassigned
	/* 0x089 */ { kKeyNone },		// unassigned
	/* 0x08a */ { kKeyNone },		// unassigned
	/* 0x08b */ { kKeyNone },		// unassigned
	/* 0x08c */ { kKeyNone },		// unassigned
	/* 0x08d */ { kKeyNone },		// unassigned
	/* 0x08e */ { kKeyNone },		// unassigned
	/* 0x08f */ { kKeyNone },		// unassigned
	/* 0x090 */ { kKeyNumLock },	// VK_NUMLOCK
	/* 0x091 */ { kKeyScrollLock },	// VK_SCROLL
	/* 0x092 */ { kKeyNone },		// unassigned
	/* 0x093 */ { kKeyNone },		// unassigned
	/* 0x094 */ { kKeyNone },		// unassigned
	/* 0x095 */ { kKeyNone },		// unassigned
	/* 0x096 */ { kKeyNone },		// unassigned
	/* 0x097 */ { kKeyNone },		// unassigned
	/* 0x098 */ { kKeyNone },		// unassigned
	/* 0x099 */ { kKeyNone },		// unassigned
	/* 0x09a */ { kKeyNone },		// unassigned
	/* 0x09b */ { kKeyNone },		// unassigned
	/* 0x09c */ { kKeyNone },		// unassigned
	/* 0x09d */ { kKeyNone },		// unassigned
	/* 0x09e */ { kKeyNone },		// unassigned
	/* 0x09f */ { kKeyNone },		// unassigned
	/* 0x0a0 */ { kKeyShift_L },	// VK_LSHIFT
	/* 0x0a1 */ { kKeyShift_R },	// VK_RSHIFT
	/* 0x0a2 */ { kKeyControl_L },	// VK_LCONTROL
	/* 0x0a3 */ { kKeyControl_R },	// VK_RCONTROL
	/* 0x0a4 */ { kKeyAlt_L },		// VK_LMENU
	/* 0x0a5 */ { kKeyAltGr },		// VK_RMENU
	/* 0x0a6 */ { kKeyNone },		// VK_BROWSER_BACK
	/* 0x0a7 */ { kKeyNone },		// VK_BROWSER_FORWARD
	/* 0x0a8 */ { kKeyNone },		// VK_BROWSER_REFRESH
	/* 0x0a9 */ { kKeyNone },		// VK_BROWSER_STOP
	/* 0x0aa */ { kKeyNone },		// VK_BROWSER_SEARCH
	/* 0x0ab */ { kKeyNone },		// VK_BROWSER_FAVORITES
	/* 0x0ac */ { kKeyNone },		// VK_BROWSER_HOME
	/* 0x0ad */ { kKeyNone },		// VK_VOLUME_MUTE
	/* 0x0ae */ { kKeyNone },		// VK_VOLUME_DOWN
	/* 0x0af */ { kKeyNone },		// VK_VOLUME_UP
	/* 0x0b0 */ { kKeyNone },		// VK_MEDIA_NEXT_TRACK
	/* 0x0b1 */ { kKeyNone },		// VK_MEDIA_PREV_TRACK
	/* 0x0b2 */ { kKeyNone },		// VK_MEDIA_STOP
	/* 0x0b3 */ { kKeyNone },		// VK_MEDIA_PLAY_PAUSE
	/* 0x0b4 */ { kKeyNone },		// VK_LAUNCH_MAIL
	/* 0x0b5 */ { kKeyNone },		// VK_LAUNCH_MEDIA_SELECT
	/* 0x0b6 */ { kKeyNone },		// VK_LAUNCH_APP1
	/* 0x0b7 */ { kKeyNone },		// VK_LAUNCH_APP2
	/* 0x0b8 */ { kKeyNone },		// unassigned
	/* 0x0b9 */ { kKeyNone },		// unassigned
	/* 0x0ba */ { kKeyNone },		// OEM specific
	/* 0x0bb */ { kKeyNone },		// OEM specific
	/* 0x0bc */ { kKeyNone },		// OEM specific
	/* 0x0bd */ { kKeyNone },		// OEM specific
	/* 0x0be */ { kKeyNone },		// OEM specific
	/* 0x0bf */ { kKeyNone },		// OEM specific
	/* 0x0c0 */ { kKeyNone },		// OEM specific
	/* 0x0c1 */ { kKeyNone },		// unassigned
	/* 0x0c2 */ { kKeyNone },		// unassigned
	/* 0x0c3 */ { kKeyNone },		// unassigned
	/* 0x0c4 */ { kKeyNone },		// unassigned
	/* 0x0c5 */ { kKeyNone },		// unassigned
	/* 0x0c6 */ { kKeyNone },		// unassigned
	/* 0x0c7 */ { kKeyNone },		// unassigned
	/* 0x0c8 */ { kKeyNone },		// unassigned
	/* 0x0c9 */ { kKeyNone },		// unassigned
	/* 0x0ca */ { kKeyNone },		// unassigned
	/* 0x0cb */ { kKeyNone },		// unassigned
	/* 0x0cc */ { kKeyNone },		// unassigned
	/* 0x0cd */ { kKeyNone },		// unassigned
	/* 0x0ce */ { kKeyNone },		// unassigned
	/* 0x0cf */ { kKeyNone },		// unassigned
	/* 0x0d0 */ { kKeyNone },		// unassigned
	/* 0x0d1 */ { kKeyNone },		// unassigned
	/* 0x0d2 */ { kKeyNone },		// unassigned
	/* 0x0d3 */ { kKeyNone },		// unassigned
	/* 0x0d4 */ { kKeyNone },		// unassigned
	/* 0x0d5 */ { kKeyNone },		// unassigned
	/* 0x0d6 */ { kKeyNone },		// unassigned
	/* 0x0d7 */ { kKeyNone },		// unassigned
	/* 0x0d8 */ { kKeyNone },		// unassigned
	/* 0x0d9 */ { kKeyNone },		// unassigned
	/* 0x0da */ { kKeyNone },		// unassigned
	/* 0x0db */ { kKeyNone },		// OEM specific
	/* 0x0dc */ { kKeyNone },		// OEM specific
	/* 0x0dd */ { kKeyNone },		// OEM specific
	/* 0x0de */ { kKeyNone },		// OEM specific
	/* 0x0df */ { kKeyNone },		// OEM specific
	/* 0x0e0 */ { kKeyNone },		// OEM specific
	/* 0x0e1 */ { kKeyNone },		// OEM specific
	/* 0x0e2 */ { kKeyNone },		// OEM specific
	/* 0x0e3 */ { kKeyNone },		// OEM specific
	/* 0x0e4 */ { kKeyNone },		// OEM specific
	/* 0x0e5 */ { kKeyNone },		// unassigned
	/* 0x0e6 */ { kKeyNone },		// OEM specific
	/* 0x0e7 */ { kKeyNone },		// unassigned
	/* 0x0e8 */ { kKeyNone },		// unassigned
	/* 0x0e9 */ { kKeyNone },		// OEM specific
	/* 0x0ea */ { kKeyNone },		// OEM specific
	/* 0x0eb */ { kKeyNone },		// OEM specific
	/* 0x0ec */ { kKeyNone },		// OEM specific
	/* 0x0ed */ { kKeyNone },		// OEM specific
	/* 0x0ee */ { kKeyNone },		// OEM specific
	/* 0x0ef */ { kKeyNone },		// OEM specific
	/* 0x0f0 */ { kKeyNone },		// OEM specific
	/* 0x0f1 */ { kKeyNone },		// OEM specific
	/* 0x0f2 */ { kKeyNone },		// OEM specific
	/* 0x0f3 */ { kKeyNone },		// OEM specific
	/* 0x0f4 */ { kKeyNone },		// OEM specific
	/* 0x0f5 */ { kKeyNone },		// OEM specific
	/* 0x0f6 */ { kKeyNone },		// VK_ATTN			
	/* 0x0f7 */ { kKeyNone },		// VK_CRSEL			
	/* 0x0f8 */ { kKeyNone },		// VK_EXSEL			
	/* 0x0f9 */ { kKeyNone },		// VK_EREOF			
	/* 0x0fa */ { kKeyNone },		// VK_PLAY			
	/* 0x0fb */ { kKeyNone },		// VK_ZOOM			
	/* 0x0fc */ { kKeyNone },		// reserved
	/* 0x0fd */ { kKeyNone },		// VK_PA1			
	/* 0x0fe */ { kKeyNone },		// VK_OEM_CLEAR		
	/* 0x0ff */ { kKeyNone },		// reserved

	/* 0x100 */ { kKeyNone },		// reserved
	/* 0x101 */ { kKeyNone },		// VK_LBUTTON
	/* 0x102 */ { kKeyNone },		// VK_RBUTTON
	/* 0x103 */ { kKeyBreak },		// VK_CANCEL
	/* 0x104 */ { kKeyNone },		// VK_MBUTTON
	/* 0x105 */ { kKeyNone },		// VK_XBUTTON1
	/* 0x106 */ { kKeyNone },		// VK_XBUTTON2
	/* 0x107 */ { kKeyNone },		// undefined
	/* 0x108 */ { kKeyNone },		// VK_BACK
	/* 0x109 */ { kKeyNone },		// VK_TAB
	/* 0x10a */ { kKeyNone },		// undefined
	/* 0x10b */ { kKeyNone },		// undefined
	/* 0x10c */ { kKeyClear },		// VK_CLEAR
	/* 0x10d */ { kKeyKP_Enter },	// VK_RETURN
	/* 0x10e */ { kKeyNone },		// undefined
	/* 0x10f */ { kKeyNone },		// undefined
	/* 0x110 */ { kKeyShift_R },	// VK_SHIFT
	/* 0x111 */ { kKeyControl_R },	// VK_CONTROL
	/* 0x112 */ { kKeyAlt_R },		// VK_MENU
	/* 0x113 */ { kKeyNone },		// VK_PAUSE
	/* 0x114 */ { kKeyNone },		// VK_CAPITAL
	/* 0x115 */ { kKeyNone },		// VK_KANA			
	/* 0x116 */ { kKeyNone },		// VK_HANGUL		
	/* 0x117 */ { kKeyNone },		// VK_JUNJA			
	/* 0x118 */ { kKeyNone },		// VK_FINAL			
	/* 0x119 */ { kKeyNone },		// VK_KANJI			
	/* 0x11a */ { kKeyNone },		// undefined
	/* 0x11b */ { kKeyNone },		// VK_ESCAPE
	/* 0x11c */ { kKeyNone },		// VK_CONVERT		
	/* 0x11d */ { kKeyNone },		// VK_NONCONVERT	
	/* 0x11e */ { kKeyNone },		// VK_ACCEPT		
	/* 0x11f */ { kKeyNone },		// VK_MODECHANGE	
	/* 0x120 */ { kKeyNone },		// VK_SPACE
	/* 0x121 */ { kKeyPageUp },		// VK_PRIOR
	/* 0x122 */ { kKeyPageDown },	// VK_NEXT
	/* 0x123 */ { kKeyEnd },		// VK_END
	/* 0x124 */ { kKeyHome },		// VK_HOME
	/* 0x125 */ { kKeyLeft },		// VK_LEFT
	/* 0x126 */ { kKeyUp },			// VK_UP
	/* 0x127 */ { kKeyRight },		// VK_RIGHT
	/* 0x128 */ { kKeyDown },		// VK_DOWN
	/* 0x129 */ { kKeySelect },		// VK_SELECT
	/* 0x12a */ { kKeyNone },		// VK_PRINT
	/* 0x12b */ { kKeyExecute },	// VK_EXECUTE
	/* 0x12c */ { kKeyPrint },		// VK_SNAPSHOT
	/* 0x12d */ { kKeyInsert },		// VK_INSERT
	/* 0x12e */ { kKeyDelete },		// VK_DELETE
	/* 0x12f */ { kKeyHelp },		// VK_HELP
	/* 0x130 */ { kKeyNone },		// VK_0
	/* 0x131 */ { kKeyNone },		// VK_1
	/* 0x132 */ { kKeyNone },		// VK_2
	/* 0x133 */ { kKeyNone },		// VK_3
	/* 0x134 */ { kKeyNone },		// VK_4
	/* 0x135 */ { kKeyNone },		// VK_5
	/* 0x136 */ { kKeyNone },		// VK_6
	/* 0x137 */ { kKeyNone },		// VK_7
	/* 0x138 */ { kKeyNone },		// VK_8
	/* 0x139 */ { kKeyNone },		// VK_9
	/* 0x13a */ { kKeyNone },		// undefined
	/* 0x13b */ { kKeyNone },		// undefined
	/* 0x13c */ { kKeyNone },		// undefined
	/* 0x13d */ { kKeyNone },		// undefined
	/* 0x13e */ { kKeyNone },		// undefined
	/* 0x13f */ { kKeyNone },		// undefined
	/* 0x140 */ { kKeyNone },		// undefined
	/* 0x141 */ { kKeyNone },		// VK_A
	/* 0x142 */ { kKeyNone },		// VK_B
	/* 0x143 */ { kKeyNone },		// VK_C
	/* 0x144 */ { kKeyNone },		// VK_D
	/* 0x145 */ { kKeyNone },		// VK_E
	/* 0x146 */ { kKeyNone },		// VK_F
	/* 0x147 */ { kKeyNone },		// VK_G
	/* 0x148 */ { kKeyNone },		// VK_H
	/* 0x149 */ { kKeyNone },		// VK_I
	/* 0x14a */ { kKeyNone },		// VK_J
	/* 0x14b */ { kKeyNone },		// VK_K
	/* 0x14c */ { kKeyNone },		// VK_L
	/* 0x14d */ { kKeyNone },		// VK_M
	/* 0x14e */ { kKeyNone },		// VK_N
	/* 0x14f */ { kKeyNone },		// VK_O
	/* 0x150 */ { kKeyNone },		// VK_P
	/* 0x151 */ { kKeyNone },		// VK_Q
	/* 0x152 */ { kKeyNone },		// VK_R
	/* 0x153 */ { kKeyNone },		// VK_S
	/* 0x154 */ { kKeyNone },		// VK_T
	/* 0x155 */ { kKeyNone },		// VK_U
	/* 0x156 */ { kKeyNone },		// VK_V
	/* 0x157 */ { kKeyNone },		// VK_W
	/* 0x158 */ { kKeyNone },		// VK_X
	/* 0x159 */ { kKeyNone },		// VK_Y
	/* 0x15a */ { kKeyNone },		// VK_Z
	/* 0x15b */ { kKeySuper_L },	// VK_LWIN
	/* 0x15c */ { kKeySuper_R },	// VK_RWIN
	/* 0x15d */ { kKeyMenu },		// VK_APPS
	/* 0x15e */ { kKeyNone },		// undefined
	/* 0x15f */ { kKeyNone },		// VK_SLEEP
	/* 0x160 */ { kKeyNone },		// VK_NUMPAD0
	/* 0x161 */ { kKeyNone },		// VK_NUMPAD1
	/* 0x162 */ { kKeyNone },		// VK_NUMPAD2
	/* 0x163 */ { kKeyNone },		// VK_NUMPAD3
	/* 0x164 */ { kKeyNone },		// VK_NUMPAD4
	/* 0x165 */ { kKeyNone },		// VK_NUMPAD5
	/* 0x166 */ { kKeyNone },		// VK_NUMPAD6
	/* 0x167 */ { kKeyNone },		// VK_NUMPAD7
	/* 0x168 */ { kKeyNone },		// VK_NUMPAD8
	/* 0x169 */ { kKeyNone },		// VK_NUMPAD9
	/* 0x16a */ { kKeyNone },		// VK_MULTIPLY
	/* 0x16b */ { kKeyNone },		// VK_ADD
	/* 0x16c */ { kKeyKP_Separator },// VK_SEPARATOR
	/* 0x16d */ { kKeyNone },		// VK_SUBTRACT
	/* 0x16e */ { kKeyNone },		// VK_DECIMAL
	/* 0x16f */ { kKeyKP_Divide },	// VK_DIVIDE
	/* 0x170 */ { kKeyNone },		// VK_F1
	/* 0x171 */ { kKeyNone },		// VK_F2
	/* 0x172 */ { kKeyNone },		// VK_F3
	/* 0x173 */ { kKeyNone },		// VK_F4
	/* 0x174 */ { kKeyNone },		// VK_F5
	/* 0x175 */ { kKeyNone },		// VK_F6
	/* 0x176 */ { kKeyNone },		// VK_F7
	/* 0x177 */ { kKeyNone },		// VK_F8
	/* 0x178 */ { kKeyNone },		// VK_F9
	/* 0x179 */ { kKeyNone },		// VK_F10
	/* 0x17a */ { kKeyNone },		// VK_F11
	/* 0x17b */ { kKeyNone },		// VK_F12
	/* 0x17c */ { kKeyF13 },		// VK_F13
	/* 0x17d */ { kKeyF14 },		// VK_F14
	/* 0x17e */ { kKeyF15 },		// VK_F15
	/* 0x17f */ { kKeyF16 },		// VK_F16
	/* 0x180 */ { kKeyF17 },		// VK_F17
	/* 0x181 */ { kKeyF18 },		// VK_F18
	/* 0x182 */ { kKeyF19 },		// VK_F19
	/* 0x183 */ { kKeyF20 },		// VK_F20
	/* 0x184 */ { kKeyF21 },		// VK_F21
	/* 0x185 */ { kKeyF22 },		// VK_F22
	/* 0x186 */ { kKeyF23 },		// VK_F23
	/* 0x187 */ { kKeyF24 },		// VK_F24
	/* 0x188 */ { kKeyNone },		// unassigned
	/* 0x189 */ { kKeyNone },		// unassigned
	/* 0x18a */ { kKeyNone },		// unassigned
	/* 0x18b */ { kKeyNone },		// unassigned
	/* 0x18c */ { kKeyNone },		// unassigned
	/* 0x18d */ { kKeyNone },		// unassigned
	/* 0x18e */ { kKeyNone },		// unassigned
	/* 0x18f */ { kKeyNone },		// unassigned
	/* 0x190 */ { kKeyNumLock },	// VK_NUMLOCK
	/* 0x191 */ { kKeyNone },		// VK_SCROLL
	/* 0x192 */ { kKeyNone },		// unassigned
	/* 0x193 */ { kKeyNone },		// unassigned
	/* 0x194 */ { kKeyNone },		// unassigned
	/* 0x195 */ { kKeyNone },		// unassigned
	/* 0x196 */ { kKeyNone },		// unassigned
	/* 0x197 */ { kKeyNone },		// unassigned
	/* 0x198 */ { kKeyNone },		// unassigned
	/* 0x199 */ { kKeyNone },		// unassigned
	/* 0x19a */ { kKeyNone },		// unassigned
	/* 0x19b */ { kKeyNone },		// unassigned
	/* 0x19c */ { kKeyNone },		// unassigned
	/* 0x19d */ { kKeyNone },		// unassigned
	/* 0x19e */ { kKeyNone },		// unassigned
	/* 0x19f */ { kKeyNone },		// unassigned
	/* 0x1a0 */ { kKeyShift_L },	// VK_LSHIFT
	/* 0x1a1 */ { kKeyShift_R },	// VK_RSHIFT
	/* 0x1a2 */ { kKeyControl_L },	// VK_LCONTROL
	/* 0x1a3 */ { kKeyControl_R },	// VK_RCONTROL
	/* 0x1a4 */ { kKeyAlt_L },		// VK_LMENU
	/* 0x1a5 */ { kKeyAltGr },		// VK_RMENU
	/* 0x1a6 */ { kKeyWWWBack },	// VK_BROWSER_BACK
	/* 0x1a7 */ { kKeyWWWForward },	// VK_BROWSER_FORWARD
	/* 0x1a8 */ { kKeyWWWRefresh },	// VK_BROWSER_REFRESH
	/* 0x1a9 */ { kKeyWWWStop },	// VK_BROWSER_STOP
	/* 0x1aa */ { kKeyWWWSearch },	// VK_BROWSER_SEARCH
	/* 0x1ab */ { kKeyWWWFavorites },// VK_BROWSER_FAVORITES
	/* 0x1ac */ { kKeyWWWHome },	// VK_BROWSER_HOME
	/* 0x1ad */ { kKeyAudioMute },	// VK_VOLUME_MUTE
	/* 0x1ae */ { kKeyAudioDown },	// VK_VOLUME_DOWN
	/* 0x1af */ { kKeyAudioUp },	// VK_VOLUME_UP
	/* 0x1b0 */ { kKeyAudioNext },	// VK_MEDIA_NEXT_TRACK
	/* 0x1b1 */ { kKeyAudioPrev },	// VK_MEDIA_PREV_TRACK
	/* 0x1b2 */ { kKeyAudioStop },	// VK_MEDIA_STOP
	/* 0x1b3 */ { kKeyAudioPlay },	// VK_MEDIA_PLAY_PAUSE
	/* 0x1b4 */ { kKeyAppMail },	// VK_LAUNCH_MAIL
	/* 0x1b5 */ { kKeyAppMedia },	// VK_LAUNCH_MEDIA_SELECT
	/* 0x1b6 */ { kKeyAppUser1 },	// VK_LAUNCH_APP1
	/* 0x1b7 */ { kKeyAppUser2 },	// VK_LAUNCH_APP2
	/* 0x1b8 */ { kKeyNone },		// unassigned
	/* 0x1b9 */ { kKeyNone },		// unassigned
	/* 0x1ba */ { kKeyNone },		// OEM specific
	/* 0x1bb */ { kKeyNone },		// OEM specific
	/* 0x1bc */ { kKeyNone },		// OEM specific
	/* 0x1bd */ { kKeyNone },		// OEM specific
	/* 0x1be */ { kKeyNone },		// OEM specific
	/* 0x1bf */ { kKeyNone },		// OEM specific
	/* 0x1c0 */ { kKeyNone },		// OEM specific
	/* 0x1c1 */ { kKeyNone },		// unassigned
	/* 0x1c2 */ { kKeyNone },		// unassigned
	/* 0x1c3 */ { kKeyNone },		// unassigned
	/* 0x1c4 */ { kKeyNone },		// unassigned
	/* 0x1c5 */ { kKeyNone },		// unassigned
	/* 0x1c6 */ { kKeyNone },		// unassigned
	/* 0x1c7 */ { kKeyNone },		// unassigned
	/* 0x1c8 */ { kKeyNone },		// unassigned
	/* 0x1c9 */ { kKeyNone },		// unassigned
	/* 0x1ca */ { kKeyNone },		// unassigned
	/* 0x1cb */ { kKeyNone },		// unassigned
	/* 0x1cc */ { kKeyNone },		// unassigned
	/* 0x1cd */ { kKeyNone },		// unassigned
	/* 0x1ce */ { kKeyNone },		// unassigned
	/* 0x1cf */ { kKeyNone },		// unassigned
	/* 0x1d0 */ { kKeyNone },		// unassigned
	/* 0x1d1 */ { kKeyNone },		// unassigned
	/* 0x1d2 */ { kKeyNone },		// unassigned
	/* 0x1d3 */ { kKeyNone },		// unassigned
	/* 0x1d4 */ { kKeyNone },		// unassigned
	/* 0x1d5 */ { kKeyNone },		// unassigned
	/* 0x1d6 */ { kKeyNone },		// unassigned
	/* 0x1d7 */ { kKeyNone },		// unassigned
	/* 0x1d8 */ { kKeyNone },		// unassigned
	/* 0x1d9 */ { kKeyNone },		// unassigned
	/* 0x1da */ { kKeyNone },		// unassigned
	/* 0x1db */ { kKeyNone },		// OEM specific
	/* 0x1dc */ { kKeyNone },		// OEM specific
	/* 0x1dd */ { kKeyNone },		// OEM specific
	/* 0x1de */ { kKeyNone },		// OEM specific
	/* 0x1df */ { kKeyNone },		// OEM specific
	/* 0x1e0 */ { kKeyNone },		// OEM specific
	/* 0x1e1 */ { kKeyNone },		// OEM specific
	/* 0x1e2 */ { kKeyNone },		// OEM specific
	/* 0x1e3 */ { kKeyNone },		// OEM specific
	/* 0x1e4 */ { kKeyNone },		// OEM specific
	/* 0x1e5 */ { kKeyNone },		// unassigned
	/* 0x1e6 */ { kKeyNone },		// OEM specific
	/* 0x1e7 */ { kKeyNone },		// unassigned
	/* 0x1e8 */ { kKeyNone },		// unassigned
	/* 0x1e9 */ { kKeyNone },		// OEM specific
	/* 0x1ea */ { kKeyNone },		// OEM specific
	/* 0x1eb */ { kKeyNone },		// OEM specific
	/* 0x1ec */ { kKeyNone },		// OEM specific
	/* 0x1ed */ { kKeyNone },		// OEM specific
	/* 0x1ee */ { kKeyNone },		// OEM specific
	/* 0x1ef */ { kKeyNone },		// OEM specific
	/* 0x1f0 */ { kKeyNone },		// OEM specific
	/* 0x1f1 */ { kKeyNone },		// OEM specific
	/* 0x1f2 */ { kKeyNone },		// OEM specific
	/* 0x1f3 */ { kKeyNone },		// OEM specific
	/* 0x1f4 */ { kKeyNone },		// OEM specific
	/* 0x1f5 */ { kKeyNone },		// OEM specific
	/* 0x1f6 */ { kKeyNone },		// VK_ATTN			
	/* 0x1f7 */ { kKeyNone },		// VK_CRSEL			
	/* 0x1f8 */ { kKeyNone },		// VK_EXSEL			
	/* 0x1f9 */ { kKeyNone },		// VK_EREOF			
	/* 0x1fa */ { kKeyNone },		// VK_PLAY			
	/* 0x1fb */ { kKeyNone },		// VK_ZOOM			
	/* 0x1fc */ { kKeyNone },		// reserved
	/* 0x1fd */ { kKeyNone },		// VK_PA1			
	/* 0x1fe */ { kKeyNone },		// VK_OEM_CLEAR		
	/* 0x1ff */ { kKeyNone }		// reserved
};

struct CWin32Modifiers {
public:
	UINT				m_vk;
	KeyModifierMask		m_mask;
};

static const CWin32Modifiers s_modifiers[] =
{
	{ VK_SHIFT,    KeyModifierShift   },
	{ VK_LSHIFT,   KeyModifierShift   },
	{ VK_RSHIFT,   KeyModifierShift   },
	{ VK_CONTROL,  KeyModifierControl },
	{ VK_LCONTROL, KeyModifierControl },
	{ VK_RCONTROL, KeyModifierControl },
	{ VK_MENU,     KeyModifierAlt     },
	{ VK_LMENU,    KeyModifierAlt     },
	{ VK_RMENU,    KeyModifierAlt     },
	{ VK_LWIN,     KeyModifierSuper   },
	{ VK_RWIN,     KeyModifierSuper   }
};

CMSWindowsKeyState::CMSWindowsKeyState(CMSWindowsDesks* desks,
				void* eventTarget) :
	m_is95Family(CArchMiscWindows::isWindows95Family()),
	m_eventTarget(eventTarget),
	m_desks(desks),
	m_keyLayout(GetKeyboardLayout(0)),
	m_fixTimer(NULL),
	m_anyAltGr(true)
{
	// look up symbol that's available on winNT family but not win95
	HMODULE userModule = GetModuleHandle("user32.dll");
	m_ToUnicodeEx = (ToUnicodeEx_t)GetProcAddress(userModule, "ToUnicodeEx");
}

CMSWindowsKeyState::~CMSWindowsKeyState()
{
	disable();
}

void
CMSWindowsKeyState::disable()
{
	if (m_fixTimer != NULL) {
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_fixTimer);
		EVENTQUEUE->deleteTimer(m_fixTimer);
		m_fixTimer = NULL;
	}
}

KeyButton
CMSWindowsKeyState::virtualKeyToButton(UINT virtualKey) const
{
	return m_virtualKeyToButton[virtualKey & 0xffu];
}

void
CMSWindowsKeyState::setKeyLayout(HKL keyLayout)
{
	m_keyLayout = keyLayout;
}

KeyID
CMSWindowsKeyState::mapKeyFromEvent(WPARAM charAndVirtKey,
				LPARAM info, KeyModifierMask* maskOut) const
{
	static const KeyModifierMask s_altGr = KeyModifierControl | KeyModifierAlt;

	// extract character, virtual key, and if we didn't use AltGr
	char c       = (char)((charAndVirtKey & 0xff00u) >> 8);
	UINT vkCode  = (charAndVirtKey & 0xffu);
	bool noAltGr = ((charAndVirtKey & 0xff0000u) != 0);

	// handle some keys via table lookup
	KeyID id     = getKeyID(vkCode, (KeyButton)((info >> 16) & 0x1ffu));

	// check if not in table;  map character to key id
	if (id == kKeyNone && c != 0) {
		if ((c & 0x80u) == 0) {
			// ASCII
			id = static_cast<KeyID>(c) & 0xffu;
		}
		else {
			// character is not really ASCII.  instead it's some
			// character in the current ANSI code page.  try to
			// convert that to a Unicode character.  if we fail
			// then use the single byte character as is.
			char src = c;
			wchar_t unicode;
			if (MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED,
										&src, 1, &unicode, 1) > 0) {
				id = static_cast<KeyID>(unicode);
			}
			else {
				id = static_cast<KeyID>(c) & 0xffu;
			}
		}
	}

	// set modifier mask
	if (maskOut != NULL) {
		KeyModifierMask active = getActiveModifiers();
		if (m_anyAltGr && !noAltGr && (active & s_altGr) == s_altGr) {
			active |=  KeyModifierAltGr;
			active &= ~s_altGr;
		}
		else {
			active &= ~KeyModifierAltGr;
		}
		*maskOut = active;
	}

	return id;
}

bool
CMSWindowsKeyState::didGroupsChange() const
{
	GroupList groups;
	return (getGroups(groups) && groups != m_groups);
}

UINT
CMSWindowsKeyState::mapKeyToVirtualKey(KeyID key) const
{
	if (key == kKeyNone) {
		return 0;
	}
	KeyToVKMap::const_iterator i = m_keyToVKMap.find(key);
	if (i == m_keyToVKMap.end()) {
		return 0;
	}
	else {
		return i->second;
	}
}

void
CMSWindowsKeyState::onKey(KeyButton button, bool down, KeyModifierMask newState)
{
	// handle win32 brokenness and forward to superclass
	fixKeys();
	CKeyState::onKey(button, down, newState);
	fixKeys();
}

void
CMSWindowsKeyState::sendKeyEvent(void* target,
							bool press, bool isAutoRepeat,
							KeyID key, KeyModifierMask mask,
							SInt32 count, KeyButton button)
{
	if (press || isAutoRepeat) {
		// if AltGr is required for this key then make sure
		// the ctrl and alt keys are *not* down on the
		// client.  windows simulates AltGr with ctrl and
		// alt for some inexplicable reason and clients
		// will get confused if they see mode switch and
		// ctrl and alt.  we'll also need to put ctrl and
		// alt back the way they were after we simulate
		// the key.
		bool ctrlL = isKeyDown(virtualKeyToButton(VK_LCONTROL));
		bool ctrlR = isKeyDown(virtualKeyToButton(VK_RCONTROL));
		bool altL  = isKeyDown(virtualKeyToButton(VK_LMENU));
		if ((mask & KeyModifierAltGr) != 0) {
			KeyModifierMask mask2 = (mask &
								~(KeyModifierControl |
								KeyModifierAlt |
								KeyModifierAltGr));
			if (ctrlL) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyControl_L, mask2, 1,
							virtualKeyToButton(VK_LCONTROL));
			}
			if (ctrlR) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyControl_R, mask2, 1,
							virtualKeyToButton(VK_RCONTROL));
			}
			if (altL) {
				CKeyState::sendKeyEvent(target, false, false,
							kKeyAlt_L, mask2, 1,
							virtualKeyToButton(VK_LMENU));
			}
		}

		// send key
		if (press) {
			CKeyState::sendKeyEvent(target, true, false,
							key, mask, 1, button);
			if (count > 0) {
				--count;
			}
		}
		if (count >= 1) {
			CKeyState::sendKeyEvent(target, true, true,
							key, mask, count, button);
		}

		// restore ctrl and alt state
		if ((mask & KeyModifierAltGr) != 0) {
			KeyModifierMask mask2 = (mask &
								~(KeyModifierControl |
								KeyModifierAlt |
								KeyModifierAltGr));
			if (ctrlL) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyControl_L, mask2, 1,
							virtualKeyToButton(VK_LCONTROL));
				mask2 |= KeyModifierControl;
			}
			if (ctrlR) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyControl_R, mask2, 1,
							virtualKeyToButton(VK_RCONTROL));
				mask2 |= KeyModifierControl;
			}
			if (altL) {
				CKeyState::sendKeyEvent(target, true, false,
							kKeyAlt_L, mask2, 1,
							virtualKeyToButton(VK_LMENU));
				mask2 |= KeyModifierAlt;
			}
		}
	}
	else {
		// do key up
		CKeyState::sendKeyEvent(target, false, false, key, mask, 1, button);
	}
}

void
CMSWindowsKeyState::fakeKeyDown(KeyID id, KeyModifierMask mask,
				KeyButton button)
{
	// reserve right alt for AltGr
	if (m_anyAltGr && id == kKeyAlt_R) {
		id = kKeyAlt_L;
	}

	// if we need AltGr then press the left control
	bool altGr = ((mask & KeyModifierAltGr) != 0) &&
					!(id >= kKeyShift_L && id <= kKeyHyper_R);
	if (altGr) {
		m_desks->fakeKeyEvent(m_virtualKeyToButton[VK_LCONTROL],
								VK_LCONTROL, true, false);
	}

	CKeyState::fakeKeyDown(id, mask, button);

	// if we pressed left control for AltGr then release it
	if (altGr) {
		m_desks->fakeKeyEvent(m_virtualKeyToButton[VK_LCONTROL],
								VK_LCONTROL, false, false);
	}
}

void
CMSWindowsKeyState::fakeKeyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
	// reserve right alt for AltGr
	if (m_anyAltGr && id == kKeyAlt_R) {
		id = kKeyAlt_L;
	}
	CKeyState::fakeKeyRepeat(id, mask, count, button);
}

bool
CMSWindowsKeyState::fakeCtrlAltDel()
{
	if (!m_is95Family) {
		// to fake ctrl+alt+del on the NT family we broadcast a suitable
		// hotkey to all windows on the winlogon desktop.  however, the
		// current thread must be on that desktop to do the broadcast
		// and we can't switch just any thread because some own windows
		// or hooks.  so start a new thread to do the real work.
		CThread cad(new CFunctionJob(&CMSWindowsKeyState::ctrlAltDelThread));
		cad.wait();
	}
	else {
		// simulate ctrl+alt+del
		fakeKeyDown(kKeyDelete, KeyModifierControl | KeyModifierAlt,
							virtualKeyToButton(VK_DELETE));
	}
	return true;
}

void
CMSWindowsKeyState::ctrlAltDelThread(void*)
{
	// get the Winlogon desktop at whatever privilege we can
	HDESK desk = OpenDesktop("Winlogon", 0, FALSE, MAXIMUM_ALLOWED);
	if (desk != NULL) {
		if (SetThreadDesktop(desk)) {
			PostMessage(HWND_BROADCAST, WM_HOTKEY, 0,
						MAKELPARAM(MOD_CONTROL | MOD_ALT, VK_DELETE));
		}
		else {
			LOG((CLOG_DEBUG "can't switch to Winlogon desk: %d", GetLastError()));
		}
		CloseDesktop(desk);
	}
	else {
		LOG((CLOG_DEBUG "can't open Winlogon desk: %d", GetLastError()));
	}
}

KeyModifierMask
CMSWindowsKeyState::pollActiveModifiers() const
{
	KeyModifierMask state = 0;

	// get non-toggle modifiers from our own shadow key state
	for (size_t i = 0; i < sizeof(s_modifiers) / sizeof(s_modifiers[0]); ++i) {
		KeyButton button = virtualKeyToButton(s_modifiers[i].m_vk);
		if (button != 0 && isKeyDown(button)) {
			state |= s_modifiers[i].m_mask;
		}
	}

	// we can get toggle modifiers from the system
	if ((GetKeyState(VK_CAPITAL) & 0x01) != 0) {
		state |= KeyModifierCapsLock;
	}
	if ((GetKeyState(VK_NUMLOCK) & 0x01) != 0) {
		state |= KeyModifierNumLock;
	}
	if ((GetKeyState(VK_SCROLL) & 0x01) != 0) {
		state |= KeyModifierScrollLock;
	}

	return state;
}

SInt32
CMSWindowsKeyState::pollActiveGroup() const
{
	// determine the thread that'll receive this event
	HWND  targetWindow = GetForegroundWindow();
	DWORD targetThread = GetWindowThreadProcessId(targetWindow, NULL);

	// get keyboard layout for the thread
	HKL hkl            = GetKeyboardLayout(targetThread);

	// get group
	GroupMap::const_iterator i = m_groupMap.find(hkl);
	if (i == m_groupMap.end()) {
		LOG((CLOG_DEBUG1 "can't find keyboard layout %08x", hkl));
		return 0;
	}

	return i->second;
}

void
CMSWindowsKeyState::pollPressedKeys(KeyButtonSet& pressedKeys) const
{
	BYTE keyState[256];
	GetKeyboardState(keyState);
	for (KeyButton i = 1; i < 256; ++i) {
		if ((keyState[i] & 0x80) != 0) {
			pressedKeys.insert(i);
		}
	}
}

void
CMSWindowsKeyState::getKeyMap(CKeyMap& keyMap)
{
	// update keyboard groups
	if (getGroups(m_groups)) {
		m_groupMap.clear();
		SInt32 numGroups = (SInt32)m_groups.size();
		for (SInt32 g = 0; g < numGroups; ++g) {
			m_groupMap[m_groups[g]] = g;
		}
	}
	HKL activeLayout = GetKeyboardLayout(0);

	// clear table
	memset(m_virtualKeyToButton, 0, sizeof(m_virtualKeyToButton));
	m_keyToVKMap.clear();

	// decide if we need the AltGr key or not
	// XXX -- should check the keyboard layouts to decide this
	// XXX -- the synergy hook DLL may get ctrl+alt key events for the
	// right alt key;  it or this object should suppress the ctrl events
	// that are as a result of the right alt key.
	m_anyAltGr = true;

	CKeyMap::KeyItem item;
	SInt32 numGroups = (SInt32)m_groups.size();
	for (SInt32 g = 0; g < numGroups; ++g) {
		item.m_group = g;
		ActivateKeyboardLayout(m_groups[g], 0);

		// clear tables
		memset(m_buttonToVK, 0, sizeof(m_buttonToVK));
		memset(m_buttonToNumpadVK, 0, sizeof(m_buttonToNumpadVK));

		// map buttons (scancodes) to virtual keys
		for (KeyButton i = 1; i < 256; ++i) {
			UINT vk = MapVirtualKey(i, 1);
			if (vk == 0) {
				// unmapped
				continue;
			}

			// deal with certain virtual keys specially
			switch (vk) {
			case VK_SHIFT:
				if (MapVirtualKey(VK_RSHIFT, 0) == i) {
					vk = VK_RSHIFT;
				}
				else {
					vk = VK_LSHIFT;
				}
				break;

			case VK_CONTROL:
				vk = VK_LCONTROL;
				break;

			case VK_MENU:
				vk = VK_LMENU;
				break;

			case VK_NUMLOCK:
				vk = VK_PAUSE;
				break;

			case VK_NUMPAD0:
			case VK_NUMPAD1:
			case VK_NUMPAD2:
			case VK_NUMPAD3:
			case VK_NUMPAD4:
			case VK_NUMPAD5:
			case VK_NUMPAD6:
			case VK_NUMPAD7:
			case VK_NUMPAD8:
			case VK_NUMPAD9:
			case VK_DECIMAL:
				// numpad keys are saved in their own table
				m_buttonToNumpadVK[i] = vk;
				continue;

			case VK_LWIN:
			case VK_RWIN:
				// add extended key only for these on 95 family
				if (m_is95Family) {
					m_buttonToVK[i | 0x100u] = vk;
					continue;
				}
				break;

			case VK_RETURN:
			case VK_PRIOR:
			case VK_NEXT:
			case VK_END:
			case VK_HOME:
			case VK_LEFT:
			case VK_UP:
			case VK_RIGHT:
			case VK_DOWN:
			case VK_INSERT:
			case VK_DELETE:
				// also add extended key for these
				m_buttonToVK[i | 0x100u] = vk;
				break;
			}

			if (m_buttonToVK[i] == 0) {
				m_buttonToVK[i] = vk;
			}
		}

		// now map virtual keys to buttons.  multiple virtual keys may map
		// to a single button.  if the virtual key matches the one in
		// m_buttonToVK then we use the button as is.  if not then it's
		// either a numpad key and we use the button as is or it's an
		// extended button.
		for (UINT i = 1; i < 255; ++i) {
			// skip virtual keys we don't want
			switch (i) {
			case VK_LBUTTON:
			case VK_RBUTTON:
			case VK_MBUTTON:
			case VK_XBUTTON1:
			case VK_XBUTTON2:
			case VK_SHIFT:
			case VK_CONTROL:
			case VK_MENU:
				continue;
			}

			// get the button
			KeyButton button = static_cast<KeyButton>(MapVirtualKey(i, 0));
			if (button == 0) {
				// unmapped
				if (!m_is95Family) {
					continue;
				}

				// the 95 family doesn't map handed virtual keys to
				// buttons but we need at least VK_RMENU to synthesize
				// AltGr.
				if (i == VK_RMENU) {
					button = static_cast<KeyButton>(MapVirtualKey(VK_MENU, 0));
				}
				if (button == 0) {
					continue;
				}
			}

			// deal with certain virtual keys specially
			switch (i) {
			case VK_NUMPAD0:
			case VK_NUMPAD1:
			case VK_NUMPAD2:
			case VK_NUMPAD3:
			case VK_NUMPAD4:
			case VK_NUMPAD5:
			case VK_NUMPAD6:
			case VK_NUMPAD7:
			case VK_NUMPAD8:
			case VK_NUMPAD9:
			case VK_DECIMAL:
				m_buttonToNumpadVK[button] = i;
				break;

			default:
				// add extended key if virtual keys don't match
				if (m_buttonToVK[button] != i) {
					m_buttonToVK[button | 0x100u] = i;
				}
				break;
			}
		}

		// add alt+printscreen
		if (m_buttonToVK[0x54u] == 0) {
			m_buttonToVK[0x54u] = VK_SNAPSHOT;
		}

		// set virtual key to button table
		if (GetKeyboardLayout(0) == m_groups[g]) {
			for (KeyButton i = 0; i < 512; ++i) {
				if (m_buttonToVK[i] != 0) {
					if (m_virtualKeyToButton[m_buttonToVK[i]] == 0) {
						m_virtualKeyToButton[m_buttonToVK[i]] = i;
					}
				}
				if (m_buttonToNumpadVK[i] != 0) {
					if (m_virtualKeyToButton[m_buttonToNumpadVK[i]] == 0) {
						m_virtualKeyToButton[m_buttonToNumpadVK[i]] = i;
					}
				}
			}
		}

		// add numpad keys
		for (KeyButton i = 0; i < 512; ++i) {
			if (m_buttonToNumpadVK[i] != 0) {
				item.m_id        = getKeyID(m_buttonToNumpadVK[i], i);
				item.m_button    = i;
				item.m_required  = KeyModifierNumLock;
				item.m_sensitive = KeyModifierNumLock | KeyModifierShift;
				item.m_generates = 0;
				item.m_client    = m_buttonToNumpadVK[i];
				addKeyEntry(keyMap, item);
			}
		}

		// add other keys
		BYTE keys[256];
		memset(keys, 0, sizeof(keys));
		for (KeyButton i = 0; i < 512; ++i) {
			if (m_buttonToVK[i] != 0) {
				// initialize item
				item.m_id        = getKeyID(m_buttonToVK[i], i);
				item.m_button    = i;
				item.m_required  = 0;
				item.m_sensitive = 0;
				item.m_client    = m_buttonToVK[i];

				// get flags for modifier keys
				CKeyMap::initModifierKey(item);

				if (item.m_id == 0) {
					// translate virtual key to a character with and without
					// shift, caps lock, and AltGr.
					struct Modifier {
						UINT			m_vk1;
						UINT			m_vk2;
						BYTE			m_state;
						KeyModifierMask	m_mask;
					};
					static const Modifier modifiers[] = {
						{ VK_SHIFT,   VK_SHIFT,   0x80u, KeyModifierShift    },
						{ VK_CAPITAL, VK_CAPITAL, 0x01u, KeyModifierCapsLock },
						{ VK_CONTROL, VK_MENU,    0x80u, KeyModifierAltGr    }
					};
					static const size_t s_numModifiers =
						sizeof(modifiers) / sizeof(modifiers[0]);
					static const size_t s_numCombinations = 1 << s_numModifiers;
					KeyID id[s_numCombinations];

					bool anyFound = false;
					KeyButton button = static_cast<KeyButton>(i & 0xffu);
					for (size_t j = 0; j < s_numCombinations; ++j) {
						for (size_t k = 0; k < s_numModifiers; ++k) {
							if ((j & (1 << k)) != 0) {
								keys[modifiers[k].m_vk1] = modifiers[k].m_state;
								keys[modifiers[k].m_vk2] = modifiers[k].m_state;
							}
							else {
								keys[modifiers[k].m_vk1] = 0;
								keys[modifiers[k].m_vk2] = 0;
							}
						}
						id[j] = getIDForKey(item, button,
										m_buttonToVK[i], keys, m_groups[g]);
						if (id[j] != 0) {
							anyFound = true;
						}
					}

					if (anyFound) {
						// determine what modifiers we're sensitive to.
						// we're sensitive if the KeyID changes when the
						// modifier does.
						item.m_sensitive = 0;
						for (size_t k = 0; k < s_numModifiers; ++k) {
							for (size_t j = 0; j < s_numCombinations; ++j) {
								if (id[j] != id[j ^ (1u << k)]) {
									item.m_sensitive |= modifiers[k].m_mask;
									break;
								}
							}
						}
						
						// save each key.  the map will automatically discard
						// duplicates, like an unshift and shifted version of
						// a key that's insensitive to shift.
						for (size_t j = 0; j < s_numCombinations; ++j) {
							item.m_id       = id[j];
							item.m_required = 0;
							for (size_t k = 0; k < s_numModifiers; ++k) {
								if ((j & (1 << k)) != 0) {
									item.m_required |= modifiers[k].m_mask;
								}
							}
							addKeyEntry(keyMap, item);
						}
					}
				}
				else {
					// found in table
					switch (m_buttonToVK[i]) {
					case VK_TAB:
						// add kKeyLeftTab, too
						item.m_id         = kKeyLeftTab;
						item.m_required  |= KeyModifierShift;
						item.m_sensitive |= KeyModifierShift;
						addKeyEntry(keyMap, item);
						item.m_id         = kKeyTab;
						item.m_required  &= ~KeyModifierShift;
						break;

					case VK_CANCEL:
						item.m_required  |= KeyModifierControl;
						item.m_sensitive |= KeyModifierControl;
						break;

					case VK_SNAPSHOT:
						item.m_sensitive |= KeyModifierAlt;
						if ((i & 0x100u) == 0) {
							// non-extended snapshot key requires alt
							item.m_required |= KeyModifierAlt;
						}
						break;
					}
					addKeyEntry(keyMap, item);
				}
			}
		}
	}

	// restore keyboard layout
	ActivateKeyboardLayout(activeLayout, 0);
}

void
CMSWindowsKeyState::fakeKey(const Keystroke& keystroke)
{
	switch (keystroke.m_type) {
	case Keystroke::kButton: {
		LOG((CLOG_DEBUG1 "  %03x (%08x) %s", keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_client, keystroke.m_data.m_button.m_press ? "down" : "up"));
		KeyButton button = keystroke.m_data.m_button.m_button;

		// get the virtual key for the button
		UINT vk = keystroke.m_data.m_button.m_client;

		// special handling of VK_SNAPSHOT
		if (vk == VK_SNAPSHOT) {
			if ((getActiveModifiers() & KeyModifierAlt) != 0) {
				// snapshot active window
				button = 1;
			}
			else {
				// snapshot full screen
				button = 0;
			}
		}

		// synthesize event
		m_desks->fakeKeyEvent(button, vk,
								keystroke.m_data.m_button.m_press,
								keystroke.m_data.m_button.m_repeat);
		break;
	}

	case Keystroke::kGroup:
		// we don't restore the group.  we'd like to but we can't be
		// sure the restoring group change will be processed after the
		// key events.
		if (!keystroke.m_data.m_group.m_restore) {
			if (keystroke.m_data.m_group.m_absolute) {
				LOG((CLOG_DEBUG1 "  group %d", keystroke.m_data.m_group.m_group));
				setWindowGroup(keystroke.m_data.m_group.m_group);
			}
			else {
				LOG((CLOG_DEBUG1 "  group %+d", keystroke.m_data.m_group.m_group));
				setWindowGroup(getEffectiveGroup(pollActiveGroup(),
									keystroke.m_data.m_group.m_group));
			}
		}
		break;
	}
}

bool
CMSWindowsKeyState::getGroups(GroupList& groups) const
{
	// get keyboard layouts
	UInt32 newNumLayouts = GetKeyboardLayoutList(0, NULL);
	if (newNumLayouts == 0) {
		LOG((CLOG_DEBUG1 "can't get keyboard layouts"));
		return false;
	}
	HKL* newLayouts = new HKL[newNumLayouts];
	newNumLayouts = GetKeyboardLayoutList(newNumLayouts, newLayouts);
	if (newNumLayouts == 0) {
		LOG((CLOG_DEBUG1 "can't get keyboard layouts"));
		delete[] newLayouts;
		return false;
	}

	groups.clear();
	groups.insert(groups.end(), newLayouts, newLayouts + newNumLayouts);
	delete[] newLayouts;
	return true;
}

void
CMSWindowsKeyState::setWindowGroup(SInt32 group)
{
	HWND targetWindow = GetForegroundWindow();

	bool sysCharSet = true;
	// XXX -- determine if m_groups[group] can be used with the system
	// character set.

	PostMessage(targetWindow, WM_INPUTLANGCHANGEREQUEST,
								sysCharSet ? 1 : 0, (LPARAM)m_groups[group]);

	// XXX -- use a short delay to let the target window process the message
	// before it sees the keyboard events.  i'm not sure why this is
	// necessary since the messages should arrive in order.  if we don't
	// delay, though, some of our keyboard events may disappear.
	Sleep(100);
}

void
CMSWindowsKeyState::fixKeys()
{
	// fake key releases for the windows keys if we think they're
	// down but they're really up.  we have to do this because if the
	// user presses and releases a windows key without pressing any
	// other key while it's down then the system will eat the key
	// release.  if we don't detect that and synthesize the release
	// then the client won't take the usual windows key release action
	// (which on windows is to show the start menu).
	//
	// only check on the windows 95 family since the NT family reports
	// the key releases as usual.
	if (!m_is95Family) {
		return;
	}

	KeyButton leftButton  = virtualKeyToButton(VK_LWIN);
	KeyButton rightButton = virtualKeyToButton(VK_RWIN);
	bool leftDown         = isKeyDown(leftButton);
	bool rightDown        = isKeyDown(rightButton);
	bool fix              = (leftDown || rightDown);
	if (fix) {
		// check if either button is not really down
		bool leftAsyncDown  = ((GetAsyncKeyState(VK_LWIN) & 0x8000) != 0);
		bool rightAsyncDown = ((GetAsyncKeyState(VK_RWIN) & 0x8000) != 0);

		if (leftAsyncDown != leftDown || rightAsyncDown != rightDown) {
			KeyModifierMask state = getActiveModifiers();
			if (!leftAsyncDown && !rightAsyncDown) {
				// no win keys are down so remove super modifier
				state &= ~KeyModifierSuper;
			}

			// report up events
			if (leftDown  && !leftAsyncDown) {
				LOG((CLOG_DEBUG1 "event: fake key release left windows key (0x%03x)", leftButton));
				CKeyState::onKey(leftButton, false, state);
				CKeyState::sendKeyEvent(m_eventTarget, false, false,
								kKeySuper_L, state, 1, leftButton);
			}
			if (rightDown && !rightAsyncDown) {
				LOG((CLOG_DEBUG1 "event: fake key release right windows key (0x%03x)", rightButton));
				CKeyState::onKey(rightButton, false, state);
				CKeyState::sendKeyEvent(m_eventTarget, false, false,
								kKeySuper_R, state, 1, rightButton);
			}
		}
	}

	if (fix && m_fixTimer == NULL) {
		// schedule check
		m_fixTimer = EVENTQUEUE->newTimer(0.1, NULL);
		EVENTQUEUE->adoptHandler(CEvent::kTimer, m_fixTimer,
							new TMethodEventJob<CMSWindowsKeyState>(
								this, &CMSWindowsKeyState::handleFixKeys));
	}
	else if (!fix && m_fixTimer != NULL) {
		// remove scheduled check
		EVENTQUEUE->removeHandler(CEvent::kTimer, m_fixTimer);
		EVENTQUEUE->deleteTimer(m_fixTimer);
		m_fixTimer = NULL;
	}
}

void
CMSWindowsKeyState::handleFixKeys(const CEvent&, void*)
{
	fixKeys();
}

KeyID
CMSWindowsKeyState::getKeyID(UINT virtualKey, KeyButton button) const
{
	if (!m_anyAltGr && virtualKey == VK_RMENU) {
		return kKeyAlt_R;
	}
	if ((button & 0x100u) != 0) {
		virtualKey += 0x100u;
	}
	return s_virtualKey[virtualKey];
}

KeyID
CMSWindowsKeyState::getIDForKey(CKeyMap::KeyItem& item,
				KeyButton button, UINT virtualKey,
				PBYTE keyState, HKL hkl) const
{
	int n;
	KeyID id;
	if (m_is95Family) {
		// XXX -- how do we get characters not in Latin-1?
		WORD ascii;
		n  = ToAsciiEx(virtualKey, button, keyState, &ascii, 0, hkl);
		id = static_cast<KeyID>(ascii & 0xffu);
	}
	else {
		WCHAR unicode[2];
		n  = m_ToUnicodeEx(virtualKey, button, keyState,
								unicode, sizeof(unicode) / sizeof(unicode[0]),
								0, hkl);
		id = static_cast<KeyID>(unicode[0]);
	}
	switch (n) {
	case -1:
		return CKeyMap::getDeadKey(id);

	default:
	case 0:
		// unmapped
		return kKeyNone;

	case 1:
		return id;

	case 2:
		// left over dead key in buffer;  oops.
		return getIDForKey(item, button, virtualKey, keyState, hkl);
	}
}

void
CMSWindowsKeyState::addKeyEntry(CKeyMap& keyMap, CKeyMap::KeyItem& item)
{
	keyMap.addKeyEntry(item);
	if (item.m_group == 0) {
		m_keyToVKMap[item.m_id] = static_cast<UINT>(item.m_client);
	}
}