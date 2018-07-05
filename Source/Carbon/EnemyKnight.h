// Sam Smith

#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EnemyKnight.generated.h"

/**
 * 
 */
UCLASS()
class CARBON_API AEnemyKnight : public AEnemyBase
{
	GENERATED_BODY()
	
public:
	AEnemyKnight();

	UPROPERTY(EditAnywhere, Category = "Animations")
	TArray<UAnimMontage*> LongAttackAnimations;

	float TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser);

protected:
	
	void StateChaseClose();

	void LongAttack(bool Rotate = true);

	void MoveForward();

private:
	// Long-range jump attack
	UPROPERTY(EditAnywhere, Category = "Combat")
	float LongAttackCooldown;
	float LongAttackTimestamp;
	float LongAttackForwardSpeed;

	// After x consecutive hits, the knight cannot be interrupted 
	int QuickHitsTaken;
	float QuickHitsTimestamp;
};
