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

protected:
	
	void StateChaseClose();

	void LongAttack(bool Rotate = true);

	void MoveForward();

private:
	UPROPERTY(EditAnywhere, Category = "Combat")
	float LongAttackCooldown;
	float LongAttackTimestamp;
	float LongAttackForwardSpeed;
};
