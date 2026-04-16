#pragma once

namespace Yuicy {

	// 编辑器视口显示设置
	// 控制 Overlay 开关、网格参数、吸附参数等。
	// TODO: 这些设置可以在后续持久化到编辑器配置文件中。
	struct EditorViewportSettings
	{
		// Overlay 显示开关
		bool showGrid              = true;
		bool showOrigin            = true;
		bool showCameraBounds      = true;
		bool showColliders         = true;
		bool showSelectionBounds   = true;
		bool showRelationshipLines = false;

		// 网格参数
		float gridMajorStep = 1.0f;
		float gridMinorStep = 0.25f;

		// 吸附参数
		bool enableTranslationSnap = false;
		bool enableRotationSnap    = false;
		bool enableScaleSnap       = false;

		float translationSnapValue = 0.5f;
		float rotationSnapValue    = 15.0f;
		float scaleSnapValue       = 0.1f;
	};

}
