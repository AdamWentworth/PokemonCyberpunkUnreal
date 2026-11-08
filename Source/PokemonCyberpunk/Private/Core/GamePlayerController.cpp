#include "Core/GamePlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Character/WesCharacter.h"
#include "Character/DialogueCharacter.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "UI/DialogueWidget.h"

void AGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext) Input->AddMappingContext(DefaultMappingContext, 0);
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
	if (DialogueUI) DialogueUI->ShowPrompt(bInDialogue ? FText::GetEmpty() : Prompt);
}

bool AGamePlayerController::OpenDialogue(ADialogueCharacter* Speaker)
{
	if (bInDialogue || !DialogueUI || !IsValid(Speaker)) return false;
	ActiveSpeaker = Speaker;
	bInDialogue = true;
	if (AWesCharacter* Wes = Cast<AWesCharacter>(GetPawn()))
	{
		Wes->PrepareForDialogue(Speaker);
	}
	DialogueUI->ShowConversation(Speaker);
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	SetShowMouseCursor(true);
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, DialogueUI, EMouseLockMode::DoNotLock, false, true);
	return true;
}

void AGamePlayerController::AdvanceDialogue()
{
	if (ActiveSpeaker.IsValid()) ActiveSpeaker->AdvanceDialogue();
}

void AGamePlayerController::RefreshDialogue()
{
	if (DialogueUI) DialogueUI->RefreshConversation();
}

void AGamePlayerController::CloseDialogue()
{
	if (!bInDialogue) return;
	bInDialogue = false;
	ActiveSpeaker.Reset();
	if (DialogueUI) DialogueUI->HideConversation();
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	SetShowMouseCursor(false);
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(this, true);
}

void AGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveSpeaker.IsValid()) ActiveSpeaker->CancelConversation();
	CloseDialogue();
	if (DialogueUI) DialogueUI->RemoveFromParent();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext) Input->RemoveMappingContext(DefaultMappingContext);
		}
	}
	Super::EndPlay(EndPlayReason);
}
