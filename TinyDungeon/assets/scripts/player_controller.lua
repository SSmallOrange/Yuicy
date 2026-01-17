local PlayerController = {}

function PlayerController:OnCreate()
    print("PlayerController created!")
    
    -- Configurable parameters
    self.speed = 3.0
    self.jumpForce = 6.0
    self.spawnPoint = { x = 2.5, y = 11.5 }
    
    -- Projectile config
    self.projectileSpeed = 30.0
    self.projectileSize = { x = 0.3, y = 0.1 }
    self.projectileColor = { r = 1.0, g = 0.8, b = 0.2 }
    self.shootCooldown = 0.0
    self.shootCooldownTime = 0.15  -- Seconds between shots
    
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

    -- Cooldowns
    if self.groundCheckCooldown > 0 then
        self.groundCheckCooldown = self.groundCheckCooldown - dt
    end
    if self.shootCooldown > 0 then
        self.shootCooldown = self.shootCooldown - dt
    end

    local rb = nil
    local currentVelX, currentVelY = 0, 0
    
    if self.entity:HasRigidbody() then
        rb = self.entity:GetRigidbody()
        local vel = rb:GetLinearVelocity()
        currentVelX = vel.x
        currentVelY = vel.y
    end

    -- Determine if grounded
    local isGrounded = self.groundContacts > 0 and self.groundCheckCooldown <= 0
    
    if not isGrounded and self.groundCheckCooldown <= 0 then
        if math.abs(currentVelY) < 0.1 and self.wasJumping then
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
        self.groundCheckCooldown = 0.15
        self.wasJumping = true
    end
    
    -- Apply velocity
    if rb then
        rb:SetLinearVelocity(vx, vy)
    end
    
    -- Shooting (left mouse button)
    if Input.IsMouseButtonPressed(0) and self.shootCooldown <= 0 then
        self:Shoot()
        self.shootCooldown = self.shootCooldownTime
    end
    
    -- Animation control
    if self.entity:HasAnimation() then
        local anim = self.entity:GetAnimation()
        if moving or not isGrounded then
            anim:Play("walk_right")
        else
            anim:Play("idle")
        end
    end
    
    -- Sprite flip
    if self.entity:HasSprite() then
        local sprite = self.entity:GetSprite()
        sprite.FlipX = self.facingRight
    end
end

function PlayerController:Shoot()
    if not self.entity:HasTransform() then
        return
    end
    
    local transform = self.entity:GetTransform()
    local px = transform.Translation.x
    local py = transform.Translation.y
    
    -- Get mouse position and convert to world coords
    local mouseX, mouseY = Input.GetMousePosition()
    
    -- Find camera entity to convert screen to world
    local camera = Scene.FindEntityByName(self.entity, "MainCamera")
    if not camera or not camera:IsValid() then
        -- Fallback: shoot in facing direction
        local dirX = self.facingRight and 1.0 or -1.0
        Scene.CreateProjectile(self.entity, px, py, dirX, 0, 
            self.projectileSpeed, 3.0, self.projectileSize.x, self.projectileSize.y,
            self.projectileColor.r, self.projectileColor.g, self.projectileColor.b)
        return
    end
    
    -- Get camera properties
    local camTransform = camera:GetTransform()
    local camComp = camera:GetCamera()
    local orthoSize = camComp:GetOrthographicSize()
    
    -- Approximate viewport size (16:9 aspect ratio fallback)
    local aspectRatio = 16.0 / 9.0
    local viewportWidth = 960  -- These should match actual viewport
    local viewportHeight = 576
    
    -- Screen to NDC
    local ndcX = (mouseX / viewportWidth) * 2.0 - 1.0
    local ndcY = 1.0 - (mouseY / viewportHeight) * 2.0
    
    -- NDC to world
    local halfHeight = orthoSize / 2.0
    local halfWidth = halfHeight * aspectRatio
    local worldX = camTransform.Translation.x + ndcX * halfWidth
    local worldY = camTransform.Translation.y + ndcY * halfHeight
    
    -- Calculate direction
    local dirX = worldX - px
    local dirY = worldY - py
    local len = math.sqrt(dirX * dirX + dirY * dirY)
    
    if len > 0.01 then
        dirX = dirX / len
        dirY = dirY / len
        Scene.CreateProjectile(self.entity, px, py, dirX, dirY,
            self.projectileSpeed, 3.0, self.projectileSize.x, self.projectileSize.y,
            self.projectileColor.r, self.projectileColor.g, self.projectileColor.b)
    end
end

function PlayerController:OnDestroy()
    print("PlayerController destroyed!")
end

function PlayerController:OnCollisionEnter(other)
    if other:IsValid() then
        local tag = other:GetTag()
        if string.find(tag, "Ground") or string.find(tag, "Floor") or string.find(tag, "Platform") then
            self.groundContacts = self.groundContacts + 1
        end
    end
end

function PlayerController:OnCollisionExit(other)
    if other:IsValid() then
        local tag = other:GetTag()
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
