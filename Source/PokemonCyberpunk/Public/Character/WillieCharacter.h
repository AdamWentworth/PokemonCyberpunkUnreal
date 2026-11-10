#pragma once

#include "CoreMinimal.h"
#include "Character/DialogueCharacter.h"
#include "WillieCharacter.generated.h"

class AWillieCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBattleChallengeAccepted, AWillieCharacter*, Trainer);

// Willie uses the shared dialogue behavior and offers a battle when the player accepts.
UCLASS()
class POKEMONCYBERPUNK_API AWillieCharacter : public ADialogueCharacter
{
	GENERATED_BODY()

public:
	AWillieCharacter();

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FBattleChallengeAccepted OnBattleChallengeAccepted;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleDialogueFinished(ADialogueCharacter* Speaker, EDialogueResult Result);
};
