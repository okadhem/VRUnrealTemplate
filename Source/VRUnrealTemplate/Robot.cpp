#include "Robot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "KadhemVRPawn.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

ARobot::ARobot() {

  PrimaryActorTick.bCanEverTick = true;
  RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

  Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
  Body->SetupAttachment(RootComponent);

  ArmAttachPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("ArmAttachPoint"));
  ArmAttachPoint->SetupAttachment(Body);

  UpperArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperArm"));
  UpperArm->SetupAttachment(RootComponent);

  Forearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Forearm"));
  Forearm->SetupAttachment(RootComponent);

  Hand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hand"));
  Hand->SetupAttachment(RootComponent);

  ControlPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ControlPoint"));
  ControlPoint->SetupAttachment(Hand);

  BodyToUpperJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("BodyUpperJoint"));
  BodyToUpperJoint->SetupAttachment(Body);

  UpperToForearmJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("UpperForearmJoint"));
  UpperToForearmJoint->SetupAttachment(UpperArm);

  ForearmToHandJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("ForearmHandJoint"));
  ForearmToHandJoint->SetupAttachment(Forearm);

  Body->SetSimulatePhysics(true);
  UpperArm->SetSimulatePhysics(true);
  Forearm->SetSimulatePhysics(true);
  Hand->SetSimulatePhysics(true);

  BodyToUpperJoint->SetConstrainedComponents(Body, NAME_None, UpperArm,
                                             NAME_None);
  UpperToForearmJoint->SetConstrainedComponents(UpperArm, NAME_None, Forearm,
                                                NAME_None);
  ForearmToHandJoint->SetConstrainedComponents(Forearm, NAME_None, Hand,
                                               NAME_None);
}

void ARobot::BeginPlay() {
  Super::BeginPlay();
  if (ArmAttachPoint != nullptr) {
    UE_LOG(LogTemp, Warning, TEXT("Found ArmAttachPoint"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find ArmAttachPoint"));
  }
}

void ARobot::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  APlayerController *PC = GetWorld()->GetFirstPlayerController();

  AKadhemVRPawn *PlayerPawn = Cast<AKadhemVRPawn>(PC->GetPawn());

  if (PlayerPawn->MotionControllerLeft->IsTracked()) {
    UE_LOG(LogTemp, Log, TEXT("Left is tracked"));
  }

  FVector ControllerVelocity;
  PlayerPawn->MotionControllerRight->GetLinearVelocity(
      ControllerVelocity); // TODO

  if (!ControllerVelocity.IsNearlyZero()) {

    FVector ViewLocation;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FMatrix CameraTransform =
        FRotationTranslationMatrix(ViewRotation, ViewLocation);
    // ViewMatrix is the World -> Camera transform
    const FMatrix ViewMatrix = CameraTransform.InverseFast();

    const FVector AccelerationToApply = GetActorTransform().TransformVector(
        ViewMatrix.TransformVector(ControllerVelocity));

    DrawDebugDirectionalArrow(
        GetWorld(), ControlPoint->GetComponentLocation(),
        ControlPoint->GetComponentLocation() + AccelerationToApply,
        20.0f, // arrow size (shaft + head)
        FColor::Red,
        false, // persistent?
        -1.0f, // lifetime (-1 = one frame)
        0,     // depth priority
        2.0f   // line thickness
    );
  } else {
    UE_LOG(LogTemp, Warning, TEXT("Zero acceleration: %f"),
           ControllerVelocity.Length());
  }
}
