/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_diff_pass_thru_process_h_
#define vidtk_diff_pass_thru_process_h_

#include <vector>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking_data/gui_frame_info.h>
#include <tracking_data/pixel_feature_array.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

// Best. Process. Ever.
template <class PixType>
class diff_pass_thru_process
  : public process
{
public:
  typedef diff_pass_thru_process self_type;

  diff_pass_thru_process( std::string const& name );
  virtual ~diff_pass_thru_process();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  VIDTK_PASS_THRU_PORT( gsd, double );
  VIDTK_PASS_THRU_PORT( timestamp, timestamp );
  VIDTK_PASS_THRU_PORT( image, vil_image_view< PixType > );
  VIDTK_PASS_THRU_PORT( src_to_ref_homography, image_to_image_homography );
  VIDTK_PASS_THRU_PORT( src_to_wld_homography, image_to_plane_homography );
  VIDTK_PASS_THRU_PORT( wld_to_src_homography, plane_to_image_homography );
  VIDTK_PASS_THRU_PORT( src_to_utm_homography, image_to_utm_homography );
  VIDTK_PASS_THRU_PORT( wld_to_utm_homography, plane_to_utm_homography );
  VIDTK_PASS_THRU_PORT( ref_to_wld_homography, image_to_plane_homography );
  VIDTK_PASS_THRU_PORT( video_modality, vidtk::video_modality );
  VIDTK_PASS_THRU_PORT( shot_break_flags, vidtk::shot_break_flags );
  VIDTK_PASS_THRU_PORT( fg, vil_image_view<bool> );
  VIDTK_PASS_THRU_PORT( diff, vil_image_view<float> );
  VIDTK_PASS_THRU_PORT( fg2, vil_image_view<bool> );
  VIDTK_PASS_THRU_PORT( diff2, vil_image_view<float> );
  VIDTK_PASS_THRU_PORT( gui_feedback, gui_frame_info );
  VIDTK_PASS_THRU_PORT( feature_array, typename pixel_feature_array< PixType >::sptr_t );

protected:

  // Internal Parameters
  config_block config_;
};

} // end namespace vidtk


#endif // vidtk_diff_pass_thru_process_h_
