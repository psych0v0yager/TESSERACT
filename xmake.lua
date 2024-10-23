-- set minimum xmake version
set_xmakever("2.8.2")

-- includes
includes("lib/commonlibsse-ng")

-- add requires
add_requires("nlohmann_json")
add_requires("libcurl")

-- set project
set_project("TESSERACT")
set_version("0.0.0")
set_license("MIT")

-- set defaults
set_languages("c++23")
set_warnings("allextra", "error")
set_defaultmode("releasedbg")

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- set policies
set_policy("package.requires_lock", true)

-- set configs
set_config("skyrim_vr", false)

-- targets
target("TESSERACT")
    -- add dependencies to target
    add_deps("commonlibsse-ng")
    add_packages("nlohmann_json")
    add_packages("libcurl")

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "TESSERACT",
        author = "psych0v0yager",
        description = "The Elder Scrolls System & Environment for Real-time Autonomous Character Toolkit"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    add_includedirs("lib/openai-cpp/include")  -- OpenAI-cpp include directory
    add_includedirs("lib/SKSE-Menu-Framework-2-Example/include")  -- Path to the specific header in the submodule
    set_pcxxheader("src/pch.h")

    -- platform specific flags
    if is_plat("windows") then
        add_cxflags("/WX-")  -- Disables treating warnings as errors in MSVC
    else
        add_cxflags("-Wno-error")  -- GCC/Clang to not treat warnings as errors
    end

    -- copy build files to MODS or GAME paths (remove this if not needed)
    after_build(function(target)
        local copy = function(env, ext)
            for _, env in pairs(env:split(";")) do
                if os.exists(env) then
                    local plugins = path.join(env, ext, "SKSE/Plugins")
                    os.mkdir(plugins)
                    os.trycp(target:targetfile(), plugins)
                    os.trycp(target:symbolfile(), plugins)
                end
            end
        end
        if os.getenv("XSE_TES5_MODS_PATH") then
            copy(os.getenv("XSE_TES5_MODS_PATH"), target:name())
        elseif os.getenv("XSE_TES5_GAME_PATH") then
            copy(os.getenv("XSE_TES5_GAME_PATH"), "Data")
        end
    end)