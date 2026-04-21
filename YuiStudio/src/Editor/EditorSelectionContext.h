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
		// 当前选择实体（第一个为主选实体）
		std::vector<UUID> selectedEntities;
		AssetHandle selectedAsset = 0;

		// 单选：清空已有选择，设为唯一选中
		void SetSelectedEntity(UUID entityUUID)
		{
			selectedEntities.clear();
			if (entityUUID != 0)
				selectedEntities.push_back(entityUUID);
		}

		// 追加选择（不重复）
		void AddEntity(UUID entityUUID)
		{
			if (entityUUID == 0)
				return;
			if (!IsEntitySelected(entityUUID))
				selectedEntities.push_back(entityUUID);
		}

		// 移除选择
		void RemoveEntity(UUID entityUUID)
		{
			auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entityUUID);
			if (it != selectedEntities.end())
				selectedEntities.erase(it);
		}

		// 切换选择（已选则移除，未选则追加）
		void ToggleEntity(UUID entityUUID)
		{
			if (entityUUID == 0)
				return;
			if (IsEntitySelected(entityUUID))
				RemoveEntity(entityUUID);
			else
				selectedEntities.push_back(entityUUID);
		}

		// 主选实体（列表中的第一个）
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

		size_t GetSelectionCount() const
		{
			return selectedEntities.size();
		}

		bool IsMultiSelection() const
		{
			return selectedEntities.size() > 1;
		}

		void ClearEntitySelection()
		{
			selectedEntities.clear();
		}

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
