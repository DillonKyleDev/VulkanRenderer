project "Vulkan-Core"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   includedirs
   {
      "Source"
   }

   libdirs 
   {
    "../vendors/Vulkan/Lib",
    "../vendors/GLM/glm",
    "../vendors/GLFW/lib-vc2022"
   }

   externalincludedirs 
   {
    "../vendors/Vulkan/include",
    "../vendors/GLM/glm",
    "../vendors/GLFW/include",
    "../vendors/GLFW/lib-vc2022",
    "../vendors/Stb_image",
    "../vendors/tinyobjloader"
   }

   links
   {
    "vulkan-1.lib",
    "glfw3dll.lib"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"