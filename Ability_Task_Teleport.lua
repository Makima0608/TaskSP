local SPAbility = require("Feature.StarP.Script.System.Ability.SPAbilityClasses")
local SPAbilityUtils = require("Feature.StarP.Script.System.Ability.SPAbilityUtils")

local Ability_Task_Teleport = UE4.Class(nil, "Ability_Task_Teleport")
local SPGameplayUtils = require("Feature.StarP.Script.System.SPGameplayUtils")
local bShowDebug = UE4.USPAbilityFunctionLibrary.IsVerboseDebug()

function Ability_Task_Teleport:OnTaskStartBP(Context)
    print("Ability_Task_Teleport:OnTaskStartBP")

    local bFindLocationSuccessed, DesiredTargetLocation, DesiredTargetRotation, ErrorText = self:FindDesiredLocation(Context)
    if bFindLocationSuccessed then
        print("Ability_Task_Teleport:bFindLocationSuccessed")

        --STP点和回到起点不应该SweepForWall
        if self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseSTPLocation or
            self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseLastTPStartLocation then
            self.SweepForWall = false
        end

        if self.SweepForWall  then
            -- local QueryObjectsArray = UE4.TArray(UE4.EObjectTypeQuery)
            -- QueryObjectsArray:Add(UE4.UMoeBlueprintLibrary.ConvertToObjectType(UE4.USPGameLibrary.GetCollisionChannelBySPTraceType(UE4.ESPTraceType.GameBuilding)))

            if self.bToGround then
                local groundHeight = self.GroundHeight or 500
                SPGameplayUtils:FlashActorToLocationToGround(Context:GetOwner(), DesiredTargetLocation, self.QueryAcceptableRadius, self.SweepQueryChannels, self.QueryNumberOfTest, groundHeight)
            else
                SPGameplayUtils:FlashActorToLocation(Context:GetOwner(), DesiredTargetLocation, self.QueryAcceptableRadius, self.SweepQueryChannels, self.QueryNumberOfTest)
                print("Ability_Task_Teleport:UseSweepForWallMove")
            end
        else
            local OwnerCapsuleComp = Context:GetOwner().CapsuleComponent
            if OwnerCapsuleComp then
                DesiredTargetLocation.Z = DesiredTargetLocation.Z + OwnerCapsuleComp:GetScaledCapsuleHalfHeight()
            end
            Context:GetOwner():K2_SetActorLocation(DesiredTargetLocation, false, nil, false)
            print("Ability_Task_Teleport:UseNormalWayMove")
        end

        if DesiredTargetRotation ~= nil then
            Context:GetOwner():K2_SetActorRotation(DesiredTargetRotation)
        end
    else
        print(ErrorText)
    end
end

function Ability_Task_Teleport:GetTaskRealmBP()
    -- return UE4.EAbleAbilityTaskRealm.ATR_Server
    if self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseLastTPStartLocation then
        return UE4.EAbleAbilityTaskRealm.ATR_Server
    else
        return UE4.EAbleAbilityTaskRealm.ATR_ClientAndServer
    end
end

function Ability_Task_Teleport:FindDesiredLocation(Context)
    local bFindLocationSuccessed = true;
    local ErrorText = ""

    local Owner = Context:GetOwner()
    local OwnerLocation = Owner:K2_GetActorLocation()
    local OwnerRotation = Owner:K2_GetActorRotation()
    local OwnerMeshComp = Owner:GetComponentByClass(UE4.USkeletalMeshComponent.StaticClass())
    if OwnerMeshComp then
        OwnerLocation = OwnerMeshComp:K2_GetComponentLocation()
        OwnerRotation = OwnerMeshComp:K2_GetComponentRotation()
    end
    if self.bSaveTPStartLocation == true and _SP.IsDSorStandalone then --是否需要存储传送开始时的位置
        Context:SetVectorParameter("TPStartLocation",OwnerLocation)
    end
    local ResultLocation = OwnerLocation
    local ResultRotation = nil
    local FixedOwnerRotation = UE4.UKismetMathLibrary.ComposeRotators(OwnerRotation, self.OffsetTransform.Rotation:ToRotator())
    local FixedLocationOffset = FixedOwnerRotation:GetRightVector() * self.OffsetTransform.Translation.X
            + FixedOwnerRotation:GetForwardVector() * -self.OffsetTransform.Translation.Y
            + FixedOwnerRotation:GetUpVector() * self.OffsetTransform.Translation.Z

    if self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseRelativeLocation then
        local FixedTargetLocation = FixedOwnerRotation:GetRightVector() * self.TargetLocation.X
                + FixedOwnerRotation:GetForwardVector() * -self.TargetLocation.Y
                + FixedOwnerRotation:GetUpVector() * self.TargetLocation.Z
        ResultLocation = ResultLocation + FixedLocationOffset + FixedTargetLocation;
    elseif self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseWorldLocation then
        ResultLocation = self.TargetLocation + FixedLocationOffset;
    elseif self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseLastTPStartLocation then --使用上一次传送存储的位置
        print("Ability_Task_Teleport:UseLastTPStartLocation")
        local TPStartLocation = UE4.FVector(0)
        local LastTPStartLocation = Context:GetVectorParameter("TPStartLocation")
        if LastTPStartLocation ~= nil then
            TPStartLocation = LastTPStartLocation
        else
            TPStartLocation = OwnerLocation --如果值不存在则使用当前位置坐标
        end
        ResultLocation = TPStartLocation + FixedLocationOffset
    elseif self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseDirection then
        ResultLocation = ResultLocation + OwnerRotation:GetRightVector():RotateAngleAxis(self.RotateAngle, UE4.FVector(0, 0, 1)) * self.Distance + FixedLocationOffset
    elseif self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseAbilityTarget then
        print("Ability_Task_Teleport:UseAbilityTarget")
        local TargetActor = self:GetSingleActorFromTargetTypeBP(Context, self.AbilityTargetType)
        if not UE4.UKismetSystemLibrary.IsValid(TargetActor) then
            return false, ResultLocation, ResultRotation, "TargetActor is not Valid"
        end
        local TargetLocation = TargetActor:K2_GetActorLocation()
        local TargetRotation = TargetActor:K2_GetActorRotation()

        local TargetMeshComp = TargetActor:GetComponentByClass(UE4.USkeletalMeshComponent.StaticClass())
        if TargetMeshComp then
            TargetLocation = TargetMeshComp:K2_GetComponentLocation()
            TargetRotation = TargetMeshComp:K2_GetComponentRotation()
        end

        local CameraMgr = UE4.UGameplayStatics.GetPlayerCameraManager(_SP.GetCurrentWorld(), 0)
        if self.TargetOrientationRule == UE4.EAbilityTargetOrientationRule.DirectlyToFace or not CameraMgr then
            local DirectionTargetToOwner = TargetLocation - OwnerLocation
            DirectionTargetToOwner.Z = 0
            DirectionTargetToOwner:Normalize()

            DirectionTargetToOwner = DirectionTargetToOwner:RotateAngleAxis(self.OffsetAngle, UE4.FVector(0, 0, 1))

            ResultLocation = TargetLocation - DirectionTargetToOwner * self.OffsetDistance + FixedLocationOffset + UE4.FVector(0, 0, self.OffsetHeight)
        else
            if self.TargetOrientationRule == UE4.EAbilityTargetOrientationRule.BaseOnTargetMeshOrientation then
                local TargetDirection = TargetRotation:GetRightVector()
                TargetDirection = TargetDirection:RotateAngleAxis(self.OffsetAngle, UE4.FVector(0, 0, 1))

                ResultLocation = TargetLocation + TargetDirection * self.OffsetDistance + FixedLocationOffset + UE4.FVector(0, 0, self.OffsetHeight)
            elseif self.TargetOrientationRule == UE4.EAbilityTargetOrientationRule.BaseOnTargetCameraOrientation then
                local CameraForward = CameraMgr:GetCameraRotation():GetForwardVector()
                CameraForward.Z = 0
                CameraForward:Normalize()

                CameraForward = CameraForward:RotateAngleAxis(self.OffsetAngle, UE4.FVector(0, 0, 1))
                ResultLocation = TargetLocation + CameraForward * self.OffsetDistance + FixedLocationOffset + UE4.FVector(0, 0, self.OffsetHeight)
            end
        end
    elseif self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseTalentUltimateLocation then
        local master = self:GetSingleActorFromTargetTypeBP(Context, self.AbilityTargetType)
        local masterMeshComp = master:GetComponentByClass(UE4.USkeletalMeshComponent.StaticClass())
        if masterMeshComp then
            -- Get Master Location
            local masterLocation = masterMeshComp:K2_GetComponentLocation()
            -- Get Face To Rotation
            local faceToRotator = masterMeshComp:K2_GetComponentRotation()
            faceToRotator.Pitch = 0
            faceToRotator.Roll = 0
            local offset = self:GetUltimateOffset(Context)
            local raletiveOffset = faceToRotator:RotateVector(offset)
            if raletiveOffset then
                ResultLocation = masterLocation - raletiveOffset
            end
        end
    elseif self.TeleportRule == _SP.SPAbilityUtils.ESPTeleportRule.UseSTPLocation then
        local STPLocation, STPRotation = self:GetConfigNearestDungeonSTP(Owner)
        if STPLocation == nil then
            bFindLocationSuccessed = false
        else
            ResultLocation = STPLocation + FixedLocationOffset
            ResultRotation = STPRotation
        end
    end

    if bShowDebug then
        UE4.UKismetSystemLibrary.DrawDebugSphere(Context:GetOwner(), ResultLocation, 150, 30, UE4.FLinearColor(0, 1, 0, 1), 5, 5)
        -- print(string.format("SPAbility Ability_Task_Teleport: using [%d] to teleport, target location is %d",self.TeleportRule,ResultLocation))
    end
    return bFindLocationSuccessed, ResultLocation, ResultRotation, ErrorText
end

function Ability_Task_Teleport:FixTargetLocation(InDesiredLocation, InAcceptableRange, OutTargetLocation)
    local World = self:GetWorld()
    if World then
        local bHasRandomLocation = UE4.UNavigationSystemV1.K2_GetRandomLocationInNavigableRadius(World, InDesiredLocation, OutTargetLocation, InAcceptableRange)
        if bHasRandomLocation then
            return true
        end
    end

    return false
end

function Ability_Task_Teleport:GetUltimateOffset(Context)
    local pet = Context:GetOwner()
    if pet == nil then
        return nil
    end

    local petTypeConfig = pet:GetMonsterTypeConfig()
    if petTypeConfig == nil then
        return nil
    end

    local ulitmateSocket = petTypeConfig.ultimateSocket
	if not ulitmateSocket then
		return UE4.FVector(0, 250, 50)
	end

    local meshComp = pet:GetComponentByClass(UE4.USkeletalMeshComponent.StaticClass())
	if meshComp and meshComp:DoesSocketExist(ulitmateSocket) then
		local relateTransform = meshComp:GetSocketTransform(ulitmateSocket, UE4.ERelativeTransformSpace.RTS_Component)
        local offset = relateTransform.Translation
        offset.Z = 0
        local scale = meshComp:K2_GetComponentScale()
		return offset * scale
	end

	return UE4.FVector(0, 250, 50)
end

---@param Character AActor
---@return TargetLocation, TargetRotation
function Ability_Task_Teleport:GetConfigNearestDungeonSTP(Character)
    --获取Monster所在的关卡偏移量
    local Locationoffset = UE4.FVector(0, 0, 0)
    local TargetMonster = Character:Cast(UE4.ASPGameMonsterBase)
    if TargetMonster ~= nil then
        Locationoffset = TargetMonster:GetMonsterDungeonLoc()
    end

    local STPIDList = {}
    local bUsePrimaryKey = false
    if self.STPUIDList:Length() > 0 then
        bUsePrimaryKey = true
        for index = 1, self.STPUIDList:Length() do
            table.insert(STPIDList, self.STPUIDList:Get(index))
        end
    else
        for index = 1, self.STPIDList:Length() do
            table.insert(STPIDList, self.STPIDList:Get(index))
        end
    end

    --获取表中配置的STP信息
    local findSTPInfo = SPAbilityUtils.GetSTPConfigList(STPIDList, bUsePrimaryKey)

    if #findSTPInfo == 0 then
        _SP.LogError("Ability_Task_Teleport", "GetConfigNearestDungeonSTP", "Unable to find matching STP points, Please check the ability configuration.")
        return nil, nil
    end

    local TargetLocation = nil
    local TargetRotation = nil
    --计算离目标最近的STP点
    local ownerLocation = Character:K2_GetActorLocation()
    local nearestDistance = nil
    for _, virtualPoint in ipairs(findSTPInfo) do
        local locParam = string.split(virtualPoint.pos, ',')
        local stpLocation = UE4.FVector(locParam[1], locParam[2], locParam[3]) + Locationoffset
        local distance = UE4.UKismetMathLibrary.Vector_Distance(ownerLocation, stpLocation)
        if nearestDistance == nil or distance < nearestDistance then
            nearestDistance = distance
            TargetLocation = stpLocation
            local rotParam = string.split(virtualPoint.rot, ',')
            if self.UseSTPRotation == true then
                TargetRotation = UE4.FRotator(rotParam[1], rotParam[2], rotParam[3])
            end
        end
    end

    return TargetLocation, TargetRotation
end


return Ability_Task_Teleport