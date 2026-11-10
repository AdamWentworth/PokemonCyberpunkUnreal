#include "Battle/BattlePokemon.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

// Sets up the Pokemon's 3D model and animation behavior.
ABattlePokemon::ABattlePokemon()
{
    // Allow frame-by-frame updates, but keep them disabled until needed.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // Create the Pokemon's root and attach its animated 3D model.
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetRelativeRotation(FRotator(0, -90, 0));

    // Disable physical collisions and configure animation playback.
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

// Initializes health and hides the Pokemon until a battle begins.
void ABattlePokemon::BeginPlay()
{
    Super::BeginPlay();

    // Ensure maximum health is at least 1 and start at full health.
    MaxHealth = FMath::Max(1, MaxHealth);
    CurrentHealth = MaxHealth;

    // Keep Pokemon hidden and their animations inactive during exploration.
    SetActorHiddenInGame(true);
    Body->SetComponentTickEnabled(false);
}

// Makes the Pokemon visible, restores its health, and starts its idle animation.
void ABattlePokemon::EnterBattle()
{
    // Avoid activating a Pokemon that is already visible.
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

// Updates health and notifies the interface when its value changes.
void ABattlePokemon::SetHealth(int32 NewHealth)
{
    // Prevent health from dropping below 0 or exceeding maximum health.
    const int32 ClampedHealth = FMath::Clamp(NewHealth, 0, MaxHealth);

    // Avoid unnecessary updates when health has not changed.
    if (CurrentHealth == ClampedHealth)
    {
        return;
    }

    CurrentHealth = ClampedHealth;

    // Notify listeners, such as the health bar, to refresh their display.
    OnHealthChanged.Broadcast();
}

// Hides the Pokemon and disables updates when it leaves combat.
void ABattlePokemon::LeaveBattle()
{
    SetActorHiddenInGame(true);
    Body->SetComponentTickEnabled(false);
    SetActorTickEnabled(false);
}

// Knocks out the Pokemon by setting its health to zero.
void ABattlePokemon::KnockOut()
{
    SetHealth(0);

    // Fainted Pokemon immediately disappear from the battlefield.
    LeaveBattle();
}
