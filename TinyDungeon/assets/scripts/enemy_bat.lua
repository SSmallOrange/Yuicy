-- enemy_bat.lua
-- Flying enemy AI for bat (no gravity)

local HealthSystem = require("assets/scripts/health_system")
local GameState = require("assets/scripts/game_state")

local EnemyBat = {}

EnemyBat.State = {
    IDLE = "idle",
    PATROL = "patrol",
    CHASE = "chase"
}

function EnemyBat:OnCreate()
    print("EnemyBat created: " .. self.entity:GetTag())
    
    -- AI parameters
    self.speed = 2.0
    self.detectRange = 8.0
    self.attackRange = 6.0  -- Ranged attack or swoop
    
    -- State
    self.state = self.State.IDLE
    self.facingRight = true
    
    -- Patrol
    self.patrolTimer = 0
    self.patrolDirection = 1  -- 1 = right, -1 = left
    self.patrolDuration = 2.0
    
    -- Vertical oscillation (flying effect)
    self.floatTime = 0
    self.floatAmplitude = 0.3
    self.floatSpeed = 2.0
    
    -- Disable gravity (flying)
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        rb:SetGravityScale(0.0)
    end
    
    -- Initialize health system
    self.health = HealthSystem.new()
    self.health:Init(self.entity, {
        maxHealth = 8,
        barOffset = { x = 0, y = 0.5 },
        barSize = { width = 0.5, height = 0.09 }
    })
end

function EnemyBat:OnUpdate(dt)
    if not self.entity:IsValid() then
        return
    end
    
    self.floatTime = self.floatTime + dt
    
    -- Find player
    local player = Scene.FindEntityByName(self.entity, "Player")
    if not player or not player:IsValid() then
        self:DoPatrol(dt)
        return
    end
    
    -- Get positions
    local myPos = self.entity:GetTransform().Translation
    local playerPos = player:GetTransform().Translation
    local dx = playerPos.x - myPos.x
    local dy = playerPos.y - myPos.y
    local distance = math.sqrt(dx * dx + dy * dy)
    
    -- Check line of sight (walls block vision)
    local canSeePlayer = false
    if distance < self.detectRange then
        canSeePlayer = Scene.HasLineOfSight(self.entity, myPos.x, myPos.y, playerPos.x, playerPos.y)
    end
    
    -- State machine
    if self.state == self.State.IDLE or self.state == self.State.PATROL then
        if canSeePlayer then
            self.state = self.State.CHASE
        else
            self:DoPatrol(dt)
        end
        
    elseif self.state == self.State.CHASE then
        if not canSeePlayer then
            -- Lost sight of player
            self.state = self.State.PATROL
            self.patrolTimer = 0
        elseif distance <= self.attackRange then
            -- In attack range, stop and idle (attack later)
            self:DoAttack(dt)
        elseif distance > self.detectRange * 1.5 then
            self.state = self.State.PATROL
            self.patrolTimer = 0
        else
            self:DoChase(dx, dy, distance, dt)
        end
    end
    
    self:UpdateAnimation()
    self:UpdateSpriteFlip()
    
    -- Update health bar position
    if self.health then
        self.health:UpdateHealthBar()
    end
end

function EnemyBat:DoPatrol(dt)
    self.patrolTimer = self.patrolTimer + dt
    
    if self.patrolTimer >= self.patrolDuration then
        self.patrolTimer = 0
        self.patrolDirection = -self.patrolDirection
    end
    
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        local vx = self.patrolDirection * self.speed * 0.5
        local vy = math.sin(self.floatTime * self.floatSpeed) * self.floatAmplitude
        
        rb:SetLinearVelocity(vx, vy)
        self.facingRight = self.patrolDirection > 0
    end
end

function EnemyBat:DoChase(dx, dy, distance, dt)
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        
        -- Normalize direction
        local nx = dx / distance
        local ny = dy / distance
        
        -- Add floating effect
        local floatOffset = math.sin(self.floatTime * self.floatSpeed) * self.floatAmplitude * 0.5
        
        local vx = nx * self.speed
        local vy = ny * self.speed + floatOffset
        
        rb:SetLinearVelocity(vx, vy)
        self.facingRight = dx > 0
    end
end

function EnemyBat:DoAttack(dt)
    -- Stop moving (hover in place)
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        local floatOffset = math.sin(self.floatTime * self.floatSpeed) * self.floatAmplitude
        rb:SetLinearVelocity(0, floatOffset)
    end
    -- TODO: Attack logic here
end

function EnemyBat:UpdateAnimation()
    if not self.entity:HasAnimation() then
        return
    end
    
    local anim = self.entity:GetAnimation()
    
    if self.state == self.State.CHASE then
        if self.facingRight then
            anim:Play("walk_right")
        else
            anim:Play("walk_left")
        end
    else
        anim:Play("idle")
    end
end

function EnemyBat:UpdateSpriteFlip()
    if self.entity:HasSprite() then
        local sprite = self.entity:GetSprite()
        -- Sprite faces left by default, flip when facing right
        sprite.FlipX = self.facingRight
    end
end

function EnemyBat:OnDestroy()
    print("EnemyBat destroyed!")
    if self.health then
        self.health:Destroy()
    end
end

function EnemyBat:OnTriggerEnter(other)
    print("[DEBUG] Bat OnTriggerEnter called!")
    if other:IsValid() then
        local tag = other:GetTag()
        print("[DEBUG] Bat Other tag: " .. tag)
        if tag == "Projectile" then
            print("[DEBUG] Bat hit by projectile!")
            -- Take damage from bullet
            local isDead = self.health:TakeDamage(3)
            
            -- Knockback effect
            if self.entity:HasRigidbody() and other:HasTransform() then
                local myPos = self.entity:GetTransform().Translation
                local bulletPos = other:GetTransform().Translation
                
                local dx = myPos.x - bulletPos.x
                local knockbackForce = 3.0
                local knockbackX = dx > 0 and knockbackForce or -knockbackForce
                local knockbackY = 0.5  -- Small vertical push
                
                print("[DEBUG] Bat knockback: " .. knockbackX .. ", " .. knockbackY)
                local rb = self.entity:GetRigidbody()
                rb:ApplyLinearImpulse(knockbackX, knockbackY)
            end
            
            if isDead then
                print("Bat died!")
                -- Add score
                GameState:OnEnemyKilled("Bat")
                -- Destroy health bar first
                if self.health then
                    self.health:Destroy()
                end
                Scene.DestroyEntity(self.entity, self.entity)
            end
            
            -- Destroy the bullet
            Scene.DestroyEntity(self.entity, other)
        elseif tag == "Player" then
            print("Bat hit player!")
        end
    end
end

return EnemyBat
