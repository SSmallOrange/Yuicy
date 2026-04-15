#pragma once

#include "Yuicy/Core/UUID.h"
#include "Yuicy/Asset/AssetTypes.h"

namespace Yuicy {

	using AssetHandle = UUID;

	class Asset
	{
	public:
		AssetHandle handle = 0;
		uint16_t Flags = (uint16_t)AssetFlag::None;

		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		virtual bool operator==(const Asset& other) const
		{
			return handle == other.handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

	private:
		friend class EditorAssetManager;

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
	};

}
