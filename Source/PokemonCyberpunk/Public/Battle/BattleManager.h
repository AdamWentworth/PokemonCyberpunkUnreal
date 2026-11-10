#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleManager.generated.h"

// Declare classes used by this header without loading their full definitions.
class AWillieCharacter;
class ABattlePokemon;
class UCameraComponent;
class ACharacter;
class USceneComponent;
class AGamePlayerController;

// The four possible stages of a battle.
UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
    Inactive,
    ChoosingCommands,
    ShowingTurnResult,
    Finished
};

// Stores a Pokemon's chosen attack and its intended target.
// The attack is executed later, when the turn is resolved.
USTRUCT(BlueprintType)
struct FBattleCommand
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<ABattlePokemon> Pokemon;

    UPROPERTY(BlueprintReadOnly)
    int32 MoveIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<ABattlePokemon> Target;
};

// Defines an event that other objects can listen to when selections change.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBattleSelectionChanged);

// Coordinates battles, including commands, turn progression, and battle outcomes.
UCLASS()
class POKEMONCYBERPUNK_API ABattleManager : public AActor
{
    GENERATED_BODY()

public:
    ABattleManager();

    // Functions exposed to Unreal Blueprints for handling player selections.
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void ChooseOption(int32 OptionIndex);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void GoBack();

    UFUNCTION(BlueprintPure, Category = "Battle")
    ABattlePokemon* GetSelectingPokemon() const;

    // Helper functions that report the current battle state.
    bool IsSelectingTarget() const { return SelectedMoveIndex != INDEX_NONE; }
    bool AreCommandsReady() const { return Phase == EBattlePhase::ChoosingCommands && SelectingPlayerIndex >= PlayerTeam.Num(); }
    bool CanGoBack() const { return Phase == EBattlePhase::ChoosingCommands && (IsSelectingTarget() || !QueuedCommands.IsEmpty()); }
    int32 GetSelectedMoveIndex() const { return SelectedMoveIndex; }
    const TArray<TObjectPtr<ABattlePokemon>>& GetOpponentTeam() const { return OpponentTeam; }
    EBattlePhase GetPhase() const { return Phase; }
    bool HasPlayerWon() const { return bPlayerWon; }
    const FText& GetTurnSummary() const { return TurnSummary; }

    UFUNCTION(BlueprintPure, Category = "Battle")
    const TArray<FBattleCommand>& GetQueuedCommands() const { return QueuedCommands; }

    // Broadcast whenever the battle menu needs to refresh.
    UPROPERTY(BlueprintAssignable, Category = "Battle")
    FBattleSelectionChanged OnSelectionChanged;

protected:
    virtual void BeginPlay() override;

    // Camera and opponent references configured through Unreal.
    UPROPERTY(VisibleAnywhere, Category = "Battle")
    TObjectPtr<UCameraComponent> BattleCamera;

    UPROPERTY(EditInstanceOnly, Category = "Battle")
    TObjectPtr<AWillieCharacter> Opponent;

    // Team members can be assigned to this battle in the Unreal Editor.
    UPROPERTY(EditInstanceOnly, Category = "Battle")
    TArray<TObjectPtr<ABattlePokemon>> PlayerTeam;

    UPROPERTY(EditInstanceOnly, Category = "Battle")
    TArray<TObjectPtr<ABattlePokemon>> OpponentTeam;

    // Invisible markers identifying where trainers stand during combat.
    UPROPERTY(VisibleAnywhere, Category = "Battle")
    TObjectPtr<USceneComponent> PlayerTrainerPosition;

    UPROPERTY(VisibleAnywhere, Category = "Battle")
    TObjectPtr<USceneComponent> OpponentTrainerPosition;

private:
    // Internal functions responsible for managing the battle.
    void PlaceTrainer(ACharacter* Trainer, const USceneComponent* Position);
    void BeginTurn();
    void SkipFaintedPokemon();
    void ResolveTurn();
    void ApplyAttack(ABattlePokemon* Attacker, ABattlePokemon* Target, int32 MoveIndex, TArray<FText>& Messages);
    void ReturnToExploration();
    ABattlePokemon* FindFirstActivePokemon(const TArray<TObjectPtr<ABattlePokemon>>& Team) const;

    // Called when Willie announces an accepted battle challenge.
    UFUNCTION()
    void StartBattle(AWillieCharacter* Trainer);

    // Stores attacks selected by the player until the turn executes.
    UPROPERTY(Transient)
    TArray<FBattleCommand> QueuedCommands;

    // Tracks the battle's current stage, selections, and outcome.
    EBattlePhase Phase = EBattlePhase::Inactive;
    int32 SelectingPlayerIndex = 0;
    int32 SelectedMoveIndex = INDEX_NONE;
    bool bPlayerWon = false;
    FText TurnSummary;

    // Remember the trainers' original positions and the player's controller.
    FTransform PlayerExplorationTransform;
    FTransform OpponentExplorationTransform;
    TWeakObjectPtr<AGamePlayerController> BattlePlayer;
};
