#pragma once

#include "Yuicy/Core/Base.h"

namespace Yuicy {

	class AssetLoaderRegistry;

	Ref<const AssetLoaderRegistry> CreateBuiltinAssetLoaders();

}
