-- game_state.lua
-- 全局游戏状态模块

local GameState = {
    score = 0,
    kills = 0,
    
    -- 击杀奖励配置
    scorePerKill = {
        Slime = 10,
        Bat = 15,
        default = 5
    }
}

-- 添加分数
function GameState:AddScore(points)
    self.score = self.score + points
    print("[GameState] Score: " .. self.score)
    
    -- 更新全局变量供 C++ 读取
    __GAME_SCORE__ = self.score
end

-- 击杀敌人加分
function GameState:OnEnemyKilled(enemyType)
    local points = self.scorePerKill[enemyType] or self.scorePerKill.default
    self.kills = self.kills + 1
    self:AddScore(points)
    print("[GameState] Killed " .. enemyType .. ", total kills: " .. self.kills)
end

-- 重置
function GameState:Reset()
    self.score = 0
    self.kills = 0
    __GAME_SCORE__ = 0
end

-- 初始化全局变量
__GAME_SCORE__ = 0

return GameState
