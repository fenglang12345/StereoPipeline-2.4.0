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

#include <vw/Core/Exception.h>          // for ArgumentErr, vw_throw, etc
#include <vw/Math/Quaternion.h>         // for Quat, Quaternion
#include <vw/Math/Vector.h>             // for Vector, Vector3, Vector4, etc
#include <vw/Cartography/Datum.h>       // for Datum

#include <asp/Sessions/DG/XML.h>
#include <asp/Sessions/RPC/RPCModel.h>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace vw;
using namespace xercesc;

namespace fs=boost::filesystem;

void asp::ImageXML::parse_meta( xercesc::DOMElement* node ) {
  // Note: dg_mosaic wipes most image tags. If it is desired to parse
  // more tags here, ensure they are not wiped by dg_mosaic and use
  // sensible values when creating them for the combined mosaics.
  cast_xmlch( get_node<DOMElement>( node, "SATID" )->getTextContent(),
              sat_id );
  cast_xmlch( get_node<DOMElement>( node, "SCANDIRECTION" )->getTextContent(),
              scan_direction );
  cast_xmlch( get_node<DOMElement>( node, "TLCTIME" )->getTextContent(),
              tlc_start_time );
  cast_xmlch( get_node<DOMElement>( node, "FIRSTLINETIME" )->getTextContent(),
              first_line_start_time );
  cast_xmlch( get_node<DOMElement>( node, "AVGLINERATE" )->getTextContent(),
              avg_line_rate );
  size_t num_tlc;
  cast_xmlch( get_node<DOMElement>( node, "NUMTLC" )->getTextContent(),
              num_tlc );
  tlc_vec.resize( num_tlc );
}

void asp::ImageXML::parse_band_p( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "TDILEVEL" )->getTextContent(),
              tdi );
}

void asp::ImageXML::parse_tlc_list( xercesc::DOMElement* node ) {
  DOMNodeList* children = node->getChildNodes();
  size_t count = 0;

  for ( XMLSize_t i = 0; i < children->getLength(); i++ ) {
    if ( children->item(i)->getNodeType() == DOMNode::ELEMENT_NODE ) {
      DOMElement* element = dynamic_cast<DOMElement*>( children->item(i) );
      std::string buffer;
      cast_xmlch( element->getTextContent(), buffer );

      std::istringstream istr( buffer );
      istr >> tlc_vec[count].first >> tlc_vec[count].second;

      count++;
    }
  }

  VW_ASSERT( count == tlc_vec.size(),
             IOErr() << "Read incorrect number of TLC." );
}

void asp::ImageXML::parse_image_size( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "NUMROWS" )->getTextContent(),
              image_size[1] );
  cast_xmlch( get_node<DOMElement>( node, "NUMCOLUMNS" )->getTextContent(),
              image_size[0] );
}

asp::ImageXML::ImageXML() : XMLBase(4) {}

void asp::ImageXML::parse( xercesc::DOMElement* node ) {
  DOMElement* image = get_node<DOMElement>( node, "IMAGE" );
  parse_meta( image );
  check_argument(0);

  tdi = 0;
  try{
    DOMElement* band_p = get_node<DOMElement>( node, "BAND_P" );
    parse_band_p( band_p );
  }catch(...){
    // An XML file after being processed by dg_mosaic may not have the tdi field.
  }
  check_argument(1);

  parse_tlc_list( get_node<DOMElement>( image, "TLCLISTList" ) );
  check_argument(2);

  parse_image_size( node );
  check_argument(3);
}

void asp::GeometricXML::parse_principal_distance( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "PD" )->getTextContent(),
              principal_distance );
}

void asp::GeometricXML::parse_optical_distortion( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "POLYORDER" )->getTextContent(),
              optical_polyorder );

  // If the optical polyorder is >= 1 and some of the polynomial
  // coefficients are non-zero, then throw an error as we did not
  // implement optical distortion.

  if ( optical_polyorder <= 0 ) return; // it actually can be negative

  std::string list_list_types[] = {"ALISTList", "BLISTList"};
  for (unsigned ls = 0; ls < sizeof(list_list_types)/sizeof(std::string); ls++){
    std::string list_list_type = list_list_types[ls];

    DOMElement* list_list = get_node<DOMElement>( node, list_list_type );
    if (list_list == NULL) return;

    DOMNodeList* children = list_list->getChildNodes();
    if (children == NULL) return;

    for ( XMLSize_t i = 0; i < children->getLength(); ++i ) {

      DOMNode* current = children->item(i);
      if ( current->getNodeType() != DOMNode::ELEMENT_NODE ) continue;

      DOMElement* element = dynamic_cast< xercesc::DOMElement* >( current );
      std::string buffer;
      cast_xmlch( element->getTextContent(), buffer );

      std::istringstream istr( buffer );
      double val;
      while (istr >> val){
        if (val != 0)
          vw_throw( NoImplErr() << "Optical distortion is not implemented.\n" );
      }
    }
  }

}

void asp::GeometricXML::parse_perspective_center( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "CX" )->getTextContent(),
              perspective_center[0] );
  cast_xmlch( get_node<DOMElement>( node, "CY" )->getTextContent(),
              perspective_center[1] );
  cast_xmlch( get_node<DOMElement>( node, "CZ" )->getTextContent(),
              perspective_center[2] );
}

void asp::GeometricXML::parse_camera_attitude( xercesc::DOMElement* node ) {
  Vector4 buffer; // We buffer since Q will want to constantly
  // renormalize itself.
  cast_xmlch( get_node<DOMElement>( node, "QCS4" )->getTextContent(),
              buffer[0] );
  cast_xmlch( get_node<DOMElement>( node, "QCS1" )->getTextContent(),
              buffer[1] );
  cast_xmlch( get_node<DOMElement>( node, "QCS2" )->getTextContent(),
              buffer[2] );
  cast_xmlch( get_node<DOMElement>( node, "QCS3" )->getTextContent(),
              buffer[3] );
  camera_attitude = Quat(buffer[0],buffer[1],buffer[2],buffer[3]);
}

void asp::GeometricXML::parse_detector_mounting( xercesc::DOMElement* node ) {

  DOMNodeList* children = node->getChildNodes();

  // Read through all the bands, storing their data
  std::vector<Vector4> bandVec;
  for ( XMLSize_t i = 0; i < children->getLength(); i++ ) {
    DOMNode* current = children->item(i);
    if ( current->getNodeType() == DOMNode::ELEMENT_NODE ) {
      DOMElement* element =
        dynamic_cast< DOMElement* > (current);

      std::string tag( XMLString::transcode(element->getTagName()) );
      if (boost::starts_with( tag, "BAND_" )) {
        Vector4 data;

        DOMElement* detector = get_node<DOMElement>( element, "DETECTOR_ARRAY" );
        cast_xmlch( get_node<DOMElement>(detector,"DETORIGINX")->getTextContent(),
                    data[0] );
        cast_xmlch( get_node<DOMElement>(detector,"DETORIGINY")->getTextContent(),
                    data[1] );
        cast_xmlch( get_node<DOMElement>(detector,"DETROTANGLE")->getTextContent(),
                    data[2] );
        cast_xmlch( get_node<DOMElement>(detector,"DETPITCH")->getTextContent(),
                    data[3] );
        bandVec.push_back(data);
      }
    }
  }

  if (bandVec.empty()){
    vw_throw(ArgumentErr() << "Could not find any bands in the "
             << "DETECTOR_MOUNTING section of the XML file.\n");
  }

  // If there are multiple bands, they must have the same values
  for (int i = 1; i < (int)bandVec.size(); i++){
    if (bandVec[0] != bandVec[i]){
      vw_throw(ArgumentErr() << "XML files with multiple and "
               << "distinct detector arrays in the DETECTOR_MOUNTING "
               << "section of the XML file are not supported.\n");
    }
  }
  
  Vector4 data = bandVec[0];
  detector_origin[0]   = data[0];
  detector_origin[1]   = data[1];
  detector_rotation    = data[2];
  detector_pixel_pitch = data[3];
  
}

asp::GeometricXML::GeometricXML() : XMLBase(5) {}

void asp::GeometricXML::parse( xercesc::DOMElement* node ) {
  DOMNodeList* children = node->getChildNodes();
  const XMLSize_t nodeCount = children->getLength();

  for ( XMLSize_t i = 0; i < nodeCount; ++i ) {
    DOMNode* current = children->item(i);
    if ( current->getNodeType() == DOMNode::ELEMENT_NODE ) {
      DOMElement* element =
        dynamic_cast< xercesc::DOMElement* >( current );

      std::string tag( XMLString::transcode(element->getTagName()) );
      if ( tag == "PRINCIPAL_DISTANCE" ) {
        parse_principal_distance( element );
        check_argument(0);
      } else if ( tag == "OPTICAL_DISTORTION" ) {
        parse_optical_distortion( element );
        check_argument(1);
      } else if ( tag == "PERSPECTIVE_CENTER" ) {
        parse_perspective_center( element );
        check_argument(2);
      } else if ( tag == "CAMERA_ATTITUDE" ) {
        parse_camera_attitude( element );
        check_argument(3);
      } else if ( tag == "DETECTOR_MOUNTING" ) {
        parse_detector_mounting( element );
        check_argument(4);
      }
    }
  }
}

void asp::EphemerisXML::parse_meta( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "STARTTIME" )->getTextContent(),
              start_time );
  cast_xmlch( get_node<DOMElement>( node, "TIMEINTERVAL" )->getTextContent(),
              time_interval );
  size_t num_points;
  cast_xmlch( get_node<DOMElement>( node, "NUMPOINTS" )->getTextContent(),
              num_points );
  position_vec.resize( num_points );
  velocity_vec.resize( num_points );
  covariance_vec.resize( num_points );
}

void asp::EphemerisXML::parse_eph_list( xercesc::DOMElement* node ) {
  DOMNodeList* children = node->getChildNodes();
  size_t count = 0;

  for ( XMLSize_t i = 0; i < children->getLength(); i++ ) {
    if ( children->item(i)->getNodeType() == DOMNode::ELEMENT_NODE ) {
      DOMElement* element = dynamic_cast<DOMElement*>( children->item(i) );
      std::string buffer;
      cast_xmlch( element->getTextContent(), buffer );

      std::istringstream istr( buffer );
      std::string index_b;
      istr >> index_b;
      size_t index;
      try{
        index = size_t(boost::lexical_cast<float>(index_b) + 0.5) - 1;
      } catch (boost::bad_lexical_cast const& e) {
        vw_throw(ArgumentErr() << "Failed to parse string: " << index_b << "\n");
      }

      istr >> position_vec[index][0] >> position_vec[index][1]
           >> position_vec[index][2] >> velocity_vec[index][0]
           >> velocity_vec[index][1] >> velocity_vec[index][2];
      istr >> covariance_vec[index][0] >> covariance_vec[index][1]
           >> covariance_vec[index][2] >> covariance_vec[index][3]
           >> covariance_vec[index][4] >> covariance_vec[index][5];

      count++;
    }
  }

  VW_ASSERT( count == position_vec.size(),
             IOErr() << "Read incorrect number of points." );
}

asp::EphemerisXML::EphemerisXML() : XMLBase(2) {}

void asp::EphemerisXML::parse( xercesc::DOMElement* node ) {
  parse_meta( node );
  check_argument(0);

  parse_eph_list( get_node<DOMElement>( node, "EPHEMLISTList" ) );
  check_argument(1);
}

void asp::AttitudeXML::parse_meta( xercesc::DOMElement* node ) {
  cast_xmlch( get_node<DOMElement>( node, "STARTTIME" )->getTextContent(),
              start_time );
  cast_xmlch( get_node<DOMElement>( node, "TIMEINTERVAL" )->getTextContent(),
              time_interval );
  size_t num_points;
  cast_xmlch( get_node<DOMElement>( node, "NUMPOINTS" )->getTextContent(),
              num_points );
  quat_vec.resize( num_points );
  covariance_vec.resize( num_points );
}

void asp::AttitudeXML::parse_att_list( xercesc::DOMElement* node ) {
  DOMNodeList* children = node->getChildNodes();
  size_t count = 0;
  Vector4 qbuf;

  for ( XMLSize_t i = 0; i < children->getLength(); i++ ) {
    if ( children->item(i)->getNodeType() == DOMNode::ELEMENT_NODE ) {
      DOMElement* element = dynamic_cast<DOMElement*>( children->item(i) );
      std::string buffer;
      cast_xmlch( element->getTextContent(), buffer );

      std::istringstream istr( buffer );
      std::string index_b;
      istr >> index_b;
      size_t index;
      try {
        index = size_t(boost::lexical_cast<float>(index_b) + 0.5) - 1;
      } catch (boost::bad_lexical_cast const& e) {
        vw_throw(ArgumentErr() << "Failed to parse string: " << index_b << "\n");
      }

      istr >> qbuf[0] >> qbuf[1] >> qbuf[2] >> qbuf[3];
      quat_vec[index] = Quat(qbuf[3], qbuf[0], qbuf[1], qbuf[2] );
      istr >> covariance_vec[index][0] >> covariance_vec[index][1]
           >> covariance_vec[index][2] >> covariance_vec[index][3]
           >> covariance_vec[index][4] >> covariance_vec[index][5]
           >> covariance_vec[index][6] >> covariance_vec[index][7]
           >> covariance_vec[index][8] >> covariance_vec[index][9];

      count++;
    }
  }

  VW_ASSERT( count == quat_vec.size(),
             IOErr() << "Read incorrect number of points." );
}

asp::AttitudeXML::AttitudeXML() : XMLBase(2) {}

void asp::AttitudeXML::parse( xercesc::DOMElement* node ) {
  parse_meta( node );
  check_argument(0);

  parse_att_list( get_node<DOMElement>( node, "ATTLISTList" ) );
  check_argument(1);
}

void asp::RPCXML::parse_vector( xercesc::DOMElement* node,
                                Vector<double,20>& vec ) {
  std::string buffer;
  cast_xmlch( node->getTextContent(), buffer );

  std::istringstream istr( buffer );
  istr >> vec[0] >> vec[1] >> vec[2] >> vec[3] >> vec[4] >> vec[5]
       >> vec[6] >> vec[7] >> vec[8] >> vec[9] >> vec[10] >> vec[11]
       >> vec[12] >> vec[13] >> vec[14] >> vec[15] >> vec[16] >> vec[17]
       >> vec[18] >> vec[19];
}

asp::RPCXML::RPCXML() : XMLBase(2) {}

void asp::RPCXML::read_from_file( std::string const& name ) {
  boost::scoped_ptr<XercesDOMParser> parser( new XercesDOMParser() );
  parser->setValidationScheme(XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);
  boost::scoped_ptr<ErrorHandler> errHandler( new HandlerBase() );
  parser->setErrorHandler(errHandler.get());

  DOMDocument* xmlDoc;
  DOMElement* elementRoot;

  try{
    parser->parse( name.c_str() );
    xmlDoc = parser->getDocument();
    elementRoot = xmlDoc->getDocumentElement();
  }catch(...){
    vw_throw( ArgumentErr() << "XML file \"" << name << "\" is invalid.\n" );
  }

  try {
    parse_rpb( get_node<DOMElement>( elementRoot, "RPB" ) );
    return;
  } catch ( vw::IOErr const& e ) {
    // Possibly RPB doesn't exist
  }
  try {
    parse_rational_function_model( get_node<DOMElement>( elementRoot, "Rational_Function_Model" ) );
    return;
  } catch ( vw::IOErr const& e ) {
    // Possibly Rational_Function_Model doesn't work
  }
  vw_throw( vw::NotFoundErr() << "Couldn't find RPB or Rational_Function_Model tag inside XML file." );
}

void asp::RPCXML::parse_rpb( xercesc::DOMElement* node ) {
  DOMElement* image = get_node<DOMElement>( node, "IMAGE" );

  // Pieces that will go into the RPC Model
  Vector<double,20> line_num_coeff, line_den_coeff, samp_num_coeff, samp_den_coeff;
  Vector2 xy_offset, xy_scale;
  Vector3 geodetic_offset, geodetic_scale;

  // Painfully extract from the XML
  cast_xmlch( get_node<DOMElement>( image, "SAMPOFFSET" )->getTextContent(),
              xy_offset.x() );
  cast_xmlch( get_node<DOMElement>( image, "LINEOFFSET" )->getTextContent(),
              xy_offset.y() );
  cast_xmlch( get_node<DOMElement>( image, "SAMPSCALE" )->getTextContent(),
              xy_scale.x() );
  cast_xmlch( get_node<DOMElement>( image, "LINESCALE" )->getTextContent(),
              xy_scale.y() );
  cast_xmlch( get_node<DOMElement>( image, "LONGOFFSET" )->getTextContent(),
              geodetic_offset.x() );
  cast_xmlch( get_node<DOMElement>( image, "LATOFFSET" )->getTextContent(),
              geodetic_offset.y() );
  cast_xmlch( get_node<DOMElement>( image, "HEIGHTOFFSET" )->getTextContent(),
              geodetic_offset.z() );
  cast_xmlch( get_node<DOMElement>( image, "LONGSCALE" )->getTextContent(),
              geodetic_scale.x() );
  cast_xmlch( get_node<DOMElement>( image, "LATSCALE" )->getTextContent(),
              geodetic_scale.y() );
  cast_xmlch( get_node<DOMElement>( image, "HEIGHTSCALE" )->getTextContent(),
              geodetic_scale.z() );
  check_argument(0);
  parse_vector( get_node<DOMElement>(get_node<DOMElement>( image, "LINENUMCOEFList" ), "LINENUMCOEF" ),
                line_num_coeff );
  parse_vector( get_node<DOMElement>(get_node<DOMElement>( image, "LINEDENCOEFList" ), "LINEDENCOEF" ),
                line_den_coeff );
  parse_vector( get_node<DOMElement>(get_node<DOMElement>( image, "SAMPNUMCOEFList" ), "SAMPNUMCOEF" ),
                samp_num_coeff );
  parse_vector( get_node<DOMElement>(get_node<DOMElement>( image, "SAMPDENCOEFList" ), "SAMPDENCOEF" ),
                samp_den_coeff );
  check_argument(1);

  // Push into the RPC Model so that it is easier to work with
  //
  // The choice of using the WGS84 datum comes from Digital Globe's
  // QuickBird Imagery Products : Products Guide. Section 11.1 makes a
  // blanket statement that all heights are meters against the WGS 84
  // ellipsoid.
  m_rpc.reset( new RPCModel( cartography::Datum("WGS84"),
                             line_num_coeff, line_den_coeff,
                             samp_num_coeff, samp_den_coeff,
                             xy_offset, xy_scale,
                             geodetic_offset, geodetic_scale ) );
}

void asp::RPCXML::parse_rational_function_model( xercesc::DOMElement* node ) {
  DOMElement* inverse_model =
    get_node<DOMElement>( node, "Inverse_Model" ); // Inverse model
                                                   // takes ground to
                                                   // image.
  DOMElement* rfmvalidity  = get_node<DOMElement>( node, "RFM_Validity" );

  // Pieces that will go into the RPC Model
  Vector<double,20> line_num_coeff, line_den_coeff, samp_num_coeff, samp_den_coeff;
  Vector2 xy_offset, xy_scale;
  Vector3 geodetic_offset, geodetic_scale;

  // Painfully extract the single values from the XML data
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_1")->getTextContent(), samp_num_coeff[0] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_2")->getTextContent(), samp_num_coeff[1] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_3")->getTextContent(), samp_num_coeff[2] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_4")->getTextContent(), samp_num_coeff[3] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_5")->getTextContent(), samp_num_coeff[4] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_6")->getTextContent(), samp_num_coeff[5] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_7")->getTextContent(), samp_num_coeff[6] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_8")->getTextContent(), samp_num_coeff[7] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_9")->getTextContent(), samp_num_coeff[8] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_10")->getTextContent(), samp_num_coeff[9] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_11")->getTextContent(), samp_num_coeff[10] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_12")->getTextContent(), samp_num_coeff[11] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_13")->getTextContent(), samp_num_coeff[12] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_14")->getTextContent(), samp_num_coeff[13] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_15")->getTextContent(), samp_num_coeff[14] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_16")->getTextContent(), samp_num_coeff[15] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_17")->getTextContent(), samp_num_coeff[16] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_18")->getTextContent(), samp_num_coeff[17] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_19")->getTextContent(), samp_num_coeff[18] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_NUM_COEFF_20")->getTextContent(), samp_num_coeff[19] );

  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_1")->getTextContent(), samp_den_coeff[0] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_2")->getTextContent(), samp_den_coeff[1] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_3")->getTextContent(), samp_den_coeff[2] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_4")->getTextContent(), samp_den_coeff[3] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_5")->getTextContent(), samp_den_coeff[4] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_6")->getTextContent(), samp_den_coeff[5] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_7")->getTextContent(), samp_den_coeff[6] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_8")->getTextContent(), samp_den_coeff[7] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_9")->getTextContent(), samp_den_coeff[8] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_10")->getTextContent(), samp_den_coeff[9] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_11")->getTextContent(), samp_den_coeff[10] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_12")->getTextContent(), samp_den_coeff[11] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_13")->getTextContent(), samp_den_coeff[12] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_14")->getTextContent(), samp_den_coeff[13] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_15")->getTextContent(), samp_den_coeff[14] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_16")->getTextContent(), samp_den_coeff[15] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_17")->getTextContent(), samp_den_coeff[16] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_18")->getTextContent(), samp_den_coeff[17] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_19")->getTextContent(), samp_den_coeff[18] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "SAMP_DEN_COEFF_20")->getTextContent(), samp_den_coeff[19] );

  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_1")->getTextContent(), line_num_coeff[0] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_2")->getTextContent(), line_num_coeff[1] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_3")->getTextContent(), line_num_coeff[2] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_4")->getTextContent(), line_num_coeff[3] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_5")->getTextContent(), line_num_coeff[4] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_6")->getTextContent(), line_num_coeff[5] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_7")->getTextContent(), line_num_coeff[6] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_8")->getTextContent(), line_num_coeff[7] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_9")->getTextContent(), line_num_coeff[8] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_10")->getTextContent(), line_num_coeff[9] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_11")->getTextContent(), line_num_coeff[10] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_12")->getTextContent(), line_num_coeff[11] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_13")->getTextContent(), line_num_coeff[12] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_14")->getTextContent(), line_num_coeff[13] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_15")->getTextContent(), line_num_coeff[14] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_16")->getTextContent(), line_num_coeff[15] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_17")->getTextContent(), line_num_coeff[16] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_18")->getTextContent(), line_num_coeff[17] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_19")->getTextContent(), line_num_coeff[18] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_NUM_COEFF_20")->getTextContent(), line_num_coeff[19] );

  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_1")->getTextContent(), line_den_coeff[0] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_2")->getTextContent(), line_den_coeff[1] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_3")->getTextContent(), line_den_coeff[2] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_4")->getTextContent(), line_den_coeff[3] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_5")->getTextContent(), line_den_coeff[4] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_6")->getTextContent(), line_den_coeff[5] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_7")->getTextContent(), line_den_coeff[6] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_8")->getTextContent(), line_den_coeff[7] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_9")->getTextContent(), line_den_coeff[8] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_10")->getTextContent(), line_den_coeff[9] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_11")->getTextContent(), line_den_coeff[10] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_12")->getTextContent(), line_den_coeff[11] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_13")->getTextContent(), line_den_coeff[12] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_14")->getTextContent(), line_den_coeff[13] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_15")->getTextContent(), line_den_coeff[14] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_16")->getTextContent(), line_den_coeff[15] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_17")->getTextContent(), line_den_coeff[16] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_18")->getTextContent(), line_den_coeff[17] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_19")->getTextContent(), line_den_coeff[18] );
  cast_xmlch( get_node<DOMElement>( inverse_model, "LINE_DEN_COEFF_20")->getTextContent(), line_den_coeff[19] );

  cast_xmlch( get_node<DOMElement>( rfmvalidity, "LONG_SCALE")->getTextContent(), geodetic_scale.x() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "LAT_SCALE")->getTextContent(), geodetic_scale.y() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "HEIGHT_SCALE")->getTextContent(), geodetic_scale.z() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "LONG_OFF")->getTextContent(), geodetic_offset.x() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "LAT_OFF")->getTextContent(), geodetic_offset.y() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "HEIGHT_OFF")->getTextContent(), geodetic_offset.z() );

  cast_xmlch( get_node<DOMElement>( rfmvalidity, "SAMP_SCALE")->getTextContent(), xy_scale.x() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "LINE_SCALE")->getTextContent(), xy_scale.y() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "SAMP_OFF")->getTextContent(), xy_offset.x() );
  cast_xmlch( get_node<DOMElement>( rfmvalidity, "LINE_OFF")->getTextContent(), xy_offset.y() );

  xy_offset -= Vector2i(1,1);

  // Push into the RPC Model so that it is easier to work with
  m_rpc.reset( new RPCModel( cartography::Datum("WGS84"),
                             line_num_coeff, line_den_coeff,
                             samp_num_coeff, samp_den_coeff,
                             xy_offset, xy_scale,
                             geodetic_offset, geodetic_scale ) );

}

// Helper functions to allow us to fill the objects
void asp::read_xml( std::string const& filename,
                    GeometricXML& geo,
                    AttitudeXML& att,
                    EphemerisXML& eph,
                    ImageXML& img,
                    RPCXML& rpc
                    ) {

  // Check if the file actually exists and throw a user helpful file.
  if ( !fs::exists( filename ) )
    vw_throw( ArgumentErr() << "XML file \"" << filename << "\" does not exist." );

  try{
    boost::scoped_ptr<XercesDOMParser> parser( new XercesDOMParser() );
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);
    boost::scoped_ptr<ErrorHandler> errHandler( new HandlerBase() );
    parser->setErrorHandler(errHandler.get());

    parser->parse( filename.c_str() );
    DOMDocument* xmlDoc = parser->getDocument();
    DOMElement* elementRoot = xmlDoc->getDocumentElement();

    DOMNodeList* children = elementRoot->getChildNodes();
    for ( XMLSize_t i = 0; i < children->getLength(); ++i ) {
      DOMNode* curr_node = children->item(i);
      if ( curr_node->getNodeType() == DOMNode::ELEMENT_NODE ) {
        DOMElement* curr_element =
          dynamic_cast<DOMElement*>( curr_node );

        std::string tag( XMLString::transcode(curr_element->getTagName()) );

        if ( tag == "GEO" )
          geo.parse( curr_element );
        else if ( tag == "EPH" )
          eph.parse( curr_element );
        else if ( tag == "ATT" )
          att.parse( curr_element );
        else if ( tag == "IMD" )
          img.parse( curr_element );
        else if ( tag == "RPB" )
          rpc.parse( curr_element );
      }
    }
  } catch ( const std::exception& e ) {                
    vw_throw( ArgumentErr() << e.what() << "XML file \"" << filename << "\" is invalid.\n" );
  }

}

vw::Vector2i asp::xml_image_size( std::string const& filename ){
  GeometricXML geo;
  AttitudeXML att;
  EphemerisXML eph;
  ImageXML img;
  RPCXML rpc;

  read_xml( filename, geo, att, eph, img, rpc);
  return img.image_size;
}

asp::RPCModel* asp::RPCXML::rpc_ptr() const {
  VW_ASSERT( m_rpc.get(),
             LogicErr() << "read_from_file or parse needs to be called first before an RPCModel is ready" );
  return m_rpc.get();
}
