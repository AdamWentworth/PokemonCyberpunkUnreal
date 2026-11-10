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

#define LOCTEXT_NAMESPACE "Battle"

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
		ACharacter* PlayerTrainer = Cast<ACharacter>(PlayerController->GetPawn());
		if (!IsValid(PlayerTrainer))
		{
			return;
		}
		// Remember where both trainers were standing so the battle can return them there.
		BattlePlayer = PlayerController;
		PlayerExplorationTransform = PlayerTrainer->GetActorTransform();
		OpponentExplorationTransform = Trainer->GetActorTransform();
		PlayerController->BeginBattleView(this);
		PlaceTrainer(PlayerTrainer, PlayerTrainerPosition);
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
		bPlayerWon = false;
		BeginTurn();
		PlayerController->ShowBattleCommands(this);
	}
}

ABattlePokemon* ABattleManager::GetSelectingPokemon() const
{
	return Phase == EBattlePhase::ChoosingCommands && PlayerTeam.IsValidIndex(SelectingPlayerIndex)
		? PlayerTeam[SelectingPlayerIndex].Get() : nullptr;
}

void ABattleManager::ChooseOption(int32 OptionIndex)
{
	// After a turn, the first button continues the battle or returns to the world.
	if (Phase == EBattlePhase::Finished)
	{
		if (OptionIndex == 0) ReturnToExploration();
		return;
	}
	if (Phase == EBattlePhase::ShowingTurnResult)
	{
		if (OptionIndex == 0) BeginTurn();
		return;
	}
	if (AreCommandsReady())
	{
		if (OptionIndex == 0) ResolveTurn();
		return;
	}
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
		SkipFaintedPokemon();
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
		SelectingPlayerIndex = PlayerTeam.IndexOfByKey(QueuedCommands.Last().Pokemon);
		QueuedCommands.Pop();
	}
	OnSelectionChanged.Broadcast();
}

ABattlePokemon* ABattleManager::FindFirstActivePokemon(const TArray<TObjectPtr<ABattlePokemon>>& Team) const
{
	for (ABattlePokemon* Pokemon : Team)
	{
		if (IsValid(Pokemon) && Pokemon->GetHealth() > 0)
		{
			return Pokemon;
		}
	}
	return nullptr;
}

void ABattleManager::BeginTurn()
{
	QueuedCommands.Reset();
	SelectedMoveIndex = INDEX_NONE;
	SelectingPlayerIndex = 0;
	TurnSummary = FText::GetEmpty();
	Phase = EBattlePhase::ChoosingCommands;
	SkipFaintedPokemon();
	OnSelectionChanged.Broadcast();
}

void ABattleManager::SkipFaintedPokemon()
{
	// Only Pokemon still able to battle need a command this turn.
	while (PlayerTeam.IsValidIndex(SelectingPlayerIndex)
		&& (!IsValid(PlayerTeam[SelectingPlayerIndex]) || PlayerTeam[SelectingPlayerIndex]->GetHealth() == 0))
	{
		++SelectingPlayerIndex;
	}
}

void ABattleManager::ApplyAttack(ABattlePokemon* Attacker, ABattlePokemon* Target, int32 MoveIndex, TArray<FText>& Messages)
{
	if (!IsValid(Attacker) || Attacker->GetHealth() == 0 || !IsValid(Target)
		|| !Attacker->GetMoveNames().IsValidIndex(MoveIndex))
	{
		return;
	}
	if (Target->GetHealth() == 0)
	{
		Messages.Add(FText::Format(LOCTEXT("TargetFainted", "{0}'s target has already fainted."), Attacker->GetDisplayName()));
		return;
	}

	// TEMPORARY MILESTONE 1 RULE: every attack hits and knocks its target out in one hit.
	// This lets us test winning, losing and returning to exploration. Replace with proper damage rules later.
	Target->KnockOut();
	Messages.Add(FText::Format(LOCTEXT("AttackKnockout", "{0} used {1}. {2} fainted!"),
		Attacker->GetDisplayName(), Attacker->GetMoveNames()[MoveIndex], Target->GetDisplayName()));
}

void ABattleManager::ResolveTurn()
{
	if (!AreCommandsReady())
	{
		return;
	}
	Phase = EBattlePhase::ShowingTurnResult;
	TArray<FText> Messages;

	// TEMPORARY TURN ORDER: the player's choices go first, then any surviving opponents reply.
	// Speed-based ordering and smarter opponent choices can replace this after the basic loop works.
	for (const FBattleCommand& Command : QueuedCommands)
	{
		ApplyAttack(Command.Pokemon, Command.Target, Command.MoveIndex, Messages);
	}
	for (ABattlePokemon* Pokemon : OpponentTeam)
	{
		ApplyAttack(Pokemon, FindFirstActivePokemon(PlayerTeam), 0, Messages);
	}

	bPlayerWon = FindFirstActivePokemon(OpponentTeam) == nullptr;
	if (bPlayerWon || FindFirstActivePokemon(PlayerTeam) == nullptr)
	{
		Phase = EBattlePhase::Finished;
	}
	TurnSummary = FText::Join(FText::FromString(TEXT("\n")), Messages);
	OnSelectionChanged.Broadcast();
}

void ABattleManager::ReturnToExploration()
{
	if (Phase != EBattlePhase::Finished)
	{
		return;
	}
	Phase = EBattlePhase::Inactive;
	QueuedCommands.Reset();
	for (ABattlePokemon* Pokemon : PlayerTeam)
	{
		if (IsValid(Pokemon)) Pokemon->LeaveBattle();
	}
	for (ABattlePokemon* Pokemon : OpponentTeam)
	{
		if (IsValid(Pokemon)) Pokemon->LeaveBattle();
	}
	if (IsValid(Opponent))
	{
		Opponent->SetActorTransform(OpponentExplorationTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (BattlePlayer.IsValid())
	{
		if (APawn* Player = BattlePlayer->GetPawn())
		{
			Player->SetActorTransform(PlayerExplorationTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
		BattlePlayer->EndBattleView();
	}
	BattlePlayer.Reset();
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

#undef LOCTEXT_NAMESPACE
