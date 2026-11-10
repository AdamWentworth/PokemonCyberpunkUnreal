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

void AGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Input->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AGamePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController() && DialogueWidgetClass)
	{
		DialogueUI = CreateWidget<UDialogueWidget>(this, DialogueWidgetClass);
		DialogueUI->AddToPlayerScreen();
	}
}

void AGamePlayerController::ShowInteractionPrompt(const FText& Prompt)
{
	if (DialogueUI)
	{
		DialogueUI->ShowPrompt(bInDialogue || bInBattle ? FText::GetEmpty() : Prompt);
	}
}

bool AGamePlayerController::OpenDialogue(ADialogueCharacter* Speaker)
{
	if (bInDialogue || bInBattle || !DialogueUI || !IsValid(Speaker))
	{
		return false;
	}
	ActiveSpeaker = Speaker;
	bInDialogue = true;
	if (AWesCharacter* Wes = Cast<AWesCharacter>(GetPawn()))
	{
		Wes->PrepareForDialogue(Speaker);
	}
	DialogueUI->ShowConversation(Speaker);
	// Dialogue takes control of the keys and mouse; Wes stays still until it closes.
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	SetShowMouseCursor(true);
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, DialogueUI, EMouseLockMode::DoNotLock, false, true);
	return true;
}

void AGamePlayerController::AdvanceDialogue()
{
	if (ActiveSpeaker.IsValid())
	{
		ActiveSpeaker->AdvanceDialogue();
	}
}

void AGamePlayerController::RefreshDialogue()
{
	if (DialogueUI)
	{
		DialogueUI->RefreshConversation();
	}
}

void AGamePlayerController::CloseDialogue()
{
	if (!bInDialogue)
	{
		return;
	}
	bInDialogue = false;
	ActiveSpeaker.Reset();
	if (DialogueUI)
	{
		DialogueUI->HideConversation();
	}
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	SetShowMouseCursor(false);
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(this, true);
}

void AGamePlayerController::BeginBattleView(AActor* BattleView)
{
	if (bInDialogue || bInBattle || !IsValid(BattleView))
	{
		return;
	}
	bInBattle = true;
	if (APawn* PlayerPawn = GetPawn())
	{
		PlayerPawn->ConsumeMovementInputVector();
		if (UPawnMovementComponent* Movement = PlayerPawn->GetMovementComponent())
		{
			Movement->StopMovementImmediately();
		}
	}
	ShowInteractionPrompt(FText::GetEmpty());
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	SetViewTarget(BattleView);
}

void AGamePlayerController::ShowBattleHUD(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
	const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam)
{
	if (!bInBattle || !BattleHUDClass)
	{
		return;
	}
	if (!BattleUI)
	{
		BattleUI = CreateWidget<UBattleHUDWidget>(this, BattleHUDClass);
		BattleUI->AddToPlayerScreen();
	}
	BattleUI->ShowTeams(PlayerTeam, OpponentTeam);
}

void AGamePlayerController::ShowBattleCommands(ABattleManager* Battle)
{
	if (!bInBattle || !BattleCommandsClass)
	{
		return;
	}
	if (!BattleCommandsUI)
	{
		BattleCommandsUI = CreateWidget<UBattleCommandsWidget>(this, BattleCommandsClass);
		BattleCommandsUI->AddToPlayerScreen();
	}
	SetShowMouseCursor(true);
	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(this, BattleCommandsUI, EMouseLockMode::DoNotLock, true);
	BattleCommandsUI->ShowBattle(Battle);
}

void AGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveSpeaker.IsValid())
	{
		ActiveSpeaker->CancelConversation();
	}
	CloseDialogue();
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
