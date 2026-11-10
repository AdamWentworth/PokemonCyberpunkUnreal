#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleHUDWidget.generated.h"

class ABattlePokemon;
class UBattleStatusWidget;

UCLASS(Abstract)
class POKEMONCYBERPUNK_API UBattleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowTeams(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
		const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> PlayerOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> PlayerTwo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> OpponentOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> OpponentTwo;
};
