workspace "miv"
    configurations { "Debug", "Release" }

project "miv"
    kind "ConsoleApp"
    language "C"
    targetdir "build/%{cfg.buildcfg}"
    
    files
    {
        "src/**.h",
        "src/**.c",
    }

    filter "system:linux"
        buildoptions
        {
            "-Wall",
            "-Wextra",
            "-pedantic",
        }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        optimize "On"
