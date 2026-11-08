#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleManager.generated.h"

class AWillieCharacter;
class ABattlePokemon;
class UCameraComponent;
class ACharacter;
class USceneComponent;

UCLASS()
class POKEMONCYBERPUNK_API ABattleManager : public AActor
{
	GENERATED_BODY()

public:
	ABattleManager();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<UCameraComponent> BattleCamera;

	UPROPERTY(EditInstanceOnly, Category = "Battle")
	TObjectPtr<AWillieCharacter> Opponent;

	UPROPERTY(EditInstanceOnly, Category = "Battle")
	TArray<TObjectPtr<ABattlePokemon>> PlayerTeam;

	UPROPERTY(EditInstanceOnly, Category = "Battle")
	TArray<TObjectPtr<ABattlePokemon>> OpponentTeam;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<USceneComponent> PlayerTrainerPosition;

	UPROPERTY(VisibleAnywhere, Category = "Battle")
	TObjectPtr<USceneComponent> OpponentTrainerPosition;

private:
	void PlaceTrainer(ACharacter* Trainer, const USceneComponent* Position);

	UFUNCTION()
	void StartBattle(AWillieCharacter* Trainer);
};
