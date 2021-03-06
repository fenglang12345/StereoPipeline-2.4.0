// __BEGIN_LICENSE__
//  Copyright (c) 2009-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NGT platform is licensed under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance with the
//  License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


/// \file BundleAdjustUtils.cc
///

#include <asp/Core/BundleAdjustUtils.h>

#include <vw/Core/Log.h>
#include <vw/Camera/CameraModel.h>
#include <vw/BundleAdjustment/ControlNetwork.h>
#include <vw/Stereo/StereoModel.h>

#include <string>

using namespace vw;
using namespace vw::camera;
using namespace vw::stereo;
using namespace vw::ba;

void read_adjustments(std::string const& filename,
                      Vector3& position_correction,
                      Quat& pose_correction) {
  Vector4 q_buffer;
  std::ifstream istr(filename.c_str());
  istr >> position_correction[0] >> position_correction[1]
       >> position_correction[2];
  istr >> q_buffer[0] >> q_buffer[1] >> q_buffer[2] >> q_buffer[3];
  pose_correction = Quat(q_buffer);
}

void write_adjustments(std::string const& filename,
                       Vector3 const& position_correction,
                       Quat const& pose_correction) {
  std::ofstream ostr(filename.c_str());
  ostr << position_correction[0] << " " << position_correction[1] << " " << position_correction[2] << "\n";
  ostr << pose_correction.w() << " " << pose_correction.x() << " "
       << pose_correction.y() << " " << pose_correction.z() << " " << "\n";
}

void compute_stereo_residuals(std::vector<boost::shared_ptr<CameraModel> > const& camera_models,
                              ControlNetwork const& cnet) {

  // Compute pre-adjustment residuals and convert to bundles
  int n = 0;
  double error_sum = 0;
  double min_error = ScalarTypeLimits<double>::highest();
  double max_error = ScalarTypeLimits<double>::lowest();
  for (size_t i = 0; i < cnet.size(); ++i) {
    for (size_t j = 0; j+1 < cnet[i].size(); ++j) {
      ++n;
      size_t cam1 = cnet[i][j].image_id();
      size_t cam2 = cnet[i][j+1].image_id();
      Vector2 pix1 = cnet[i][j].position();
      Vector2 pix2 = cnet[i][j+1].position();

      StereoModel sm( camera_models[cam1].get(),
                      camera_models[cam2].get() );
      double error;
      sm(pix1,pix2,error);
      error_sum += error;
      min_error = std::min(min_error, error);
      max_error = std::max(max_error, error);
    }
  }
  vw_out() << "\nStereo Intersection Residuals -- Min: " << min_error
           << "  Max: " << max_error << "  Average: " << (error_sum/n) << "\n";
}
