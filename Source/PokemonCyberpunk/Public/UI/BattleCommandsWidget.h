#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleCommandsWidget.generated.h"

class ABattleManager;
class UButton;
class UTextBlock;

// Controls the battle menu where players select moves and targets.
// The same four buttons are reused for different battle actions.
UCLASS(Abstract)
class POKEMONCYBERPUNK_API UBattleCommandsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Connects the interface to the active battle manager.
    void ShowBattle(ABattleManager* InBattle);

protected:
    // Called when the widget initializes and when it is destroyed.
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

    // BindWidget connects these variables to UI elements
    // with matching names in the Widget Blueprint.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PromptText;

    // Displays selected attacks or the results of the current turn.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SelectionSummary;

    // Four reusable buttons for moves, targets, and turn controls.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> OptionOne;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> OptionTwo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> OptionThree;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> OptionFour;

    // Allows the player to undo a selection before executing the turn.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BackButton;

private:
    // Updates the menu whenever the battle's current selections change.
    UFUNCTION()
    void RefreshOptions();

    // Each button calls its corresponding function when clicked.
    UFUNCTION()
    void ChooseFirst();

    UFUNCTION()
    void ChooseSecond();

    UFUNCTION()
    void ChooseThird();

    UFUNCTION()
    void ChooseFourth();

    UFUNCTION()
    void GoBack();

    // Forwards the selected button index to the BattleManager.
    void ChooseOption(int32 Index);

    // Weak reference to the active battle manager.
    // Does not prevent the manager from being destroyed.
    TWeakObjectPtr<ABattleManager> Battle;
};
