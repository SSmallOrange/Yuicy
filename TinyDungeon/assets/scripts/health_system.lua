-- health_system.lua
-- 可复用的生命值系统模块

local HealthSystem = {}

-- 配置默认值
HealthSystem.defaults = {
    maxHealth = 100,
    barOffset = { x = 0, y = 0.8 },
    barSize = { width = 0.8, height = 0.1 },
    barColor = { r = 0.2, g = 0.8, b = 0.2, a = 1.0 },
    bgColor = { r = 0.3, g = 0.3, b = 0.3, a = 0.8 },
    showBar = true,
    barZOffset = 0.01  -- 血条在实体上方的Z偏移
}

-- 初始化生命值系统（在 OnCreate 中调用）
function HealthSystem:Init(entity, config)
    config = config or {}
    
    self.entity = entity
    self.maxHealth = config.maxHealth or self.defaults.maxHealth
    self.currentHealth = self.maxHealth
    self.showBar = config.showBar ~= false
    
    -- 血条配置
    self.barOffset = config.barOffset or self.defaults.barOffset
    self.barSize = config.barSize or self.defaults.barSize
    self.barColor = config.barColor or self.defaults.barColor
    self.bgColor = config.bgColor or self.defaults.bgColor
    self.barZOffset = config.barZOffset or self.defaults.barZOffset
    
    -- 创建血条实体
    if self.showBar then
        self:CreateHealthBar()
    end
    
    return self
end

-- 创建血条（背景 + 前景）
function HealthSystem:CreateHealthBar()
    local tag = self.entity:GetTag()
    -- 背景条
    self.bgBar = Scene.CreateEntity(self.entity, tag .. "_HealthBg")
    if self.bgBar and self.bgBar:IsValid() then
        self.bgBar:AddSprite()
        local bgSprite = self.bgBar:GetSprite()
        bgSprite.Color = Vec4(self.bgColor.r, self.bgColor.g, self.bgColor.b, self.bgColor.a)
        bgSprite.SortingOrder = 1000
    end
    
    -- 前景条（生命值）
    self.fgBar = Scene.CreateEntity(self.entity, tag .. "_HealthFg")
    if self.fgBar and self.fgBar:IsValid() then
        self.fgBar:AddSprite()
        local fgSprite = self.fgBar:GetSprite()
        fgSprite.Color = Vec4(self.barColor.r, self.barColor.g, self.barColor.b, self.barColor.a)
        fgSprite.SortingOrder = 1001
    end
    
    self:UpdateHealthBar()
end

-- 更新血条位置和大小
function HealthSystem:UpdateHealthBar()
    if not self.showBar then return end
    if not self.entity:IsValid() then return end
    
    local myPos = self.entity:GetTransform().Translation
    local percent = self.currentHealth / self.maxHealth
    
    local barX = myPos.x + self.barOffset.x
    local barY = myPos.y + self.barOffset.y
    local barZ = myPos.z + self.barZOffset
    
    -- 更新背景条
    if self.bgBar and self.bgBar:IsValid() then
        local bgTransform = self.bgBar:GetTransform()
        bgTransform.Translation = Vec3(barX, barY, barZ)
        bgTransform.Scale = Vec3(self.barSize.width, self.barSize.height, 1)
    end
    
    -- 更新前景条（根据血量缩放）
    if self.fgBar and self.fgBar:IsValid() then
        local fgWidth = self.barSize.width * percent
        local fgOffset = (self.barSize.width - fgWidth) / 2  -- 左对齐偏移
        
        local fgTransform = self.fgBar:GetTransform()
        fgTransform.Translation = Vec3(barX - fgOffset, barY, barZ + 0.001)
        fgTransform.Scale = Vec3(fgWidth, self.barSize.height * 0.8, 1)
        
        -- 根据血量变色
        local fgSprite = self.fgBar:GetSprite()
        if percent > 0.5 then
            fgSprite.Color = Vec4(0.2, 0.8, 0.2, 1.0)  -- 绿
        elseif percent > 0.25 then
            fgSprite.Color = Vec4(0.9, 0.7, 0.1, 1.0)  -- 黄
        else
            fgSprite.Color = Vec4(0.9, 0.2, 0.2, 1.0)  -- 红
        end
    end
end

-- 受到伤害
function HealthSystem:TakeDamage(amount)
    self.currentHealth = math.max(0, self.currentHealth - amount)
    self:UpdateHealthBar()
    
    if self.currentHealth <= 0 then
        return true  -- 死亡
    end
    return false
end

-- 治疗
function HealthSystem:Heal(amount)
    self.currentHealth = math.min(self.maxHealth, self.currentHealth + amount)
    self:UpdateHealthBar()
end

-- 设置血量
function HealthSystem:SetHealth(current, max)
    if max then self.maxHealth = max end
    self.currentHealth = math.min(current, self.maxHealth)
    self:UpdateHealthBar()
end

-- 获取血量百分比
function HealthSystem:GetPercent()
    return self.currentHealth / self.maxHealth
end

-- 是否死亡
function HealthSystem:IsDead()
    return self.currentHealth <= 0
end

-- 显示/隐藏血条
function HealthSystem:SetBarVisible(visible)
    self.showBar = visible
    -- TODO: 实际隐藏/显示实体
end

-- 清理（在 OnDestroy 中调用）
function HealthSystem:Destroy()
    if self.bgBar and self.bgBar:IsValid() then
        Scene.DestroyEntity(self.entity, self.bgBar)
    end
    if self.fgBar and self.fgBar:IsValid() then
        Scene.DestroyEntity(self.entity, self.fgBar)
    end
end

-- 创建新的 HealthSystem 实例
function HealthSystem.new()
    local instance = {}
    setmetatable(instance, { __index = HealthSystem })
    return instance
end

return HealthSystem
