function EngineIncludes()
	-- Main source files
	includedirs { "%{wks.location}/Projects/Engine/Src" }
	
	-- External files
    externalincludedirs
	{
		"%{includeDir.glm}",
		"%{includeDir.dxc}",
		"%{includeDir.imgui}",
		"%{includeDir.spdlog}",
		"%{includeDir.stb}",
		"%{includeDir.json}",
		"%{includeDir.glfw}",
		"%{includeDir.renderdoc}",
		"%{includeDir.freetype2}",
		"%{includeDir.PortAudio}"
	}
end

function EngineLinks()
    filter "configurations:Debug"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"dxguid.lib",
			"%{libDir.dxc}dxcompiler.lib",
			"%{libDir.freetype2}windows/x64/DebugStatic/freetype.lib",
			"%{libDir.PortAudio}Debug/portaudio_static_x64.lib"
		}

	filter "configurations:Release"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"%{libDir.dxc}dxcompiler.lib",
			"%{libDir.freetype2}windows/x64/ReleaseStatic/freetype.lib",
			"%{libDir.PortAudio}Release/portaudio_static_x64.lib"
		}
	filter "configurations:Production"
		links
		{
			"glfw",
			"imgui",
			"d3d12.lib",
			"dxgi.lib",
			"%{libDir.dxc}dxcompiler.lib",
			"%{libDir.freetype2}windows/x64/ReleaseStatic/freetype.lib",
			"%{libDir.PortAudio}Release/portaudio_static_x64.lib"
		}
	filter {}
end

function UseEngine()
	links "Engine"
	EngineIncludes()

	filter {"system:windows"}
		--D:\Dev\Projects\C++\DirectX\DX12Projects\RayTracingProject\Projects\Sandbox
		postbuildcommands {"xcopy /y /d %{dllDir.dxc}dxil.dll %{wks.location}Build\\bin\\" .. outputdir .. "\\%{prj.name}\\"}
		postbuildcommands {"xcopy /y /d %{dllDir.dxc}dxcompiler.dll %{wks.location}Build\\bin\\" .. outputdir .. "\\%{prj.name}\\"}
	filter {}
end

function GetFiles(path)
	return path .. "**.hpp", path .. "**.h", path .. "**.cpp", path .. "**.c"
end
