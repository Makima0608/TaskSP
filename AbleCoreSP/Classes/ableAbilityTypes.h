// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "AlphaBlend.h"
#include "GameFramework/DamageType.h"
#include "Engine/EngineTypes.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Engine/DataTable.h"

#include "ableAbilityTypes.generated.h"

class UAbleCollisionFilter;
class UAbleAbilityTargetingFilter;
UENUM(BlueprintType)
enum EAbleAbilityTargetType
{
	ATT_Self UMETA(DisplayName = "Self"),
	ATT_Owner UMETA(DisplayName = "Owner"),
	ATT_Instigator UMETA(DisplayName = "Instigator"),
	ATT_TargetActor UMETA(DisplayName = "Target Actor"),
	ATT_Camera UMETA(DisplayName = "Camera"),
	ATT_Location UMETA(DisplayName = "Location"),
	ATT_RandomLocations UMETA(DisplayName = "Random Locations"),
	ATT_World UMETA(DisplayName = "World")
};

UENUM(BlueprintType)
enum class ESPRelativeDirType : uint8
{
	Default = 1 UMETA(DisplayName="用Actor/Socket的Rotation"),
	SpawnToTargetDir = 2 UMETA(DisplayName="对着目标朝向"),
	SpawnToTargetPitchDirButForward = 3 UMETA(DisplayName="对着目标的Pitch朝向"),
	SpawnToTargetSnapDir = 4 UMETA(DisplayName="对着目标初始位置朝向"),
	InitialOrientation = 5 UMETA(DisplayName="使用技能触发时朝向"),
};

/* Helper Struct for Blend Times. */
USTRUCT(BlueprintType)
struct ABLECORESP_API FAbleBlendTimes
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (DisplayName = "Blend In"))
	float m_BlendIn;

	// Blend out time (in seconds).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (DisplayName = "Blend Out"))
	float m_BlendOut;

	FAbleBlendTimes()
		: m_BlendIn(0.0f),
		m_BlendOut(0.0f)
	{}
};

USTRUCT(BlueprintType)
struct FAbleCollisionFilterStruct
{
	GENERATED_USTRUCT_BODY()
	FAbleCollisionFilterStruct() {}
public:
	UPROPERTY(EditAnywhere, Instanced, meta = (DisplayName = "Filters"))
	TArray<UAbleCollisionFilter*> m_Filters;
};

USTRUCT(BlueprintType)
struct FAbleAbilityTargetingFilterStruct
{
	GENERATED_USTRUCT_BODY()
	FAbleAbilityTargetingFilterStruct() {}
public:
	UPROPERTY(EditAnywhere, Instanced, meta = (DisplayName = "Filters"))
	TArray<UAbleAbilityTargetingFilter*> m_Filters;
};

/* A Simple struct for shared logic that takes in a various information and returns a Transform based on options selected by the user. */
USTRUCT(BlueprintType)
struct ABLECORESP_API FAbleAbilityTargetTypeLocation
{
	GENERATED_USTRUCT_BODY()
public:
	FAbleAbilityTargetTypeLocation();

	void GetTargetTransform(const UAbleAbilityContext& Context, int32 TargetIndex, FTransform& OutTransform) const;
	void GetTransform(const UAbleAbilityContext& Context, FTransform& OutTransform) const;
	AActor* GetSourceActor(const UAbleAbilityContext& Context) const;
	EAbleAbilityTargetType GetSourceTargetType() const { return m_Source.GetValue(); }
	bool NeedSpawnTraceToGround() const { return m_SpawnTraceToGround; }
	bool GetModifyRotation(const UAbleAbilityContext& Context, FRotator& Rotator) const;
	void AutoAdjustPitchTargetLocation(FVector& OutLocation, const FVector & SourceLocation, const UAbleAbilityContext& Context) const;
	bool GetSocketTransform(const UAbleAbilityContext& Context, FTransform& OutTransform) const;
	bool NeedModifyRotation(const UAbleAbilityContext& Context) const;
	bool TryLocationToGround(const UAbleAbilityContext& Context, FTransform& OutTransform) const;
	
	FORCEINLINE const FName& GetSocketName() const { return m_Socket; }
	FORCEINLINE const FVector& GetOffset() const { return m_Offset; }
	FORCEINLINE const FRotator& GetRotation() const { return m_Rotation; }
	FORCEINLINE const ESPRelativeDirType& GetRelativeDirType() const { return m_RelativeDirType; }
	FORCEINLINE const int32 GetTargetIndex() const { return m_TargetIndex; }
	FORCEINLINE const bool IsUseSocketRotation() const { return m_UseSocketRotation; }
	FORCEINLINE const bool IsUseActorRotation() const { return m_UseActorRotation; }
	FORCEINLINE const bool IsUseWorldRotation() const { return m_UseWorldRotation; }
protected:
	/* The source to launch this targeting query from. All checks (distance, etc) will be in relation to this source. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Source"))
	TEnumAsByte<EAbleAbilityTargetType> m_Source;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Target Index", EditCondition = "m_Source == EAbleAbilityTargetType::ATT_TargetActor", EditConditionHides))
	int32 m_TargetIndex;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Spawn Trace To Ground"))
	bool m_SpawnTraceToGround;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "贴地检测是否忽略自己", EditCondition = "m_SpawnTraceToGround == true", EditConditionHides))
	bool m_IgnoreSelfWhenTraceToGround;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "贴地后Z轴偏移调整值", EditCondition = "m_SpawnTraceToGround == true", EditConditionHides))
	float m_ZOffsetAfterTraceToGround;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "是否允许旋转贴地"))
	bool m_AllowRotateTraceToGround;
	
	/* An additional Offset to provide to our location. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Location Offset"))
	FVector m_Offset;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "是否是世界坐标偏移"))
	bool m_bWorldOffset;
	
	/* Rotation to use for the location. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Rotation"))
	FRotator m_Rotation;

	/* Socket to use as the Base Transform or Location.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Socket"))
	FName m_Socket;

	/* If true, the Socket rotation will be used as well as its location.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Use Socket Rotation", EditCondition = "!m_UseActorRotation && m_RelativeDirType == ESPRelativeDirType::Default"))
	bool m_UseSocketRotation;

	/* If true, the Actor rotation will be used.*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Use Actor Rotation", EditCondition = "!m_UseSocketRotation && m_RelativeDirType == ESPRelativeDirType::Default"))
	bool m_UseActorRotation;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Use World Rotation", EditCondition = "!m_UseSocketRotation && !m_UseActorRotation && m_RelativeDirType == ESPRelativeDirType::Default"))
	bool m_UseWorldRotation = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "World Rotation", EditCondition = "m_UseWorldRotation && m_RelativeDirType == ESPRelativeDirType::Default"))
	FRotator m_WorldRotator;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Use Actor Mesh Rotation", EditCondition = "!m_UseActorRotation && !m_UseSocketRotation && m_RelativeDirType == ESPRelativeDirType::Default"))
	bool m_UseActorMeshRotation;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "目标在发射点背后是否能被瞄准"))
	bool m_bCanAimingByTargetBehind;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "当目标在背后是否修正旋转"))
	bool m_bModifyBehindTarget;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "旋转类型设置"))
	ESPRelativeDirType m_RelativeDirType = ESPRelativeDirType::Default;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "朝向旋转最小值", EditCondition = "m_limitRotationPitch||m_limitRotationYaw||m_limitRotationRoll"))
	FRotator m_RotationMin;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "朝向旋转最大值", EditCondition = "m_limitRotationPitch||m_limitRotationYaw||m_limitRotationRoll"))
	FRotator m_RotationhMax;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "开启限制Roll", EditCondition = "m_RelativeDirType != ESPRelativeDirType::Default"))
	bool m_limitRotationRoll;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "开启限制Pitch", EditCondition = "m_RelativeDirType != ESPRelativeDirType::Default"))
	bool m_limitRotationPitch;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "开启限制Yaw", EditCondition = "m_RelativeDirType != ESPRelativeDirType::Default"))
	bool m_limitRotationYaw;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "是否启用子弹自动调整Pitch", EditCondition = "m_RelativeDirType == ESPRelativeDirType::SpawnToTargetPitchDirButForward"))
	bool m_bAutoAdjustRotationPitch;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Random Location Index", EditCondition = "m_Source==EAbleAbilityTargetType::ATT_RandomLocations"))
	int32 m_RandomLocationIndex;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Location", meta = (DisplayName = "Navigation Extend"))
	FVector m_NavigationExtend;
};

UENUM(BlueprintType)
enum EAbleAbilityTaskResult
{
	// Task finished normally (e.g. reached its end time).
	Successful UMETA(DisplayName = "Successful"),

	// Ability is being branched to another ability.
	Branched UMETA(DisplayName = "Branched"),

	// Ability was interrupted.
	Interrupted UMETA(DisplayName = "Interrupted"),

    // Ability was decayed.
    Decayed UMETA(DisplayName = "Decayed"),

	BranchSegment UMETA(DisplayName = "BranchSegment"),

	StopAcross UMETA(DisplayName = "StopAcross")
};

UENUM(BlueprintType)
enum EAbleAbilityTaskRealm
{
	ATR_Client = 0 UMETA(DisplayName = "Client"),
	ATR_Server UMETA(DisplayName = "Server"),
	ATR_ClientAndServer UMETA(DisplayName = "Client And Server"),

	ATR_TotalRealms UMETA(DisplayName = "Internal_DO_NOT_USE")
};

UENUM(BlueprintType)
enum EAbleConditionResults
{
	ACR_Passed = 0 UMETA(DisplayName = "Passed"),
	ACR_Failed UMETA(DisplayName = "Failed"),
	ACR_Ignored UMETA(DisplayName = "Ignored")
};

UENUM(BlueprintType)
enum class EAblePlayCameraShakeStopMode : uint8
{
	DontStop,
	Stop,
	StopImmediately,
};

/**
 * 碰撞通道枚举
 * 这个枚举是ESPTraceType的映射，由于Able.uplugin中无法访问Feature_SP,需要有映射字段来维护
 * 该枚举规则是只能在后边加，不能调整顺序和在中间位置插入
 */
UENUM(BlueprintType)
enum ESPAbleTraceType
{
	AT_None = 0,
	AT_Gameplay,
	AT_Weapon,
	AT_Pawn,
	AT_WorldStatic,
	AT_Floor,
	AT_Climbable,
	AT_CaptureBall,
	AT_CaptureTrace,
	AT_CallMonster,
	AT_WildMonster,
	AT_GameBuilding,
	AT_GameProps,
	AT_Choppable,
	AT_BlockBuilding,
	AT_ActualMesh,
	AT_PackageAirWall,
	AT_InterWater,
};

UENUM(BlueprintType)
enum EAbleChannelPresent
{
	ACP_Default UMETA(DisplayName = "预设-默认预设[玩家|星兽|矿树|建筑]"),
	ACP_Soc UMETA(DisplayName = "预设-Soc矿树"),
	ACP_PlaceHolder1 UMETA(DisplayName = "预设-占位用1"),
	ACP_PlaceHolder2 UMETA(DisplayName = "预设-[空气墙|静态物体]"),
	ACP_PlaceHolder3 UMETA(DisplayName = "预设-占位用3"),
	ACP_Character UMETA(DisplayName = "预设-[玩家|星兽]"),
	ACP_PlaceHolder5 UMETA(DisplayName = "预设-占位用5"),
	ACP_NonePresent UMETA(DisplayName = "预设-不使用"),
};

USTRUCT(BlueprintType)
struct FAbleChannelPresent : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TEnumAsByte<EAbleChannelPresent> Present;
	
	UPROPERTY(EditAnywhere)
	TArray<TEnumAsByte<ESPAbleTraceType>> Channels;
};

#define ABL_DECLARE_DYNAMIC_PROPERTY(Type, Property) TAttribute<Type> Property##Binding;
#define ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, Property) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context.Get(), Property) : Property)
#define ABL_GET_DYNAMIC_PROPERTY_VALUE_ENUM(Context, Property) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context.Get(), Property.GetValue()) : Property.GetValue())
#define ABL_GET_DYNAMIC_PROPERTY_VALUE_RAW(Context, Property) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context, Property) : Property)
#define ABL_BIND_DYNAMIC_PROPERTY(Ability, Property, DisplayName) (Property##Delegate.BindUFunction(Ability, GetDynamicDelegateName(DisplayName)))

namespace UMAbleAbility
{
	enum
	{
		// [PropertyMetadata] This property allows properties to be bind-able to a BP delegate, otherwise it uses the constant value defined by the user.
		// UPROPERTY(meta=(AbleBindableProperty))
		AbleBindableProperty,

		// [PropertyMetadata] Specifies the name of the BP Function to call by default for a binding property.
		// UPROPERTY(meta=(AbleDefaultBinding))
		AbleDefaultBinding,
	};
}

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSoftObjectPtr<UAnimationAsset>, FGetAbleAnimation, const UAbleAbilityContext*, Context, TSoftObjectPtr<UAnimationAsset> , StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(USoundBase*, FGetAbleSound, const UAbleAbilityContext*, Context, USoundBase*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(USoundAttenuation*, FGetAbleAttenuation, const UAbleAbilityContext*, Context, USoundAttenuation*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(USoundConcurrency*, FGetAbleConcurrency, const UAbleAbilityContext*, Context, USoundConcurrency*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSoftObjectPtr<UParticleSystem>, FGetAbleParticleSystem, const UAbleAbilityContext*, Context, TSoftObjectPtr<UParticleSystem>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UMaterialInterface*, FGetAbleMaterial, const UAbleAbilityContext*, Context, UMaterialInterface*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(UTexture*, FGetAbleTexture, const UAbleAbilityContext*, Context, UTexture*, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FVector, FGetAbleVector, const UAbleAbilityContext*, Context, FVector, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FLinearColor, FGetAbleColor, const UAbleAbilityContext*, Context, FLinearColor, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(float, FGetAbleFloat, const UAbleAbilityContext*, Context, float, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FGetAbleBool, const UAbleAbilityContext*, Context, bool, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(int32, FGetAbleInt, const UAbleAbilityContext*, Context, int32, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FAbleBlendTimes, FGetAbleBlendTimes, const UAbleAbilityContext*, Context, FAbleBlendTimes, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSubclassOf<UDamageType>, FGetDamageTypeClass, const UAbleAbilityContext*, Context, TSubclassOf<UDamageType>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FAbleAbilityTargetTypeLocation, FGetAbleTargetLocation, const UAbleAbilityContext*, Context, FAbleAbilityTargetTypeLocation, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TSubclassOf<UMatineeCameraShake>, FGetCameraShakeClass, const UAbleAbilityContext*, Context, TSubclassOf<UMatineeCameraShake>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(EAblePlayCameraShakeStopMode, FGetCameraStopMode, const UAbleAbilityContext*, Context, EAblePlayCameraShakeStopMode, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(TArray<FName>, FGetNameArray, const UAbleAbilityContext*, Context, TArray<FName>, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(EAbleAbilityTargetType, FGetAbleTargetType, const UAbleAbilityContext*, Context, EAbleAbilityTargetType, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FName, FGetAbleName, const UAbleAbilityContext*, Context, FName, StaticValue);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FRadialDamageParams, FGetAbleRadialDamageParams, const UAbleAbilityContext*, Context, FRadialDamageParams, StaticValue);

// One offs
DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FGetAbleBoolWithResult, const UAbleAbilityContext*, Context, bool, StaticValue, EAbleAbilityTaskResult, Result);
#define ABL_GET_DYNAMIC_PROPERTY_VALUE_THREE(Context, Property, Result) (Property##Delegate.IsBound() ? Property##Delegate.Execute(Context.Get(), Property, Result) : Property)