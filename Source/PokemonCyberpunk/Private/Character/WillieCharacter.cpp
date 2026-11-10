#include "Character/WillieCharacter.h"

// Sets Willie's name, which will appear in dialogue and other UI.
AWillieCharacter::AWillieCharacter()
{
	DisplayName = NSLOCTEXT("Willie", "DisplayName", "Willie");
}

// Runs when Willie enters the game and connects his dialogue to the battle system.
void AWillieCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the conversation-ending event.
	// When dialogue finishes, HandleDialogueFinished will run automatically.
	OnDialogueFinished.AddDynamic(this, &AWillieCharacter::HandleDialogueFinished);
}

// Checks the conversation's outcome and starts a battle if the player agreed.
void AWillieCharacter::HandleDialogueFinished(ADialogueCharacter* Speaker, EDialogueResult Result)
{
	// Only an accepted response triggers the battle.
	// Declining or cancelling the conversation does nothing.
	if (Result == EDialogueResult::Accepted)
	{
		// Notify the BattleManager that Willie has issued an accepted challenge.
		// The BattleManager listens for this event and handles starting combat.
		OnBattleChallengeAccepted.Broadcast(this);
	}
}
