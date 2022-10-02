// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "NiagaraComponent.h"
#include "PlayerCharacter.generated.h"

enum class ELineTrace : uint8
{
	ELT_Both,
	ELT_Prev,
	ELT_Next
};

UCLASS()
class EDGERUNNER_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	APlayerCharacter();

protected:
	
	virtual void BeginPlay() override;

public:	

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LookSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float MaxDashDistanceVel = 1500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float MaxDashLimitingAngle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float DashPredictionFrequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float MaxDashPredictionHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	bool bShowDashPredictionLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	UNiagaraSystem* DashPredictorMarker;
	
private:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

protected:

	void MoveSideWays(float Value);
	void MoveForward(float Value);
	void LookUp(float Value);
	void Turn(float Value);
	void DashPressed();
	void DashReleased();

	struct HitLocations
	{
		FVector NextLoc;
		FRotator NextRot;
		FVector PrevLoc;
		FRotator PrevRot;
	};

	float GetHitAngle(FVector HitPoint) const;
	void PredictionTrace(FHitResult Result, ELineTrace LineTrace, HitLocations &Hit);

	

public:

	FTransform CalculateDashLocation(FVector CurrentLocation, float MaxDistance);
	
};