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


// These are objects that relate directly to block in XML that we need
// to read. They only read and then store the raw values. Other
// objects will interpret the results.

#ifndef __STEREO_SESSION_DG_XML_H__
#define __STEREO_SESSION_DG_XML_H__

#include <vw/Core/FundamentalTypes.h>
#include <vw/Math/Vector.h>
#include <vw/Math/Quaternion.h>
#include <asp/Sessions/DG/XMLBase.h>

#include <vector>
#include <string>

#include <boost/smart_ptr/scoped_ptr.hpp>

namespace asp {

  // Forward declare so that we can cut down on the headers.
  class RPCModel;

  // Objects that represent read data from XML. These also provide a
  // storage structure for modification later on.
  class ImageXML : public XMLBase {

    void parse_meta( xercesc::DOMElement* node );
    void parse_band_p( xercesc::DOMElement* node );
    void parse_tlc_list( xercesc::DOMElement* node );
    void parse_image_size( xercesc::DOMElement* node );

  public:
    ImageXML();

    void parse( xercesc::DOMElement* node );

    std::string  tlc_start_time;
    std::string  first_line_start_time;
    std::vector<std::pair<double,double> > tlc_vec; // Line -> time offset pairings
    std::string  sat_id;
    std::string  scan_direction;
    int          tdi;
    double       avg_line_rate;
    vw::Vector2i image_size;
  };

  class GeometricXML : public XMLBase {

    void parse_principal_distance( xercesc::DOMElement* node );
    void parse_optical_distortion( xercesc::DOMElement* node );
    void parse_perspective_center( xercesc::DOMElement* node );
    void parse_camera_attitude( xercesc::DOMElement* node );
    void parse_detector_mounting( xercesc::DOMElement* node );

  public:
    GeometricXML();

    void parse( xercesc::DOMElement* node );

    double principal_distance;   // mm
    vw::int32 optical_polyorder;
    vw::Vector<double> optical_a, optical_b; // Don't currently support these
    vw::Vector3 perspective_center;  // meters in spacecraft frame
    vw::Quat camera_attitude;
    vw::Vector2 detector_origin;     // mm
    double detector_rotation;    // degrees about Z+ in camera frame
    double detector_pixel_pitch; // mm
  };

  class EphemerisXML : public XMLBase {

    void parse_meta( xercesc::DOMElement* node );
    void parse_eph_list( xercesc::DOMElement* node );

  public:
    EphemerisXML();

    void parse( xercesc::DOMElement* node );

    std::string start_time;      // UTC
    double time_interval;        // seconds
    std::vector<vw::Vector3> position_vec, velocity_vec; // ECEF
    std::vector<vw::Vector<double,6> > covariance_vec;   // Tri-diagonal
  };

  class AttitudeXML : public XMLBase {

    void parse_meta( xercesc::DOMElement* node );
    void parse_att_list( xercesc::DOMElement* node );

  public:
    AttitudeXML();

    void parse( xercesc::DOMElement* node );

    std::string start_time;
    double time_interval;
    std::vector<vw::Quat> quat_vec;
    std::vector<vw::Vector<double,10> > covariance_vec;
  };

  // Reads from Digital Globe XML format
  class RPCXML : public XMLBase {
    boost::scoped_ptr<RPCModel> m_rpc;
    void parse_vector( xercesc::DOMElement* node,
                       vw::Vector<double,20> & vec );

    void parse_rpb( xercesc::DOMElement* node ); // Digital Globe XML
    void parse_rational_function_model( xercesc::DOMElement* node ); // Pleiades / Astrium

  public:
    RPCXML();

    void read_from_file( std::string const& name );
    void parse( xercesc::DOMElement* node ) { parse_rpb( node ); }

    RPCModel* rpc_ptr() const;
  };

  // Helper functions to allow us to fill the objects. This doesn't
  // really help with code reuse but I think it makes it easer to
  // read.
  void read_xml( std::string const& filename,
                 GeometricXML& geo,
                 AttitudeXML& att,
                 EphemerisXML& eph,
                 ImageXML& img,
                 RPCXML& rpc );
  vw::Vector2i xml_image_size( std::string const& filename );

} //end namespace asp

#endif//__STEREO_SESSION_DG_XML_H__
