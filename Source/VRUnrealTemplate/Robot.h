#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Robot.generated.h"

UCLASS()
class VRUNREALTEMPLATE_API ARobot : public AActor {
  GENERATED_BODY()

public:
  ARobot();

  virtual void Tick(float DeltaTime);

protected:
  virtual void BeginPlay() override;

public:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *Body;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *UpperArm;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *Forearm;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  UStaticMeshComponent *Hand;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *ArmAttachPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
  USceneComponent *ControlPoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *BodyToUpperJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *UpperToForearmJoint;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Joints")
  class UPhysicsConstraintComponent *ForearmToHandJoint;
};
