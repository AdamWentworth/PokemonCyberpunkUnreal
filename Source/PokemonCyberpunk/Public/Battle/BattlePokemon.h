#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattlePokemon.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPokemonHealthChanged);

// One Pokemon in the battle: its model, available moves and current health.
// Species-specific appearance is configured in its Blueprint and materials.
UCLASS()
class POKEMONCYBERPUNK_API ABattlePokemon : public AActor
{
	GENERATED_BODY()

public:
	ABattlePokemon();
	void EnterBattle();
	void LeaveBattle();
	void KnockOut();

	const TArray<FText>& GetMoveNames() const { return MoveNames; }

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

	UFUNCTION(BlueprintCallable, Category = "Pokemon")
	void SetHealth(int32 NewHealth);

	UPROPERTY(BlueprintAssignable, Category = "Pokemon")
	FOnPokemonHealthChanged OnHealthChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon", meta = (ClampMin = "1"))
	int32 MaxHealth = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pokemon")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
	TArray<FText> MoveNames;

private:
	UPROPERTY(Transient)
	int32 CurrentHealth = 0;
};
