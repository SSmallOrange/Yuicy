# AGENTS.md

本文件面向 AI 编码助手（以及新加入的开发者），描述在本仓库中工作时**必须知道的事实与约定**。
项目介绍见 [README.md](README.md)。

阅读顺序建议：第 1 节（项目结构）→ 第 4 节（硬性规则）→ 第 3 节（跨平台约定）→ 按任务查第 6 节清单。

## 1. 项目速览

- **Yuicy**：C++20 + OpenGL 4.5 的 2D 引擎（静态库），学习自 Hazel。
- **YuiStudio**：基于 Yuicy + ImGui 的场景编辑器（默认启动项目）。
- **Sandbox**：功能临时验证用的最小应用。
- 依赖：GLFW、GLAD、ImGui(+ImGuizmo)、EnTT、Box2D、Lua + sol2、yaml-cpp、spdlog、glm、stb_image、tinyrefl。

依赖方向（**禁止反向依赖**）：

```
YuiStudio ──▶ Yuicy ◀── Sandbox
                │
                ▼
          Yuicy/thirdparty
```

`Yuicy/src/Yuicy/**` 不得 include 任何 `YuiStudio` 的代码；编辑器专属状态不得写入运行时 ECS 组件
（例如锁定/隐藏存放在 `EditorEntityMetadata`，而不是 Component 里）。

引擎内部还有一层方向约束：`Yuicy/src/Yuicy/**`（平台无关）不得依赖 `Yuicy/src/Platform/**`（平台实现），
唯一例外是工厂函数所在的 `.cpp`（见第 3.2 节）。

## 2. 构建与运行

| 事项 | 说明 |
|---|---|
| 可运行平台 | **目前仅 Windows**（这是现状与待办，不是设计目标，见第 3 节） |
| 工具链 | Visual Studio 2022 / MSVC，C++20，`/utf-8` |
| 生成工程 | `GenerateProject.bat`（经 `premake/premake.ps1` 执行 `premake5 vs2022`），产物 `Yuicy.sln` 已被 git 忽略 |
| premake | 不入库；两端脚本首次运行时下载固定版本（`5.0.0-beta7`）到 `bin/tools/` 并校验 SHA256，可用环境变量 `PREMAKE5` 覆盖。升级版本需**同时**修改 `premake/premake.ps1` 与 `GenerateProject.sh` 中的版本号与哈希 |
| 编译数据库 | Windows：`GenerateProject.bat`；macOS / Linux：`./GenerateProject.sh` → 根目录 `compile_commands.json`（git 忽略，含绝对路径需本机生成）；实现在 `premake/export-compile-commands.lua` |
| 子模块 | `git submodule update --init --recursive` |
| 配置 | `Debug`（`YUICY_PROFILE_DEBUG`、断言开启）/ `Release`（`NDEBUG`） |
| 输出 | `bin/<Config>-x64/<Project>/`，中间文件 `bin/int/...` |
| 工作目录 | `debugdir` 为可执行文件目录，资源以 `assets/...` 相对路径加载 |

新增 `.h/.cpp` 无需修改 `premake5.lua`（`files` 使用 `src/**` 通配），但需要重新运行 `GenerateProject.bat`
（在 macOS / Linux 上是 `./GenerateProject.sh`，用于刷新 `compile_commands.json`）。

## 3. 跨平台约定（改动平台相关代码前必读）

**现状**：只有 Windows + OpenGL 一套后端能编译运行。
**目标**：保持向 macOS / Linux 及其他渲染后端扩展的可能性。

因此判断标准不是"能不能在 Windows 上跑通"，而是：**平台专属代码是否被关在 `Platform/` 里，
平台无关层是否只依赖抽象接口**。第 3.4 节列出的既有违例属于**技术债**，是反例，不是可以照抄的先例。

### 3.1 Platform/ 的职责边界

`Yuicy/src/Platform/` 是**唯一**允许出现操作系统 API 与图形后端 API 的地方：

| 目录 | 现有文件 | 允许出现的东西 |
|---|---|---|
| `Platform/Windows/` | `WindowsWindow.{h,cpp}`、`WindowsInput.cpp` | Win32 API（`<Windows.h>`、`<windowsx.h>`）、GLFW |
| `Platform/OpenGL/` | `OpenGLBuffer` / `OpenGLContext` / `OpenGLFramebuffer` / `OpenGLRendererAPI` / `OpenGLShader` / `OpenGLTexture` / `OpenGLVertexArray`（各 `.h`+`.cpp`） | `<glad/glad.h>`、`gl*` 调用、GLFW |

反过来，以下位置**不应**出现 `#include <Windows.h>` / `<commdlg.h>` / `<shellapi.h>`、Win32 类型
（`HWND`、`LRESULT`、`WNDPROC`、`HANDLE`）、裸 `gl*` 调用、`glfwGetWin32Window`：

- `Yuicy/src/Yuicy/**`（引擎平台无关层）
- `YuiStudio/src/**`、`Sandbox/src/**`（客户端）

> 注意：`Platform/Windows/WindowsInput.cpp` 的实现其实只用到 GLFW、不含任何 Win32 调用，
> 它位于 `Windows/` 目录属于历史归类。新增同类"只依赖 GLFW"的实现时不要再放进 `Windows/`。

### 3.2 现有平台分发机制（新增实现必须沿用其中一种）

| 抽象 | 平台无关接口 | 现有实现 | 分发方式 |
|---|---|---|---|
| 窗口 | `Yuicy/Core/Window.h`（纯虚） | `Platform/Windows/WindowsWindow` | `Window::Create`（`Window.cpp`）中 `#ifdef PLATFORM_WINDOWS` 编译期分发 |
| 输入 | `Yuicy/Core/input.h`（静态类，无虚函数） | `Platform/Windows/WindowsInput.cpp` | 链接期分发：平台 `.cpp` 提供静态成员函数的定义 |
| 渲染指令 | `Yuicy/Renderer/RendererAPI.h`（纯虚） | `Platform/OpenGL/OpenGLRendererAPI` | `RenderCommand.cpp` 中 `s_RendererAPI = new OpenGLRendererAPI`（**目前硬编码**） |
| 渲染资源 | `Buffer` / `VertexArray` / `Shader` / `Texture` / `Framebuffer`（`Yuicy/Renderer/`） | `Platform/OpenGL/OpenGL*` | 各 `Create()` 中 `switch (Renderer::GetAPI())` 运行时枚举分发 |
| 图形上下文 | `Yuicy/Renderer/GraphicsContext.h` | `Platform/OpenGL/OpenGLContext` | `GraphicsContext::Create` **目前硬编码 OpenGL**（switch 被注释掉） |
| 调试断点 | `YUICY_DEBUGBREAK()`（`Yuicy/Core/Assert.h`） | — | 按**编译器**分支：`_MSC_VER` → `__debugbreak()`，`__GNUC__` → `raise(SIGTRAP)` |

**平台宏叫 `PLATFORM_WINDOWS`**（没有 `YUICY_` 前缀），仅在 `premake5.lua` 的 `defines` 中定义，
没有任何头文件从 `_WIN32` 推导它。使用中的位置只有 5 处：两个 `pch.h`、`Core/Core.h`、`Core/Window.cpp`、`Core/EntryPoint.h`。

渲染资源工厂（上表第 4 行）是目前抽象质量最好的部分：新增一个渲染后端只需加 `enum` 值、
在各 `switch` 里加 `case`、并补一套 `Platform/<Backend>/` 实现，业务代码零改动。**新增渲染资源类型请保持这个模式。**

### 3.3 新增平台相关能力时的硬性要求

只要功能涉及"不同操作系统/后端实现不同"（文件对话框、剪贴板、Shell 集成、进程、动态库加载、
高精度计时、线程优先级、字体枚举、渲染后端调用……），必须：

- [ ] **先定接口**：OS 能力放 `Yuicy/src/Yuicy/Core/`，图形能力放 `Yuicy/src/Yuicy/Renderer/`。
      接口签名只允许出现标准库类型、`glm`、`Ref<T>`/`Scope<T>` 与本项目自有类型；平台句柄一律以 `void*` 出入
      （参考 `Window::GetNativeWindow()`）。
- [ ] **实现进 Platform/**：新建 `Platform/<Windows|OpenGL|…>/<Platform><Feature>.{h,cpp}`，命名沿用 `WindowsXxx` / `OpenGLXxx`。
- [ ] **接入分发点**：按第 3.2 表选一种既有模式，不要发明第四种。
- [ ] **`#ifdef` 必须有 `#else` 兜底**：给出 `YUICY_CORE_ASSERT(false, "Unknown platform!")` 并返回 `nullptr`，
      或显式 `#error`；**不要静默返回成功**，否则移植时会变成难查的运行期错误。
- [ ] **平台实现的头文件不得泄漏平台类型**：`<Windows.h>` 等只在 `.cpp` 内 include，
      头文件需要平台成员时用前向声明或 pimpl（`Platform/Windows/WindowsWindow.h` 是现存反例）。
- [ ] **不要向任何 `pch.h` 添加平台专属 include**。
- [ ] **编译器差异用编译器宏**（`_MSC_VER` / `__GNUC__` / `__clang__`），不要用 `PLATFORM_WINDOWS` 代替，
      参考 `Assert.h` 的写法。

同时避开这些常见的隐式平台绑定：

- 不用 MSVC 专属 CRT：`localtime_s`、`strncpy_s`、`sprintf_s`、`fopen_s`、`_dupenv_s` 等，
  改用标准写法或自建小封装。
- 路径一律用 `std::filesystem::path` 与 `/`，不要手拼 `\\`、不要假设 `MAX_PATH`、不要假设路径是 `char` 窄字符串。
- 文件名大小写必须与 `#include` 里写的完全一致（Linux 文件系统区分大小写；仓库现存 `Core/input.h`
  与 `#include "Yuicy/Core/Input.h"` 不一致的问题，见第 3.4 节）。
- 换行、文本编码按第 4 节第 3 条（UTF-8 无 BOM + LF）。

### 3.4 已知技术债与移植阻碍（反例清单）

**下列代码不代表本项目认可的做法**。若任务恰好需要改动这些文件，优先顺手把它收进抽象；
若任务与之无关，不要夹带重构（见第 3.5 节）。

**A. 编译期硬阻断（移植时必然要动）**

| 位置 | 问题 |
|---|---|
| `Yuicy/src/Yuicy/Core/Core.h:13-15` | `#else #error Yuicy Only Support Windows!`，非 Windows 直接编译失败 |
| `Yuicy/src/pch.h:3-5`、`YuiStudio/src/pch.h:3-5` | `#ifdef PLATFORM_WINDOWS` → `#include <Windows.h>`，把 Win32 注入每个翻译单元，掩盖了下方 B 类问题 |
| `Yuicy/src/Yuicy/Core/EntryPoint.h:3` | 整个 `main()` 被 `#ifdef PLATFORM_WINDOWS` 包住（函数体本身是标准 C++） |
| `premake5.lua:61,119,168` | `PLATFORM_WINDOWS` 定义在 `filter "system:windows"` **之外**，任何系统生成工程都会定义它；且无 `filter "system:macosx"/"linux"`，`opengl32/user32/gdi32/shell32` 只在 Windows filter 内 |
| `premake5.lua:11` | `rundir` 用反斜杠硬拼路径 |

**B. 抽象泄漏（平台无关层直接调平台 API）**

| 位置 | 问题 |
|---|---|
| `YuiStudio/src/Editor/EditorSceneController.cpp:18,550-593` | 编辑器业务类里无 `#ifdef` 保护地 `#include <commdlg.h>`，用 `OPENFILENAMEA` / `GetOpenFileNameA` / `GetSaveFileNameA` + `glfwGetWin32Window` 实现文件对话框。**应抽象为 `Yuicy/Core/FileDialogs.h` 接口 + 平台实现**（仓库目前没有任何 `FileDialogs` / `PlatformUtils`） |
| `YuiStudio/src/Editor/EditorAssetWorkflow.cpp:13,220,234` | 无保护地 `#include <shellapi.h>`，用 `ShellExecuteW` 实现"在资源管理器中显示" / "用外部程序打开" |
| `Yuicy/src/Platform/Windows/WindowsWindow.h:5,50-51` | 平台实现的**头文件**里 `#include <Windows.h>` 并暴露 `HWND`/`LRESULT`/`WNDPROC` 成员 |
| `YuiStudio/src/Panels/AssetInspectorPanel.cpp:216,256` | `localtime_s` |
| `YuiStudio/src/Panels/ComponentEditors/AnimationEditor.cpp:136` | `strncpy_s` |
| `Yuicy/src/Yuicy/Core/input.h` | 文件名小写，但 `pch.h:23` 写作 `Input.h`；在区分大小写的文件系统上会编译失败 |
| `Yuicy/src/Yuicy/Core/Application.cpp:8-10`、`Yuicy/ImGui/ImGuiLayer.cpp:13-15` | 平台无关层 include 了 `<glad/glad.h>` / `<GLFW/glfw3.h>`，实际只用到 `glfwGetTime` 与 `glfw*CurrentContext`（无裸 `gl*` 调用），属多余依赖 |

**C. 渲染后端层面的结构性限制（非"加一个 Platform 实现"能解决）**

- `Platform/OpenGL/` 大量使用 **GL 4.5 DSA** API：`glCreateTextures`、`glCreateBuffers`、
  `glCreateVertexArrays`、`glCreateFramebuffers`、`glTextureStorage2D`、`glTextureSubImage2D`、
  `glTextureParameteri`、`glBindTextureUnit`，以及 GL 4.4 的 `glClearTexImage`。
  → Linux 无障碍；**macOS 上限 GL 4.1，这些 API 全部不存在**，需降级重写或新增 Metal/Vulkan 后端。
- `WindowsWindow.cpp:65` 只设置了 `GLFW_DECORATED`，**未设置** `GLFW_CONTEXT_VERSION_MAJOR/MINOR`
  与 `GLFW_OPENGL_CORE_PROFILE`；版本要求靠 `OpenGLContext.cpp:28` 的运行时断言（要求 ≥ 4.5）。
  macOS 必须显式请求 core profile + forward compat 才能拿到 3.2+ 上下文。
- Shader 的 `#version` 混用 **330 / 410 / 450** 三套（`YuiStudio/assets/shaders/*` 为 330，
  `Sandbox/assets/shaders/Shadow2D.glsl`、`Light2D.glsl` 为 450，ImGui 后端初始化为 410），
  与 4.5 的上下文断言并不一致。
- `WindowsWindow.cpp:167-170,311-342` 用 Win32 `WndProc` 子类化拦截 `WM_NCHITTEST` 实现无边框标题栏拖拽。
  这是**合理的平台实现位置**，但该能力在其他平台需完全重写。

### 3.5 在 macOS / Linux 上工作时

本仓库当前**无法**在 macOS / Linux 上编译或运行（原因见 3.4 的 A、C 两类）。因此：

- **允许也欢迎推进跨平台移植**，但要作为**明确的任务**来做，并按 3.4 的清单分步推进
  （建议顺序：premake 平台 filter → 移除 pch 的 `<Windows.h>` → 抽出 `FileDialogs` / Shell 集成
  → 放开 `Core.h` / `EntryPoint.h` → 最后处理 GL 后端）。
  移除 pch 里的 `<Windows.h>` 会让 B 类问题立刻暴露为编译错误，可当作"债务探测器"。
- **不要在无关任务里夹带平台重构**：修一个面板 Bug 时不要顺手改 `premake5.lua` 或渲染后端。
- **无法编译验证时，必须在结果中明确说明"未经编译验证"**，并列出需要人工在 Windows 上验证的点。
- 需要语义导航（跳转定义 / 查引用）时优先用 clangd / LSP 而非文本搜索；
  根目录没有 `compile_commands.json` 或增删过源文件时，先运行 `./GenerateProject.sh`。
- `WindowsWindow.cpp`、`EditorSceneController.cpp`、`EditorAssetWorkflow.cpp` 等直接调用 Win32 API 的文件
  在 macOS 上会有 clangd 报错，**属预期**（`premake/clangd-win32-stubs/` 里只有 `Windows.h`、`commdlg.h`、
  `shellapi.h`、`windowsx.h` 四个空桩，通过 `-idirafter` 兜底），不要据此修改代码。

## 4. 硬性规则（违反即视为错误）

1. **不要修改** `Yuicy/thirdparty/**` 与 `Yuicy/src/Yuicy/ImGui/ImGuizmo.*`；不要升级/切换子模块版本，除非任务明确要求。
2. 带 PCH 的工程（Yuicy、YuiStudio）中，每个 `.cpp` 的**第一行**必须是 `#include "pch.h"`（`ImGuizmo.cpp` 除外）。
3. 所有源文件使用 **UTF-8（无 BOM）+ LF**；注释可用中文。
4. **平台专属代码（OS API / 图形后端 API）只能写在 `Yuicy/src/Platform/` 下，并通过第 3.2 节的既有分发机制接入**；
   平台无关层与客户端代码一律走抽象接口。详见第 3 节。
5. 编辑器中**结构性修改**（实体增删、组件增删、父子关系变更、Gizmo 变换）必须通过
   `EditorCommandHistory::ExecuteCommand` 执行一个 `IEditorCommand`，以支持 Undo/Redo。
   属性面板里的字段编辑目前直接修改组件，**任何修改场景数据的路径都必须调用** `EditorDirtyTracker::MarkSceneDirty()`。
6. 资源引用使用 `AssetHandle`（UUID），不要在组件中存储文件路径；资源的增删改走 `EditorAssetManager` / `EditorAssetWorkflow`。
7. 日志用 `YUICY_CORE_*`（引擎内）/ `YUICY_*`（客户端），断言用 `YUICY_CORE_ASSERT` / `YUICY_ASSERT`；不要使用 `std::cout` / `printf`。
8. 智能指针用 `Ref<T>` / `Scope<T>` 与 `CreateRef` / `CreateScope`（`Yuicy/Core/Base.h`）。
9. 不要批量重排格式；只格式化自己改动的代码（见第 7 节）。
10. 不要提交构建产物、`.sln` / `.vcxproj` / `imgui.ini` 的无关改动。

## 5. 目录地图

```
Yuicy/src/
  pch.h / pch.cpp          # 引擎预编译头（勿加平台专属 include）
  Yuicy.h                  # 对客户端（YuiStudio/Sandbox）暴露的总头文件
  Yuicy/Core/              # Application, Layer/LayerStack, Log, Assert, UUID, Input, Window, Base(Ref/Scope), Core.h(YUICY_API)
  Yuicy/Events/            # 事件系统（Application/Key/Mouse）
  Yuicy/Renderer/          # Renderer2D, RenderCommand/RendererAPI, GraphicsContext, Shader, Texture, Framebuffer, Camera, SortingLayer
  Yuicy/Scene/             # Scene, Entity, Components.h, SceneSerializer, SceneCamera, ContactListener
  Yuicy/Asset/             # Asset, AssetHandle, AssetRegistry, EditorAssetManager, AssetImporter, 扩展名映射
  Yuicy/Project/           # Project(.yproj) 与序列化，SortingLayers / CollisionLayers 配置
  Yuicy/Physics/           # Box2D 封装, CollisionLayerConfig
  Yuicy/Scripting/         # LuaScriptEngine, LuaBindings（sol2）
  Yuicy/ImGui/             # ImGuiLayer，ImGuizmo（外部拷贝代码，勿改）
  Yuicy/Debug/             # Instrumentor（YUICY_PROFILE_*）
  Platform/OpenGL/         # 渲染后端实现：RendererAPI / Buffer / VertexArray / Shader / Texture / Framebuffer / Context
  Platform/Windows/        # OS 平台实现：WindowsWindow（含 Win32 WndProc）/ WindowsInput
Yuicy/thirdparty/          # 第三方依赖（子模块或拷贝源码），勿改

YuiStudio/src/
  pch.h / pch.cpp          # 编辑器预编译头
  YuiStudioApp.cpp         # 入口
  EditorLayer.*            # 编辑器主 Layer：组装各服务与面板、快捷键
  Editor/                  # 编辑器服务层（无 UI）
    EditorContext.h        #   全局共享状态：场景、运行模式、文档、选择、视口、实体元数据
    EditorCommandHistory.* #   Undo/Redo 栈
    Commands/              #   IEditorCommand 具体实现 + EntitySnapshot
    EditorSceneController.*#   新建/打开/保存 场景与项目，Play/Simulate/Stop（含待抽象的 Win32 文件对话框）
    EditorAssetWorkflow.*  #   文件与资源相关操作（含待抽象的 Win32 Shell 集成）
    EditorDirtyTracker.*   #   脏标记与 AutoSave
    EditorRenderPipeline.* / EditorOverlayRenderer.*  # 视口渲染与叠加绘制（选中框、碰撞体等）
  Panels/                  # ImGui 面板：Viewport, SceneHierarchy, Properties, ContentBrowser, AssetInspector
    ComponentEditors/      #   各组件在 Properties 面板中的编辑 UI
  Utils/
YuiStudio/assets/          # 编辑器资源（shaders、textures、fonts）

premake/
  export-compile-commands.lua  # 生成 compile_commands.json 的 premake 模块
  premake.ps1                  # Windows 侧 premake 下载引导（固定版本 + SHA256 校验）
  clangd-win32-stubs/          # 4 个空的 Win32 头文件桩，仅供非 Windows 主机上的 clangd
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

### 6.2 新增一个可撤销的编辑器操作

- [ ] 在 `YuiStudio/src/Editor/Commands/` 新建 `XxxCommand.h`，继承 `IEditorCommand`，实现 `Execute` / `Undo` / `GetName`。
- [ ] 命令中通过 **UUID** 保存实体引用，执行时用 `Scene::FindEntityByUUID` 取回（`entt::entity` 在删除/恢复后会失效）。
- [ ] 连续操作（如拖拽）只产生一条历史：参考 `EditorViewportPanel` 中 Gizmo 的做法，在拖拽开始时记录旧值、结束时提交一条 `SetTransformCommand`；
      或实现 `GetCommandID` + `TryMerge`，由 `EditorCommandHistory` 自动合并。
- [ ] 调用方使用 `m_commandHistory->ExecuteCommandT<XxxCommand>(...)`，并标记脏状态。

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
- [ ] 若用到 GL 4.5 DSA API，在 PR/说明中注明（会影响 macOS 移植，见 3.4 C）。

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

## 8. 提交规范

沿用已有风格：`<type>(<scope>): <中文描述>`，正文用 `- ` 列出要点。

- type：`feat` / `fix` / `refactor` / `perf` / `docs` / `chore` / `build` / `test`
- scope：模块名，如 `YuiStudio`、`Yuicy`、`Scene`、`Renderer`、`Platform`、`Lua`、`SpriteCommand`、`Infra`
- 分支：`feature/<PascalCase>`，如 `feature/YuiStudio2D`、`feature/AIInfra`

## 9. 交付前自检

当前仓库尚无自动化测试与 CI，提交前至少确认：

- [ ] 改动范围与任务一致，没有顺手重构 / 批量格式化 / 修改第三方代码 / 夹带平台移植改动。
- [ ] 第 4 节硬性规则全部满足；涉及组件时第 6.1 节清单逐项核对。
- [ ] 平台相关代码满足第 3 节：没有在 `Yuicy/src/Yuicy/**` 或 `YuiStudio/src/**` 新增 Win32 / 裸 `gl*` 调用，
      没有新增 MSVC 专属 CRT 函数，路径处理走 `std::filesystem`，新文件名大小写与 `#include` 一致。
- [ ] 在 Windows 上 Debug 与 Release 均能编译；若无法编译验证，已明确告知并列出待人工验证的点。
- [ ] 涉及序列化时：保存 → 重新打开场景，数据一致；涉及编辑操作时：执行 → Undo → Redo 结果一致。
