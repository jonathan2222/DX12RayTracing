project "Engine"
	kind "StaticLib"
	language "C++"

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

    EngineIncludes()
	
	EngineLinks()

project "*"