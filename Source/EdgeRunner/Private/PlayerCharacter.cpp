// Fill out your copyright notice in the Description page of Project Settings.
#include "PlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"


APlayerCharacter::APlayerCharacter() :
MoveSpeed(20.0f),
LookSpeed(1.5f),
MaxDashDistanceVel(1500.0f),
MaxDashLimitingAngle(35.0f),
DashPredictionFrequency(20.0f),
MaxDashPredictionHeight(200.0f),
bShowDashPredictionLocation(false)
{
	PrimaryActorTick.bCanEverTick = true;
	
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(GetMesh(), FName("Head"));

	Camera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 90.0f));
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll= false;
	bUseControllerRotationYaw = true;
	Camera->bUsePawnControlRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bShowDashPredictionLocation)
	{
		FTransform center =  CalculateDashLocation(Camera->GetComponentLocation(), 1000.0f);
		/*if(DashPredictorMarker)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DashPredictorMarker, center.GetLocation(), center.GetRotation().Rotator() + FRotator(90, 0, 0));
		}*/

		DrawDebugSphere(GetWorld(), center.GetLocation(), 15.0f, 10.0f, FColor::Green);
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(FName("MoveForward"), this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(FName("MoveSideWays"), this, &APlayerCharacter::MoveSideWays);
	PlayerInputComponent->BindAxis(FName("Turn"), this, &APlayerCharacter::Turn);
	PlayerInputComponent->BindAxis(FName("LookUp"), this, &APlayerCharacter::LookUp);

	PlayerInputComponent->BindAction(FName("Dash"), EInputEvent::IE_Pressed, this, &APlayerCharacter::DashPressed);
	PlayerInputComponent->BindAction(FName("Dash"), EInputEvent::IE_Released, this, &APlayerCharacter::DashReleased);

}

void APlayerCharacter::MoveForward(float Value)
{
	if(Controller && Value!=0)
	{
		const FRotator YawRotation = {0.0f, Controller->GetControlRotation().Yaw, 0.0f};
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value*MoveSpeed);
	}
}

void APlayerCharacter::MoveSideWays(float Value)
{
	if(Controller && Value!=0)
	{
		const FRotator YawRotation = {0.0f, Controller->GetControlRotation().Yaw, 0.0f};
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value*MoveSpeed);
	}
}

void APlayerCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value*LookSpeed);
}

void APlayerCharacter::Turn(float Value)
{
	AddControllerYawInput(Value*LookSpeed);
}

void APlayerCharacter::DashPressed()
{
	bShowDashPredictionLocation = true;
}

void APlayerCharacter::DashReleased()
{
	bShowDashPredictionLocation = false;
}


FTransform APlayerCharacter::CalculateDashLocation(FVector CurrentLocation, float MaxDistance)
{
	FTransform DashLocation;
	FPredictProjectilePathParams Params;
	FPredictProjectilePathResult Result;

	Params.StartLocation = CurrentLocation +  Camera->GetForwardVector()*DashPredictionFrequency;
	Params.LaunchVelocity = Camera->GetForwardVector()*MaxDashDistanceVel;
	Params.ProjectileRadius = 5.0f;
	Params.TraceChannel = ECC_Visibility;
	Params.bTraceWithCollision = true;
	
	bool hit = UGameplayStatics::PredictProjectilePath(GetWorld(), Params, Result);
	if(hit)
	{
		HitLocations Hit;
		if(GetHitAngle(Result.HitResult.ImpactNormal)>=MaxDashLimitingAngle)
		{
			PredictionTrace(Result.HitResult, ELineTrace::ELT_Both, Hit);
			float NextDistance = FVector::Distance(Result.HitResult.Location, Hit.NextLoc);
			float PrevDistance = FVector::Distance(Result.HitResult.Location, Hit.PrevLoc);

			DashLocation.SetLocation(NextDistance<PrevDistance ? Hit.NextLoc : Hit.PrevLoc);
			DashLocation.SetRotation(NextDistance<PrevDistance ? Hit.NextRot.Quaternion() : Hit.PrevRot.Quaternion());
		}
		else
		{
			DashLocation.SetLocation(Result.HitResult.Location);
			DashLocation.SetRotation(Result.HitResult.ImpactNormal.Rotation().Quaternion());
			DrawDebugSphere(GetWorld(), Result.HitResult.Location, 15.0f, 10.0f, FColor::Red);
			return DashLocation;
		}
	}
	
	return DashLocation;
}

float APlayerCharacter::GetHitAngle(FVector HitPoint) const
{
	const float dot = FVector::DotProduct(HitPoint, FVector(0,0,1));
	const float angle = UKismetMathLibrary::DegAcos(dot/(HitPoint.Size()));

	return angle;
}

void APlayerCharacter::PredictionTrace(FHitResult Result, ELineTrace LineTrace, HitLocations &Hit)
{
	if(LineTrace == ELineTrace::ELT_Next || LineTrace == ELineTrace::ELT_Both)
	{
		FHitResult NextHitResult;
		FVector NextStartLocation = Result.Location + Camera->GetForwardVector()*DashPredictionFrequency + FVector(0,0,1)*MaxDashPredictionHeight;
		FVector NextEndLocation = NextStartLocation + FVector(0,0,-1)*500.0f;
		
		GetWorld()->LineTraceSingleByChannel(NextHitResult, NextStartLocation, NextEndLocation, ECC_Visibility);

		if(NextHitResult.bBlockingHit)
		{
			if(GetHitAngle(NextHitResult.ImpactNormal)>MaxDashLimitingAngle)
			{
				PredictionTrace(NextHitResult, ELineTrace::ELT_Next, Hit);
			}
			else
			{
				Hit.NextLoc = NextHitResult.Location;
				Hit.NextRot = NextHitResult.ImpactNormal.Rotation();
			}
		}

	}
	if(LineTrace == ELineTrace::ELT_Prev || LineTrace == ELineTrace::ELT_Both)
	{
		FHitResult PrevHitResult;
		FVector PrevStartLocation = Result.Location - Camera->GetForwardVector()*DashPredictionFrequency + FVector(0,0,1)*MaxDashPredictionHeight;
		FVector PrevEndLocation = PrevStartLocation + FVector(0,0,-1)*500.0f;
		
		GetWorld()->LineTraceSingleByChannel(PrevHitResult, PrevStartLocation, PrevEndLocation, ECC_Visibility);
		if(PrevHitResult.bBlockingHit)
		{
			if(GetHitAngle(PrevHitResult.ImpactNormal)>MaxDashLimitingAngle)
			{
				PredictionTrace(PrevHitResult, ELineTrace::ELT_Prev, Hit);
			}
			else
			{
				Hit.PrevLoc = PrevHitResult.Location;
				Hit.PrevRot = PrevHitResult.ImpactNormal.Rotation();
			}
		}
	}
}

