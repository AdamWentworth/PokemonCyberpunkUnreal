#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleHUDWidget.generated.h"

class ABattlePokemon;
class UBattleStatusWidget;

// Organizes the health/status cards displayed during battle.
// Each card manages its own Pokemon's name and health information.
UCLASS(Abstract)
class POKEMONCYBERPUNK_API UBattleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Assigns Pokemon from both teams to their corresponding status cards.
	// The implementation currently displays up to two Pokemon per team.
	void ShowTeams(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
		const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam);

protected:
	// References to the player's two Pokemon status cards.
	// BindWidget connects them to matching Widget Blueprint elements.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> PlayerOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> PlayerTwo;

	// References to the opponent's two Pokemon status cards.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> OpponentOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBattleStatusWidget> OpponentTwo;
};
