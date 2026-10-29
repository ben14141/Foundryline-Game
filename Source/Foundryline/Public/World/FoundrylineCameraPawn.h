#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FoundrylineCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

// Local presentation only. Camera movement never changes authoritative simulation state.
UCLASS()
class FOUNDRYLINE_API AFoundrylineCameraPawn : public APawn
{
	GENERATED_BODY()
public:
	AFoundrylineCameraPawn();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
	UFUNCTION(BlueprintCallable, Category="Camera")
	void FocusOnWorldPosition(FVector PositionCm);
 void ClearCameraInput(){ForwardInput=RightInput=RotationInput=0;bMouseRotating=false;}

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<USpringArmComponent> Arm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="100"))
	float MinZoomCm = 500.f;
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="100"))
	float MaxZoomCm = 150000.f;
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="1"))
	float PanSpeedCmPerSecond = 1800.f;
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="1"))
	float RotationDegreesPerSecond = 80.f;
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.01", ClampMax="0.5"))
	float ZoomFractionPerNotch = 0.12f;
	UPROPERTY(EditAnywhere, Category="Camera")
	FVector2D MinPanCm = FVector2D(-500000.f, -500000.f);
	UPROPERTY(EditAnywhere, Category="Camera")
	FVector2D MaxPanCm = FVector2D(500000.f, 500000.f);

private:
	void Forward(float Value) { ForwardInput = Value; }
	void Right(float Value) { RightInput = Value; }
	void Rotate(float Value) { RotationInput = Value; }
	void Zoom(float Value);
	void MouseRotate(float Value);
	void StartRotate() { bMouseRotating = true; }
	void StopRotate() { bMouseRotating = false; }
	float ForwardInput = 0.f;
	float RightInput = 0.f;
	float RotationInput = 0.f;
	float TargetZoomCm = 17000.f;
	bool bMouseRotating = false;
};
