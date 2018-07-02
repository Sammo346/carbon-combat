// Sam Smith

#include "Combatant.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
ACombatant::ACombatant()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TargetLocked = false;
	NextAttackReady = false;
	Attacking = false;
	AttackDamaging = false;
	MovingForward = false;
	MovingBackwards = false;
	RotateTowardsTarget = true;
	Stumbling = false;
	RotationSmoothing = 5.0f;
	LastRotationSpeed = 0.0f;
}

// Called when the game starts or when spawned
void ACombatant::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACombatant::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Look towards target
	if (RotateTowardsTarget)
		LookAtSmooth();

}

// Called to bind functionality to input
void ACombatant::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ACombatant::Attack()
{
	Attacking = true;
	NextAttackReady = false;
	AttackDamaging = false;
	AttackHitActors.Empty();
}

void ACombatant::AttackLunge()
{
	// Look at target
	if (Target != NULL)
	{
		FVector Direction = Target->GetActorLocation() - GetActorLocation();
		Direction = FVector(Direction.X, Direction.Y, 0);
		FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();
		SetActorRotation(Rotation);
	}

	// Teleport forward
	FVector NewLocation = GetActorLocation() + (GetActorForwardVector() * 70);
	SetActorLocation(NewLocation, true);
}

void ACombatant::EndAttack()
{
	Attacking = false;
	NextAttackReady = false;
}

void ACombatant::SetAttackDamaging(bool Damaging)
{
	AttackDamaging = Damaging;
}

void ACombatant::SetMovingForward(bool IsMovingForward)
{
	MovingForward = IsMovingForward;
}

void ACombatant::SetMovingBackwards(bool IsMovingBackwards)
{
	MovingBackwards = IsMovingBackwards;
}

void ACombatant::EndStumble()
{
	Stumbling = false;
}

void ACombatant::AttackNextReady()
{
	NextAttackReady = true;
}

void ACombatant::LookAtSmooth()
{
	// Smoothly rotate towards target
	if (Target != NULL && TargetLocked && !Attacking && !GetCharacterMovement()->IsFalling())
	{
		FVector Direction = Target->GetActorLocation() - GetActorLocation();
		Direction = FVector(Direction.X, Direction.Y, 0);
		FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();

		FRotator SmoothedRotation = FMath::Lerp(GetActorRotation(), Rotation, RotationSmoothing * GetWorld()->DeltaTimeSeconds);
		// Save yaw difference to variable (for anim)
		LastRotationSpeed = SmoothedRotation.Yaw - GetActorRotation().Yaw;
		SetActorRotation(SmoothedRotation);
	}
}

float ACombatant::GetCurrentRotationSpeed()
{
	if (RotateTowardsTarget)
	{
		return LastRotationSpeed;
	}
	else
		return 0.0f;
}

