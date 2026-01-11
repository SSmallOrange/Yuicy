local PlayerController = {}

function PlayerController:OnCreate()
    print("PlayerController created!")
    self.speed = 8.0
end

function PlayerController:OnUpdate(dt)
    if not self.entity:IsValid() then
        return
    end

    local transform = self.entity:GetTransform()
    local pos = transform.Translation
    
    if Input.IsKeyPressed(Key.W) or Input.IsKeyPressed(Key.Up) then
        pos.y = pos.y + self.speed * dt
    end
    if Input.IsKeyPressed(Key.S) or Input.IsKeyPressed(Key.Down) then
        pos.y = pos.y - self.speed * dt
    end
    if Input.IsKeyPressed(Key.A) or Input.IsKeyPressed(Key.Left) then
        pos.x = pos.x - self.speed * dt
    end
    if Input.IsKeyPressed(Key.D) or Input.IsKeyPressed(Key.Right) then
        pos.x = pos.x + self.speed * dt
    end
    
    transform.Translation = pos
end

function PlayerController:OnDestroy()
    print("PlayerController destroyed!")
end

function PlayerController:OnCollisionEnter(other)
    if other:IsValid() then
        print("Player collided with: " .. other:GetTag())
    end
end

function PlayerController:OnCollisionExit(other)
    if other:IsValid() then
        print("Player collision exit: " .. other:GetTag())
    end
end

function PlayerController:OnTriggerEnter(other)
    if other:IsValid() then
        print("Player triggered by: " .. other:GetTag())
    end
end

function PlayerController:OnTriggerExit(other)
    if other:IsValid() then
        print("Player trigger exit: " .. other:GetTag())
    end
end

return PlayerController
