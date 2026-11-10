#include "UI/BattleCommandsWidget.h"

#include "Battle/BattleManager.h"
#include "Battle/BattlePokemon.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "BattleCommands"

void UBattleCommandsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	OptionOne->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseFirst);
	OptionTwo->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseSecond);
	OptionThree->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseThird);
	OptionFour->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseFourth);
	BackButton->OnClicked.AddDynamic(this, &UBattleCommandsWidget::GoBack);
}

void UBattleCommandsWidget::ShowBattle(ABattleManager* InBattle)
{
	if (Battle.IsValid())
	{
		Battle->OnSelectionChanged.RemoveDynamic(this, &UBattleCommandsWidget::RefreshOptions);
	}
	Battle = InBattle;
	if (Battle.IsValid())
	{
		Battle->OnSelectionChanged.AddDynamic(this, &UBattleCommandsWidget::RefreshOptions);
	}
	RefreshOptions();
}

void UBattleCommandsWidget::RefreshOptions()
{
	if (!Battle.IsValid())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ABattlePokemon* Pokemon = Battle->GetSelectingPokemon();
	TArray<FText> Options;
	TArray<bool> Enabled;
	if (Battle->AreCommandsReady())
	{
		PromptText->SetText(LOCTEXT("Ready", "Commands ready"));
	}
	else if (IsValid(Pokemon))
	{
		if (Battle->IsSelectingTarget())
		{
			PromptText->SetText(FText::Format(LOCTEXT("ChooseTarget", "{0}: choose a target for {1}"),
				Pokemon->GetDisplayName(), Pokemon->GetMoveNames()[Battle->GetSelectedMoveIndex()]));
			for (ABattlePokemon* Target : Battle->GetOpponentTeam())
			{
				Options.Add(IsValid(Target) ? Target->GetDisplayName() : FText::GetEmpty());
				Enabled.Add(IsValid(Target) && Target->GetHealth() > 0);
			}
		}
		else
		{
			PromptText->SetText(FText::Format(LOCTEXT("ChooseMove", "{0}: choose a move"), Pokemon->GetDisplayName()));
			Options = Pokemon->GetMoveNames();
			Enabled.Init(true, Options.Num());
		}
	}

	// The four buttons are reused for either list, so both screens behave the same way.
	const TArray<UButton*> Buttons = { OptionOne, OptionTwo, OptionThree, OptionFour };
	UButton* FirstAvailable = nullptr;
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		UButton* Button = Buttons[Index];
		const bool bHasOption = Options.IsValidIndex(Index);
		Button->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button->SetIsEnabled(bHasOption && Enabled[Index]);
		if (bHasOption)
		{
			CastChecked<UTextBlock>(Button->GetContent())->SetText(Options[Index]);
			if (!FirstAvailable && Enabled[Index])
			{
				FirstAvailable = Button;
			}
		}
	}
	BackButton->SetIsEnabled(Battle->CanGoBack());
	if (FirstAvailable)
	{
		FirstAvailable->SetUserFocus(GetOwningPlayer());
	}
	else if (Battle->CanGoBack())
	{
		BackButton->SetUserFocus(GetOwningPlayer());
	}

	TArray<FText> Choices;
	for (const FBattleCommand& Command : Battle->GetQueuedCommands())
	{
		if (IsValid(Command.Pokemon) && IsValid(Command.Target)
			&& Command.Pokemon->GetMoveNames().IsValidIndex(Command.MoveIndex))
		{
			Choices.Add(FText::Format(LOCTEXT("Selection", "{0}: {1} on {2}"), Command.Pokemon->GetDisplayName(),
				Command.Pokemon->GetMoveNames()[Command.MoveIndex], Command.Target->GetDisplayName()));
		}
	}
	SelectionSummary->SetText(FText::Join(FText::FromString(TEXT("\n")), Choices));
	SelectionSummary->SetVisibility(Choices.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UBattleCommandsWidget::ChooseOption(int32 Index)
{
	if (Battle.IsValid())
	{
		Battle->ChooseOption(Index);
	}
}

void UBattleCommandsWidget::ChooseFirst() { ChooseOption(0); }
void UBattleCommandsWidget::ChooseSecond() { ChooseOption(1); }
void UBattleCommandsWidget::ChooseThird() { ChooseOption(2); }
void UBattleCommandsWidget::ChooseFourth() { ChooseOption(3); }

void UBattleCommandsWidget::GoBack()
{
	if (Battle.IsValid())
	{
		Battle->GoBack();
	}
}

void UBattleCommandsWidget::NativeDestruct()
{
	if (Battle.IsValid())
	{
		Battle->OnSelectionChanged.RemoveDynamic(this, &UBattleCommandsWidget::RefreshOptions);
	}
	Battle.Reset();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
