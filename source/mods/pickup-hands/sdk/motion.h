#ifndef MOON_PICKUP_HANDS_MOTION_H
#define MOON_PICKUP_HANDS_MOTION_H
/* Compatibility facade for the shared filter used by items and pets. */
#include "../../../runtime/sdk/pose_motion.h"
typedef MoonPoseVector MoonHoldVector;
typedef MoonPoseRotation MoonHoldRotation;
typedef MoonPose MoonHoldPose;
typedef MoonPoseTuning MoonHoldTuning;
#define moon_hold_finite moon_pose_finite
#define moon_hold_sqrt moon_pose_sqrt
#define moon_hold_normalize moon_pose_normalize
#define moon_hold_pose_valid moon_pose_valid
#define moon_hold_response moon_pose_response
#define moon_hold_step moon_pose_step
#endif
