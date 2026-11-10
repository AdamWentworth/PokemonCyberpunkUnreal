#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleStatusWidget.generated.h"

class ABattlePokemon;
class UProgressBar;
class UTextBlock;

UCLASS(Abstract)
class POKEMONCYBERPUNK_API UBattleStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPokemon(ABattlePokemon* InPokemon);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PokemonName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	FLinearColor HealthyColor = FLinearColor(0.12f, 0.8f, 0.38f);

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	FLinearColor LowHealthColor = FLinearColor(1.0f, 0.65f, 0.12f);

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	FLinearColor CriticalHealthColor = FLinearColor(0.95f, 0.16f, 0.12f);

private:
	UFUNCTION()
	void RefreshHealth();

	TWeakObjectPtr<ABattlePokemon> Pokemon;
};
