#pragma once

#include "CoreMinimal.h"
#include "MotionControllerComponent.h"
#include "KadhemMotionControllerComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VRUNREALTEMPLATE_API UKadhemMotionControllerComponent
    : public UMotionControllerComponent {
  GENERATED_BODY()

public:
  UKadhemMotionControllerComponent();

protected:
  virtual void BeginPlay() override;

public:
  virtual void
  TickComponent(float DeltaTime, enum ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  using UMotionControllerComponent::GetLinearAcceleration;
  using UMotionControllerComponent::GetLinearVelocity;
};
