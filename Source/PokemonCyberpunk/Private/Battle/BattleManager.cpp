#include "Battle/BattleManager.h"
#include "Battle/BattlePokemon.h"

#include "Camera/CameraComponent.h"
#include "Character/WillieCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/GamePlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ABattleManager::ABattleManager()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Battlefield"));
	BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
	BattleCamera->SetupAttachment(RootComponent);
	BattleCamera->SetFieldOfView(65.0f);
	PlayerTrainerPosition = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerTrainerPosition"));
	PlayerTrainerPosition->SetupAttachment(RootComponent);
	PlayerTrainerPosition->SetRelativeLocation(FVector(0, -568.75f, 0));
	PlayerTrainerPosition->SetRelativeRotation(FRotator(0, 90, 0));
	OpponentTrainerPosition = CreateDefaultSubobject<USceneComponent>(TEXT("OpponentTrainerPosition"));
	OpponentTrainerPosition->SetupAttachment(RootComponent);
	OpponentTrainerPosition->SetRelativeLocation(FVector(0, 568.75f, 0));
	OpponentTrainerPosition->SetRelativeRotation(FRotator(0, -90, 0));
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(Opponent))
	{
		Opponent->OnBattleChallengeAccepted.AddDynamic(this, &ABattleManager::StartBattle);
	}
}

void ABattleManager::StartBattle(AWillieCharacter* Trainer)
{
	if (!IsValid(Trainer) || Trainer != Opponent) return;
	if (AGamePlayerController* Player = Cast<AGamePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (Player->IsInBattle() || Player->IsInDialogue()) return;
		Player->BeginBattleView(this);
		PlaceTrainer(Cast<ACharacter>(Player->GetPawn()), PlayerTrainerPosition);
		PlaceTrainer(Trainer, OpponentTrainerPosition);
		for (ABattlePokemon* Pokemon : PlayerTeam)
		{
			if (IsValid(Pokemon)) Pokemon->EnterBattle();
		}
		for (ABattlePokemon* Pokemon : OpponentTeam)
		{
			if (IsValid(Pokemon)) Pokemon->EnterBattle();
		}
		Player->ShowBattleHUD(PlayerTeam, OpponentTeam);
	}
}

void ABattleManager::PlaceTrainer(ACharacter* Trainer, const USceneComponent* Position)
{
	if (!IsValid(Trainer)) return;
	Trainer->GetCharacterMovement()->StopMovementImmediately();
	// Formation markers sit on the ground; a character's origin is its capsule center.
	const FVector Height(0, 0, Trainer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Trainer->SetActorLocationAndRotation(Position->GetComponentLocation() + Height,
		Position->GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}
