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

		// 注册所有内置预设
		RegisterPreset("LightRain", LightRain());
		RegisterPreset("Rain", Rain());
		RegisterPreset("HeavyRain", HeavyRain());
		RegisterPreset("Storm", Storm());

		RegisterPreset("LightSnow", LightSnow());
		RegisterPreset("Snow", Snow());
		RegisterPreset("HeavySnow", HeavySnow());
		RegisterPreset("Blizzard", Blizzard());

		RegisterPreset("Drizzle", Drizzle());
		RegisterPreset("Sandstorm", Sandstorm());
		RegisterPreset("FallingLeaves", FallingLeaves());
		RegisterPreset("Fireflies", Fireflies());
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

		case WeatherType::Snow:
			switch (intensity)
			{
			case WeatherIntensity::Light:   return LightSnow();
			case WeatherIntensity::Normal:  return Snow();
			case WeatherIntensity::Heavy:   return HeavySnow();
			case WeatherIntensity::Extreme: return Blizzard();
			}
			break;

		case WeatherType::Storm:
			return Storm();

		case WeatherType::Drizzle:
			return Drizzle();

		case WeatherType::Blizzard:
			return Blizzard();

		case WeatherType::Sandstorm:
			return Sandstorm();

		case WeatherType::Leaves:
			return FallingLeaves();

		case WeatherType::Fireflies:
			return Fireflies();

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
		config.type = WeatherType::Storm;
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

	WeatherConfig WeatherPresets::LightSnow()
	{
		WeatherConfig config;
		config.type = WeatherType::Snow;
		config.name = "LightSnow";
		config.intensity = 0.5f;
		config.windStrength = 0.1f;

		config.particles.spawnRate = 40.0f;
		config.particles.particleLifetime = 10.0f;
		config.particles.velocity = { 0.0f, -0.8f };
		config.particles.velocityVariation = { 0.4f, 0.2f };
		config.particles.sizeMin = 0.03f;
		config.particles.sizeMax = 0.07f;
		config.particles.colorStart = { 1.0f, 1.0f, 1.0f, 0.8f };
		config.particles.colorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
		config.particles.motionType = ParticleMotion::Swaying;
		config.particles.motionFrequency = 3.0f;
		config.particles.motionAmplitude = 0.2f;
		config.particles.rotationSpeed = 0.3f;
		config.particles.spawnWidthMultiplier = 1.6f;
		config.particles.spawnHeightOffset = 0.7f;

		config.transition.duration = 4.0f;

		return config;
	}

	WeatherConfig WeatherPresets::Snow()
	{
		WeatherConfig config;
		config.type = WeatherType::Snow;
		config.name = "Snow";
		config.intensity = 1.0f;
		config.windStrength = 0.2f;

		config.particles.spawnRate = 80.0f;
		config.particles.particleLifetime = 8.0f;
		config.particles.velocity = { 0.0f, -1.5f };
		config.particles.velocityVariation = { 0.8f, 0.3f };
		config.particles.sizeMin = 0.04f;
		config.particles.sizeMax = 0.1f;
		config.particles.colorStart = { 1.0f, 1.0f, 1.0f, 0.9f };
		config.particles.colorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
		config.particles.motionType = ParticleMotion::Swaying;
		config.particles.motionFrequency = 5.0f;
		config.particles.motionAmplitude = 0.3f;
		config.particles.rotationSpeed = 0.5f;
		config.particles.spawnWidthMultiplier = 1.8f;
		config.particles.spawnHeightOffset = 0.7f;

		return config;
	}

	WeatherConfig WeatherPresets::HeavySnow()
	{
		WeatherConfig config;
		config.type = WeatherType::Snow;
		config.name = "HeavySnow";
		config.intensity = 1.8f;
		config.windStrength = 0.4f;

		config.particles.spawnRate = 150.0f;
		config.particles.particleLifetime = 6.0f;
		config.particles.velocity = { 0.5f, -2.5f };
		config.particles.velocityVariation = { 1.2f, 0.5f };
		config.particles.sizeMin = 0.05f;
		config.particles.sizeMax = 0.12f;
		config.particles.colorStart = { 0.95f, 0.97f, 1.0f, 0.9f };
		config.particles.colorEnd = { 0.95f, 0.97f, 1.0f, 0.0f };
		config.particles.motionType = ParticleMotion::Swaying;
		config.particles.motionFrequency = 4.0f;
		config.particles.motionAmplitude = 0.4f;
		config.particles.rotationSpeed = 0.8f;
		config.particles.spawnWidthMultiplier = 2.0f;

		return config;
	}

	WeatherConfig WeatherPresets::Blizzard()
	{
		WeatherConfig config;
		config.type = WeatherType::Blizzard;
		config.name = "Blizzard";
		config.intensity = 3.0f;
		config.windStrength = 0.9f;

		config.particles.spawnRate = 300.0f;
		config.particles.particleLifetime = 4.0f;
		config.particles.velocity = { 4.0f, -4.0f };
		config.particles.velocityVariation = { 2.0f, 1.5f };
		config.particles.sizeMin = 0.04f;
		config.particles.sizeMax = 0.15f;
		config.particles.colorStart = { 0.9f, 0.93f, 1.0f, 0.85f };
		config.particles.colorEnd = { 0.9f, 0.93f, 1.0f, 0.0f };
		config.particles.motionType = ParticleMotion::Swaying;
		config.particles.motionFrequency = 8.0f;
		config.particles.motionAmplitude = 0.6f;
		config.particles.rotationSpeed = 2.0f;
		config.particles.spawnWidthMultiplier = 2.5f;

		config.transition.duration = 2.0f;

		return config;
	}

	// ============================================================
	// 特殊天气预设
	// ============================================================

	WeatherConfig WeatherPresets::Drizzle()
	{
		WeatherConfig config;
		config.type = WeatherType::Drizzle;
		config.name = "Drizzle";
		config.intensity = 0.4f;

		// 毛毛雨：非常细小、缓慢的雨滴
		config.particles.spawnRate = 150.0f;
		config.particles.particleLifetime = 3.0f;
		config.particles.velocity = { 0.0f, -4.0f };
		config.particles.velocityVariation = { 0.2f, 0.5f };
		config.particles.sizeMin = 0.008f;
		config.particles.sizeMax = 0.015f;
		config.particles.colorStart = { 0.8f, 0.85f, 0.95f, 0.3f };
		config.particles.colorEnd = { 0.8f, 0.85f, 0.95f, 0.0f };
		config.particles.motionType = ParticleMotion::Linear;

		config.transition.duration = 4.0f;

		return config;
	}

	WeatherConfig WeatherPresets::Sandstorm()
	{
		WeatherConfig config;
		config.type = WeatherType::Sandstorm;
		config.name = "Sandstorm";
		config.intensity = 2.0f;
		config.windStrength = 0.85f;

		// 沙尘暴：横向移动的黄色粒子
		config.particles.spawnRate = 400.0f;
		config.particles.particleLifetime = 3.0f;
		config.particles.velocity = { 8.0f, -1.0f };
		config.particles.velocityVariation = { 3.0f, 2.0f };
		config.particles.sizeMin = 0.02f;
		config.particles.sizeMax = 0.08f;
		config.particles.colorStart = { 0.85f, 0.7f, 0.4f, 0.6f };
		config.particles.colorEnd = { 0.85f, 0.7f, 0.4f, 0.0f };
		config.particles.motionType = ParticleMotion::Random;
		config.particles.motionFrequency = 6.0f;
		config.particles.motionAmplitude = 0.5f;
		config.particles.rotationSpeed = 3.0f;
		config.particles.spawnWidthMultiplier = 2.0f;
		config.particles.spawnHeightOffset = 0.3f;  // 从侧面生成更多

		return config;
	}

	WeatherConfig WeatherPresets::FallingLeaves()
	{
		WeatherConfig config;
		config.type = WeatherType::Leaves;
		config.name = "FallingLeaves";
		config.intensity = 0.6f;
		config.windStrength = 0.3f;

		config.particles.spawnRate = 15.0f;
		config.particles.particleLifetime = 12.0f;
		config.particles.velocity = { 0.5f, -0.8f };
		config.particles.velocityVariation = { 0.8f, 0.3f };
		config.particles.sizeMin = 0.08f;
		config.particles.sizeMax = 0.15f;

		config.particles.colorStart = { 0.9f, 0.6f, 0.2f, 0.9f };
		config.particles.colorEnd = { 0.8f, 0.4f, 0.1f, 0.0f };
		config.particles.motionType = ParticleMotion::Swaying;
		config.particles.motionFrequency = 2.0f;
		config.particles.motionAmplitude = 0.8f;
		config.particles.rotationSpeed = 1.5f;
		config.particles.spawnWidthMultiplier = 1.5f;

		config.transition.duration = 5.0f;

		return config;
	}

	WeatherConfig WeatherPresets::Fireflies()
	{
		WeatherConfig config;
		config.type = WeatherType::Fireflies;
		config.name = "Fireflies";
		config.intensity = 0.8f;

		config.particles.spawnRate = 8.0f;
		config.particles.particleLifetime = 15.0f;
		config.particles.velocity = { 0.0f, 0.2f };  // 轻微向上
		config.particles.velocityVariation = { 0.5f, 0.5f };
		config.particles.sizeMin = 0.03f;
		config.particles.sizeMax = 0.06f;
		
		config.particles.colorStart = { 0.7f, 1.0f, 0.3f, 0.9f };
		config.particles.colorEnd = { 0.5f, 0.8f, 0.2f, 0.0f };
		config.particles.motionType = ParticleMotion::Random;
		config.particles.motionFrequency = 2.0f;
		config.particles.motionAmplitude = 0.4f;
		config.particles.rotationSpeed = 0.0f;

		config.particles.spawnWidthMultiplier = 1.2f;
		config.particles.spawnHeightOffset = 0.0f;  // 从中间区域生成

		config.transition.duration = 6.0f;

		return config;
	}
}
