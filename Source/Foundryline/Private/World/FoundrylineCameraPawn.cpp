#include "World/FoundrylineCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UI/FoundrylinePlayerController.h"
#include "Logistics/FoundrylineFreightRig.h"
#include "Logistics/FoundrylineMaterialHandlingCell.h"

AFoundrylineCameraPawn::AFoundrylineCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
 bOnlyRelevantToOwner = true;
 SetReplicateMovement(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("CameraOrigin")));
	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	Arm->SetupAttachment(RootComponent);
	Arm->TargetArmLength = TargetZoomCm;
	Arm->SetRelativeRotation(FRotator(-55.f, 45.f, 0.f));
	Arm->bDoCollisionTest = false;
	Arm->bUsePawnControlRotation = false;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Arm, USpringArmComponent::SocketName);
	Camera->FieldOfView = 50.f;
	bUseControllerRotationYaw = false;
}

void AFoundrylineCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	MinZoomCm = FMath::Max(MinZoomCm, 100.f);
	MaxZoomCm = FMath::Max(MaxZoomCm, MinZoomCm);
	TargetZoomCm = FMath::Clamp(Arm->TargetArmLength, MinZoomCm, MaxZoomCm);
}

void AFoundrylineCameraPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	Input->BindAxis(TEXT("CameraForward"), this, &AFoundrylineCameraPawn::Forward);
	Input->BindAxis(TEXT("CameraRight"), this, &AFoundrylineCameraPawn::Right);
	Input->BindAxis(TEXT("CameraRotate"), this, &AFoundrylineCameraPawn::Rotate);
	// Wheel input is an impulse, never a held axis. A cached wheel-axis value
	// must not repeatedly multiply the zoom after the scroll has finished.
	for (const bool In : {true, false})
	{
		FInputKeyBinding Wheel(FInputChord(In ? EKeys::MouseScrollUp : EKeys::MouseScrollDown), IE_Pressed);
		Wheel.KeyDelegate.GetDelegateForManualSet().BindLambda([this, In]() { Zoom(In ? 1.f : -1.f); });
		Input->KeyBindings.Add(MoveTemp(Wheel));
	}
	Input->BindAxis(TEXT("CameraMouseRotate"), this, &AFoundrylineCameraPawn::MouseRotate);
	Input->BindAction(TEXT("CameraDragRotate"), IE_Pressed, this, &AFoundrylineCameraPawn::StartRotate);
	Input->BindAction(TEXT("CameraDragRotate"), IE_Released, this, &AFoundrylineCameraPawn::StopRotate);
}

void AFoundrylineCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsLocallyControlled()) return;
 if(const auto* P=Cast<AFoundrylinePlayerController>(GetController());P&&(P->FactoryMachine>0||P->OperationsScreen>0))return;
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
 if(const auto* P=Cast<AFoundrylinePlayerController>(GetController());P && P->LocalMaterialCell) {FocusOnWorldPosition(P->LocalMaterialCell->ForkliftWorldPosition());Arm->TargetArmLength=FMath::FInterpTo(Arm->TargetArmLength,TargetZoomCm,Dt,12.f);return;}
 if(const auto* P=Cast<AFoundrylinePlayerController>(GetController());P && P->LocalFreightRig) {FocusOnWorldPosition(P->LocalFreightRig->TractorWorldPosition());Arm->TargetArmLength=FMath::FInterpTo(Arm->TargetArmLength,TargetZoomCm,Dt,12.f);return;}
	AddActorWorldRotation(FRotator(0.f, RotationInput * RotationDegreesPerSecond * Dt, 0.f));
	const FRotator Yaw(0.f, Arm->GetComponentRotation().Yaw, 0.f);
	// Read the local player's held keys directly. Canvas HUD interaction must not
	// depend on legacy axis delegates reaching the pawn's input component.
	const auto* Player = Cast<APlayerController>(GetController());
	const float PanForward = Player
		? float(Player->IsInputKeyDown(EKeys::W) || Player->IsInputKeyDown(EKeys::Up))
			- float(Player->IsInputKeyDown(EKeys::S) || Player->IsInputKeyDown(EKeys::Down))
		: ForwardInput;
	const float PanRight = Player
		? float(Player->IsInputKeyDown(EKeys::D) || Player->IsInputKeyDown(EKeys::Right))
			- float(Player->IsInputKeyDown(EKeys::A) || Player->IsInputKeyDown(EKeys::Left))
		: RightInput;
	const FVector Direction = (Yaw.Vector() * PanForward
		+ FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * PanRight).GetClampedToMaxSize(1.f);
	const float SpeedScale = FMath::Clamp(TargetZoomCm / 8000.f, 0.2f, 15.f);
	FocusOnWorldPosition(GetActorLocation() + Direction * PanSpeedCmPerSecond * SpeedScale * Dt);
	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, TargetZoomCm, Dt, 12.f);
}

void AFoundrylineCameraPawn::FocusOnWorldPosition(FVector PositionCm)
{
	if (PositionCm.ContainsNaN()) return;
	PositionCm.X = FMath::Clamp(PositionCm.X, FMath::Min(MinPanCm.X, MaxPanCm.X), FMath::Max(MinPanCm.X, MaxPanCm.X));
	PositionCm.Y = FMath::Clamp(PositionCm.Y, FMath::Min(MinPanCm.Y, MaxPanCm.Y), FMath::Max(MinPanCm.Y, MaxPanCm.Y));
	PositionCm.Z = 0.f; // First map is a flat ground plane.
	SetActorLocation(PositionCm);
}

void AFoundrylineCameraPawn::Zoom(float Value)
{
	if (!FMath::IsFinite(Value) || FMath::IsNearlyZero(Value)) return;
	TargetZoomCm = FMath::Clamp(TargetZoomCm * FMath::Pow(1.f + ZoomFractionPerNotch, -Value), MinZoomCm, MaxZoomCm);
}

void AFoundrylineCameraPawn::MouseRotate(float Value)
{
	if (bMouseRotating && IsLocallyControlled()) AddActorWorldRotation(FRotator(0.f, Value * 0.3f, 0.f));
}
