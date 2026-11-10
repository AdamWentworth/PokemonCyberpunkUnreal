#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattlePokemon.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

// Event fired whenever a Pokemon's health changes.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPokemonHealthChanged);

// Represents one Pokemon participating in battle.
// Its appearance and stats can be configured through a Blueprint.
UCLASS()
class POKEMONCYBERPUNK_API ABattlePokemon : public AActor
{
    GENERATED_BODY()

public:
    ABattlePokemon();

    // Controls entering combat, leaving combat, and fainting.
    void EnterBattle();
    void LeaveBattle();
    void KnockOut();

    // Returns the names of the Pokemon's available attacks.
    const TArray<FText>& GetMoveNames() const { return MoveNames; }

    // Getter functions expose Pokemon information to other systems and Blueprints.
    UFUNCTION(BlueprintPure, Category = "Pokemon")
    FText GetDisplayName() const
    {
        return DisplayName;
    }

    UFUNCTION(BlueprintPure, Category = "Pokemon")
    int32 GetHealth() const
    {
        return CurrentHealth;
    }

    UFUNCTION(BlueprintPure, Category = "Pokemon")
    int32 GetMaxHealth() const
    {
        return MaxHealth;
    }

    // Changes health, keeping it between zero and the maximum allowed.
    UFUNCTION(BlueprintCallable, Category = "Pokemon")
    void SetHealth(int32 NewHealth);

    // Allows interface elements, such as health bars, to react to HP changes.
    UPROPERTY(BlueprintAssignable, Category = "Pokemon")
    FOnPokemonHealthChanged OnHealthChanged;

protected:
    virtual void BeginPlay() override;

    // Pokemon name and maximum HP can be configured in the Unreal Editor.
    UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, Category = "Pokemon", meta = (ClampMin = "1"))
    int32 MaxHealth = 1;

    // The skeletal mesh is the Pokemon's animated 3D model.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pokemon")
    TObjectPtr<USkeletalMeshComponent> Body;

    // Animation played while the Pokemon is standing in battle.
    UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
    TObjectPtr<UAnimSequence> IdleAnimation;

    // Names of the attacks this Pokemon can select during combat.
    UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
    TArray<FText> MoveNames;

private:
    // Stores the Pokemon's actual remaining HP during gameplay.
    UPROPERTY(Transient)
    int32 CurrentHealth = 0;
};
