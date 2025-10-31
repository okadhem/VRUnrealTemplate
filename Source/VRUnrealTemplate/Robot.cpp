#include "Robot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/TextureStreamingTypes.h"
#include "HAL/Platform.h"
#include "KadhemVRPawn.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"
#include "DrawDebugHelpers.h"
#include "Math/MathFwd.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IOpenXRHMDModule.h"
#include "IOpenXRHMD.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/WorldSettings.h"

// Note: Robot motion controller mapping
//
// We interpret the motion reported by the hand controllers to produce a vector
// that will then be applied to the robot hand effector. the key question is
// what space is used as reference for controller motion: Option 1 : reference
// space is at tracks the torso / upper body, its origin can be in any place at
// upper body, orientation has to follow torso direction *not head*. there are
// some limitation here: hardware support: meta has some support for torso
// orientation, unsure about how good is the data. software: supported OpenXR
// with a Meta extension, not supported in Unreal.
//
// What we can do without support for torso / upper body in OpenXR:
// reference space is HeadMountedDisplay with some modification, in the
// following we describe this space with respect to the world space.
// Origin at neck, this is supposed to be the case for Quest3 but not 100% sure,
// the important part is that it must be fixed to the body, neck is the only
// placement which will be fixed even when head is moving.
// X axis, Y axis : alligned with world axis, torso in normal player position is
// always straight with no rotation in these axis.
// Z axis: alligned with HeadMountedDisplay z axis, this is wrong, since its
// pretty normal for the head and torso to be at different z angles, but we
// don't have an option here.
//

void UiMsg(FString &msg, int32 key) {
  GEngine->AddOnScreenDebugMessage(key, 3.0, FColor::Yellow, msg);
}

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

  IOpenXRHMD *OpenXRHMD =
      static_cast<IOpenXRHMD *>(GEngine->XRSystem->GetIOpenXRHMD());

  FQuat HMDOrientation;
  FVector HMDPosition;
  FVector HMDLinearVelocityWorld;
  FRotator HMDAngularVelocity;
  FVector AngularVelocityAsAxisAndLength;
  FVector HMDLinearAcceleration;
  bool bTimeWasUsed;
  bool bProvidedLinearVelocity;
  bool bProvidedAngularVelocity;
  bool bProvidedLinearAcceleration;
  bool result = OpenXRHMD->GetPoseForTime(
      IXRTrackingSystem::HMDDeviceId, OpenXRHMD->GetDisplayTime(), bTimeWasUsed,
      HMDOrientation, HMDPosition, bProvidedLinearVelocity,
      HMDLinearVelocityWorld, bProvidedAngularVelocity,
      AngularVelocityAsAxisAndLength, bProvidedLinearAcceleration,
      HMDLinearAcceleration, GetWorld()->GetWorldSettings()->WorldToMeters);

  auto msg = FString::Printf(TEXT("HMDLinearVelocity.Length %f"),
                             HMDLinearVelocityWorld.Length());
  UiMsg(msg, 1);

  FVector ControllerVelocityWorld;
  PlayerPawn->MotionControllerRight->GetLinearVelocity(ControllerVelocityWorld);

  FVector ViewLocation;
  FRotator ViewRotation;
  PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

  // we are doing the mapping from input controller to the final vector we will
  // use to controll the robot. see note: Robot motion controller mapping Yaw is

  // up is Z axis
  ViewRotation.Pitch = 0.0;
  ViewRotation.Roll = 0.0;
  const FMatrix CameraTransform =
      FRotationTranslationMatrix(ViewRotation, ViewLocation);
  // ViewMatrix is the 'World -> Camera' transform
  const FMatrix ViewMatrix = CameraTransform.InverseFast();

  const FVector FinalInputVectorWorld =
      GetActorTransform().TransformVector(ViewMatrix.TransformVector(
          ControllerVelocityWorld - HMDLinearVelocityWorld));

  const FVector DebugVector =
      ControlPoint->GetComponentLocation() + FinalInputVectorWorld;

  DrawDebugDirectionalArrow(GetWorld(), ControlPoint->GetComponentLocation(),
                            DebugVector,
                            20.0f, // arrow size (shaft + head)
                            FColor::Yellow,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
  DrawDebugDirectionalArrow(GetWorld(), ControlPoint->GetComponentLocation(),
                            DebugVector.X * FVector::XAxisVector,
                            20.0f, // arrow size (shaft + head)
                            FColor::Red,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
  DrawDebugDirectionalArrow(GetWorld(), ControlPoint->GetComponentLocation(),
                            DebugVector.Y * FVector::YAxisVector,
                            20.0f, // arrow size (shaft + head)
                            FColor::Green,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
  DrawDebugDirectionalArrow(GetWorld(), ControlPoint->GetComponentLocation(),
                            DebugVector.Z * FVector::ZAxisVector,
                            20.0f, // arrow size (shaft + head)
                            FColor::Blue,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
}
