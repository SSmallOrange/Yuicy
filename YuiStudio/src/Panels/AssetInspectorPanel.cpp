#include "pch.h"

#include "AssetInspectorPanel.h"

#include "../Editor/EditorContext.h"
#include "../Editor/EditorAssetWorkflow.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Asset/AssetTypes.h"
#include "Yuicy/Renderer/Texture.h"

#include <chrono>
#include <ctime>
#include <filesystem>

namespace Yuicy {

	// 不用 std::chrono::clock_cast：Apple libc++ 未实现。按两个时钟当前时间的差值换算，误差在微秒级
	static std::chrono::system_clock::time_point ToSystemTime(std::filesystem::file_time_type fileTime)
	{
		auto offset = fileTime - std::filesystem::file_time_type::clock::now();
		return std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(offset);
	}

	// std::localtime 返回共享的静态缓冲区，不是线程安全的；两端的线程安全版本参数顺序相反
	static std::tm ToLocalTime(std::time_t time)
	{
		std::tm result{};
#ifdef _MSC_VER
		localtime_s(&result, &time);
#else
		localtime_r(&time, &result);
#endif
		return result;
	}

	static std::string FormatLastWriteTime(std::filesystem::file_time_type fileTime)
	{
		std::tm tm = ToLocalTime(std::chrono::system_clock::to_time_t(ToSystemTime(fileTime)));
		char timeBuf[64];
		std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
		return timeBuf;
	}

	void AssetInspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Asset Inspector");

		if (!m_context || !m_context->selection.HasAssetSelection())
		{
			DrawNoSelection();
			ImGui::End();
			return;
		}

		AssetHandle selectedHandle = m_context->selection.selectedAsset;
		EditorAssetManager* assetManager = m_context->GetAssetManager();
		if (!assetManager || !assetManager->IsAssetHandleValid(selectedHandle))
		{
			ImGui::TextDisabled("Invalid asset handle.");
			ImGui::End();
			return;
		}

		const AssetMetadata& metadata = assetManager->GetMetadata(selectedHandle);
		if (!metadata.IsValid())
		{
			ImGui::TextDisabled("Asset metadata not found.");
			ImGui::End();
			return;
		}

		DrawAssetHeader(*assetManager, metadata);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		switch (metadata.type)
		{
		case AssetType::Texture:   DrawTextureInspector(*assetManager, metadata); break;
		case AssetType::Scene:     DrawSceneInspector(*assetManager, metadata); break;
		case AssetType::LuaScript: DrawScriptInspector(*assetManager, metadata); break;
		case AssetType::Shader:    DrawShaderInspector(*assetManager, metadata); break;
		case AssetType::Font:      DrawFontInspector(*assetManager, metadata); break;
		default:
			ImGui::TextDisabled("No inspector available for this asset type.");
			break;
		}

		ImGui::End();
	}

	void AssetInspectorPanel::DrawNoSelection()
	{
		float availWidth = ImGui::GetContentRegionAvail().x;
		float availHeight = ImGui::GetContentRegionAvail().y;

		const char* hint = "Select an asset in the Content Browser\nto inspect its properties.";
		ImVec2 textSize = ImGui::CalcTextSize(hint);

		ImGui::SetCursorPosX((availWidth - textSize.x) * 0.5f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availHeight * 0.3f);
		ImGui::TextDisabled("%s", hint);
	}

	void AssetInspectorPanel::DrawAssetHeader(EditorAssetManager& assetManager, const AssetMetadata& metadata)
	{
		// 文件名
		std::string filename = metadata.filePath.filename().string();
		ImGui::Text("%s", filename.c_str());

		// 资源类型标签
		ImGui::SameLine();
		const char* typeStr = Utils::AssetTypeToString(metadata.type);
		ImGui::TextDisabled("[%s]", typeStr);

		// 相对路径
		ImGui::TextDisabled("Path: %s", metadata.filePath.string().c_str());

		// Handle ID
		ImGui::TextDisabled("Handle: %llu", (unsigned long long)(uint64_t)metadata.handle);

		// 加载状态
		ImGui::TextDisabled("Loaded: %s", metadata.isDataLoaded ? "Yes" : "No");

		// 操作按钮
		ImGui::Spacing();

		if (ImGui::Button("Reload"))
			assetManager.ReloadData(metadata.handle);

		ImGui::SameLine();

		if (ImGui::Button("Open"))
		{
			if (m_assetWorkflow)
			{
				std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);
				m_assetWorkflow->OpenAsset(fullPath);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Show in Explorer"))
		{
			if (m_assetWorkflow)
			{
				std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);
				m_assetWorkflow->RevealInExplorer(fullPath.parent_path());
			}
		}
	}

	void AssetInspectorPanel::DrawTextureInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata)
	{
		ImGui::Text("Texture Properties");
		ImGui::Spacing();

		Ref<Texture2D> texture = assetManager.GetAssetAs<Texture2D>(metadata.handle);
		if (!texture)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Failed to load texture data.");
			return;
		}

		uint32_t width = texture->GetWidth();
		uint32_t height = texture->GetHeight();

		ImGui::Text("Width:  %u px", width);
		ImGui::Text("Height: %u px", height);

		// 文件大小
		std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024 * 1024)
				ImGui::Text("File Size: %.2f MB", fileSize / (1024.0f * 1024.0f));
			else if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		// 纹理预览
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("Preview");
		ImGui::Spacing();

		float previewMaxWidth = ImGui::GetContentRegionAvail().x;
		float previewMaxHeight = 256.0f;
		float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

		float previewWidth = previewMaxWidth;
		float previewHeight = previewWidth / aspect;
		if (previewHeight > previewMaxHeight)
		{
			previewHeight = previewMaxHeight;
			previewWidth = previewHeight * aspect;
		}

		ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)texture->GetRendererID());
		ImGui::Image(texID, ImVec2{ previewWidth, previewHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	}

	void AssetInspectorPanel::DrawSceneInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata)
	{
		ImGui::Text("Scene Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		// 最后修改时间
		auto lastWrite = std::filesystem::last_write_time(fullPath, ec);
		if (!ec)
			ImGui::Text("Last Modified: %s", FormatLastWriteTime(lastWrite).c_str());

		ImGui::Spacing();

		if (ImGui::Button("Open Scene"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenAsset(fullPath);
		}
	}

	void AssetInspectorPanel::DrawScriptInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata)
	{
		ImGui::Text("Lua Script Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		// 最后修改时间
		auto lastWrite = std::filesystem::last_write_time(fullPath, ec);
		if (!ec)
			ImGui::Text("Last Modified: %s", FormatLastWriteTime(lastWrite).c_str());

		ImGui::Spacing();

		if (ImGui::Button("Open in Editor"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenFileExternal(fullPath);
		}

		ImGui::SameLine();

		if (ImGui::Button("Reload Script"))
			assetManager.ReloadData(metadata.handle);
	}

	void AssetInspectorPanel::DrawShaderInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata)
	{
		ImGui::Text("Shader Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		ImGui::Spacing();

		if (ImGui::Button("Open in Editor"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenFileExternal(fullPath);
		}
	}

	void AssetInspectorPanel::DrawFontInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata)
	{
		ImGui::Text("Font Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = assetManager.GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}
	}

}
