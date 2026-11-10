#include "Battle/BattlePokemon.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

ABattlePokemon::ABattlePokemon()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetRelativeRotation(FRotator(0, -90, 0));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void ABattlePokemon::BeginPlay()
{
	Super::BeginPlay();
	MaxHealth = FMath::Max(1, MaxHealth);
	CurrentHealth = MaxHealth;
	// Pokemon wait out of sight and stop animating while the player is exploring.
	SetActorHiddenInGame(true);
	Body->SetComponentTickEnabled(false);
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
	// Keep health within its allowed range, then notify any health cards watching it.
	const int32 ClampedHealth = FMath::Clamp(NewHealth, 0, MaxHealth);
	if (CurrentHealth == ClampedHealth)
	{
		return;
	}
	CurrentHealth = ClampedHealth;
	OnHealthChanged.Broadcast();
}

void ABattlePokemon::LeaveBattle()
{
	SetActorHiddenInGame(true);
	Body->SetComponentTickEnabled(false);
	SetActorTickEnabled(false);
}

void ABattlePokemon::KnockOut()
{
	SetHealth(0);
	// For the basic battle loop, fainted Pokemon leave the field immediately.
	LeaveBattle();
}
