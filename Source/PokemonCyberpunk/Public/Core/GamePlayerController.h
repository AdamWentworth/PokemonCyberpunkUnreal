#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GamePlayerController.generated.h"

// Forward declarations for gameplay classes and interface widgets.
class ADialogueCharacter;
class ABattlePokemon;
class ABattleManager;
class UBattleCommandsWidget;
class UBattleHUDWidget;
class UDialogueWidget;
class UInputMappingContext;

// Controls the player's interface, camera, and input.
// Switches between exploration, conversations, and battles.
UCLASS()
class POKEMONCYBERPUNK_API AGamePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // Displays interaction prompts and manages NPC conversations.
    void ShowInteractionPrompt(const FText& Prompt);
    bool OpenDialogue(ADialogueCharacter* Speaker);
    void AdvanceDialogue();
    void RefreshDialogue();
    void CloseDialogue();

    // Switches the camera and controls into or out of battle mode.
    void BeginBattleView(AActor* BattleView);
    void EndBattleView();

    // Displays the battle command menu and Pokemon health interface.
    void ShowBattleCommands(ABattleManager* Battle);
    void ShowBattleHUD(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
        const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam);

    // Allows Blueprints to check whether dialogue is currently active.
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsInDialogue() const
    {
        return bInDialogue;
    }

    // Allows Blueprints to check whether the player is in battle.
    UFUNCTION(BlueprintPure, Category = "Battle")
    bool IsInBattle() const
    {
        return bInBattle;
    }

protected:
    // Unreal lifecycle functions for startup, input registration, and cleanup.
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Maps keyboard/controller inputs to gameplay actions.
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    // Blueprint classes used to create the various gameplay interfaces.
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UDialogueWidget> DialogueWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UBattleHUDWidget> BattleHUDClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UBattleCommandsWidget> BattleCommandsClass;

private:
    // References to the currently created interface widgets.
    // Transient means these runtime references aren't saved as persistent game data.
    UPROPERTY(Transient)
    TObjectPtr<UDialogueWidget> DialogueUI;

    UPROPERTY(Transient)
    TObjectPtr<UBattleHUDWidget> BattleUI;

    UPROPERTY(Transient)
    TObjectPtr<UBattleCommandsWidget> BattleCommandsUI;

    // Weak references to the active NPC and previous exploration camera.
    TWeakObjectPtr<ADialogueCharacter> ActiveSpeaker;
    TWeakObjectPtr<AActor> ExplorationViewTarget;

    // Stores camera rotation so it can be restored after battle.
    FRotator ExplorationControlRotation;

    // Boolean flags tracking the player's current gameplay state.
    bool bInDialogue = false;
    bool bInBattle = false;
};
