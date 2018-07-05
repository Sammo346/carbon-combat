// Sam Smith

#include "EnemyBase.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AEnemyBase::AEnemyBase()
{
	// Create weapon
	Weapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), "RightHandItem");
	Weapon->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MovingForward = false;
	Attacking = false;
	Interruptable = true;
	LastStumbleIndex = 0;
}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	ActiveState = State::IDLE;

	//* Temporary NPC target solution */
	Target = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickStateMachine();
}

void AEnemyBase::TickStateMachine()
{
	switch (ActiveState)
	{
		case State::IDLE:
			StateIdle();
			break;
		case State::CHASE_CLOSE:
			StateChaseClose();
			break;
		case State::CHASE_FAR:
			StateChaseFar();
			break;
		case State::ATTACK:
			StateAttack();
			break;
		case State::STUMBLE:
			StateStumble();
			break;
		case State::DEAD:
			StateDead();
			break;
	}
}

void AEnemyBase::SetState(State NewState)
{
	if (ActiveState != State::DEAD)
		ActiveState = NewState;
}

void AEnemyBase::StateIdle()
{
	//* Temporary 'target sensing' implementation */
	// Check if player within distance
	if (Target && FVector::Distance(Target->GetActorLocation(), GetActorLocation()) <= 1200.0f)
	{
		TargetLocked = true;

		SetState(State::CHASE_CLOSE);
	}
}

void AEnemyBase::StateChaseClose()
{
	// DEFAULT:
	//		Attack target when close,
	//		otherwise move towards target

	if (Target && !Attacking && !Stumbling)
	{
		float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());

		// CLOSE ENOUGH TO ATTACK
		if (Distance <= 300.0f)
		{
			// Attack if looking towards target
			FVector TargetDirection = Target->GetActorLocation() - GetActorLocation();
			float DotProduct = FVector::DotProduct(GetActorForwardVector(), TargetDirection.GetSafeNormal());

			if (DotProduct >= 0.95f && !Attacking && !Stumbling)
				Attack(false);
		}
		else
		{
			// Move towards target
			AAIController* AIController = Cast<AAIController>(Controller);
			if (!AIController->IsFollowingAPath())
			{
				AIController->MoveToActor(Target);
			}
		}
	}
}

void AEnemyBase::StateChaseFar()
{
	// DEFAULT:
	//		Idle behaviour until player comes within range

	if (FVector::Distance(GetActorLocation(), Target->GetActorLocation()) < 850.0f)
	{
		SetState(State::CHASE_CLOSE);
	}
}

void AEnemyBase::StateAttack()
{
	// DEFAULT:
	//		Move forward if active attack needs is
	//		More advanced implementations may make use of the 'AttackReady' bool to string attacks

	// Check if weapon overlapping other actors
	if (AttackDamaging)
	{
		TSet<AActor*> OverlappingActors;
		Weapon->GetOverlappingActors(OverlappingActors);

		for (AActor* OtherActor : OverlappingActors)
		{
			if (OtherActor == this)
				continue;

			if (!AttackHitActors.Contains(OtherActor))
			{
				float AppliedDamage = UGameplayStatics::ApplyDamage(OtherActor, 1.0f, GetController(), this, UDamageType::StaticClass());
				if (AppliedDamage > 0.0f)
				{
					AttackHitActors.Add(OtherActor);
				}
			}
		}
	}

	if (MovingForward)
		MoveForward();
}

void AEnemyBase::StateStumble()
{
	if (Stumbling)
	{
		if (MovingBackwards)
			AddMovementInput(-GetActorForwardVector(), 40.0f * GetWorld()->GetDeltaSeconds());
	}
	else
		SetState(State::CHASE_CLOSE);		
}

void AEnemyBase::StateTaunt()
{

}

void AEnemyBase::StateDead()
{

}

// Called to bind functionality to input
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyBase::FocusTarget()
{
	AAIController* Control = Cast<AAIController>(GetController());
	Control->SetFocus(Target);
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	// DEFAULT:
	//		Cancel current attack
	//		Play random stumble animation
	//		Rotate towards damage source

	if (DamageCauser == this)
		return 0.0f;

	//* TODO: Remove health *//

	// Don't stumble if not interruptable (still take damage though)
	if (!Interruptable)
		return DamageAmount;

	// Cancel any existing states, and prepare for stumble
	EndAttack();
	SetMovingBackwards(false);
	SetMovingForward(false);
	Stumbling = true;
	SetState(State::STUMBLE);
	Cast<AAIController>(Controller)->StopMovement();

	// Play random stumble animation from array - Does not repeat last animation used
	int AnimationIndex;
	do { AnimationIndex = FMath::RandRange(0, TakeHit_StumbleBackwards.Num() - 1); }
	while (AnimationIndex == LastStumbleIndex);

	PlayAnimMontage(TakeHit_StumbleBackwards[AnimationIndex]);
	LastStumbleIndex = AnimationIndex;


	// Rotate towards source of damage
	FVector Direction = DamageCauser->GetActorLocation() - GetActorLocation();
	Direction = FVector(Direction.X, Direction.Y, 0);
	FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();
	SetActorRotation(Rotation);

	return DamageAmount;
}


void AEnemyBase::MoveForward()
{
	FVector NewLocation = GetActorLocation() + (GetActorForwardVector() * 500.0f * GetWorld()->GetDeltaSeconds());
	SetActorLocation(NewLocation, true);
}

void AEnemyBase::Attack(bool Rotate)
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

	int RandomIndex = FMath::RandRange(0, AttackAnimations.Num() - 1);
	PlayAnimMontage(AttackAnimations[RandomIndex]);
}

void AEnemyBase::AttackNextReady()
{
	Super::AttackNextReady();
}

void AEnemyBase::EndAttack()
{
	Super::EndAttack();
	SetState(State::CHASE_CLOSE);
}

void AEnemyBase::AttackLunge()
{
	Super::AttackLunge();
}


