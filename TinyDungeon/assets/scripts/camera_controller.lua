local CameraController = {}

function CameraController:OnCreate()
    print("CameraController created!")
    
    -- Configurable parameters
    self.smoothSpeed = 5.0           -- Camera follow smoothness (higher = faster)
    self.targetEntityName = "Player" -- Entity to follow
    
    -- Map bounds (should match your tile map size)
    self.mapWidth = 50.0
    self.mapHeight = 30.0
end

function CameraController:OnUpdate(dt)
    if not self.entity:IsValid() then
        return
    end
    
    -- Find target entity (cache after first find)
    if not self.targetEntity then
        self.targetEntity = Scene.FindEntityByName(self.entity, self.targetEntityName)
        if not self.targetEntity then
            return
        end
    end
    
    if not self.targetEntity:IsValid() then
        self.targetEntity = nil
        return
    end
    
    -- Get transforms
    local cameraTransform = self.entity:GetTransform()
    local targetTransform = self.targetEntity:GetTransform()
    
    local targetX = targetTransform.Translation.x
    local targetY = targetTransform.Translation.y
    
    -- Get camera component for zoom level
    local camera = self.entity:GetCamera()
    local zoomLevel = camera:GetOrthographicSize()
    
    -- Calculate visible area half-size
    local aspectRatio = 960.0 / 576.0  -- TODO: get from viewport
    local halfHeight = zoomLevel / 2.0
    local halfWidth = halfHeight * aspectRatio
    
    -- Clamp camera to map bounds
    local minCamX = halfWidth
    local maxCamX = self.mapWidth - halfWidth
    local minCamY = halfHeight
    local maxCamY = self.mapHeight - halfHeight
    
    if minCamX > maxCamX then
        targetX = self.mapWidth / 2.0
    else
        targetX = math.max(minCamX, math.min(maxCamX, targetX))
    end
    
    if minCamY > maxCamY then
        targetY = self.mapHeight / 2.0
    else
        targetY = math.max(minCamY, math.min(maxCamY, targetY))
    end
    
    -- Smooth follow (lerp)
    local t = math.min(1.0, self.smoothSpeed * dt)
    cameraTransform.Translation.x = cameraTransform.Translation.x + (targetX - cameraTransform.Translation.x) * t
    cameraTransform.Translation.y = cameraTransform.Translation.y + (targetY - cameraTransform.Translation.y) * t
end

function CameraController:OnDestroy()
    print("CameraController destroyed!")
end

return CameraController
