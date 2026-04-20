#pragma once

#include "Yuicy.h"
#include "Yuicy/Asset/AssetMetadata.h"

namespace Yuicy {

	struct EditorContext;
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
		void DrawAssetHeader(const AssetMetadata& metadata);
		void DrawTextureInspector(const AssetMetadata& metadata);
		void DrawSceneInspector(const AssetMetadata& metadata);
		void DrawScriptInspector(const AssetMetadata& metadata);
		void DrawShaderInspector(const AssetMetadata& metadata);
		void DrawFontInspector(const AssetMetadata& metadata);

	private:
		EditorContext* m_context = nullptr;
		EditorAssetWorkflow* m_assetWorkflow = nullptr;
	};

}
