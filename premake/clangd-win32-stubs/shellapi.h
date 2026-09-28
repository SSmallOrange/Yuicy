#pragma once

// 空的 Win32 头文件桩，仅用于非 Windows 主机上的 clangd 语义分析（见 premake/export-compile-commands.lua）。
// 通过 -idirafter 引入：优先级最低，存在真实头文件时不会生效；Win32 API 调用处仍会报未声明，属预期行为。
