#include "UI/BattleStatusWidget.h"

#include "Battle/BattlePokemon.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

// Assigns a Pokemon to this status card and listens for changes to its health.
void UBattleStatusWidget::SetPokemon(ABattlePokemon* InPokemon)
{
    // Stop listening to the previous Pokemon before assigning a new one.
    if (Pokemon.IsValid())
    {
        Pokemon->OnHealthChanged.RemoveDynamic(this, &UBattleStatusWidget::RefreshHealth);
    }

    Pokemon = InPokemon;

    // Automatically refresh the health display whenever this Pokemon's HP changes.
    if (Pokemon.IsValid())
    {
        Pokemon->OnHealthChanged.AddDynamic(this, &UBattleStatusWidget::RefreshHealth);
    }

    RefreshHealth();
}

// Updates the Pokemon's displayed name, HP numbers, and health bar.
void UBattleStatusWidget::RefreshHealth()
{
    // Hide the entire status card if no valid Pokemon is assigned.
    SetVisibility(Pokemon.IsValid() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (!Pokemon.IsValid())
    {
        return;
    }

    // Display the Pokemon's name in uppercase and its current/maximum HP.
    PokemonName->SetText(Pokemon->GetDisplayName().ToUpper());
    HealthText->SetText(FText::Format(NSLOCTEXT("Battle", "Health", "HP  {0} / {1}"),
        FText::AsNumber(Pokemon->GetHealth()), FText::AsNumber(Pokemon->GetMaxHealth())));

    // Convert remaining health into a percentage between 0.0 and 1.0.
    // For example, 75/100 HP produces 0.75, filling 75% of the bar.
    const float HealthFraction = static_cast<float>(Pokemon->GetHealth()) / Pokemon->GetMaxHealth();
    HealthBar->SetPercent(HealthFraction);

    // Change the health bar's color depending on how much HP remains.
    if (HealthFraction > 0.5f)
    {
        // Above 50% health: healthy color (normally green).
        HealthBar->SetFillColorAndOpacity(HealthyColor);
    }
    else if (HealthFraction > 0.2f)
    {
        // Above 20%, up to 50%: low health color (normally yellow/orange).
        HealthBar->SetFillColorAndOpacity(LowHealthColor);
    }
    else
    {
        // 20% or below: critical health color (normally red).
        HealthBar->SetFillColorAndOpacity(CriticalHealthColor);
    }
}

// Cleans up the Pokemon reference when this status card is destroyed.
void UBattleStatusWidget::NativeDestruct()
{
    // Remove the health-change listener so the old Pokemon
    // cannot attempt to update a destroyed widget.
    SetPokemon(nullptr);
    Super::NativeDestruct();
}
