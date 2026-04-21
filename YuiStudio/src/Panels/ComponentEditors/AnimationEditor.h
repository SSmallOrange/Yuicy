#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Asset/Asset.h"

#include <string>

namespace Yuicy {

	struct AnimationComponent;
	class EditorDirtyTracker;
	class Texture2D;

	class AnimationEditor
	{
	public:
		AnimationEditor() = default;

		void Draw(AnimationComponent& component, EditorDirtyTracker* dirtyTracker);

	private:
		void EnsureIconsLoaded();

		// Animation preview
		bool m_previewPlaying = false;
		float m_previewTimer = 0.0f;
		int m_previewFrame = 0;

		// 从 Sprite Sheet 添加帧数据
		AssetHandle m_sheetTextureHandle = 0;
		int m_sheetCellWidth = 32;
		int m_sheetCellHeight = 32;
		int m_sheetStartX = 0;
		int m_sheetStartY = 0;
		int m_sheetFrameCount = 1;
		bool m_sheetHorizontal = true;
		std::string m_sheetTargetClip;  // 目标 Clip 名称
		bool m_openSheetPopup = false;

		// Icons (lazy loaded)
		bool m_iconsLoaded = false;
		Ref<Texture2D> m_addIcon;
		Ref<Texture2D> m_removeIcon;
		Ref<Texture2D> m_upIcon;
		Ref<Texture2D> m_downIcon;
		Ref<Texture2D> m_closeIcon;
	};

}
