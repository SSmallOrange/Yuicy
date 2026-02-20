#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Effects/PostProcessTypes.h"
#include "Yuicy/Effects/PostProcessPipeline.h"
#include "Yuicy/Renderer/Framebuffer.h"
#include "Yuicy/Renderer/IRaindropsPass.h"
#include "Yuicy/Renderer/IVignettePass.h"
#include "Yuicy/Renderer/ILightingComposePass.h"

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

		void Resize(uint32_t width, uint32_t height);

	public:
		void SetConfig(const PostProcessConfig& config);
		PostProcessConfig& GetConfig() { return m_finalConfig; }
		const PostProcessConfig& GetConfig() const { return m_finalConfig; }

		void PushEffect(const std::string& name, const PostProcessConfig& config, int priority = 0);
		void PopEffect(const std::string& name);
		bool HasEffect(const std::string& name) const;

		PostProcessConfig* GetEffect(const std::string& name);

		void ClearAllEffects();

	public:
		void FadeTo(const PostProcessConfig& target, float duration = 1.0f);

		void SetVignetteEnabled(bool enabled);
		void SetVignetteIntensity(float intensity);
		void SetVignetteRadius(float radius);

		void SetRaindropsEnabled(bool enabled);
		void SetRaindropsIntensity(float intensity);

		void SetLightingEnabled(bool enabled);
		void SetLightMapTextureID(uint32_t textureID);

	private:
		void MergeEffects();
		void SyncConfigToPasses();

	private:
		Ref<PostProcessPipeline> m_pipeline;

		IRaindropsPass* m_raindropsPass = nullptr;
		IVignettePass* m_vignettePass = nullptr;
		ILightingComposePass* m_lightingComposePass = nullptr;

		PostProcessConfig m_finalConfig;
		struct EffectLayer
		{
			PostProcessConfig config;
			int priority = 0;
		};
		std::unordered_map<std::string, EffectLayer> m_effectLayers;

		bool m_fadeActive = false;
		float m_fadeTimer = 0.0f;
		float m_fadeDuration = 1.0f;
		PostProcessConfig m_fadeStartConfig;
		PostProcessConfig m_fadeTargetConfig;

		bool m_initialized = false;
	};

}