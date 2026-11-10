#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleManager.generated.h"

class AWillieCharacter;
class ABattlePokemon;
class UCameraComponent;
class ACharacter;
class USceneComponent;

// A choice for one Pokemon. Selecting a command does not apply its damage yet.
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBattleSelectionChanged);

// Sets up the encounter and keeps the player's choices separate from the UI.
UCLASS()
class POKEMONCYBERPUNK_API ABattleManager : public AActor
{
	GENERATED_BODY()

public:
	ABattleManager();

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void ChooseOption(int32 OptionIndex);

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void GoBack();

	UFUNCTION(BlueprintPure, Category = "Battle")
	ABattlePokemon* GetSelectingPokemon() const;

	bool IsSelectingTarget() const { return SelectedMoveIndex != INDEX_NONE; }
	bool AreCommandsReady() const { return bSelectingCommands && SelectingPlayerIndex >= PlayerTeam.Num(); }
	bool CanGoBack() const { return bSelectingCommands && (IsSelectingTarget() || SelectingPlayerIndex > 0); }
	int32 GetSelectedMoveIndex() const { return SelectedMoveIndex; }
	const TArray<TObjectPtr<ABattlePokemon>>& GetOpponentTeam() const { return OpponentTeam; }

	UFUNCTION(BlueprintPure, Category = "Battle")
	const TArray<FBattleCommand>& GetQueuedCommands() const { return QueuedCommands; }

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FBattleSelectionChanged OnSelectionChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<UCameraComponent> BattleCamera;

	UPROPERTY(EditInstanceOnly, Category = "Battle")
	TObjectPtr<AWillieCharacter> Opponent;

	UPROPERTY(EditInstanceOnly, Category = "Battle")
	TArray<TObjectPtr<ABattlePokemon>> PlayerTeam;

	UPROPERTY(EditInstanceOnly, Category = "Battle")
	TArray<TObjectPtr<ABattlePokemon>> OpponentTeam;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<USceneComponent> PlayerTrainerPosition;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<USceneComponent> OpponentTrainerPosition;

private:
	void PlaceTrainer(ACharacter* Trainer, const USceneComponent* Position);

	UFUNCTION()
	void StartBattle(AWillieCharacter* Trainer);

	UPROPERTY(Transient)
	TArray<FBattleCommand> QueuedCommands;

	bool bSelectingCommands = false;
	int32 SelectingPlayerIndex = 0;
	int32 SelectedMoveIndex = INDEX_NONE;
};
