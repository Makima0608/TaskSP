// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Animation/AnimNode_SPAbilityAnimPlayer.h"

#include "AbleCoreSPPrivate.h"

#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"

FAnimNode_SPAbilityAnimPlayer::FAnimNode_SPAbilityAnimPlayer()
	: m_CachedOutputSequence(nullptr),
	m_CachedOutputTime(0.0f),
	m_AnimationQueue(),
	m_BlendType(EvaluateBlendType::Single)
{

}

float FAnimNode_SPAbilityAnimPlayer::GetCurrentAssetTime()
{
	if (m_AnimationQueue.Num())
	{
		return m_AnimationQueue[0].TimeAccumulator;
	}

	return 0.0f;
}

float FAnimNode_SPAbilityAnimPlayer::GetCurrentAssetLength()
{
	if (m_AnimationQueue.Num() && m_AnimationQueue[0].AnimationSequence)
	{
		return m_AnimationQueue[0].AnimationSequence->SequenceLength;
	}

	return 0.0f;
}

float FAnimNode_SPAbilityAnimPlayer::GetCurrentAssetTimePlayRateAdjusted()
{
	if (m_AnimationQueue.Num() && m_AnimationQueue[0].AnimationSequence)
	{
		const float SequencePlayRate = m_AnimationQueue[0].AnimationSequence->RateScale;
		const float FinalPlayRate = m_AnimationQueue[0].PlayRate * SequencePlayRate;
		return FinalPlayRate < 0.0f ? GetCurrentAssetLength() - m_AnimationQueue[0].TimeAccumulator : m_AnimationQueue[0].TimeAccumulator;
	}

	return 0.0f;
}

void FAnimNode_SPAbilityAnimPlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);
}

void FAnimNode_SPAbilityAnimPlayer::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	float BlendValue = 1.0f;
	m_BlendType = EvaluateBlendType::Single;

	// Handle popping off any entries first.
	if (m_AnimationQueue.Num())
	{
		// Remove the entry if we've:

		// Finished our animation.
		bool popEntry = m_AnimationQueue[0].GetTimeRemaining() <= 0.0f;
		
		if (m_AnimationQueue.Num() > 1)
		{
			if (m_AnimationQueue[1].BlendIn.GetBlendTimeRemaining() <= 0.0f)
			{
				// We have just another entry waiting.
				popEntry = true;
			}
			else
			{
				// Or we have another entry, with blend in, and we've finished the blend in.
				popEntry |= m_AnimationQueue[1].BlendIn.IsComplete();
			}
		}

		if (popEntry)
		{
			m_AnimationQueue.RemoveAt(0);
		}
	}

	// Now, Update normally.
	if (m_AnimationQueue.Num())
	{
		FSPAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
		FSPAbilityAnimEntry* NextEntry = m_AnimationQueue.Num() > 1 ? &m_AnimationQueue[1] : nullptr;

		CurrentEntry.UpdateEntry(Context.GetDeltaTime());
		if (NextEntry)
		{
			NextEntry->UpdateEntry(Context.GetDeltaTime());
		}

		float TimeRemaining = CurrentEntry.GetTimeRemaining();

		// Find out what Blend we're using, if any. 
		if (NextEntry)
		{
			NextEntry->BlendIn.Update(Context.GetDeltaTime());
			BlendValue -= NextEntry->BlendIn.GetAlpha();

			m_BlendType = EvaluateBlendType::Multi;
		}
		else if (CurrentEntry.GetTimeRemaining() <= CurrentEntry.BlendOut.GetBlendTimeRemaining()) 
		{
			CurrentEntry.BlendOut.Update(Context.GetDeltaTime());
			BlendValue -= CurrentEntry.BlendOut.GetAlpha();

			// Turning this off for now, need to think about WHAT to blend to. Likely need an animation layer of some sort...
			//m_BlendType = EvaluateBlendType::SingleBlendOut;
		}

		switch (m_BlendType)
		{
			default:
			case EvaluateBlendType::Single:
			case EvaluateBlendType::SingleBlendOut:
			{
				if (Context.AnimInstanceProxy->IsSkeletonCompatible(CurrentEntry.AnimationSequence->GetSkeleton()))
				{
					if (FAnimInstanceProxy* Proxy = Context.AnimInstanceProxy)
					{
						FAnimGroupInstance* SyncGroup;
						FAnimTickRecord& SequenceTickRecord = Proxy->CreateUninitializedTickRecord(SyncGroup, NAME_None);
						Proxy->MakeSequenceTickRecord(SequenceTickRecord, const_cast<UAnimSequence*>(CurrentEntry.AnimationSequence), false, CurrentEntry.PlayRate, BlendValue, CurrentEntry.TimeAccumulator, CurrentEntry.MarkerTickRecord);
					}
				}
			}
			break;
			case EvaluateBlendType::Multi:
			{
				if (Context.AnimInstanceProxy->IsSkeletonCompatible(CurrentEntry.AnimationSequence->GetSkeleton()))
				{
					if (FAnimInstanceProxy* Proxy = Context.AnimInstanceProxy)
					{
						FAnimGroupInstance* SyncGroup;
						FAnimTickRecord& SequenceTickRecord = Proxy->CreateUninitializedTickRecord(SyncGroup, NAME_None);
						Proxy->MakeSequenceTickRecord(SequenceTickRecord, const_cast<UAnimSequence*>(CurrentEntry.AnimationSequence), false, CurrentEntry.PlayRate, BlendValue, CurrentEntry.TimeAccumulator, CurrentEntry.MarkerTickRecord);

						FAnimTickRecord& NextSequenceTickRecord = Proxy->CreateUninitializedTickRecord(SyncGroup, NAME_None);
						Proxy->MakeSequenceTickRecord(NextSequenceTickRecord, const_cast<UAnimSequence*>(NextEntry->AnimationSequence), false, NextEntry->PlayRate, 1.0f - BlendValue, NextEntry->TimeAccumulator, NextEntry->MarkerTickRecord);
					}
				}
			}
			break;
		}
	}
}

void FAnimNode_SPAbilityAnimPlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	check(Output.AnimInstanceProxy);
	FAnimInstanceProxy* Proxy = Output.AnimInstanceProxy;
	FAnimationPoseData PoseData(Output);
	if (!m_AnimationQueue.Num())
	{
		if (m_CachedOutputSequence)
		{
			m_CachedOutputSequence->GetAnimationPose(PoseData, FAnimExtractContext(m_CachedOutputTime, Output.AnimInstanceProxy->ShouldExtractRootMotion()));
		}
		else
		{
			Output.ResetToRefPose();
		}

		return;
	}

	switch (m_BlendType)
	{
		case EvaluateBlendType::Single:
		{
			FSPAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
			CurrentEntry.AnimationSequence->GetAnimationPose(PoseData, FAnimExtractContext(CurrentEntry.TimeAccumulator, Output.AnimInstanceProxy->ShouldExtractRootMotion()));
		}
		break;
		case EvaluateBlendType::Multi:
		case EvaluateBlendType::SingleBlendOut:
		{
			FSPAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
			FSPAbilityAnimEntry* NextEntry = m_AnimationQueue.Num() > 1 ? &m_AnimationQueue[1] : nullptr;

			FCompactPose Poses[2];
			FBlendedCurve Curves[2];
			FStackCustomAttributes Attribs[2];
			float Weights[2] = { 0.0f };

			FAnimationPoseData PoseA(Poses[0], Curves[0], Attribs[0]);
			FAnimationPoseData PoseB(Poses[1], Curves[1], Attribs[1]);

			const FBoneContainer& RequiredBone = Proxy->GetRequiredBones();
			Poses[0].SetBoneContainer(&RequiredBone);
			Poses[1].SetBoneContainer(&RequiredBone);

			Curves[0].InitFrom(RequiredBone);
			Curves[1].InitFrom(RequiredBone);

			float AlphaValue = 0.0f;

			if (NextEntry)
			{
				AlphaValue += NextEntry->BlendIn.GetAlpha();
			}

			Weights[0] = 1.0f - AlphaValue;
			Weights[1] = AlphaValue;

			CurrentEntry.AnimationSequence->GetAnimationPose(PoseA, FAnimExtractContext(CurrentEntry.TimeAccumulator, Proxy->ShouldExtractRootMotion()));
			if (NextEntry)
			{
				NextEntry->AnimationSequence->GetAnimationPose(PoseB, FAnimExtractContext(NextEntry->TimeAccumulator, Proxy->ShouldExtractRootMotion()));
			}

			FAnimationRuntime::BlendPosesTogether(Poses, Curves, Attribs, Weights, PoseData);
		}
		break;
		default:
			checkNoEntry();
			break;
	}

	if (m_AnimationQueue.Num())
	{
		m_CachedOutputSequence = m_AnimationQueue[0].AnimationSequence;
		m_CachedOutputTime = m_AnimationQueue[0].TimeAccumulator;
	}
}

void FAnimNode_SPAbilityAnimPlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugData.AddDebugItem(DebugLine, true);
}

void FAnimNode_SPAbilityAnimPlayer::PlayAnimationSequence(const UAnimSequence* Animation, float PlayRate,const FAlphaBlend& BlendIn, const FAlphaBlend& BlendOut)
{
	check(Animation);

	m_AnimationQueue.Add(FSPAbilityAnimEntry(Animation, BlendIn, BlendOut, PlayRate));
}

void FAnimNode_SPAbilityAnimPlayer::OnAbilityInterrupted(bool clearQueue)
{
	if (clearQueue)
	{
		m_AnimationQueue.Empty();
		m_CachedOutputSequence = nullptr;
		m_CachedOutputTime = 0.0f;
	}
	else if (m_AnimationQueue.Num() != 0)
	{
		m_AnimationQueue.RemoveAt(0);
	}
}

void FAnimNode_SPAbilityAnimPlayer::SetAnimationTime(float NewTime)
{
	if (m_AnimationQueue.Num())
	{
		m_AnimationQueue[0].TimeAccumulator = NewTime;
	}
}

void FAnimNode_SPAbilityAnimPlayer::ResetInternalTimeAccumulator()
{
	if (m_AnimationQueue.Num())
	{
		FSPAbilityAnimEntry& CurrentEntry = m_AnimationQueue[0];
		if (CurrentEntry.AnimationSequence && (CurrentEntry.PlayRate * CurrentEntry.AnimationSequence->RateScale) < 0.0f)
		{
			CurrentEntry.TimeAccumulator = CurrentEntry.AnimationSequence->SequenceLength;
		}
		else
		{
			CurrentEntry.TimeAccumulator = 0.0f;
		}
	}
}

FSPAbilityAnimEntry::FSPAbilityAnimEntry()
	: AnimationSequence(nullptr),
	TimeAccumulator(0.0f),
	BlendIn(),
	BlendOut(),
	PlayRate(1.0f)
{
	BlendIn.Reset();
	BlendOut.Reset();
}

FSPAbilityAnimEntry::FSPAbilityAnimEntry(const UAnimSequence* InSequence, const FAlphaBlend& InBlend, const FAlphaBlend& OutBlend, float InPlayRate)
	: AnimationSequence(InSequence),
	TimeAccumulator(0.0f),
	BlendIn(InBlend),
	BlendOut(OutBlend),
	PlayRate(InPlayRate)
{
	BlendIn.Reset();
	BlendOut.Reset();
}

void FSPAbilityAnimEntry::UpdateEntry(float deltaTime)
{
	TimeAccumulator = FMath::Clamp(TimeAccumulator, 0.0f, AnimationSequence->SequenceLength);
}

float FSPAbilityAnimEntry::GetTimeRemaining() const
{
	return FMath::Max<float>(AnimationSequence->SequenceLength - TimeAccumulator, 0.0f);
}
