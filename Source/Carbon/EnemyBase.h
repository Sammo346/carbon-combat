// Sam Smith

#pragma once

#include "CoreMinimal.h"
#include "Combatant.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

UENUM(BlueprintType)
enum class State : uint8
{
	IDLE,					// Outside of combat
	CHASE_CLOSE,			// Combat, staying close to target
	CHASE_FAR,				// Combat, doesn't care about range
	ATTACK,					// In the process of attacking
	STUMBLE,				// Stumbling from being damaged/interrupted
	TAUNT,					// Emoting during a fight
	DEAD					// Dead
};

UCLASS()
class CARBON_API AEnemyBase : public ACombatant
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Weapon;

public:
	// Sets default values for this character's properties
	AEnemyBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Finite State Machine")
	State ActiveState;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, class AController * EventInstigator, AActor * DamageCauser);

	UPROPERTY(EditAnywhere, Category = "Animations")
	UAnimMontage * OverheadSmash;

	int LastStumbleIndex;

	// Not implemented movement speed variables yet
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
		float ChaseFarMovementSpeed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
		float ChaseCloseMovementSpeed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
		float PassiveMovementSpeed;*/

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void TickStateMachine();

	void SetState(State NewState);

	virtual void StateIdle();

	// State: Actively trying to keep close and attack the target
	virtual void StateChaseClose();

	// State: Engaged but not currently trying to attack (idle behaviour)
	virtual void StateChaseFar();

	virtual void StateAttack();

	virtual void StateStumble();

	virtual void StateTaunt();

	virtual void StateDead();

	virtual void MoveForward();

	virtual void Attack(bool Rotate = true);

	void AttackNextReady();

	void EndAttack();

	virtual void AttackLunge();

	void OnWeaponBeginOverlap(UPrimitiveComponent * OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	bool Interruptable;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void FocusTarget();

	
	
};
