#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattlePokemon.generated.h"

class UAnimInstance;
class UAnimSequence;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPokemonHealthChanged);

UCLASS()
class POKEMONCYBERPUNK_API ABattlePokemon : public AActor
{
	GENERATED_BODY()

public:
	ABattlePokemon();
	void EnterBattle();
	virtual void Tick(float DeltaSeconds) override;

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

	UPROPERTY(VisibleAnywhere, Category = "Pokemon")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon")
	TObjectPtr<UAnimSequence> IdleAnimation;

	// Optional material driven by the animation's RingGlow curves.
	UPROPERTY(EditDefaultsOnly, Category = "Pokemon|Appearance")
	FName GlowMaterialSlot;

	UPROPERTY(VisibleAnywhere, Category = "Pokemon|Appearance")
	TObjectPtr<UStaticMeshComponent> GemSparkle;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon|Appearance")
	FName GemBone;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon|Appearance")
	FVector GemUp = FVector::UpVector;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon|Appearance")
	FVector2D GemSize = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Pokemon|Appearance")
	FVector GemQuadOffset = FVector::ZeroVector;

private:
	void UpdateRingGlow(const UAnimInstance& Animation);
	void UpdateGemSparkle(const UAnimInstance& Animation);

	UPROPERTY(Transient)
	int32 CurrentHealth = 0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
};
