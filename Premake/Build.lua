-- premake5.lua
workspace "Vulkan Renderer"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Vulkan-Runtime"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Vulkan-Core"
	include "Vulkan-Core/Build-Core.lua"
group ""

include "Vulkan-Runtime/Build-Runtime.lua"