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
    includedirs { "Src", "$(IntDir)CompiledShaders\\" }

    externalincludedirs
	{
		"%{includeDir.glm}",
		"%{includeDir.dxc}",
		"%{includeDir.imgui}",
		"%{includeDir.spdlog}",
		"%{includeDir.stb}",
		"%{includeDir.json}",
		"%{includeDir.glfw}"
	}

	filter("files:**.hlsl")
		shadermodel "6.3"
		--shadertype "Library"
		shadervariablename "g_p%(Filename)"
		shaderheaderfileoutput "$(IntDir)CompiledShaders\\%(Filename).hlsl.h"
	filter {}

	function S(s)
		return s:gsub("/", "\\")
	end

	filter {"system:windows"}
		--D:\Dev\Projects\C++\DirectX\DX12Projects\RayTracingProject\Projects\Sandbox
		postbuildcommands {"xcopy /y /d %{dllDir.dxc}dxil.dll %{wks.location}Build\\bin\\" .. outputdir .. "\\%{prj.name}\\"}
	filter {}

	filter "configurations:Debug"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"dxguid.lib",
			"%{libDir.dxc}dxcompiler.lib"
		}

	filter "configurations:Release"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"%{libDir.dxc}dxcompiler.lib"
		}
	filter "configurations:Production"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"%{libDir.dxc}dxcompiler.lib"
		}
	filter {}

project "*"