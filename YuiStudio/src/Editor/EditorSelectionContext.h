#pragma once

#include "Yuicy/Core/UUID.h"
#include "Yuicy/Asset/Asset.h"

#include <vector>
#include <algorithm>

namespace Yuicy {

	// 编辑器共享选择上下文
	// 所有面板（Hierarchy、Viewport、Properties 等）共享同一份选择状态
	struct EditorSelectionContext
	{
		// 当前选择实体
		std::vector<UUID> selectedEntities;
		AssetHandle selectedAsset = 0;

		// 单选快捷方法
		void SetSelectedEntity(UUID entityUUID)
		{
			selectedEntities.clear();
			if (entityUUID != 0)
				selectedEntities.push_back(entityUUID);
		}

		UUID GetPrimarySelectedEntityUUID() const
		{
			return selectedEntities.empty() ? UUID(0) : selectedEntities.front();
		}

		bool IsEntitySelected(UUID entityUUID) const
		{
			return std::find(selectedEntities.begin(), selectedEntities.end(), entityUUID) != selectedEntities.end();
		}

		bool HasEntitySelection() const
		{
			return !selectedEntities.empty();
		}

		void ClearEntitySelection()
		{
			selectedEntities.clear();
		}

		// 当前是否选择资源
		bool HasAssetSelection() const
		{
			return selectedAsset != 0;
		}

		void ClearAssetSelection()
		{
			selectedAsset = 0;
		}

		void ClearAll()
		{
			ClearEntitySelection();
			ClearAssetSelection();
		}
	};

}
