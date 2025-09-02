include "dependencies.lua"

workspace "Emu64X"
	architecture "x86_64"
	startproject "Emu64X"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

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