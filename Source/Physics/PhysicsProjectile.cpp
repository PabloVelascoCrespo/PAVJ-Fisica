// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Weapons/WeaponDamageType.h"
#include "Weapons/ProjectileWeaponComponent.h"
#include "PhysicsCharacter.h"
#include <Kismet/GameplayStatics.h>

APhysicsProjectile::APhysicsProjectile()
{
  // Use a sphere as a simple collision representation
  CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
  CollisionComp->InitSphereRadius(5.0f);
  CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
  CollisionComp->OnComponentHit.AddDynamic(this, &APhysicsProjectile::OnHit);

  // Players can't walk on it
  CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
  CollisionComp->CanCharacterStepUpOn = ECB_No;

  // Set as root component
  RootComponent = CollisionComp;

  // Use a ProjectileMovementComponent to govern this projectile's movement
  ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
  ProjectileMovement->UpdatedComponent = CollisionComp;
  ProjectileMovement->InitialSpeed = 3000.f;
  ProjectileMovement->MaxSpeed = 3000.f;
  ProjectileMovement->bRotationFollowsVelocity = true;
  ProjectileMovement->bShouldBounce = true;

  // Die after 3 seconds by default
  InitialLifeSpan = 3.0f;
}

void APhysicsProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
  // @TODO: Handle impact

  if (OtherActor && OtherActor != this)
  {
    // TODO: change the UDamageType::StaticClass()
    // TODO: subir la funcionalidad al arma?
    FHitResult HitResult;
    UGameplayStatics::ApplyPointDamage(OtherActor, m_Damage, GetVelocity().GetSafeNormal(), HitResult, m_OwnerWeapon->Character->GetController(), this, UDamageType::StaticClass());
    if (!m_OwnerWeapon->bProjectileExplode && OtherComp->IsSimulatingPhysics()) // LINEAL
    {
      OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
    }
    else // EXPLOSION
    {
      TArray<FHitResult> HitResults;
      FCollisionQueryParams QueryParams;
      QueryParams.AddIgnoredActor(this);
      FVector Center = GetActorLocation();
      GetWorld()->SweepMultiByChannel(HitResults, Center, Center, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(m_Radius), QueryParams);
      for (const FHitResult& Overlap : HitResults)
      {
        if (AActor* OverlappedActor = Overlap.GetActor())
        {
          //UGameplayStatics::ApplyRadialDamage(GetWorld(), m_Damage, Center, m_Radius, UDamageType::StaticClass(),, this, );
          UGameplayStatics::ApplyDamage(OtherActor, m_Damage, m_OwnerWeapon->Character->GetController(), this, UDamageType::StaticClass());
        }
        if (UPrimitiveComponent* OverlappedActorComponent = Overlap.GetComponent())
        {
          if (OverlappedActorComponent->IsSimulatingPhysics())
          {
            OverlappedActorComponent->AddRadialImpulse(Center, m_Radius, m_Strength, ERadialImpulseFalloff::RIF_Linear);
          }
        }
      }
    }
  }


  if (m_DestroyOnHit)
  {
    Destroy();
  }
}