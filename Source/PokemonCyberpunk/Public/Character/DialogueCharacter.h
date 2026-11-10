#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interactable/Interactable.h"
#include "DialogueCharacter.generated.h"

class ADialogueCharacter;
class AGamePlayerController;

// Identifies which stage of a conversation is currently active.
UENUM(BlueprintType)
enum class EDialogueStage : uint8
{
    Closed,
    Introduction,
    Choice,
    Accepted,
    Declined
};

// Describes how a conversation ended.
UENUM(BlueprintType)
enum class EDialogueResult : uint8
{
    Completed,
    Accepted,
    Declined,
    Cancelled
};

// Event that announces when dialogue finishes and what the outcome was.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueFinished, ADialogueCharacter*, Speaker, EDialogueResult, Result);

// Base class for NPCs that the player can talk to.
// Implements the interaction system and manages conversation progression.
UCLASS()
class POKEMONCYBERPUNK_API ADialogueCharacter : public ACharacter, public IInteractable
{
    GENERATED_BODY()

public:
    ADialogueCharacter();

    // Functions required by IInteractable so the player can interact with the NPC.
    virtual void Interact(AActor* Interactor, UPrimitiveComponent* HitComponent) override;
    virtual FText GetInteractionPrompt() const override;

    // Starts a conversation with the specified player.
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void BeginConversation(APawn* Player);

    // Advances dialogue or records the player's Yes/No response.
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void AdvanceDialogue();

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void ChooseResponse(bool bYes);

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void CancelConversation();

    // Returns the dialogue line currently being displayed.
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    FText GetCurrentLine() const;

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    FText GetSpeakerName() const
    {
        return DisplayName;
    }

    // Reports whether the NPC is waiting for a Yes/No answer.
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsChoosingResponse() const
    {
        return DialogueStage == EDialogueStage::Choice;
    }

    // Current conversation stage, accessible to Blueprints.
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    EDialogueStage DialogueStage = EDialogueStage::Closed;

    // Other systems can listen for this event to react to dialogue outcomes.
    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FDialogueFinished OnDialogueFinished;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // NPC-specific dialogue can be configured directly in the Unreal Editor.
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    FText DisplayName;

    // Introductory dialogue can contain multiple pages of text.
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    TArray<FText> Introduction;

    // Leave empty for a conversation without a Yes/No question.
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    FText Question;

    // Separate dialogue responses depending on the player's decision.
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    TArray<FText> YesResponse;

    UPROPERTY(EditAnywhere, Category = "Dialogue")
    TArray<FText> NoResponse;

private:
    // Retrieves the active dialogue pages and closes conversations when finished.
    const TArray<FText>& GetCurrentPages() const;
    void FinishConversation(EDialogueResult Result);

    // Weak references do not prevent objects from being destroyed.
    TWeakObjectPtr<AGamePlayerController> TalkingPlayer;

    // Stores the NPC's original orientation and current dialogue page.
    FRotator RestingRotation;
    int32 PageIndex = 0;
};
