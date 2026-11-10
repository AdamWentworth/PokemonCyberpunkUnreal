#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class ADialogueCharacter;
class UBorder;
class UButton;
class UHorizontalBox;
class UTextBlock;

// Manages the visual interface for NPC conversations.
// The NPC controls the dialogue itself; this class handles its display and inputs.
UCLASS(Abstract)
class POKEMONCYBERPUNK_API UDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Displays an interaction prompt, such as "E - Talk to Willie".
    void ShowPrompt(const FText& Prompt);

    // Opens and closes the conversation panel for an NPC.
    void ShowConversation(ADialogueCharacter* Character);
    void HideConversation();

    // Updates displayed text and buttons to match the current dialogue stage.
    void RefreshConversation();

protected:
    // Initializes buttons and processes keyboard input during conversations.
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

    // BindWidget connects these variables to matching Widget Blueprint elements.
    // The prompt appears when approaching an interactable NPC.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> TalkPrompt;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PromptText;

    // Main dialogue panel containing the conversation interface.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> DialoguePanel;

    // Displays the NPC's name and current dialogue text.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SpeakerName;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DialogueText;

    // Displays instructions explaining which keys the player can press.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ControlsText;

    // Container holding the Yes and No response buttons.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHorizontalBox> Choices;

    // Buttons used to answer questions or advance dialogue.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> YesButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> NoButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ContinueButton;

    // Colors used to distinguish selected and unselected dialogue choices.
    // Both can be customized in the Unreal Editor.
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    FLinearColor SelectedColor = FLinearColor(0.08f, 0.5f, 0.7f);

    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    FLinearColor UnselectedColor = FLinearColor(0.12f, 0.15f, 0.19f);

private:
    // Functions called when the player clicks the dialogue buttons.
    UFUNCTION()
    void ContinueDialogue();

    UFUNCTION()
    void ChooseYes();

    UFUNCTION()
    void ChooseNo();

    // Updates which Yes/No response is visually highlighted.
    void SelectAnswer(bool bYes);

    // Weak reference to the NPC currently speaking.
    TWeakObjectPtr<ADialogueCharacter> Speaker;

    // Tracks the highlighted response, defaulting to Yes.
    bool bYesSelected = true;
};
