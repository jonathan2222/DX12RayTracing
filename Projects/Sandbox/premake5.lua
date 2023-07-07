project "Sandbox"
	kind "ConsoleApp"
	language "C++"

	-- Targets
	targetdir ("%{wks.location}/Build/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/Build/obj/" .. outputdir .. "/%{prj.name}")

    -- Files to include
	files { GetFiles("Src/") }

    --includedirs { "Src" }

	UseEngine()

project "*"