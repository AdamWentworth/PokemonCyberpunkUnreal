#include "Character/DialogueCharacter.h"

#include "Core/GamePlayerController.h"

ADialogueCharacter::ADialogueCharacter()
{
	DisplayName = NSLOCTEXT("Dialogue", "DefaultSpeakerName", "NPC");
}

void ADialogueCharacter::Interact(AActor* Interactor, UPrimitiveComponent* HitComponent)
{
	BeginConversation(Cast<APawn>(Interactor));
}

FText ADialogueCharacter::GetInteractionPrompt() const
{
	return FText::Format(NSLOCTEXT("Dialogue", "TalkToSpeaker", "Talk to {0}"), DisplayName);
}

void ADialogueCharacter::BeginConversation(APawn* Player)
{
	if (!IsValid(Player) || DialogueStage != EDialogueStage::Closed || (Introduction.IsEmpty() && Question.IsEmpty()))
	{
		return;
	}
	AGamePlayerController* PlayerController = Cast<AGamePlayerController>(Player->GetController());
	if (!PlayerController || PlayerController->IsInDialogue())
	{
		return;
	}

	TalkingPlayer = PlayerController;
	RestingRotation = GetActorRotation();
	const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	PageIndex = 0;
	DialogueStage = Introduction.IsEmpty() ? EDialogueStage::Choice : EDialogueStage::Introduction;
	if (!PlayerController->OpenDialogue(this))
	{
		FinishConversation(EDialogueResult::Cancelled);
	}
}

const TArray<FText>& ADialogueCharacter::GetCurrentPages() const
{
	if (DialogueStage == EDialogueStage::Accepted)
	{
		return YesResponse;
	}
	if (DialogueStage == EDialogueStage::Declined)
	{
		return NoResponse;
	}
	return Introduction;
}

FText ADialogueCharacter::GetCurrentLine() const
{
	if (DialogueStage == EDialogueStage::Closed)
	{
		return FText::GetEmpty();
	}
	if (DialogueStage == EDialogueStage::Choice)
	{
		return Question;
	}
	const TArray<FText>& Pages = GetCurrentPages();
	return Pages.IsValidIndex(PageIndex) ? Pages[PageIndex] : FText::GetEmpty();
}

void ADialogueCharacter::AdvanceDialogue()
{
	if (!TalkingPlayer.IsValid() || DialogueStage == EDialogueStage::Closed || IsChoosingResponse())
	{
		return;
	}

	++PageIndex;
	if (PageIndex >= GetCurrentPages().Num())
	{
		if (DialogueStage == EDialogueStage::Introduction)
		{
			if (Question.IsEmpty())
			{
				FinishConversation(EDialogueResult::Completed);
				return;
			}
			DialogueStage = EDialogueStage::Choice;
			PageIndex = 0;
		}
		else
		{
			const EDialogueResult Result = DialogueStage == EDialogueStage::Accepted
				? EDialogueResult::Accepted : EDialogueResult::Declined;
			FinishConversation(Result);
			return;
		}
	}
	TalkingPlayer->RefreshDialogue();
}

void ADialogueCharacter::ChooseResponse(bool bYes)
{
	if (!IsChoosingResponse() || !TalkingPlayer.IsValid())
	{
		return;
	}
	DialogueStage = bYes ? EDialogueStage::Accepted : EDialogueStage::Declined;
	PageIndex = 0;
	if (GetCurrentPages().IsEmpty())
	{
		FinishConversation(bYes ? EDialogueResult::Accepted : EDialogueResult::Declined);
		return;
	}
	TalkingPlayer->RefreshDialogue();
}

void ADialogueCharacter::CancelConversation()
{
	FinishConversation(EDialogueResult::Cancelled);
}

void ADialogueCharacter::FinishConversation(EDialogueResult Result)
{
	if (DialogueStage == EDialogueStage::Closed)
	{
		return;
	}
	DialogueStage = EDialogueStage::Closed;
	SetActorRotation(RestingRotation);
	if (TalkingPlayer.IsValid())
	{
		TalkingPlayer->CloseDialogue();
	}
	TalkingPlayer.Reset();
	OnDialogueFinished.Broadcast(this, Result);
}

void ADialogueCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelConversation();
	Super::EndPlay(EndPlayReason);
}
