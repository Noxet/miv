workspace "ned"
    configurations { "Debug", "Release", "ASAN", "MSAN" }

project "ned"
    kind "ConsoleApp"
    language "C"
    cdialect "gnu11"
    targetdir "build/%{cfg.buildcfg}"
    toolset "clang"
    
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
            "-Wshadow",
            "-pedantic",
            "-Wswitch-enum",
        }


    filter "configurations:ASAN"
        buildoptions { "-fsanitize=address" }
        linkoptions { "-lasan" }
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:MSAN"
        buildoptions { "-fsanitize=memory" }
        linkoptions { "-fsanitize=memory" }
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        optimize "On"
