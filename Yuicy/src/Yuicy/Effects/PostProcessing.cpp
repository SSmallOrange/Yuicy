#include "pch.h"
#include "Yuicy/Core/Application.h"
#include "Yuicy/Effects/PostProcessing.h"

#include <algorithm>

namespace Yuicy {

	PostProcessing::PostProcessing()
	{
	}

	void PostProcessing::Init()
	{
		if (m_initialized)
			return;

		auto& window = Application::Get().GetWindow();
		m_pipeline = CreateRef<PostProcessPipeline>();
		m_pipeline->Init(window.GetWidth(), window.GetHeight());

		auto raindrops = IRaindropsPass::Create();
		auto raindropsAsPass = std::dynamic_pointer_cast<IPostProcessPass>(raindrops);
		m_pipeline->AddPass(raindropsAsPass);
		raindropsAsPass->Init();
		m_raindropsPass = raindrops.get();

		auto vignette = IVignettePass::Create();
		auto vignetteAsPass = std::dynamic_pointer_cast<IPostProcessPass>(vignette);
		m_pipeline->AddPass(vignetteAsPass);
		vignetteAsPass->Init();
		m_vignettePass = vignette.get();

		auto lightingCompose = ILightingComposePass::Create();
		auto lightingComposeAsPass = std::dynamic_pointer_cast<IPostProcessPass>(lightingCompose);
		m_pipeline->AddPass(lightingComposeAsPass);
		lightingComposeAsPass->Init();
		m_lightingComposePass = lightingCompose.get();

		m_initialized = true;

		YUICY_CORE_INFO("PostProcessing: Initialized (Multi-Pass Pipeline)");
	}

	void PostProcessing::Shutdown()
	{
		if (m_pipeline)
		{
			m_pipeline->Shutdown();
			m_pipeline = nullptr;
		}

		m_raindropsPass = nullptr;
		m_vignettePass = nullptr;
		m_lightingComposePass = nullptr;

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
		m_fadeActive = false;
	}

	void PostProcessing::Resize(uint32_t width, uint32_t height)
	{
		if (m_pipeline)
			m_pipeline->Resize(width, height);
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

		std::sort(sortedLayers.begin(), sortedLayers.end(),
			[](const EffectLayer* a, const EffectLayer* b) {
				return a->priority < b->priority;
			});

		for (const EffectLayer* layer : sortedLayers)
		{
			const auto& cfg = layer->config;

			if (cfg.vignetteEnabled)
			{
				merged.vignetteEnabled = true;
				merged.vignetteIntensity = glm::max(merged.vignetteIntensity, cfg.vignetteIntensity);
				merged.vignetteRadius = glm::min(merged.vignetteRadius, cfg.vignetteRadius);
			}

			if (cfg.raindropsEnabled)
			{
				merged.raindropsEnabled = true;
				merged.raindropsIntensity = glm::max(merged.raindropsIntensity, cfg.raindropsIntensity);
			}
		}

		m_finalConfig = merged;
	}

	void PostProcessing::FadeTo(const PostProcessConfig& target, float duration)
	{
		m_fadeActive = true;
		m_fadeTimer = 0.0f;
		m_fadeDuration = duration;
		m_fadeStartConfig = m_finalConfig;
		m_fadeTargetConfig = target;
	}

	void PostProcessing::SetVignetteEnabled(bool enabled) { m_finalConfig.vignetteEnabled = enabled; }
	void PostProcessing::SetVignetteIntensity(float intensity) { m_finalConfig.vignetteIntensity = glm::clamp(intensity, 0.0f, 1.0f); }
	void PostProcessing::SetVignetteRadius(float radius) { m_finalConfig.vignetteRadius = glm::clamp(radius, 0.0f, 1.0f); }
	void PostProcessing::SetRaindropsEnabled(bool enabled) { m_finalConfig.raindropsEnabled = enabled; }
	void PostProcessing::SetRaindropsIntensity(float intensity) { m_finalConfig.raindropsIntensity = glm::clamp(intensity, 0.0f, 1.0f); }

	void PostProcessing::SetLightingEnabled(bool enabled) { m_finalConfig.lightingEnabled = enabled; }
	void PostProcessing::SetLightMapTextureID(uint32_t textureID) { m_finalConfig.lightMapTextureID = textureID; }

	void PostProcessing::OnUpdate(Timestep ts)
	{
		float dt = static_cast<float>(ts);

		if (m_finalConfig.raindropsEnabled)
		{
			m_finalConfig.raindropsTime += dt;
		}

		if (m_fadeActive)
		{
			m_fadeTimer += dt;
			float t = glm::clamp(m_fadeTimer / m_fadeDuration, 0.0f, 1.0f);

			float smoothT = t * t * (3.0f - 2.0f * t);

			m_finalConfig.vignetteIntensity = glm::mix(m_fadeStartConfig.vignetteIntensity, m_fadeTargetConfig.vignetteIntensity, smoothT);

			if (t >= 1.0f)
			{
				m_fadeActive = false;
				m_finalConfig = m_fadeTargetConfig;
			}
		}
	}

	void PostProcessing::SyncConfigToPasses()
	{
		if (!m_initialized)
			return;

		if (m_raindropsPass)
		{
			m_raindropsPass->SetEnabled(m_finalConfig.raindropsEnabled && m_finalConfig.raindropsIntensity > 0.0f);
			m_raindropsPass->SetIntensity(m_finalConfig.raindropsIntensity);
			m_raindropsPass->SetTime(m_finalConfig.raindropsTime);
		}

		if (m_vignettePass)
		{
			m_vignettePass->SetEnabled(m_finalConfig.vignetteEnabled && m_finalConfig.vignetteIntensity > 0.0f);
			m_vignettePass->SetIntensity(m_finalConfig.vignetteIntensity);
			m_vignettePass->SetRadius(m_finalConfig.vignetteRadius);
		}

		if (m_lightingComposePass)
		{
			m_lightingComposePass->SetEnabled(m_finalConfig.lightingEnabled && m_finalConfig.lightMapTextureID != 0);
			m_lightingComposePass->SetLightMapTextureID(m_finalConfig.lightMapTextureID);
		}
	}

	void PostProcessing::Render(const Ref<Framebuffer>& framebuffer)
	{
		if (!m_initialized || !m_pipeline)
		{
			YUICY_CORE_ERROR("PostProcessing: Not initialized!");
			return;
		}

		SyncConfigToPasses();

		m_pipeline->Execute(framebuffer->GetColorAttachmentRendererID(0));
	}
}
