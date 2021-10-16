workspace "RayTracingProjectD3D12"
	startproject "Sandbox"
	architecture "x64"
	warnings "extra"
	flags { "MultiProcessorCompile" }
	
	-- Disable C4201 nonstandard extension used: nameless struct/union
	disablewarnings { "4201" }

	-- Link warning suppression
	-- LNK4006: Sympbol already defined in another library will pick first definition
	-- LNK4099: Debugging Database file (pdb) missing for given obj
	-- LNK4098: defaultlib 'library' conflicts with use of other libs; use /NODEFAULTLIB:library
	linkoptions { "-IGNORE:4006,4099,4098" }

	-- Platform
	platforms
	{
		"x64"
	}

	-- Configurations
	configurations
	{
		"Debug",
		"Release",
		"Production",
	}

	filter "configurations:Debug"
		symbols "on"
		runtime "Debug"
		defines
		{
			"RS_CONFIG_DEBUG",
		}
	filter "configurations:Release"
		symbols "on"
		runtime "Release"
		optimize "Full"
		defines
		{
			"RS_CONFIG_RELEASE",
		}
		-- Disable C100 Unused parameter
		disablewarnings { "4100" }
	filter "configurations:Production"
		symbols "off"
		runtime "Release"
		optimize "Full"
		defines
		{
			"RS_CONFIG_PRODUCTION",
		}
		-- Disable C100 Unused parameter
		disablewarnings { "4100" }
	filter {}

	-- Compiler option
	filter "action:vs*"
		defines
		{
			"RS_VISUAL_STUDIO",
			"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
			"_CRT_SECURE_NO_WARNINGS",
		}
	filter { "action:vs*", "configurations:Debug" }
		defines
		{
			"_CRTDBG_MAP_ALLOC",
		}

	filter "system:windows"
		defines
		{
			"RS_PLATFORM_WINDOWS",
		}

	filter {}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}-%{cfg.platform}"

includeDir = {}
includeDir["glfw"] 		= "%{wks.location}/Externals/glfw/include"
includeDir["imgui"] 	= "%{wks.location}/Externals/imgui"
includeDir["glm"] 		= "%{wks.location}/Externals/glm/"
includeDir["spdlog"] 	= "%{wks.location}/Externals/spdlog/include"
includeDir["stb"] 		= "%{wks.location}/Externals/stb/"
includeDir["json"] 		= "%{wks.location}/Externals/json/single_include"

group "Externals"
	include "Externals/glfw"
	include "Externals/imgui"
group ""

include "Projects/Sandbox"