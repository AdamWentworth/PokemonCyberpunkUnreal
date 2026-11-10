#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleCommandsWidget.generated.h"

class ABattleManager;
class UButton;
class UTextBlock;

// The same buttons show moves first, then targets. The battle manager stores the choices.
UCLASS(Abstract)
class POKEMONCYBERPUNK_API UBattleCommandsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowBattle(ABattleManager* InBattle);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// BindWidget connects these fields to equally named controls in the Widget Blueprint.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectionSummary;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionTwo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionThree;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionFour;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

private:
	UFUNCTION()
	void RefreshOptions();

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

	void ChooseOption(int32 Index);
	TWeakObjectPtr<ABattleManager> Battle;
};
