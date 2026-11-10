#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WesCharacter.generated.h"

// Forward declarations for Unreal components and input types.
class UBlendSpace1D;
class UCameraComponent;
class UInputAction;
class UPrimitiveComponent;
class USpringArmComponent;
struct FInputActionValue;

// The playable trainer. Unreal handles walking and collision;
// this class connects input, camera movement, and interactions.
UCLASS()
class POKEMONCYBERPUNK_API AWesCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AWesCharacter();

    // Runs every frame to update animations and interaction prompts.
    virtual void Tick(float DeltaTime) override;

    // Connects keyboard/controller actions to the character's functions.
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // Stops Wes and turns him toward an NPC before dialogue begins.
    void PrepareForDialogue(AActor* Speaker);

    // Allows Blueprints to check whether Wes is in a conversation.
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsInDialogue() const;

protected:
    // Called when Wes enters the running game.
    virtual void BeginPlay() override;

private:
    // Handle movement, camera rotation, running, and interactions.
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartRunning();
    void StopRunning();
    void Interact();

    // Finds the closest interactable object that isn't blocked by an obstacle.
    UPrimitiveComponent* FindInteractionTarget() const;

    // Spring arm positions the third-person camera behind Wes.
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    // The camera through which the player sees the world.
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    // Animation system that blends between standing, walking, and running.
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UBlendSpace1D> Locomotion;

    // Input actions configured in the Unreal Editor.
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> RunAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    // Normal walking and running speeds, measured in Unreal units per second.
    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float WalkSpeed = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float RunSpeed = 400.0f;
};
