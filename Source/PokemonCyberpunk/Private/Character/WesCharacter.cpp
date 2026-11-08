#include "Character/WesCharacter.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace1D.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GamePlayerController.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Interactable/Interactable.h"
#include "UObject/ConstructorHelpers.h"

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

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WesMesh(TEXT("/Game/Characters/Wes/SK_Wes"));
	GetMesh()->SetSkeletalMeshAsset(WesMesh.Object);
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

	static ConstructorHelpers::FObjectFinder<UBlendSpace1D> MovementAnimation(TEXT("/Game/Characters/Wes/Animations/BS_Wes_Locomotion"));
	Locomotion = MovementAnimation.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveInput(TEXT("/Game/Input/IA_Move"));
	MoveAction = MoveInput.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> LookInput(TEXT("/Game/Input/IA_Look"));
	LookAction = LookInput.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> RunInput(TEXT("/Game/Input/IA_Run"));
	RunAction = RunInput.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractInput(TEXT("/Game/Input/IA_Interact"));
	InteractAction = InteractInput.Object;
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
	if (APlayerController* Player = Cast<APlayerController>(GetController()))
	{
		Player->SetControlRotation(FRotator(-15.0f, GetActorRotation().Yaw, 0.0f));
		if (Player->PlayerCameraManager)
		{
			Player->PlayerCameraManager->ViewPitchMin = -65.0f;
			Player->PlayerCameraManager->ViewPitchMax = 20.0f;
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
	const FRotator CameraYaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X), Direction.Y);
	AddMovementInput(FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y), Direction.X);
}

void AWesCharacter::Look(const FInputActionValue& Value)
{
	if (!Controller || Controller->IsLookInputIgnored()) return;
	const FVector2D Direction = Value.Get<FVector2D>();
	AddControllerYawInput(Direction.X);
	AddControllerPitchInput(Direction.Y);
}

void AWesCharacter::StartRunning()
{
	if (!Controller || Controller->IsMoveInputIgnored()) return;
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
		Animation->SetBlendSpacePosition(FVector(GetVelocity().Size2D(), 0.0f, 0.0f));
	}
	if (AGamePlayerController* Player = Cast<AGamePlayerController>(GetController()))
	{
		UPrimitiveComponent* Target = Player->IsInDialogue() || Player->IsInBattle() ? nullptr : FindInteractionTarget();
		IInteractable* Interactable = Target ? Cast<IInteractable>(Target->GetOwner()) : nullptr;
		Player->ShowInteractionPrompt(Interactable ? Interactable->GetInteractionPrompt() : FText::GetEmpty());
	}
}

UPrimitiveComponent* AWesCharacter::FindInteractionTarget() const
{
	const float InteractionDistance = 220.0f;
	TArray<FOverlapResult> Nearby;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(Interaction), false, this);
	GetWorld()->OverlapMultiByObjectType(Nearby, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams::AllDynamicObjects, FCollisionShape::MakeSphere(InteractionDistance), Query);

	UPrimitiveComponent* Nearest = nullptr;
	float NearestDistance = FMath::Square(InteractionDistance);
	const FVector HeadHeight(0.0f, 0.0f, 40.0f);
	for (const FOverlapResult& Result : Nearby)
	{
		AActor* Target = Result.GetActor();
		if (!IsValid(Target) || !Cast<IInteractable>(Target)) continue;
		const float Distance = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
		if (Distance > NearestDistance) continue;

		FCollisionQueryParams VisibilityQuery = Query;
		VisibilityQuery.AddIgnoredActor(Target);
		if (!GetWorld()->LineTraceTestByChannel(GetActorLocation() + HeadHeight,
			Target->GetActorLocation() + HeadHeight, ECC_Visibility, VisibilityQuery))
		{
			Nearest = Result.GetComponent();
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

void AWesCharacter::Interact()
{
	AGamePlayerController* Player = Cast<AGamePlayerController>(GetController());
	if (!Player || Player->IsInBattle()) return;
	if (Player->IsInDialogue())
	{
		Player->AdvanceDialogue();
	}
	else if (UPrimitiveComponent* Target = FindInteractionTarget())
	{
		CastChecked<IInteractable>(Target->GetOwner())->Interact(this, Target);
	}
}

void AWesCharacter::PrepareForDialogue(AActor* Speaker)
{
	StopRunning();
	GetCharacterMovement()->StopMovementImmediately();
	const FVector ToSpeaker = Speaker->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotator(0.0f, ToSpeaker.Rotation().Yaw, 0.0f));
}

bool AWesCharacter::IsInDialogue() const
{
	const AGamePlayerController* Player = Cast<AGamePlayerController>(GetController());
	return Player && Player->IsInDialogue();
}
