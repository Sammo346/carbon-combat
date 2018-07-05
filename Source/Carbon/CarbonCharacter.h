// Sam Smith  - Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combatant.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "CarbonCharacter.generated.h"

UCLASS(config=Game)
class ACarbonCharacter : public ACombatant
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	USphereComponent* SphereCollider;

public:
	ACarbonCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float PassiveMovementSpeed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float CombatMovementSpeed;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent * OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	void OnSphereEndOverlap(class UPrimitiveComponent* OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex);

	void CycleTarget(bool Clockwise = true);

	UFUNCTION()
	void CycleTargetClockwise();
	UFUNCTION()
	void CycleTargetCounterClockwise();

	void LookAtSmooth();

	float TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser);

	UPROPERTY(EditAnywhere, Category="Animations")
	TArray<UAnimMontage*> Attacks;

	UPROPERTY(EditAnywhere, Category = "Animations")
	UAnimMontage * CombatRoll;

	UPROPERTY(EditAnywhere, Category = Camera)
	TSubclassOf<UCameraShake> CameraShakeMinor;

	bool Rolling;
	FRotator RollRotation;
	int AttackIndex;
	float TargetLockDistance;
	TArray<AActor*> NearbyEnemies;
	int LastStumbleIndex;

	FVector InputDirection;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** Called to attempt beginning an attack */
	void Attack();

	/** Called by Anim to signal the damaging section of an attack has ended */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndAttack();

	void Roll();

	/** Called by Anim to signal the damaging section of an attack has started */
	UFUNCTION(BlueprintCallable, Category = "Combat")
		void StartRoll();

	/** Called by Anim to signal the damaging section of an attack has ended */
	UFUNCTION(BlueprintCallable, Category = "Combat")
		void EndRoll();

	void RollRotateSmooth();

	void FocusTarget();

	void ToggleCombatMode();

	void SetInCombat(bool _InCombat);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	/** Returns Weapon subobject **/
	FORCEINLINE class UStaticMeshComponent* GetWeapon() const { return Weapon; }
};

