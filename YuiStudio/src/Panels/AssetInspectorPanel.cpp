#include "pch.h"

#include "AssetInspectorPanel.h"

#include "../Editor/EditorContext.h"
#include "../Editor/EditorAssetWorkflow.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Asset/AssetTypes.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Project/Project.h"

#include <filesystem>

namespace Yuicy {

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
		auto assetManager = Project::GetEditorAssetManager();
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

		DrawAssetHeader(metadata);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		switch (metadata.type)
		{
		case AssetType::Texture:  DrawTextureInspector(metadata); break;
		case AssetType::Scene:    DrawSceneInspector(metadata);   break;
		case AssetType::LuaScript: DrawScriptInspector(metadata); break;
		case AssetType::Shader:   DrawShaderInspector(metadata);  break;
		case AssetType::Font:     DrawFontInspector(metadata);    break;
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

	void AssetInspectorPanel::DrawAssetHeader(const AssetMetadata& metadata)
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
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
				assetManager->ReloadData(metadata.handle);
		}

		ImGui::SameLine();

		if (ImGui::Button("Open"))
		{
			if (m_assetWorkflow)
			{
				std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
				m_assetWorkflow->OpenAsset(fullPath);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Show in Explorer"))
		{
			if (m_assetWorkflow)
			{
				std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
				m_assetWorkflow->RevealInExplorer(fullPath.parent_path());
			}
		}
	}

	void AssetInspectorPanel::DrawTextureInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Texture Properties");
		ImGui::Spacing();

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
			return;

		Ref<Texture2D> texture = assetManager->GetAsset<Texture2D>(metadata.handle);
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
		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
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

	void AssetInspectorPanel::DrawSceneInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Scene Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

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
		{
			auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(lastWrite);
			auto time = std::chrono::system_clock::to_time_t(sctp);
			std::tm tm{};
			localtime_s(&tm, &time);
			char timeBuf[64];
			std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
			ImGui::Text("Last Modified: %s", timeBuf);
		}

		ImGui::Spacing();

		if (ImGui::Button("Open Scene"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenAsset(fullPath);
		}
	}

	void AssetInspectorPanel::DrawScriptInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Lua Script Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

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
		{
			auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(lastWrite);
			auto time = std::chrono::system_clock::to_time_t(sctp);
			std::tm tm{};
			localtime_s(&tm, &time);
			char timeBuf[64];
			std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
			ImGui::Text("Last Modified: %s", timeBuf);
		}

		ImGui::Spacing();

		if (ImGui::Button("Open in Editor"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenFileExternal(fullPath);
		}

		ImGui::SameLine();

		if (ImGui::Button("Reload Script"))
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
				assetManager->ReloadData(metadata.handle);
		}
	}

	void AssetInspectorPanel::DrawShaderInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Shader Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

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

	void AssetInspectorPanel::DrawFontInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Font Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

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
