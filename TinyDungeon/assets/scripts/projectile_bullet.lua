-- projectile_bullet.lua
-- 子弹脚本 - 处理碰撞、销毁和伤害逻辑

local Bullet = {}

function Bullet:OnCreate()
    self.damage = 3
    self.knockbackForce = 5.0
    self.lifetime = 3.0
    self.timer = 0
end

function Bullet:OnUpdate(dt)
    -- 超时销毁
    self.timer = self.timer + dt
    if self.timer >= self.lifetime then
        Scene.DestroyEntity(self.entity, self.entity)
    end
end

function Bullet:OnTriggerEnter(other)
    if not other or not other:IsValid() then
        return
    end
    
    local tag = other:GetTag()
    if string.find(tag, "Ground") or string.find(tag, "Wall") or string.find(tag, "Tile") then
        Scene.DestroyEntity(self.entity, self.entity)
        return
    end
end

function Bullet:OnDestroy()
end

return Bullet
