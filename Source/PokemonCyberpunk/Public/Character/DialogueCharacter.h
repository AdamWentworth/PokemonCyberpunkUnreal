#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interactable/Interactable.h"
#include "DialogueCharacter.generated.h"

class ADialogueCharacter;
class AGamePlayerController;

// Tracks which part of the conversation is currently on screen.
UENUM(BlueprintType)
enum class EDialogueStage : uint8
{
	Closed,
	Introduction,
	Choice,
	Accepted,
	Declined
};

UENUM(BlueprintType)
enum class EDialogueResult : uint8
{
	Completed,
	Accepted,
	Declined,
	Cancelled
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueFinished, ADialogueCharacter*, Speaker, EDialogueResult, Result);

// Shared conversation behavior. Each NPC Blueprint supplies its own name and dialogue text.
UCLASS()
class POKEMONCYBERPUNK_API ADialogueCharacter : public ACharacter, public IInteractable
{
	GENERATED_BODY()

public:
	ADialogueCharacter();
	virtual void Interact(AActor* Interactor, UPrimitiveComponent* HitComponent) override;
	virtual FText GetInteractionPrompt() const override;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginConversation(APawn* Player);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ChooseResponse(bool bYes);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void CancelConversation();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetCurrentLine() const;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetSpeakerName() const
	{
		return DisplayName;
	}

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsChoosingResponse() const
	{
		return DialogueStage == EDialogueStage::Choice;
	}

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	EDialogueStage DialogueStage = EDialogueStage::Closed;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FDialogueFinished OnDialogueFinished;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FText> Introduction;

	// Leave empty for a conversation without a Yes/No question.
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText Question;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FText> YesResponse;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FText> NoResponse;

private:
	const TArray<FText>& GetCurrentPages() const;
	void FinishConversation(EDialogueResult Result);

	// A weak reference becomes invalid if the player disappears; it does not keep the player alive.
	TWeakObjectPtr<AGamePlayerController> TalkingPlayer;
	FRotator RestingRotation;
	int32 PageIndex = 0;
};
