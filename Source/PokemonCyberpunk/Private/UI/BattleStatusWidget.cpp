#include "UI/BattleStatusWidget.h"

#include "Battle/BattlePokemon.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBattleStatusWidget::SetPokemon(ABattlePokemon* InPokemon)
{
	// Stop listening to the previous Pokemon before showing a different one.
	if (Pokemon.IsValid())
	{
		Pokemon->OnHealthChanged.RemoveDynamic(this, &UBattleStatusWidget::RefreshHealth);
	}
	Pokemon = InPokemon;
	if (Pokemon.IsValid())
	{
		Pokemon->OnHealthChanged.AddDynamic(this, &UBattleStatusWidget::RefreshHealth);
	}
	RefreshHealth();
}

void UBattleStatusWidget::RefreshHealth()
{
	// Read the Pokemon's actual health; the UI does not keep a separate health total.
	SetVisibility(Pokemon.IsValid() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Pokemon.IsValid())
	{
		return;
	}
	PokemonName->SetText(Pokemon->GetDisplayName().ToUpper());
	HealthText->SetText(FText::Format(NSLOCTEXT("Battle", "Health", "HP  {0} / {1}"),
		FText::AsNumber(Pokemon->GetHealth()), FText::AsNumber(Pokemon->GetMaxHealth())));

	const float HealthFraction = static_cast<float>(Pokemon->GetHealth()) / Pokemon->GetMaxHealth();
	HealthBar->SetPercent(HealthFraction);
	if (HealthFraction > 0.5f)
	{
		HealthBar->SetFillColorAndOpacity(HealthyColor);
	}
	else if (HealthFraction > 0.2f)
	{
		HealthBar->SetFillColorAndOpacity(LowHealthColor);
	}
	else
	{
		HealthBar->SetFillColorAndOpacity(CriticalHealthColor);
	}
}

void UBattleStatusWidget::NativeDestruct()
{
	SetPokemon(nullptr);
	Super::NativeDestruct();
}
