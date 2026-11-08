#include "Core/GamePlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"

void AGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext) Input->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext) Input->RemoveMappingContext(DefaultMappingContext);
		}
	}
	Super::EndPlay(EndPlayReason);
}
