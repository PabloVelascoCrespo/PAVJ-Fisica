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
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		AActor* OtherActor = HitResult.GetActor();
		ApplyDamage(OtherActor, HitResult);
		onHitscanImpact.Broadcast(OtherActor, HitResult.ImpactPoint, ForwardVector);
	}

}
