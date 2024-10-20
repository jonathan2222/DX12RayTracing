#pragma once

namespace RS
{
	class Engine
	{
	public:

		static void SetInternalDataFilePath(const std::string& dataFilePath)
		{
			s_InternalDataFilePath = dataFilePath;
		}
		static void SetDataFilePath(const std::string& dataFilePath)
		{
			s_DataFilePath = dataFilePath;
		}
		static void SetDebugFilePath(const std::string& debugFilePath)
		{
			s_DebugFilePath = debugFilePath;
		}
		static void SetTempFilePath(const std::string& tempFilePath)
		{
			s_TempFilePath = tempFilePath;
		}

		static const std::string& GetInternalDataFilePath()
		{
			return s_InternalDataFilePath;
		}
		static const std::string& GetDataFilePath()
		{
			return s_DataFilePath;
		}
		static const std::string& GetDebugFilePath()
		{
			return s_DebugFilePath;
		}
		static const std::string& GetTempFilePath()
		{
			return s_TempFilePath;
		}

	private:
		static inline std::string s_InternalDataFilePath = "../../Assets/"; // Where all the engine data is. Textures, Models, Shaders, etc.

		static inline std::string s_DataFilePath = "../../Assets/"; // Where all data is. Textures, Models, Shaders, etc.
		static inline std::string s_DebugFilePath = "../../Debug/"; // Debug data, logs, configs, etc.
		static inline std::string s_TempFilePath = "../../Debug/Tmp/"; // Temporary data, configs, etc.
	};
}