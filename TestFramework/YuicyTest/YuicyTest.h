#pragma once

// 测试代码统一 include 本文件：glm 的 StringMaker 特化必须在断言展开前可见，否则失败信息只会打印 {?}
#include <doctest/doctest.h>

#include "YuicyTest/AssertCapture.h"
#include "YuicyTest/GlmSupport.h"
#include "YuicyTest/SceneFixture.h"
#include "YuicyTest/ScopedProject.h"
#include "YuicyTest/TempDirectory.h"
