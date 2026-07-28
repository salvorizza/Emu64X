include "dependencies.lua"

workspace "Emu64X"
	architecture "x86_64"
	startproject "Emu64X"
	toolset "clang"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	multiprocessorcompile "on"

	filter "system:windows"
		systemversion "latest"
		defines { "_CRT_SECURE_NO_WARNINGS" }
		buildoptions { "-Wno-unused-command-line-argument" }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "Full"

	filter "configurations:Dist"
		runtime "Release"
		optimize "Full"

	filter {}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Emu64X/vendor/Glad"
	include "Emu64X/vendor/glfw"
	include "Emu64X/vendor/imgui"
	include "Emu64X/vendor/stb"
	include "Emu64X/vendor/imgui-console"
	include "Emu64X/vendor/optick"
	include "Emu64X/vendor/miniaudio"
group ""

include "Emu64X"