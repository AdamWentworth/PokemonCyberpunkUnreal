
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

// Sets up the battlefield, camera, and positions for both trainers.
ABattleManager::ABattleManager()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Battlefield"));
    BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
    BattleCamera->SetupAttachment(RootComponent);
    BattleCamera->SetFieldOfView(65.0f);

    // Position trainers on opposite sides of the battlefield, facing each other.
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

// Runs when the game starts and listens for accepted battle challenges.
void ABattleManager::BeginPlay()
{
    Super::BeginPlay();
    if (IsValid(Opponent))
    {
        Opponent->OnBattleChallengeAccepted.AddDynamic(this, &ABattleManager::StartBattle);
    }
}

// Begins combat by switching cameras, positioning trainers, and activating Pokemon.
void ABattleManager::StartBattle(AWillieCharacter* Trainer)
{
    if (!IsValid(Trainer) || Trainer != Opponent)
    {
        return;
    }
    if (AGamePlayerController* PlayerController = Cast<AGamePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
    {
        // Prevent battles from starting during another battle or conversation.
        if (PlayerController->IsInBattle() || PlayerController->IsInDialogue())
        {
            return;
        }
        ACharacter* PlayerTrainer = Cast<ACharacter>(PlayerController->GetPawn());
        if (!IsValid(PlayerTrainer))
        {
            return;
        }

        // Save original positions so both trainers can return after battle.
        BattlePlayer = PlayerController;
        PlayerExplorationTransform = PlayerTrainer->GetActorTransform();
        OpponentExplorationTransform = Trainer->GetActorTransform();
        PlayerController->BeginBattleView(this);
        PlaceTrainer(PlayerTrainer, PlayerTrainerPosition);
        PlaceTrainer(Trainer, OpponentTrainerPosition);

        // Make both teams visible and ready to fight.
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

// Returns the Pokemon currently choosing an attack, or nullptr if none.
ABattlePokemon* ABattleManager::GetSelectingPokemon() const
{
    return Phase == EBattlePhase::ChoosingCommands && PlayerTeam.IsValidIndex(SelectingPlayerIndex)
        ? PlayerTeam[SelectingPlayerIndex].Get() : nullptr;
}

// Processes menu selections based on the current stage of combat.
void ABattleManager::ChooseOption(int32 OptionIndex)
{
    // Button 0 either ends the battle, advances turns, or executes commands.
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

    // First select an attack, then select which opponent it targets.
    if (!IsSelectingTarget())
    {
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

        // Store the chosen attack for execution later in the turn.
        FBattleCommand Command;
        Command.Pokemon = Pokemon;
        Command.MoveIndex = SelectedMoveIndex;
        Command.Target = OpponentTeam[OptionIndex];
        QueuedCommands.Add(Command);
        ++SelectingPlayerIndex;
        SkipFaintedPokemon();
        SelectedMoveIndex = INDEX_NONE;
    }

    // Notify the battle menu to update its available options.
    OnSelectionChanged.Broadcast();
}

// Undoes the latest selection before the turn executes.
void ABattleManager::GoBack()
{
    if (!CanGoBack())
    {
        return;
    }
    if (IsSelectingTarget())
    {
        // Return from target selection to move selection.
        SelectedMoveIndex = INDEX_NONE;
    }
    else
    {
        // Remove the previous Pokemon's command so it can be changed.
        SelectingPlayerIndex = PlayerTeam.IndexOfByKey(QueuedCommands.Last().Pokemon);
        QueuedCommands.Pop();
    }
    OnSelectionChanged.Broadcast();
}

// Finds the first Pokemon on a team that hasn't fainted.
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

// Resets selections and prepares the next round of combat.
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

// Advances past fainted Pokemon that cannot participate in the turn.
void ABattleManager::SkipFaintedPokemon()
{
    while (PlayerTeam.IsValidIndex(SelectingPlayerIndex)
        && (!IsValid(PlayerTeam[SelectingPlayerIndex]) || PlayerTeam[SelectingPlayerIndex]->GetHealth() == 0))
    {
        ++SelectingPlayerIndex;
    }
}

// Executes one attack and records the result for display.
void ABattleManager::ApplyAttack(ABattlePokemon* Attacker, ABattlePokemon* Target, int32 MoveIndex, TArray<FText>& Messages)
{
    // Ignore invalid attacks, missing targets, or fainted attackers.
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

    // TEMPORARY RULE: Every attack hits and causes an instant knockout.
    // Proper damage calculations will replace this later.
    Target->KnockOut();
    Messages.Add(FText::Format(LOCTEXT("AttackKnockout", "{0} used {1}. {2} fainted!"),
        Attacker->GetDisplayName(), Attacker->GetMoveNames()[MoveIndex], Target->GetDisplayName()));
}

// Executes the queued attacks and checks whether either team has won.
void ABattleManager::ResolveTurn()
{
    if (!AreCommandsReady())
    {
        return;
    }
    Phase = EBattlePhase::ShowingTurnResult;
    TArray<FText> Messages;

    // TEMPORARY TURN ORDER: Player attacks first, then surviving opponents.
    // Opponents always use their first move against the first active player Pokemon.
    for (const FBattleCommand& Command : QueuedCommands)
    {
        ApplyAttack(Command.Pokemon, Command.Target, Command.MoveIndex, Messages);
    }
    for (ABattlePokemon* Pokemon : OpponentTeam)
    {
        ApplyAttack(Pokemon, FindFirstActivePokemon(PlayerTeam), 0, Messages);
    }

    // End combat when either side has no Pokemon remaining.
    bPlayerWon = FindFirstActivePokemon(OpponentTeam) == nullptr;
    if (bPlayerWon || FindFirstActivePokemon(PlayerTeam) == nullptr)
    {
        Phase = EBattlePhase::Finished;
    }

    // Combine battle messages into text for the results screen.
    TurnSummary = FText::Join(FText::FromString(TEXT("\n")), Messages);
    OnSelectionChanged.Broadcast();
}

// Ends combat and restores the normal exploration state.
void ABattleManager::ReturnToExploration()
{
    if (Phase != EBattlePhase::Finished)
    {
        return;
    }
    Phase = EBattlePhase::Inactive;
    QueuedCommands.Reset();

    // Hide Pokemon and disable their battle behavior.
    for (ABattlePokemon* Pokemon : PlayerTeam)
    {
        if (IsValid(Pokemon)) Pokemon->LeaveBattle();
    }
    for (ABattlePokemon* Pokemon : OpponentTeam)
    {
        if (IsValid(Pokemon)) Pokemon->LeaveBattle();
    }

    // Teleport the opponent back to their saved location.
    if (IsValid(Opponent))
    {
        Opponent->SetActorTransform(OpponentExplorationTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }

    // Restore the player's position, camera, and exploration controls.
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

// Teleports a trainer to a battlefield marker and stops their movement.
void ABattleManager::PlaceTrainer(ACharacter* Trainer, const USceneComponent* Position)
{
    if (!IsValid(Trainer))
    {
        return;
    }
    Trainer->GetCharacterMovement()->StopMovementImmediately();

    // Raise the character by half its collision capsule height
    // because the battlefield marker is positioned at ground level.
    const FVector CapsuleOffset(0, 0, Trainer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    Trainer->SetActorLocationAndRotation(Position->GetComponentLocation() + CapsuleOffset,
        Position->GetComponentRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}

#undef LOCTEXT_NAMESPACE
