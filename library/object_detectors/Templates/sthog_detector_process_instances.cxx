/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifdef USE_OPENCV

#include <vxl_config.h>

#include <object_detectors/sthog_detector_process.txx>

template class vidtk::sthog_detector_process<vxl_byte>;
template class vidtk::sthog_detector_process<vxl_uint_16>;

#endif
