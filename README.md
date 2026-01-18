# Yuicy Framework

> 注：本项目学习自：``Hazel引擎``的开源部分，舍弃原本引擎的编辑界面，并从FBO部分开始脱离，专注于OpenGL相关内容，以下是Hazel引擎相关链接：
>
> Github：[TheCherno/Hazel: Hazel Engine](https://github.com/TheCherno/Hazel)
>
> Youtube：[Introducing the GAME ENGINE series! - YouTube](https://www.youtube.com/watch?v=JxIZbV_XjAs&list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT)
>
> Bilibili翻译版：BV1wtLazEEmC

Yuicy 是一个 **C++20 + OpenGL** 的轻量级**渲染学习框架（Framework）**。主要用来学习完整的GPU渲染流程，并继续向外拓展。

> 核心目标：通过实现简单Demo来整合现代 OpenGL 渲染流程，包括渲染抽象、2D 批渲染、FBO、光照、后处理、粒子特效、脚本等。

---

## 内容演示



https://github.com/user-attachments/assets/2473dd1a-5b34-441e-bd5e-eb17a5bc7249




###  当前支持效果

- 2D 渲染：精灵绘制与批渲染，纹理与精灵表切分
- 离屏渲染与后处理：基于 FBO 的效果链，支持基础色彩调节、雾、暗角、雨滴叠加
- 2D 光照：点光源与聚光灯
- 天气系统：粒子雨与溅射，支持预设配置
- 简单的2D 物理与交互

### 框架层能力

- 应用框架：`Application / Layer / LayerStack`
- 窗口与输入：目前只支持Windows 平台的窗口、键鼠输入、事件系统
- ECS/场景：基于`entt`封装ECS，默认支持组件：Tag/Transform/Sprite/Camera/Animation/LuaScript
- 2D 物理：基于`Box2D`封装，支持物理效果
- TileMap：预设基础的地图解析框架，支持按照格式区分不同的解析方式
- Lua 脚本：通过 `sol2`支持Lua脚本功能，脚本通过（`OnCreate`、`OnUpdate`、`OnDestroy`、`OnCollisionEnter`等方法，涵盖实体的生命周期和碰撞

---

### 目录结构

```
Yuicy/
  Yuicy/                 # 静态库
    src/
      Yuicy/Core/        # Application/Layer/Log/Assert
      Yuicy/Renderer/    # Renderer2D/Shader/Texture/Framebuffer
      Yuicy/Scene/       # Scene/Entity/Components
      Yuicy/Scripting/   # Lua 引擎与绑定
      Yuicy/Physics/     # Box2D 封装
      Yuicy/TileMap/     # TileMap解析
      Yuicy/Effects/     # Weather/Lighting2D/PostProcessing等额外效果
      Platform/          # OpenGL + Windows 平台隔离
      
    thirdparty/          # 第三方依赖

  TinyDungeon/           # Demo展示（内容驱动：地图/脚本/敌人/效果）
    src/
    assets/				# 地图、人物、脚本资源

  Sandbox/                # 功能临时验证
  
  premake5.lua            # 总工程生成脚本
  GenerateProject.bat     # 一键生成 VS2022 工程
  premake5.exe
```



---

## 构建与运行（Windows）

### 环境要求

- Windows
- Visual Studio 2022（`GenerateProject.bat` 默认生成 vs2022 工程）
- C++20 工具链 + Windows SDK

### 拉取代码（重要：包含子模块）

```bash
git clone --recursive https://github.com/SSmallOrange/Yuicy
```

如果已 clone 未带子模块：

```bash
git submodule update --init --recursive
```

### 生成 VS 工程

双击运行：

```bat
GenerateProject.bat
```

或手动执行：

```bat
premake5.exe vs2022
```

### 运行

1. 打开生成的 `.sln`
2. 默认以 `TinyDungeon` 作为启动项目
3. 编译并运行

---

## 依赖与第三方

依赖由根目录 `premake5.lua` 统一管理，集中在 `Yuicy/thirdparty/`。

- GLFW（子模块）
- GLAD（仓库内带源码）
- ImGui（仓库内带源码）
- Box2D（子模块）
- Lua（仓库内带源码）
- sol2（子模块，Lua 绑定）
- glm（子模块）
- stb_image（仓库内带源码）
- EnTT（仓库内带源码，ECS）
- spdlog（子模块，日志）

---
## TODO

- 对三方库的二次封装，避免接口暴漏
- 支持更多的2D功能
- 拓展3D功能
- 支持Vulkan

---

## License

TBD
