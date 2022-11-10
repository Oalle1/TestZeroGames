// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestZeroGamesPawn.h"
#include "TestZeroGamesWheelFront.h"
#include "TestZeroGamesWheelRear.h"
#include "TestZeroGamesHud.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "Math/UnrealMathVectorCommon.h"
#include "Kismet/KismetMathLibrary.h"
#include <chrono>
#include "GameFramework/Controller.h"

#ifndef HMD_MODULE_INCLUDED
#define HMD_MODULE_INCLUDED 0
#endif

// Needed for VR Headset
#if HMD_MODULE_INCLUDED
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#endif // HMD_MODULE_INCLUDED

const FName ATestZeroGamesPawn::LookUpBinding("LookUp");
const FName ATestZeroGamesPawn::LookRightBinding("LookRight");

#define LOCTEXT_NAMESPACE "VehiclePawn"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

ATestZeroGamesPawn::ATestZeroGamesPawn()
{
	// Car mesh
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(TEXT("/Game/Vehicle/Sedan/Sedan_SkelMesh.Sedan_SkelMesh"));
	GetMesh()->SetSkeletalMesh(CarMesh.Object);

	static ConstructorHelpers::FClassFinder<UObject> AnimBPClass(TEXT("/Game/Vehicle/Sedan/Sedan_AnimBP"));
	GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);
	
	// Simulation
	UWheeledVehicleMovementComponent4W* Vehicle4W = CastChecked<UWheeledVehicleMovementComponent4W>(GetVehicleMovement());

	check(Vehicle4W->WheelSetups.Num() == 4);

	Vehicle4W->WheelSetups[0].WheelClass = UTestZeroGamesWheelFront::StaticClass();
	Vehicle4W->WheelSetups[0].BoneName = FName("Wheel_Front_Left");
	Vehicle4W->WheelSetups[0].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[1].WheelClass = UTestZeroGamesWheelFront::StaticClass();
	Vehicle4W->WheelSetups[1].BoneName = FName("Wheel_Front_Right");
	Vehicle4W->WheelSetups[1].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	Vehicle4W->WheelSetups[2].WheelClass = UTestZeroGamesWheelRear::StaticClass();
	Vehicle4W->WheelSetups[2].BoneName = FName("Wheel_Rear_Left");
	Vehicle4W->WheelSetups[2].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[3].WheelClass = UTestZeroGamesWheelRear::StaticClass();
	Vehicle4W->WheelSetups[3].BoneName = FName("Wheel_Rear_Right");
	Vehicle4W->WheelSetups[3].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->TargetOffset = FVector(0.f, 0.f, 200.f);
	SpringArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 7.f;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 45.f;

	


	// Create In-Car camera component 
	InternalCameraOrigin = FVector(0.0f, -40.0f, 120.0f);

	InternalCameraBase = CreateDefaultSubobject<USceneComponent>(TEXT("InternalCameraBase"));
	InternalCameraBase->SetRelativeLocation(InternalCameraOrigin);
	InternalCameraBase->SetupAttachment(GetMesh());

	InternalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InternalCamera"));
	InternalCamera->bUsePawnControlRotation = false;
	InternalCamera->FieldOfView = 90.f;
	InternalCamera->SetupAttachment(InternalCameraBase);

	TransitionCameraBase = CreateDefaultSubobject<USceneComponent>(TEXT("TransitionCameraBase"));
	TransitionCameraBase->SetRelativeLocation(InternalCameraBase->GetRelativeLocation());
	TransitionCameraBase->SetupAttachment(GetMesh());

	TransitionCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraTransition"));
	TransitionCamera->bUsePawnControlRotation = false;
	TransitionCamera->FieldOfView = 90.f;
	TransitionCamera->SetupAttachment(TransitionCameraBase);

	//Setup TextRenderMaterial
	static ConstructorHelpers::FObjectFinder<UMaterial> TextMaterial(TEXT("Material'/Engine/EngineMaterials/AntiAliasedTextMaterialTranslucent.AntiAliasedTextMaterialTranslucent'"));
	
	UMaterialInterface* Material = TextMaterial.Object;
	
	// Colors for the incar gear display. One for normal one for reverse
	GearDisplayReverseColor = FColor(255, 0, 0, 255);
	GearDisplayColor = FColor(255, 255, 255, 255);

	// Colors for the in-car gear display. One for normal one for reverse
	GearDisplayReverseColor = FColor(255, 0, 0, 255);
	GearDisplayColor = FColor(255, 255, 255, 255);

	bInReverseGear = false;
	PrimaryActorTick.bCanEverTick = true;
}

void ATestZeroGamesPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATestZeroGamesPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATestZeroGamesPawn::MoveRight);
	PlayerInputComponent->BindAxis("LookUp");
	PlayerInputComponent->BindAxis("LookRight");

	PlayerInputComponent->BindAction("Handbrake", IE_Pressed, this, &ATestZeroGamesPawn::OnHandbrakePressed);
	PlayerInputComponent->BindAction("Handbrake", IE_Released, this, &ATestZeroGamesPawn::OnHandbrakeReleased);
	PlayerInputComponent->BindAction("SwitchCamera", IE_Pressed, this, &ATestZeroGamesPawn::OnToggleCamera);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ATestZeroGamesPawn::OnResetVR); 
}

void ATestZeroGamesPawn::MoveForward(float Val)
{
	GetVehicleMovementComponent()->SetThrottleInput(Val);
}

void ATestZeroGamesPawn::MoveRight(float Val)
{
	GetVehicleMovementComponent()->SetSteeringInput(Val);
}

void ATestZeroGamesPawn::OnHandbrakePressed()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void ATestZeroGamesPawn::OnHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

void ATestZeroGamesPawn::OnToggleCamera()
{
	bToggle = true;
}

void ATestZeroGamesPawn::EnableIncarView(const bool bState, const bool bForce)
{
	if ((bState != bInCarCameraActive) || ( bForce == true ))
	{
		bInCarCameraActive = bState;
		FVector LocationTemp;
		FRotator RotationTemp;
		if (bState == true)
		{
			OnResetVR();
			Camera->Deactivate();
			TransitionCamera->Deactivate();
			InternalCamera->Activate();
		}
		else
		{
			InternalCamera->Deactivate();
			TransitionCamera->Deactivate();
			Camera->Activate();
		}
	}
}

void ATestZeroGamesPawn::MoveCamera()
{
	float Elapsed;
	if (DeltaTemp + time > FPlatformTime::Seconds())
	{
		Elapsed = FPlatformTime::Seconds() - DeltaTemp;
		if (bInCarCameraActive == false)
		{
			TransitionCamera->SetWorldLocation(Camera->GetComponentLocation() - (Camera->GetComponentLocation() - InternalCamera->GetComponentLocation()) * (Elapsed / time));
			TransitionCamera->SetWorldRotation(Camera->GetComponentRotation() - (Camera->GetComponentRotation() - InternalCamera->GetComponentRotation()) * (Elapsed / time));
			TransitionCamera->SetFieldOfView(Camera->FieldOfView - (Camera->FieldOfView - InternalCamera->FieldOfView)*(Elapsed/time));
		}
		else
		{
			TransitionCamera->SetWorldLocation(InternalCamera->GetComponentLocation() - (InternalCamera->GetComponentLocation() - Camera->GetComponentLocation()) * (Elapsed / time));
			TransitionCamera->SetWorldRotation(InternalCamera->GetComponentRotation() - (InternalCamera->GetComponentRotation() - Camera->GetComponentRotation()) * (Elapsed / time));
			TransitionCamera->SetFieldOfView(InternalCamera->FieldOfView - (InternalCamera->FieldOfView - Camera->FieldOfView) * (Elapsed / time));
		}
	}
	else if (bInCarCameraActive == false)
	{
		bInCarCameraActive = true;
		bToggle = false;
		bInitDelta = false;
		TransitionCamera->Deactivate();
		InternalCamera->Activate();
	}
	else
	{
		bInCarCameraActive = false;
		bToggle = false;
		bInitDelta = false;
		TransitionCamera->Deactivate();
		Camera->Activate();
	}
}

void ATestZeroGamesPawn::InitDeltaTemp()
{
	if (!bInitDelta)
	{
		DeltaTemp = FPlatformTime::Seconds();
		bInitDelta = true;
	}
}

void ATestZeroGamesPawn::Tick(float Delta)
{
	Super::Tick(Delta);
	
	// Setup the flag to say we are in reverse gear
	bInReverseGear = GetVehicleMovement()->GetCurrentGear() < 0;
	if (bToggle == true)
	{
		InitDeltaTemp();
		if(Camera->IsActive())
			Camera->Deactivate();
		if(InternalCamera->IsActive())
			InternalCamera->Deactivate();
		if(!TransitionCamera->IsActive())
			TransitionCamera->Activate();
		MoveCamera();
	}

	bool bHMDActive = false;
#if HMD_MODULE_INCLUDED
	if ((GEngine->XRSystem.IsValid() == true) && ((GEngine->XRSystem->IsHeadTrackingAllowed() == true) || (GEngine->IsStereoscopic3D() == true)))
	{
		bHMDActive = true;
	}
#endif // HMD_MODULE_INCLUDED
	if (bHMDActive == false)
	{
		if ( (InputComponent) && (bInCarCameraActive == true ))
		{
			FRotator HeadRotation = InternalCamera->GetRelativeRotation();
			HeadRotation.Pitch += InputComponent->GetAxisValue(LookUpBinding);
			HeadRotation.Yaw += InputComponent->GetAxisValue(LookRightBinding);
			InternalCamera->SetRelativeRotation(HeadRotation);
		}
	}
}

void ATestZeroGamesPawn::BeginPlay()
{
	Super::BeginPlay();

	bool bEnableInCar = false;
	bToggle = false;
#if HMD_MODULE_INCLUDED
	bEnableInCar = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
#endif // HMD_MODULE_INCLUDED
	EnableIncarView(bEnableInCar,true);
}

void ATestZeroGamesPawn::OnResetVR()
{
#if HMD_MODULE_INCLUDED
	if (GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->ResetOrientationAndPosition();
		InternalCamera->SetRelativeLocation(InternalCameraOrigin);
		GetController()->SetControlRotation(FRotator());
	}
#endif // HMD_MODULE_INCLUDED
}

#undef LOCTEXT_NAMESPACE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
