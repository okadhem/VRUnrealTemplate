#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Robot.generated.h"

struct FMomentumData {
  FVector LinearMomentum;     // kg·cm/s
  FVector LinearMomentum_SI;  // kg·m/s
  FVector AngularMomentum;    // kg·cm²/s
  FVector AngularMomentum_SI; // kg·m²/s
};

enum class LowerUpper {
  Lower,
  Upper,
};

UCLASS() class VRUNREALTEMPLATE_API ARobot : public AActor {
  GENERATED_BODY()

public:
  ARobot();

  virtual void Tick(float DeltaTime);

protected:
  virtual void BeginPlay() override;

public:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *Head;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *Torso;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *LUpperArm;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *RUpperArm;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *LForearm;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *RForearm;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *LHand;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *RHand;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *LArmAttachPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *RArmAttachPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *LArmControlPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *RArmControlPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *LThigh;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *RThigh;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *LLowerLeg;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *RLowerLeg;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *LFoot;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *RFoot;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *LLegAttachPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *RLegAttachPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *LLegControlPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *RLegControlPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *HeadToTorsoJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *LTorsoToUpperJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *LUpperToForearmJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *LForearmToHandJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *RTorsoToUpperJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *RUpperToForearmJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *RForearmToHandJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *LTorsoToThighJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *RTorsoToThighJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *LThighToLowerLegJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *RThighToLowerLegJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *LLowerLegToFootJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *RLowerLegToFootJoint;

private:
  static FMomentumData GetLinearAndAngularMomentum(UStaticMeshComponent *Mesh);

  bool PreviousTickEnableLeftLimbMotion = false;
  bool PreviousTickEnableRightLimbMotion = false;
  float MaxLeftHandSpeed = 0.0;
  float MaxLeftFootSpeed = 0.0;
  LowerUpper LeftSelectedLimb = LowerUpper::Upper;
  LowerUpper RightSelectedLimb = LowerUpper::Upper;
};
