#pragma once

#include "imgui/imgui.h"

#include <filesystem>
#include <optional>
#include <string>

// ContentBrowser 文件拖拽的 payload 统一为 UTF-8 字节串（含结尾 '\0'）。
// 不能直接写 path::native()：其字符类型随平台变化（Windows 为 2 字节 wchar_t，macOS 为 char），接收端无法可移植地解读
namespace Yuicy::ContentBrowserDragDrop {

	inline constexpr const char* kPayloadType = "CONTENT_BROWSER_ITEM";

	// 在 BeginDragDropSource / EndDragDropSource 之间调用；ImGui 会拷贝数据，path 不需要保持存活
	inline void SetPayload(const std::filesystem::path& path)
	{
		const std::u8string utf8 = path.u8string();
		ImGui::SetDragDropPayload(kPayloadType, utf8.c_str(), (utf8.size() + 1) * sizeof(char8_t));
	}

	// 在 BeginDragDropTarget / EndDragDropTarget 之间调用；松开鼠标投放时才返回路径，其余情况返回 std::nullopt
	inline std::optional<std::filesystem::path> AcceptPayload()
	{
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayloadType);
		if (!payload || !payload->Data || payload->DataSize <= 0)
			return std::nullopt;

		// 按 DataSize 取长度而不依赖结尾 '\0'，DataSize 已包含它
		const auto* data = static_cast<const char8_t*>(payload->Data);
		return std::filesystem::path(std::u8string(data, static_cast<size_t>(payload->DataSize) - 1));
	}

}
