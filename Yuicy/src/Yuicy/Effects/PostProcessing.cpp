#include "pch.h"
#include "PostProcessing.h"

#include <algorithm>

namespace Yuicy {

	PostProcessing::PostProcessing()
	{
	}

	void PostProcessing::Init()
	{
		if (m_initialized)
			return;

		m_renderPass = PostProcessPass::Create();
		m_renderPass->Init();
		m_initialized = true;

		YUICY_CORE_INFO("PostProcessing: Initialized");
	}

	void PostProcessing::Shutdown()
	{
		if (m_renderPass)
		{
			m_renderPass->Shutdown();
			m_renderPass = nullptr;
		}
		m_effectLayers.clear();
		m_initialized = false;
	}

	void PostProcessing::SetConfig(const PostProcessConfig& config)
	{
		m_finalConfig = config;
	}

	void PostProcessing::Reset()
	{
		m_finalConfig = PostProcessConfig();
		m_effectLayers.clear();
		m_flashActive = false;
		m_fadeActive = false;
	}

	void PostProcessing::PushEffect(const std::string& name, const PostProcessConfig& config, int priority)
	{
		EffectLayer layer;
		layer.config = config;
		layer.config.sourceName = name;
		layer.priority = priority;
		m_effectLayers[name] = layer;

		MergeEffects();
	}

	void PostProcessing::PopEffect(const std::string& name)
	{
		auto it = m_effectLayers.find(name);
		if (it != m_effectLayers.end())
		{
			m_effectLayers.erase(it);
			MergeEffects();
		}
	}

	bool PostProcessing::HasEffect(const std::string& name) const
	{
		return m_effectLayers.find(name) != m_effectLayers.end();
	}

	PostProcessConfig* PostProcessing::GetEffect(const std::string& name)
	{
		auto it = m_effectLayers.find(name);
		if (it != m_effectLayers.end())
			return &it->second.config;
		return nullptr;
	}

	void PostProcessing::ClearAllEffects()
	{
		m_effectLayers.clear();
		m_finalConfig = PostProcessConfig();
	}

	void PostProcessing::MergeEffects()
	{
		PostProcessConfig merged;

		std::vector<EffectLayer*> sortedLayers;
		for (auto& [name, layer] : m_effectLayers)
		{
			sortedLayers.push_back(&layer);
		}

		// 优先级排序
		std::sort(sortedLayers.begin(), sortedLayers.end(),
			[](const EffectLayer* a, const EffectLayer* b) {
				return a->priority < b->priority;
			});

		for (const EffectLayer* layer : sortedLayers)
		{
			const auto& cfg = layer->config;

			// 颜色效果
			merged.ambientTint *= cfg.ambientTint;
			merged.brightness *= cfg.brightness;
			merged.contrast *= cfg.contrast;
			merged.saturation *= cfg.saturation;

			// 雾效（覆盖处理）
			if (cfg.fogEnabled)
			{
				merged.fogEnabled = true;
				merged.fogColor = cfg.fogColor;
				merged.fogDensity = glm::max(merged.fogDensity, cfg.fogDensity);
			}

			// 暗角：取最大值
			if (cfg.vignetteEnabled)
			{
				merged.vignetteEnabled = true;
				merged.vignetteIntensity = glm::max(merged.vignetteIntensity, cfg.vignetteIntensity);
				merged.vignetteRadius = glm::min(merged.vignetteRadius, cfg.vignetteRadius);
			}

			// 闪光：取最大值
			if (cfg.flashEnabled)
			{
				merged.flashEnabled = true;
				merged.flashIntensity = glm::max(merged.flashIntensity, cfg.flashIntensity);
				merged.flashColor = cfg.flashColor;
			}
		}

		m_finalConfig = merged;
	}

	void PostProcessing::TriggerFlash(float intensity, const glm::vec3& color, float duration)
	{
		m_flashActive = true;
		m_flashTimer = 0.0f;
		m_flashDuration = duration;
		m_flashStartIntensity = intensity;
		m_flashColor = color;
	}

	void PostProcessing::FadeTo(const PostProcessConfig& target, float duration)
	{
		m_fadeActive = true;
		m_fadeTimer = 0.0f;
		m_fadeDuration = duration;
		m_fadeStartConfig = m_finalConfig;
		m_fadeTargetConfig = target;
	}

	void PostProcessing::SetAmbientTint(const glm::vec4& tint) { m_finalConfig.ambientTint = tint; }
	void PostProcessing::SetBrightness(float brightness) { m_finalConfig.brightness = glm::clamp(brightness, 0.0f, 2.0f); }
	void PostProcessing::SetContrast(float contrast) { m_finalConfig.contrast = glm::clamp(contrast, 0.5f, 1.5f); }
	void PostProcessing::SetSaturation(float saturation) { m_finalConfig.saturation = glm::clamp(saturation, 0.0f, 2.0f); }
	void PostProcessing::SetFogEnabled(bool enabled) { m_finalConfig.fogEnabled = enabled; }
	void PostProcessing::SetFogColor(const glm::vec4& color) { m_finalConfig.fogColor = color; }
	void PostProcessing::SetFogDensity(float density) { m_finalConfig.fogDensity = glm::clamp(density, 0.0f, 1.0f); }
	void PostProcessing::SetVignetteEnabled(bool enabled) { m_finalConfig.vignetteEnabled = enabled; }
	void PostProcessing::SetVignetteIntensity(float intensity) { m_finalConfig.vignetteIntensity = glm::clamp(intensity, 0.0f, 1.0f); }
	void PostProcessing::SetVignetteRadius(float radius) { m_finalConfig.vignetteRadius = glm::clamp(radius, 0.0f, 1.0f); }

	void PostProcessing::OnUpdate(Timestep ts)
	{
		float dt = static_cast<float>(ts);

		// 处理闪光衰减
		if (m_flashActive)
		{
			m_flashTimer += dt;
			float progress = m_flashTimer / m_flashDuration;

			if (progress >= 1.0f)
			{
				m_flashActive = false;
				m_finalConfig.flashEnabled = false;
				m_finalConfig.flashIntensity = 0.0f;
			}
			else
			{
				// 快速衰减
				float fadeOut = 1.0f - progress * progress;
				m_finalConfig.flashEnabled = true;
				m_finalConfig.flashIntensity = m_flashStartIntensity * fadeOut;
				m_finalConfig.flashColor = m_flashColor;
			}
		}

		// 处理渐变效果
		if (m_fadeActive)
		{
			m_fadeTimer += dt;
			float t = glm::clamp(m_fadeTimer / m_fadeDuration, 0.0f, 1.0f);

			// 平滑插值
			float smoothT = t * t * (3.0f - 2.0f * t);

			// 插值所有参数
			m_finalConfig.ambientTint = glm::mix(m_fadeStartConfig.ambientTint, m_fadeTargetConfig.ambientTint, smoothT);
			m_finalConfig.brightness = glm::mix(m_fadeStartConfig.brightness, m_fadeTargetConfig.brightness, smoothT);
			m_finalConfig.contrast = glm::mix(m_fadeStartConfig.contrast, m_fadeTargetConfig.contrast, smoothT);
			m_finalConfig.saturation = glm::mix(m_fadeStartConfig.saturation, m_fadeTargetConfig.saturation, smoothT);
			m_finalConfig.fogDensity = glm::mix(m_fadeStartConfig.fogDensity, m_fadeTargetConfig.fogDensity, smoothT);
			m_finalConfig.vignetteIntensity = glm::mix(m_fadeStartConfig.vignetteIntensity, m_fadeTargetConfig.vignetteIntensity, smoothT);

			if (t >= 1.0f)
			{
				m_fadeActive = false;
				m_finalConfig = m_fadeTargetConfig;
			}
		}
	}

	void PostProcessing::Render(const Ref<Framebuffer>& framebuffer)
	{
		if (!m_initialized || !m_renderPass)
		{
			YUICY_CORE_ERROR("PostProcessing: Not initialized!");
			return;
		}

		m_renderPass->Execute(framebuffer, m_finalConfig);
	}
}
