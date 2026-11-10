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

private:
	UFUNCTION()
	void RefreshHealth();

	TWeakObjectPtr<ABattlePokemon> Pokemon;
};
