#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WesCharacter.generated.h"

class UBlendSpace1D;
class UCameraComponent;
class UInputAction;
class UPrimitiveComponent;
class USpringArmComponent;
struct FInputActionValue;

// The playable trainer. Unreal handles walking and collision; this class connects input and interaction.
UCLASS()
class POKEMONCYBERPUNK_API AWesCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWesCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void PrepareForDialogue(AActor* Speaker);

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const;

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartRunning();
	void StopRunning();
	void Interact();
	UPrimitiveComponent* FindInteractionTarget() const;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UBlendSpace1D> Locomotion;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 400.0f;
};
