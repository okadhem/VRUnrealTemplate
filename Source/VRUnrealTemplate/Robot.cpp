#include "Robot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/TextureStreamingTypes.h"
#include "Engine/World.h"
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

void drawVectorDebug(UWorld *world, FVector location, FVector vector) {
  DrawDebugDirectionalArrow(world, location, location + vector,
                            20.0f, // arrow size (shaft + head)
                            FColor::Yellow,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
  DrawDebugDirectionalArrow(world, location,
                            location + (vector.X * FVector::XAxisVector),
                            20.0f, // arrow size (shaft + head)
                            FColor::Red,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
  DrawDebugDirectionalArrow(world, location,
                            location + (vector.Y * FVector::YAxisVector),
                            20.0f, // arrow size (shaft + head)
                            FColor::Green,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
  DrawDebugDirectionalArrow(world, location,
                            location + (vector.Z * FVector::ZAxisVector),
                            20.0f, // arrow size (shaft + head)
                            FColor::Blue,
                            false, // persistent?
                            -1.0f, // lifetime (-1 = one frame)
                            0,     // depth priority
                            1.5f   // line thickness
  );
}

ARobot::ARobot() {

  PrimaryActorTick.bCanEverTick = true;
  RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

  Torso = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Torso"));
  Torso->SetupAttachment(RootComponent);

  Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
  Head->SetupAttachment(RootComponent);

  LArmAttachPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("LArmAttachPoint"));
  LArmAttachPoint->SetupAttachment(Torso);

  LUpperArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LUpperArm"));
  LUpperArm->SetupAttachment(RootComponent);

  LForearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LForearm"));
  LForearm->SetupAttachment(RootComponent);

  LHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LHand"));
  LHand->SetupAttachment(RootComponent);

  LArmControlPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("LArmControlPoint"));
  LArmControlPoint->SetupAttachment(LHand);

  RArmAttachPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("RArmAttachPoint"));
  RArmAttachPoint->SetupAttachment(Torso);

  RUpperArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RUpperArm"));
  RUpperArm->SetupAttachment(RootComponent);

  RForearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RForearm"));
  RForearm->SetupAttachment(RootComponent);

  RHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RHand"));
  RHand->SetupAttachment(RootComponent);

  RArmControlPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("RArmControlPoint"));
  RArmControlPoint->SetupAttachment(RHand);

  // lower half

  LLegAttachPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("LLegAttachPoint"));
  LLegAttachPoint->SetupAttachment(Torso);

  LThigh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LThigh"));
  LThigh->SetupAttachment(RootComponent);

  LLowerLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LLowerLeg"));
  LLowerLeg->SetupAttachment(RootComponent);

  LFoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LFoot"));
  LFoot->SetupAttachment(RootComponent);

  LLegControlPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("LLegControlPoint"));
  LLegControlPoint->SetupAttachment(LFoot);

  RLegAttachPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("RLegAttachPoint"));
  RLegAttachPoint->SetupAttachment(Torso);

  RThigh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RThigh"));
  RThigh->SetupAttachment(RootComponent);

  RLowerLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RLowerLeg"));
  RLowerLeg->SetupAttachment(RootComponent);

  RFoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RFoot"));
  RFoot->SetupAttachment(RootComponent);

  RLegControlPoint =
      CreateDefaultSubobject<USceneComponent>(TEXT("RLegControlPoint"));
  RLegControlPoint->SetupAttachment(RFoot);

  // joints

  HeadToTorsoJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("HeadToTorsoJoint"));
  HeadToTorsoJoint->SetupAttachment(Head);

  LTorsoToUpperJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("LTorsoToUpperJoint"));
  LTorsoToUpperJoint->SetupAttachment(Torso);

  LUpperToForearmJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("LUpperToForearmJoint"));
  LUpperToForearmJoint->SetupAttachment(LUpperArm);

  LForearmToHandJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("LForearmToHandJoint"));
  LForearmToHandJoint->SetupAttachment(LForearm);

  RTorsoToUpperJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("RTorsoToUpperJoint"));
  RTorsoToUpperJoint->SetupAttachment(Torso);

  RUpperToForearmJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("RUpperToForearmJoint"));
  RUpperToForearmJoint->SetupAttachment(RUpperArm);

  RForearmToHandJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("RForearmToHandJoint"));
  RForearmToHandJoint->SetupAttachment(RForearm);

  // lower joints

  LTorsoToThighJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("LTorsoToThighJoint"));
  LTorsoToThighJoint->SetupAttachment(Torso);

  LThighToLowerLegJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("LThighToLowerLegJoint"));
  LThighToLowerLegJoint->SetupAttachment(LThigh);

  LLowerLegToFootJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("LLowerLegToFootJoint"));
  LLowerLegToFootJoint->SetupAttachment(LLowerLeg);

  RTorsoToThighJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("RTorsoToThighJoint"));
  RTorsoToThighJoint->SetupAttachment(Torso);

  RThighToLowerLegJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("RThighToLowerLegJoint"));
  RThighToLowerLegJoint->SetupAttachment(RThigh);

  RLowerLegToFootJoint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
      TEXT("RLowerLegToFootJoint"));
  RLowerLegToFootJoint->SetupAttachment(RLowerLeg);

  Head->SetSimulatePhysics(true);
  Torso->SetSimulatePhysics(true);
  LUpperArm->SetSimulatePhysics(true);
  LForearm->SetSimulatePhysics(true);
  LHand->SetSimulatePhysics(true);
  LThigh->SetSimulatePhysics(true);
  LLowerLeg->SetSimulatePhysics(true);
  LFoot->SetSimulatePhysics(true);
  RUpperArm->SetSimulatePhysics(true);
  RForearm->SetSimulatePhysics(true);
  RHand->SetSimulatePhysics(true);
  RThigh->SetSimulatePhysics(true);
  RLowerLeg->SetSimulatePhysics(true);
  RFoot->SetSimulatePhysics(true);

  HeadToTorsoJoint->SetConstrainedComponents(Head, NAME_None, Torso, NAME_None);

  LTorsoToUpperJoint->SetConstrainedComponents(Torso, NAME_None, LUpperArm,
                                               NAME_None);
  LUpperToForearmJoint->SetConstrainedComponents(LUpperArm, NAME_None, LForearm,
                                                 NAME_None);
  LForearmToHandJoint->SetConstrainedComponents(LForearm, NAME_None, LHand,
                                                NAME_None);
  LTorsoToThighJoint->SetConstrainedComponents(Torso, NAME_None, LThigh,
                                               NAME_None);
  LThighToLowerLegJoint->SetConstrainedComponents(LThigh, NAME_None, LLowerLeg,
                                                  NAME_None);
  LLowerLegToFootJoint->SetConstrainedComponents(LLowerLeg, NAME_None, LFoot,
                                                 NAME_None);
  RTorsoToUpperJoint->SetConstrainedComponents(Torso, NAME_None, RUpperArm,
                                               NAME_None);
  RUpperToForearmJoint->SetConstrainedComponents(RUpperArm, NAME_None, RForearm,
                                                 NAME_None);
  RForearmToHandJoint->SetConstrainedComponents(RForearm, NAME_None, RHand,
                                                NAME_None);
  RTorsoToThighJoint->SetConstrainedComponents(Torso, NAME_None, RThigh,
                                               NAME_None);
  RThighToLowerLegJoint->SetConstrainedComponents(RThigh, NAME_None, RLowerLeg,
                                                  NAME_None);
  RLowerLegToFootJoint->SetConstrainedComponents(RLowerLeg, NAME_None, RFoot,
                                                 NAME_None);

  UPhysicsConstraintComponent *joints[13] = {
      HeadToTorsoJoint,     LTorsoToThighJoint,    LTorsoToUpperJoint,
      LLowerLegToFootJoint, LThighToLowerLegJoint, LUpperToForearmJoint,
      LForearmToHandJoint,  RTorsoToThighJoint,    RTorsoToUpperJoint,
      RLowerLegToFootJoint, RThighToLowerLegJoint, RUpperToForearmJoint,
      RForearmToHandJoint};

  for (int i = 0; i < 13; i++) {

    UE_LOG(LogTemp, Warning, TEXT("Running joint init from c++"));

    joints[i]->SetDisableCollision(true);
    joints[i]->ConstraintInstance.DisableMassConditioning();
    joints[i]->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
    joints[i]->SetAngularVelocityDriveTwistAndSwing(true, true);
    joints[i]->SetAngularDriveParams(0.0,   // stiffness
                                     600.0, // damping
                                     100000);
  }
}

void ARobot::BeginPlay() {
  Super::BeginPlay();
  if (LArmAttachPoint != nullptr) {
    UE_LOG(LogTemp, Warning, TEXT("Found ArmAttachPoint"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to find ArmAttachPoint"));
  }
}

void ARobot::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  {
    // conservation of momentum debugging

    /*
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
                                TotalAngularMomentum.X,
    TotalAngularMomentum.Y, TotalAngularMomentum.Z);
    // UiMsg(msg1, 2);
    // UiMsg(msg2, 3);
    */
  }

  UStaticMeshComponent *BodyParts[14] = {
      Head,  Torso,     LUpperArm, LForearm, LHand,  LThigh,    LLowerLeg,
      LFoot, RUpperArm, RForearm,  RHand,    RThigh, RLowerLeg, RFoot};
  {
    // drawing the COM

    auto totalMass = 0;
    auto acc = FVector::Zero();
    for (int i = 0; i < 14; i++) {
      totalMass += BodyParts[i]->GetMass();
      acc += BodyParts[i]->GetMass() * BodyParts[i]->GetCenterOfMass();
    }
    auto centerOfMass = (1.0 / totalMass) * acc;

    auto msg = FString::Printf(TEXT("centerOfMass (%f,%f,%f)"), centerOfMass.X,
                               centerOfMass.Y, centerOfMass.Z);

    UiMsg(msg, 8);

    DrawDebugSphere(GetWorld(), centerOfMass,
                    5.0f, // radius
                    12,   // segments
                    FColor::Purple,
                    false, // persistent?
                    -1.0f, // lifetime (-1 = one frame)
                    1.0);
  }

  APlayerController *PC = GetWorld()->GetFirstPlayerController();

  AKadhemVRPawn *PlayerPawn = Cast<AKadhemVRPawn>(PC->GetPawn());

  const auto playerInput = PlayerPawn->GetPlayerInput();

  auto LeftLimbMotionTurnedOn = playerInput.EnableLeftLimbMotion.Get<bool>() &&
                                !PreviousTickEnableLeftLimbMotion;

  auto RightLimbMotionTurnedOn =
      playerInput.EnableRightLimbMotion.Get<bool>() &&
      !PreviousTickEnableRightLimbMotion;

  auto LeftLimbMotionTurnedOff =
      !playerInput.EnableLeftLimbMotion.Get<bool>() &&
      PreviousTickEnableLeftLimbMotion;

  auto RightLimbMotionTurnedOff =
      !playerInput.EnableRightLimbMotion.Get<bool>() &&
      PreviousTickEnableRightLimbMotion;
  {
    // motion max speed

    if (LeftLimbMotionTurnedOn) {
      MaxLeftHandSpeed = 0.0;
      MaxLeftFootSpeed = 0.0;
    }
    auto LHandSpeed = LHand->GetPhysicsLinearVelocity().Length();
    if (playerInput.EnableLeftLimbMotion.Get<bool>() &&
        LHandSpeed > MaxLeftHandSpeed) {
      MaxLeftHandSpeed = LHandSpeed;
    }
    auto handMsg =
        FString::Printf(TEXT("LeftHandMaxSpeed %f"), MaxLeftHandSpeed);
    UiMsg(handMsg, 9);

    auto LFootSpeed = LFoot->GetPhysicsLinearVelocity().Length();
    if (playerInput.EnableLeftLimbMotion.Get<bool>() &&
        LFootSpeed > MaxLeftFootSpeed) {
      MaxLeftFootSpeed = LFootSpeed;
    }
    auto footMsg =
        FString::Printf(TEXT("LeftFootMaxSpeed %f"), MaxLeftFootSpeed);
    UiMsg(footMsg, 9);
  }

  FVector HMDLinearVelocityWorld;
  {
    IOpenXRHMD *OpenXRHMD =
        static_cast<IOpenXRHMD *>(GEngine->XRSystem->GetIOpenXRHMD());

    FQuat HMDOrientation;
    FVector HMDPosition;
    FRotator HMDAngularVelocity;
    FVector AngularVelocityAsAxisAndLength;
    FVector HMDLinearAcceleration;
    bool bTimeWasUsed;
    bool bProvidedLinearVelocity;
    bool bProvidedAngularVelocity;
    bool bProvidedLinearAcceleration;
    bool result = OpenXRHMD->GetPoseForTime(
        IXRTrackingSystem::HMDDeviceId, OpenXRHMD->GetDisplayTime(),
        bTimeWasUsed, HMDOrientation, HMDPosition, bProvidedLinearVelocity,
        HMDLinearVelocityWorld, bProvidedAngularVelocity,
        AngularVelocityAsAxisAndLength, bProvidedLinearAcceleration,
        HMDLinearAcceleration, GetWorld()->GetWorldSettings()->WorldToMeters);
  }

  FMatrix ViewMatrix;
  {
    FVector ViewLocation;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

    // we are doing the mapping from input controller to the final vector we
    // will use to controll the robot. see note: Robot motion controller mapping

    // up is Z axis
    ViewRotation.Pitch = 0.0;
    ViewRotation.Roll = 0.0;
    const FMatrix CameraTransform =
        FRotationTranslationMatrix(ViewRotation, ViewLocation);
    // ViewMatrix is the 'World -> Camera' transform
    ViewMatrix = CameraTransform.InverseFast();
  }

  if (playerInput.SwitchLowerLeftLimb.Get<bool>()) {
    LeftSelectedLimb = LowerUpper::Lower;
  }
  if (playerInput.SwitchUpperLeftLimb.Get<bool>()) {
    LeftSelectedLimb = LowerUpper::Upper;
  }
  if (playerInput.SwitchLowerRightLimb.Get<bool>()) {
    RightSelectedLimb = LowerUpper::Lower;
  }
  if (playerInput.SwitchUpperRightLimb.Get<bool>()) {
    RightSelectedLimb = LowerUpper::Upper;
  }
  {
    if (LeftLimbMotionTurnedOn && LeftSelectedLimb == LowerUpper::Upper) {
      LTorsoToUpperJoint->SetAngularOrientationDrive(false, false);
      LUpperToForearmJoint->SetAngularOrientationDrive(false, false);
      LForearmToHandJoint->SetAngularOrientationDrive(false, false);
    }
    if (LeftLimbMotionTurnedOn && LeftSelectedLimb == LowerUpper::Lower) {
      LTorsoToThighJoint->SetAngularOrientationDrive(false, false);
      LThighToLowerLegJoint->SetAngularOrientationDrive(false, false);
      LLowerLegToFootJoint->SetAngularOrientationDrive(false, false);
    }
    if (RightLimbMotionTurnedOn && RightSelectedLimb == LowerUpper::Upper) {
      RTorsoToUpperJoint->SetAngularOrientationDrive(false, false);
      RUpperToForearmJoint->SetAngularOrientationDrive(false, false);
      RForearmToHandJoint->SetAngularOrientationDrive(false, false);
    }
    if (RightLimbMotionTurnedOn && RightSelectedLimb == LowerUpper::Lower) {
      RTorsoToThighJoint->SetAngularOrientationDrive(false, false);
      RThighToLowerLegJoint->SetAngularOrientationDrive(false, false);
      RLowerLegToFootJoint->SetAngularOrientationDrive(false, false);
    }
  }

  if (playerInput.EnableLeftLimbMotion.Get<bool>()) {
    FVector ControllerVelocityWorld;
    PlayerPawn->MotionControllerLeft->GetLinearVelocity(
        ControllerVelocityWorld);

    FVector FinalInputVectorWorld =
        GetActorTransform().TransformVector(ViewMatrix.TransformVector(
            ControllerVelocityWorld - HMDLinearVelocityWorld));

    const auto Force = 130.0 * FinalInputVectorWorld;
    if (LeftSelectedLimb == LowerUpper::Upper) {
      LHand->AddForceAtLocation(Force,
                                LArmControlPoint->GetComponentLocation());
      Torso->AddForceAtLocation(-Force,
                                LArmAttachPoint->GetComponentLocation());

      drawVectorDebug(GetWorld(), LArmControlPoint->GetComponentLocation(),
                      FinalInputVectorWorld);
    } else {
      LFoot->AddForceAtLocation(Force,
                                LLegControlPoint->GetComponentLocation());
      Torso->AddForceAtLocation(-Force,
                                LLegAttachPoint->GetComponentLocation());

      drawVectorDebug(GetWorld(), LLegControlPoint->GetComponentLocation(),
                      FinalInputVectorWorld);
    }
  }

  if (playerInput.EnableRightLimbMotion.Get<bool>()) {
    FVector ControllerVelocityWorld;
    PlayerPawn->MotionControllerRight->GetLinearVelocity(
        ControllerVelocityWorld);

    FVector FinalInputVectorWorld =
        GetActorTransform().TransformVector(ViewMatrix.TransformVector(
            ControllerVelocityWorld - HMDLinearVelocityWorld));

    const auto Force = 130.0 * FinalInputVectorWorld;
    if (RightSelectedLimb == LowerUpper::Upper) {
      RHand->AddForceAtLocation(Force,
                                RArmControlPoint->GetComponentLocation());
      Torso->AddForceAtLocation(-Force,
                                RArmAttachPoint->GetComponentLocation());

      drawVectorDebug(GetWorld(), RArmControlPoint->GetComponentLocation(),
                      FinalInputVectorWorld);
    } else {
      RFoot->AddForceAtLocation(Force,
                                RLegControlPoint->GetComponentLocation());
      Torso->AddForceAtLocation(-Force,
                                RLegAttachPoint->GetComponentLocation());

      drawVectorDebug(GetWorld(), RLegControlPoint->GetComponentLocation(),
                      FinalInputVectorWorld);
    }
  }

  PreviousTickEnableLeftLimbMotion =
      playerInput.EnableLeftLimbMotion.Get<bool>();
  PreviousTickEnableRightLimbMotion =
      playerInput.EnableRightLimbMotion.Get<bool>();
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
