#include "UI/BattleStatusWidget.h"

#include "Battle/BattlePokemon.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBattleStatusWidget::SetPokemon(ABattlePokemon* InPokemon)
{
	if (Pokemon.IsValid()) Pokemon->OnHealthChanged.RemoveDynamic(this, &UBattleStatusWidget::RefreshHealth);
	Pokemon = InPokemon;
	if (Pokemon.IsValid()) Pokemon->OnHealthChanged.AddDynamic(this, &UBattleStatusWidget::RefreshHealth);
	RefreshHealth();
}

void UBattleStatusWidget::RefreshHealth()
{
	SetVisibility(Pokemon.IsValid() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Pokemon.IsValid()) return;
	PokemonName->SetText(Pokemon->GetDisplayName().ToUpper());
	HealthText->SetText(FText::Format(NSLOCTEXT("Battle", "Health", "HP  {0} / {1}"),
		FText::AsNumber(Pokemon->GetHealth()), FText::AsNumber(Pokemon->GetMaxHealth())));
	const float Fraction = static_cast<float>(Pokemon->GetHealth()) / Pokemon->GetMaxHealth();
	HealthBar->SetPercent(Fraction);
	HealthBar->SetFillColorAndOpacity(Fraction > 0.5f ? FLinearColor(0.12f, 0.8f, 0.38f)
		: Fraction > 0.2f ? FLinearColor(1.0f, 0.65f, 0.12f) : FLinearColor(0.95f, 0.16f, 0.12f));
}

void UBattleStatusWidget::NativeDestruct()
{
	SetPokemon(nullptr);
	Super::NativeDestruct();
}
