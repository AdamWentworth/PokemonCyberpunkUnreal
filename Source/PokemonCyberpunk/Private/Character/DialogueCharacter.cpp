#include "Character/DialogueCharacter.h"

#include "Core/GamePlayerController.h"

// Sets the default name for NPCs that don't have a custom name.
ADialogueCharacter::ADialogueCharacter()
{
    DisplayName = NSLOCTEXT("Dialogue", "DefaultSpeakerName", "NPC");
}

// Starts a conversation when the player interacts with this NPC.
void ADialogueCharacter::Interact(AActor* Interactor, UPrimitiveComponent* HitComponent)
{
    BeginConversation(Cast<APawn>(Interactor));
}

// Returns the interaction prompt, such as "Talk to Willie".
FText ADialogueCharacter::GetInteractionPrompt() const
{
    return FText::Format(NSLOCTEXT("Dialogue", "TalkToSpeaker", "Talk to {0}"), DisplayName);
}

// Opens a conversation, faces the NPC toward the player, and displays dialogue.
void ADialogueCharacter::BeginConversation(APawn* Player)
{
    // Prevent conversations with invalid players, already-active NPCs, or NPCs without dialogue.
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

    // Remember the NPC's original rotation so it can face that direction afterward.
    RestingRotation = GetActorRotation();
    const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));

    // Start at the first dialogue page, or jump to the question if no introduction exists.
    PageIndex = 0;
    DialogueStage = Introduction.IsEmpty() ? EDialogueStage::Choice : EDialogueStage::Introduction;

    // Cancel if the player's controller cannot open the dialogue interface.
    if (!PlayerController->OpenDialogue(this))
    {
        FinishConversation(EDialogueResult::Cancelled);
    }
}

// Returns the appropriate dialogue pages based on the player's response.
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

// Retrieves the current line of dialogue to display on screen.
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

    // Return the current page, or empty text if the page doesn't exist.
    const TArray<FText>& Pages = GetCurrentPages();
    return Pages.IsValidIndex(PageIndex) ? Pages[PageIndex] : FText::GetEmpty();
}

// Advances to the next dialogue page or finishes the conversation.
void ADialogueCharacter::AdvanceDialogue()
{
    // Don't advance if the conversation is inactive or waiting for a Yes/No answer.
    if (!TalkingPlayer.IsValid() || DialogueStage == EDialogueStage::Closed || IsChoosingResponse())
    {
        return;
    }

    ++PageIndex;

    // Check whether we've reached the end of the current dialogue pages.
    if (PageIndex >= GetCurrentPages().Num())
    {
        if (DialogueStage == EDialogueStage::Introduction)
        {
            // End normally if there is no question after the introduction.
            if (Question.IsEmpty())
            {
                FinishConversation(EDialogueResult::Completed);
                return;
            }

            // Otherwise, display the Yes/No question.
            DialogueStage = EDialogueStage::Choice;
            PageIndex = 0;
        }
        else
        {
            // Finish after displaying the response to the player's choice.
            const EDialogueResult Result = DialogueStage == EDialogueStage::Accepted
                ? EDialogueResult::Accepted : EDialogueResult::Declined;
            FinishConversation(Result);
            return;
        }
    }

    // Tell the player's dialogue interface to display the updated text.
    TalkingPlayer->RefreshDialogue();
}

// Handles the player's Yes or No selection.
void ADialogueCharacter::ChooseResponse(bool bYes)
{
    if (!IsChoosingResponse() || !TalkingPlayer.IsValid())
    {
        return;
    }

    // Switch to the appropriate response and begin at its first page.
    DialogueStage = bYes ? EDialogueStage::Accepted : EDialogueStage::Declined;
    PageIndex = 0;

    // Finish immediately if there are no response pages to display.
    if (GetCurrentPages().IsEmpty())
    {
        FinishConversation(bYes ? EDialogueResult::Accepted : EDialogueResult::Declined);
        return;
    }

    TalkingPlayer->RefreshDialogue();
}

// Cancels the current conversation.
void ADialogueCharacter::CancelConversation()
{
    FinishConversation(EDialogueResult::Cancelled);
}

// Closes dialogue, restores the NPC, and announces the conversation's result.
void ADialogueCharacter::FinishConversation(EDialogueResult Result)
{
    if (DialogueStage == EDialogueStage::Closed)
    {
        return;
    }

    DialogueStage = EDialogueStage::Closed;

    // Return the NPC to its original facing direction.
    SetActorRotation(RestingRotation);

    // Close the player's dialogue interface and clear the reference.
    if (TalkingPlayer.IsValid())
    {
        TalkingPlayer->CloseDialogue();
    }
    TalkingPlayer.Reset();

    // Notify other systems of the result only after dialogue closes.
    // For example, Willie can use an accepted response to initiate a battle.
    OnDialogueFinished.Broadcast(this, Result);
}

// Cleans up any active conversation when the NPC leaves the game.
void ADialogueCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelConversation();
    Super::EndPlay(EndPlayReason);
}
