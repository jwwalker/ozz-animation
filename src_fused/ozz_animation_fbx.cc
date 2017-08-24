// This file is autogenerated. Do not modify it.

// Including fbx.cc file.

//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/offline/fbx/fbx.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

FbxManagerInstance::FbxManagerInstance() : fbx_manager_(NULL) {
  // Instantiate Fbx manager, mostly a memory manager.
  fbx_manager_ = FbxManager::Create();

  // Logs SDK version.
  ozz::log::Log() << "FBX importer version " << fbx_manager_->GetVersion()
                  << "." << std::endl;
}

FbxManagerInstance::~FbxManagerInstance() {
  // Destroy the manager and all associated objects.
  fbx_manager_->Destroy();
  fbx_manager_ = NULL;
}

FbxDefaultIOSettings::FbxDefaultIOSettings(const FbxManagerInstance& _manager)
    : io_settings_(NULL) {
  io_settings_ = FbxIOSettings::Create(_manager, IOSROOT);
  io_settings_->SetBoolProp(IMP_FBX_MATERIAL, false);
  io_settings_->SetBoolProp(IMP_FBX_TEXTURE, false);
  io_settings_->SetBoolProp(IMP_FBX_MODEL, false);
  io_settings_->SetBoolProp(IMP_FBX_SHAPE, false);
  io_settings_->SetBoolProp(IMP_FBX_LINK, false);
  io_settings_->SetBoolProp(IMP_FBX_GOBO, false);
}

FbxDefaultIOSettings::~FbxDefaultIOSettings() {
  io_settings_->Destroy();
  io_settings_ = NULL;
}

FbxAnimationIOSettings::FbxAnimationIOSettings(
    const FbxManagerInstance& _manager)
    : FbxDefaultIOSettings(_manager) {}

FbxSkeletonIOSettings::FbxSkeletonIOSettings(const FbxManagerInstance& _manager)
    : FbxDefaultIOSettings(_manager) {
  settings()->SetBoolProp(IMP_FBX_ANIMATION, false);
}

FbxSceneLoader::FbxSceneLoader(const char* _filename, const char* _password,
                               const FbxManagerInstance& _manager,
                               const FbxDefaultIOSettings& _io_settings)
    : scene_(NULL), converter_(NULL) {
  // Create an importer.
  FbxImporter* importer = FbxImporter::Create(_manager, "ozz file importer");
  const bool initialized = importer->Initialize(_filename, -1, _io_settings);

  ImportScene(importer, initialized, _password, _manager, _io_settings);

  // Destroy the importer
  importer->Destroy();
}

FbxSceneLoader::FbxSceneLoader(FbxStream* _stream, const char* _password,
                               const FbxManagerInstance& _manager,
                               const FbxDefaultIOSettings& _io_settings)
    : scene_(NULL), converter_(NULL) {
  // Create an importer.
  FbxImporter* importer = FbxImporter::Create(_manager, "ozz stream importer");
  const bool initialized =
      importer->Initialize(_stream, NULL, -1, _io_settings);

  ImportScene(importer, initialized, _password, _manager, _io_settings);

  // Destroy the importer
  importer->Destroy();
}

void FbxSceneLoader::ImportScene(FbxImporter* _importer,
                                 const bool _initialized, const char* _password,
                                 const FbxManagerInstance& _manager,
                                 const FbxDefaultIOSettings& _io_settings) {
  // Get the version of the FBX file format.
  int major, minor, revision;
  _importer->GetFileVersion(major, minor, revision);

  if (!_initialized)  // Problem with the file to be imported.
  {
    const FbxString error = _importer->GetStatus().GetErrorString();
    ozz::log::Err() << "FbxImporter initialization failed with error: "
                    << error.Buffer() << std::endl;

    if (_importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion) {
      ozz::log::Err() << "FBX file version is " << major << "." << minor << "."
                      << revision << "." << std::endl;
    }
  } else {
    if (_importer->IsFBX()) {
      ozz::log::Log() << "FBX file version is " << major << "." << minor << "."
                      << revision << "." << std::endl;
    }

    // Load the scene.
    scene_ = FbxScene::Create(_manager, "ozz scene");
    bool imported = _importer->Import(scene_);

    if (!imported &&  // The import file may have a password
        _importer->GetStatus().GetCode() == FbxStatus::ePasswordError) {
      _io_settings.settings()->SetStringProp(IMP_FBX_PASSWORD, _password);
      _io_settings.settings()->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

      // Retries to import the scene.
      imported = _importer->Import(scene_);

      if (!imported &&
          _importer->GetStatus().GetCode() == FbxStatus::ePasswordError) {
        ozz::log::Err() << "Incorrect password." << std::endl;

        // scene_ will be destroyed because imported is false.
      }
    }

    // Setup axis and system converter.
    if (imported) {
      FbxGlobalSettings& settings = scene_->GetGlobalSettings();
      converter_ = ozz::memory::default_allocator()->New<FbxSystemConverter>(
          settings.GetAxisSystem(), settings.GetSystemUnit());
    }

    // Clear the scene if import failed.
    if (!imported) {
      scene_->Destroy();
      scene_ = NULL;
    }
  }
}

FbxSceneLoader::~FbxSceneLoader() {
  if (scene_) {
    scene_->Destroy();
    scene_ = NULL;
  }

  if (converter_) {
    ozz::memory::default_allocator()->Delete(converter_);
    converter_ = NULL;
  }
}

namespace {
ozz::math::Float4x4 BuildAxisSystemMatrix(const FbxAxisSystem& _system) {
  int sign;
  ozz::math::SimdFloat4 up = ozz::math::simd_float4::y_axis();
  ozz::math::SimdFloat4 at = ozz::math::simd_float4::z_axis();

  // The EUpVector specifies which axis has the up and down direction in the
  // system (typically this is the Y or Z axis). The sign of the EUpVector is
  // applied to represent the direction (1 is up and -1 is down relative to the
  // observer).
  const FbxAxisSystem::EUpVector eup = _system.GetUpVector(sign);
  switch (eup) {
    case FbxAxisSystem::eXAxis: {
      up = math::simd_float4::Load(1.f * sign, 0.f, 0.f, 0.f);
      // If the up axis is X, the remain two axes will be Y And Z, so the
      // ParityEven is Y, and the ParityOdd is Z
      if (_system.GetFrontVector(sign) == FbxAxisSystem::eParityEven) {
        at = math::simd_float4::Load(0.f, 1.f * sign, 0.f, 0.f);
      } else {
        at = math::simd_float4::Load(0.f, 0.f, 1.f * sign, 0.f);
      }
      break;
    }
    case FbxAxisSystem::eYAxis: {
      up = math::simd_float4::Load(0.f, 1.f * sign, 0.f, 0.f);
      // If the up axis is Y, the remain two axes will X And Z, so the
      // ParityEven is X, and the ParityOdd is Z
      if (_system.GetFrontVector(sign) == FbxAxisSystem::eParityEven) {
        at = math::simd_float4::Load(1.f * sign, 0.f, 0.f, 0.f);
      } else {
        at = math::simd_float4::Load(0.f, 0.f, 1.f * sign, 0.f);
      }
      break;
    }
    case FbxAxisSystem::eZAxis: {
      up = math::simd_float4::Load(0.f, 0.f, 1.f * sign, 0.f);
      // If the up axis is Z, the remain two axes will X And Y, so the
      // ParityEven is X, and the ParityOdd is Y
      if (_system.GetFrontVector(sign) == FbxAxisSystem::eParityEven) {
        at = math::simd_float4::Load(1.f * sign, 0.f, 0.f, 0.f);
      } else {
        at = math::simd_float4::Load(0.f, 1.f * sign, 0.f, 0.f);
      }
      break;
    }
    default: {
      assert(false && "Invalid FbxAxisSystem");
      break;
    }
  }

  // If the front axis and the up axis are determined, the third axis will be
  // automatically determined as the left one. The ECoordSystem enum is a
  // parameter to determine the direction of the third axis just as the
  // EUpVector sign. It determines if the axis system is right-handed or
  // left-handed just as the enum values.
  ozz::math::SimdFloat4 right;
  if (_system.GetCoorSystem() == FbxAxisSystem::eRightHanded) {
    right = math::Cross3(up, at);
  } else {
    right = math::Cross3(at, up);
  }

  const ozz::math::Float4x4 matrix = {
      {right, up, at, ozz::math::simd_float4::w_axis()}};

  return matrix;
}
}  // namespace

FbxSystemConverter::FbxSystemConverter(const FbxAxisSystem& _from_axis,
                                       const FbxSystemUnit& _from_unit) {
  // Build axis system conversion matrix.
  const math::Float4x4 from_matrix = BuildAxisSystemMatrix(_from_axis);

  // Finds unit conversion ratio to meters, where GetScaleFactor() is in cm.
  const float to_meters =
      static_cast<float>(_from_unit.GetScaleFactor()) * .01f;

  // Builds conversion matrices.
  convert_ = Invert(from_matrix) *
             math::Float4x4::Scaling(math::simd_float4::Load1(to_meters));
  inverse_convert_ = Invert(convert_);
  inverse_transpose_convert_ = Transpose(inverse_convert_);
}

math::Float4x4 FbxSystemConverter::ConvertMatrix(const FbxAMatrix& _m) const {
  const ozz::math::Float4x4 m = {{
      ozz::math::simd_float4::Load(
          static_cast<float>(_m[0][0]), static_cast<float>(_m[0][1]),
          static_cast<float>(_m[0][2]), static_cast<float>(_m[0][3])),
      ozz::math::simd_float4::Load(
          static_cast<float>(_m[1][0]), static_cast<float>(_m[1][1]),
          static_cast<float>(_m[1][2]), static_cast<float>(_m[1][3])),
      ozz::math::simd_float4::Load(
          static_cast<float>(_m[2][0]), static_cast<float>(_m[2][1]),
          static_cast<float>(_m[2][2]), static_cast<float>(_m[2][3])),
      ozz::math::simd_float4::Load(
          static_cast<float>(_m[3][0]), static_cast<float>(_m[3][1]),
          static_cast<float>(_m[3][2]), static_cast<float>(_m[3][3])),
  }};
  return convert_ * m * inverse_convert_;
}

math::Float3 FbxSystemConverter::ConvertPoint(const FbxVector4& _p) const {
  const math::SimdFloat4 p_in = math::simd_float4::Load(
      static_cast<float>(_p[0]), static_cast<float>(_p[1]),
      static_cast<float>(_p[2]), 1.f);
  const math::SimdFloat4 p_out = convert_ * p_in;
  math::Float3 ret;
  math::Store3PtrU(p_out, &ret.x);
  return ret;
}

math::Float3 FbxSystemConverter::ConvertNormal(const FbxVector4& _p) const {
  const math::SimdFloat4 p_in = math::simd_float4::Load(
      static_cast<float>(_p[0]), static_cast<float>(_p[1]),
      static_cast<float>(_p[2]), 0.f);
  const math::SimdFloat4 p_out = inverse_transpose_convert_ * p_in;
  math::Float3 ret;
  math::Store3PtrU(p_out, &ret.x);
  return ret;
}

bool FbxSystemConverter::ConvertTransform(const FbxAMatrix& _m,
                                          math::Transform* _transform) const {
  assert(_transform);

  const math::Float4x4 matrix = ConvertMatrix(_m);

  math::SimdFloat4 translation, rotation, scale;
  if (ToAffine(matrix, &translation, &rotation, &scale)) {
    ozz::math::Transform transform;
    math::Store3PtrU(translation, &_transform->translation.x);
    math::StorePtrU(math::Normalize4(rotation), &_transform->rotation.x);
    math::Store3PtrU(scale, &_transform->scale.x);
    return true;
  }

  // Failed to decompose matrix, reset transform to identity.
  *_transform = ozz::math::Transform::identity();
  return false;
}
}  // namespace fbx
}  // namespace offline
}  // namespace animation
}  // namespace ozz

// Including fbx_animation.cc file.

//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/offline/fbx/fbx_animation.h"

#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_track.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include "ozz/base/log.h"

#include "ozz/base/maths/transform.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

namespace {
struct SamplingInfo {
  float start;
  float end;
  float duration;
  float period;
};

SamplingInfo ExtractSamplingInfo(FbxScene* _scene, FbxAnimStack* _anim_stack,
                                 float _sampling_rate) {
  SamplingInfo info;

  // Extract animation duration.
  FbxTimeSpan time_spawn;
  const FbxTakeInfo* take_info = _scene->GetTakeInfo(_anim_stack->GetName());
  if (take_info) {
    time_spawn = take_info->mLocalTimeSpan;
  } else {
    _scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(time_spawn);
  }

  // Get frame rate from the scene.
  FbxTime::EMode mode = _scene->GetGlobalSettings().GetTimeMode();
  const float scene_frame_rate =
      static_cast<float>((mode == FbxTime::eCustom)
                             ? _scene->GetGlobalSettings().GetCustomFrameRate()
                             : FbxTime::GetFrameRate(mode));

  // Deduce sampling period.
  // Scene frame rate is used when provided argument is <= 0.
  float sampling_rate;
  if (_sampling_rate > 0.f) {
    sampling_rate = _sampling_rate;
    log::LogV() << "Using sampling rate of " << sampling_rate << "hz."
                << std::endl;
  } else {
    sampling_rate = scene_frame_rate;
    log::LogV() << "Using scene sampling rate of " << sampling_rate << "hz."
                << std::endl;
  }
  info.period = 1.f / sampling_rate;

  // Get scene start and end.
  info.start = static_cast<float>(time_spawn.GetStart().GetSecondDouble());
  info.end = static_cast<float>(time_spawn.GetStop().GetSecondDouble());

  // Duration could be 0 if it's just a pose. In this case we'll set a default
  // 1s duration.
  if (info.end > info.start) {
    info.duration = info.end - info.start;
  } else {
    info.duration = 1.f;
  }

  return info;
}

bool ExtractAnimation(FbxSceneLoader& _scene_loader, const SamplingInfo& _info,
                      const Skeleton& _skeleton, RawAnimation* _animation) {
  FbxScene* scene = _scene_loader.scene();
  assert(scene);

  // Set animation data.
  _animation->duration = _info.duration;

  // Allocates all tracks with the same number of joints as the skeleton.
  // Tracks that would not be found will be set to skeleton bind-pose
  // transformation.
  _animation->tracks.resize(_skeleton.num_joints());

  // Iterate all skeleton joints and fills there track with key frames.
  FbxAnimEvaluator* evaluator = scene->GetAnimationEvaluator();
  for (int i = 0; i < _skeleton.num_joints(); i++) {
    RawAnimation::JointTrack& track = _animation->tracks[i];

    // Find a node that matches skeleton joint.
    const char* joint_name = _skeleton.joint_names()[i];
    FbxNode* node = scene->FindNodeByName(joint_name);

    if (!node) {
      // Empty joint track.
      ozz::log::LogV() << "No animation track found for joint \"" << joint_name
                       << "\". Using skeleton bind pose instead." << std::endl;

      // Get joint's bind pose.
      const ozz::math::Transform& bind_pose =
          ozz::animation::GetJointLocalBindPose(_skeleton, i);

      const RawAnimation::TranslationKey tkey = {0.f, bind_pose.translation};
      track.translations.push_back(tkey);

      const RawAnimation::RotationKey rkey = {0.f, bind_pose.rotation};
      track.rotations.push_back(rkey);

      const RawAnimation::ScaleKey skey = {0.f, bind_pose.scale};
      track.scales.push_back(skey);

      continue;
    }

    // Reserve keys in animation tracks (allocation strategy optimization
    // purpose).
    const int max_keys =
        static_cast<int>(3.f + (_info.end - _info.start) / _info.period);
    track.translations.reserve(max_keys);
    track.rotations.reserve(max_keys);
    track.scales.reserve(max_keys);

    // Evaluate joint transformation at the specified time.
    // Make sure to include "end" time, and enter the loop once at least.
    bool loop_again = true;
    for (float t = _info.start; loop_again; t += _info.period) {
      if (t >= _info.end) {
        t = _info.end;
        loop_again = false;
      }

      // Evaluate transform matrix at t.
      const FbxAMatrix matrix =
          _skeleton.joint_properties()[i].parent == Skeleton::kNoParentIndex
              ? evaluator->GetNodeGlobalTransform(node, FbxTimeSeconds(t))
              : evaluator->GetNodeLocalTransform(node, FbxTimeSeconds(t));

      // Convert to a transform object in ozz unit/axis system.
      ozz::math::Transform transform;
      if (!_scene_loader.converter()->ConvertTransform(matrix, &transform)) {
        ozz::log::Err() << "Failed to extract animation transform for joint \""
                        << joint_name << "\" at t = " << t << "s." << std::endl;
        return false;
      }

      // Fills corresponding track.
      const float local_time = t - _info.start;
      const RawAnimation::TranslationKey translation = {local_time,
                                                        transform.translation};
      track.translations.push_back(translation);
      const RawAnimation::RotationKey rotation = {local_time,
                                                  transform.rotation};
      track.rotations.push_back(rotation);
      const RawAnimation::ScaleKey scale = {local_time, transform.scale};
      track.scales.push_back(scale);
    }
  }

  return _animation->Validate();
}

bool GetValue(FbxPropertyValue& _property_value, EFbxType _type,
              float* _value) {
  switch (_type) {
    case eFbxBool: {
      bool value;
      bool success = _property_value.Get(&value, eFbxBool);
      *_value = value ? 1.f : 0.f;
      return success;
    }
    case eFbxInt: {
      int value;
      bool success = _property_value.Get(&value, eFbxInt);
      *_value = static_cast<float>(value);
      return success;
    }
    case eFbxFloat: {
      return _property_value.Get(_value, eFbxFloat);
    }
    case eFbxDouble: {
      double value;
      bool success = _property_value.Get(&value, eFbxDouble);
      *_value = static_cast<float>(value);
      return success;
    }
    default: {
      assert(false);
      return false;
    }
  }
}

bool GetValue(FbxPropertyValue& _property_value, EFbxType _type,
              ozz::math::Float2* _value) {
  (void)_type;
  assert(_type == eFbxDouble2);
  double dvalue[2];
  if (!_property_value.Get(&dvalue, eFbxDouble2)) {
    return false;
  }
  _value->x = static_cast<float>(dvalue[0]);
  _value->y = static_cast<float>(dvalue[1]);

  return true;
}

bool GetValue(FbxPropertyValue& _property_value, EFbxType _type,
              ozz::math::Float3* _value) {
  (void)_type;
  assert(_type == eFbxDouble3);
  double dvalue[3];
  if (!_property_value.Get(&dvalue, eFbxDouble3)) {
    return false;
  }
  _value->x = static_cast<float>(dvalue[0]);
  _value->y = static_cast<float>(dvalue[1]);
  _value->z = static_cast<float>(dvalue[2]);

  return true;
}

template <typename _Track>
bool ExtractCurve(FbxSceneLoader& _scene_loader, FbxProperty& _property,
                  EFbxType _type, const SamplingInfo& _info, _Track* _track) {
  assert(_track->keyframes.size() == 0);

  FbxScene* scene = _scene_loader.scene();
  assert(scene);

  FbxAnimEvaluator* evaluator = scene->GetAnimationEvaluator();

  if (_property.IsAnimated()) {
    FbxPropertyValue& property_value =
        evaluator->GetPropertyValue(_property, FbxTimeSeconds(0.));

    typename _Track::ValueType value;
    bool success = GetValue(property_value, _type, &value);
    if (!success) {
      return false;
    }

    // Build and push keyframe
    const typename _Track::Keyframe key = {RawTrackInterpolation::kStep, 0.f,
                                           value};
    _track->keyframes.push_back(key);
  } else {
    // Reserve keys
    const int max_keys =
        static_cast<int>(3.f + (_info.end - _info.start) / _info.period);
    _track->keyframes.reserve(max_keys);

    // Evaluate values at the specified time.
    // Make sure to include "end" time, and enter the loop once at least.
    bool loop_again = true;
    for (float t = _info.start; loop_again; t += _info.period) {
      if (t >= _info.end) {
        t = _info.end;
        loop_again = false;
      }

      FbxPropertyValue& property_value =
          evaluator->GetPropertyValue(_property, FbxTimeSeconds(t));

      // It shouldn't fail as property type is known.
      typename _Track::ValueType value;
      bool success = GetValue(property_value, _type, &value);
      (void)success;
      assert(success);

      // Build and push keyframe
      const typename _Track::Keyframe key = {RawTrackInterpolation::kLinear,
                                             (t - _info.start) / _info.duration,
                                             value};
      _track->keyframes.push_back(key);
    }
  }

  return _track->Validate();
}

const char* FbxTypeToString(EFbxType _type) {
  switch (_type) {
    case eFbxUndefined:
      return "eFbxUndefined - Unidentified";
    case eFbxChar:
      return "eFbxChar - 8 bit signed integer";
    case eFbxUChar:
      return "eFbxUChar - 8 bit unsigned integer";
    case eFbxShort:
      return "eFbxShort - 16 bit signed integer";
    case eFbxUShort:
      return "eFbxUShort - 16 bit unsigned integer";
    case eFbxUInt:
      return "eFbxUInt - 32 bit unsigned integer";
    case eFbxLongLong:
      return "eFbxLongLong - 64 bit signed integer";
    case eFbxULongLong:
      return "eFbxULongLong - 64 bit unsigned integer";
    case eFbxHalfFloat:
      return "eFbxHalfFloat - 16 bit floating point";
    case eFbxBool:
      return "eFbxBool - Boolean";
    case eFbxInt:
      return "eFbxInt - 32 bit signed integer";
    case eFbxFloat:
      return "eFbxFloat - Floating point value";
    case eFbxDouble:
      return "eFbxDouble - Double width floating point value";
    case eFbxDouble2:
      return "eFbxDouble2 - Vector of two double values";
    case eFbxDouble3:
      return "eFbxDouble3 - Vector of three double values";
    case eFbxDouble4:
      return "eFbxDouble4 - Vector of four double values";
    case eFbxDouble4x4:
      return "eFbxDouble4x4 - Four vectors of four double values";
    case eFbxEnum:
      return "eFbxEnum - Enumeration";
    case eFbxEnumM:
      return "eFbxEnumM - Enumeration allowing duplicated items";
    case eFbxString:
      return "eFbxString - String";
    case eFbxTime:
      return "eFbxTime - Time value";
    case eFbxReference:
      return "eFbxReference - Reference to object or property";
    case eFbxBlob:
      return "eFbxBlob - Binary data block type";
    case eFbxDistance:
      return "eFbxDistance - Distance";
    case eFbxDateTime:
      return "eFbxDateTime - Date and time";
    default:
      return "Unknown";
  }
}

bool ExtractProperty(FbxSceneLoader& _scene_loader, const SamplingInfo& _info,
                     FbxProperty& _property, RawFloatTrack* _track) {
  const EFbxType type = _property.GetPropertyDataType().GetType();
  switch (type) {
    case eFbxBool:
    case eFbxInt:
    case eFbxFloat:
    case eFbxDouble: {
      return ExtractCurve(_scene_loader, _property, type, _info, _track);
    }
    default: {
      log::Err() << "Float track can't be imported from a track of type: "
                 << FbxTypeToString(type) << "\"" << std::endl;
      return false;
    }
  }
}

bool ExtractProperty(FbxSceneLoader& _scene_loader, const SamplingInfo& _info,
                     FbxProperty& _property, RawFloat2Track* _track) {
  const EFbxType type = _property.GetPropertyDataType().GetType();
  switch (type) {
    case eFbxDouble2: {
      return ExtractCurve(_scene_loader, _property, type, _info, _track);
    }
    default: {
      log::Err() << "Float track can't be imported from a track of type: "
                 << FbxTypeToString(type) << "\"" << std::endl;
      return false;
    }
  }
}

bool ExtractProperty(FbxSceneLoader& _scene_loader, const SamplingInfo& _info,
                     FbxProperty& _property, RawFloat3Track* _track) {
  const EFbxType type = _property.GetPropertyDataType().GetType();
  switch (type) {
    case eFbxDouble3: {
      return ExtractCurve(_scene_loader, _property, type, _info, _track);
    }
    default: {
      log::Err() << "Float track can't be imported from a track of type: "
                 << FbxTypeToString(type) << "\"" << std::endl;
      return false;
    }
  }
}

template <typename _Track>
bool ExtractTrackImpl(const char* _animation_name, const char* _node_name,
                      const char* _track_name, FbxSceneLoader& _scene_loader,
                      float _sampling_rate, _Track* _track) {
  FbxScene* scene = _scene_loader.scene();
  assert(scene);

  // Reset track
  *_track = _Track();

  FbxAnimStack* anim_stack =
      scene->FindSrcObject<FbxAnimStack>(_animation_name);
  if (!anim_stack) {
    return false;
  }

  // Extract sampling info relative to the stack.
  const SamplingInfo& info =
      ExtractSamplingInfo(scene, anim_stack, _sampling_rate);

  ozz::log::Log() << "Extracting animation track \"" << _node_name << ":"
                  << _track_name << "\"" << std::endl;

  FbxNode* node = scene->FindNodeByName(_node_name);
  if (!node) {
    ozz::log::Err() << "Invalid node name \"" << _node_name << "\""
                    << std::endl;
    return false;
  }

  FbxProperty property = node->FindProperty(_track_name);
  if (!property.IsValid()) {
    ozz::log::Err() << "Invalid property name \"" << _track_name << "\""
                    << std::endl;
    return false;
  }

  return ExtractProperty(_scene_loader, info, property, _track);
}
}  // namespace

AnimationNames GetAnimationNames(FbxSceneLoader& _scene_loader) {
  AnimationNames names;

  const FbxScene* scene = _scene_loader.scene();
  for (int i = 0; i < scene->GetSrcObjectCount<FbxAnimStack>(); ++i) {
    FbxAnimStack* anim_stack = scene->GetSrcObject<FbxAnimStack>(i);
    names.push_back(ozz::String::Std(anim_stack->GetName()));
  }

  return names;
}

bool ExtractAnimation(const char* _animation_name,
                      FbxSceneLoader& _scene_loader, const Skeleton& _skeleton,
                      float _sampling_rate,
                      ozz::animation::offline::RawAnimation* _animation) {
  FbxScene* scene = _scene_loader.scene();
  assert(scene);

  bool success = false;

  FbxAnimStack* anim_stack =
      scene->FindSrcObject<FbxAnimStack>(_animation_name);
  if (anim_stack) {
    // Extract sampling info relative to the stack.
    const SamplingInfo& info =
        ExtractSamplingInfo(scene, anim_stack, _sampling_rate);

    ozz::log::Log() << "Extracting animation \"" << anim_stack->GetName()
                    << "\"" << std::endl;

    // Setup Fbx animation evaluator.
    scene->SetCurrentAnimationStack(anim_stack);

    _animation->name = anim_stack->GetName();
    success = ExtractAnimation(_scene_loader, info, _skeleton, _animation);
  }

  // Clears output if something failed during import, avoids partial data.
  if (!success) {
    *_animation = ozz::animation::offline::RawAnimation();
  }

  return success;
}

bool ExtractTrack(const char* _animation_name, const char* _node_name,
                  const char* _track_name, FbxSceneLoader& _scene_loader,
                  float _sampling_rate, RawFloatTrack* _track) {
  return ExtractTrackImpl(_animation_name, _node_name, _track_name,
                          _scene_loader, _sampling_rate, _track);
}

bool ExtractTrack(const char* _animation_name, const char* _node_name,
                  const char* _track_name, FbxSceneLoader& _scene_loader,
                  float _sampling_rate, RawFloat2Track* _track) {
  return ExtractTrackImpl(_animation_name, _node_name, _track_name,
                          _scene_loader, _sampling_rate, _track);
}

bool ExtractTrack(const char* _animation_name, const char* _node_name,
                  const char* _track_name, FbxSceneLoader& _scene_loader,
                  float _sampling_rate, RawFloat3Track* _track) {
  return ExtractTrackImpl(_animation_name, _node_name, _track_name,
                          _scene_loader, _sampling_rate, _track);
}
}  // namespace fbx
}  // namespace offline
}  // namespace animation
}  // namespace ozz

// Including fbx_skeleton.cc file.

//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/offline/fbx/fbx_skeleton.h"

#include "ozz/animation/offline/raw_skeleton.h"

#include "ozz/base/log.h"

namespace ozz {
namespace animation {
namespace offline {
namespace fbx {

namespace {

enum RecurseReturn { kError, kSkeletonFound, kNoSkeleton };

RecurseReturn RecurseNode(FbxNode* _node, FbxSystemConverter* _converter,
                          RawSkeleton* _skeleton, RawSkeleton::Joint* _parent,
                          int _depth) {
  bool skeleton_found = false;
  RawSkeleton::Joint* this_joint = NULL;

  bool process_node = false;

  // Push this node if it's below a skeleton root (aka has a parent).
  process_node |= _parent != NULL;

  // Push this node as a new joint if it has a joint compatible attribute.
  FbxNodeAttribute* node_attribute = _node->GetNodeAttribute();
  process_node |=
      node_attribute &&
      node_attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton;

  // Process node if required.
  if (process_node) {
    skeleton_found = true;

    RawSkeleton::Joint::Children* sibling = NULL;
    if (_parent) {
      sibling = &_parent->children;
    } else {
      sibling = &_skeleton->roots;
    }

    // Adds a new child.
    sibling->resize(sibling->size() + 1);
    this_joint = &sibling->back();  // Will not be resized inside recursion.
    this_joint->name = _node->GetName();

    // Outputs hierarchy on verbose stream.
    for (int i = 0; i < _depth; ++i) {
      ozz::log::LogV() << '.';
    }
    ozz::log::LogV() << this_joint->name.c_str() << std::endl;

    // Extract bind pose.
    const FbxAMatrix matrix = _parent ? _node->EvaluateLocalTransform()
                                      : _node->EvaluateGlobalTransform();
    if (!_converter->ConvertTransform(matrix, &this_joint->transform)) {
      ozz::log::Err() << "Failed to extract skeleton transform for joint \""
                      << this_joint->name << "\"." << std::endl;
      return kError;
    }

    // One level deeper in the hierarchy.
    _depth++;
  }

  // Iterate node's children.
  for (int i = 0; i < _node->GetChildCount(); i++) {
    FbxNode* child = _node->GetChild(i);
    const RecurseReturn ret =
        RecurseNode(child, _converter, _skeleton, this_joint, _depth);
    if (ret == kError) {
      return ret;
    }
    skeleton_found |= (ret == kSkeletonFound);
  }

  return skeleton_found ? kSkeletonFound : kNoSkeleton;
}
}  // namespace

bool ExtractSkeleton(FbxSceneLoader& _loader, RawSkeleton* _skeleton) {
  RecurseReturn ret = RecurseNode(_loader.scene()->GetRootNode(),
                                  _loader.converter(), _skeleton, NULL, 0);
  if (ret == kNoSkeleton) {
    ozz::log::Err() << "No skeleton found in Fbx scene." << std::endl;
    return false;
  } else if (ret == kError) {
    ozz::log::Err() << "Failed to extract skeleton." << std::endl;
    return false;
  }
  return true;
}
}  // namespace fbx
}  // namespace offline
}  // namespace animation
}  // namespace ozz

