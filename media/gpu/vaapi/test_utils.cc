// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/test_utils.h"

#include <sys/mman.h>

#include <array>
#include <memory>

#include "base/bits.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/containers/span_writer.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "components/viz/common/resources/shared_image_format_utils.h"
#include "media/base/video_types.h"
#include "media/gpu/vaapi/vaapi_utils.h"
#include "third_party/libdrm/src/include/drm/drm_fourcc.h"
#include "third_party/libyuv/include/libyuv.h"


namespace media {
namespace vaapi_test_utils {

namespace {

// Credit to the Mesa project for writing extensive documentation on the Tile4
// format. https://docs.mesa3d.org/isl/tiling.html#tile4
//
// Tile4 has 2 levels of tiling. The "main" tiles are 8x8 squares of subtiles.
// Subtiles are 16x4 rectangles of pixels. Pixels within subtiles are laid out
// in raster order. Subtiles within tiles are laid out in a repeating "Z"
// pattern. The "Z"s are 4 subtiles wide and 2 subtiles tall. Main tiles within
// a plane are laid out in raster order.
//
// Subtiles are conveniently the size of one cache line, and tiles are
// conveniently the size of one (4K) page.
//
// This detiling algorithm prioritizes linear writes at the expense of very
// non-linear reads so that we take advantage of the write combiner. One low
// hanging fruit optimization might be to experiment with prefetching to help
// with the unusual memory read pattern.

constexpr size_t kTile4TileWidth = 8;
constexpr size_t kTile4TileHeight = 8;
constexpr size_t kTile4SubtileWidth = 16;
constexpr size_t kTile4SubtileHeight = 4;
constexpr size_t kTile4SubtileSizeBytes =
    kTile4SubtileWidth * kTile4SubtileHeight;
constexpr size_t kTile4TileWidthBytes = kTile4TileWidth * kTile4SubtileWidth;
constexpr size_t kTile4TileHeightBytes = kTile4TileHeight * kTile4SubtileHeight;

}  // namespace

void Detile4(base::span<uint8_t> linear_dest,
             base::span<const uint8_t> tiled_src,
             size_t width,
             size_t height) {
  constexpr size_t kTile4TileSizeBytes =
      kTile4TileWidthBytes * kTile4TileHeightBytes;
  constexpr size_t kTile4ZWidth = 4;

  width = base::bits::AlignDown(width, kTile4TileWidthBytes);
  height = base::bits::AlignDown(height, kTile4TileHeightBytes);

  base::SpanWriter<uint8_t> dest(linear_dest);
  base::SpanReader<const uint8_t> src(tiled_src);

  for (size_t y = 0; y < height; y += kTile4TileHeight * kTile4SubtileHeight) {
    for (size_t tile_y = 0; tile_y < kTile4TileHeight; tile_y++) {
      for (size_t subtile_y = 0; subtile_y < kTile4SubtileHeight; subtile_y++) {
        base::SpanReader<const uint8_t> row = src;
        for (size_t x = 0; x < width;
             x += kTile4TileWidth * kTile4SubtileWidth) {
          size_t tile_x = 0;
          // Copy 1 row from 4 subtiles.
          for (; tile_x < kTile4ZWidth; tile_x++) {
            dest.Write(*row.Read<kTile4SubtileWidth>());
            row.Skip(kTile4SubtileSizeBytes - kTile4SubtileWidth);
          }
          row.Skip(kTile4ZWidth * kTile4SubtileSizeBytes);
          // Copy 1 row from another 4 subtiles.
          for (; tile_x < kTile4TileWidth; tile_x++) {
            dest.Write(*row.Read<kTile4SubtileWidth>());
            row.Skip(kTile4SubtileSizeBytes - kTile4SubtileWidth);
          }

          // Advance to the tile to the right.
          row.Skip(kTile4TileSizeBytes -
                   (3 * kTile4ZWidth * kTile4SubtileSizeBytes));
        }

        // Advance to the next row in the subtile.
        src.Skip(kTile4SubtileWidth);
      }

      // Advance to the next row in the tile.
      if (tile_y % 2 == 0) {
        src.Skip(kTile4ZWidth * kTile4SubtileSizeBytes -
                 kTile4SubtileSizeBytes);
      } else {
        src.Skip(3 * kTile4ZWidth * kTile4SubtileSizeBytes -
                 kTile4SubtileSizeBytes);
      }
    }

    // Advance to the tile below.
    src.Skip(width * kTile4TileHeight * kTile4SubtileHeight -
             kTile4TileSizeBytes);
  }
}

DecodedImage::DecodedImage() = default;
DecodedImage::DecodedImage(const DecodedImage&) = default;
DecodedImage& DecodedImage::operator=(const DecodedImage&) = default;
DecodedImage::~DecodedImage() = default;

std::string TestParamToString(
    const testing::TestParamInfo<TestParam>& param_info) {
  return param_info.param.test_name;
}

DecodedImage ScopedVAImageToDecodedImage(const ScopedVAImage* scoped_va_image) {
  DecodedImage decoded_image{};

  decoded_image.fourcc = scoped_va_image->image()->format.fourcc;
  decoded_image.number_of_planes = scoped_va_image->image()->num_planes;
  decoded_image.size =
      gfx::Size(base::strict_cast<int>(scoped_va_image->image()->width),
                base::strict_cast<int>(scoped_va_image->image()->height));

  DCHECK_LE(base::strict_cast<size_t>(decoded_image.number_of_planes),
            kMaxNumberPlanes);

  // This is safe because |number_of_planes| is retrieved from the VA-API and it
  // can not be greater than 3, which is also the size of the |planes| array.
  const auto offsets = base::span(scoped_va_image->image()->offsets);
  const auto pitches = base::span(scoped_va_image->image()->pitches);
  for (uint32_t i = 0u; i < decoded_image.number_of_planes; ++i) {
    // SAFETY: `va_buffer()->data()` points to the mapped VAImage buffer, and
    // `offsets[i]` is the byte offset to plane `i` provided by VA-API.
    decoded_image.planes[i].data = UNSAFE_BUFFERS(
        static_cast<uint8_t*>(scoped_va_image->va_buffer()->data()) +
        offsets[i]);
    decoded_image.planes[i].stride = base::checked_cast<int>(pitches[i]);
  }

  return decoded_image;
}


bool CompareImages(const DecodedImage& reference_image,
                   const DecodedImage& hw_decoded_image,
                   double min_ssim) {
  if (reference_image.fourcc != VA_FOURCC_I420)
    return false;

  // Uses the reference image's size as the ground truth.
  const gfx::Size image_size = reference_image.size;
  if (image_size != hw_decoded_image.size) {
    LOG(ERROR) << "Wrong expected software decoded image size, "
               << image_size.ToString() << " versus VaAPI provided "
               << hw_decoded_image.size.ToString();
    return false;
  }

  double ssim = 0;
  const uint32_t hw_fourcc = hw_decoded_image.fourcc;
  if (hw_fourcc == VA_FOURCC_I420) {
    ssim = libyuv::I420Ssim(
        reference_image.planes[0].data, reference_image.planes[0].stride,
        reference_image.planes[1].data, reference_image.planes[1].stride,
        reference_image.planes[2].data, reference_image.planes[2].stride,
        hw_decoded_image.planes[0].data, hw_decoded_image.planes[0].stride,
        hw_decoded_image.planes[1].data, hw_decoded_image.planes[1].stride,
        hw_decoded_image.planes[2].data, hw_decoded_image.planes[2].stride,
        image_size.width(), image_size.height());
  } else if (hw_fourcc == VA_FOURCC_NV12 || hw_fourcc == VA_FOURCC_YUY2 ||
             hw_fourcc == VA_FOURCC('Y', 'U', 'Y', 'V')) {
    // Calculate the stride for the chroma planes.
    const gfx::Size half_image_size((image_size.width() + 1) / 2,
                                    (image_size.height() + 1) / 2);
    // Temporary planes to hold intermediate conversions to I420 (i.e. NV12 to
    // I420 or YUYV/2 to I420).
    auto temp_y = std::make_unique<uint8_t[]>(image_size.GetArea());
    auto temp_u = std::make_unique<uint8_t[]>(half_image_size.GetArea());
    auto temp_v = std::make_unique<uint8_t[]>(half_image_size.GetArea());
    int conversion_result = -1;

    if (hw_fourcc == VA_FOURCC_NV12) {
      conversion_result = libyuv::NV12ToI420(
          hw_decoded_image.planes[0].data, hw_decoded_image.planes[0].stride,
          hw_decoded_image.planes[1].data, hw_decoded_image.planes[1].stride,
          temp_y.get(), image_size.width(), temp_u.get(),
          half_image_size.width(), temp_v.get(), half_image_size.width(),
          image_size.width(), image_size.height());
    } else {
      // |hw_fourcc| is YUY2 or YUYV, which are handled the same.
      // TODO(crbug.com/40586948): support other formats/planarities/pitches.
      conversion_result = libyuv::YUY2ToI420(
          hw_decoded_image.planes[0].data, hw_decoded_image.planes[0].stride,
          temp_y.get(), image_size.width(), temp_u.get(),
          half_image_size.width(), temp_v.get(), half_image_size.width(),
          image_size.width(), image_size.height());
    }
    if (conversion_result != 0) {
      LOG(ERROR) << "libyuv conversion error";
      return false;
    }

    ssim = libyuv::I420Ssim(
        reference_image.planes[0].data, reference_image.planes[0].stride,
        reference_image.planes[1].data, reference_image.planes[1].stride,
        reference_image.planes[2].data, reference_image.planes[2].stride,
        temp_y.get(), image_size.width(), temp_u.get(), half_image_size.width(),
        temp_v.get(), half_image_size.width(), image_size.width(),
        image_size.height());
  } else {
    LOG(ERROR) << "HW FourCC not supported: " << FourccToString(hw_fourcc);
    return false;
  }

  if (ssim < min_ssim) {
    LOG(ERROR) << "SSIM too low: " << ssim << " < " << min_ssim;
    return false;
  }

  return true;
}

}  // namespace vaapi_test_utils
}  // namespace media
