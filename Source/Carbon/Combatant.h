// Sam Smith

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combatant.generated.h"

UCLASS()
class CARBON_API ACombatant : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACombatant();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	AActor * Target;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool TargetLocked;

	bool Attacking;
	bool AttackDamaging;
	bool MovingForward;
	bool MovingBackwards;
	bool NextAttackReady;
	bool Stumbling;

	bool RotateTowardsTarget;
	UPROPERTY(EditAnywhere, Category = "Animation")
	float RotationSmoothing;

	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> AttackAnimations;

	UPROPERTY(EditAnywhere, Category = "Animations")
		TArray<UAnimMontage*> TakeHit_StumbleBackwards;

	/* Actors hit with the last attack - Used to stop duplicate hits*/
	TArray<AActor*> AttackHitActors;

	virtual void Attack();

	/** Anim called: Rotate and jump towards target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AttackLunge();

	/** Anim called: Attack finished, is free to change state */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void EndAttack();

	/** Set if weapon applies damage */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetAttackDamaging(bool Damaging);

	/** Anim called: Set if moving forward*/
	UFUNCTION(BlueprintCallable, Category = "Animation")
	virtual void SetMovingForward(bool IsMovingForward);

	/** Anim called: Set if moving backwards */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	virtual void SetMovingBackwards(bool IsMovingBackwards);

	/** Anim called: Set if moving backwards */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	virtual void EndStumble();

	/** Called by Anim to signal that the next attack is potentially allowed */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void AttackNextReady();

	virtual void LookAtSmooth();

	/** Anim called: Get rate of actor's look rotation */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	float GetCurrentRotationSpeed();
	float LastRotationSpeed;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
