#pragma once

#include "CoreMinimal.h"
#include "Character/DialogueCharacter.h"
#include "WillieCharacter.generated.h"

class AWillieCharacter;

// Event broadcast when the player accepts Willie's battle challenge.
// Passes a reference to Willie so the battle system knows who initiated it.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBattleChallengeAccepted, AWillieCharacter*, Trainer);

// Willie inherits conversation functionality from ADialogueCharacter
// and adds the ability to trigger a battle when the player agrees.
UCLASS()
class POKEMONCYBERPUNK_API AWillieCharacter : public ADialogueCharacter
{
	GENERATED_BODY()

public:
	AWillieCharacter();

	// Allows other systems, such as BattleManager, to listen for accepted challenges.
	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FBattleChallengeAccepted OnBattleChallengeAccepted;

protected:
	// Runs when Willie enters the game and registers his dialogue listener.
	virtual void BeginPlay() override;

private:
	// Checks how the conversation ended and broadcasts a challenge if accepted.
	UFUNCTION()
	void HandleDialogueFinished(ADialogueCharacter* Speaker, EDialogueResult Result);
};
