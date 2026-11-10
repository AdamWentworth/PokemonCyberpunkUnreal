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
		if (Slot != INDEX_NONE) GlowMaterial = Body->CreateDynamicMaterialInstance(Slot);
	}
}

void ABattlePokemon::EnterBattle()
{
	if (!IsHidden()) return;
	SetHealth(MaxHealth);
	SetActorHiddenInGame(false);
	Body->SetComponentTickEnabled(true);
	Body->PlayAnimation(IdleAnimation, true);
	SetActorTickEnabled(true);
}

void ABattlePokemon::SetHealth(int32 NewHealth)
{
	const int32 ClampedHealth = FMath::Clamp(NewHealth, 0, MaxHealth);
	if (CurrentHealth == ClampedHealth) return;
	CurrentHealth = ClampedHealth;
	OnHealthChanged.Broadcast();
}

void ABattlePokemon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UAnimInstance* Animation = Body->GetAnimInstance();
	if (!Animation) return;
	if (GlowMaterial)
	{
		GlowMaterial->SetVectorParameterValue(TEXT("GlowColor"), FLinearColor(
			Animation->GetCurveValue(TEXT("RingGlowR")),
			Animation->GetCurveValue(TEXT("RingGlowG")),
			Animation->GetCurveValue(TEXT("RingGlowB"))));
	}

	const float SparkleScale = Animation->GetCurveValue(TEXT("GemScale"));
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const bool bShowSparkle = !GemBone.IsNone() && SparkleScale > 0.0003f && Camera;
	GemSparkle->SetVisibility(bShowSparkle);
	if (!bShowSparkle) return;

	const FTransform Head = Body->GetSocketTransform(GemBone);
	const FVector Offset(Animation->GetCurveValue(TEXT("GemOffsetX")),
		Animation->GetCurveValue(TEXT("GemOffsetY")), Animation->GetCurveValue(TEXT("GemOffsetZ")));
	const FVector Forward = -Camera->GetCameraRotation().Vector();
	FVector Right = FVector::CrossProduct(Head.TransformVectorNoScale(GemUp), Forward).GetSafeNormal();
	if (Right.IsNearlyZero()) Right = Camera->GetActorRightVector();
	const FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();
	const float Scale = SparkleScale * Body->GetComponentScale().GetAbs().X;
	const FVector Center = Head.TransformPosition(Offset) + Scale *
		(Right * GemQuadOffset.X + Up * GemQuadOffset.Y + Forward * GemQuadOffset.Z);
	GemSparkle->SetWorldLocationAndRotation(Center, FRotationMatrix::MakeFromXZ(Right, Forward).ToQuat());
	// The shared plane is one metre wide; the imported attachment sizes are centimetres.
	GemSparkle->SetWorldScale3D(FVector(GemSize.X * Scale / 100, GemSize.Y * Scale / 100, 1));
}
