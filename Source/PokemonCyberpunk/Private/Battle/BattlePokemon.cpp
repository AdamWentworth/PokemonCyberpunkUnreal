#include "Battle/BattlePokemon.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

ABattlePokemon::ABattlePokemon()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetRelativeRotation(FRotator(0, -90, 0));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GemSparkle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GemSparkle"));
	GemSparkle->SetupAttachment(RootComponent);
	GemSparkle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GemSparkle->SetCastShadow(false);
}

void ABattlePokemon::BeginPlay()
{
	Super::BeginPlay();
	MaxHealth = FMath::Max(1, MaxHealth);
	CurrentHealth = MaxHealth;
	SetActorHiddenInGame(true);
	Body->SetComponentTickEnabled(false);
	GemSparkle->SetVisibility(false);
	if (!GlowMaterialSlot.IsNone())
	{
		const int32 Slot = Body->GetMaterialIndex(GlowMaterialSlot);
		if (Slot != INDEX_NONE)
		{
			GlowMaterial = Body->CreateDynamicMaterialInstance(Slot);
		}
	}
}

void ABattlePokemon::EnterBattle()
{
	if (!IsHidden())
	{
		return;
	}
	SetHealth(MaxHealth);
	SetActorHiddenInGame(false);
	Body->SetComponentTickEnabled(true);
	Body->PlayAnimation(IdleAnimation, true);
	SetActorTickEnabled(true);
}

void ABattlePokemon::SetHealth(int32 NewHealth)
{
	const int32 ClampedHealth = FMath::Clamp(NewHealth, 0, MaxHealth);
	if (CurrentHealth == ClampedHealth)
	{
		return;
	}
	CurrentHealth = ClampedHealth;
	OnHealthChanged.Broadcast();
}

void ABattlePokemon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UAnimInstance* Animation = Body->GetAnimInstance();
	if (!Animation)
	{
		return;
	}

	UpdateRingGlow(*Animation);
	UpdateGemSparkle(*Animation);
}

void ABattlePokemon::UpdateRingGlow(const UAnimInstance& Animation)
{
	if (!GlowMaterial)
	{
		return;
	}

	const FLinearColor GlowColor(
		Animation.GetCurveValue(TEXT("RingGlowR")),
		Animation.GetCurveValue(TEXT("RingGlowG")),
		Animation.GetCurveValue(TEXT("RingGlowB")));
	GlowMaterial->SetVectorParameterValue(TEXT("GlowColor"), GlowColor);
}

void ABattlePokemon::UpdateGemSparkle(const UAnimInstance& Animation)
{
	constexpr float MinimumSparkleScale = 0.0003f;
	const float SparkleScale = Animation.GetCurveValue(TEXT("GemScale"));
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const bool bShowSparkle = !GemBone.IsNone() && SparkleScale > MinimumSparkleScale && Camera;
	GemSparkle->SetVisibility(bShowSparkle);
	if (!bShowSparkle)
	{
		return;
	}

	// The animation offset is local to the gem bone. The quad faces the camera,
	// with its up direction following that bone's orientation.
	const FTransform GemTransform = Body->GetSocketTransform(GemBone);
	const FVector AnimationOffset(Animation.GetCurveValue(TEXT("GemOffsetX")),
		Animation.GetCurveValue(TEXT("GemOffsetY")), Animation.GetCurveValue(TEXT("GemOffsetZ")));
	const FVector BillboardForward = -Camera->GetCameraRotation().Vector();
	FVector BillboardRight = FVector::CrossProduct(
		GemTransform.TransformVectorNoScale(GemUp), BillboardForward).GetSafeNormal();
	if (BillboardRight.IsNearlyZero())
	{
		BillboardRight = Camera->GetActorRightVector();
	}
	const FVector BillboardUp = FVector::CrossProduct(BillboardForward, BillboardRight).GetSafeNormal();
	const float WorldSparkleScale = SparkleScale * Body->GetComponentScale().GetAbs().X;
	const FVector SparkleLocation = GemTransform.TransformPosition(AnimationOffset) + WorldSparkleScale *
		(BillboardRight * GemQuadOffset.X + BillboardUp * GemQuadOffset.Y + BillboardForward * GemQuadOffset.Z);
	GemSparkle->SetWorldLocationAndRotation(SparkleLocation,
		FRotationMatrix::MakeFromXZ(BillboardRight, BillboardForward).ToQuat());

	// Convert the imported centimetre sizes to scale factors for the one-metre plane.
	constexpr float PlaneWidthCm = 100.0f;
	GemSparkle->SetWorldScale3D(FVector(GemSize.X * WorldSparkleScale / PlaneWidthCm,
		GemSize.Y * WorldSparkleScale / PlaneWidthCm, 1.0f));
}
