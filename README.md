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
├── cmake/                      # CMake 辅助模块：第三方依赖 target（ThirdParty.cmake）、公共设置（YuicyHelpers.cmake）
├── CMakeLists.txt              # 根构建脚本（Yuicy/、YuiStudio/、Sandbox/ 下各有一份子脚本）
├── CMakePresets.json           # 构建预设：debug / release（Makefile）、vs2022
└── AGENTS.md                   # AI 编码助手 / 贡献者工作约定
```

---

## 构建与运行（Windows）

### 环境要求

- Windows 10/11
- Visual Studio 2022（含 "使用 C++ 的桌面开发" 工作负载、Windows SDK；自带 CMake）
- CMake ≥ 3.21（VS2022 自带的即可）
- 支持 OpenGL 4.5 的显卡驱动

> macOS / Linux 目前可以完成 CMake 配置并编译第三方库，但引擎源码尚未完成跨平台适配，编译会在平台相关代码处失败。

### 获取代码

```bash
git clone --recursive https://github.com/SSmallOrange/Yuicy
```

如果 clone 时未带子模块：

```bash
git submodule update --init --recursive
```

### 生成与构建

构建系统为 CMake，预设定义在 `CMakePresets.json`，构建目录为 `build/<preset>/`（已被 git 忽略）。

**Windows（Visual Studio 2022）**

- 方式一：VS2022 直接"打开本地文件夹"选择仓库根目录，VS 会识别 `CMakePresets.json`，选择 `vs2022` 预设即可。
- 方式二：命令行生成解决方案后打开 `build/vs2022/Yuicy.sln`：

```bat
cmake --preset vs2022
cmake --build --preset vs2022-debug
```

默认启动项目为 `YuiStudio`（也可将 `Sandbox` 设为启动项目），调试工作目录为工程目录（`YuiStudio/`、`Sandbox/`），资源以 `assets/...` 相对路径加载。

**macOS / Linux（Unix Makefiles）**

```bash
cmake --preset debug                 # 生成 build/debug/Makefile（Release 用 release）
cmake --build --preset debug         # 编译全部目标
cmake --build --preset debug --target YuiStudio
cmake --build --preset debug --target run-YuiStudio   # 以工程目录为工作目录运行
```

构建产物：可执行文件在 `build/<preset>/bin/`（VS 为 `bin/<Config>/`），静态库在 `build/<preset>/lib/`。

> 新增 / 删除源文件无需修改 CMake 脚本（以通配方式收集 `src/**`，并带 `CONFIGURE_DEPENDS`，下次构建时自动重新配置）。

### 编译数据库（clangd / LSP）

使用 Makefile 预设（`debug` / `release`）配置时，CMake 会生成 `build/<preset>/compile_commands.json`，
并在仓库根目录创建指向它的符号链接 `compile_commands.json`（已被 git 忽略），clangd 可直接使用。

- 文件中使用的是绝对路径，**需要在使用它的机器上生成**。
- Visual Studio 生成器不产生编译数据库；Windows 上需要时可在 VS 开发者命令行中用 `cmake -S . -B build/ninja -G Ninja` 生成。

---

## 依赖

依赖位于 `Yuicy/thirdparty/`，其 CMake target 统一在 `cmake/ThirdParty.cmake` 中声明
（GLFW / Box2D / yaml-cpp 使用上游 CMakeLists，GLAD / imgui / Lua 在该文件中定义，其余为纯头文件）：

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
