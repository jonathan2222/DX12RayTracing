#pragma once

#include "Types.h"
#include <unordered_map>
#include <string>
#include <type_traits>
#include <climits>
#include <format>
#include <filesystem>
#include <fstream>
#include <variant>

namespace RS
{
	/*
	* Header only variant data, with serialization support.
	* VMap is a structure that holds variant data and can group them.
	* Example:
	* VMap vmap;
	* vmap["Version"] = 1.0;
	* vmap["Options"]["Fullscreen"] = false;
	* vmap["Options"]["Width"] = 1920;
	* vmap["Options"]["Height"] = 1080;
	* vmap["Title"] = "Example App";
	* 
	* VMap subMap;
	* subMap["SomeData"] = { 1, 2, 3, 4 };
	* subMap["OtherData"] = { true, 0.12f, "Hej" };
	* vmap["Child"] = subMap;
	* 
	* One can also read from it the same way:
	* float version = vmap["Version"];
	* int width = vmap["Options"]["Width"];
	* int element = vmap["Child"]["SomeData"][2];
	* 
	* And one can read from disk and write to it.
	* vmap.WriteToDisk("./Some/Path.txt", errorMsgOut);
	* VMap vmap = VMap::ReadFromDisk("./Some/Path.txt", errorMsgOut);
	* 
	* Supported types:
	*	- bool
	*	- float
	*	- int
	*	- string
	*	- array (is just another VMap with TABLE as its type)
	*/

	static_assert(sizeof(float) * CHAR_BIT == 32, "Require 32 bits floats");
	class VMap
	{
	public:
		struct VElement
		{
			enum class Type { UNKNOWN = 0, BOOL, INT, FLOAT, STRING };
			VElement() : m_Int32(0u) {}
			VElement(bool x) : m_Type(Type::BOOL), m_Bool(x) {};
			VElement(int32 x) : m_Type(Type::INT), m_Int32(x) {};
			VElement(float x) : m_Type(Type::FLOAT), m_Float(x) {};
			VElement(const std::string& x) : m_Int32(0), m_Type(Type::STRING), m_String(x) {};
			VElement(const VElement& other) : m_Int32(other.m_Int32), m_Type(other.m_Type), m_String(other.m_String) {}
			VElement& operator=(const VElement& other) { return Copy(other); }
			VElement& operator=(VElement&& other) noexcept { Move(std::move(other)); return *this; }
			VElement(VElement&& other) noexcept { Move(std::move(other)); }
			~VElement() {}

			void Clear() { m_Type = Type::UNKNOWN; m_Int32 = 0; m_String.clear(); }

			// Implicit convertions:
			operator bool&()					{ RS_ASSERT_NO_MSG(m_Type == Type::BOOL); return m_Bool; }
			operator bool() const				{ RS_ASSERT_NO_MSG(m_Type == Type::BOOL); return m_Bool; }
			operator int32&()					{ RS_ASSERT_NO_MSG(m_Type == Type::INT); return m_Int32; }
			operator int32() const				{ RS_ASSERT_NO_MSG(m_Type == Type::INT); return m_Int32; }
			operator float&()					{ RS_ASSERT_NO_MSG(m_Type == Type::FLOAT); return m_Float; }
			operator float() const				{ RS_ASSERT_NO_MSG(m_Type == Type::FLOAT); return m_Float; }
			operator std::string&()				{ RS_ASSERT_NO_MSG(m_Type == Type::STRING); return m_String; }
			operator const std::string&() const	{ RS_ASSERT_NO_MSG(m_Type == Type::STRING); return m_String; }

			Type m_Type = Type::UNKNOWN;
			union
			{
				bool		m_Bool;
				int32		m_Int32;
				float		m_Float;
			};
			std::string m_String;

		private:
			VElement& Copy(const VElement& other)
			{
				m_Type = other.m_Type;
				m_String = other.m_String;
				m_Int32 = other.m_Int32;
				return *this;
			}
			void Move(VElement&& other) noexcept
			{
				m_Type = std::move(other.m_Type);
				m_String = std::move(other.m_String);
				m_Int32 = std::move(other.m_Int32);
			}
		};
	private:
		enum class Type { TABLE, ELEMENT };

	public:
		VMap() : m_Type(Type::TABLE) {}
		VMap(const VElement& element) : m_Type(Type::ELEMENT) { m_ElementData = element; }
		VMap(std::initializer_list<VElement> list) : m_Type(Type::TABLE) { Init(list); }
		VMap(const VMap& other) { Copy(other); }
		VMap(VMap&& other) noexcept { Move(std::move(other)); }

		VMap& operator=(const VElement& element) { m_Type = Type::ELEMENT; m_ElementData = element; return *this; }
		VMap& operator=(std::initializer_list<VElement> list) { Init(list); return *this; }
		VMap& operator=(const VMap& other) { return Copy(other); }
		VMap& operator=(VMap&& other) noexcept { return Move(std::move(other)); }

		// Implicit conversions:
		operator VElement&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator VElement() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }

		operator bool&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator bool() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator int32&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator int32() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator float&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator float() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator std::string&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator const std::string&() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }

		// Index operators:
		VMap& operator[](uint index) { return At(std::format("{}", index), true); }
		const VMap& operator[](uint index) const { return At(std::format("{}", index)); }
		VMap& operator[](const char* key) { return At(key, false); }
		const VMap& operator[](const char* key) const { return At(key); }
		VMap& operator[](const std::string& key) { return At(key, false); }
		const VMap& operator[](const std::string& key) const { return At(key); }

		void Clear()
		{
			m_Data.clear();
			m_ElementData.Clear();
			m_Type = Type::TABLE;
		}

		bool WriteToDisk(const std::filesystem::path path, std::optional<std::string>& errorMsg, bool compact = false)
		{
			errorMsg.reset();
			if (!std::filesystem::exists(path.parent_path()))
				std::filesystem::create_directories(path.parent_path());

			std::ofstream stream(path, std::ios::out);
			if (stream.is_open() == false)
			{
				errorMsg = std::format("Could not open file {}.", path.string());
				return false;
			}
			stream << m_sVersion << "\n";
			stream << (compact ? "true" : "false") << "\n";
			uint lineNumber = 2;
			if (!WriteToStream(stream, *this, 0, compact, errorMsg, lineNumber))
				stream.clear();
			stream.close();
			return true;
		}

		static VMap ReadFromDisk(const std::filesystem::path path, std::optional<std::string>& errorMsg)
		{
			errorMsg.reset();
			bool compact = false;
			VMap vmap;
			if (!std::filesystem::exists(path))
				return vmap;
			std::ifstream stream(path, std::ios::in);
			if (stream.is_open() == false)
			{
				errorMsg = std::format("Could not open file {}.", path.string());
				return vmap;
			}
			uint lineNumber = 0;
			std::string tmp;
			std::getline(stream, tmp);
			lineNumber++;
			Utils::Trim(tmp);
			try
			{
				int version = std::stoi(tmp);
				if (version != m_sVersion)
				{
					errorMsg = std::format("Failed to read from disk. Version on disk '{}' does not match current supported version '{}'", version, m_sVersion);
					stream.close();
					return vmap;
				}
				std::getline(stream, tmp);
				lineNumber++;
				Utils::Trim(tmp);
				if (tmp == "true")
					compact = true;
			}
			catch (std::exception& e)
			{
				errorMsg = std::format("Failed to read from disk at line {}. {}", lineNumber, e.what());
				stream.close();
				return vmap;
			}
			if (!ReadFromStream(stream, vmap, compact, errorMsg, lineNumber, ""))
				return {};
			stream.close();
			return vmap;
		}

	private:
		VMap& At(const std::string& key, bool isElement)
		{
			RS_ASSERT(m_Type == Type::TABLE, "VMap must be an array!");

			size_t p = key.find_first_of('/');
			if (p == std::string::npos)
			{
				VMap& vmap = m_Data[key];
				if (isElement) vmap.m_Type = Type::ELEMENT;
				vmap.m_Key = key;
				return vmap;
			}

			std::string firstKey = key.substr(0, p);
			VMap& vmap = m_Data.at(firstKey);
			if (isElement) vmap.m_Type = Type::ELEMENT;
			vmap.m_Key = key;
			if (key.size() > (p + 1))
			{
				std::string restOfTheKeys = key.substr(firstKey.size() + 1);
				return vmap.At(restOfTheKeys, isElement);
			}
			return vmap;
		}
		const VMap& At(const std::string& key) const
		{
			RS_ASSERT(m_Type == Type::TABLE, "VMap must be an array!");

			size_t p = key.find_first_of('/');
			if (p == std::string::npos)
				return m_Data.at(key);

			std::string firstKey = key.substr(0, p);
			const VMap& vmap = m_Data.at(firstKey);
			if (key.size() > (p+1))
			{
				std::string restOfTheKeys = key.substr(firstKey.size() + 1);
				return vmap.At(restOfTheKeys);
			}
			return vmap;
		}

		void Init(std::initializer_list<VElement> list)
		{
			m_Type = Type::TABLE;
			uint index = 0;
			for (auto it = list.begin(); it != list.end(); it++)
			{
				std::string key = std::format("{}", index++);
				m_Data[key] = *it;
			}
		}

		VMap& Copy(const VMap& other)
		{
			m_Type = other.m_Type;
			if (m_Type == Type::ELEMENT)
				m_ElementData = other.m_ElementData;
			else
				m_Data = other.m_Data;
			return *this;
		}

		VMap& Move(VMap&& other) noexcept
		{
			m_Type = std::move(other.m_Type);
			if (m_Type == Type::ELEMENT)
				m_ElementData = std::move(other.m_ElementData);
			else
				m_Data = std::move(other.m_Data);
			return *this;
		}

		void AddToLine(std::string& line, uint indent, const std::string& value)
		{
			std::string indentStr(indent, '\t');
			std::stringstream ss;
			ss << indentStr << "\"" << value << "\"";
			line += ss.str();
		}

		void AddToLine(std::string& line, const bool& value)
		{
			std::stringstream ss;
			ss << (value ? "true" : "false");
			line += ss.str();
		}

		void AddToLine(std::string& line, const auto& value)
		{
			std::stringstream ss;
			ss << value;
			line += ss.str();
		}

		bool WriteToStream(std::ofstream& stream, const VMap& vmap, uint indentationIndex, bool compact, std::optional<std::string>& errorMsg, uint& lineNumber)
		{
			auto PushLine = [&stream, &indentationIndex, compact, &lineNumber](uint indent, const auto& value) {
				if (compact) stream << value << "\n";
				else
				{
					std::string indentStr(indentationIndex + indent, '\t');
					stream << indentStr << value << "\n";
					lineNumber++;
				}
			};

			try
			{
				if (!compact) PushLine(0, "{");
				std::string line = TypeToString(vmap.m_Type);
				if (vmap.m_Type == Type::ELEMENT)
				{
					// Example:
					//		E_I 10
					//		E_S "Some string"
					line += "_" + TypeToString(vmap.m_ElementData.m_Type) + " ";
					switch (vmap.m_ElementData.m_Type)
					{
					case VElement::Type::BOOL: AddToLine(line, vmap.m_ElementData.m_Bool); break;
					case VElement::Type::INT: AddToLine(line, vmap.m_ElementData.m_Int32); break;
					case VElement::Type::FLOAT: AddToLine(line, vmap.m_ElementData.m_Float); break;
					case VElement::Type::STRING: AddToLine(line, 1, vmap.m_ElementData.m_String); break;
					default:
					{
						AddToLine(line, '_');
						errorMsg = std::format("Failed to write to disk at line {}. Type of key {} is not supported!", lineNumber, vmap.m_Key);
						break;
					}
					}
					PushLine(1, line);
				}
				else // TABLE
				{
					// Example:
					//		T 4
					line += " " + std::to_string(vmap.m_Data.size());
					PushLine(1, line);
					for (auto& [key, value] : vmap.m_Data)
					{
						// Example:
						//		"Version":
						PushLine(1, "\"" + key + "\":");
						if (!WriteToStream(stream, value, indentationIndex + 1, compact, errorMsg, lineNumber))
							return false;
					}
				}
				if (!compact) PushLine(0, "}");
				stream.flush();
				return true;
			}
			catch (std::exception e)
			{
				errorMsg = std::format("Failed to write to disk at line {}. {}", lineNumber, e.what());
				return false;
			}
		}

		static bool ReadFromStream(std::ifstream& stream, VMap& vmap, bool compact, std::optional<std::string>& errorMsg, uint& lineNumber, const std::string& key)
		{
			try
			{
				std::string tmp;
				auto FetchLine = [&tmp, &stream, &lineNumber]()
				{
					std::getline(stream, tmp);
					Utils::Trim(tmp);
					lineNumber++;
				};

				if (!compact)
				{
					FetchLine(); // {
				}
				FetchLine();
				if (StringToTType(tmp) == Type::ELEMENT)
				{
					vmap.m_Type = Type::ELEMENT;
					if (tmp[1] != '_' || tmp.size() < 4)
					{
						errorMsg = std::format("Failed to read from disk at line {}. Data is missing for element with key {}!", lineNumber, key);
						return false;
					}
					VElement::Type eType = StringToEType(std::string(1, tmp[2]));
					tmp = tmp.substr(4);
					switch (eType)
					{
						case RS::VMap::VElement::Type::BOOL:
						{
							vmap.m_ElementData.m_Type = RS::VMap::VElement::Type::BOOL;
							vmap.m_ElementData.m_Bool = tmp == "true";
							break;
						}
						case RS::VMap::VElement::Type::INT:
						{
							vmap.m_ElementData.m_Type = RS::VMap::VElement::Type::INT;
							vmap.m_ElementData.m_Int32 = std::stoi(tmp);
							break;
						}
						case RS::VMap::VElement::Type::FLOAT:
						{
							vmap.m_ElementData.m_Type = RS::VMap::VElement::Type::FLOAT;
							vmap.m_ElementData.m_Int32 = std::stof(tmp);
							break;
						}
						case RS::VMap::VElement::Type::STRING:
						{
							vmap.m_ElementData.m_Type = RS::VMap::VElement::Type::STRING;
							vmap.m_ElementData.m_String = tmp.substr(1, tmp.size() - 2);
							break;
						}
						case RS::VMap::VElement::Type::UNKNOWN:
						default:
						{
							errorMsg = std::format("Failed to read from disk at line {}. Type of key {} is not supported!", lineNumber, key);
							return false;
							break;
						}
					}
				}
				else // TABLE
				{
					vmap.m_Type = Type::TABLE;
					tmp = tmp.substr(2);
					uint dataCount = std::stoll(tmp);
					for (uint i = 0; i < dataCount; ++i)
					{
						FetchLine();
						if (tmp.size() < 3)
						{
							errorMsg = std::format("Failed to read from disk at line {}. Not enough characters for the key! Need to be in the format: \"<some key name>\":", lineNumber);
							return false;
						}
						tmp = tmp.substr(1, tmp.size()-3);
						if (tmp.empty())
						{
							errorMsg = std::format("Failed to read from disk at line {}. Key is empty!", lineNumber);
							return false;
						}
						VMap& subMap = vmap.m_Data[tmp];
						if (!ReadFromStream(stream, subMap, compact, errorMsg, lineNumber, tmp))
							return false;
					}
				}

				if (!compact) FetchLine(); // }
				return true;
			}
			catch (std::exception e)
			{
				errorMsg = std::format("Failed to read from disk at line {}. {}", lineNumber, e.what());
				return false;
			}
		}

		static std::string TypeToString(Type type)
		{
			switch (type)
			{
			case RS::VMap::Type::ELEMENT: return "E";
			case RS::VMap::Type::TABLE:
			default: return "T";
			}
		}

		static std::string TypeToString(VElement::Type type)
		{
			switch (type)
			{
			case RS::VMap::VElement::Type::BOOL: return "B";
			case RS::VMap::VElement::Type::INT: return "I";
			case RS::VMap::VElement::Type::FLOAT: return "F";
			case RS::VMap::VElement::Type::STRING: return "S";
			case RS::VMap::VElement::Type::UNKNOWN:
			default: return "_";
			}
		}

		static Type StringToTType(const std::string& str)
		{
			if (str.empty()) return Type::TABLE;
			if (str[0] == 'E') return Type::ELEMENT;
			return Type::TABLE;
		}

		static VElement::Type StringToEType(const std::string& str)
		{
			if (str.empty()) return VElement::Type::UNKNOWN;
			if (str[0] == 'B') return VElement::Type::BOOL;
			if (str[0] == 'I') return VElement::Type::INT;
			if (str[0] == 'F') return VElement::Type::FLOAT;
			if (str[0] == 'S') return VElement::Type::STRING;
			return VElement::Type::UNKNOWN;
		}

	private:
		std::unordered_map<std::string, VMap> m_Data;
		VElement m_ElementData;
		Type m_Type;
		std::string m_Key;

		inline static uint m_sVersion = 1;
	};

	using VArray = VMap;
}
