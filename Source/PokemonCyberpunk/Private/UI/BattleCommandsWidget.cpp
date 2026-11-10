#include "UI/BattleCommandsWidget.h"

#include "Battle/BattleManager.h"
#include "Battle/BattlePokemon.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "BattleCommands"

// Connects the battle menu's buttons to their corresponding functions.
void UBattleCommandsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // AddDynamic registers a function to run when a button is clicked.
    OptionOne->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseFirst);
    OptionTwo->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseSecond);
    OptionThree->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseThird);
    OptionFour->OnClicked.AddDynamic(this, &UBattleCommandsWidget::ChooseFourth);
    BackButton->OnClicked.AddDynamic(this, &UBattleCommandsWidget::GoBack);
}

// Connects this interface to the active battle and listens for selection changes.
void UBattleCommandsWidget::ShowBattle(ABattleManager* InBattle)
{
    // Stop listening to any previously assigned battle.
    if (Battle.IsValid())
    {
        Battle->OnSelectionChanged.RemoveDynamic(this, &UBattleCommandsWidget::RefreshOptions);
    }

    Battle = InBattle;

    // Automatically refresh the menu whenever the battle's selections change.
    if (Battle.IsValid())
    {
        Battle->OnSelectionChanged.AddDynamic(this, &UBattleCommandsWidget::RefreshOptions);
    }
    RefreshOptions();
}

// Updates the available buttons, prompts, and summaries based on the battle's current state.
void UBattleCommandsWidget::RefreshOptions()
{
    // Hide the interface if there is no active battle.
    if (!Battle.IsValid())
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    ABattlePokemon* Pokemon = Battle->GetSelectingPokemon();

    // These arrays hold the button labels and whether each button is usable.
    TArray<FText> Options;
    TArray<bool> Enabled;
    const EBattlePhase Phase = Battle->GetPhase();

    // When the battle ends, display the outcome and a return button.
    if (Phase == EBattlePhase::Finished)
    {
        PromptText->SetText(Battle->HasPlayerWon() ? LOCTEXT("Won", "You won!") : LOCTEXT("Lost", "You lost!"));
        Options.Add(LOCTEXT("Return", "Return to exploration"));
        Enabled.Add(true);
    }
    // After a turn finishes, allow the player to proceed to the next turn.
    else if (Phase == EBattlePhase::ShowingTurnResult)
    {
        PromptText->SetText(LOCTEXT("TurnComplete", "Turn complete"));
        Options.Add(LOCTEXT("NextTurn", "Next turn"));
        Enabled.Add(true);
    }
    // Once every Pokemon has a command, offer a button to execute the turn.
    else if (Battle->AreCommandsReady())
    {
        PromptText->SetText(LOCTEXT("Ready", "Commands ready"));
        Options.Add(LOCTEXT("PlayTurn", "Play turn"));
        Enabled.Add(true);
    }
    else if (IsValid(Pokemon))
    {
        // After selecting a move, display the opponents available as targets.
        if (Battle->IsSelectingTarget())
        {
            PromptText->SetText(FText::Format(LOCTEXT("ChooseTarget", "{0}: choose a target for {1}"),
                Pokemon->GetDisplayName(), Pokemon->GetMoveNames()[Battle->GetSelectedMoveIndex()]));

            for (ABattlePokemon* Target : Battle->GetOpponentTeam())
            {
                Options.Add(IsValid(Target) ? Target->GetDisplayName() : FText::GetEmpty());

                // Fainted or missing Pokemon cannot be targeted.
                Enabled.Add(IsValid(Target) && Target->GetHealth() > 0);
            }
        }
        else
        {
            // Otherwise, display the current Pokemon's available moves.
            PromptText->SetText(FText::Format(LOCTEXT("ChooseMove", "{0}: choose a move"), Pokemon->GetDisplayName()));
            Options = Pokemon->GetMoveNames();
            Enabled.Init(true, Options.Num());
        }
    }

    // Reuse the same four buttons for moves, targets, and turn controls.
    const TArray<UButton*> Buttons = { OptionOne, OptionTwo, OptionThree, OptionFour };
    UButton* FirstAvailable = nullptr;

    for (int32 Index = 0; Index < Buttons.Num(); ++Index)
    {
        UButton* Button = Buttons[Index];
        const bool bHasOption = Options.IsValidIndex(Index);

        // Hide unused buttons and disable choices that aren't allowed.
        Button->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Button->SetIsEnabled(bHasOption && Enabled[Index]);

        if (bHasOption)
        {
            // Replace the button's displayed text with its current option.
            CastChecked<UTextBlock>(Button->GetContent())->SetText(Options[Index]);

            if (!FirstAvailable && Enabled[Index])
            {
                FirstAvailable = Button;
            }
        }
    }

    // Enable Back only when the player has a selection they can undo.
    BackButton->SetIsEnabled(Battle->CanGoBack());

    // Give keyboard/controller focus to the first available button.
    if (FirstAvailable)
    {
        FirstAvailable->SetUserFocus(GetOwningPlayer());
    }
    else if (Battle->CanGoBack())
    {
        BackButton->SetUserFocus(GetOwningPlayer());
    }

    // Build a readable summary of the attacks the player has queued.
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

    // Display the results after a turn, or the planned attacks before it executes.
    const bool bShowingResult = Phase == EBattlePhase::ShowingTurnResult || Phase == EBattlePhase::Finished;
    const FText Summary = bShowingResult ? Battle->GetTurnSummary() : FText::Join(FText::FromString(TEXT("\n")), Choices);
    SelectionSummary->SetText(Summary);
    SelectionSummary->SetVisibility(Summary.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

// Sends the selected button's index to the BattleManager for processing.
void UBattleCommandsWidget::ChooseOption(int32 Index)
{
    if (Battle.IsValid())
    {
        Battle->ChooseOption(Index);
    }
}

// Each button passes its own index into the shared selection function.
void UBattleCommandsWidget::ChooseFirst() { ChooseOption(0); }
void UBattleCommandsWidget::ChooseSecond() { ChooseOption(1); }
void UBattleCommandsWidget::ChooseThird() { ChooseOption(2); }
void UBattleCommandsWidget::ChooseFourth() { ChooseOption(3); }

// Asks the BattleManager to undo the most recent selection.
void UBattleCommandsWidget::GoBack()
{
    if (Battle.IsValid())
    {
        Battle->GoBack();
    }
}

// Cleans up event listeners when this interface is removed or destroyed.
void UBattleCommandsWidget::NativeDestruct()
{
    // Prevent the old battle from trying to update a destroyed interface.
    if (Battle.IsValid())
    {
        Battle->OnSelectionChanged.RemoveDynamic(this, &UBattleCommandsWidget::RefreshOptions);
    }
    Battle.Reset();
    Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
