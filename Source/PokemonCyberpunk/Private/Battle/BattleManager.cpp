#include "Battle/BattleManager.h"
#include "Battle/BattlePokemon.h"

#include "Camera/CameraComponent.h"
#include "Character/WillieCharacter.h"
#include "Components/SceneComponent.h"
#include "Core/GamePlayerController.h"
#include "Kismet/GameplayStatics.h"

ABattleManager::ABattleManager()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Battlefield"));
	BattleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BattleCamera"));
	BattleCamera->SetupAttachment(RootComponent);
	BattleCamera->SetFieldOfView(65.0f);
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(Opponent))
	{
		Opponent->OnBattleChallengeAccepted.AddDynamic(this, &ABattleManager::StartBattle);
	}
}

void ABattleManager::StartBattle(AWillieCharacter* Trainer)
{
	if (!IsValid(Trainer) || Trainer != Opponent) return;
	if (AGamePlayerController* Player = Cast<AGamePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (Player->IsInBattle() || Player->IsInDialogue()) return;
		Player->BeginBattleView(this);
		for (ABattlePokemon* Pokemon : PlayerTeam)
		{
			if (IsValid(Pokemon)) Pokemon->EnterBattle();
		}
	}
}
