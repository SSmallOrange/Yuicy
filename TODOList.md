# Yuicy 基建 TODO

> 来源：对照 [AI_INFRA_ROADMAP.md](AI_INFRA_ROADMAP.md) 与仓库现状（2026-09-30）整理。
> 不含 Windows 回归与 CI 相关事项。按优先级分组，组内大致按推荐顺序排列。

---

## P0：文档与现状纠偏（成本低、收益最高）

- [x] **更新 `AGENTS.md` 平台相关章节**（与 CI 解耦，可立即做）
  - [x] 第 2 节：「目前仅 Windows」→ macOS arm64 可编译运行，Windows 待回归
  - [x] 第 3.1 / 3.2 / 5 节：`Platform/Windows/WindowsInput.cpp` → `Platform/GLFW/GlfwInput.cpp`；补充 `Platform/GLFW`、`Platform/MacOS`（`.mm`）目录说明
  - [x] 第 3.4 节：删除已偿还的技术债（`FileDialogs` / `PlatformUtils` 已抽象、`Core.h` / `EntryPoint.h` 已放开、`localtime_s` / `strncpy_s` 已修复等）
  - [x] 第 3.5 节：「mac 上尚无法编译运行」直接移除
  - [ ] 可选：把第 3 节平台细节移到 `docs/platform.md`，`AGENTS.md`（当前 494 行）只保留规则
- [x] **`.gitignore` 补充 `YuiStudio/Profiles-*.json`**（路线图 4.5 写了要加，实际没加）
- [x] **处理 `YuiStudio/imgui.ini`**：改为提交 `imgui.default.ini` 作为默认布局，`imgui.ini` 由 git 忽略，本地不存在时由 `ImGuiLayer` 复制
- [x] **路线图自身更新**
  - [x] 「推荐推进顺序」表中第 5 项（测试工程）已完成，调整位置

---

## P1：本地自我验证闭环

- [ ] **一键自检脚本** `scripts/check.sh`：配置 → 构建 → `ctest` → `git clang-format --diff`，作为 AI 判断「完成」的统一标准
- 【】
- [ ] **断言在测试中转为用例失败**：`YUICY_CORE_ASSERT` 目前 `raise(SIGTRAP)` 直接崩进程；在 `YuicyTestFramework` 中提供断言钩子，转为 doctest 失败报告或抛异常
- [ ] **编译警告**（路线图 4.1）：`yuicy_configure_target` 开启 `-Wall -Wextra`，清零后开启 `COMPILE_WARNING_AS_ERROR`
  - [ ] **优先排查** `Components.h:157`：`delete nsc->Instance` 删除的 `ScriptableEntity` 只有前向声明，可能是未定义行为
  - [ ] `Buffer.h:14,50`、`OpenGLVertexArray.cpp:10` 的 `-Wswitch`
  - [ ] `OpenGLVertexArray.h:19-20` 的 `-Winconsistent-missing-override`
- [ ] **Ninja 预设 + ccache**（`CMAKE_CXX_COMPILER_LAUNCHER`）：缩短本地构建时间
- [ ] **CMake 配置时检查子模块**：`cmake/ThirdParty.cmake` 中 `if(NOT EXISTS …)` 直接报错，并提示执行 `git submodule update --init --recursive`

---

## P2：测试补盲

- [ ] **Lua 绑定**：加载一段 Lua 脚本，调用各 `GetXxx` / `HasXxx` 并断言，防止绑定名写错
- [ ] **`Physics2D`**：固定步长模拟 N 步后断言位置；覆盖碰撞层过滤
- [ ] **资源系统**：`AssetImporter`、`EditorAssetManager`、`AssetExtensions` 扩展名映射
- [ ] **`EditorDirtyTracker`**：各修改路径都会标脏（AGENTS.md 规则 5 目前无测试兜底）
- [ ] **Shader 离线校验**：用 `glslangValidator` 按 `#type` 拆分后校验 `.glsl`，顺带统一 `#version`（现存 330 / 410 / 450 混用）
- [ ] **覆盖率预设** `coverage`：`-fprofile-instr-generate -fcoverage-mapping` + `llvm-cov`

---

## P3：防呆与流程化

- [ ] **组件注册表**（路线图 3.2）：`ComponentGroup<...>` / `AllComponents` 类型列表，`Scene::Copy`、`EntitySnapshot`、`DuplicateEntity` 模板遍历
  - [ ] 完成后补上 `EntitySnapshot` 完整性测试（用例中已留 TODO）
- [ ] **操作手册**（路线图 3.1），放在 `.codebuddy/skills/`
  - [ ] `add-component`
  - [ ] `add-editor-command`
  - [ ] `add-panel`
  - [ ] `add-lua-binding`
  - [ ] `add-shader`
  - [ ] `add-platform-feature`
- [ ] **提交前检查**（路线图 3.3）：`.githooks/`，与 `scripts/check.sh` 复用同一套检查逻辑
  - [ ] `git clang-format --diff`
  - [ ] UTF-8 无 BOM + LF
  - [ ] 禁止提交 `build/`、根目录 `compile_commands.json`、`Profiles-*.json`；`imgui.default.ini`（默认窗口布局）改动给出提醒
  - [ ] 提交信息格式 `<type>(<scope>): <desc>`
- [ ] **多 AI 工具入口文件**（均指向 `AGENTS.md`，不复制内容）
  - [ ] `CLAUDE.md`
  - [ ] `.codebuddy/rules/`
  - [ ] `.cursor/rules/`、`.github/copilot-instructions.md`（按需）

---

## P4：稳定性

- [ ] **序列化格式版本号**：`.yui` / `.yproj` 加 `Version` 字段，为后续字段变更提供迁移入口；旧版本样例文件放入 `Yuicy/tests/data/` 做兼容测试
- [ ] **修复测试中记录的产品问题**
  - [ ] `CreateEntityCommand` 每次 `Execute` 生成新 UUID，Redo 后后续命令找不到实体（现有 `should_fail` 用例）
  - [ ] 确认 `EditorCommandHistory::ExecuteCommand` 在 `TryMerge` 成功时的语义（未调用新命令 `Execute`、未清空 Redo 栈）
- [ ] **关键不变量断言**（路线图 4.3）：UUID 唯一、父子关系一致
- [ ] **静态分析**（路线图 4.2）：`.clang-tidy`，先启用 `bugprone-*`、`performance-*`、`modernize-use-override`、`modernize-use-nullptr`

---

## P5：AI 可观测性与渲染回归

- [ ] **日志文件位置**：从非工程目录启动时 `Yuicy.log` 的输出位置（配合 mac 适配方案阶段 4）
- [ ] **命令行参数**（路线图 4.4）
  - [ ] `--project <path> --scene <path>`
  - [ ] `--frames N`
  - [ ] `--screenshot <out.png>`
  - [ ] `--headless`（依赖 Headless 渲染后端）
- [ ] **Headless 渲染后端**（路线图 2.3）：`RendererAPI` / `GraphicsContext` 空实现，需要时再做
- [ ] **渲染回归测试**：固定场景 → 截图 → 与基准图像素比较
- [ ] **性能基线**（路线图 4.5）：用 `Instrumentor` 输出典型场景的 Chrome Tracing JSON

---

## P6：知识沉淀（持续）

- [ ] **架构文档** `docs/architecture.md`（路线图 5.1）：渲染链路、ECS 生命周期、编辑器三层结构、资源系统、平台层分发方式
- [ ] **ADR** `docs/adr/`（路线图 5.2），建议先写
  - [ ] 为什么 OpenGL 后端降到 4.1 Core、纹理槽为 16
  - [ ] 为什么 Command 通过 UUID 而非 `entt::entity` 引用实体
  - [ ] 其余：`EditorEntityMetadata`、AssetManager 接管脚本路径、迁移 CMake、mac 优先
- [ ] **依赖清单** `Yuicy/thirdparty/README.md`（路线图 5.3）：非子模块依赖的版本与来源、子模块 commit、已知版本约束（ImGui 1.88 docking、GLAD 4.6 compatibility loader）
- [ ] **Lua 脚本 API 类型声明**：LuaLS `---@meta` 注解文件，并加测试校验与 `LuaBindings.cpp` 一致
