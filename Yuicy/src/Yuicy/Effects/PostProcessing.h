#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Effects/PostProcessTypes.h"
#include "Yuicy/Renderer/Shader.h"
#include "Yuicy/Renderer/VertexArray.h"
#include "Yuicy/Renderer/Framebuffer.h"
#include "Yuicy/Renderer/PostProcessPass.h"

#include <string>
#include <unordered_map>

namespace Yuicy {

	class PostProcessing
	{
	public:
		PostProcessing();
		~PostProcessing() = default;

		void Init();
		void Shutdown();
		void Reset();

	public:
		void OnUpdate(Timestep ts);
		void Render(const Ref<Framebuffer>& framebuffer);

	public:
		// Config
		void SetConfig(const PostProcessConfig& config);
		PostProcessConfig& GetConfig() { return m_finalConfig; }
		const PostProcessConfig& GetConfig() const { return m_finalConfig; }

		void PushEffect(const std::string& name, const PostProcessConfig& config, int priority = 0);
		void PopEffect(const std::string& name);
		bool HasEffect(const std::string& name) const;

		PostProcessConfig* GetEffect(const std::string& name);

		void ClearAllEffects();

	public:
		void TriggerFlash(float intensity = 0.8f, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }, float duration = 0.1f);

		// 渐变到目标配置
		void FadeTo(const PostProcessConfig& target, float duration = 1.0f);

	public:
		void SetAmbientTint(const glm::vec4& tint);
		void SetBrightness(float brightness);
		void SetContrast(float contrast);
		void SetSaturation(float saturation);

		void SetFogEnabled(bool enabled);
		void SetFogColor(const glm::vec4& color);
		void SetFogDensity(float density);

		void SetVignetteEnabled(bool enabled);
		void SetVignetteIntensity(float intensity);
		void SetVignetteRadius(float radius);

	private:
		void MergeEffects();  // 合并所有效果层到 m_finalConfig

	private:
		Ref<PostProcessPass> m_renderPass;

		PostProcessConfig m_finalConfig;		// 最终后期配置
		struct EffectLayer
		{
			PostProcessConfig config;
			int priority = 0;
		};
		std::unordered_map<std::string, EffectLayer> m_effectLayers;

		// 闪光效果状态
		bool m_flashActive = false;
		float m_flashTimer = 0.0f;
		float m_flashDuration = 0.1f;
		float m_flashStartIntensity = 0.0f;
		glm::vec3 m_flashColor = { 1.0f, 1.0f, 1.0f };

		// 渐变效果状态o
		bool m_fadeActive = false;
		float m_fadeTimer = 0.0f;
		float m_fadeDuration = 1.0f;
		PostProcessConfig m_fadeStartConfig;
		PostProcessConfig m_fadeTargetConfig;

		bool m_initialized = false;
	};

}