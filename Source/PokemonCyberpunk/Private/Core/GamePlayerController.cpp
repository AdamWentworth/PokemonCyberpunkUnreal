#include "Core/GamePlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Character/DialogueCharacter.h"
#include "Character/WesCharacter.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UI/BattleHUDWidget.h"
#include "UI/BattleCommandsWidget.h"
#include "UI/DialogueWidget.h"

// Registers the default keyboard/controller input mappings for the player.
void AGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Find the local player's Enhanced Input system.
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            // Activate the project's configured input mappings.
            if (DefaultMappingContext)
            {
                Input->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

// Creates the dialogue interface when the player's game begins.
void AGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Only create the UI for a locally controlled player.
    if (IsLocalController() && DialogueWidgetClass)
    {
        DialogueUI = CreateWidget<UDialogueWidget>(this, DialogueWidgetClass);
        DialogueUI->AddToPlayerScreen();
    }
}

// Displays an interaction prompt unless the player is in dialogue or battle.
void AGamePlayerController::ShowInteractionPrompt(const FText& Prompt)
{
    if (DialogueUI)
    {
        DialogueUI->ShowPrompt(bInDialogue || bInBattle ? FText::GetEmpty() : Prompt);
    }
}

// Starts a conversation and changes the player's controls to dialogue mode.
bool AGamePlayerController::OpenDialogue(ADialogueCharacter* Speaker)
{
    // Prevent overlapping conversations, battles, or invalid speakers.
    if (bInDialogue || bInBattle || !DialogueUI || !IsValid(Speaker))
    {
        return false;
    }

    ActiveSpeaker = Speaker;
    bInDialogue = true;

    // Stop Wes and turn him toward the person speaking.
    if (AWesCharacter* Wes = Cast<AWesCharacter>(GetPawn()))
    {
        Wes->PrepareForDialogue(Speaker);
    }

    DialogueUI->ShowConversation(Speaker);

    // Disable movement and camera controls while allowing UI interaction.
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetShowMouseCursor(true);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, DialogueUI, EMouseLockMode::DoNotLock, false, true);

    return true;
}

// Tells the current NPC to advance to the next dialogue line.
void AGamePlayerController::AdvanceDialogue()
{
    if (ActiveSpeaker.IsValid())
    {
        ActiveSpeaker->AdvanceDialogue();
    }
}

// Refreshes the dialogue interface after the conversation changes.
void AGamePlayerController::RefreshDialogue()
{
    if (DialogueUI)
    {
        DialogueUI->RefreshConversation();
    }
}

// Ends dialogue and restores normal exploration controls.
void AGamePlayerController::CloseDialogue()
{
    if (!bInDialogue)
    {
        return;
    }

    bInDialogue = false;
    ActiveSpeaker.Reset();

    // Hide the conversation interface.
    if (DialogueUI)
    {
        DialogueUI->HideConversation();
    }

    // Restore movement, camera control, and normal gameplay input.
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    SetShowMouseCursor(false);
    UWidgetBlueprintLibrary::SetInputMode_GameOnly(this, true);
}

// Switches from exploration into the battle camera and control mode.
void AGamePlayerController::BeginBattleView(AActor* BattleView)
{
    if (bInDialogue || bInBattle || !IsValid(BattleView))
    {
        return;
    }

    // Remember the exploration camera and its rotation for later restoration.
    ExplorationViewTarget = GetViewTarget();
    ExplorationControlRotation = GetControlRotation();
    bInBattle = true;

    // Stop any movement still being applied to the player's character.
    if (APawn* PlayerPawn = GetPawn())
    {
        PlayerPawn->ConsumeMovementInputVector();
        if (UPawnMovementComponent* Movement = PlayerPawn->GetMovementComponent())
        {
            Movement->StopMovementImmediately();
        }
    }

    // Disable exploration controls and switch to the battle camera.
    ShowInteractionPrompt(FText::GetEmpty());
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetViewTarget(BattleView);
}

// Ends the battle view and returns control to the player.
void AGamePlayerController::EndBattleView()
{
    if (!bInBattle)
    {
        return;
    }

    bInBattle = false;

    // Remove battle menus so a future encounter can create fresh ones.
    if (BattleCommandsUI)
    {
        BattleCommandsUI->RemoveFromParent();
        BattleCommandsUI = nullptr;
    }
    if (BattleUI)
    {
        BattleUI->RemoveFromParent();
        BattleUI = nullptr;
    }

    // Restore the previous camera, or use the player character as a fallback.
    SetViewTarget(ExplorationViewTarget.IsValid() ? ExplorationViewTarget.Get() : GetPawn());
    SetControlRotation(ExplorationControlRotation);
    ExplorationViewTarget.Reset();

    // Re-enable normal movement, camera, and gameplay input.
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    SetShowMouseCursor(false);
    UWidgetBlueprintLibrary::SetInputMode_GameOnly(this, true);
}

// Creates the battle HUD and displays both teams' Pokemon information.
void AGamePlayerController::ShowBattleHUD(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
    const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam)
{
    // Only show the HUD if a battle is active and its UI class is configured.
    if (!bInBattle || !BattleHUDClass)
    {
        return;
    }

    // Create the HUD only once per battle.
    if (!BattleUI)
    {
        BattleUI = CreateWidget<UBattleHUDWidget>(this, BattleHUDClass);
        BattleUI->AddToPlayerScreen();
    }

    // Pass both teams to the HUD so it can display their health and names.
    BattleUI->ShowTeams(PlayerTeam, OpponentTeam);
}

// Opens the battle command menu where the player chooses moves and targets.
void AGamePlayerController::ShowBattleCommands(ABattleManager* Battle)
{
    if (!bInBattle || !BattleCommandsClass)
    {
        return;
    }

    // Create the command interface if it doesn't already exist.
    if (!BattleCommandsUI)
    {
        BattleCommandsUI = CreateWidget<UBattleCommandsWidget>(this, BattleCommandsClass);
        BattleCommandsUI->AddToPlayerScreen();
    }

    // Give the mouse and keyboard to the battle menu instead of gameplay.
    SetShowMouseCursor(true);
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(this, BattleCommandsUI, EMouseLockMode::DoNotLock, true);

    // Connect the menu to the active BattleManager.
    BattleCommandsUI->ShowBattle(Battle);
}

// Cleans up interfaces, conversations, and input mappings when the controller is destroyed.
void AGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any conversation that is still running.
    if (ActiveSpeaker.IsValid())
    {
        ActiveSpeaker->CancelConversation();
    }
    CloseDialogue();

    // Remove UI elements to prevent leftover screens and references.
    if (DialogueUI)
    {
        DialogueUI->RemoveFromParent();
    }
    if (BattleUI)
    {
        BattleUI->RemoveFromParent();
    }
    if (BattleCommandsUI)
    {
        BattleCommandsUI->RemoveFromParent();
    }

    // Unregister the input mappings added when this controller started.
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
            {
                Input->RemoveMappingContext(DefaultMappingContext);
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}
