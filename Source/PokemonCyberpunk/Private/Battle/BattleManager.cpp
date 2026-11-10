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
		QueuedCommands.Reset();
		SelectingPlayerIndex = 0;
		SelectedMoveIndex = INDEX_NONE;
		bSelectingCommands = true;
		PlayerController->ShowBattleCommands(this);
	}
}

ABattlePokemon* ABattleManager::GetSelectingPokemon() const
{
	return bSelectingCommands && PlayerTeam.IsValidIndex(SelectingPlayerIndex)
		? PlayerTeam[SelectingPlayerIndex].Get() : nullptr;
}

void ABattleManager::ChooseOption(int32 OptionIndex)
{
	ABattlePokemon* Pokemon = GetSelectingPokemon();
	if (!IsValid(Pokemon) || Pokemon->GetHealth() == 0)
	{
		return;
	}

	if (!IsSelectingTarget())
	{
		// The first click chooses a move. The next click chooses who receives it.
		if (!Pokemon->GetMoveNames().IsValidIndex(OptionIndex))
		{
			return;
		}
		SelectedMoveIndex = OptionIndex;
	}
	else
	{
		if (!OpponentTeam.IsValidIndex(OptionIndex) || !IsValid(OpponentTeam[OptionIndex])
			|| OpponentTeam[OptionIndex]->GetHealth() == 0)
		{
			return;
		}
		FBattleCommand Command;
		Command.Pokemon = Pokemon;
		Command.MoveIndex = SelectedMoveIndex;
		Command.Target = OpponentTeam[OptionIndex];
		QueuedCommands.Add(Command);
		++SelectingPlayerIndex;
		SelectedMoveIndex = INDEX_NONE;
	}
	OnSelectionChanged.Broadcast();
}

void ABattleManager::GoBack()
{
	if (!CanGoBack())
	{
		return;
	}
	if (IsSelectingTarget())
	{
		SelectedMoveIndex = INDEX_NONE;
	}
	else
	{
		// Undo the previous Pokemon's choice so the player can change it.
		--SelectingPlayerIndex;
		QueuedCommands.Pop();
	}
	OnSelectionChanged.Broadcast();
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
