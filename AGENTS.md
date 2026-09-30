# AGENTS.md

本文件面向 AI 编码助手（以及新加入的开发者），描述在本仓库中工作时**必须知道的事实与约定**。
项目介绍见 [README.md](README.md)。

阅读顺序建议：第 1 节（项目结构）→ 第 4 节（硬性规则）→ 第 3 节（跨平台约定）→ 按任务查第 6 节清单。

## 1. 项目速览

- **Yuicy**：C++20 + OpenGL 4.1 Core 的 2D 引擎（静态库），学习自 Hazel。
- **YuiStudio**：基于 Yuicy + ImGui 的场景编辑器（默认启动项目）。
- **Sandbox**：功能临时验证用的最小应用。
- 依赖：GLFW、GLAD、ImGui(+ImGuizmo)、EnTT、Box2D、Lua + sol2、yaml-cpp、spdlog、glm、stb_image、tinyrefl；测试框架 doctest。

依赖方向（**禁止反向依赖**）：

```
YuiStudio ──▶ YuiStudioCore ──▶ Yuicy ◀── Sandbox
                   ▲              ▲   │
                   │              │   ▼
            YuiStudioTests    YuicyTests    Yuicy/thirdparty（含 doctest）
                   └──────┬───────┘
                          ▼
                 YuicyTestFramework ──▶ Yuicy
```

`YuiStudio` 可执行文件只含入口 `YuiStudioApp.cpp`，其余编辑器代码在静态库 `YuiStudioCore` 中，供测试链接。
`YuicyTestFramework` 位于根目录 `TestFramework/`，不属于引擎；各模块的测试用例放在模块自己的 `tests/` 下。
`Yuicy/src`、`YuiStudio/src`、`Sandbox/src` 不得 include 测试代码与 doctest。

`Yuicy/src/Yuicy/**` 不得 include 任何 `YuiStudio` 的代码；编辑器专属状态不得写入运行时 ECS 组件
（例如锁定/隐藏存放在 `EditorEntityMetadata`，而不是 Component 里）。

引擎内部还有一层方向约束：`Yuicy/src/Yuicy/**`（平台无关）不得依赖 `Yuicy/src/Platform/**`（平台实现），
唯一例外是工厂函数所在的 `.cpp`（见第 3.2 节）。

## 2. 构建与运行

| 事项 | 说明 |
|---|---|
| 可运行平台 | **macOS arm64**（主力开发平台，可编译、运行、测试）；Windows 代码保留但**未经编译验证**，待回归（`MACOS_PORTING_PLAN.md` 阶段 6）；Linux 缺少平台实现。详见第 3 节 |
| 工具链 | C++20，CMake ≥ 3.21。macOS：Apple clang（Xcode Command Line Tools），`APPLE` 下启用 ObjC / ObjC++（`.mm`）；Windows：Visual Studio 2022 / MSVC，`/utf-8` |
| 构建系统 | CMake。根 `CMakeLists.txt` + `Yuicy/`、`YuiStudio/`、`Sandbox/`、`TestFramework/` 各一份子脚本（两个 `tests/` 下另有测试工程的脚本）；第三方 target 在 `cmake/ThirdParty.cmake`，公共设置在 `cmake/YuicyHelpers.cmake` |
| 预设 | `CMakePresets.json`：`debug` / `release`（Unix Makefiles，macOS / Linux）、`vs2022`（Windows）。构建目录 `build/<preset>/`（git 忽略） |
| 常用命令 | `cmake --preset debug` → `cmake --build --preset debug [--target YuiStudio]`；运行用 `--target run-YuiStudio`（工作目录为工程目录）。Windows：`cmake --preset vs2022` 后打开 `build/vs2022/Yuicy.sln`，或 VS2022 直接打开文件夹 |
| 单元测试 | 构建后 `ctest --preset debug`（Windows：`vs2022-debug`）。按模块 / 标签过滤：`-R "^YuicyTests\."`、`-L Scene`；单独调试：`build/debug/bin/YuicyTests --test-case="<名称>"`。`-DYUICY_BUILD_TESTS=OFF` 可关闭测试工程。详见第 6.6 节 |
| 编译数据库 | Makefile 预设配置时自动生成 `build/<preset>/compile_commands.json`，并在根目录建立符号链接（git 忽略，含绝对路径需本机生成） |
| 子模块 | `git submodule update --init --recursive` |
| 配置 | `Debug`（`YUICY_PROFILE_DEBUG`、断言开启）/ `Release`（`NDEBUG`，由 CMake 自动定义） |
| MSVC 运行时 | 全部工程与第三方库统一静态 CRT（`/MT`、`/MTd`），由根脚本 `CMAKE_MSVC_RUNTIME_LIBRARY` 控制，不要在单个 target 上改 |
| 输出 | 可执行文件 `build/<preset>/bin/`（VS 多配置为 `bin/<Config>/`），静态库 `build/<preset>/lib/` |
| 工作目录 | 工程目录（`YuiStudio/`、`Sandbox/`），资源以 `assets/...` 相对路径加载；VS 通过 `VS_DEBUGGER_WORKING_DIRECTORY` 设置 |

新增 `.h/.cpp/.mm` 无需修改 CMake 脚本（`src/**` 通配 + `CONFIGURE_DEPENDS`，下次构建时自动重新配置）；
`Platform/` 下按目录区分参与编译的平台，见第 3.1 节。
新增第三方库：放入 `Yuicy/thirdparty/`，在 `cmake/ThirdParty.cmake` 中声明 target（上游有 CMakeLists 优先 `add_subdirectory`，
并关闭其 tests / examples / install 选项），加入 `YUICY_THIRDPARTY_TARGETS`，再由 `Yuicy/CMakeLists.txt` 链接。

## 3. 跨平台约定（改动平台相关代码前必读）

**现状**：

| 平台 | 状态 |
|---|---|
| macOS arm64 | 可编译、运行、测试；OpenGL 4.1 Core，使用系统标题栏 |
| Windows | 代码保留，**未经编译验证**；回归清单见 `MACOS_PORTING_PLAN.md` 阶段 6 |
| Linux | 能识别平台，但缺少窗口、文件对话框、Shell 集成的实现（见第 3.4 节 D） |

**目标**：保持向其他操作系统及其他渲染后端扩展的可能性。

因此判断标准不是"能不能在当前平台上跑通"，而是：**平台专属代码是否被关在 `Platform/` 里，
平台无关层是否只依赖抽象接口**。第 3.4 节列出的既有违例属于**技术债**，是反例，不是可以照抄的先例。

### 3.1 Platform/ 的职责边界

`Yuicy/src/Platform/` 是**唯一**允许出现操作系统 API 与图形后端 API 的地方：

| 目录 | 现有文件 | 允许出现的东西 | 参与编译 |
|---|---|---|---|
| `Platform/GLFW/` | `GlfwWindow.{h,cpp}`、`GlfwInput.cpp` | GLFW | 所有平台 |
| `Platform/Windows/` | `WindowsWindow.{h,cpp}`、`WindowsFileDialogs.cpp`、`WindowsPlatformUtils.cpp` | Win32 API（`<Windows.h>`、`<windowsx.h>`、`<commdlg.h>`、`<shellapi.h>`）、GLFW（含 `glfw3native.h`） | 仅 Windows |
| `Platform/MacOS/` | `MacOSFileDialogs.mm`、`MacOSPlatformUtils.mm` | Cocoa / AppKit / UniformTypeIdentifiers、Objective-C 语法（只在 `.mm` 中）、`glfwGetCocoaWindow` | 仅 macOS |
| `Platform/OpenGL/` | `OpenGLBuffer` / `OpenGLContext` / `OpenGLDebug` / `OpenGLFramebuffer` / `OpenGLRendererAPI` / `OpenGLShader` / `OpenGLTexture` / `OpenGLVertexArray`（各 `.h`+`.cpp`） | `<glad/glad.h>`、`gl*` 调用、GLFW（上下文创建与交换） | 所有平台 |

"参与编译"由 `cmake/YuicyHelpers.cmake` 的 `yuicy_collect_sources` 控制：非 Windows 排除 `Platform/Windows/`，非 Apple 排除 `Platform/MacOS/` 与所有 `.mm`。
新增平台目录时在那里补充排除规则；只依赖 GLFW、各平台通用的实现放进 `Platform/GLFW/`，不要放进某个操作系统的目录。

反过来，以下位置**不应**出现 Win32 头文件与类型（`HWND`、`LRESULT`、`WNDPROC`、`HANDLE`）、Cocoa 头文件与 Objective-C 代码、
GLFW 头文件与 `glfw*` 调用（含 `glfwGetWin32Window` / `glfwGetCocoaWindow`）、裸 `gl*` 调用，也不应 include `Platform/**` 的头文件
（工厂函数所在的 `.cpp` 除外，见第 1 节）：

- `Yuicy/src/Yuicy/**`（引擎平台无关层）
- `YuiStudio/src/**`、`Sandbox/src/**`（客户端）

### 3.2 现有平台分发机制（新增实现必须沿用其中一种）

| 抽象 | 平台无关接口 | 现有实现 | 分发方式 |
|---|---|---|---|
| 窗口 | `Yuicy/Core/Window.h`（纯虚） | Windows：`Platform/Windows/WindowsWindow`；macOS：`Platform/GLFW/GlfwWindow` | 编译期：`Window::Create`（`Window.cpp`）中按 `PLATFORM_WINDOWS` / `PLATFORM_MACOS` 选择，`#else` 断言并返回 `nullptr` |
| 输入 | `Yuicy/Core/Input.h`（静态类，无虚函数） | `Platform/GLFW/GlfwInput.cpp` | 链接期：平台实现文件提供静态成员函数的定义 |
| 文件对话框 | `Yuicy/Core/FileDialogs.h`（静态类） | `Platform/Windows/WindowsFileDialogs.cpp`、`Platform/MacOS/MacOSFileDialogs.mm` | 链接期 |
| Shell 集成 | `Yuicy/Core/PlatformUtils.h`（静态类） | `Platform/Windows/WindowsPlatformUtils.cpp`、`Platform/MacOS/MacOSPlatformUtils.mm` | 链接期 |
| 渲染指令 | `Yuicy/Renderer/RendererAPI.h`（纯虚） | `Platform/OpenGL/OpenGLRendererAPI` | 运行时工厂：`RendererAPI::Create()` 中 `switch (s_API)`；`RenderCommand` 以 `Scope<RendererAPI>` 持有 |
| 渲染资源 | `Buffer` / `VertexArray` / `Shader` / `Texture` / `Framebuffer`（`Yuicy/Renderer/`） | `Platform/OpenGL/OpenGL*` | 运行时工厂：各 `Create()` 中 `switch (Renderer::GetAPI())` |
| 图形上下文 | `Yuicy/Renderer/GraphicsContext.h` | `Platform/OpenGL/OpenGLContext` | 运行时工厂：`Create()` 与 `SetWindowHints()`（创建原生窗口前调用）中 `switch (Renderer::GetAPI())` |
| 调试断点 | `YUICY_DEBUGBREAK()`（`Yuicy/Core/Assert.h`） | — | 按**编译器**分支：`_MSC_VER` → `__debugbreak()`，`__GNUC__` → `raise(SIGTRAP)` |

**平台宏**：`PLATFORM_WINDOWS` / `PLATFORM_MACOS` / `PLATFORM_LINUX` 由 `Yuicy/Core/PlatformDetection.h` 根据编译器预定义宏
（`_WIN32`、`__APPLE__` + `TARGET_OS_OSX`、`__linux__`）推导，经 `pch.h` → `Log.h` → `Core.h` 引入；非 macOS 的 Apple 平台与未知平台直接 `#error`。
判断操作系统一律用 `PLATFORM_*`，不要在 `PlatformDetection.h` 以外直接写 `_WIN32` / `__APPLE__`。
Windows 上 `Yuicy/CMakeLists.txt` 还会以 PUBLIC 定义 `PLATFORM_WINDOWS`，属于技术债（见第 3.4 节 A）。

链接期分发的平台缺少实现时会在链接期报错，这是预期行为：不要为了让某个平台链接通过而补一个静默返回成功的空实现。

渲染后端的运行时工厂（上表"运行时工厂"几行）是目前抽象质量最好的部分：新增一个渲染后端只需加 `enum` 值、
在各 `switch` 里加 `case`、并补一套 `Platform/<Backend>/` 实现。**新增渲染资源类型请保持这个模式。**

### 3.3 新增平台相关能力时的硬性要求

只要功能涉及"不同操作系统/后端实现不同"（文件对话框、剪贴板、Shell 集成、进程、动态库加载、
高精度计时、线程优先级、字体枚举、渲染后端调用……），必须：

- [ ] **先定接口**：OS 能力放 `Yuicy/src/Yuicy/Core/`，图形能力放 `Yuicy/src/Yuicy/Renderer/`。
      接口签名只允许出现标准库类型、`glm`、`Ref<T>`/`Scope<T>` 与本项目自有类型；平台句柄一律以 `void*` 出入
      （参考 `Window::GetNativeWindow()`）。
- [ ] **实现进 Platform/**：新建 `Platform/<Windows|MacOS|GLFW|OpenGL|…>/<Platform><Feature>.{h,cpp,mm}`，
      命名沿用 `WindowsXxx` / `MacOSXxx` / `GlfwXxx` / `OpenGLXxx`。完整范例：`Core/FileDialogs.h` + `WindowsFileDialogs.cpp` / `MacOSFileDialogs.mm`。
- [ ] **接入分发点**：按第 3.2 表选一种既有模式（编译期 / 链接期 / 运行时工厂），不要发明第四种。
- [ ] **`#ifdef` 必须有 `#else` 兜底**：给出 `YUICY_CORE_ASSERT(false, "Unknown platform!")` 并返回 `nullptr`，
      或显式 `#error`；**不要静默返回成功**，否则移植时会变成难查的运行期错误。链接期分发的要求见第 3.2 节末尾。
- [ ] **平台实现的头文件不得泄漏平台类型**：`<Windows.h>`、Cocoa 头文件等只在 `.cpp` / `.mm` 内 include，
      头文件需要平台成员时用前向声明或 pimpl（`Platform/Windows/WindowsWindow.h` 是现存反例）。
- [ ] **Objective-C 只写在 `Platform/MacOS/*.mm`**：头文件保持纯 C++；Cocoa 对象由 ARC 管理（`-fobjc-arc`）；
      用到 AppKit 以外的 framework 时，在 `Yuicy/CMakeLists.txt` 的 `if(APPLE)` 中链接。
- [ ] **不要向任何 `pch.h` 添加平台专属 include**。
- [ ] **编译器差异用编译器宏**（`_MSC_VER` / `__GNUC__` / `__clang__`），不要用 `PLATFORM_WINDOWS` 代替，
      参考 `Assert.h` 的写法。

同时避开这些常见的隐式平台绑定：

- 不用 MSVC 专属 CRT：`localtime_s`、`strncpy_s`、`sprintf_s`、`fopen_s`、`_dupenv_s` 等，
  改用标准写法；确需平台差异时按 `_MSC_VER` 分支封装成小函数（参考 `AssetInspectorPanel.cpp` 的 `ToLocalTime`）。
- 路径一律用 `std::filesystem::path` 与 `/`，不要手拼 `\\`、不要假设 `MAX_PATH`、不要假设路径是 `char` 窄字符串。
  路径需要转成字节串传递时（如 ImGui 拖拽 payload）用 UTF-8（`path::u8string()`，参考 `YuiStudio/src/Utils/ContentBrowserDragDrop.h`）。
- 文件名大小写必须与 `#include` 里写的完全一致。macOS 与 Windows 的默认文件系统都不区分大小写，
  本机编译通过不能证明大小写正确，Linux 上会直接编译失败。
- 换行、文本编码按第 4 节第 3 条（UTF-8 无 BOM + LF）。

### 3.4 已知技术债（反例清单）

**下列代码不代表本项目认可的做法**。若任务恰好需要改动这些文件，优先顺手把它收进抽象；
若任务与之无关，不要夹带重构（见第 3.5 节）。Windows 侧各项的处理计划见 `MACOS_PORTING_PLAN.md` 阶段 6。

**A. 平台无关层与客户端的抽象泄漏**

| 位置 | 问题 |
|---|---|
| `Yuicy/src/pch.h`、`YuiStudio/src/pch.h` | `#ifdef PLATFORM_WINDOWS` → `#include <Windows.h>`，把 Win32 注入 Windows 上的每个翻译单元，会掩盖隐式依赖 Win32 的代码。该 `#ifdef` 位于所有 include 之前，只能拿到 CMake 定义的宏 |
| `Yuicy/CMakeLists.txt` | Windows 上以 PUBLIC 定义 `PLATFORM_WINDOWS`，与 `PlatformDetection.h` 形成两个来源；删除前须先处理上一行的 `pch.h`，否则 `<Windows.h>` 会被静默跳过 |
| `Yuicy/src/Yuicy/ImGui/ImGuiLayer.cpp` | include `<GLFW/glfw3.h>`，直接使用 ImGui 的 GLFW / OpenGL3 后端与 `glfwGetCurrentContext` / `glfwMakeContextCurrent`；用 `#ifndef PLATFORM_MACOS` 在 macOS 上关闭多视口 |
| `Yuicy/src/Yuicy/Renderer/Renderer.cpp` | `Renderer::Submit` 把 `Shader` 用 `dynamic_pointer_cast` 转成 `OpenGLShader` 来上传 uniform |
| `Sandbox/src/Sandbox.cpp`、`Sandbox/src/Sandbox2D.cpp` | 客户端 include `Platform/OpenGL/OpenGLShader.h`，`Sandbox.cpp` 直接调用 `OpenGLShader` 的方法 |

**B. Platform/ 内部的问题**

| 位置 | 问题 |
|---|---|
| `Platform/Windows/WindowsWindow.h` | 平台实现的**头文件**里 `#include <Windows.h>` 并暴露 `HWND` / `LRESULT` / `WNDPROC` 成员 |
| `Platform/Windows/WindowsWindow.cpp` | 与 `GlfwWindow` 大部分代码重复；没有调用 `GraphicsContext::SetWindowHints()`，Windows 上拿到的是驱动默认上下文而不是 4.1 Core |
| `Platform/Windows/WindowsFileDialogs.cpp` | 使用 `A` 版本 API（`GetOpenFileNameA` / `GetSaveFileNameA`），非 ASCII 路径会出错 |
| `Platform/Windows/WindowsPlatformUtils.cpp` | `RevealInFileBrowser` 传入文件时会用默认程序打开该文件，与接口契约（打开所在目录并选中）不符 |

**C. 渲染后端的结构性限制（非"加一个 Platform 实现"能解决）**

- OpenGL 基线是 **4.1 Core + forward-compatible**（macOS 的上限）：不能使用 DSA 及其他 4.2+ API，规则见第 6.5 节。
- 纹理槽固定为 16：`Renderer2D.cpp` 的 `MaxTextureSlots` 与 `YuiStudio`、`Sandbox` 两份 `assets/shaders/Texture.glsl` 中的数组大小、`switch` 必须同步修改。
- forward-compatible 上下文的最大线宽为 1px，`OpenGLRendererAPI::SetLineWidth` 会按上下文能力截断。
- `glDebugMessageCallback` 需要 GL 4.3，macOS 上不可用；Debug 下由 `Platform/OpenGL/OpenGLDebug.h` 的 `OpenGLCheckErrors`
  在纹理创建、`Framebuffer::Invalidate`、着色器编译后以及每帧 `SwapBuffers` 时检查 `glGetError`。
- `ImGui::Image` 直接把 GL 纹理 ID 当作 `ImTextureID`（如 `EditorViewportPanel.cpp`），`Shader` 按名字上传 uniform：接入非 OpenGL 后端前都需要重新设计。
- 自定义标题栏只有 Windows 实现：`WindowsWindow.cpp` 用 Win32 `WndProc` 子类化拦截 `WM_NCHITTEST`，这是**合理的平台实现位置**；
  macOS 使用系统标题栏，`Window::HasCustomTitleBar()` 返回 `false`，编辑器据此不绘制窗口按钮。

**D. Linux 缺少的实现**

- `Window::Create` 没有 `PLATFORM_LINUX` 分支（断言后返回 `nullptr`；`GlfwWindow` 本身可以复用）。
- `FileDialogs` / `PlatformUtils` 没有 Linux 实现，链接期报错。

### 3.5 跨平台工作方式

- 在 macOS 上改完代码要在本机完成验证：`cmake --build --preset debug && ctest --preset debug`；
  涉及运行期行为时用 `cmake --build --preset debug --target run-YuiStudio` 启动检查。
- **Windows 代码在 macOS 上不参与编译**。改动 `Platform/Windows/**`、`_MSC_VER` 分支，或共享代码中可能受 MSVC 影响的写法时，
  必须在结果中明确说明"Windows 未经编译验证"，并列出需要人工在 Windows 上验证的点。
- Windows 回归、Linux 支持、新渲染后端要作为**明确的任务**来做（Windows 按 `MACOS_PORTING_PLAN.md` 阶段 6 推进）。
  **不要在无关任务里夹带平台重构**：修一个面板 Bug 时不要顺手改 CMake 脚本或渲染后端。
- 需要语义导航（跳转定义 / 查引用）时优先用 clangd / LSP 而非文本搜索；
  根目录没有 `compile_commands.json` 时，先运行 `cmake --preset debug`。
- `Platform/Windows/**` 不在 macOS 的编译数据库中，clangd 在这些文件里报找不到 `<Windows.h>` 等错误**属预期**；
  不要为了消除报错而修改 CMake 或手动定义 `PLATFORM_WINDOWS`。

## 4. 硬性规则（违反即视为错误）

1. **不要修改** `Yuicy/thirdparty/**` 与 `Yuicy/src/Yuicy/ImGui/ImGuizmo.*`；不要升级/切换子模块版本，除非任务明确要求。
2. 带 PCH 的工程（Yuicy、YuiStudioCore、YuicyTests、YuiStudioTests）中，每个 `.cpp` / `.mm` 的**第一行**必须是 `#include "pch.h"`（`ImGuizmo.cpp` 除外）。
   `.mm` 不使用预编译头（`yuicy_configure_target` 统一设置 `SKIP_PRECOMPILE_HEADERS`），`pch.h` 在其中按普通头文件展开。
   `YuicyTestFramework`（`TestFramework/`）没有 PCH。
3. 所有源文件使用 **UTF-8（无 BOM）+ LF**；注释用简体中文，格式与内容要求见第 8 节。
4. **平台专属代码（OS API / 图形后端 API）只能写在 `Yuicy/src/Platform/` 下，并通过第 3.2 节的既有分发机制接入**；
   平台无关层与客户端代码一律走抽象接口。详见第 3 节。
5. 编辑器中**结构性修改**（实体增删、组件增删、父子关系变更、Gizmo 变换）必须通过
   `EditorCommandHistory::ExecuteCommand` 执行一个 `IEditorCommand`，以支持 Undo/Redo。
   属性面板里的字段编辑目前直接修改组件，**任何修改场景数据的路径都必须调用** `EditorDirtyTracker::MarkSceneDirty()`。
6. 资源引用使用 `AssetHandle`（UUID），不要在组件中存储文件路径；资源的增删改走 `EditorAssetManager` / `EditorAssetWorkflow`。
7. 日志用 `YUICY_CORE_*`（引擎内）/ `YUICY_*`（客户端），断言用 `YUICY_CORE_ASSERT` / `YUICY_ASSERT`；不要使用 `std::cout` / `printf`。
8. 智能指针用 `Ref<T>` / `Scope<T>` 与 `CreateRef` / `CreateScope`（`Yuicy/Core/Base.h`）。
9. 不要批量重排格式；只格式化自己改动的代码（见第 7 节）。
10. 不要提交构建产物（`build/`）与运行时生成的文件（`imgui.ini`、`Profiles-*.json`、`*.log` 均已被 git 忽略）。
    默认窗口布局是各工程目录下的 `imgui.default.ini`，本地没有 `imgui.ini` 时由 `ImGuiLayer` 复制；只有任务明确要求调整默认布局时才修改它。

## 5. 目录地图

```
Yuicy/src/
  pch.h / pch.cpp          # 引擎预编译头（勿加平台专属 include）
  Yuicy.h                  # 对客户端（YuiStudio/Sandbox）暴露的总头文件
  Yuicy/Core/              # Application, Layer/LayerStack, Log, Assert, UUID, Input, Window, Base(Ref/Scope), Core.h(YUICY_API)
                           #   平台抽象：PlatformDetection.h(PLATFORM_*), FileDialogs, PlatformUtils
  Yuicy/Events/            # 事件系统（Application/Key/Mouse）
  Yuicy/Renderer/          # Renderer2D, RenderCommand/RendererAPI, GraphicsContext, Shader, Texture, Framebuffer, Camera, SortingLayer
  Yuicy/Scene/             # Scene, Entity, Components.h, SceneSerializer, SceneCamera, ContactListener
  Yuicy/Asset/             # Asset, AssetHandle, AssetRegistry, EditorAssetManager, AssetImporter, 扩展名映射
  Yuicy/Project/           # Project(.yproj) 与序列化，SortingLayers / CollisionLayers 配置
  Yuicy/Physics/           # Box2D 封装, CollisionLayerConfig
  Yuicy/Scripting/         # LuaScriptEngine, LuaBindings（sol2）
  Yuicy/ImGui/             # ImGuiLayer，ImGuizmo（外部拷贝代码，勿改）
  Yuicy/Debug/             # Instrumentor（YUICY_PROFILE_*）
  Platform/OpenGL/         # 渲染后端实现：RendererAPI / Buffer / VertexArray / Shader / Texture / Framebuffer / Context / Debug
  Platform/GLFW/           # 各平台通用的 GLFW 实现：GlfwWindow / GlfwInput
  Platform/Windows/        # Windows 实现：WindowsWindow（含 Win32 WndProc 标题栏）/ FileDialogs / PlatformUtils
  Platform/MacOS/          # macOS 实现（ObjC++ .mm）：FileDialogs / PlatformUtils
Yuicy/thirdparty/          # 第三方依赖（子模块或拷贝源码），勿改；doctest 仅供测试使用
Yuicy/tests/               # YuicyTests：引擎单元测试，目录结构对应 Yuicy/src/Yuicy/（Scene/、Asset/、Project/…）
  data/                    #   只读测试数据（如参考场景 Scenes/Reference.yui），代码中经 YUICY_TEST_DATA_DIR 访问

YuiStudio/src/             # 除入口外全部编译进静态库 YuiStudioCore
  pch.h / pch.cpp          # 编辑器预编译头
  YuiStudioApp.cpp         # 入口（唯一属于 YuiStudio 可执行文件的源文件）
  EditorLayer.*            # 编辑器主 Layer：组装各服务与面板、快捷键
  Editor/                  # 编辑器服务层（无 UI）
    EditorContext.h        #   全局共享状态：场景、运行模式、文档、选择、视口、实体元数据
    EditorCommandHistory.* #   Undo/Redo 栈
    Commands/              #   IEditorCommand 具体实现 + EntitySnapshot
    EditorSceneController.*#   新建/打开/保存 场景与项目，Play/Simulate/Stop
    EditorAssetWorkflow.*  #   文件与资源相关操作
    EditorDirtyTracker.*   #   脏标记与 AutoSave
    EditorRenderPipeline.* / EditorOverlayRenderer.*  # 视口渲染与叠加绘制（选中框、碰撞体等）
  Panels/                  # ImGui 面板：Viewport, SceneHierarchy, Properties, ContentBrowser, AssetInspector
    ComponentEditors/      #   各组件在 Properties 面板中的编辑 UI
  Utils/
YuiStudio/tests/           # YuiStudioTests：编辑器单元测试（Command、Undo/Redo、EntitySnapshot）
YuiStudio/assets/          # 编辑器资源（shaders、textures、fonts）
YuiStudio/imgui.default.ini  # 默认窗口布局（Sandbox/ 下同样有一份）；运行时读写的 imgui.ini 被 git 忽略

TestFramework/YuicyTest/   # YuicyTestFramework：各测试工程共用的测试 main、日志初始化、夹具（临时目录 / 活动项目 / 场景）、glm 向量的近似比较与打印

CMakeLists.txt                 # 根构建脚本：全局设置（C++20、静态 CRT、输出目录）、YUICY_BUILD_TESTS / enable_testing，并引入子目录
CMakePresets.json              # 构建 / 测试预设（debug / release / vs2022）
cmake/
  ThirdParty.cmake             # 第三方依赖 target（GLFW / Box2D / yaml-cpp / doctest 用上游 CMake，GLAD / imgui / lua / 纯头文件库在此声明）
  YuicyHelpers.cmake           # 自有工程公共函数：源文件收集、编译选项、调试工作目录与 run-<target>
  YuicyTesting.cmake           # yuicy_add_test()：创建 <Module>Tests 可执行文件并把每个用例注册到 CTest
Yuicy/CMakeLists.txt、YuiStudio/CMakeLists.txt、Sandbox/CMakeLists.txt、TestFramework/CMakeLists.txt  # 各工程的 target 定义
```

资源扩展名：场景 `.yui`、项目 `.yproj`（均为 YAML 文本），脚本 `.lua`，着色器 `.glsl`。

## 6. 高频改动清单

### 6.1 新增一个 ECS 组件

以 `CircleCollider2DComponent` / `AnimationComponent` 为参照，逐项检查：

- [ ] `Yuicy/src/Yuicy/Scene/Components.h`：定义结构体，提供默认构造与拷贝构造；运行时指针（如 `RuntimeBody`）置 `nullptr`。
- [ ] `Yuicy/src/Yuicy/Scene/Scene.cpp`：`Scene::Copy` 中加入 `CopyComponent<T>`；如有运行时逻辑，加入对应 `OnUpdate*` / `OnRuntimeStart` 等。
- [ ] `Yuicy/src/Yuicy/Scene/SceneSerializer.cpp`：**序列化与反序列化两处都要加**，YAML 键名与组件名一致。
- [ ] `YuiStudio/src/Editor/Commands/EntitySnapshot.h`：`Capture` 与 `Restore` 两处都要加（运行时指针需清空），否则删除实体后 Undo 会丢数据。
- [ ] `YuiStudio/src/Panels/PropertiesPanel.cpp`：`DrawAddComponentEntry<T>` 与 `DrawComponentUI<T>`。
- [ ] 如 UI 较复杂：在 `YuiStudio/src/Panels/ComponentEditors/` 新建 `XxxEditor.{h,cpp}`。
- [ ] 需要脚本访问时：`Yuicy/src/Yuicy/Scripting/LuaBindings.cpp` 注册 usertype 与 `Entity` 的 `GetXxx` / `HasXxx`。
- [ ] 需要可视化时：`YuiStudio/src/Editor/EditorOverlayRenderer.cpp`。
- [ ] 测试：`Yuicy/tests/Scene/SceneSerializerTests.cpp` 中覆盖全部字段的 round-trip 用例（字段取非默认值）、
      `Yuicy/tests/data/Scenes/Reference.yui` 参考场景，以及 `YuiStudio/tests/Editor/EntitySnapshotTests.cpp` 都要加上新组件。

### 6.2 新增一个可撤销的编辑器操作

- [ ] 在 `YuiStudio/src/Editor/Commands/` 新建 `XxxCommand.h`，继承 `IEditorCommand`，实现 `Execute` / `Undo` / `GetName`。
- [ ] 命令中通过 **UUID** 保存实体引用，执行时用 `Scene::FindEntityByUUID` 取回（`entt::entity` 在删除/恢复后会失效）。
- [ ] 连续操作（如拖拽）只产生一条历史：参考 `EditorViewportPanel` 中 Gizmo 的做法，在拖拽开始时记录旧值、结束时提交一条 `SetTransformCommand`；
      或实现 `GetCommandID` + `TryMerge`，由 `EditorCommandHistory` 自动合并。
- [ ] 调用方使用 `m_commandHistory->ExecuteCommandT<XxxCommand>(...)`，并标记脏状态。
- [ ] 在 `YuiStudio/tests/Editor/EditorCommandsTests.cpp` 补用例：Undo 后场景回到 Execute 之前的状态，Redo 后与 Execute 之后一致；实现了 `TryMerge` 时覆盖合并。

### 6.3 新增一个编辑器面板

- [ ] `YuiStudio/src/Panels/XxxPanel.{h,cpp}`，状态从 `EditorContext` 读写，不在面板间互相持有指针。
- [ ] 在 `EditorLayer` 中创建、注入依赖（Context / CommandHistory / DirtyTracker）并在 `OnImGuiRender` 中绘制。

### 6.4 新增一个平台相关能力

见第 3.3 节的完整清单。最小要求：接口在 `Yuicy/Core/` 或 `Yuicy/Renderer/`，
实现在 `Platform/<Platform>/`，通过第 3.2 节的既有分发机制接入，`#ifdef` 带 `#else` 兜底。

### 6.5 新增一个渲染资源类型

- [ ] 在 `Yuicy/src/Yuicy/Renderer/` 定义纯虚接口与静态 `Create(...)` 工厂。
- [ ] 工厂内 `switch (Renderer::GetAPI())`：`API::None` 走断言返回 `nullptr`，`API::OpenGL` 返回 `CreateRef<OpenGLXxx>(...)`
      （参照 `Framebuffer.cpp` / `Texture.cpp`）。
- [ ] 在 `Platform/OpenGL/` 实现 `OpenGLXxx.{h,cpp}`，`gl*` 调用只出现在这里。
- [ ] 只使用 GL 4.1 Core 以内的 API：不用 DSA 与 4.2+ 的函数（`glCreate*`、`glTexture*`、`glTexStorage*`、`glBindTextureUnit`、`glClearTexImage`、`glNamed*`），
      着色器写 `#version 410 core`（见 3.4 C）。
- [ ] 绑定式写法会改动全局绑定状态：创建 / 上传后解绑，临时绑定其他对象时结束后恢复原绑定（参照 `OpenGLTexture.cpp`、`OpenGLFramebuffer.cpp`）。
      `GL_ELEMENT_ARRAY_BUFFER` 属于当前 VAO 的状态，不要在可能有 VAO 绑定时借它上传数据（参照 `OpenGLIndexBuffer`）。
- [ ] Debug 下在创建与上传之后调用 `OpenGLCheckErrors("<位置>")`（`Platform/OpenGL/OpenGLDebug.h`）。

### 6.6 编写单元测试

结构参考 O3DE：被测模块是静态库，每个模块在自己目录下有 `tests/` 并产出一个 `<Module>Tests`，各测试工程共用的代码放在 `YuicyTestFramework`。

| 被测代码 | 放在 | 链接 |
|---|---|---|
| `Yuicy/src/Yuicy/<Dir>/` | `Yuicy/tests/<Dir>/XxxTests.cpp` | `YuicyTests` → `Yuicy` |
| `YuiStudio/src/<Dir>/` | `YuiStudio/tests/<Dir>/XxxTests.cpp` | `YuiStudioTests` → `YuiStudioCore` |

- [ ] 新文件放进对应目录即可，无需改 CMake（`CONFIGURE_DEPENDS` 通配）；首行 `#include "pch.h"`，测试工程的 `pch.h` 已包含 `YuicyTest/YuicyTest.h`。
- [ ] 用例放进 `TEST_SUITE("<模块>")`，现有：`Scene` / `Asset` / `Project` / `Editor`。`TEST_SUITE` 名会成为 CTest 标签（`ctest -L Scene`）。
- [ ] 用例名用英文短句描述被验证的行为，它同时是 CTest 测试名与 `--test-case=` 的过滤参数；不要含逗号（`--test-case=` 用逗号分隔多个过滤条件）。
- [ ] 需要场景时用 `TEST_CASE_FIXTURE(Test::SceneFixture, ...)`；只需要活动项目或临时文件时用 `Test::ScopedActiveProject` / `Test::ScopedTempDirectory`。
      **不要**把文件写进源码树或工作目录，也不要依赖工作目录下的资源。
- [ ] 浮点向量比较用 `CHECK(v == Test::ApproxVec(expected))`，标量用 `doctest::Approx`，失败时会打印两侧的值。
- [ ] 只读测试数据放 `Yuicy/tests/data/`，路径以 `YUICY_TEST_DATA_DIR` 开头拼接。
- [ ] **不能创建窗口与 GL 上下文**：不调用 `Renderer2D` / `Texture2D::Create` / `Shader::Create`。
      组件里的 `AssetHandle` 只填未在资产注册表中登记的值，这样反序列化 `AnimationComponent` 时不会加载纹理，`Frames` 中对应元素为 `nullptr`。
      需要 GPU 时等 Headless 后端（`AI_INFRA_ROADMAP.md` 2.3）。
- [ ] 活动项目是 `Project` 的静态成员，用例之间会互相影响：只通过 `SceneFixture` / `ScopedActiveProject` 设置，不要直接调用 `Project::SetActive`；两者都不能嵌套使用。
- [ ] 发现被测代码有 Bug 但不在本次任务内修复时，写一个复现用例并加 `* doctest::should_fail()`，上方一行注释说明问题，
      再加 `// TODO: 删除 should_fail（<修复条件>）`。修复后用例中的断言全部通过，doctest 会把它判为失败，提醒删除 `should_fail`。
- [ ] 新增一个模块的测试工程：在 `<Module>/tests/CMakeLists.txt` 调用 `yuicy_add_test(<Module>Tests SOURCE_DIR ... LINK <被测库>)`，
      并在 `<Module>/CMakeLists.txt` 中 `if(YUICY_BUILD_TESTS) add_subdirectory(tests) endif()`；被测模块是可执行文件时先按 `YuiStudio` 的做法拆出 `<Module>Core` 静态库。
- [ ] 排查时：`YUICY_TEST_LOG_LEVEL=trace build/debug/bin/YuicyTests --test-case="<名称>"` 输出全部引擎日志（默认只输出 warn 及以上）。
      引擎的 `.cpp` 在 Debug 与 Release 下都开启 `YUICY_CORE_ASSERT`，断言失败会调用 `YUICY_DEBUGBREAK()`，没有挂调试器时测试进程直接退出，
      CTest 报告的是进程异常退出而不是 doctest 断言失败，此时在输出中找 `Assertion` 开头的错误日志。

## 7. 代码风格

格式由根目录 `.clang-format` 定义（从现有代码反推）；编辑器基础设置见 `.editorconfig`，
排除项见 `.clang-format-ignore`。

- Tab 缩进（宽 4），Allman 花括号；`namespace Yuicy {` 同行且内部整体缩进一级。
- 类型/函数：`PascalCase`；组件公有字段：`PascalCase`；局部变量/参数：`camelCase`。
- 成员变量：`m_` 前缀，**跟随所在模块已有风格**——引擎 Core/Renderer/Scene 多为 `m_PascalCase`，YuiStudio 与 Asset 为 `m_camelCase`；静态成员 `s_`，常量 `k` 前缀（如 `kDefaultMaxStackSize`）。
- 指针/引用贴类型：`const Ref<Texture2D>& texture`。
- 头文件使用 `#pragma once`；能前置声明的不在头文件中 include，重型头文件放到 `.cpp`。
- 只格式化改动部分：

  ```bash
  git clang-format            # 格式化已暂存改动涉及的行
  git clang-format --diff     # 仅预览
  ```

## 8. 代码注释

**总原则：代码说明"怎么做"，注释说明"为什么"，以及代码本身表达不了的信息。**
能用改名、抽函数、具名常量、`enum` 表达的，就不要写注释。

适用范围：`Yuicy/src`、`YuiStudio/src`、`Sandbox/src` 中**新增或修改**的注释。`thirdparty/`、`ImGuizmo.*` 不适用（第 4 节第 1 条）。
存量注释不批量改写，改到哪条就按本节修正哪条（第 4 节第 9 条）。

### 8.1 中文化

- 注释用简体中文。与其写半吊子英文，不如用中文说清楚。
- 标识符、API 名、专有名词保留原文，不要硬译：写 `Framebuffer`、`glTextureStorage2D`、Box2D、UUID、Undo/Redo，
  不要自造"帧缓冲对象""撤销/重做栈"之类的译名。
- 修改某条英文注释的内容时，顺手改成中文；只改代码、没动那条注释时，不要为了翻译单独改它。
- 从外部复制的代码保留原有英文注释和出处链接。

### 8.2 格式统一

| 场景 | 写法 |
|---|---|
| 普通注释 | `//` 独占一行，写在被说明代码的**上方**，与代码同缩进 |
| 多行注释 | 连续多行 `//`；不用 `/* */` 块注释，不用 Doxygen（`///`、`/** */`、`@param`） |
| 行尾注释 | 只用于字段、常量、枚举值的简短说明，同一组内对齐 |
| 接口注释 | 写在 `.h` 的声明上方，说明用途、用法和边界情况；`.cpp` 定义处只写实现原因，**不要复制 `.h` 里的注释**；不要以函数名、类名开头 |
| 实参注释 | 含义不明的字面量实参写成 `/*参数名=*/`，如 `Foo(spec, /*multisample=*/false)`；更好的做法是改用具名常量、`enum` 或参数结构体 |
| 分组标记 | 只在长文件里给大段代码分组时用，而且要少用。统一写成 `// ==================== 物理组件 ====================`（沿用 `Components.h`、`Renderer2D.cpp` 的写法） |
| TODO | `// TODO: <要做什么>（<触发条件或关联 issue>）`，例：`// TODO: 改为读取 RenderCaps（接入第二个渲染后端时）`。禁止不写内容的 `// TODO`（`AssetImporter.cpp` 中的写法是反例）；不使用 `FIXME` / `HACK` / `XXX` 等其他标记 |

排版（依据《中文文案排版指北》）：

- `//` 后空一格；中文与英文、数字之间加一个空格：`// 最多 32 个纹理槽位`、`// 交给 Box2D 处理`。
- 中文句子用全角标点，全角标点前后不加空格；英文片段和代码片段内部用半角。
- 单句注释末尾不加句号；多句注释每句都以句号结尾。
- 专有名词大小写写对：OpenGL、ImGui、Lua、GitHub。

### 8.3 简洁：默认一行说清

- 一条注释只说一件事，默认一行写完。
- 接口契约、复杂算法、不直观的取舍可以写多行，但一般不超过 3 行。超过时先想一想：是不是函数该拆、名字该改，
  或者这些内容该放进文档，注释里只留链接。
- 如果函数体里需要很多分段注释，说明函数太长，应该拆分。
- 不写背景故事和讨论过程，不大段摘抄规范原文；需要引用时只给链接或编号。

### 8.4 应该写的注释

- **为什么**这样做，以及为什么不用看起来更直观的写法。
- 代码表达不了的约束：单位（像素 / 世界单位、弧度 / 角度）、坐标系、取值范围、特殊值的含义（如 `-1` 表示未知、`nullptr` 表示未加载）、调用顺序、生命周期。
- 接口边界：会不会返回 `nullptr`、是否接管所有权或长期持有引用、输出参数是追加还是覆盖、有没有明显的性能代价。
- 警示后果：删掉某行或调换顺序会出什么问题。
- 看起来多余或不合惯例、别人很可能想"顺手简化"的代码。
- 外部出处：抄来或参考的代码、算法、规范，附上链接。

仓库中的正面例子：`EditorOverlayRenderer.h` 的 `// 调用者负责 Renderer2D::BeginScene / EndScene`（调用约定）、
`EditorCommand.h` 的 `// 返回 true 表示合并成功，不再单独入栈`（返回值语义）。

### 8.5 禁止傻瓜注释

傻瓜注释是把代码逐字翻译成中文的注释：读完注释得到的信息，不比读代码多。
**判断方法：删掉这条注释，读者会少知道什么？如果什么都不少，就删掉。**

- 复述函数名或代码：`void Undo();` 上方写 `// 撤销`、`bool CanRedo() const;` 上方写 `// 是否可以重做`
  （`EditorCommandHistory.h` 中现存的反例），`for` 循环上方写 `// 遍历所有实体`。
- 构造函数、析构函数、override 上的套话：`// 构造函数`、`// 默认析构`、`// 重写基类方法`。
- 右括号注释：`} // if`、`} // for`。
- 为了"每个函数都要有注释"而硬凑的注释：名字已经说清楚的函数、简单的 getter / setter 不写注释。
- 用注释弥补糟糕的命名：`Node n; // 最佳候选节点` 应该把变量改名为 `bestNode`，再删掉注释。

```cpp
// 差：复述代码
// 纹理槽索引重置为 1
s_Data.TextureSlotIndex = 1;

// 好：说明代码看不出来的原因
// 0 号槽固定给白色纹理（纯色绘制用），用户纹理从 1 号开始
s_Data.TextureSlotIndex = 1;
```

### 8.6 禁止"东坡肉"式注释

东坡肉式注释记录的是**某一次改动或某一次对话**的上下文，而不是代码**现在**的样子。它在当次改动里看起来合情合理，
但就像不小心放进西红柿炒蛋里的东坡肉：后来的人不知道它为什么在这里，也不敢删，于是它永远留在了代码里。

**判断方法：假设从零重写这段代码，这条注释还会存在吗？不会，就不要写。**
注释只描述代码当前的行为和约束，改动过程写进提交信息（第 9 节）。

禁止的写法：

- 改动记录：`// 新增：`、`// 修改：`、`// 优化：`、`// 重构后`、`// v2`、`// 2026-09-29 改`。
- 新旧对比：`// 原来用 X，现在改成 Y`、`// 不再调用 Foo()`、`// 旧逻辑已删除`。
- 修复经过：`// 修复了拖拽时崩溃的 bug`。**问题本身值得留下，经过不值得**：改写成对现状的约束，
  如 `// Undo 后 entt::entity 会失效，必须按 UUID 重新查找`。
- 任务或对话上下文：`// 按需求调整`、`// 根据用户要求`、`// 如上所述`、`// 第 2 步`、`// 本次只处理 X`。
- 署名和日期：`// Added by xxx`、`// @author`，这些由 `git blame` 负责。
- 注释掉的代码：直接删除，版本库里查得到。确实需要保留的调试代码，用 `#if 0` … `#endif` 包起来，并在上方写一行说明原因。
- 非本地信息：在调用处解释被调函数内部的实现，或者写出由别处决定的值（如 `// 默认 60 帧`）。别处一改，这里就成了错误信息。

改代码时，同时维护受影响的注释：

- 改动让已有注释失效时，必须同步修改或删除。与代码不符的注释比没有注释更糟。
- 仍然正确的已有注释不要删，也不要借机改写。
- 提交前检查 diff：确认没有新增上面禁止的写法，也没有遗留失效的注释。

## 9. 提交规范

沿用已有风格：`<type>(<scope>): <中文描述>`，正文用 `- ` 列出要点。

- type：`feat` / `fix` / `refactor` / `perf` / `docs` / `chore` / `build` / `test`
- scope：模块名，如 `YuiStudio`、`Yuicy`、`Scene`、`Renderer`、`Platform`、`Lua`、`SpriteCommand`、`Infra`
- 分支：`feature/<PascalCase>`，如 `feature/YuiStudio2D`、`feature/AIInfra`

## 10. 交付前自检

当前仓库尚无 CI，提交前至少确认：

- [ ] `cmake --build --preset debug && ctest --preset debug` 全部通过（Windows：`vs2022-debug`）。

- [ ] 改动范围与任务一致，没有顺手重构 / 批量格式化 / 修改第三方代码 / 夹带平台移植改动。
- [ ] 第 4 节硬性规则全部满足；涉及组件时第 6.1 节清单逐项核对。
- [ ] 平台相关代码满足第 3 节：没有在 `Yuicy/src/Yuicy/**` 或 `YuiStudio/src/**` 新增 Win32 / 裸 `gl*` 调用，
      没有新增 MSVC 专属 CRT 函数，路径处理走 `std::filesystem`，新文件名大小写与 `#include` 一致。
- [ ] 新增和修改的注释满足第 8 节：没有傻瓜注释、东坡肉式注释和注释掉的代码，被改动影响的旧注释已同步更新。
- [ ] 本机 Debug 与 Release 均能编译（`cmake --build --preset release`）；涉及 Windows 代码时已注明"Windows 未经编译验证"并列出待人工验证的点（第 3.5 节）。
- [ ] 涉及序列化时：保存 → 重新打开场景，数据一致；涉及编辑操作时：执行 → Undo → Redo 结果一致。
      能用单元测试覆盖的，按第 6.6 节补用例，而不是只做手动验证。
