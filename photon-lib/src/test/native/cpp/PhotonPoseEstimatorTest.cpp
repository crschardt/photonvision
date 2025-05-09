/*
 * MIT License
 *
 * Copyright (c) PhotonVision
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <map>
#include <utility>
#include <vector>

#include <frc/apriltag/AprilTagFieldLayout.h>
#include <frc/geometry/Pose3d.h>
#include <frc/geometry/Rotation3d.h>
#include <frc/geometry/Transform3d.h>
#include <units/angle.h>
#include <units/length.h>
#include <wpi/SmallVector.h>

#include "gtest/gtest.h"
#include "photon/PhotonCamera.h"
#include "photon/PhotonPoseEstimator.h"
#include "photon/dataflow/structures/Packet.h"
#include "photon/targeting/MultiTargetPNPResult.h"
#include "photon/targeting/PhotonPipelineResult.h"
#include "photon/targeting/PhotonTrackedTarget.h"
#include "photon/targeting/PnpResult.h"

static std::vector<frc::AprilTag> tags = {
    {0, frc::Pose3d(units::meter_t(3), units::meter_t(3), units::meter_t(3),
                    frc::Rotation3d())},
    {1, frc::Pose3d(units::meter_t(5), units::meter_t(5), units::meter_t(5),
                    frc::Rotation3d())}};

static frc::AprilTagFieldLayout aprilTags{tags, 54_ft, 27_ft};

static std::vector<photon::TargetCorner> corners{
    photon::TargetCorner{1., 2.}, photon::TargetCorner{3., 4.},
    photon::TargetCorner{5., 6.}, photon::TargetCorner{7., 8.}};
static std::vector<photon::TargetCorner> detectedCorners{
    photon::TargetCorner{1., 2.}, photon::TargetCorner{3., 4.},
    photon::TargetCorner{5., 6.}, photon::TargetCorner{7., 8.}};

TEST(PhotonPoseEstimatorTest, LowestAmbiguityStrategy) {
  photon::PhotonCamera cameraOne = photon::PhotonCamera("test");

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(1_m, 2_m, 3_m),
                           frc::Rotation3d(1_rad, 2_rad, 3_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 2_m, 3_m),
                           frc::Rotation3d(1_rad, 2_rad, 3_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(4_m, 2_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(4_m, 2_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(1_m, 2_m, 3_m),
                           frc::Rotation3d(1_rad, 2_rad, 3_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 2_m, 3_m),
                           frc::Rotation3d(1_rad, 2_rad, 3_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.test = true;
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(11));

  photon::PhotonPoseEstimator estimator(aprilTags, photon::LOWEST_AMBIGUITY,
                                        frc::Transform3d{});

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }
  ASSERT_TRUE(estimatedPose);
  frc::Pose3d pose = estimatedPose.value().estimatedPose;

  EXPECT_NEAR(11, units::unit_cast<double>(estimatedPose.value().timestamp),
              .02);
  EXPECT_NEAR(1, units::unit_cast<double>(pose.X()), .01);
  EXPECT_NEAR(3, units::unit_cast<double>(pose.Y()), .01);
  EXPECT_NEAR(2, units::unit_cast<double>(pose.Z()), .01);
}

TEST(PhotonPoseEstimatorTest, ClosestToCameraHeightStrategy) {
  std::vector<frc::AprilTag> tags = {
      {0, frc::Pose3d(units::meter_t(3), units::meter_t(3), units::meter_t(3),
                      frc::Rotation3d())},
      {1, frc::Pose3d(units::meter_t(5), units::meter_t(5), units::meter_t(5),
                      frc::Rotation3d())},
  };
  auto aprilTags = frc::AprilTagFieldLayout(tags, 54_ft, 27_ft);

  std::vector<std::pair<photon::PhotonCamera, frc::Transform3d>> cameras;

  photon::PhotonCamera cameraOne = photon::PhotonCamera("test");

  // ID 0 at 3,3,3
  // ID 1 at 5,5,5

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(0_m, 0_m, 0_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 1_m, 1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2_m, 2_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(4_m, 4_m, 4_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(5_m, 5_m, 5_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.test = true;
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(17_s);

  photon::PhotonPoseEstimator estimator(
      aprilTags, photon::CLOSEST_TO_CAMERA_HEIGHT, {{0_m, 0_m, 4_m}, {}});

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }
  ASSERT_TRUE(estimatedPose);

  frc::Pose3d pose = estimatedPose.value().estimatedPose;

  EXPECT_NEAR(17, units::unit_cast<double>(estimatedPose.value().timestamp),
              .02);
  EXPECT_NEAR(4, units::unit_cast<double>(pose.X()), .01);
  EXPECT_NEAR(4, units::unit_cast<double>(pose.Y()), .01);
  EXPECT_NEAR(0, units::unit_cast<double>(pose.Z()), .01);
}

TEST(PhotonPoseEstimatorTest, ClosestToReferencePoseStrategy) {
  photon::PhotonCamera cameraOne = photon::PhotonCamera("test");

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(0_m, 0_m, 0_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 1_m, 1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2_m, 2_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2.2_m, 2.2_m, 2.2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(2_m, 1.9_m, 2.1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.test = true;
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(17));

  photon::PhotonPoseEstimator estimator(aprilTags,
                                        photon::CLOSEST_TO_REFERENCE_POSE, {});
  estimator.SetReferencePose(
      frc::Pose3d(1_m, 1_m, 1_m, frc::Rotation3d(0_rad, 0_rad, 0_rad)));

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  ASSERT_TRUE(estimatedPose);
  frc::Pose3d pose = estimatedPose.value().estimatedPose;

  EXPECT_NEAR(17, units::unit_cast<double>(estimatedPose.value().timestamp),
              .01);
  EXPECT_NEAR(1, units::unit_cast<double>(pose.X()), .01);
  EXPECT_NEAR(1.1, units::unit_cast<double>(pose.Y()), .01);
  EXPECT_NEAR(.9, units::unit_cast<double>(pose.Z()), .01);
}

TEST(PhotonPoseEstimatorTest, ClosestToLastPose) {
  photon::PhotonCamera cameraOne = photon::PhotonCamera("test");

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(0_m, 0_m, 0_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 1_m, 1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2_m, 2_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2.2_m, 2.2_m, 2.2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(2_m, 1.9_m, 2.1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.test = true;
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(17));

  photon::PhotonPoseEstimator estimator(aprilTags, photon::CLOSEST_TO_LAST_POSE,
                                        {});
  estimator.SetLastPose(
      frc::Pose3d(1_m, 1_m, 1_m, frc::Rotation3d(0_rad, 0_rad, 0_rad)));

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  ASSERT_TRUE(estimatedPose);
  frc::Pose3d pose = estimatedPose.value().estimatedPose;

  std::vector<photon::PhotonTrackedTarget> targetsThree{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(0_m, 0_m, 0_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 1_m, 1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2.1_m, 1.9_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2.4_m, 2.4_m, 2.2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(2_m, 1_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targetsThree,
      std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(21));

  // std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  ASSERT_TRUE(estimatedPose);
  pose = estimatedPose.value().estimatedPose;

  EXPECT_NEAR(21.0, units::unit_cast<double>(estimatedPose.value().timestamp),
              .01);
  EXPECT_NEAR(.9, units::unit_cast<double>(pose.X()), .01);
  EXPECT_NEAR(1.1, units::unit_cast<double>(pose.Y()), .01);
  EXPECT_NEAR(1, units::unit_cast<double>(pose.Z()), .01);
}

TEST(PhotonPoseEstimatorTest, AverageBestPoses) {
  photon::PhotonCamera cameraOne = photon::PhotonCamera("test");

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2_m, 2_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 1_m, 1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(0_m, 0_m, 0_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(2_m, 1.9_m, 2.1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.test = true;
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(15));

  photon::PhotonPoseEstimator estimator(aprilTags, photon::AVERAGE_BEST_TARGETS,
                                        {});

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  ASSERT_TRUE(estimatedPose);
  frc::Pose3d pose = estimatedPose.value().estimatedPose;

  EXPECT_NEAR(15.0, units::unit_cast<double>(estimatedPose.value().timestamp),
              .01);
  EXPECT_NEAR(2.15, units::unit_cast<double>(pose.X()), .01);
  EXPECT_NEAR(2.15, units::unit_cast<double>(pose.Y()), .01);
  EXPECT_NEAR(2.15, units::unit_cast<double>(pose.Z()), .01);
}

TEST(PhotonPoseEstimatorTest, PoseCache) {
  photon::PhotonCamera cameraOne = photon::PhotonCamera("test2");

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(2_m, 2_m, 2_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 1_m, 1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(3_m, 3_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          9.0, -2.0, 19.0, 3.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(0_m, 0_m, 0_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(2_m, 1.9_m, 2.1_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.4, corners, detectedCorners}};

  cameraOne.test = true;

  photon::PhotonPoseEstimator estimator(aprilTags, photon::AVERAGE_BEST_TARGETS,
                                        {});

  // empty input, expect empty out
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000},
      std::vector<photon::PhotonTrackedTarget>{}, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(1));

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  EXPECT_FALSE(estimatedPose);

  // Set result, and update -- expect present and timestamp to be 15
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 3000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(15));

  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  ASSERT_TRUE(estimatedPose);
  EXPECT_NEAR((15_s - 3_ms).to<double>(),
              estimatedPose.value().timestamp.to<double>(), 1e-6);

  // And again -- now pose cache should be empty
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }

  EXPECT_FALSE(estimatedPose);
}

TEST(PhotonPoseEstimatorTest, MultiTagOnRioFallback) {
  photon::PhotonCamera cameraOne = photon::PhotonCamera("test");

  std::vector<photon::PhotonTrackedTarget> targets{
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.0, 4.0, 0, -1, -1.f,
          frc::Transform3d(frc::Translation3d(1_m, 2_m, 3_m),
                           frc::Rotation3d(1_rad, 2_rad, 3_rad)),
          frc::Transform3d(frc::Translation3d(1_m, 2_m, 3_m),
                           frc::Rotation3d(1_rad, 2_rad, 3_rad)),
          0.7, corners, detectedCorners},
      photon::PhotonTrackedTarget{
          3.0, -4.0, 9.1, 6.7, 1, -1, -1.f,
          frc::Transform3d(frc::Translation3d(4_m, 2_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          frc::Transform3d(frc::Translation3d(4_m, 2_m, 3_m),
                           frc::Rotation3d(0_rad, 0_rad, 0_rad)),
          0.3, corners, detectedCorners}};

  cameraOne.test = true;
  cameraOne.testResult = {photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt}};
  cameraOne.testResult[0].SetReceiveTimestamp(units::second_t(11));

  photon::PhotonPoseEstimator estimator(aprilTags, photon::LOWEST_AMBIGUITY,
                                        frc::Transform3d{});

  std::optional<photon::EstimatedRobotPose> estimatedPose;
  for (const auto& result : cameraOne.GetAllUnreadResults()) {
    estimatedPose = estimator.Update(result);
  }
  ASSERT_TRUE(estimatedPose);
  frc::Pose3d pose = estimatedPose.value().estimatedPose;

  // Make sure values match what we'd expect for the LOWEST_AMBIGUITY strategy
  EXPECT_NEAR(11, units::unit_cast<double>(estimatedPose.value().timestamp),
              .02);
  EXPECT_NEAR(1, units::unit_cast<double>(pose.X()), .01);
  EXPECT_NEAR(3, units::unit_cast<double>(pose.Y()), .01);
  EXPECT_NEAR(2, units::unit_cast<double>(pose.Z()), .01);
}

TEST(PhotonPoseEstimatorTest, CopyResult) {
  std::vector<photon::PhotonTrackedTarget> targets{};

  auto testResult = photon::PhotonPipelineResult{
      photon::PhotonPipelineMetadata{0, 0, 2000, 1000}, targets, std::nullopt};
  testResult.SetReceiveTimestamp(units::second_t(11));

  auto test2 = testResult;

  EXPECT_NEAR(testResult.GetTimestamp().to<double>(),
              test2.GetTimestamp().to<double>(), 0.001);
}
