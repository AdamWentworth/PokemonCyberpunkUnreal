#include "UI/DialogueWidget.h"

#include "Character/DialogueCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "Dialogue"

void UDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	YesButton->OnClicked.AddDynamic(this, &UDialogueWidget::ChooseYes);
	NoButton->OnClicked.AddDynamic(this, &UDialogueWidget::ChooseNo);
	ContinueButton->OnClicked.AddDynamic(this, &UDialogueWidget::ContinueDialogue);
	TalkPrompt->SetVisibility(ESlateVisibility::Collapsed);
	DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UDialogueWidget::ShowPrompt(const FText& Prompt)
{
	const bool bVisible = !Prompt.IsEmpty() && !Speaker.IsValid();
	TalkPrompt->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bVisible) PromptText->SetText(FText::Format(LOCTEXT("InteractionPrompt", "E  -  {0}"), Prompt));
}

void UDialogueWidget::ShowConversation(ADialogueCharacter* Character)
{
	Speaker = Character;
	bYesSelected = true;
	TalkPrompt->SetVisibility(ESlateVisibility::Collapsed);
	DialoguePanel->SetVisibility(ESlateVisibility::Visible);
	RefreshConversation();
}

void UDialogueWidget::HideConversation()
{
	Speaker.Reset();
	DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UDialogueWidget::RefreshConversation()
{
	if (!Speaker.IsValid())
	{
		return;
	}
	SpeakerName->SetText(Speaker->GetSpeakerName().ToUpper());
	DialogueText->SetText(Speaker->GetCurrentLine());
	const bool bChoosing = Speaker->IsChoosingResponse();
	Choices->SetVisibility(bChoosing ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ContinueButton->SetVisibility(bChoosing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	ControlsText->SetText(bChoosing
		? LOCTEXT("ChoiceControls", "Left / Right: choose    E / Enter: confirm    Backspace: No")
		: LOCTEXT("PageControls", "E / Enter: continue    Backspace: close"));
	SelectAnswer(bYesSelected);
	SetKeyboardFocus();
}

void UDialogueWidget::SelectAnswer(bool bYes)
{
	bYesSelected = bYes;
	YesButton->SetBackgroundColor(bYes ? SelectedColor : UnselectedColor);
	NoButton->SetBackgroundColor(bYes ? UnselectedColor : SelectedColor);
}

void UDialogueWidget::ContinueDialogue()
{
	if (Speaker.IsValid())
	{
		Speaker->AdvanceDialogue();
	}
}

void UDialogueWidget::ChooseYes()
{
	if (Speaker.IsValid())
	{
		Speaker->ChooseResponse(true);
	}
}

void UDialogueWidget::ChooseNo()
{
	if (Speaker.IsValid())
	{
		Speaker->ChooseResponse(false);
	}
}

FReply UDialogueWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (!Speaker.IsValid())
	{
		return Super::NativeOnKeyDown(Geometry, KeyEvent);
	}
	const FKey Key = KeyEvent.GetKey();
	const bool bChoosing = Speaker->IsChoosingResponse();
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		if (!KeyEvent.IsRepeat())
		{
			if (bChoosing) Speaker->ChooseResponse(bYesSelected);
			else ContinueDialogue();
		}
		return FReply::Handled();
	}
	if (bChoosing && (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Right || Key == EKeys::Down))
	{
		SelectAnswer(Key == EKeys::Left || Key == EKeys::Up);
		return FReply::Handled();
	}
	if (Key == EKeys::BackSpace)
	{
		if (!KeyEvent.IsRepeat())
		{
			if (bChoosing) ChooseNo();
			else Speaker->CancelConversation();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, KeyEvent);
}

#undef LOCTEXT_NAMESPACE
