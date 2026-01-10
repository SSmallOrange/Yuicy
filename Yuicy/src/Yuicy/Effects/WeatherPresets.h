#pragma once

#include "Yuicy/Effects/WeatherTypes.h"
#include <unordered_map>
#include <string>

namespace Yuicy {

	class WeatherPresets
	{
	public:
		// 获取内置预设
		static WeatherConfig Get(WeatherType type, WeatherIntensity intensity = WeatherIntensity::Normal);

		// 根据名称获取预设
		static WeatherConfig GetByName(const std::string& name);

		// 注册自定义预设
		static void RegisterPreset(const std::string& name, const WeatherConfig& config);

		// 检查预设是否存在
		static bool HasPreset(const std::string& name);

		// 获取所有已注册预设的名称
		static std::vector<std::string> GetAllPresetNames();

		// 雨天系列
		static WeatherConfig LightRain();
		static WeatherConfig Rain();
		static WeatherConfig HeavyRain();
		static WeatherConfig Storm();

		// 雪天系列
		static WeatherConfig LightSnow();
		static WeatherConfig Snow();
		static WeatherConfig HeavySnow();
		static WeatherConfig Blizzard();

		// 特殊效果
		static WeatherConfig Drizzle();
		static WeatherConfig Sandstorm();
		static WeatherConfig FallingLeaves();
		static WeatherConfig Fireflies();

	private:
		// 内置预设初始化
		static void InitializeBuiltInPresets();

	private:
		static bool s_initialized;
		static std::unordered_map<std::string, WeatherConfig> s_presets;
	};
}
