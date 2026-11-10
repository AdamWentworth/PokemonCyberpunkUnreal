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

AWesCharacter::AWesCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(30.0f, 88.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 1200.0f;

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -87.2f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 55.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AWesCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetMesh()->PlayAnimation(Locomotion, true);
}

void AWesCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetControlRotation(FRotator(-15.0f, GetActorRotation().Yaw, 0.0f));
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->ViewPitchMin = -65.0f;
			PlayerController->PlayerCameraManager->ViewPitchMax = 20.0f;
		}
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWesCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWesCharacter::Look);
	Input->BindAction(RunAction, ETriggerEvent::Started, this, &AWesCharacter::StartRunning);
	Input->BindAction(RunAction, ETriggerEvent::Completed, this, &AWesCharacter::StopRunning);
	Input->BindAction(RunAction, ETriggerEvent::Canceled, this, &AWesCharacter::StopRunning);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AWesCharacter::Interact);
}

void AWesCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || Controller->IsMoveInputIgnored())
	{
		return;
	}
	const FVector2D Direction = Value.Get<FVector2D>();
	// Forward on the controls means forward from the camera, not from Wes's current facing.
	const FRotator CameraYaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X), Direction.Y);
	AddMovementInput(FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y), Direction.X);
}

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

void AWesCharacter::StartRunning()
{
	if (!Controller || Controller->IsMoveInputIgnored())
	{
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void AWesCharacter::StopRunning()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AWesCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (UAnimSingleNodeInstance* Animation = GetMesh()->GetSingleNodeInstance())
	{
		// Speed chooses the blend between the idle, walking and running animations.
		Animation->SetBlendSpacePosition(FVector(GetVelocity().Size2D(), 0.0f, 0.0f));
	}
	if (AGamePlayerController* PlayerController = Cast<AGamePlayerController>(GetController()))
	{
		const bool bCanInteract = !PlayerController->IsInDialogue() && !PlayerController->IsInBattle();
		UPrimitiveComponent* Target = bCanInteract ? FindInteractionTarget() : nullptr;
		IInteractable* Interactable = Target ? Cast<IInteractable>(Target->GetOwner()) : nullptr;
		PlayerController->ShowInteractionPrompt(Interactable ? Interactable->GetInteractionPrompt() : FText::GetEmpty());
	}
}

UPrimitiveComponent* AWesCharacter::FindInteractionTarget() const
{
	// Pick the nearest interactable in reach. Ignore anyone hidden behind a wall.
	const float InteractionDistance = 220.0f;
	TArray<FOverlapResult> NearbyOverlaps;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(Interaction), false, this);
	GetWorld()->OverlapMultiByObjectType(NearbyOverlaps, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams::AllDynamicObjects, FCollisionShape::MakeSphere(InteractionDistance), Query);

	UPrimitiveComponent* NearestComponent = nullptr;
	float NearestDistanceSquared = FMath::Square(InteractionDistance);
	const FVector TraceHeightOffset(0.0f, 0.0f, 40.0f);
	for (const FOverlapResult& Result : NearbyOverlaps)
	{
		AActor* Target = Result.GetActor();
		if (!IsValid(Target) || !Cast<IInteractable>(Target))
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
		if (DistanceSquared > NearestDistanceSquared)
		{
			continue;
		}

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

void AWesCharacter::Interact()
{
	AGamePlayerController* PlayerController = Cast<AGamePlayerController>(GetController());
	if (!PlayerController || PlayerController->IsInBattle())
	{
		return;
	}
	if (PlayerController->IsInDialogue())
	{
		PlayerController->AdvanceDialogue();
	}
	else if (UPrimitiveComponent* Target = FindInteractionTarget())
	{
		CastChecked<IInteractable>(Target->GetOwner())->Interact(this, Target);
	}
}

void AWesCharacter::PrepareForDialogue(AActor* Speaker)
{
	// Stop leftover movement before turning toward the person being spoken to.
	StopRunning();
	GetCharacterMovement()->StopMovementImmediately();
	const FVector ToSpeaker = Speaker->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.0f, ToSpeaker.Rotation().Yaw, 0.0f));
}

bool AWesCharacter::IsInDialogue() const
{
	const AGamePlayerController* PlayerController = Cast<AGamePlayerController>(GetController());
	return PlayerController && PlayerController->IsInDialogue();
}
