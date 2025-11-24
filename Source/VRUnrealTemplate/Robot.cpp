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
#include "Math/UnrealMathNeon.h"
#include "PhysicsEngine/ConstraintDrives.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IOpenXRHMDModule.h"
#include "IOpenXRHMD.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/WorldSettings.h"
#include "RHIDefinitions.h"
#include "Kismet/KismetMathLibrary.h"

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

  {
    // conservation of momentum debugging

    auto BodyMomentum = GetLinearAndAngularMomentum(Body);
    auto HandMomentum = GetLinearAndAngularMomentum(Hand);
    auto ForearmMomentum = GetLinearAndAngularMomentum(Forearm);
    auto UpperArmMomentum = GetLinearAndAngularMomentum(UpperArm);
    auto TotalLinearMomentum =
        BodyMomentum.LinearMomentum + HandMomentum.LinearMomentum +
        ForearmMomentum.LinearMomentum + UpperArmMomentum.LinearMomentum;
    auto TotalAngularMomentum =
        BodyMomentum.AngularMomentum + HandMomentum.AngularMomentum +
        ForearmMomentum.AngularMomentum + UpperArmMomentum.AngularMomentum;

    auto msg1 = FString::Printf(TEXT("TotalLinearMomentum (%f,%f,%f)"),
                                TotalLinearMomentum.X, TotalLinearMomentum.Y,
                                TotalLinearMomentum.Z);
    auto msg2 = FString::Printf(TEXT("TotalAngularMomentum (%f,%f,%f)"),
                                TotalAngularMomentum.X, TotalAngularMomentum.Y,
                                TotalAngularMomentum.Z);
    // UiMsg(msg1, 2);
    // UiMsg(msg2, 3);
  }

  {
    // drawing the COM
    auto totalMass = Body->GetMass() + Hand->GetMass() + Forearm->GetMass() +
                     UpperArm->GetMass();
    auto COM = 1 / totalMass *
               (Body->GetMass() * Body->GetCenterOfMass() +
                Hand->GetMass() * Hand->GetCenterOfMass() +
                Forearm->GetMass() * Forearm->GetCenterOfMass() +
                UpperArm->GetMass() * UpperArm->GetCenterOfMass());

    auto msg = FString::Printf(TEXT("COM (%f,%f,%f)"), COM.X, COM.Y, COM.Z);

    UiMsg(msg, 8);

    DrawDebugSphere(GetWorld(), COM,
                    5.0f, // radius
                    12,   // segments
                    FColor::Purple,
                    false, // persistent?
                    -1.0f, // lifetime (-1 = one frame)
                    1.0);
  }

  APlayerController *PC = GetWorld()->GetFirstPlayerController();

  AKadhemVRPawn *PlayerPawn = Cast<AKadhemVRPawn>(PC->GetPawn());

  const auto HandMotionEnabled =
      PlayerPawn->GetEnableHandMotionValue().Get<bool>();

  {
    // motion max speed
    auto HandSpeed = Hand->GetPhysicsLinearVelocity().Length();
    if (HandMotionEnabled && !PreviousTickHandMotionEnabled) {
      MaxHandSpeed = 0.0;
    }
    if (HandMotionEnabled && HandSpeed > MaxHandSpeed) {
      MaxHandSpeed = HandSpeed;

      auto msg = FString::Printf(TEXT("HandMaxSpeed %f"), MaxHandSpeed);
      UiMsg(msg, 9);
    }
  }

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
  PlayerPawn->MotionControllerLeft->GetLinearVelocity(ControllerVelocityWorld);

  FVector ViewLocation;
  FRotator ViewRotation;
  PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

  // we are doing the mapping from input controller to the final vector we will
  // use to controll the robot. see note: Robot motion controller mapping

  // up is Z axis
  ViewRotation.Pitch = 0.0;
  ViewRotation.Roll = 0.0;
  const FMatrix CameraTransform =
      FRotationTranslationMatrix(ViewRotation, ViewLocation);
  // ViewMatrix is the 'World -> Camera' transform
  const FMatrix ViewMatrix = CameraTransform.InverseFast();

  FVector FinalInputVectorWorld =
      GetActorTransform().TransformVector(ViewMatrix.TransformVector(
          ControllerVelocityWorld - HMDLinearVelocityWorld));

  if (!FinalInputVectorWorld.IsNearlyZero() && HandMotionEnabled) {

    const auto Force = 130.0 * FinalInputVectorWorld;
    Hand->AddForceAtLocation(Force, ControlPoint->GetComponentLocation());
    Body->AddForceAtLocation(-Force, ArmAttachPoint->GetComponentLocation());

    const FVector Base = ControlPoint->GetComponentLocation();

    DrawDebugDirectionalArrow(GetWorld(), Base, Base + FinalInputVectorWorld,
                              20.0f, // arrow size (shaft + head)
                              FColor::Yellow,
                              false, // persistent?
                              -1.0f, // lifetime (-1 = one frame)
                              0,     // depth priority
                              1.5f   // line thickness
    );
    DrawDebugDirectionalArrow(
        GetWorld(), Base,
        Base + (FinalInputVectorWorld.X * FVector::XAxisVector),
        20.0f, // arrow size (shaft + head)
        FColor::Red,
        false, // persistent?
        -1.0f, // lifetime (-1 = one frame)
        0,     // depth priority
        1.5f   // line thickness
    );
    DrawDebugDirectionalArrow(
        GetWorld(), Base,
        Base + (FinalInputVectorWorld.Y * FVector::YAxisVector),
        20.0f, // arrow size (shaft + head)
        FColor::Green,
        false, // persistent?
        -1.0f, // lifetime (-1 = one frame)
        0,     // depth priority
        1.5f   // line thickness
    );
    DrawDebugDirectionalArrow(
        GetWorld(), Base,
        Base + (FinalInputVectorWorld.Z * FVector::ZAxisVector),
        20.0f, // arrow size (shaft + head)
        FColor::Blue,
        false, // persistent?
        -1.0f, // lifetime (-1 = one frame)
        0,     // depth priority
        1.5f   // line thickness
    );
  }
  PreviousTickHandMotionEnabled = HandMotionEnabled;
}

FMomentumData ARobot::GetLinearAndAngularMomentum(UStaticMeshComponent *Mesh) {
  FMomentumData Result{};
  if (!Mesh)
    return Result;

  // --- Linear momentum ---
  const float Mass = Mesh->GetMass();                   // kg
  const FVector Vel = Mesh->GetPhysicsLinearVelocity(); // cm/s
  Result.LinearMomentum = Mass * Vel;
  Result.LinearMomentum_SI = Result.LinearMomentum / 100.0f; // convert cm→m

  // --- Angular momentum ---
  const FVector AngVel =
      Mesh->GetPhysicsAngularVelocityInRadians(); // world space
  const FVector InertiaLocalDiag =
      Mesh->GetInertiaTensor(); // local space, mass-normalized

  // Build local-space inertia matrix (mass-normalized)
  const FMatrix InertiaTensorLocal = FMatrix(
      FPlane(InertiaLocalDiag.X, 0, 0, 0), FPlane(0, InertiaLocalDiag.Y, 0, 0),
      FPlane(0, 0, InertiaLocalDiag.Z, 0), FPlane(0, 0, 0, 1));

  // Convert to world-space
  const FMatrix Rot = FRotationMatrix(Mesh->GetComponentRotation());
  const FMatrix InertiaTensorWorld =
      Rot * InertiaTensorLocal * Rot.GetTransposed();

  // Multiply by mass (since GetInertiaTensor() is normalized)
  const FMatrix InertiaTensorWorldMass = InertiaTensorWorld * Mass;

  // Angular momentum
  Result.AngularMomentum = InertiaTensorWorldMass.TransformVector(AngVel);
  Result.AngularMomentum_SI =
      Result.AngularMomentum / (100.0f * 100.0f); // cm²→m²

  return Result;
}
