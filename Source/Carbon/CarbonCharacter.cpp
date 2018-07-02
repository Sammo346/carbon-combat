// Sam Smith - Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CarbonCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/DamageType.h"
#include "Camera/CameraShake.h"
#include "Engine/World.h"
#include "EnemyBase.h"
#include "Containers/Set.h"
#include "DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////
// ACarbonCharacter

ACarbonCharacter::ACarbonCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	TargetLockDistance = 1500.0f;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create weapon
	Weapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), "RightHandItem");
	Weapon->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	// Create sphere collider (for keeping track of nearby pawns)
	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollider"));
	SphereCollider->SetupAttachment(RootComponent);
	SphereCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SphereCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	SphereCollider->SetSphereRadius(TargetLockDistance);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	Attacking = false;
	Rolling = false;
	TargetLocked = false;
	NextAttackReady = false;
	AttackDamaging = false;
	AttackIndex = 0;

	PassiveMovementSpeed = 450.0f;
	CombatMovementSpeed = 250.0f;
	GetCharacterMovement()->MaxWalkSpeed = PassiveMovementSpeed;
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACarbonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &ACarbonCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACarbonCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ACarbonCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ACarbonCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ACarbonCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ACarbonCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ACarbonCharacter::OnResetVR);

	// Combat input
	PlayerInputComponent->BindAction("CombatModeToggle", IE_Pressed, this, &ACarbonCharacter::ToggleCombatMode);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ACarbonCharacter::Attack);
	PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &ACarbonCharacter::Roll);
	PlayerInputComponent->BindAction("CycleTarget+", IE_Pressed, this, &ACarbonCharacter::CycleTargetClockwise);
	PlayerInputComponent->BindAction("CycleTarget-", IE_Pressed, this, &ACarbonCharacter::CycleTargetCounterClockwise);
}

void ACarbonCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Sphere
	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &ACarbonCharacter::OnSphereBeginOverlap);
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &ACarbonCharacter::OnSphereEndOverlap);
	// Weapon
	Weapon->OnComponentBeginOverlap.AddDynamic(this, &ACarbonCharacter::OnWeaponBeginOverlap);

	// Add any nearby enemies to NearbyEnemies array
	TSet<AActor*> NearActors;
	SphereCollider->GetOverlappingActors(NearActors);
	for (auto& Elem : NearActors)
	{
		// Cast to EnemyBase class to check if they are an enemy
		if (Cast<AEnemyBase>(Elem))
			NearbyEnemies.Add(Elem);
	}
}

void ACarbonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check if target (if any) is still valid,
	FocusTarget();


	if (Rolling)
	{
		AddMovementInput(GetActorForwardVector(), 600 * GetWorld()->GetDeltaSeconds());
		//RollRotateSmooth();
	}

	//* Camera auto-adjustment */
	//FRotator Rotation = Controller->GetControlRotation();
	//FRotator YawRotation(0, Rotation.Yaw, 0);
	//FVector ForwardVector = UKismetMathLibrary::GetForwardVector(YawRotation);
	//FVector RightVector = UKismetMathLibrary::GetRightVector(YawRotation);
	//FVector MovementDirection = ((ForwardVector * InputDirection.X) + (RightVector * InputDirection.Y)).GetSafeNormal();
	//float RotationDifference = UKismetMathLibrary::NormalizedDeltaRotator(MovementDirection.ToOrientationRotator(), Rotation).Yaw;
	//float InputVectorLength = FMath::Clamp(InputDirection.Size(), 0.0f, 1.0f); //FMath::Clamp(FMath::Abs(InputDirection.X) + FMath::Abs(InputDirection.Y), 0.0f, 1.0f);
	//float ScaledDotProduct = FMath::Pow((1 - FMath::Abs(FVector::DotProduct(ForwardVector, MovementDirection))), 5.0f);
	//float YawAmount = FMath::Clamp(DeltaTime * InputVectorLength * ScaledDotProduct, 0.0f, 0.6f) * RotationDifference;
	//AddControllerYawInput(YawAmount);

	//* Camera focus */
	if (Target != NULL && TargetLocked)
	{
		FVector TargetDirection = Target->GetActorLocation() - GetActorLocation();
		if (TargetDirection.Size2D() > 400)
		{
			FRotator Difference = UKismetMathLibrary::NormalizedDeltaRotator(Controller->GetControlRotation(), TargetDirection.ToOrientationRotator());

			if (FMath::Abs(Difference.Yaw) > 30.0f)
				AddControllerYawInput(DeltaTime * -Difference.Yaw * 0.5f);
		}			
	}
}

void ACarbonCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ACarbonCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ACarbonCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ACarbonCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ACarbonCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ACarbonCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && !Attacking && !Rolling)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}

	InputDirection.X = Value;
}

void ACarbonCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) && !Attacking && !Rolling)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}

	InputDirection.Y = Value;
}

void ACarbonCharacter::Attack()
{
	if ((!Attacking || NextAttackReady) && !Rolling && !GetCharacterMovement()->IsFalling())
	{
		Super::Attack();

		// Fringe-case out-of-bounds check
		//		Should not happen due to last attack in array SHOULD
		//      be forced to EndAttack() before the next can be played.
		if (AttackIndex >= Attacks.Num())
			AttackIndex = 0;

		PlayAnimMontage(Attacks[AttackIndex++]);
	}
}

void ACarbonCharacter::EndAttack()
{
	Super::EndAttack();

	AttackIndex = 0;
}

void ACarbonCharacter::Roll()
{
	if (Rolling)
		return;

	// Rotation code based on answer from:
	//		https://www.reddit.com/r/unrealengine/comments/3g3xem/getting_the_world_direction_of_a_players_input/ctumoe1/
	//

	// Snap to face input direction at start of roll
	if (InputDirection != FVector::ZeroVector)
	{
		FRotator PlayerRotZeroPitch = Controller->GetControlRotation();
		PlayerRotZeroPitch.Pitch = 0;
		FVector PlayerRight = FRotationMatrix(PlayerRotZeroPitch).GetUnitAxis(EAxis::Y);
		FVector PlayerForward = FRotationMatrix(PlayerRotZeroPitch).GetUnitAxis(EAxis::X);
		// Scale the forward and right vectors by movementInputDirection
		FVector DodgeDir = PlayerForward * InputDirection.X + PlayerRight * InputDirection.Y;

		RollRotation = DodgeDir.ToOrientationRotator();
	}
	else
		RollRotation = GetActorRotation();

	SetActorRotation(RollRotation);

	PlayAnimMontage(CombatRoll);
	Rolling = true;
}

void ACarbonCharacter::StartRoll()
{
	Rolling = true;

	// Speed up movement speed
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;

	// Inform class that attack has been cancelled
	EndAttack();
}

void ACarbonCharacter::EndRoll()
{
	Rolling = false;
	GetCharacterMovement()->MaxWalkSpeed = TargetLocked ? CombatMovementSpeed : PassiveMovementSpeed;
}

void ACarbonCharacter::RollRotateSmooth()
{
	FRotator SmoothedRotation = FMath::Lerp(GetActorRotation(), RollRotation, RotationSmoothing * GetWorld()->DeltaTimeSeconds);
	SetActorRotation(SmoothedRotation);
}

void ACarbonCharacter::FocusTarget()
{
	// Check if moved too far away from target
	if (Target != NULL)
	{
		if (FVector::Dist(GetActorLocation(), Target->GetActorLocation()) >= TargetLockDistance)
			ToggleCombatMode();
	}
}

void ACarbonCharacter::ToggleCombatMode()
{
	if (!TargetLocked)
	{
		// Attempt to find and lock a target
		CycleTarget();
	}
	else
	{
		SetInCombat(false);
	}
}

void ACarbonCharacter::SetInCombat(bool _InCombat)
{
	TargetLocked = _InCombat;
	GetCharacterMovement()->bOrientRotationToMovement = !TargetLocked;
	GetCharacterMovement()->MaxWalkSpeed = TargetLocked ? CombatMovementSpeed : PassiveMovementSpeed;
	if (!TargetLocked)
		Target = NULL;
}

void ACarbonCharacter::OnWeaponBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Attacking && !AttackHitActors.Contains(OtherActor))
	{
		float AppliedDamage = UGameplayStatics::ApplyDamage(OtherActor, 1.0f, GetController(), this, UDamageType::StaticClass());
		if (AppliedDamage > 0.0f)
		{
			AttackHitActors.Add(OtherActor);

			UCameraShake * CameraShake = UCameraShake::StaticClass()->GetDefaultObject<UCameraShake>();
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->PlayCameraShake(CameraShakeMinor);
		}
	}
}

void ACarbonCharacter::OnSphereBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AEnemyBase>(OtherActor) && !NearbyEnemies.Contains(OtherActor))
		NearbyEnemies.Add(OtherActor);
}

void ACarbonCharacter::OnSphereEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<AEnemyBase>(OtherActor) && NearbyEnemies.Contains(OtherActor))
		NearbyEnemies.Remove(OtherActor);
}

void ACarbonCharacter::CycleTarget(bool Clockwise)
{
	//* Find next target to the left/right to the current one (if any) /

	AActor* SuitableTarget = NULL;
	if (Target)
	{
		// Loop through nearby enemies, comparing yaw difference to find the next closest enemy to the current target
		FVector CameraLocation = Cast<APlayerController>(GetController())->PlayerCameraManager->GetCameraLocation();

		FRotator TargetDirection = (Target->GetActorLocation() - CameraLocation).ToOrientationRotator();
		float BestYawDifference = INFINITY;
		for (auto& Elem : NearbyEnemies)
		{
			if (Elem == Target)
				continue;

			FVector ElemDirection = Elem->GetActorLocation() - CameraLocation;
			FRotator Difference = UKismetMathLibrary::NormalizedDeltaRotator(ElemDirection.ToOrientationRotator(), TargetDirection);

			if ((Clockwise && Difference.Yaw <= 0.0f) || (!Clockwise && Difference.Yaw >= 0.0f))
				continue;

			float YawDifference = FMath::Abs(Difference.Yaw);
			if (YawDifference < BestYawDifference)
			{
				BestYawDifference = YawDifference;
				SuitableTarget = Elem;
			}
		}
	}

	//* Find closest enemy if no existing target to cycle from
	else
	{
		float BestDistance = INFINITY;
		for (auto& Elem : NearbyEnemies)
		{
			float Distance = FVector::Dist(GetActorLocation(), Elem->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				SuitableTarget = Elem;
			}
		}
	}
	

	if (SuitableTarget != NULL)
	{
		Target = SuitableTarget;

		// If not in combat but (successfully) attempted to switch target, put into combat mode
		if (!TargetLocked)
		{
			SetInCombat(true);
		}
	}		
}

void ACarbonCharacter::CycleTargetClockwise()
{
	CycleTarget(true);
}

void ACarbonCharacter::CycleTargetCounterClockwise()
{
	CycleTarget(false);
}

void ACarbonCharacter::LookAtSmooth()
{
	// Add !Rolling condition to LookAtSmooth() function
	if (!Rolling)
		Super::LookAtSmooth();
}

float ACarbonCharacter::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	// DEFAULT:
	//		Cancel current attack
	//		Play random stumble animation
	//		Rotate towards damage source

	if (DamageCauser == this || Attacking)
		return 0.0f;

	EndAttack();
	SetMovingBackwards(false);
	SetMovingForward(false);
	Stumbling = true;

	// Play random stumble animation from array - Does not repeat last animation used
	int AnimationIndex;
	do { AnimationIndex = FMath::RandRange(0, TakeHit_StumbleBackwards.Num() - 1); } while (AnimationIndex == LastStumbleIndex);

	PlayAnimMontage(TakeHit_StumbleBackwards[AnimationIndex]);
	LastStumbleIndex = AnimationIndex;


	// Rotate towards source of damage
	FVector Direction = DamageCauser->GetActorLocation() - GetActorLocation();
	Direction = FVector(Direction.X, Direction.Y, 0);
	FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();
	SetActorRotation(Rotation);

	return DamageAmount;
}
