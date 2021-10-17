function GetFiles(path)
	return path .. "**.hpp", path .. "**.h", path .. "**.cpp", path .. "**.c"
end

project "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	systemversion "latest"

    -- Pre-Compiled Headers
    pchheader "PreCompiled.h"
    pchsource "Src/PreCompiled.cpp"

    forceincludes
	{
		"PreCompiled.h"
	}

	-- Targets
	targetdir ("%{wks.location}/Build/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/Build/obj/" .. outputdir .. "/%{prj.name}")

    -- Files to include
	files { GetFiles("Src/") }

	--Includes
    includedirs { "Src" }

    sysincludedirs
	{
		"%{includeDir.glm}",
		"%{includeDir.imgui}",
		"%{includeDir.spdlog}",
		"%{includeDir.stb}",
		"%{includeDir.json}",
		"%{includeDir.glfw}"
	}

	filter "configurations:Debug"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"d3dcompiler.lib"
		}

	filter "configurations:Release"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"d3dcompiler.lib"
		}
	filter "configurations:Production"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"d3dcompiler.lib"
		}
	filter {}

project "*"