local CameraController = {}

function CameraController:OnCreate()
    print("CameraController created!")
end

function CameraController:OnUpdate(dt)
    -- Logic to follow player would go here once Scene.FindEntity is available
    -- local transform = self.entity:GetTransform()
end

return CameraController
