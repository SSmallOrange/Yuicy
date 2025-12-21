#pragma once

#include "Yuicy.h"

// 相机跟随脚本
class CameraController : public Yuicy::ScriptableEntity
{
public:
	Yuicy::Entity Target;  // 跟随目标
	float SmoothSpeed = 5.0f;
	glm::vec2 Offset = { 0.0f, 1.0f };  // 相机偏移

	void OnUpdate(Yuicy::Timestep ts) override
	{
		if (!Target)
			return;

		auto& cameraTransform = GetComponent<Yuicy::TransformComponent>();
		auto& targetTransform = Target.GetComponent<Yuicy::TransformComponent>();

		// 目标位置（加上偏移）
		glm::vec3 targetPos = {
			targetTransform.Translation.x + Offset.x,
			targetTransform.Translation.y + Offset.y,
			cameraTransform.Translation.z  // 保持 Z 不变
		};

		// 平滑跟随
		cameraTransform.Translation = glm::mix(
			cameraTransform.Translation,
			targetPos,
			SmoothSpeed * (float)ts
		);
	}
};