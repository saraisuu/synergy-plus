/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#include "COSXKeyState.h"
#include "CLog.h"
#include "CArch.h"

// Hardcoded virtual key table.  Oddly, Apple doesn't document the
// meaning of virtual key codes.  The whole point of *virtual* key
// codes is to make them hardware independent so these codes should
// be constant across OS versions and hardware.  Yet they don't
// tell us what codes map to what keys so we have to figure it out
// for ourselves.
//
// Note that some virtual keys codes appear more than once.  The
// first instance of a virtual key code maps to the KeyID that we
// want to generate for that code.  The others are for mapping
// different KeyIDs to a single key code.
static const UInt32 s_shiftVK    = 56;
static const UInt32 s_controlVK  = 59;
static const UInt32 s_altVK      = 55;
static const UInt32 s_superVK    = 58;
static const UInt32 s_capsLockVK = 57;
struct CKeyEntry {
public:
	KeyID				m_keyID;
	UInt32				m_virtualKey;
};
static const CKeyEntry	s_controlKeys[] = {
	// cursor keys.  if we don't do this we'll may still get these from
	// the keyboard resource but they may not correspond to the arrow
	// keys.
	{ kKeyLeft,			123 },
	{ kKeyRight,		124 },
	{ kKeyUp,			126 },
	{ kKeyDown,			125 },
	{ kKeyHome,			115 },
	{ kKeyEnd,			119 },
	{ kKeyPageUp,		116 },
	{ kKeyPageDown,		121 },

	// function keys
	{ kKeyF1,			122 },
	{ kKeyF2,			120 },
	{ kKeyF3,			99 },
	{ kKeyF4,			118 },
	{ kKeyF5,			96 },
	{ kKeyF6,			97 },
	{ kKeyF7,			98 },
	{ kKeyF8,			100 },
	{ kKeyF9,			101 },
	{ kKeyF10,			109 },
	{ kKeyF11,			103 },
	{ kKeyF12,			111 },
	{ kKeyF13,			105 },
	{ kKeyF14,			107 },
	{ kKeyF15,			113 },
	{ kKeyF16,			106 },

	// virtual key 110 is fn+enter and i have no idea what that's supposed
	// to map to.  also the enter key with numlock on is a modifier but i
	// don't know which.

	// modifier keys.  OS X doesn't seem to support right handed versions
	// of modifier keys so we map them to the left handed versions.
	{ kKeyShift_L,		s_shiftVK },
	{ kKeyShift_R,		s_shiftVK }, // 60
	{ kKeyControl_L,	s_controlVK },
	{ kKeyControl_R,	s_controlVK }, // 62
	{ kKeyAlt_L,		s_altVK },
	{ kKeyAlt_R,		s_altVK },
	{ kKeySuper_L,		s_superVK },
	{ kKeySuper_R,		s_superVK }, // 61
	{ kKeyMeta_L,		s_superVK },
	{ kKeyMeta_R,		s_superVK }, // 61

	// toggle modifiers
//	{ kKeyNumLock,		71 },
	{ kKeyCapsLock,		s_capsLockVK }
};


//
// COSXKeyState
//

COSXKeyState::COSXKeyState() :
	m_deadKeyState(0)
{
	// build virtual key map
	for (size_t i = 0; i < sizeof(s_controlKeys) /
								sizeof(s_controlKeys[0]); ++i) {
		m_virtualKeyMap[s_controlKeys[i].m_virtualKey] =
			s_controlKeys[i].m_keyID;
	}
}

COSXKeyState::~COSXKeyState()
{
	// do nothing
}

KeyModifierMask
COSXKeyState::mapModifiersFromOSX(UInt32 mask) const
{
	// convert
	KeyModifierMask outMask = 0;
	if ((mask & shiftKey) != 0) {
		outMask |= KeyModifierShift;
	}
	if ((mask & rightShiftKey) != 0) {
		outMask |= KeyModifierShift;
	}
	if ((mask & controlKey) != 0) {
		outMask |= KeyModifierControl;
	}
	if ((mask & rightControlKey) != 0) {
		outMask |= KeyModifierControl;
	}
	if ((mask & cmdKey) != 0) {
		outMask |= KeyModifierAlt;
	}
	if ((mask & optionKey) != 0) {
		outMask |= KeyModifierSuper;
	}
	if ((mask & rightOptionKey) != 0) {
		outMask |= KeyModifierSuper;
	}
	if ((mask & alphaLock) != 0) {
		outMask |= KeyModifierCapsLock;
	}

	return outMask;
}

KeyButton 
COSXKeyState::mapKeyFromEvent(CKeyIDs& ids,
				KeyModifierMask* maskOut, EventRef event) const
{
	ids.clear();

	// map modifier key
	if (maskOut != NULL) {
		KeyModifierMask activeMask = getActiveModifiers();
		activeMask &= ~KeyModifierAltGr;
		*maskOut    = activeMask;
	}

	// get virtual key
	UInt32 vkCode;
	GetEventParameter(event, kEventParamKeyCode, typeUInt32,
							NULL, sizeof(vkCode), NULL, &vkCode);

	// handle up events
	UInt32 eventKind = GetEventKind(event);
	if (eventKind == kEventRawKeyUp) {
		// the id isn't used.  we just need the same button we used on
		// the key press.  note that we don't use or reset the dead key
		// state;  up events should not affect the dead key state.
		ids.push_back(kKeyNone);
		return mapVirtualKeyToKeyButton(vkCode);
	}

	// check for special keys
	CVirtualKeyMap::const_iterator i = m_virtualKeyMap.find(vkCode);
	if (i != m_virtualKeyMap.end()) {
		m_deadKeyState = 0;
		ids.push_back(i->second);
		return mapVirtualKeyToKeyButton(vkCode);
	}

	// get keyboard info
	KeyboardLayoutRef keyboardLayout;
	OSStatus status = KLGetCurrentKeyboardLayout(&keyboardLayout);
	if (status != noErr) {
		return kKeyNone;
	}

	// get the event modifiers and remove the command and control
	// keys.  note if we used them though.
	UInt32 modifiers;
	GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
								NULL, sizeof(modifiers), NULL, &modifiers);
	static const UInt32 s_commandModifiers =
		cmdKey | controlKey | rightControlKey;
	bool isCommand = ((modifiers & s_commandModifiers) != 0);
	modifiers &= ~s_commandModifiers;

	// if we've used a command key then we want the glyph produced without
	// the option key (i.e. the base glyph).
	if (isCommand) {
		modifiers &= ~optionKey;
	}

	// translate via uchr resource
	const void* resource;
	if (KLGetKeyboardLayoutProperty(keyboardLayout,
								kKLuchrData, &resource) == noErr) {
		// choose action
		UInt16 action;
		switch (eventKind) {
		case kEventRawKeyDown:
			action = kUCKeyActionDown;
			break;

		case kEventRawKeyRepeat:
			action = kUCKeyActionAutoKey;
			break;

		default:
			return 0;
		}

		// translate key
		UniCharCount count;
		UniChar chars[2];
		OSStatus status = UCKeyTranslate((const UCKeyboardLayout*)resource,
							vkCode & 0xffu, action,
							(modifiers >> 8) & 0xffu,
							LMGetKbdType(), 0, &m_deadKeyState,
							sizeof(chars) / sizeof(chars[0]), &count, chars);

		// get the characters
		if (status == 0) {
			if (count != 0 || m_deadKeyState == 0) {
				m_deadKeyState = 0;
				for (UniCharCount i = 0; i < count; ++i) {
					ids.push_back(CKeyResource::unicharToKeyID(chars[i]));
				}
				adjustAltGrModifier(ids, maskOut, isCommand);
				return mapVirtualKeyToKeyButton(vkCode);
			}
			return 0;
		}
	}

	// translate via KCHR resource
	if (KLGetKeyboardLayoutProperty(keyboardLayout,
								kKLKCHRData, &resource) == noErr) {
		// build keycode
		UInt16 keycode =
			static_cast<UInt16>((modifiers & 0xff00u) | (vkCode & 0x00ffu));

		// translate key
		UInt32 result = KeyTranslate(resource, keycode, &m_deadKeyState);

		// get the characters
		UInt8 c1 = static_cast<UInt8>((result >> 16) & 0xffu);
		UInt8 c2 = static_cast<UInt8>( result        & 0xffu);
		if (c2 != 0) {
			m_deadKeyState = 0;
			if (c1 != 0) {
				ids.push_back(CKeyResource::getKeyID(c1));
			}
			ids.push_back(CKeyResource::getKeyID(c2));
			adjustAltGrModifier(ids, maskOut, isCommand);
			return mapVirtualKeyToKeyButton(vkCode);
		}
	}

	return 0;
}

bool
COSXKeyState::fakeCtrlAltDel()
{
	// pass keys through unchanged
	return false;
}

KeyModifierMask
COSXKeyState::pollActiveModifiers() const
{
	return mapModifiersFromOSX(GetCurrentKeyModifiers());
}

SInt32
COSXKeyState::pollActiveGroup() const
{
	KeyboardLayoutRef keyboardLayout;
	OSStatus status = KLGetCurrentKeyboardLayout(&keyboardLayout);
	if (status == noErr) {
		GroupMap::const_iterator i = m_groupMap.find(keyboardLayout);
		if (i != m_groupMap.end()) {
			return i->second;
		}
	}
	return 0;
}

void
COSXKeyState::pollPressedKeys(KeyButtonSet& pressedKeys) const
{
	KeyMap km;
	GetKeys(km);
	const UInt8* m = reinterpret_cast<const UInt8*>(km);
	for (UInt32 i = 0; i < 16; ++i) {
		for (UInt32 j = 0; j < 8; ++j) {
			if ((m[i] & (1u << j)) != 0) {
				pressedKeys.insert(mapVirtualKeyToKeyButton(8 * i + j));
			}
		}
	}
}

void
COSXKeyState::getKeyMap(CKeyMap& keyMap)
{
	// update keyboard groups
	if (getGroups(m_groups)) {
		m_groupMap.clear();
		SInt32 numGroups = (SInt32)m_groups.size();
		for (SInt32 g = 0; g < numGroups; ++g) {
			m_groupMap[m_groups[g]] = g;
		}
	}

	UInt32 keyboardType = LMGetKbdType();
	for (SInt32 g = 0, n = (SInt32)m_groups.size(); g < n; ++g) {
		// add special keys
		getKeyMapForSpecialKeys(keyMap, g);

		// add regular keys

		// try uchr resource first
		const void* resource;
		if (KLGetKeyboardLayoutProperty(m_groups[g],
								kKLuchrData, &resource) == noErr) {
			CUCHRKeyResource uchr(resource, keyboardType);
			if (uchr.isValid()) {
				LOG((CLOG_DEBUG1 "using uchr resource for group %d", g));
				getKeyMap(keyMap, g, uchr);
				continue;
			}
		}

		// try KCHR resource
		if (KLGetKeyboardLayoutProperty(m_groups[g],
								kKLKCHRData, &resource) == noErr) {
			CKCHRKeyResource kchr(resource);
			if (kchr.isValid()) {
				LOG((CLOG_DEBUG1 "using KCHR resource for group %d", g));
				getKeyMap(keyMap, g, kchr);
				continue;
			}
		}

		LOG((CLOG_DEBUG1 "no keyboard resource for group %d", g));
	}
}

void
COSXKeyState::fakeKey(const Keystroke& keystroke)
{
	switch (keystroke.m_type) {
	case Keystroke::kButton:
		LOG((CLOG_DEBUG1 "  %03x (%08x) %s", keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_client, keystroke.m_data.m_button.m_press ? "down" : "up"));

		// let system figure out character for us
		CGPostKeyboardEvent(0, mapKeyButtonToVirtualKey(
									keystroke.m_data.m_button.m_button),
								keystroke.m_data.m_button.m_press);

		// add a delay if client data isn't zero
		if (keystroke.m_data.m_button.m_client) {
			ARCH->sleep(0.01);
		}
		break;

	case Keystroke::kGroup:
		if (keystroke.m_data.m_group.m_absolute) {
			LOG((CLOG_DEBUG1 "  group %d", keystroke.m_data.m_group.m_group));
			setGroup(keystroke.m_data.m_group.m_group);
		}
		else {
			LOG((CLOG_DEBUG1 "  group %+d", keystroke.m_data.m_group.m_group));
			setGroup(getEffectiveGroup(pollActiveGroup(),
									keystroke.m_data.m_group.m_group));
		}
		break;
	}
}

void
COSXKeyState::getKeyMapForSpecialKeys(CKeyMap& keyMap, SInt32 group) const
{
	// special keys are insensitive to modifers and none are dead keys
	CKeyMap::KeyItem item;
	for (size_t i = 0; i < sizeof(s_controlKeys) /
								sizeof(s_controlKeys[0]); ++i) {
		const CKeyEntry& entry = s_controlKeys[i];
		item.m_id        = entry.m_keyID;
		item.m_group     = group;
		item.m_button    = mapVirtualKeyToKeyButton(entry.m_virtualKey);
		item.m_required  = 0;
		item.m_sensitive = 0;
		item.m_dead      = false;
		item.m_client    = 0;
		CKeyMap::initModifierKey(item);
		keyMap.addKeyEntry(item);

		if (item.m_lock) {
			// all locking keys are half duplex on OS X
			keyMap.addHalfDuplexButton(item.m_button);
		}
	}

	// note:  we don't special case the number pad keys.  querying the
	// mac keyboard returns the non-keypad version of those keys but
	// a CKeyState always provides a mapping from keypad keys to
	// non-keypad keys so we'll be able to generate the characters
	// anyway.
}

bool
COSXKeyState::getKeyMap(CKeyMap& keyMap,
				SInt32 group, const CKeyResource& r) const
{
	if (!r.isValid()) {
		return false;
	}

	// space for all possible modifier combinations
	std::vector<bool> modifiers(r.getNumModifierCombinations());

	// make space for the keys that any single button can synthesize
	std::vector<std::pair<KeyID, bool> > buttonKeys(r.getNumTables());

	// iterate over each button
	CKeyMap::KeyItem item;
	for (UInt32 i = 0; i < r.getNumButtons(); ++i) {
		item.m_button = mapVirtualKeyToKeyButton(i);

		// the KeyIDs we've already handled
		std::set<KeyID> keys;

		// convert the entry in each table for this button to a KeyID
		for (UInt32 j = 0; j < r.getNumTables(); ++j) {
			buttonKeys[j].first  = r.getKey(j, i);
			buttonKeys[j].second = CKeyMap::isDeadKey(buttonKeys[j].first);
		}

		// iterate over each character table
		for (UInt32 j = 0; j < r.getNumTables(); ++j) {
			// get the KeyID for the button/table
			KeyID id = buttonKeys[j].first;
			if (id == kKeyNone) {
				continue;
			}

			// if we've already handled the KeyID in the table then
			// move on to the next table
			if (keys.count(id) > 0) {
				continue;
			}
			keys.insert(id);

			// prepare item.  the client state is 1 for dead keys.
			item.m_id     = id;
			item.m_group  = group;
			item.m_dead   = buttonKeys[j].second;
			item.m_client = buttonKeys[j].second ? 1 : 0;
			CKeyMap::initModifierKey(item);
			if (item.m_lock) {
				// all locking keys are half duplex on OS X
				keyMap.addHalfDuplexButton(i);
			}

			// collect the tables that map to the same KeyID.  we know it
			// can't be any earlier tables because of the check above.
			std::set<UInt8> tables;
			tables.insert(static_cast<UInt8>(j));
			for (UInt32 k = j + 1; k < r.getNumTables(); ++k) {
				if (buttonKeys[k].first == id) {
					tables.insert(static_cast<UInt8>(k));
				}
			}

			// collect the modifier combinations that map to any of the
			// tables we just collected
			for (UInt32 k = 0; k < r.getNumModifierCombinations(); ++k) {
				modifiers[k] = (tables.count(r.getTableForModifier(k)) > 0);
			}

			// figure out which modifiers the key is sensitive to.  the
			// key is insensitive to a modifier if for every modifier mask
			// with the modifier bit unset in the modifiers we also find
			// the same mask with the bit set.
			//
			// we ignore a few modifiers that we know aren't important
			// for generating characters.  in fact, we want to ignore any
			// characters generated by the control key.  we don't map
			// those and instead expect the control modifier plus a key.
			UInt32 sensitive = 0;
			for (UInt32 k = 0; (1u << k) <
								r.getNumModifierCombinations(); ++k) {
				UInt32 bit = (1u << k);
				if ((bit << 8) == cmdKey ||
					(bit << 8) == controlKey ||
					(bit << 8) == rightControlKey) {
					continue;
				}
				for (UInt32 m = 0; m < r.getNumModifierCombinations(); ++m) {
					if (modifiers[m] != modifiers[m ^ bit]) {
						sensitive |= bit;
						break;
					}
				}
			}

			// find each required modifier mask.  the key can be synthesized
			// using any of the masks.
			std::set<UInt32> required;
			for (UInt32 k = 0; k < r.getNumModifierCombinations(); ++k) {
				if ((k & sensitive) == k && modifiers[k & sensitive]) {
					required.insert(k);
				}
			}

			// now add a key entry for each key/required modifier pair.
			item.m_sensitive = mapModifiersFromOSX(sensitive << 8);
			for (std::set<UInt32>::iterator k = required.begin();
											k != required.end(); ++k) {
				item.m_required = mapModifiersFromOSX(*k << 8);
				keyMap.addKeyEntry(item);
			}
		}
	}

	return true;
}

bool
COSXKeyState::mapSynergyHotKeyToMac(KeyID key, KeyModifierMask mask,
				UInt32 &macVirtualKey, UInt32 &macModifierMask) const
{
	// look up button for key
	KeyButton button = getButton(key, pollActiveGroup());
	if (button == 0 && key != kKeyNone) {
		return false;
	}
	macVirtualKey = mapKeyButtonToVirtualKey(button);
	
	// calculate modifier mask
	macModifierMask = 0;
	if ((mask & KeyModifierShift) != 0) {
		macModifierMask |= shiftKey;
	}
	if ((mask & KeyModifierControl) != 0) {
		macModifierMask |= controlKey;
	}
	if ((mask & KeyModifierAlt) != 0) {
		macModifierMask |= cmdKey;
	}
	if ((mask & KeyModifierSuper) != 0) {
		macModifierMask |= optionKey;
	}
	if ((mask & KeyModifierCapsLock) != 0) {
		macModifierMask |= alphaLock;
	}
	
	return true;
}
						
void
COSXKeyState::handleModifierKeys(void* target,
				KeyModifierMask oldMask, KeyModifierMask newMask)
{
	// compute changed modifiers
	KeyModifierMask changed = (oldMask ^ newMask);

	// synthesize changed modifier keys
	if ((changed & KeyModifierShift) != 0) {
		handleModifierKey(target, s_shiftVK, kKeyShift_L,
							(newMask & KeyModifierShift) != 0, newMask);
	}
	if ((changed & KeyModifierControl) != 0) {
		handleModifierKey(target, s_controlVK, kKeyControl_L,
							(newMask & KeyModifierControl) != 0, newMask);
	}
	if ((changed & KeyModifierAlt) != 0) {
		handleModifierKey(target, s_altVK, kKeyAlt_L,
							(newMask & KeyModifierAlt) != 0, newMask);
	}
	if ((changed & KeyModifierSuper) != 0) {
		handleModifierKey(target, s_superVK, kKeySuper_L,
							(newMask & KeyModifierSuper) != 0, newMask);
	}
	if ((changed & KeyModifierCapsLock) != 0) {
		handleModifierKey(target, s_capsLockVK, kKeyCapsLock,
							(newMask & KeyModifierCapsLock) != 0, newMask);
	}
}

void
COSXKeyState::handleModifierKey(void* target,
				UInt32 virtualKey, KeyID id,
				bool down, KeyModifierMask newMask)
{
	KeyButton button = mapVirtualKeyToKeyButton(virtualKey);
	onKey(button, down, newMask);
	sendKeyEvent(target, down, false, id, newMask, 0, button);
}

bool
COSXKeyState::getGroups(GroupList& groups) const
{
	// get number of layouts
	CFIndex n;
	OSStatus status = KLGetKeyboardLayoutCount(&n);
	if (status != noErr) {
		LOG((CLOG_DEBUG1 "can't get keyboard layouts"));
		return false;
	}

	// get each layout
	groups.clear();
	for (CFIndex i = 0; i < n; ++i) {
		KeyboardLayoutRef keyboardLayout;
		status = KLGetKeyboardLayoutAtIndex(i, &keyboardLayout);
		if (status == noErr) {
			groups.push_back(keyboardLayout);
		}
	}
	return true;
}

void
COSXKeyState::setGroup(SInt32 group)
{
	KLSetCurrentKeyboardLayout(m_groups[group]);
}

void
COSXKeyState::checkKeyboardLayout()
{
	// XXX -- should call this when notified that groups have changed.
	// if no notification for that then we should poll.
	GroupList groups;
	if (getGroups(groups) && groups != m_groups) {
		updateKeyMap();
		updateKeyState();
	}
}

void
COSXKeyState::adjustAltGrModifier(const CKeyIDs& ids,
				KeyModifierMask* mask, bool isCommand) const
{
	if (!isCommand) {
		for (CKeyIDs::const_iterator i = ids.begin(); i != ids.end(); ++i) {
			KeyID id = *i;
			if (id != kKeyNone &&
				((id < 0xe000u || id > 0xefffu) ||
				(id >= kKeyKP_Equal && id <= kKeyKP_9))) {
				*mask |= KeyModifierAltGr;
				return;
			}
		}
	}
}

KeyButton
COSXKeyState::mapVirtualKeyToKeyButton(UInt32 keyCode)
{
	// 'A' maps to 0 so shift every id
	return static_cast<KeyButton>(keyCode + KeyButtonOffset);
}

UInt32
COSXKeyState::mapKeyButtonToVirtualKey(KeyButton keyButton)
{
	return static_cast<UInt32>(keyButton - KeyButtonOffset);
}


//
// COSXKeyState::CKeyResource
//

KeyID
COSXKeyState::CKeyResource::getKeyID(UInt8 c)
{
	if (c == 0) {
		return kKeyNone;
	}
	else if (c >= 32 && c < 127) {
		// ASCII
		return static_cast<KeyID>(c);
	}
	else {
		// handle special keys
		switch (c) {
		case 0x01:
			return kKeyHome;

		case 0x02:
			return kKeyKP_Enter;

		case 0x03:
			return kKeyKP_Enter;

		case 0x04:
			return kKeyEnd;

		case 0x05:
			return kKeyHelp;

		case 0x08:
			return kKeyBackSpace;

		case 0x09:
			return kKeyTab;

		case 0x0b:
			return kKeyPageUp;

		case 0x0c:
			return kKeyPageDown;

		case 0x0d:
			return kKeyReturn;

		case 0x10:
			// OS X maps all the function keys (F1, etc) to this one key.
			// we can't determine the right key here so we have to do it
			// some other way.
			return kKeyNone;

		case 0x1b:
			return kKeyEscape;

		case 0x1c:
			return kKeyLeft;

		case 0x1d:
			return kKeyRight;

		case 0x1e:
			return kKeyUp;

		case 0x1f:
			return kKeyDown;

		case 0x7f:
			return kKeyDelete;

		case 0x06:
		case 0x07:
		case 0x0a:
		case 0x0e:
		case 0x0f:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
			// discard other control characters
			return kKeyNone;

		default:
			// not special or unknown
			break;
		}

		// create string with character
		char str[2];
		str[0] = static_cast<char>(c);
		str[1] = 0;

		// convert to unicode
		CFStringRef cfString =
			CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
							str, GetScriptManagerVariable(smKeyScript),
							kCFAllocatorNull);

		// convert to precomposed
		CFMutableStringRef mcfString =
			CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
		CFRelease(cfString);
		CFStringNormalize(mcfString, kCFStringNormalizationFormC);

		// check result
		int unicodeLength = CFStringGetLength(mcfString);
		if (unicodeLength == 0) {
			CFRelease(mcfString);
			return kKeyNone;
		}
		if (unicodeLength > 1) {
			// FIXME -- more than one character, we should handle this
			CFRelease(mcfString);
			return kKeyNone;
		}

		// get unicode character
		UniChar uc = CFStringGetCharacterAtIndex(mcfString, 0);
		CFRelease(mcfString);

		// convert to KeyID
		return static_cast<KeyID>(uc);
	}
}

KeyID
COSXKeyState::CKeyResource::unicharToKeyID(UniChar c)
{
	switch (c) {
	case 3:
		return kKeyKP_Enter;

	case 8:
		return kKeyBackSpace;

	case 9:
		return kKeyTab;

	case 13:
		return kKeyReturn;

	case 27:
		return kKeyEscape;

	case 127:
		return kKeyDelete;

	default:
		if (c < 32) {
			return kKeyNone;
		}
		return static_cast<KeyID>(c);
	}
}


//
// COSXKeyState::CKCHRKeyResource
//

COSXKeyState::CKCHRKeyResource::CKCHRKeyResource(const void* resource)
{
	m_resource = reinterpret_cast<const KCHRResource*>(resource);
}

bool
COSXKeyState::CKCHRKeyResource::isValid() const
{
	return (m_resource != NULL);
}

UInt32
COSXKeyState::CKCHRKeyResource::getNumModifierCombinations() const
{
	return 256;
}

UInt32
COSXKeyState::CKCHRKeyResource::getNumTables() const
{
	return m_resource->m_numTables;
}

UInt32
COSXKeyState::CKCHRKeyResource::getNumButtons() const
{
	return 128;
}

UInt32
COSXKeyState::CKCHRKeyResource::getTableForModifier(UInt32 mask) const
{
	assert(mask < getNumModifierCombinations());

	return m_resource->m_tableSelectionIndex[mask];
}

KeyID
COSXKeyState::CKCHRKeyResource::getKey(UInt32 table, UInt32 button) const
{
	assert(table < getNumTables());
	assert(button < getNumButtons());

	UInt8 c = m_resource->m_characterTables[table][button];
	if (c == 0) {
		// could be a dead key
		const CKCHRDeadKeys* dkp =
			reinterpret_cast<const CKCHRDeadKeys*>(
				m_resource->m_characterTables[getNumTables()]);
		const CKCHRDeadKeyRecord* dkr = dkp->m_records;
		for (SInt16 i = 0; i < dkp->m_numRecords; ++i) {
			if (dkr->m_tableIndex == table && dkr->m_virtualKey == button) {
				// get the no completion entry
				c = dkr->m_completion[dkr->m_numCompletions][1];
				return CKeyMap::getDeadKey(getKeyID(c));
			}

			// next table.  skip all the completions and the no match
			// pair to get the next table.
			dkr = reinterpret_cast<const CKCHRDeadKeyRecord*>(
							dkr->m_completion[dkr->m_numCompletions + 1]);
		}
	}

	return getKeyID(c);
}


//
// COSXKeyState::CUCHRKeyResource
//

COSXKeyState::CUCHRKeyResource::CUCHRKeyResource(const void* resource,
				UInt32 keyboardType) :
	m_m(NULL),
	m_cti(NULL),
	m_sdi(NULL),
	m_sri(NULL),
	m_st(NULL)
{
	m_resource = reinterpret_cast<const UCKeyboardLayout*>(resource);
	if (m_resource == NULL) {
		return;
	}

	// find the keyboard info for the current keyboard type
	const UCKeyboardTypeHeader* th = NULL;
	const UCKeyboardLayout* r = m_resource;
	for (ItemCount i = 0; i < r->keyboardTypeCount; ++i) {
		if (keyboardType >= r->keyboardTypeList[i].keyboardTypeFirst &&
			keyboardType <= r->keyboardTypeList[i].keyboardTypeLast) {
			th = r->keyboardTypeList + i;
			break;
		}
		if (r->keyboardTypeList[i].keyboardTypeFirst == 0) {
			// found the default.  use it unless we find a match.
			th = r->keyboardTypeList + i;
		}
	}
	if (th == NULL) {
		// cannot find a suitable keyboard type
		return;
	}

	// get tables for keyboard type
	const UInt8* base = reinterpret_cast<const UInt8*>(m_resource);
	m_m   = reinterpret_cast<const UCKeyModifiersToTableNum*>(base +
								th->keyModifiersToTableNumOffset);
	m_cti = reinterpret_cast<const UCKeyToCharTableIndex*>(base +
								th->keyToCharTableIndexOffset);
	m_sdi = reinterpret_cast<const UCKeySequenceDataIndex*>(base +
								th->keySequenceDataIndexOffset);
	if (th->keyStateRecordsIndexOffset != 0) {
		m_sri = reinterpret_cast<const UCKeyStateRecordsIndex*>(base +
								th->keyStateRecordsIndexOffset);
	}
	if (th->keyStateTerminatorsOffset != 0) {
		m_st = reinterpret_cast<const UCKeyStateTerminators*>(base +
								th->keyStateTerminatorsOffset);
	}

	// find the space key, but only if it can combine with dead keys.
	// a dead key followed by a space yields the non-dead version of
	// the dead key.
	m_spaceOutput = 0xffffu;
	UInt32 table  = getTableForModifier(0);
	for (UInt32 button = 0, n = getNumButtons(); button < n; ++button) {
		KeyID id = getKey(table, button);
		if (id == 0x20) {
			UCKeyOutput c =
				reinterpret_cast<const UCKeyOutput*>(base +
								m_cti->keyToCharTableOffsets[table])[button];
			if ((c & kUCKeyOutputTestForIndexMask) ==
								kUCKeyOutputStateIndexMask) {
				m_spaceOutput = (c & kUCKeyOutputGetIndexMask);
				break;
			}
		}
	}
}

bool
COSXKeyState::CUCHRKeyResource::isValid() const
{
	return (m_m != NULL);
}

UInt32
COSXKeyState::CUCHRKeyResource::getNumModifierCombinations() const
{
	return 256;
}

UInt32
COSXKeyState::CUCHRKeyResource::getNumTables() const
{
	return m_cti->keyToCharTableCount;
}

UInt32
COSXKeyState::CUCHRKeyResource::getNumButtons() const
{
	return m_cti->keyToCharTableSize;
}

UInt32
COSXKeyState::CUCHRKeyResource::getTableForModifier(UInt32 mask) const
{
	if (mask >= m_m->modifiersCount) {
		return m_m->defaultTableNum;
	}
	else {
		return m_m->tableNum[mask];
	}
}

KeyID
COSXKeyState::CUCHRKeyResource::getKey(UInt32 table, UInt32 button) const
{
	assert(table < getNumTables());
	assert(button < getNumButtons());

	const UInt8* base   = reinterpret_cast<const UInt8*>(m_resource);
	const UCKeyOutput c = reinterpret_cast<const UCKeyOutput*>(base +
								m_cti->keyToCharTableOffsets[table])[button];

	KeySequence keys;
	switch (c & kUCKeyOutputTestForIndexMask) {
	case kUCKeyOutputStateIndexMask:
		if (!getDeadKey(keys, c & kUCKeyOutputGetIndexMask)) {
			return kKeyNone;
		}
		break;

	case kUCKeyOutputSequenceIndexMask:
	default:
		if (!addSequence(keys, c)) {
			return kKeyNone;
		}
		break;
	}

	// XXX -- no support for multiple characters
	if (keys.size() != 1) {
		return kKeyNone;
	}

	return keys.front();
}

bool
COSXKeyState::CUCHRKeyResource::getDeadKey(
				KeySequence& keys, UInt16 index) const
{
	if (m_sri == NULL || index >= m_sri->keyStateRecordCount) {
		// XXX -- should we be using some other fallback?
		return false;
	}

	UInt16 state = 0;
	if (!getKeyRecord(keys, index, state)) {
		return false;
	}
	if (state == 0) {
		// not a dead key
		return true;
	}

	// no dead keys if we couldn't find the space key
	if (m_spaceOutput == 0xffffu) {
		return false;
	}

	// the dead key should not have put anything in the key list
	if (!keys.empty()) {
		return false;
	}

	// get the character generated by pressing the space key after the
	// dead key.  if we're still in a compose state afterwards then we're
	// confused so we bail.
	if (!getKeyRecord(keys, m_spaceOutput, state) || state != 0) {
		return false;
	}

	// convert keys to their dead counterparts
	for (KeySequence::iterator i = keys.begin(); i != keys.end(); ++i) {
		*i = CKeyMap::getDeadKey(*i);
	}

	return true;
}

bool
COSXKeyState::CUCHRKeyResource::getKeyRecord(
				KeySequence& keys, UInt16 index, UInt16& state) const
{
	const UInt8* base = reinterpret_cast<const UInt8*>(m_resource);
	const UCKeyStateRecord* sr =
		reinterpret_cast<const UCKeyStateRecord*>(base +
								m_sri->keyStateRecordOffsets[index]);
	const UCKeyStateEntryTerminal* kset =
		reinterpret_cast<const UCKeyStateEntryTerminal*>(sr->stateEntryData);

	UInt16 nextState = 0;
	bool found       = false;
	if (state == 0) {
		found     = true;
		nextState = sr->stateZeroNextState;
		if (!addSequence(keys, sr->stateZeroCharData)) {
			return false;
		}
	}
	else {
		// we have a next entry
		switch (sr->stateEntryFormat) {
		case kUCKeyStateEntryTerminalFormat:
			for (UInt16 j = 0; j < sr->stateEntryCount; ++j) {
				if (kset[j].curState == state) {
					if (!addSequence(keys, kset[j].charData)) {
						return false;
					}
					nextState = 0;
					found     = true;
					break;
				}
			}
			break;

		case kUCKeyStateEntryRangeFormat:
			// XXX -- not supported yet
			break;

		default:
			// XXX -- unknown format
			return false;
		}
	}
	if (!found) {
		// use a terminator
		if (m_st != NULL && state < m_st->keyStateTerminatorCount) {
			if (!addSequence(keys, m_st->keyStateTerminators[state - 1])) {
				return false;
			}
		}
		nextState = sr->stateZeroNextState;
		if (!addSequence(keys, sr->stateZeroCharData)) {
			return false;
		}
	}

	// next
	state = nextState;

	return true;
}

bool
COSXKeyState::CUCHRKeyResource::addSequence(
				KeySequence& keys, UCKeyCharSeq c) const
{
	if ((c & kUCKeyOutputTestForIndexMask) == kUCKeyOutputSequenceIndexMask) {
		UInt16 index = (c & kUCKeyOutputGetIndexMask);
		if (index < m_sdi->charSequenceCount &&
			m_sdi->charSequenceOffsets[index] !=
				m_sdi->charSequenceOffsets[index + 1]) {
			// XXX -- sequences not supported yet
			return false;
		}
	}

	if (c != 0xfffe && c != 0xffff) {
		KeyID id = unicharToKeyID(c);
		if (id != kKeyNone) {
			keys.push_back(id);
		}
	}

	return true;
}
