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
	* vmap["Version"] = 24u;
	* vmap["Options"]["Fullscreen"] = false;
	* vmap["Options"]["Width"] = 1920;
	* vmap["Options"]["Height"] = 1080;
	* vmap["Title"] = "Example App";
	* 
	* VMap subMap;
	* subMap["SomeData"] = { 1.f, 2.f, 3.f, 4.f };
	* subMap["OtherData"] = { true, 0.12f, "Hej" };
	* vmap["Child"] = subMap;
	* 
	* One can also read from it the same way:
	* uint version = vmap["Version"];
	* int width = vmap["Options/Width"];
	* float element = vmap["Child"]["SomeData"][2];
	* bool fullscreen = vmap.Fetch("Options/Fullscreen", true);
	* 
	* And one can read from disk and write to it.
	* VMap::WriteToDisk(vmap, "./Some/Path.txt", errorMsgOut);
	* VMap vmap = VMap::ReadFromDisk("./Some/Path.txt", errorMsgOut);
	* 
	* Supported types:
	*	- bool
	*	- float
	*	- int
	*	- uint
	*	- string
	*	- array (is just another VMap with TABLE as its type)
	*/

	static_assert(sizeof(float) * CHAR_BIT == 32, "Require 32 bits floats");
	class VMap
	{
	public:
		struct VElement
		{
			enum class Type { UNKNOWN = 0, BOOL, INT, UINT, FLOAT, STRING };
			VElement() : m_Int32(0u) {}
			VElement(bool x) : m_Type(Type::BOOL), m_Bool(x) {};
			VElement(int32 x) : m_Type(Type::INT), m_Int32(x) {};
			VElement(uint32 x) : m_Type(Type::UINT), m_UInt32(x) {};
			VElement(float x) : m_Type(Type::FLOAT), m_Float(x) {};
			VElement(const std::string& x) : m_Int32(0), m_Type(Type::STRING), m_String(x) {};
			VElement(const char* x) : m_Int32(0), m_Type(Type::STRING), m_String(x) {};
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
			operator uint32&()					{ RS_ASSERT_NO_MSG(m_Type == Type::UINT); return m_UInt32; }
			operator uint32() const				{ RS_ASSERT_NO_MSG(m_Type == Type::UINT); return m_UInt32; }
			operator float&()					{ RS_ASSERT_NO_MSG(m_Type == Type::FLOAT); return m_Float; }
			operator float() const				{ RS_ASSERT_NO_MSG(m_Type == Type::FLOAT); return m_Float; }
			operator std::string&()				{ RS_ASSERT_NO_MSG(m_Type == Type::STRING); return m_String; }
			operator const std::string&() const	{ RS_ASSERT_NO_MSG(m_Type == Type::STRING); return m_String; }

			template<typename T>
			bool IsOfType()
			{
				switch (m_Type)
				{
				case Type::BOOL: return std::is_same<T, bool>::value;
				case Type::INT: return std::is_same<T, int32>::value;
				case Type::UINT: return std::is_same<T, uint32>::value;
				case Type::FLOAT: return std::is_same<T, float>::value;
				case Type::STRING: return std::is_same<T, std::string>::value;
				default:
					break;
				}
				return false;
			}

			Type m_Type = Type::UNKNOWN;
			union
			{
				bool		m_Bool;
				int32		m_Int32;
				uint32		m_UInt32;
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

	public:
		enum class FileIOErrorCode { NONE, DOES_NOT_EXIST, OTHER };

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
		operator uint32& () { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator uint32() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator float&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator float() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator std::string&() { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }
		operator const std::string&() const { RS_ASSERT_NO_MSG(m_Type == Type::ELEMENT); return m_ElementData; }

		// Index operators:
		VMap& operator[](uint index) { return Get(std::format("{}", index), true); }
		const VMap& operator[](uint index) const { return At(std::format("{}", index)); }
		VMap& operator[](const char* key) { return Get(key, false); }
		const VMap& operator[](const char* key) const { return At(key); }
		VMap& operator[](const std::string& key) { return Get(key, false); }
		const VMap& operator[](const std::string& key) const { return At(key); }

		void Clear()
		{
			m_Data.clear();
			m_ElementData.Clear();
			m_Type = Type::TABLE;
		}

		VMap* GetIfExists(const std::string& key)
		{
			size_t p = key.find_first_of('/');
			if (p == std::string::npos)
			{
				auto it = m_Data.find(key);
				if (it != m_Data.end())
					return &it->second;
				return nullptr;
			}

			std::string firstKey = key.substr(0, p);
			auto it = m_Data.find(firstKey);
			if (it == m_Data.end())
				return nullptr;
			VMap& vmap = it->second;
			if (key.size() > (p + 1))
			{
				std::string restOfTheKeys = key.substr(firstKey.size() + 1);
				return vmap.GetIfExists(restOfTheKeys);
			}
			return &vmap;
		}

		const VMap* GetIfExists(const std::string& key) const
		{
			size_t p = key.find_first_of('/');
			if (p == std::string::npos)
			{
				auto it = m_Data.find(key);
				if (it != m_Data.end())
					return &it->second;
				return nullptr;
			}

			std::string firstKey = key.substr(0, p);
			auto it = m_Data.find(firstKey);
			if (it == m_Data.end())
				return nullptr;
			const VMap& vmap = it->second;
			if (key.size() > (p + 1))
			{
				std::string restOfTheKeys = key.substr(firstKey.size() + 1);
				return vmap.GetIfExists(restOfTheKeys);
			}
			return &vmap;
		}

		bool HasKey(const std::string& key) const
		{
			return GetIfExists(key) != nullptr;
		}

		VMap& Fetch(const std::string& key, const VElement& defaultValue)
		{
			VMap* vmap = GetIfExists(key);
			if (vmap) return *vmap;

			vmap = &Get(key, true);
			*vmap = defaultValue;
			return *vmap;
		}

		VMap& Fetch(const std::string& key, const VMap& defaultValue)
		{
			VMap* vmap = GetIfExists(key);
			if (vmap) return *vmap;

			vmap = &Get(key, defaultValue.m_Type == Type::ELEMENT);
			*vmap = defaultValue;
			return *vmap;
		}

		template<typename T>
		bool IsOfType()
		{
			if (m_Type == Type::TABLE)
				return false;
			return m_ElementData.IsOfType<T>();
		}

		static bool WriteToDisk(const VMap& vmap, const std::filesystem::path path, bool compact, std::optional<std::string>& errorMsg, FileIOErrorCode& errorCode)
		{
			errorCode = FileIOErrorCode::NONE;
			errorMsg.reset();
			if (!std::filesystem::exists(path.parent_path()))
				std::filesystem::create_directories(path.parent_path());

			std::ofstream stream(path, std::ios::out | std::ios::beg);
			if (stream.is_open() == false)
			{
				errorMsg = std::format("Could not open file {}.", path.string());
				errorCode = FileIOErrorCode::OTHER;
				return false;
			}
			stream.clear();
			stream << m_sVersion << "\n";
			stream << (compact ? "true" : "false") << "\n";
			uint lineNumber = 2;
			if (!WriteToStream(stream, vmap, 0, compact, errorMsg, errorCode, lineNumber))
				stream.clear();
			stream.close();
			return true;
		}

		static bool WriteToDisk(const VMap& vmap, const std::filesystem::path path, bool compact, FileIOErrorCode& errorCode)
		{
			std::optional<std::string> errorMessage;
			bool result = WriteToDisk(vmap, path, compact, errorMessage, errorCode);
			if (errorMessage.has_value())
				LOG_ERROR("Failed to write {}! Error: {}", (const char*)path.c_str(), errorMessage->c_str());
			return result;
		}

		static bool WriteToDisk(const VMap& vmap, const std::filesystem::path path, bool compact = false)
		{
			FileIOErrorCode errorCode;
			return WriteToDisk(vmap, path, compact, errorCode);
		}

		static VMap ReadFromDisk(const std::filesystem::path path, std::optional<std::string>& errorMsg, FileIOErrorCode& errorCode)
		{
			errorCode = FileIOErrorCode::NONE;
			errorMsg.reset();
			bool compact = false;
			VMap vmap;
			if (!std::filesystem::exists(path))
			{
				errorCode = FileIOErrorCode::DOES_NOT_EXIST;
				return vmap;
			}
			std::ifstream stream(path, std::ios::in);
			if (stream.is_open() == false)
			{
				errorMsg = std::format("Could not open file {}.", path.string());
				errorCode = FileIOErrorCode::OTHER;
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
					errorCode = FileIOErrorCode::OTHER;
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
				errorCode = FileIOErrorCode::OTHER;
				return vmap;
			}
			if (!ReadFromStream(stream, vmap, compact, errorMsg, errorCode, lineNumber, ""))
			{
				errorCode = FileIOErrorCode::OTHER;
				return {};
			}
			stream.close();
			return vmap;
		}

		static VMap ReadFromDisk(const std::filesystem::path path, FileIOErrorCode& errorCode)
		{
			std::optional<std::string> errorMessage;
			VMap vmap = VMap::ReadFromDisk(path, errorMessage, errorCode);
			if (errorMessage.has_value())
				LOG_ERROR("Failed to load {}! Error: {}", (const char*)path.c_str(), errorMessage->c_str());
			return vmap;
		}

		static VMap ReadFromDisk(const std::filesystem::path path)
		{
			FileIOErrorCode errorCode;
			VMap vmap = ReadFromDisk(path, errorCode);
			return vmap;
		}

	private:
		VMap& Get(const std::string& key, bool isElement)
		{
			size_t p = key.find_first_of('/');
			if (p == std::string::npos)
			{
				VMap& vmap = m_Data[key];
				if (isElement) vmap.m_Type = Type::ELEMENT;
				vmap.m_Key = key;
				return vmap;
			}

			std::string firstKey = key.substr(0, p);
			VMap& vmap = m_Data[firstKey];
			vmap.m_Key = key;
			if (key.size() > (p + 1))
			{
				std::string restOfTheKeys = key.substr(firstKey.size() + 1);
				return vmap.Get(restOfTheKeys, isElement);
			}
			return vmap;
		}
		const VMap& At(const std::string& key) const
		{
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

		static void AddToLine(std::string& line, const std::string& value)
		{
			std::stringstream ss;
			ss << "\"" << value << "\"";
			line += ss.str();
		}

		static void AddToLine(std::string& line, const bool& value)
		{
			std::stringstream ss;
			ss << (value ? "true" : "false");
			line += ss.str();
		}

		static void AddToLine(std::string& line, const auto& value)
		{
			std::stringstream ss;
			ss << value;
			line += ss.str();
		}

		static bool WriteToStream(std::ofstream& stream, const VMap& vmap, uint indentationIndex, bool compact, std::optional<std::string>& errorMsg, FileIOErrorCode& errorCode, uint& lineNumber)
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
					case VElement::Type::UINT: AddToLine(line, vmap.m_ElementData.m_UInt32); break;
					case VElement::Type::FLOAT: AddToLine(line, vmap.m_ElementData.m_Float); break;
					case VElement::Type::STRING: AddToLine(line, vmap.m_ElementData.m_String); break;
					default:
					{
						AddToLine(line, '_');
						errorMsg = std::format("Failed to write to disk at line {}. Type of key {} is not supported!", lineNumber, vmap.m_Key);
						errorCode = FileIOErrorCode::OTHER;
						return false;
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
						if (!WriteToStream(stream, value, indentationIndex + 1, compact, errorMsg, errorCode, lineNumber))
						{
							errorCode = FileIOErrorCode::OTHER;
							return false;
						}
					}
				}
				if (!compact) PushLine(0, "}");
				stream.flush();
				return true;
			}
			catch (std::exception e)
			{
				errorMsg = std::format("Failed to write to disk at line {}. {}", lineNumber, e.what());
				errorCode = FileIOErrorCode::OTHER;
				return false;
			}
		}

		static bool ReadFromStream(std::ifstream& stream, VMap& vmap, bool compact, std::optional<std::string>& errorMsg, FileIOErrorCode& errorCode, uint& lineNumber, const std::string& key)
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
						errorCode = FileIOErrorCode::OTHER;
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
						case RS::VMap::VElement::Type::UINT:
						{
							vmap.m_ElementData.m_Type = RS::VMap::VElement::Type::UINT;
							vmap.m_ElementData.m_UInt32 = (uint32)std::stoull(tmp);
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
							errorCode = FileIOErrorCode::OTHER;
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
							errorCode = FileIOErrorCode::OTHER;
							return false;
						}
						tmp = tmp.substr(1, tmp.size()-3);
						if (tmp.empty())
						{
							errorMsg = std::format("Failed to read from disk at line {}. Key is empty!", lineNumber);
							errorCode = FileIOErrorCode::OTHER;
							return false;
						}
						VMap& subMap = vmap.m_Data[tmp];
						if (!ReadFromStream(stream, subMap, compact, errorMsg, errorCode, lineNumber, tmp))
						{
							errorCode = FileIOErrorCode::OTHER;
							return false;
						}
					}
				}

				if (!compact) FetchLine(); // }
				return true;
			}
			catch (std::exception e)
			{
				errorMsg = std::format("Failed to read from disk at line {}. {}", lineNumber, e.what());
				errorCode = FileIOErrorCode::OTHER;
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
			case RS::VMap::VElement::Type::UINT: return "U";
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
			if (str[0] == 'U') return VElement::Type::UINT;
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
