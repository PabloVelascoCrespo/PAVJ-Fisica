// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/HitscanWeaponComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <Kismet/GameplayStatics.h>
#include "PhysicsCharacter.h"
#include "PhysicsWeaponComponent.h"
#include <Camera/CameraComponent.h>
#include <Components/SphereComponent.h>

void UHitscanWeaponComponent::Fire()
{
	Super::Fire();

	// @TODO: Add firing functionality
	if (!GetOwner())
	{
		return;
	}

	FVector Start = GetComponentLocation();
	FVector ForwardVector = Character->FirstPersonCameraComponent->GetForwardVector();
	FVector End = Start + (ForwardVector * m_fRange);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

	// DEBUG BORRAR!!!
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 1.0f);
	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		if (UPrimitiveComponent* HitComp = HitResult.GetComponent())
		{
			if (HitComp->IsSimulatingPhysics())
			{
				FVector Impulse = ForwardVector * m_ImpactForce;
				HitComp->AddImpulseAtLocation(Impulse, HitResult.ImpactPoint);
			}
		}
		onHitscanImpact.Broadcast(HitActor, HitResult.ImpactPoint, ForwardVector);
	}

}
