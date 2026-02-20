#include "pch.h"
#include "WeatherPresets.h"

namespace Yuicy {

	bool WeatherPresets::s_initialized = false;
	std::unordered_map<std::string, WeatherConfig> WeatherPresets::s_presets;

	// ============================================================
	// 初始化内置预设
	// ============================================================
	void WeatherPresets::InitializeBuiltInPresets()
	{
		if (s_initialized) return;
		s_initialized = true;

		RegisterPreset("LightRain", LightRain());
		RegisterPreset("Rain", Rain());
		RegisterPreset("HeavyRain", HeavyRain());
		RegisterPreset("Storm", Storm());
	}

	// ============================================================
	// 预设获取接口
	// ============================================================

	WeatherConfig WeatherPresets::Get(WeatherType type, WeatherIntensity intensity)
	{
		InitializeBuiltInPresets();

		switch (type)
		{
		case WeatherType::Rain:
			switch (intensity)
			{
			case WeatherIntensity::Light:   return LightRain();
			case WeatherIntensity::Normal:  return Rain();
			case WeatherIntensity::Heavy:   return HeavyRain();
			case WeatherIntensity::Extreme: return Storm();
			}
			break;

		default:
			break;
		}

		WeatherConfig none;
		none.type = WeatherType::None;
		none.name = "None";
		return none;
	}

	WeatherConfig WeatherPresets::GetByName(const std::string& name)
	{
		InitializeBuiltInPresets();

		auto it = s_presets.find(name);
		if (it != s_presets.end())
			return it->second;

		WeatherConfig none;
		none.type = WeatherType::None;
		none.name = "None";
		return none;
	}

	void WeatherPresets::RegisterPreset(const std::string& name, const WeatherConfig& config)
	{
		WeatherConfig preset = config;
		preset.name = name;
		s_presets[name] = preset;
	}

	bool WeatherPresets::HasPreset(const std::string& name)
	{
		InitializeBuiltInPresets();
		return s_presets.find(name) != s_presets.end();
	}

	std::vector<std::string> WeatherPresets::GetAllPresetNames()
	{
		InitializeBuiltInPresets();

		std::vector<std::string> names;
		names.reserve(s_presets.size());
		for (const auto& [name, config] : s_presets)
			names.push_back(name);
		return names;
	}

	WeatherConfig WeatherPresets::LightRain()
	{
		WeatherConfig config;
		config.type = WeatherType::Rain;
		config.name = "LightRain";
		config.intensity = 0.5f;

		config.particles.spawnRate = 100.0f;
		config.particles.particleLifetime = 2.5f;
		config.particles.velocity = { 0.0f, -8.0f };
		config.particles.velocityVariation = { 0.3f, 1.0f };
		config.particles.sizeMin = 0.015f;
		config.particles.sizeMax = 0.03f;
		config.particles.colorStart = { 0.75f, 0.82f, 0.95f, 0.4f };
		config.particles.colorEnd = { 0.75f, 0.82f, 0.95f, 0.0f };
		config.particles.motionType = ParticleMotion::Linear;

		config.transition.duration = 3.0f;

		return config;
	}

	WeatherConfig WeatherPresets::Rain()
	{
		WeatherConfig config;
		config.type = WeatherType::Rain;
		config.name = "Rain";
		config.intensity = 1.0f;

		config.particles.spawnRate = 300.0f;
		config.particles.particleLifetime = 2.0f;
		config.particles.velocity = { 0.0f, -12.0f };
		config.particles.velocityVariation = { 0.5f, 2.0f };
		config.particles.sizeMin = 0.02f;
		config.particles.sizeMax = 0.04f;
		config.particles.colorStart = { 0.7f, 0.8f, 1.0f, 0.6f };
		config.particles.colorEnd = { 0.7f, 0.8f, 1.0f, 0.0f };
		config.particles.motionType = ParticleMotion::Linear;
		config.particles.spawnWidthMultiplier = 1.5f;
		config.particles.spawnHeightOffset = 0.6f;

		// 物理雨滴配置
		config.particles.enablePhysics = true;
		config.particles.physicsRatio = 0.02f;
		config.particles.splashConfig.colorStart = { 0.6f, 0.7f, 0.9f, 0.7f };
		config.particles.splashConfig.colorEnd = { 0.5f, 0.6f, 0.8f, 0.0f };
		config.particles.splashConfig.sizeMin = 0.02f;
		config.particles.splashConfig.sizeMax = 0.04f;
		config.particles.splashConfig.speedMin = 1.5f;
		config.particles.splashConfig.speedMax = 3.0f;
		config.particles.splashConfig.lifetime = 0.25f;
		config.particles.splashConfig.particleCount = 5;
		config.particles.splashConfig.spreadAngle = 1.8f;

		return config;
	}

	WeatherConfig WeatherPresets::HeavyRain()
	{
		WeatherConfig config;
		config.type = WeatherType::Rain;
		config.name = "HeavyRain";
		config.intensity = 1.8f;
		config.windStrength = 0.3f;

		config.particles.spawnRate = 500.0f;
		config.particles.particleLifetime = 1.5f;
		config.particles.velocity = { 1.5f, -16.0f };
		config.particles.velocityVariation = { 1.0f, 3.0f };
		config.particles.sizeMin = 0.025f;
		config.particles.sizeMax = 0.05f;
		config.particles.colorStart = { 0.65f, 0.75f, 0.9f, 0.7f };
		config.particles.colorEnd = { 0.65f, 0.75f, 0.9f, 0.0f };
		config.particles.motionType = ParticleMotion::Linear;
		config.particles.spawnWidthMultiplier = 1.8f;

		return config;
	}

	WeatherConfig WeatherPresets::Storm()
	{
		WeatherConfig config;
		config.type = WeatherType::Rain;
		config.name = "Storm";
		config.intensity = 2.5f;
		config.windStrength = 0.7f;

		config.particles.spawnRate = 600.0f;
		config.particles.particleLifetime = 1.2f;
		config.particles.velocity = { 3.0f, -20.0f };
		config.particles.velocityVariation = { 2.0f, 4.0f };
		config.particles.sizeMin = 0.03f;
		config.particles.sizeMax = 0.06f;
		config.particles.colorStart = { 0.6f, 0.7f, 0.85f, 0.8f };
		config.particles.colorEnd = { 0.6f, 0.7f, 0.85f, 0.0f };
		config.particles.motionType = ParticleMotion::Linear;
		config.particles.spawnWidthMultiplier = 2.0f;

		config.transition.duration = 1.5f;

		return config;
	}
}
