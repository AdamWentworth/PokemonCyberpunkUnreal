#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleStatusWidget.generated.h"

class ABattlePokemon;
class UProgressBar;
class UTextBlock;

// Represents one Pokemon's health/status card during battle.
// The main BattleHUD creates separate cards for each Pokemon.
UCLASS(Abstract)
class POKEMONCYBERPUNK_API UBattleStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Assigns a Pokemon to this card and connects its health updates.
    void SetPokemon(ABattlePokemon* InPokemon);

protected:
    // Runs when the widget is destroyed, allowing event listeners to be removed.
    virtual void NativeDestruct() override;

    // BindWidget links these variables to matching elements in the Widget Blueprint.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PokemonName;

    // Displays current and maximum HP, such as "HP 75 / 100".
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealthText;

    // Visual bar showing the percentage of health remaining.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;

    // Color used when the Pokemon has more than 50% HP.
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    FLinearColor HealthyColor = FLinearColor(0.12f, 0.8f, 0.38f);

    // Color used when HP is above 20% but no higher than 50%.
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    FLinearColor LowHealthColor = FLinearColor(1.0f, 0.65f, 0.12f);

    // Color used when HP drops to 20% or below.
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    FLinearColor CriticalHealthColor = FLinearColor(0.95f, 0.16f, 0.12f);

private:
    // Refreshes the Pokemon's name, HP numbers, and health bar.
    // Called automatically when the Pokemon's health changes.
    UFUNCTION()
    void RefreshHealth();

    // Weak reference to the Pokemon represented by this card.
    // Does not prevent the Pokemon object from being destroyed.
    TWeakObjectPtr<ABattlePokemon> Pokemon;
};
