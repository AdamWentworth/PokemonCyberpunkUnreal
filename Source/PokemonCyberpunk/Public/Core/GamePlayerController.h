#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GamePlayerController.generated.h"

class ADialogueCharacter;
class UDialogueWidget;
class UInputMappingContext;

UCLASS()
class POKEMONCYBERPUNK_API AGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void ShowInteractionPrompt(const FText& Prompt);
	bool OpenDialogue(ADialogueCharacter* Speaker);
	void AdvanceDialogue();
	void RefreshDialogue();
	void CloseDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const { return bInDialogue; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UDialogueWidget> DialogueWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDialogueWidget> DialogueUI;

	TWeakObjectPtr<ADialogueCharacter> ActiveSpeaker;
	bool bInDialogue = false;
};
