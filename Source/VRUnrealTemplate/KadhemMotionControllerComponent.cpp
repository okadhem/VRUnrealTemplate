#include "KadhemMotionControllerComponent.h"
#include "GameFramework/Actor.h"

void UKadhemMotionControllerComponent::TickComponent(
    float DeltaTime, enum ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
UKadhemMotionControllerComponent::UKadhemMotionControllerComponent() {
  PrimaryComponentTick.bCanEverTick = true;
}

void UKadhemMotionControllerComponent::BeginPlay() {
  Super::BeginPlay();

  UE_LOG(LogTemp, Log, TEXT("KadhemControllerComponent initialized for %s"),
         *GetOwner()->GetName());
}
