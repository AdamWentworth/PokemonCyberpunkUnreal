#include "UI/BattleHUDWidget.h"

#include "Battle/BattlePokemon.h"
#include "UI/BattleStatusWidget.h"

void UBattleHUDWidget::ShowTeams(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
	const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam)
{
	PlayerOne->SetPokemon(PlayerTeam.IsValidIndex(0) ? PlayerTeam[0].Get() : nullptr);
	PlayerTwo->SetPokemon(PlayerTeam.IsValidIndex(1) ? PlayerTeam[1].Get() : nullptr);
	OpponentOne->SetPokemon(OpponentTeam.IsValidIndex(0) ? OpponentTeam[0].Get() : nullptr);
	OpponentTwo->SetPokemon(OpponentTeam.IsValidIndex(1) ? OpponentTeam[1].Get() : nullptr);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
