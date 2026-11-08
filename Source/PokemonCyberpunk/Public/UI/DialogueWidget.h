#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class ADialogueCharacter;
class UBorder;
class UButton;
class UHorizontalBox;
class UTextBlock;

UCLASS(Abstract)
class POKEMONCYBERPUNK_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowPrompt(const FText& Prompt);
	void ShowConversation(ADialogueCharacter* Character);
	void HideConversation();
	void RefreshConversation();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TalkPrompt;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> DialoguePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeakerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> Choices;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> YesButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NoButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	FLinearColor SelectedColor = FLinearColor(0.08f, 0.5f, 0.7f);

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	FLinearColor UnselectedColor = FLinearColor(0.12f, 0.15f, 0.19f);

private:
	UFUNCTION()
	void ContinueDialogue();

	UFUNCTION()
	void ChooseYes();

	UFUNCTION()
	void ChooseNo();

	void SelectAnswer(bool bYes);

	TWeakObjectPtr<ADialogueCharacter> Speaker;
	bool bYesSelected = true;
};
