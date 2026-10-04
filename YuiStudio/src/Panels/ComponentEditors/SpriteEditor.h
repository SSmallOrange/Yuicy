#pragma once

namespace Yuicy {

	struct SpriteRendererComponent;
	struct EditorContext;
	class EditorDirtyTracker;

	class SpriteEditor
	{
	public:
		SpriteEditor() = default;

		void SetEditorContext(const EditorContext* context) { m_editorContext = context; }

		// 没有打开项目时不能指定纹理，排序层下拉框列出 SortingLayerConfig 的默认配置
		void Draw(SpriteRendererComponent& component, EditorDirtyTracker* dirtyTracker);

	private:
		const EditorContext* m_editorContext = nullptr;
	};

}
