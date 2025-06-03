local Ability_Task_SetPetAbnormal = UE4.Class(nil, "Ability_Task_SetPetAbnormal")

function Ability_Task_SetPetAbnormal:ErrLog(__FUNC_NAME__, ...)
    _SP.LogError("[Ability_Task_SetPosNear]" .. __FUNC_NAME__, ...)
end
function Ability_Task_SetPetAbnormal:OnTaskStartBP(Context)
    _SP.Log("SPAbilityTask", "Ability_Task_SetPetAbnormal:OnTaskStartBP")

    local OwnerActor = Context:GetOwner()
    if OwnerActor == nil then
        self:ErrLog("The OwnerActor is nil")
        return
    end

    local OwnerMonster = OwnerActor:Cast(UE4.ASPGameMonsterBase)
    if OwnerMonster == nil then
        self:ErrLog("The OwnerMonster is nil")
        return
    end

    if self.m_TaskTargets == nil then
        return
    end
    local len = self.m_TaskTargets:Length()
    for iter = 1, len do
        local TargetType = self.m_TaskTargets:Get(iter)
        local TargetActor = self:GetSingleActorFromTargetTypeBP(Context, TargetType)
        if TargetActor ~= nil and TargetActor:Cast(UE4.ASPGameMonsterBase) then
            if self.bSetAbnormal then
                _SP.SPAbilityUtils.EnterAbnormalState(TargetActor)
            else
                _SP.SPAbilityUtils.LeaveAbnormalState(TargetActor)
            end
        end
    end
end

function Ability_Task_SetPetAbnormal:OnTaskEndBP(Context)
    _SP.Log("SPAbilityTask", "Ability_Task_SetPetAbnormal:OnTaskEndBP")
    if self.m_TaskTargets == nil then
        return
    end
    local len = self.m_TaskTargets:Length()
    for iter = 1, len do
        local TargetType = self.m_TaskTargets:Get(iter)
        local TargetActor = self:GetSingleActorFromTargetTypeBP(Context, TargetType)
        if TargetActor ~= nil and TargetActor:Cast(UE4.ASPGameMonsterBase) then
            if self.bSetAbnormal then
                _SP.SPAbilityUtils.LeaveAbnormalState(TargetActor)
            else
                _SP.SPAbilityUtils.EnterAbnormalState(TargetActor)
            end
        end
    end
end

function Ability_Task_SetPetAbnormal:GetTaskRealmBP()
    return UE4.EAbleAbilityTaskRealm.ATR_Server
end

function Ability_Task_SetPetAbnormal:IsSingleFrameBP()
    return false
end

return Ability_Task_SetPetAbnormal