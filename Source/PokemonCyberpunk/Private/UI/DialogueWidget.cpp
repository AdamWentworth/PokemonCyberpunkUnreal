#include "UI/DialogueWidget.h"

#include "Character/DialogueCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "Dialogue"

// Initializes the dialogue interface and connects buttons to their functions.
void UDialogueWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Register functions that execute when the player clicks each button.
    YesButton->OnClicked.AddDynamic(this, &UDialogueWidget::ChooseYes);
    NoButton->OnClicked.AddDynamic(this, &UDialogueWidget::ChooseNo);
    ContinueButton->OnClicked.AddDynamic(this, &UDialogueWidget::ContinueDialogue);

    // Keep dialogue and interaction prompts hidden until needed.
    TalkPrompt->SetVisibility(ESlateVisibility::Collapsed);
    DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
}

// Displays an interaction prompt, such as "E - Talk to Willie".
void UDialogueWidget::ShowPrompt(const FText& Prompt)
{
    // Only show prompts when text exists and no conversation is active.
    const bool bVisible = !Prompt.IsEmpty() && !Speaker.IsValid();
    TalkPrompt->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

    if (bVisible)
    {
        PromptText->SetText(FText::Format(LOCTEXT("InteractionPrompt", "E  -  {0}"), Prompt));
    }
}

// Opens the dialogue panel and connects it to the selected NPC.
void UDialogueWidget::ShowConversation(ADialogueCharacter* Character)
{
    Speaker = Character;

    // Default the Yes/No selection to Yes.
    bYesSelected = true;

    // Replace the interaction prompt with the conversation interface.
    TalkPrompt->SetVisibility(ESlateVisibility::Collapsed);
    DialoguePanel->SetVisibility(ESlateVisibility::Visible);
    RefreshConversation();
}

// Closes the conversation interface and clears the NPC reference.
void UDialogueWidget::HideConversation()
{
    Speaker.Reset();
    DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
}

// Updates the NPC's name, dialogue text, and available controls.
void UDialogueWidget::RefreshConversation()
{
    if (!Speaker.IsValid())
    {
        return;
    }

    // Retrieve the current speaker's name and dialogue line.
    SpeakerName->SetText(Speaker->GetSpeakerName().ToUpper());
    DialogueText->SetText(Speaker->GetCurrentLine());

    // Show Yes/No buttons when the NPC is asking a question.
    // Otherwise, show the Continue button.
    const bool bChoosing = Speaker->IsChoosingResponse();
    Choices->SetVisibility(bChoosing ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ContinueButton->SetVisibility(bChoosing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

    // Display keyboard instructions appropriate to the current dialogue stage.
    ControlsText->SetText(bChoosing
        ? LOCTEXT("ChoiceControls", "Left / Right: choose    E / Enter: confirm    Backspace: No")
        : LOCTEXT("PageControls", "E / Enter: continue    Backspace: close"));

    // Update the highlighted Yes/No button and focus the interface.
    SelectAnswer(bYesSelected);
    SetKeyboardFocus();
}

// Highlights the currently selected Yes or No response.
void UDialogueWidget::SelectAnswer(bool bYes)
{
    bYesSelected = bYes;

    // Apply different background colors to selected and unselected buttons.
    YesButton->SetBackgroundColor(bYes ? SelectedColor : UnselectedColor);
    NoButton->SetBackgroundColor(bYes ? UnselectedColor : SelectedColor);
}

// Tells the NPC to advance to the next dialogue line.
void UDialogueWidget::ContinueDialogue()
{
    if (Speaker.IsValid())
    {
        Speaker->AdvanceDialogue();
    }
}

// Sends a Yes response to the NPC's dialogue system.
void UDialogueWidget::ChooseYes()
{
    if (Speaker.IsValid())
    {
        Speaker->ChooseResponse(true);
    }
}

// Sends a No response to the NPC's dialogue system.
void UDialogueWidget::ChooseNo()
{
    if (Speaker.IsValid())
    {
        Speaker->ChooseResponse(false);
    }
}

// Handles keyboard input while the dialogue interface is active.
FReply UDialogueWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    // Use Unreal's default keyboard handling if no NPC is assigned.
    if (!Speaker.IsValid())
    {
        return Super::NativeOnKeyDown(Geometry, KeyEvent);
    }

    // Identify the pressed key and whether the player is choosing Yes or No.
    const FKey Key = KeyEvent.GetKey();
    const bool bChoosing = Speaker->IsChoosingResponse();

    // E, Enter, and Space either confirm an answer or advance dialogue.
    if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar)
    {
        // Ignore repeated key events to prevent skipping several dialogue pages.
        if (!KeyEvent.IsRepeat())
        {
            if (bChoosing)
            {
                Speaker->ChooseResponse(bYesSelected);
            }
            else
            {
                ContinueDialogue();
            }
        }

        // Tell Unreal this key press has already been handled.
        return FReply::Handled();
    }

    // Arrow keys change which Yes/No response is highlighted.
    if (bChoosing && (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Right || Key == EKeys::Down))
    {
        // Left/Up select Yes, while Right/Down select No.
        SelectAnswer(Key == EKeys::Left || Key == EKeys::Up);
        return FReply::Handled();
    }

    // Backspace declines a question or cancels ordinary dialogue.
    if (Key == EKeys::BackSpace)
    {
        if (!KeyEvent.IsRepeat())
        {
            if (bChoosing)
            {
                ChooseNo();
            }
            else
            {
                Speaker->CancelConversation();
            }
        }
        return FReply::Handled();
    }

    // Allow Unreal to process any other keys normally.
    return Super::NativeOnKeyDown(Geometry, KeyEvent);
}

#undef LOCTEXT_NAMESPACE
