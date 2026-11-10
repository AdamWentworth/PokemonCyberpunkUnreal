#include "UI/BattleHUDWidget.h"

#include "Battle/BattlePokemon.h"
#include "UI/BattleStatusWidget.h"

// Assigns Pokemon from both teams to their corresponding HUD status cards.
void UBattleHUDWidget::ShowTeams(const TArray<TObjectPtr<ABattlePokemon>>& PlayerTeam,
	const TArray<TObjectPtr<ABattlePokemon>>& OpponentTeam)
{
	// Display the first two Pokemon belonging to the player.
	// If a team position is empty, pass nullptr so its status card can be hidden.
	PlayerOne->SetPokemon(PlayerTeam.IsValidIndex(0) ? PlayerTeam[0].Get() : nullptr);
	PlayerTwo->SetPokemon(PlayerTeam.IsValidIndex(1) ? PlayerTeam[1].Get() : nullptr);

	// Do the same for the opponent's first two Pokemon.
	OpponentOne->SetPokemon(OpponentTeam.IsValidIndex(0) ? OpponentTeam[0].Get() : nullptr);
	OpponentTwo->SetPokemon(OpponentTeam.IsValidIndex(1) ? OpponentTeam[1].Get() : nullptr);

	// Show the HUD without allowing it to intercept mouse interactions.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
