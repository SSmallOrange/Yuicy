#pragma once

#include "Yuicy/Asset/Asset.h"

#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Yuicy {

	enum class GridLayout
	{
		Rectangular = 0,
		Isometric,
		Hexagonal
	};

	enum class GridCellSwizzle
	{
		XYZ = 0,
		XZY,
		YXZ,
		YZX,
		ZXY,
		ZYX
	};

	enum class TileColliderType
	{
		None = 0,
		Sprite,
		Grid
	};

	enum class TilemapRenderMode
	{
		Chunk = 0,
		Individual
	};

	struct GridPosition
	{
		int m_x = 0;
		int m_y = 0;
		int m_z = 0;

		bool operator==(const GridPosition& other) const = default;
		bool operator<(const GridPosition& other) const
		{
			if (m_z != other.m_z)
				return m_z < other.m_z;
			if (m_y != other.m_y)
				return m_y < other.m_y;
			return m_x < other.m_x;
		}
	};

	struct GridPositionHash
	{
		size_t operator()(const GridPosition& position) const
		{
			size_t seed = 0;
			HashCombine(seed, position.m_x);
			HashCombine(seed, position.m_y);
			HashCombine(seed, position.m_z);
			return seed;
		}

	private:
		static void HashCombine(size_t& seed, int value)
		{
			std::hash<int> hasher;
			seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};

	struct TileCell
	{
		AssetHandle m_tileHandle = 0;
		glm::vec4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		uint32_t m_transformFlags = 0;
	};

	namespace TilemapUtils {

		inline const char* GridLayoutToString(GridLayout layout)
		{
			switch (layout)
			{
				case GridLayout::Rectangular: return "Rectangular";
				case GridLayout::Isometric:   return "Isometric";
				case GridLayout::Hexagonal:   return "Hexagonal";
			}

			return "Rectangular";
		}

		inline GridLayout GridLayoutFromString(std::string_view layout)
		{
			if (layout == "Rectangular") return GridLayout::Rectangular;
			if (layout == "Isometric")   return GridLayout::Isometric;
			if (layout == "Hexagonal")   return GridLayout::Hexagonal;
			return GridLayout::Rectangular;
		}

		inline const char* GridCellSwizzleToString(GridCellSwizzle swizzle)
		{
			switch (swizzle)
			{
				case GridCellSwizzle::XYZ: return "XYZ";
				case GridCellSwizzle::XZY: return "XZY";
				case GridCellSwizzle::YXZ: return "YXZ";
				case GridCellSwizzle::YZX: return "YZX";
				case GridCellSwizzle::ZXY: return "ZXY";
				case GridCellSwizzle::ZYX: return "ZYX";
			}

			return "XYZ";
		}

		inline GridCellSwizzle GridCellSwizzleFromString(std::string_view swizzle)
		{
			if (swizzle == "XYZ") return GridCellSwizzle::XYZ;
			if (swizzle == "XZY") return GridCellSwizzle::XZY;
			if (swizzle == "YXZ") return GridCellSwizzle::YXZ;
			if (swizzle == "YZX") return GridCellSwizzle::YZX;
			if (swizzle == "ZXY") return GridCellSwizzle::ZXY;
			if (swizzle == "ZYX") return GridCellSwizzle::ZYX;
			return GridCellSwizzle::XYZ;
		}

		inline const char* TileColliderTypeToString(TileColliderType colliderType)
		{
			switch (colliderType)
			{
				case TileColliderType::None:   return "None";
				case TileColliderType::Sprite: return "Sprite";
				case TileColliderType::Grid:   return "Grid";
			}

			return "None";
		}

		inline TileColliderType TileColliderTypeFromString(std::string_view colliderType)
		{
			if (colliderType == "None")   return TileColliderType::None;
			if (colliderType == "Sprite") return TileColliderType::Sprite;
			if (colliderType == "Grid")   return TileColliderType::Grid;
			return TileColliderType::None;
		}

		inline const char* TilemapRenderModeToString(TilemapRenderMode mode)
		{
			switch (mode)
			{
				case TilemapRenderMode::Chunk:      return "Chunk";
				case TilemapRenderMode::Individual: return "Individual";
			}

			return "Chunk";
		}

		inline TilemapRenderMode TilemapRenderModeFromString(std::string_view mode)
		{
			if (mode == "Chunk")      return TilemapRenderMode::Chunk;
			if (mode == "Individual") return TilemapRenderMode::Individual;
			return TilemapRenderMode::Chunk;
		}

	}

}
