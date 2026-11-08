#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattlePokemon.generated.h"

class UAnimSequence;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS()
class POKEMONCYBERPUNK_API ABattlePokemon : public AActor
{
	GENERATED_BODY()

public:
	ABattlePokemon();
	void EnterBattle();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

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
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
};
