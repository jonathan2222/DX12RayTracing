project "Game1"
	kind "ConsoleApp"
	language "C++"

	-- Targets
	targetdir ("%{wks.location}/Build/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/Build/obj/" .. outputdir .. "/%{prj.name}")

    -- Files to include
	files { GetFiles("Src/") }

    --includedirs { "Src" }

	defines
	{
		"RS_LOG_FILE_PATH=\"./Debug/Tmp/\"",
		"RS_ASSETS_PATH=\"./Assets/\""
	}

	UseEngine()

project "*"