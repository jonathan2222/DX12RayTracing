#pragma once

namespace RS
{
	enum class KeyState
	{
		RELEASED = 0,
		FIRST_RELEASED = 1,
		FIRST_PRESSED = 2,
		PRESSED = 3
	};

	enum class MB
	{
		LEFT = 0,
		RIGHT = 1,
		MIDDLE = 2
	};

	inline static constexpr const char* MB_Strings[3] = { "LEFT", "RIGHT", "MIDDLE" };

	enum class Key
	{
		UNKNOWN = -1,

		/* Printable keys */
		SPACE = 32,
		COMMA = 44,  /* , */
		MINUS = 45,  /* - */
		PERIOD = 46,  /* . */
		NUMBER0 = 48,
		NUMBER1 = 49,
		NUMBER2 = 50,
		NUMBER3 = 51,
		NUMBER4 = 52,
		NUMBER5 = 53,
		NUMBER6 = 54,
		NUMBER7 = 55,
		NUMBER8 = 56,
		NUMBER9 = 57,
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		MICRO = 96, // §

		/* Function keys */
		ESCAPE = 256,
		ENTER = 257,
		TAB = 258,
		BACKSPACE = 259,
		INSERT = 260,
		DELETE_KEY = 261,
		RIGHT = 262,
		LEFT = 263,
		DOWN = 264,
		UP = 265,
		PAGE_UP = 266,
		PAGE_DOWN = 267,
		HOME = 268,
		END = 269,
		CAPS_LOCK = 280,
		SCROLL_LOCK = 281,
		NUM_LOCK = 282,
		PRINT_SCREEN = 283,
		PAUSE = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		KP_0 = 320,
		KP_1 = 321,
		KP_2 = 322,
		KP_3 = 323,
		KP_4 = 324,
		KP_5 = 325,
		KP_6 = 326,
		KP_7 = 327,
		KP_8 = 328,
		KP_9 = 329,
		KP_DECIMAL = 330,
		KP_DIVIDE = 331,
		KP_MULTIPLY = 332,
		KP_SUBTRACT = 333,
		KP_ADD = 334,
		LEFT_SHIFT = 340,
		LEFT_CONTROL = 341,
		LEFT_ALT = 342,
		RIGHT_SHIFT = 344,
		RIGHT_CONTROL = 345,
		RIGHT_ALT = 346,
		MENU = 348,

		FIRST = SPACE,
		LAST = MENU
	};

	inline static const char* ToString(Key key, bool asChars = true)
	{
		switch (key)
		{
		case RS::Key::UNKNOWN: return "UNKNOWN";
		case RS::Key::SPACE: return asChars ? " " : "Space";
		case RS::Key::COMMA: return asChars ? "," : "Comma";
		case RS::Key::MINUS: return asChars ? "-" : "Muinus";
		case RS::Key::PERIOD: return asChars ? "." : "Period";
		case RS::Key::NUMBER0: return asChars ? "0" : "NUMBER0";
		case RS::Key::NUMBER1: return asChars ? "1" : "NUMBER1";
		case RS::Key::NUMBER2: return asChars ? "2" : "NUMBER2";
		case RS::Key::NUMBER3: return asChars ? "3" : "NUMBER3";
		case RS::Key::NUMBER4: return asChars ? "4" : "NUMBER4";
		case RS::Key::NUMBER5: return asChars ? "5" : "NUMBER5";
		case RS::Key::NUMBER6: return asChars ? "6" : "NUMBER6";
		case RS::Key::NUMBER7: return asChars ? "7" : "NUMBER7";
		case RS::Key::NUMBER8: return asChars ? "8" : "NUMBER8";
		case RS::Key::NUMBER9: return asChars ? "9" : "NUMBER9";
		case RS::Key::A: return "A";
		case RS::Key::B: return "B";
		case RS::Key::C: return "C";
		case RS::Key::D: return "D";
		case RS::Key::E: return "E";
		case RS::Key::F: return "F";
		case RS::Key::G: return "G";
		case RS::Key::H: return "H";
		case RS::Key::I: return "I";
		case RS::Key::J: return "J";
		case RS::Key::K: return "K";
		case RS::Key::L: return "L";
		case RS::Key::M: return "M";
		case RS::Key::N: return "N";
		case RS::Key::O: return "O";
		case RS::Key::P: return "P";
		case RS::Key::Q: return "Q";
		case RS::Key::R: return "R";
		case RS::Key::S: return "S";
		case RS::Key::T: return "T";
		case RS::Key::U: return "U";
		case RS::Key::V: return "V";
		case RS::Key::W: return "W";
		case RS::Key::X: return "X";
		case RS::Key::Y: return "Y";
		case RS::Key::Z: return "Z";
		case RS::Key::MICRO: return asChars ? "§" : "Micro";
		case RS::Key::ESCAPE: return "Escape";
		case RS::Key::ENTER: return "Enter";
		case RS::Key::TAB: return "Tab";
		case RS::Key::BACKSPACE: return "Backspace";
		case RS::Key::INSERT: return "Insert";
		case RS::Key::DELETE_KEY: return "Delete";
		case RS::Key::RIGHT: return "Right";
		case RS::Key::LEFT: return "Left";
		case RS::Key::DOWN: return "Down";
		case RS::Key::UP: return "Up";
		case RS::Key::PAGE_UP: return "PageUp";
		case RS::Key::PAGE_DOWN: return "PageDown";
		case RS::Key::HOME: return "Home";
		case RS::Key::END: return "End";
		case RS::Key::CAPS_LOCK: return "CapsLock";
		case RS::Key::SCROLL_LOCK: return "ScrollLock";
		case RS::Key::NUM_LOCK: return "NumLock";
		case RS::Key::PRINT_SCREEN: return "PrintScreen";
		case RS::Key::PAUSE: return "Pause";
		case RS::Key::F1: return "F1";
		case RS::Key::F2: return "F2";
		case RS::Key::F3: return "F3";
		case RS::Key::F4: return "F4";
		case RS::Key::F5: return "F5";
		case RS::Key::F6: return "F6";
		case RS::Key::F7: return "F7";
		case RS::Key::F8: return "F8";
		case RS::Key::F9: return "F9";
		case RS::Key::F10: return "F10";
		case RS::Key::F11: return "F11";
		case RS::Key::F12: return "F12";
		case RS::Key::F13: return "F13";
		case RS::Key::F14: return "F14";
		case RS::Key::F15: return "F15";
		case RS::Key::F16: return "F16";
		case RS::Key::F17: return "F17";
		case RS::Key::F18: return "F18";
		case RS::Key::F19: return "F19";
		case RS::Key::F20: return "F20";
		case RS::Key::F21: return "F21";
		case RS::Key::F22: return "F22";
		case RS::Key::F23: return "F23";
		case RS::Key::F24: return "F24";
		case RS::Key::KP_0: return "0";
		case RS::Key::KP_1: return "1";
		case RS::Key::KP_2: return "2";
		case RS::Key::KP_3: return "3";
		case RS::Key::KP_4: return "4";
		case RS::Key::KP_5: return "5";
		case RS::Key::KP_6: return "6";
		case RS::Key::KP_7: return "7";
		case RS::Key::KP_8: return "8";
		case RS::Key::KP_9: return "9";
		case RS::Key::KP_DECIMAL: return asChars ? "," : "Decimal";
		case RS::Key::KP_DIVIDE: return asChars ? "/" : "Divide";
		case RS::Key::KP_MULTIPLY: return asChars ? "*" : "Multiply";
		case RS::Key::KP_SUBTRACT: return asChars ? "-" : "Subtract";
		case RS::Key::KP_ADD: return asChars ? "+" : "Add";
		case RS::Key::LEFT_SHIFT: return "LeftShift";
		case RS::Key::LEFT_CONTROL: return "LeftControl";
		case RS::Key::LEFT_ALT: return "LeftAlt";
		case RS::Key::RIGHT_SHIFT: return "RightShift";
		case RS::Key::RIGHT_CONTROL: return "RightControl";
		case RS::Key::RIGHT_ALT: return "RightAlt";
		case RS::Key::MENU: return "Menu";
		default:
			return "NoStringForKey";
		}
	}
}