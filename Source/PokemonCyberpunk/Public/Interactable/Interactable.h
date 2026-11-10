#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class UPrimitiveComponent;

// Registers this interface with Unreal's reflection system,
// allowing Unreal objects to recognize classes that implement it.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

// Defines the functions any interactable object must provide.
// For example, NPCs can use these to start conversations.
class POKEMONCYBERPUNK_API IInteractable
{
	GENERATED_BODY()

public:
	// Called when the player interacts with an object.
	// Interactor identifies who initiated the interaction.
	// HitComponent identifies which component was interacted with.
	virtual void Interact(AActor* Interactor, UPrimitiveComponent* HitComponent) = 0;

	// Returns the text shown when the player approaches,
	// such as "Talk to Willie".
	virtual FText GetInteractionPrompt() const = 0;
};
