#include "Character/WesCharacter.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace1D.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GamePlayerController.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Interactable/Interactable.h"

// Configures Wes's movement, collision, 3D model, and third-person camera.
AWesCharacter::AWesCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Set the size of the invisible collision capsule surrounding Wes.
    GetCapsuleComponent()->InitCapsuleSize(30.0f, 88.0f);

    // Make Wes turn toward his movement direction rather than always following the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->BrakingDecelerationWalking = 1200.0f;

    // Align the character's visual model with its collision capsule.
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -87.2f));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

    // A spring arm holds the camera behind Wes and manages its positioning.
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 55.0f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bInheritRoll = false;

    // Attach the camera to the end of the spring arm.
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

// Runs when gameplay begins and starts the character's movement animation.
void AWesCharacter::BeginPlay()
{
    Super::BeginPlay();
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetMesh()->PlayAnimation(Locomotion, true);
}

// Connects player inputs to movement, camera, running, and interaction functions.
void AWesCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        // Start the camera slightly above Wes and restrict vertical camera movement.
        PlayerController->SetControlRotation(FRotator(-15.0f, GetActorRotation().Yaw, 0.0f));
        if (PlayerController->PlayerCameraManager)
        {
            PlayerController->PlayerCameraManager->ViewPitchMin = -65.0f;
            PlayerController->PlayerCameraManager->ViewPitchMax = 20.0f;
        }
    }

    // Bind Unreal's Enhanced Input actions to the corresponding functions.
    // Started = button pressed; Completed/Canceled = button released or interrupted.
    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWesCharacter::Move);
    Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWesCharacter::Look);
    Input->BindAction(RunAction, ETriggerEvent::Started, this, &AWesCharacter::StartRunning);
    Input->BindAction(RunAction, ETriggerEvent::Completed, this, &AWesCharacter::StopRunning);
    Input->BindAction(RunAction, ETriggerEvent::Canceled, this, &AWesCharacter::StopRunning);
    Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AWesCharacter::Interact);
}

// Moves Wes relative to the camera's direction rather than his current facing.
void AWesCharacter::Move(const FInputActionValue& Value)
{
    // Ignore movement while controls are disabled, such as during dialogue.
    if (!Controller || Controller->IsMoveInputIgnored())
    {
        return;
    }

    // Read the two-dimensional movement input (forward/backward and left/right).
    const FVector2D Direction = Value.Get<FVector2D>();

    // Use only the camera's horizontal rotation to calculate movement directions.
    const FRotator CameraYaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X), Direction.Y);
    AddMovementInput(FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y), Direction.X);
}

// Rotates the camera horizontally and vertically using player input.
void AWesCharacter::Look(const FInputActionValue& Value)
{
    if (!Controller || Controller->IsLookInputIgnored())
    {
        return;
    }

    const FVector2D Direction = Value.Get<FVector2D>();
    AddControllerYawInput(Direction.X);
    AddControllerPitchInput(Direction.Y);
}

// Increases movement speed when the player starts running.
void AWesCharacter::StartRunning()
{
    if (!Controller || Controller->IsMoveInputIgnored())
    {
        return;
    }
    GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

// Restores normal walking speed when running stops.
void AWesCharacter::StopRunning()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

// Runs every frame to update animations and nearby interaction prompts.
void AWesCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Blend between standing, walking, and running animations based on movement speed.
    if (UAnimSingleNodeInstance* Animation = GetMesh()->GetSingleNodeInstance())
    {
        Animation->SetBlendSpacePosition(FVector(GetVelocity().Size2D(), 0.0f, 0.0f));
    }

    // Show an interaction prompt when Wes is near an interactable object or NPC.
    if (AGamePlayerController* PlayerController = Cast<AGamePlayerController>(GetController()))
    {
        // Interaction prompts are disabled during dialogue and battles.
        const bool bCanInteract = !PlayerController->IsInDialogue() && !PlayerController->IsInBattle();
        UPrimitiveComponent* Target = bCanInteract ? FindInteractionTarget() : nullptr;
        IInteractable* Interactable = Target ? Cast<IInteractable>(Target->GetOwner()) : nullptr;
        PlayerController->ShowInteractionPrompt(Interactable ? Interactable->GetInteractionPrompt() : FText::GetEmpty());
    }
}

// Finds the closest interactable object within reach that isn't blocked by a wall.
UPrimitiveComponent* AWesCharacter::FindInteractionTarget() const
{
    // Search for nearby objects within 220 Unreal units (centimeters by default).
    const float InteractionDistance = 220.0f;
    TArray<FOverlapResult> NearbyOverlaps;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(Interaction), false, this);

    // Detect nearby dynamic objects using an invisible spherical search area.
    GetWorld()->OverlapMultiByObjectType(NearbyOverlaps, GetActorLocation(), FQuat::Identity,
        FCollisionObjectQueryParams::AllDynamicObjects, FCollisionShape::MakeSphere(InteractionDistance), Query);

    UPrimitiveComponent* NearestComponent = nullptr;
    float NearestDistanceSquared = FMath::Square(InteractionDistance);
    const FVector TraceHeightOffset(0.0f, 0.0f, 40.0f);

    // Examine each nearby object to find the closest valid interaction target.
    for (const FOverlapResult& Result : NearbyOverlaps)
    {
        AActor* Target = Result.GetActor();

        // Ignore objects that don't implement the interactable interface.
        if (!IsValid(Target) || !Cast<IInteractable>(Target))
        {
            continue;
        }

        // Compare squared distances, avoiding unnecessary square-root calculations.
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
        if (DistanceSquared > NearestDistanceSquared)
        {
            continue;
        }

        // Perform a visibility check so Wes cannot interact through walls.
        FCollisionQueryParams VisibilityQuery = Query;
        VisibilityQuery.AddIgnoredActor(Target);
        if (!GetWorld()->LineTraceTestByChannel(GetActorLocation() + TraceHeightOffset,
            Target->GetActorLocation() + TraceHeightOffset, ECC_Visibility, VisibilityQuery))
        {
            NearestComponent = Result.GetComponent();
            NearestDistanceSquared = DistanceSquared;
        }
    }

    return NearestComponent;
}

// Handles the interaction button, either advancing dialogue or interacting with an object.
void AWesCharacter::Interact()
{
    AGamePlayerController* PlayerController = Cast<AGamePlayerController>(GetController());

    // Do not permit ordinary interactions during battles.
    if (!PlayerController || PlayerController->IsInBattle())
    {
        return;
    }

    // If already speaking to an NPC, advance to the next dialogue line.
    if (PlayerController->IsInDialogue())
    {
        PlayerController->AdvanceDialogue();
    }
    else if (UPrimitiveComponent* Target = FindInteractionTarget())
    {
        // Otherwise, interact with the nearest eligible object or NPC.
        CastChecked<IInteractable>(Target->GetOwner())->Interact(this, Target);
    }
}

// Stops Wes and turns him toward the NPC when a conversation begins.
void AWesCharacter::PrepareForDialogue(AActor* Speaker)
{
    StopRunning();
    GetCharacterMovement()->StopMovementImmediately();

    // Calculate the direction toward the speaker and face them horizontally.
    const FVector ToSpeaker = Speaker->GetActorLocation() - GetActorLocation();
    SetActorRotation(FRotator(0.0f, ToSpeaker.Rotation().Yaw, 0.0f));
}

// Checks with the player controller to determine whether dialogue is active.
bool AWesCharacter::IsInDialogue() const
{
    const AGamePlayerController* PlayerController = Cast<AGamePlayerController>(GetController());
    return PlayerController && PlayerController->IsInDialogue();
}
