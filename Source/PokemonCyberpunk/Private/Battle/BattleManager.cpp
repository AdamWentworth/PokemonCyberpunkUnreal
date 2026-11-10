#include "Battle/BattleManager.h"

#include "Battle/BattlePokemon.h"
#include "Camera/CameraComponent.h"
#include "Character/WillieCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
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

	constexpr float TrainerDistanceFromCenter = 568.75f;
	PlayerTrainerPosition = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerTrainerPosition"));
	PlayerTrainerPosition->SetupAttachment(RootComponent);
	PlayerTrainerPosition->SetRelativeLocation(FVector(0, -TrainerDistanceFromCenter, 0));
	PlayerTrainerPosition->SetRelativeRotation(FRotator(0, 90, 0));
	OpponentTrainerPosition = CreateDefaultSubobject<USceneComponent>(TEXT("OpponentTrainerPosition"));
	OpponentTrainerPosition->SetupAttachment(RootComponent);
	OpponentTrainerPosition->SetRelativeLocation(FVector(0, TrainerDistanceFromCenter, 0));
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
	if (!IsValid(Trainer) || Trainer != Opponent)
	{
		return;
	}
	if (AGamePlayerController* PlayerController = Cast<AGamePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (PlayerController->IsInBattle() || PlayerController->IsInDialogue())
		{
			return;
		}
		PlayerController->BeginBattleView(this);
		PlaceTrainer(Cast<ACharacter>(PlayerController->GetPawn()), PlayerTrainerPosition);
		PlaceTrainer(Trainer, OpponentTrainerPosition);
		for (ABattlePokemon* Pokemon : PlayerTeam)
		{
			if (IsValid(Pokemon))
			{
				Pokemon->EnterBattle();
			}
		}
		for (ABattlePokemon* Pokemon : OpponentTeam)
		{
			if (IsValid(Pokemon))
			{
				Pokemon->EnterBattle();
			}
		}
		PlayerController->ShowBattleHUD(PlayerTeam, OpponentTeam);
	}
}

void ABattleManager::PlaceTrainer(ACharacter* Trainer, const USceneComponent* Position)
{
	if (!IsValid(Trainer))
	{
		return;
	}
	Trainer->GetCharacterMovement()->StopMovementImmediately();
	// Formation markers sit on the ground; a character's origin is its capsule center.
	const FVector CapsuleOffset(0, 0, Trainer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	Trainer->SetActorLocationAndRotation(Position->GetComponentLocation() + CapsuleOffset,
		Position->GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}
