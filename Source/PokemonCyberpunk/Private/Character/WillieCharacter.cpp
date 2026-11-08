#include "Character/WillieCharacter.h"

#include "Core/GamePlayerController.h"

void AWillieCharacter::Interact(AActor* Interactor, UPrimitiveComponent* HitComponent)
{
	BeginConversation(Cast<APawn>(Interactor));
}

FText AWillieCharacter::GetInteractionPrompt() const
{
	return NSLOCTEXT("Willie", "InteractionPrompt", "Talk to Willie");
}

void AWillieCharacter::BeginConversation(APawn* Player)
{
	if (!Player || DialogueStage != EWillieDialogueStage::Closed || Introduction.IsEmpty())
	{
		return;
	}

	AGamePlayerController* PlayerController = Cast<AGamePlayerController>(Player->GetController());
	if (!PlayerController || PlayerController->IsInDialogue()) return;

	TalkingPlayer = PlayerController;
	RestingRotation = GetActorRotation();
	const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	PageIndex = 0;
	DialogueStage = EWillieDialogueStage::Introduction;
	if (!PlayerController->OpenDialogue(this)) FinishConversation(false);
}

const TArray<FText>& AWillieCharacter::GetCurrentPages() const
{
	if (DialogueStage == EWillieDialogueStage::Accepted)
	{
		return AcceptResponse;
	}
	if (DialogueStage == EWillieDialogueStage::Declined)
	{
		return DeclineResponse;
	}
	return Introduction;
}

FText AWillieCharacter::GetCurrentLine() const
{
	if (DialogueStage == EWillieDialogueStage::Closed)
	{
		return FText::GetEmpty();
	}
	if (DialogueStage == EWillieDialogueStage::BattleChoice)
	{
		return BattleInvitation;
	}
	const TArray<FText>& Pages = GetCurrentPages();
	return Pages.IsValidIndex(PageIndex) ? Pages[PageIndex] : FText::GetEmpty();
}

void AWillieCharacter::AdvanceDialogue()
{
	if (!TalkingPlayer.IsValid() || DialogueStage == EWillieDialogueStage::BattleChoice)
	{
		return;
	}
	if (++PageIndex >= GetCurrentPages().Num())
	{
		if (DialogueStage == EWillieDialogueStage::Introduction)
		{
			DialogueStage = EWillieDialogueStage::BattleChoice;
			PageIndex = 0;
		}
		else
		{
			FinishConversation(DialogueStage == EWillieDialogueStage::Accepted);
			return;
		}
	}
	TalkingPlayer->RefreshDialogue();
}

void AWillieCharacter::ChooseBattle(bool bAccept)
{
	if (DialogueStage != EWillieDialogueStage::BattleChoice || !TalkingPlayer.IsValid())
	{
		return;
	}
	DialogueStage = bAccept ? EWillieDialogueStage::Accepted : EWillieDialogueStage::Declined;
	PageIndex = 0;
	TalkingPlayer->RefreshDialogue();
}

void AWillieCharacter::CancelConversation()
{
	FinishConversation(false);
}

void AWillieCharacter::FinishConversation(bool bAccepted)
{
	if (DialogueStage == EWillieDialogueStage::Closed)
	{
		return;
	}
	DialogueStage = EWillieDialogueStage::Closed;
	SetActorRotation(RestingRotation);
	if (TalkingPlayer.IsValid())
	{
		TalkingPlayer->CloseDialogue();
	}
	TalkingPlayer.Reset();
	if (bAccepted)
	{
		OnBattleChallengeAccepted.Broadcast(this);
	}
}

void AWillieCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelConversation();
	Super::EndPlay(EndPlayReason);
}
