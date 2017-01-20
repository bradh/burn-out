/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_scene_obstruction_detector_h_
#define vidtk_scene_obstruction_detector_h_

#include <vil/vil_image_view.h>
#include <vil/vil_rgb.h>
#include <vgl/vgl_box_2d.h>

#include <object_detectors/pixel_feature_writer.h>

#include <video_properties/border_detection_process.h>

#include <utilities/ring_buffer.h>
#include <utilities/timestamp.h>

#include <vector>
#include <limits>
#include <string>

#include <classifier/hashed_image_classifier.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{


/// Various learned properties of the scene obstructor for the current frame.
template< typename PixType >
struct scene_obstruction_properties
{
  /// Average color of the obstruction
  vil_rgb<PixType> color_;

  /// Average intensity of the obstructor
  PixType intensity_;

  /// Did the output mask recently change by a large factor?
  bool break_flag_;

  /// Is the information in this structure valid?
  bool is_valid_;

  // Defaults
  scene_obstruction_properties()
  : color_( 0, 0, 0 ),
    intensity_( 0 ),
    break_flag_( false ),
    is_valid_( true )
  {}
};


/// External settings for the scene_obstructor_detector class.
struct scene_obstruction_detector_settings
{
  /// Main classifier filename
  std::string primary_classifier_filename_;

  /// Appearance-only classifier filename
  std::string appearance_classifier_filename_;

  /// Initial classifier threshold [unitless, depends on model]
  double initial_threshold_;

  /// Should we use a classifier that doesn't use temporal features for
  /// the first n frames?
  bool use_appearance_classifier_;

  /// Number of frames after a break to use the appearance only classifier for.
  unsigned appearance_frames_;

  /// Variance scale factor which maps the input variance image to the
  /// feature type.
  double variance_scale_factor_;

  /// Should we utilize a spatial feature prior? This is typically an image
  /// divided into different segments, although it can also be autogenerated
  /// internally via griding if no file is specified.
  bool use_spatial_prior_feature_;

  /// Filename for spatial prior features, if we want to use an image instead
  /// of an automatically generated image.
  std::string spatial_prior_filename_;

  /// Divides the location feature image into length x length regions, with
  /// each region having a unique ID
  unsigned spatial_prior_grid_length_;

  /// Threshold colors to pure black or pure white only.
  bool map_colors_to_nearest_extreme_;

  /// Threshold colors to pure black or pure white only.
  bool map_colors_near_extremes_only_;

  /// Gray is not a usable color
  bool no_gray_filter_;

  /// Adaptive thresholding parameters
  ///@{
  bool use_adaptive_thresh_;
  unsigned at_pivot_1_;
  unsigned at_pivot_2_;
  double at_interval_1_adj_;
  double at_interval_2_adj_;
  double at_interval_3_adj_;
  ///@}

  /// Mask break detection variables
  ///@{
  bool enable_mask_break_detection_;
  unsigned mask_count_history_length_;
  unsigned mask_intensity_history_length_;
  double count_percent_change_req_;
  double count_std_dev_req_;
  unsigned min_hist_for_intensity_diff_;
  double intensity_diff_req_;
  ///@}

  /// Training data output mode parameters
  ///@{
  bool is_training_mode_;
  bool output_feature_image_mode_;
  bool output_classifier_image_mode_;
  std::string groundtruth_dir_;
  std::string output_filename_;
  ///@}

  // Defaults
  scene_obstruction_detector_settings()
  : primary_classifier_filename_( "" ),
    appearance_classifier_filename_( "" ),
    initial_threshold_( 0.0 ),
    use_appearance_classifier_( true ),
    appearance_frames_( 10 ),
    variance_scale_factor_( 0.32 ),
    use_spatial_prior_feature_( true ),
    spatial_prior_filename_( "" ),
    spatial_prior_grid_length_( 5 ),
    map_colors_to_nearest_extreme_( true ),
    map_colors_near_extremes_only_( false ),
    no_gray_filter_( true ),
    use_adaptive_thresh_( false ),
    at_pivot_1_( 10 ),
    at_pivot_2_( 10 ),
    at_interval_1_adj_( 0.0 ),
    at_interval_2_adj_( 0.0 ),
    at_interval_3_adj_( 0.0 ),
    enable_mask_break_detection_( true ),
    mask_count_history_length_( 20 ),
    mask_intensity_history_length_( 40 ),
    count_percent_change_req_( 3.0 ),
    count_std_dev_req_( 5 ),
    min_hist_for_intensity_diff_( 30 ),
    intensity_diff_req_( 90 ),
    is_training_mode_( false ),
    output_feature_image_mode_( false ),
    output_classifier_image_mode_( false ),
    groundtruth_dir_( "" ),
    output_filename_( "" )
  {}
};


/// The scene_obstruction_detector takes in a series of input features,
/// as produced by the pixel_feature_extractor_super_process, in addition
/// to the input (source image). From these features, it uses a hashed_image
/// classifier to come up with an initial estimate of locations of potential
/// scene obstructions. This initial approximation is then averaged between
/// successive frames. If a signifant change in the initial approximation is
/// detected, a "mask shot break" is triggered which resets the internal average.
///
/// As output, it produces an output image containing 2 planes, 1 detailing
/// the classification values for the running average, and one for the intial
/// approximation. This process can be configured to try and detect a few
/// effects, such as HUDs, dust, or other obstructors.
///
/// The process also has several optional sub-systems including adaptive
/// weighting, performing a 2 level classifier, and outputting training
/// data for the creation of different classifiers.
template <typename PixType, typename FeatureType>
class scene_obstruction_detector
{

public:

  typedef vil_image_view< PixType > source_image;
  typedef vil_image_view< FeatureType > feature_image;
  typedef std::vector< feature_image > feature_array;
  typedef vil_image_view< bool > mask_type;
  typedef vil_image_view< double > classified_image;
  typedef vil_image_view< double > variance_image;
  typedef scene_obstruction_properties< PixType > properties;
  typedef pixel_feature_writer< FeatureType > feature_writer;
  typedef boost::scoped_ptr< feature_writer > feature_writer_sptr;

  scene_obstruction_detector() : is_valid_(false) {}
  scene_obstruction_detector( const scene_obstruction_detector_settings& options );
  virtual ~scene_obstruction_detector() {}

  /// Re-configure the detector with new settings.
  bool configure( const scene_obstruction_detector_settings& options );

  /// Process a new frame, given it's feature vector.
  void process_frame( const source_image& input_image,
                      const feature_array& input_features,
                      const image_border& input_border,
                      const variance_image& pixel_variance,
                      classified_image& output_image,
                      properties& output_properties );

private:

  // Were all models loaded successfully?
  bool is_valid_;

  // Externally set options
  scene_obstruction_detector_settings options_;

  // Internal properties
  properties props_;

  // Internal buffers/counters/classifiers
  hashed_image_classifier< FeatureType > initial_classifier_;
  hashed_image_classifier< FeatureType > appearance_classifier_;
  unsigned frame_counter_;
  unsigned frames_since_last_break_;
  double var_hash_scale_;
  vil_image_view< PixType > var_hash_;
  vil_image_view< PixType > intensity_diff_;
  vil_image_view< double > summed_history_;
  vil_image_view< FeatureType > spatial_prior_image_;

  // Mask break detection variables
  double cumulative_intensity_;
  double cumulative_mask_count_;
  vidtk::ring_buffer< double > mask_count_history_;
  vidtk::ring_buffer< double > mask_intensity_history_;
  unsigned color_hist_bitshift_;
  unsigned color_hist_scale_;

  // Training mode variables
  feature_writer_sptr training_data_extractor_;

  // Variance buffers
  vil_image_view< double > summed_variance_;
  vil_image_view< PixType > normalized_variance_;

  // Generate initial obstruction heatmap
  void perform_initial_approximation( const feature_array& features,
                                      const image_border& border,
                                      classified_image& output_image );

  // Estimate obstruction properties (color, size)
  bool estimate_mask_properties( const source_image& input,
                                 const image_border& border,
                                 const classified_image& output );

  // Helper function to aid with adaptive thresholding
  double adaptive_threshold_contribution( const unsigned& frame_num );

  // Output all feature images used for classification
  void output_feature_images( const feature_array& features );

  // For debug purposes, output intermediate classification images
  void output_classifier_images( const feature_array& features,
                                 const image_border& border,
                                 const classified_image& output );

  // Called when a change in obstructions is likely
  void trigger_mask_break( const source_image& input );

  // Generate the spatial prior feature given an input image
  void configure_spatial_prior( const source_image& input );
};


} // end namespace vidtk


#endif // vidtk_scene_obstruction_detector_h_
