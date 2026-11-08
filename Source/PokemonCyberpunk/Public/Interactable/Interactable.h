#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class UPrimitiveComponent;

UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class POKEMONCYBERPUNK_API IInteractable
{
	GENERATED_BODY()

public:
	virtual void Interact(AActor* Interactor, UPrimitiveComponent* HitComponent) = 0;
	virtual FText GetInteractionPrompt() const = 0;
};
