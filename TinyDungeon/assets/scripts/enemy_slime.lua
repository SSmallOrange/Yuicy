-- enemy_slime.lua
-- Simple FSM-based enemy AI for slime

local HealthSystem = require("assets/scripts/health_system")
local GameState = require("assets/scripts/game_state")

local EnemySlime = {}

EnemySlime.State = {
    IDLE = "idle",
    PATROL = "patrol",
    CHASE = "chase",
    ATTACK = "attack"
}

function EnemySlime:OnCreate()
    print("EnemySlime created: " .. self.entity:GetTag())
    
    -- AI parameters (can be overridden from JSON stats)
    self.speed = 1.5
    self.detectRange = 5.0
    self.attackRange = 0.8
    self.damage = 1
    
    -- State machine
    self.state = self.State.IDLE
    self.idleTimer = 0
    self.idleDuration = 2.0  -- Seconds to stay idle
    
    self.facingRight = true
    
    -- Initialize health system
    self.health = HealthSystem.new()
    self.health:Init(self.entity, {
        maxHealth = 10,
        barOffset = { x = 0, y = 0.6 },
        barSize = { width = 0.6, height = 0.09 }
    })
    
    -- Enable gravity
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        rb:SetGravityScale(1.0)
    end
end

function EnemySlime:OnUpdate(dt)
    if not self.entity:IsValid() then
        return
    end
    
    -- Find player
    local player = Scene.FindEntityByName(self.entity, "Player")
    if not player or not player:IsValid() then
        self:DoIdle(dt)
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
    
    -- State machine transitions
    if self.state == self.State.IDLE then
        if canSeePlayer then
            self.state = self.State.CHASE
        else
            self:DoIdle(dt)
        end
        
    elseif self.state == self.State.CHASE then
        if not canSeePlayer then
            -- Lost sight of player
            self.state = self.State.IDLE
            self.idleTimer = 0
        elseif distance < self.attackRange then
            self.state = self.State.ATTACK
        elseif distance > self.detectRange * 1.5 then
            self.state = self.State.IDLE
            self.idleTimer = 0
        else
            self:DoChase(dx, distance, dt)
        end
        
    elseif self.state == self.State.ATTACK then
        self:DoAttack()
        self.state = self.State.CHASE
    end
    
    -- Update animation
    self:UpdateAnimation()
    
    -- Update sprite flip
    self:UpdateSpriteFlip()
    
    -- Update health bar position
    if self.health then
        self.health:UpdateHealthBar()
    end
end

function EnemySlime:DoIdle(dt)
    self.idleTimer = self.idleTimer + dt
    
    -- Stop movement
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        local vel = rb:GetLinearVelocity()
        rb:SetLinearVelocity(0, vel.y)
    end
end

function EnemySlime:DoChase(dx, distance, dt)
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        local vel = rb:GetLinearVelocity()
        
        -- Move towards player (horizontal only)
        local vx = 0
        if dx > 0.1 then
            vx = self.speed
            self.facingRight = true
        elseif dx < -0.1 then
            vx = -self.speed
            self.facingRight = false
        end
        
        rb:SetLinearVelocity(vx, vel.y)
    end
end

function EnemySlime:DoAttack()
    -- TODO: Deal damage to player, play attack animation
    print("Slime attacks!")
end

function EnemySlime:UpdateAnimation()
    if not self.entity:HasAnimation() then
        return
    end
    
    local anim = self.entity:GetAnimation()
    
    if self.state == self.State.IDLE then
        anim:Play("idle")
    elseif self.state == self.State.CHASE then
        if self.facingRight then
            anim:Play("walk_right")
        else
            anim:Play("walk_left")
        end
    end
end

function EnemySlime:UpdateSpriteFlip()
    if self.entity:HasSprite() then
        local sprite = self.entity:GetSprite()
        -- Sprite faces left by default, flip when facing right
        sprite.FlipX = self.facingRight
    end
end

function EnemySlime:OnDestroy()
    print("EnemySlime destroyed!")
    if self.health then
        self.health:Destroy()
    end
end

function EnemySlime:OnTriggerEnter(other)
    print("[DEBUG] Slime OnTriggerEnter called!")
    if other:IsValid() then
        local tag = other:GetTag()
        print("[DEBUG] Other tag: " .. tag)
        if tag == "Projectile" then
            -- Take damage from bullet
            local isDead = self.health:TakeDamage(3)
            
            -- Knockback effect
            if self.entity:HasRigidbody() and other:HasTransform() then
                local myPos = self.entity:GetTransform().Translation
                local bulletPos = other:GetTransform().Translation
                
                local dx = myPos.x - bulletPos.x
                local knockbackForce = 4.0
                local knockbackX = dx > 0 and knockbackForce or -knockbackForce
                local knockbackY = 1.0  -- Smaller upward bounce
                
                print("[DEBUG] Applying knockback: " .. knockbackX .. ", " .. knockbackY)
                local rb = self.entity:GetRigidbody()
                rb:ApplyLinearImpulse(knockbackX, knockbackY)
            end
            
            if isDead then
                print("Slime died!")
                -- Add score
                GameState:OnEnemyKilled("Slime")
                -- Destroy health bar first
                if self.health then
                    self.health:Destroy()
                end
                Scene.DestroyEntity(self.entity, self.entity)
            end
            
            -- Destroy the bullet
            Scene.DestroyEntity(self.entity, other)
        elseif tag == "Player" then
            print("Slime hit player!")
        end
    end
end

return EnemySlime
