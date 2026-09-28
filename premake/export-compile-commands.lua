-- 用法（在仓库根目录）：
--   premake5 export-compile-commands                                        # 默认使用第一个配置（Debug）
--   premake5 export-compile-commands --export-compile-commands-config=Release

local p = premake

p.modules.export_compile_commands = {}
local m = p.modules.export_compile_commands

m.kOutputFile = "compile_commands.json"

-- 非 Windows 主机（如 macOS 上的 AI / clangd）没有 Win32 SDK：pch.h 中的 <Windows.h> 找不到会成为致命错误，
-- 导致整个翻译单元无法解析。以 -idirafter 引入空头文件桩兜底，只让真正调用 Win32 API 的少数位置报错。
m.kWin32StubDir = path.join(_SCRIPT_DIR, "clangd-win32-stubs")

local kCExtensions   = { [".c"] = true }
local kCppExtensions = { [".cpp"] = true, [".cc"] = true, [".cxx"] = true, [".c++"] = true }

local kWindowsTriples = {
    x86    = "i686-pc-windows-msvc",
    x86_64 = "x86_64-pc-windows-msvc",
    arm64  = "aarch64-pc-windows-msvc",
}

newoption {
    trigger     = "export-compile-commands-config",
    value       = "CONFIG",
    description = "Configuration exported by 'export-compile-commands' (default: first configuration)",
}

local function appendAll(dst, src)
    for _, v in ipairs(src or {}) do
        table.insert(dst, v)
    end
end

local function isFlagSet(cfg, flag)
    return cfg.flags ~= nil and cfg.flags[flag] == true
end

-- "C++20" -> "-std=c++20"，"gnu++17" -> "-std=gnu++17"，"C11" -> "-std=c11"
local function languageStandardFlag(dialect)
    if dialect == nil or dialect == "Default" then
        return nil
    end
    local d = dialect:lower()
    if d == "c++latest" then
        d = "c++2b"
    end
    return "-std=" .. d
end

-- MSVC 运行时库：影响 _DEBUG / _MT / _DLL 等预定义宏，以及 STL 的调试迭代器等分支
local function msvcRuntimeLibFlag(cfg)
    local isDebug
    if cfg.runtime ~= nil then
        isDebug = (cfg.runtime == "Debug")
    else
        isDebug = p.config.isDebugBuild(cfg)
    end
    local lib = (cfg.staticruntime == "On") and "static" or "dll"
    if isDebug then
        lib = lib .. "_dbg"
    end
    return "-fms-runtime-lib=" .. lib
end

-- pchheader 是 #include 中写的名字（如 "pch.h"），需要在 includedirs / pchsource 所在目录中定位到实际文件
local function findPchHeader(prj, cfg)
    if cfg.pchheader == nil or isFlagSet(cfg, "NoPCH") or cfg.enablepch == "Off" then
        return nil
    end

    local candidates = {}
    appendAll(candidates, cfg.includedirs)
    if cfg.pchsource ~= nil then
        table.insert(candidates, path.getdirectory(cfg.pchsource))
    end
    table.insert(candidates, prj.basedir)

    for _, dir in ipairs(candidates) do
        local header = path.join(dir, cfg.pchheader)
        if os.isfile(header) then
            return header
        end
    end

    p.warn("export-compile-commands: cannot locate pchheader '%s' of project '%s'", cfg.pchheader, prj.name)
    return nil
end

local function resolveForceInclude(prj, cfg, name)
    for _, dir in ipairs(cfg.includedirs or {}) do
        local header = path.join(dir, name)
        if os.isfile(header) then
            return header
        end
    end
    return path.getabsolute(name, prj.basedir)
end

local function appendBuildOptions(args, options)
    for _, opt in ipairs(options or {}) do
        -- MSVC 选项（/utf-8、/Zc:... 等）对 clang 驱动无意义且会被误识别为输入文件
        if opt:sub(1, 1) ~= "/" then
            table.insert(args, opt)
        end
    end
end

local function appendDefinesAndIncludes(args, prj, cfg)
    for _, def in ipairs(cfg.defines or {}) do
        table.insert(args, "-D" .. def)
    end
    for _, undef in ipairs(cfg.undefines or {}) do
        table.insert(args, "-U" .. undef)
    end
    for _, dir in ipairs(cfg.includedirs or {}) do
        table.insert(args, "-I" .. dir)
    end
    for _, dir in ipairs(cfg.externalincludedirs or cfg.sysincludedirs or {}) do
        table.insert(args, "-isystem")
        table.insert(args, dir)
    end
    for _, name in ipairs(cfg.forceincludes or {}) do
        table.insert(args, "-include")
        table.insert(args, resolveForceInclude(prj, cfg, name))
    end
end

function m.getConfig(prj)
    local name = _OPTIONS["export-compile-commands-config"]
    if name == nil then
        for cfg in p.project.eachconfig(prj) do
            return cfg
        end
        return nil
    end

    local cfg = p.project.getconfig(prj, name)
    if cfg == nil then
        p.error("export-compile-commands: configuration '%s' not found in project '%s'", name, prj.name)
    end
    return cfg
end

function m.getFileArguments(prj, cfg, fcfg, file, isC, pchHeader)
    local args = { isC and "clang" or "clang++" }

    if cfg.system == p.WINDOWS then
        local triple = kWindowsTriples[(cfg.architecture or "x86_64"):lower()]
        if triple ~= nil then
            table.insert(args, "--target=" .. triple)
        end
        table.insert(args, msvcRuntimeLibFlag(cfg))
    end

    if os.host() ~= p.WINDOWS then
        -- 与 MSVC 行为对齐（__debugbreak、__declspec 等），并兜底缺失的 Win32 头文件
        table.insert(args, "-fms-extensions")
        table.insert(args, "-idirafter")
        table.insert(args, m.kWin32StubDir)
    end

    local std = languageStandardFlag(isC and cfg.cdialect or cfg.cppdialect)
    if std ~= nil then
        table.insert(args, std)
    end

    appendDefinesAndIncludes(args, prj, cfg)
    appendDefinesAndIncludes(args, prj, fcfg)

    -- PCH 仅用于 C++ 源文件；pchsource 本身已 #include 了该头文件
    if pchHeader ~= nil and not isC and not isFlagSet(fcfg, "NoPCH") and file ~= cfg.pchsource then
        table.insert(args, "-include")
        table.insert(args, pchHeader)
    end

    appendBuildOptions(args, cfg.buildoptions)
    appendBuildOptions(args, fcfg.buildoptions)

    table.insert(args, "-c")
    table.insert(args, file)
    return args
end

function m.collectProject(prj, entries)
    local cfg = m.getConfig(prj)
    if cfg == nil then
        return
    end

    local pchHeader = findPchHeader(prj, cfg)
    local tree = p.project.getsourcetree(prj)

    p.tree.traverse(tree, {
        onleaf = function(node)
            local ext = path.getextension(node.abspath):lower()
            local isC = kCExtensions[ext] == true
            if not isC and not kCppExtensions[ext] then
                return
            end

            local fcfg = p.fileconfig.getconfig(node, cfg)
            if fcfg == nil or isFlagSet(fcfg, "ExcludeFromBuild") or fcfg.buildaction == "None" then
                return
            end

            table.insert(entries, {
                directory = prj.location,
                file      = node.abspath,
                arguments = m.getFileArguments(prj, cfg, fcfg, node.abspath, isC, pchHeader),
            })
        end,
    })
end

local function jsonString(s)
    local escaped = s:gsub('[%c"\\]', function(c)
        if c == '"' then
            return '\\"'
        elseif c == "\\" then
            return "\\\\"
        elseif c == "\n" then
            return "\\n"
        elseif c == "\r" then
            return "\\r"
        elseif c == "\t" then
            return "\\t"
        end
        return string.format("\\u%04x", c:byte())
    end)
    return '"' .. escaped .. '"'
end

function m.writeEntries(entries)
    p.w("[")
    for i, entry in ipairs(entries) do
        local args = {}
        for _, arg in ipairs(entry.arguments) do
            table.insert(args, jsonString(arg))
        end
        p.w("  {")
        p.w('    "directory": %s,', jsonString(entry.directory))
        p.w('    "file": %s,', jsonString(entry.file))
        p.w('    "arguments": [%s]', table.concat(args, ", "))
        p.w(i < #entries and "  }," or "  }")
    end
    p.w("]")
end

function m.onWorkspace(wks)
    local entries = {}
    for prj in p.workspace.eachproject(wks) do
        m.collectProject(prj, entries)
    end
    p.generate(wks, m.kOutputFile, function()
        m.writeEntries(entries)
    end)
end

newaction {
    trigger     = "export-compile-commands",
    description = "Export a JSON compilation database (compile_commands.json) for clangd",
    onWorkspace = m.onWorkspace,
}

return m
