// Sam Smith

#include "EnemyKnight.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyKnight::AEnemyKnight()
{
	LongAttackCooldown = 5.0f;
	LongAttackTimestamp = -LongAttackCooldown;
}

void AEnemyKnight::StateChaseClose()
{
	// KNIGHT:
	//		Long range attack || short range attack || move closer

	if (Target && !Attacking && !Stumbling)
	{
		float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
		AAIController* AIController = Cast<AAIController>(Controller);

		// Attack if looking towards target
		FVector TargetDirection = Target->GetActorLocation() - GetActorLocation();
		float DotProduct = FVector::DotProduct(GetActorForwardVector(), TargetDirection.GetSafeNormal());
		// Close enough for an attack
		if (Distance <= 900.0f && DotProduct >= 0.95f)
		{
			if (Distance <= 300.0f)
			{
				Attack(false);
				return;
			}
			else if (UGameplayStatics::GetTimeSeconds(GetWorld()) >= LongAttackTimestamp + LongAttackCooldown
				&& AIController->LineOfSightTo(Target))
			{
				LongAttackTimestamp = UGameplayStatics::GetTimeSeconds(GetWorld());
				LongAttack(true);
				return;
			}
		}
		// Move towards target
		if (!AIController->IsFollowingAPath())
		{
			AIController->MoveToActor(Target);
		}
	}
}

void AEnemyKnight::LongAttack(bool Rotate)
{
	Super::Attack();
	SetMovingBackwards(false);
	SetMovingForward(false);
	SetState(State::ATTACK);
	Cast<AAIController>(Controller)->StopMovement();

	// Rotate towards target
	if (Rotate)
	{
		FVector Direction = Target->GetActorLocation() - GetActorLocation();
		Direction = FVector(Direction.X, Direction.Y, 0);
		FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();
		SetActorRotation(Rotation);
	}

	// Calculate speed of jump (based on distance to player at *start* of jump)
	float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
	LongAttackForwardSpeed = Distance + 600.0f;

	// Play attack animation
	int RandomIndex = FMath::RandRange(0, LongAttackAnimations.Num() - 1);
	PlayAnimMontage(LongAttackAnimations[RandomIndex]);
}

void AEnemyKnight::MoveForward()
{
	FVector NewLocation = GetActorLocation() + (GetActorForwardVector() * LongAttackForwardSpeed * GetWorld()->GetDeltaSeconds());
	SetActorLocation(NewLocation);
}
