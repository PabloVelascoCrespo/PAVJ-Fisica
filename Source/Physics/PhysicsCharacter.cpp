// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsCharacter.h"
#include "PhysicsProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Components/StaticMeshComponent.h>
#include <PhysicsEngine/PhysicsHandleComponent.h>

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

APhysicsCharacter::APhysicsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	m_PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
}

void APhysicsCharacter::BeginPlay()
{
	Super::BeginPlay();

	bBlockSprint = false;
	m_CurrentStamina = m_MaxStamina;
	CharacterMovementComponent = GetCharacterMovement();
}

void APhysicsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// @TODO: Stamina update
	UpdateStamina(DeltaSeconds);
	// @TODO: Physics objects highlight
	FindGrabbableObjects();
	// @TODO: Grabbed object update
	UpdateGrabbedObject();
}

void APhysicsCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APhysicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Look);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::GrabObject);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Completed, this, &APhysicsCharacter::ReleaseObject);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void APhysicsCharacter::SetIsSprinting(bool NewIsSprinting)
{
	// @TODO: Enable/disable sprinting use CharacterMovementComponent
	if (!CharacterMovementComponent)
	{
		CharacterMovementComponent = GetCharacterMovement();
	}

	bIsTryingToRun = NewIsSprinting;
	if (!bIsTryingToRun)
	{
		bBlockSprint = false;
	}
	if (NewIsSprinting && m_CurrentStamina > 0.0f && !bBlockSprint)
	{
		CharacterMovementComponent->MaxWalkSpeed = m_SprintSpeed;
	}
	else
	{
		CharacterMovementComponent->MaxWalkSpeed = m_WalkSpeed;

	}
}

FHitResult APhysicsCharacter::RayCast() const
{
	FHitResult Hit;
	FVector Start = GetActorLocation() + FirstPersonCameraComponent->GetRelativeLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * m_MaxGrabDistance);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);
	return Hit;
}

float APhysicsCharacter::GetStamina() const
{
	return m_CurrentStamina;
}

void APhysicsCharacter::UpdateGrabbedObject()
{
	if (!m_GrabbedComponent)
	{
		return;
	}
	FVector Forward = FirstPersonCameraComponent->GetForwardVector();
	FVector Location = GetActorLocation() + Forward * m_DistanceWithGrabbedObject;
	m_PhysicsHandle->SetTargetLocation(Location);
}

void APhysicsCharacter::FindGrabbableObjects()
{
	FHitResult Hit = RayCast();
	if (!Hit.GetActor() || !(Hit.GetComponent()->Mobility == EComponentMobility::Movable))
	{
		return;
	}
	UMeshComponent* MeshComponent = Hit.GetActor()->GetComponentByClass<UMeshComponent>();
	SetHighlightedMesh(MeshComponent);
}

void APhysicsCharacter::UpdateStamina(float DeltaSeconds)
{
	if (!CharacterMovementComponent)
	{
		CharacterMovementComponent = GetCharacterMovement();
	}

	bool bIsSprinting = CharacterMovementComponent->GetVelocityForNavMovement().Length() > m_WalkSpeed * 1.1f;

	if (bIsSprinting)
	{
		float AuxStamina = m_CurrentStamina - m_StaminaDepletionRate * DeltaSeconds;
		m_CurrentStamina = (AuxStamina < 0.0f) ? 0.0f : AuxStamina;
		if (m_CurrentStamina <= KINDA_SMALL_NUMBER)
		{
			bBlockSprint = true;
		}
	}
	else if(bIsTryingToRun)
	{
		float AuxStamina = m_CurrentStamina + m_StaminaRecoveryRate * DeltaSeconds;
		m_CurrentStamina = (AuxStamina > m_MaxStamina) ? m_MaxStamina : AuxStamina;
	}
	else
	{
		float AuxStamina = m_CurrentStamina + m_StaminaRecoveryRate * DeltaSeconds;
		m_CurrentStamina = (AuxStamina > m_MaxStamina) ? m_MaxStamina : AuxStamina;
	}
}

void APhysicsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void APhysicsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APhysicsCharacter::Sprint(const FInputActionValue& Value)
{
	SetIsSprinting(Value.Get<bool>());
}

void APhysicsCharacter::GrabObject(const FInputActionValue& Value)
{
	// @TODO: Grab objects using UPhysicsHandleComponent
	if (!m_GrabbedComponent)
	{
		FHitResult Hit = RayCast();
		if (!Hit.GetActor() || !(Hit.GetComponent()->Mobility == EComponentMobility::Movable))
		{
			return;
		}
		m_GrabbedComponent = Hit.GetActor()->GetComponentByClass<UPrimitiveComponent>();
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, (TEXT("Grabbing: %s"), *m_GrabbedComponent->GetName()));
		m_PhysicsHandle->GrabComponentAtLocationWithRotation(m_GrabbedComponent, FName(""), Hit.Location, Hit.GetActor()->GetActorRotation());
		m_DistanceWithGrabbedObject = Hit.Distance;
	}
}

void APhysicsCharacter::ReleaseObject(const FInputActionValue& Value)
{
	// @TODO: Release grabebd object using UPhysicsHandleComponent
	m_GrabbedComponent = nullptr;
	m_DistanceWithGrabbedObject = 0.0f;
	m_PhysicsHandle->ReleaseComponent();
}

void APhysicsCharacter::SetHighlightedMesh(UMeshComponent* StaticMesh)
{
	if (m_HighlightedMesh)
	{
		m_HighlightedMesh->SetOverlayMaterial(nullptr);
	}
	m_HighlightedMesh = StaticMesh;
	if (m_HighlightedMesh)
	{
		m_HighlightedMesh->SetOverlayMaterial(m_HighlightMaterial);
	}
}
