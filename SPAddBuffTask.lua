---@class SPAddBuffTask : UAbleAbilityTask
local SPAddBuffTask = UE4.Class(nil, "SPAddBuffTask")

local function Log(...)
    _SP.Log("SPAbility", "[SPAddBuffTask]", ...)
end

function SPAddBuffTask:OnTaskStartBP(Context)
    if self.m_Layer <= 0 then
        return
    end

    local TargetArray = UE4.TArray(UE4.AActor)
    self:GetActorsForTaskBP(Context, TargetArray)
    if TargetArray:Length() == 0 then
        return
    end

    local Instigator = self:GetSingleActorFromTargetTypeBP(Context, self.m_Instigator)
    for i = 1, TargetArray:Length() do
        local Target = TargetArray:Get(i)
        if Target and Target.GetAbilityComponent then
            ---@type SPAbilityComponent
            local AbilityComponent = Target:GetAbilityComponent()
            if AbilityComponent then
                local Ret = AbilityComponent:AddBuff(self.m_BuffID, Target, Instigator, self.m_Layer, 0, "SPAddBuffTask:OnTaskStartBP")
                if not Ret then
                    Log("OnTaskStart AddBuff id = ", self.m_BuffID, "to ", Target:GetName(), "failed")
                end
                -- Log("OnTaskStart AddBuff id = ", self.m_BuffID, "to ", Target:GetName())
            end
        end
    end
end

function SPAddBuffTask:OnTaskEndBP(Context)
    if self.m_bRemoveOnEnd then
        local TargetArray = UE4.TArray(UE4.AActor)
        self:GetActorsForTaskBP(Context, TargetArray)
        if TargetArray:Length() == 0 then
            return
        end
        local Target
        for i = 1, TargetArray:Length() do
            Target = TargetArray:Get(i)
            if Target and Target.GetAbilityComponent then
                ---@type SPAbilityComponent
                local AbilityComponent = Target:GetAbilityComponent()
                if AbilityComponent then
                    local Ret = AbilityComponent:RemoveBuff(self.m_BuffID, Target, self.m_Layer)
                    if not Ret then
                        Log("OnTaskEndBP RemoveBuff id = ", self.m_BuffID, "to ", Target:GetName(), "failed")
                    end
                    -- Log("OnTaskEndBP RemoveBuff id = ", self.m_BuffID, "to ", Target:GetName())
                end
            end
        end
    end
end

return SPAddBuffTask
