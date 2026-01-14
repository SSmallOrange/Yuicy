local PlayerController = {}

function PlayerController:OnCreate()
    print("PlayerController created!")
    
    -- Configurable parameters
    self.speed = 3.0
    self.jumpForce = 6.0
    self.spawnPoint = { x = 2.5, y = 11.5 }  -- Starting position
    
    self.facingRight = true
    self.groundContacts = 0
    self.groundCheckCooldown = 0
    self.wasJumping = false
    
    -- Set spawn position
    if self.entity:HasTransform() then
        local transform = self.entity:GetTransform()
        transform.Translation.x = self.spawnPoint.x
        transform.Translation.y = self.spawnPoint.y
    end
    
    -- Enable gravity for platformer movement
    if self.entity:HasRigidbody() then
        local rb = self.entity:GetRigidbody()
        rb:SetGravityScale(1.0)
    end
end

function PlayerController:OnUpdate(dt)
    if not self.entity:IsValid() then
        return
    end

    -- Ground check cooldown (after jump)
    if self.groundCheckCooldown > 0 then
        self.groundCheckCooldown = self.groundCheckCooldown - dt
    end

    local rb = nil
    local currentVelX, currentVelY = 0, 0
    
    if self.entity:HasRigidbody() then
        rb = self.entity:GetRigidbody()
        local vel = rb:GetLinearVelocity()
        currentVelX = vel.x
        currentVelY = vel.y
    end

    -- Determine if grounded (contact count > 0 and not in jump cooldown)
    local isGrounded = self.groundContacts > 0 and self.groundCheckCooldown <= 0
    
    -- Velocity-based ground check as backup (falling and nearly stopped)
    if not isGrounded and self.groundCheckCooldown <= 0 then
        if math.abs(currentVelY) < 0.1 and self.wasJumping then
            -- We were jumping and now stopped, likely landed
            isGrounded = true
            self.wasJumping = false
        end
    end

    -- Horizontal movement
    local vx = 0
    local moving = false
    
    if Input.IsKeyPressed(Key.A) or Input.IsKeyPressed(Key.Left) then
        vx = vx - self.speed
        moving = true
        self.facingRight = false
    end
    if Input.IsKeyPressed(Key.D) or Input.IsKeyPressed(Key.Right) then
        vx = vx + self.speed
        moving = true
        self.facingRight = true
    end
    
    -- Jump (only when grounded)
    local vy = currentVelY
    if isGrounded and Input.IsKeyPressed(Key.W) then
        vy = self.jumpForce
        self.groundCheckCooldown = 0.15  -- Prevent immediate re-grounding
        self.wasJumping = true
    end
    
    -- Apply velocity
    if rb then
        rb:SetLinearVelocity(vx, vy)
    end
    
    -- Animation control: walk when moving OR in air, idle when stationary on ground
    if self.entity:HasAnimation() then
        local anim = self.entity:GetAnimation()
        if moving or not isGrounded then
            anim:Play("walk_right")
        else
            anim:Play("idle")
        end
    end
    
    -- Sprite flip: texture faces LEFT, so flip when facing RIGHT
    if self.entity:HasSprite() then
        local sprite = self.entity:GetSprite()
        sprite.FlipX = self.facingRight
    end
end

function PlayerController:OnDestroy()
    print("PlayerController destroyed!")
end

function PlayerController:OnCollisionEnter(other)
    if other:IsValid() then
        local tag = other:GetTag()
        -- Count ground contacts
        if string.find(tag, "Ground") or string.find(tag, "Floor") or string.find(tag, "Platform") then
            self.groundContacts = self.groundContacts + 1
        end
    end
end

function PlayerController:OnCollisionExit(other)
    if other:IsValid() then
        local tag = other:GetTag()
        -- Decrease ground contact count
        if string.find(tag, "Ground") or string.find(tag, "Floor") or string.find(tag, "Platform") then
            self.groundContacts = self.groundContacts - 1
            if self.groundContacts < 0 then
                self.groundContacts = 0
            end
        end
    end
end

function PlayerController:OnTriggerEnter(other)
    if other:IsValid() then
        print("Player triggered by: " .. other:GetTag())
    end
end

return PlayerController
