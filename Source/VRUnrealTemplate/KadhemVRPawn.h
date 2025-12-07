#pragma once
#include "Camera/CameraComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "MotionControllerComponent.h"
#include "KadhemMotionControllerComponent.h"
#include "EnhancedInputSubsystems.h"
#include "KadhemVRPawn.generated.h" // always last

struct PlayerInput {
  FInputActionValue EnableLeftLimbMotion;
  FInputActionValue EnableRightLimbMotion;
  FInputActionValue SwitchUpperLeftLimb;
  FInputActionValue SwitchUpperRightLimb;
  FInputActionValue SwitchLowerLeftLimb;
  FInputActionValue SwitchLowerRightLimb;
};

UCLASS()
class VRUNREALTEMPLATE_API AKadhemVRPawn : public APawn {
  GENERATED_BODY()

public:
  AKadhemVRPawn();

protected:
  virtual void BeginPlay() override;
  virtual void SetupPlayerInputComponent(
      class UInputComponent *PlayerInputComponent) override;

  void MoveForward(const FInputActionInstance &Value);
  void MoveRight(const FInputActionInstance &Value);

public:
  virtual void Tick(float DeltaTime) override;

  FInputActionValue GetEnableLeftLimbMotion();
  FInputActionValue GetEnableRightLimbMotion();

  PlayerInput GetPlayerInput();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_MoveForward;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_MoveRight;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_EnableLeftLimbMotion;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_EnableRightLimbMotion;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_SwitchLowerLeftLimb;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_SwitchLowerRightLimb;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_SwitchUpperLeftLimb;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputAction *IA_SwitchUpperRightLimb;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  UInputMappingContext *IMC_Player;

  UPROPERTY(VisibleAnywhere)
  UKadhemMotionControllerComponent *MotionControllerHead;

  UPROPERTY(VisibleAnywhere)
  UKadhemMotionControllerComponent *MotionControllerLeft;

  UPROPERTY(VisibleAnywhere)
  UKadhemMotionControllerComponent *MotionControllerRight;

  UPROPERTY(VisibleAnywhere)
  UCameraComponent *VRCamera;

private:
  UPROPERTY(VisibleAnywhere)
  USceneComponent *VROrigin;

  // Movement
  UPROPERTY(EditAnywhere, Category = "VR Movement")
  float MoveSpeed = 200.f;

  UEnhancedInputLocalPlayerSubsystem *EnhancedInputLocalPlayerSubsystem;
};
