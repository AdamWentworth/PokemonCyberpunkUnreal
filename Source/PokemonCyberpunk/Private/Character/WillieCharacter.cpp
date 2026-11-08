#include "Character/WillieCharacter.h"

AWillieCharacter::AWillieCharacter()
{
	DisplayName = NSLOCTEXT("Willie", "DisplayName", "Willie");
}

void AWillieCharacter::BeginPlay()
{
	Super::BeginPlay();
	OnDialogueFinished.AddDynamic(this, &AWillieCharacter::HandleDialogueFinished);
}

void AWillieCharacter::HandleDialogueFinished(ADialogueCharacter* Speaker, EDialogueResult Result)
{
	if (Result == EDialogueResult::Accepted)
	{
		OnBattleChallengeAccepted.Broadcast(this);
	}
}
