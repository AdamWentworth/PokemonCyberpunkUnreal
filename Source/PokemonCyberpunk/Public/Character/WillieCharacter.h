#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interactable/Interactable.h"
#include "WillieCharacter.generated.h"

class AGamePlayerController;
class AWillieCharacter;

UENUM(BlueprintType)
enum class EWillieDialogueStage : uint8
{
	Closed,
	Introduction,
	BattleChoice,
	Accepted,
	Declined
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBattleChallengeAccepted, AWillieCharacter*, Trainer);

UCLASS()
class POKEMONCYBERPUNK_API AWillieCharacter : public ACharacter, public IInteractable
{
	GENERATED_BODY()

public:
	virtual void Interact(AActor* Interactor, UPrimitiveComponent* HitComponent) override;
	virtual FText GetInteractionPrompt() const override;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginConversation(APawn* Player);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ChooseBattle(bool bAccept);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void CancelConversation();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetCurrentLine() const;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	EWillieDialogueStage DialogueStage = EWillieDialogueStage::Closed;

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FBattleChallengeAccepted OnBattleChallengeAccepted;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TArray<FText> Introduction;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FText BattleInvitation;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TArray<FText> AcceptResponse;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TArray<FText> DeclineResponse;

private:
	const TArray<FText>& GetCurrentPages() const;
	void FinishConversation(bool bAccepted);

	TWeakObjectPtr<AGamePlayerController> TalkingPlayer;
	FRotator RestingRotation;
	int32 PageIndex = 0;
};
