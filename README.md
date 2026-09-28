# Yuicy

> 本项目学习自 [Hazel 引擎](https://github.com/TheCherno/Hazel) 的开源部分，并在此基础上独立演进。
>
> - YouTube：[Introducing the GAME ENGINE series!](https://www.youtube.com/watch?v=JxIZbV_XjAs&list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT)
> - Bilibili 翻译版：BV1wtLazEEmC

Yuicy 是一个 **C++20 + OpenGL 4.5** 的轻量级 **2D 引擎**，配套场景编辑器 **YuiStudio**。
主要用来学习完整的 GPU 渲染流程和引擎/编辑器架构，并持续向外扩展。

---

## 功能概览

### 引擎（Yuicy）

- **应用框架**：`Application` / `Layer` / `LayerStack`，事件系统，窗口与键鼠输入（目前仅 Windows）
- **2D 渲染**：`Renderer2D` 批渲染（Quad / Sprite / Line / Rect / Circle）、纹理与 SubTexture 切片、Framebuffer（含实体 ID 拾取附件）
- **渲染排序**：Sorting Layer + Sorting Order 共同决定绘制顺序，层级在项目中配置
- **ECS 场景**：基于 `EnTT` 封装，支持实体父子层级、场景拷贝与 YAML 序列化（`.yui`）
- **内置组件**：ID / Tag / Relationship / Transform / SpriteRenderer / Camera / Animation / LuaScript / NativeScript / Rigidbody2D / BoxCollider2D / CircleCollider2D
- **2D 物理**：基于 `Box2D`，支持刚体、碰撞体、触发器与碰撞层（Collision Layer）过滤
- **Lua 脚本**：基于 `sol2`，脚本可实现 `OnCreate` / `OnUpdate` / `OnDestroy` / `OnCollisionEnter` / `OnCollisionExit` / `OnTriggerEnter` / `OnTriggerExit`
- **资源与项目**：基于 `AssetHandle`(UUID) 的资源注册表与导入器；项目文件 `.yproj` 管理资源目录、排序层与碰撞层
- **调试**：spdlog 日志、断言、`YUICY_PROFILE_*` 性能采样（Chrome Tracing 格式）

### 编辑器（YuiStudio）

- **视口**：编辑器相机（`Alt+WASD` 移动、`Shift` 加速、中键拖拽平移、滚轮以鼠标为中心缩放、`F` 聚焦选中实体）、Gizmo（`Q/W/E/R` 切换 无/平移/旋转/缩放）
- **选择**：单击、`Ctrl+Click` 切换选择、`Shift+Click` 追加选择、鼠标框选，多选高亮
- **场景层级面板**：实体树、拖拽调整父子关系、节点锁定与隐藏
- **属性面板**：组件增删与编辑（Sprite、Animation 帧切片、Camera 宽高比与安全区、Collider 与碰撞层等）
- **内容浏览器**：面包屑导航、搜索与类型过滤、右键新建/重命名/删除文件与文件夹、新建脚本与场景、拖拽资源到场景或组件
- **资源检查面板**：与内容浏览器联动，展示资源详情
- **Undo / Redo**：实体与组件的增删、父子关系变更、变换操作均可撤销（`Ctrl+Z` / `Ctrl+Y`）
- **运行模式**：Edit / Play / Simulate，支持暂停与单步
- **文件**：新建/打开/保存 场景（`Ctrl+N` / `Ctrl+O` / `Ctrl+S` / `Ctrl+Shift+S`）与项目，脏标记与自动保存

---

## 目录结构

```
Yuicy/
├── Yuicy/                      # 引擎（静态库）
│   ├── src/
│   │   ├── Yuicy.h             # 客户端总头文件
│   │   ├── Yuicy/
│   │   │   ├── Core/           # Application / Layer / Log / Assert / UUID / Input / Window
│   │   │   ├── Events/         # 事件系统
│   │   │   ├── Renderer/       # Renderer2D / Shader / Texture / Framebuffer / Camera
│   │   │   ├── Scene/          # Scene / Entity / Components / SceneSerializer
│   │   │   ├── Asset/          # 资源句柄、注册表、导入器、EditorAssetManager
│   │   │   ├── Project/        # 项目配置与序列化
│   │   │   ├── Physics/        # Box2D 封装、碰撞层
│   │   │   ├── Scripting/      # Lua 引擎与绑定
│   │   │   ├── ImGui/          # ImGuiLayer、ImGuizmo
│   │   │   └── Debug/          # Instrumentor
│   │   └── Platform/
│   │       ├── OpenGL/         # OpenGL 实现
│   │       └── Windows/        # Windows 窗口与输入
│   └── thirdparty/             # 第三方依赖
├── YuiStudio/                  # 场景编辑器（默认启动项目）
│   ├── src/
│   │   ├── EditorLayer.*       # 编辑器主 Layer
│   │   ├── Editor/             # 编辑器服务：上下文、命令/撤销、场景控制、资源工作流、渲染管线
│   │   └── Panels/             # ImGui 面板与组件编辑器
│   └── assets/                 # 编辑器资源（shaders / textures / fonts）
├── Sandbox/                    # 功能临时验证
├── premake/                    # premake 下载引导（premake.ps1）、扩展模块与第三方工程脚本
├── premake5.lua                # 工程生成脚本
├── GenerateProject.bat         # Windows：生成 VS2022 工程与 compile_commands.json
├── GenerateProject.sh          # macOS / Linux：生成 compile_commands.json
└── AGENTS.md                   # AI 编码助手 / 贡献者工作约定
```

---

## 构建与运行（Windows）

### 环境要求

- Windows 10/11
- Visual Studio 2022（含 "使用 C++ 的桌面开发" 工作负载、Windows SDK）
- 支持 OpenGL 4.5 的显卡驱动

### 获取代码

```bash
git clone --recursive https://github.com/SSmallOrange/Yuicy
```

如果 clone 时未带子模块：

```bash
git submodule update --init --recursive
```

### 生成解决方案

双击运行 `GenerateProject.bat`。

- 仓库不再内置 `premake5.exe`：首次运行时自动下载固定版本（`5.0.0-beta7`）到 `bin/tools/`（已被 git 忽略），校验 SHA256 后使用，之后复用缓存。
- 需要联网访问 GitHub；离线或已有 premake 时，可 `set PREMAKE5=C:\path\to\premake5.exe` 后再运行。
- 手动执行任意 premake 命令：

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File premake\premake.ps1 vs2022
```

### 运行

1. 打开生成的 `Yuicy.sln`
2. 默认启动项目为 `YuiStudio`（也可将 `Sandbox` 设为启动项目）
3. 选择 `Debug` 或 `Release`，编译并运行

构建产物位于 `bin/<Config>-x64/<Project>/`。

> 新增源文件后需重新运行 `GenerateProject.bat`（`premake5.lua` 以通配方式收集 `src/**`）。

### 编译数据库（clangd / LSP）

在仓库根目录生成 `compile_commands.json`（已被 git 忽略），供 clangd 等工具做跳转定义、查引用。三个平台均可生成：

| 平台 | 命令 |
|---|---|
| Windows | `GenerateProject.bat`（生成 VS 工程的同时导出） |
| macOS / Linux | `./GenerateProject.sh`（同样自动下载固定版本的 premake 到 `bin/tools/` 并校验 SHA256） |

导出其他配置：

```bash
# macOS / Linux
./GenerateProject.sh export-compile-commands --export-compile-commands-config=Release
# Windows
powershell -NoProfile -ExecutionPolicy Bypass -File premake\premake.ps1 export-compile-commands --export-compile-commands-config=Release
```

- 实现见 `premake/export-compile-commands.lua`；文件中使用的是绝对路径，**需要在使用它的机器上生成**，不能拷贝到其他机器。
- 已有 premake5 时可通过环境变量 `PREMAKE5` 指定，如 `PREMAKE5=/path/to/premake5 ./GenerateProject.sh`。
- macOS / Linux 缺失的 Win32 头文件由 `premake/clangd-win32-stubs/` 中的空头文件兜底，仅直接调用 Win32 API 的少数文件会有 clangd 报错。
- 新增 / 删除源文件后需重新生成。

---

## 依赖

依赖由根目录 `premake5.lua` 统一管理，位于 `Yuicy/thirdparty/`：

| 库 | 用途 | 引入方式 |
|---|---|---|
| [GLFW](https://github.com/TheCherno/glfw) | 窗口与输入 | 子模块 |
| GLAD | OpenGL 函数加载 | 源码 |
| [Dear ImGui](https://github.com/ocornut/imgui) | 编辑器 UI | 源码 |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | 变换 Gizmo | 源码（`Yuicy/src/Yuicy/ImGui/`） |
| [EnTT](https://github.com/skypjack/entt) | ECS | 源码 |
| [Box2D](https://github.com/thecherno/box2d) | 2D 物理 | 子模块 |
| Lua | 脚本虚拟机 | 源码 |
| [sol2](https://github.com/ThePhD/sol2) | Lua 绑定 | 子模块 |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | 场景 / 项目序列化 | 子模块 |
| [spdlog](https://github.com/gabime/spdlog) | 日志 | 子模块 |
| [glm](https://github.com/g-truc/glm) | 数学库 | 子模块 |
| stb_image | 图片加载 | 源码 |
| [TinyReflection](https://github.com/SSmallOrange/TinyReflection) | 反射 | 子模块 |

---

## 参与开发

- 代码风格：见 `.clang-format` / `.editorconfig`，只格式化自己改动的代码（`git clang-format`）
- 提交信息：`<type>(<scope>): <描述>`，例如 `feat(YuiStudio): 新增多选实体功能`
- 使用 AI 编码助手时，请先阅读 [AGENTS.md](AGENTS.md)

---

## License

TBD
