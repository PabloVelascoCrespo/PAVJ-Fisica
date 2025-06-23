// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsGameMode.h"
#include "PhysicsCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "BreakableTarget.h"
#include <Kismet/GameplayStatics.h>

APhysicsGameMode::APhysicsGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void APhysicsGameMode::BeginPlay()
{
	Super::BeginPlay();
	// @TODO: Get total and current target configuration
	ABreakableTarget::OnTargetBroken.AddDynamic(this, &APhysicsGameMode::ReduceRemainingTargets);
	m_TotalTargets = ABreakableTarget::OnTargetBroken.GetAllObjects().Num();
	m_RemainingTargets = m_TotalTargets;
}

void APhysicsGameMode::ReduceRemainingTargets(ABreakableTarget* BrokenTarget)
{
	// @TODO: make sure to notify other components, including blueprints
	if (m_RemainingTargets >= 1)
	{
		m_RemainingTargets--;
		OnTargetCountChange.Broadcast();
	}
	else
	{
		OnWinConditionMet.Broadcast();
	}
}
