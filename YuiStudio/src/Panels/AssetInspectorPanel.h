#pragma once

#include "Yuicy/Asset/AssetMetadata.h"

namespace Yuicy {

	struct EditorContext;
	class EditorAssetManager;
	class EditorAssetWorkflow;

	// 资源检查面板
	// 显示当前选中资源的元数据和属性信息
	class AssetInspectorPanel
	{
	public:
		AssetInspectorPanel() = default;

		void SetContext(EditorContext* context) { m_context = context; }
		void SetAssetWorkflow(EditorAssetWorkflow* workflow) { m_assetWorkflow = workflow; }

		void OnImGuiRender();

	private:
		void DrawNoSelection();
		void DrawAssetHeader(EditorAssetManager& assetManager, const AssetMetadata& metadata);
		void DrawTextureInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata);
		void DrawSceneInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata);
		void DrawScriptInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata);
		void DrawShaderInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata);
		void DrawFontInspector(EditorAssetManager& assetManager, const AssetMetadata& metadata);

	private:
		EditorContext* m_context = nullptr;
		EditorAssetWorkflow* m_assetWorkflow = nullptr;
	};

}
