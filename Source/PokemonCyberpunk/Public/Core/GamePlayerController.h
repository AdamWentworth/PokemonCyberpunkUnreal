#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GamePlayerController.generated.h"

class ADialogueCharacter;
class ABattlePokemon;
class ABattleManager;
class UBattleCommandsWidget;
class UBattleHUDWidget;
class UDialogueWidget;
class UInputMappingContext;

// Owns the player's screens and switches input between exploring and using menus.
UCLASS()
class POKEMONCYBERPUNK_API AGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void ShowInteractionPrompt(const FText& Prompt);
	bool OpenDialogue(ADialogueCharacter* Speaker);
	void AdvanceDialogue();
	void RefreshDialogue();
	void CloseDialogue();
	void BeginBattleView(AActor* BattleView);
	void ShowBattleCommands(ABattleManager* Battle);
	void ShowBattleHUD(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
		const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam);

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const
	{
		return bInDialogue;
	}

	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsInBattle() const
	{
		return bInBattle;
	}

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UDialogueWidget> DialogueWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBattleHUDWidget> BattleHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBattleCommandsWidget> BattleCommandsClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDialogueWidget> DialogueUI;

	UPROPERTY(Transient)
	TObjectPtr<UBattleHUDWidget> BattleUI;

	UPROPERTY(Transient)
	TObjectPtr<UBattleCommandsWidget> BattleCommandsUI;

	TWeakObjectPtr<ADialogueCharacter> ActiveSpeaker;
	bool bInDialogue = false;
	bool bInBattle = false;
};
