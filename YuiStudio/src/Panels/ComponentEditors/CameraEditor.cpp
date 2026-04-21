#include "CameraEditor.h"

#include "imgui/imgui.h"

#include <algorithm>

namespace Yuicy {

	// 预设分辨率
	struct ResolutionPreset
	{
		const char* name;
		int width;
		int height;
	};

	static const ResolutionPreset s_presets[] = {
		{ "1920x1080 (16:9)",   1920, 1080 },
		{ "1280x720  (16:9)",   1280,  720 },
		{ "2560x1440 (16:9)",   2560, 1440 },
		{ "1920x1200 (16:10)",  1920, 1200 },
		{ "1024x768  (4:3)",    1024,  768 },
		{ "800x600   (4:3)",     800,  600 },
		{ "1080x1080 (1:1)",    1080, 1080 },
		{ "Custom",                0,    0 },
	};

	static constexpr int s_presetCount = sizeof(s_presets) / sizeof(s_presets[0]);

	void CameraEditor::Draw(CameraComponent& component, EditorDirtyTracker* dt)
	{
		auto& camera = component.Camera;

		if (ImGui::Checkbox("Primary", &component.Primary))
			if (dt) dt->MarkSceneDirty();

		float orthoSize = camera.GetOrthographicSize();
		if (ImGui::DragFloat("Orthographic Size", &orthoSize, 0.1f))
		{
			camera.SetOrthographicSize(orthoSize);
			if (dt) dt->MarkSceneDirty();
		}

		float orthoNear = camera.GetOrthographicNearClip();
		if (ImGui::DragFloat("Near Clip", &orthoNear, 0.1f))
		{
			camera.SetOrthographicNearClip(orthoNear);
			if (dt) dt->MarkSceneDirty();
		}

		float orthoFar = camera.GetOrthographicFarClip();
		if (ImGui::DragFloat("Far Clip", &orthoFar, 0.1f))
		{
			camera.SetOrthographicFarClip(orthoFar);
			if (dt) dt->MarkSceneDirty();
		}

		if (ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio))
		{
			if (component.FixedAspectRatio && component.DesignHeight > 0)
			{
				float fixedAspect = (float)component.DesignWidth / (float)component.DesignHeight;
				camera.SetFixedAspectRatio(fixedAspect);
			}
			if (dt) dt->MarkSceneDirty();
		}

		// 设计分辨率（FixedAspectRatio 启用时显示）
		if (component.FixedAspectRatio)
		{
			ImGui::Indent();

			// 查找当前匹配的预设
			int currentPreset = s_presetCount - 1;
			for (int i = 0; i < currentPreset; i++)
			{
				if (s_presets[i].width == component.DesignWidth && s_presets[i].height == component.DesignHeight)
				{
					currentPreset = i;
					break;
				}
			}

			if (ImGui::BeginCombo("Resolution", s_presets[currentPreset].name))
			{
				for (int i = 0; i < s_presetCount; i++)
				{
					bool isSelected = (currentPreset == i);
					if (ImGui::Selectable(s_presets[i].name, isSelected))
					{
						if (s_presets[i].width > 0 && s_presets[i].height > 0)
						{
							component.DesignWidth = s_presets[i].width;
							component.DesignHeight = s_presets[i].height;
							float fixedAspect = (float)component.DesignWidth / (float)component.DesignHeight;
							camera.SetFixedAspectRatio(fixedAspect);
							if (dt) dt->MarkSceneDirty();
						}
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// 自定义分辨率输入
			bool widthChanged = ImGui::InputInt("Width", &component.DesignWidth, 1, 100);
			bool heightChanged = ImGui::InputInt("Height", &component.DesignHeight, 1, 100);
			if (widthChanged || heightChanged)
			{
				component.DesignWidth = std::max(component.DesignWidth, 1);
				component.DesignHeight = std::max(component.DesignHeight, 1);
				float fixedAspect = (float)component.DesignWidth / (float)component.DesignHeight;
				camera.SetFixedAspectRatio(fixedAspect);
				if (dt) dt->MarkSceneDirty();
			}

			float displayAspect = (float)component.DesignWidth / (float)std::max(component.DesignHeight, 1);
			ImGui::Text("Aspect Ratio: %.3f", displayAspect);

			ImGui::Unindent();
		}

		ImGui::Separator();

		// 安全框
		if (ImGui::Checkbox("Show Safe Area", &component.ShowSafeArea))
			if (dt) dt->MarkSceneDirty();

		if (component.ShowSafeArea)
		{
			ImGui::Indent();
			if (ImGui::SliderFloat("Margin", &component.SafeAreaMargin, 0.0f, 0.45f, "%.2f"))
				if (dt) dt->MarkSceneDirty();
			ImGui::Unindent();
		}
	}

}
