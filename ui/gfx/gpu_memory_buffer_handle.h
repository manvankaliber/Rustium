// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_GPU_MEMORY_BUFFER_HANDLE_H_
#define UI_GFX_GPU_MEMORY_BUFFER_HANDLE_H_

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "base/check_op.h"
#include "base/component_export.h"
#include "base/memory/unsafe_shared_memory_region.h"
#include "base/notreached.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_OZONE)
#include "ui/gfx/native_pixmap_handle.h"
#elif BUILDFLAG(IS_APPLE)
#include "ui/gfx/mac/io_surface.h"
#elif BUILDFLAG(IS_WIN)
#include <optional>

#include "base/types/token_type.h"
#include "base/win/scoped_handle.h"
#endif

namespace mojo {
template <typename, typename>
struct StructTraits;
template <typename, typename>
struct UnionTraits;
}  // namespace mojo

namespace gfx {
namespace mojom {
class DXGIHandleDataView;
class GpuMemoryBufferPlatformHandleDataView;
}  // namespace mojom

enum GpuMemoryBufferType {
  EMPTY_BUFFER,
  SHARED_MEMORY_BUFFER,
#if BUILDFLAG(IS_APPLE)
  IO_SURFACE_BUFFER,
#elif BUILDFLAG(IS_OZONE)
  NATIVE_PIXMAP,
#elif BUILDFLAG(IS_WIN)
  DXGI_SHARED_HANDLE,
#endif
};

#if BUILDFLAG(IS_WIN)
using DXGIHandleToken = base::TokenType<class DXGIHandleTokenTypeMarker>;

// A simple type that bundles together the various bits for working with DXGI
// handles in Chrome. It consists of:
// - `buffer_handle`: the shared handle to the DXGI resource
// - `token`: A strongly-typed UnguessableToken used to determine if two
//            instances of `DXGIHandle` represent the same underlying shared
//            handle. Needed because the handle itself may be duplicated virtual
//            `DuplicateHandle()`.
// - `region`: An optional shared memory region. A DXGI handle's buffer can only
//             be read in the GPU process; this is used to provide support for
//             `Map()`ing the buffer in other processes. Under the hood, this is
//             implemented by having the GPU process copy the buffer into a
//             shared memory region that other processes can read.
class COMPONENT_EXPORT(GFX) DXGIHandle {
 public:
  // Creates a DXGIHandle suitable for use in a barebones unit test. The
  // `buffer_handle` won't actually usable as a DXGI shared handle, so this
  // helper is not suitable for integration tests.
  static DXGIHandle CreateFakeForTest();

  // Constructs an instance where `IsValid() == false`.
  DXGIHandle();
  ~DXGIHandle();

  // Constructs an instance, taking ownership of `scoped_handle` and associating
  // it with a new DXGIHandleToken. `scoped_handle` must be a valid handle.
  explicit DXGIHandle(base::win::ScopedHandle scoped_handle);
  // Typically only used by IPC deserialization. `buffer_handle` must be valid,
  // but `region` may be invalid.
  DXGIHandle(base::win::ScopedHandle buffer_handle,
             const DXGIHandleToken& token,
             base::UnsafeSharedMemoryRegion region);

  DXGIHandle(DXGIHandle&&);
  DXGIHandle& operator=(DXGIHandle&&);

  DXGIHandle(const DXGIHandle&) = delete;
  DXGIHandle& operator=(const DXGIHandle&) = delete;

  // Whether or not `this` has a valid underlying platform handle. This method
  // can return true even if `region()` is not a valid shmem region.
  bool IsValid() const;
  // Creates a copy of `this`. The underlying `buffer_handle` is duplicated into
  // a new handle, but `token()` will be preserved, so callers should use
  // `token()` to check if two `DXGIHandle`s actually refer to the same
  // resource.
  DXGIHandle Clone() const;
  // Similar to above but also associate `region` with the cloned `DXGIHandle`.
  //
  // Precondition: `this` must not have an associated region, i.e.
  // `region().IsValid()` is false.
  DXGIHandle CloneWithRegion(base::UnsafeSharedMemoryRegion region) const;

  HANDLE buffer_handle() const { return buffer_handle_.Get(); }
  base::win::ScopedHandle TakeBufferHandle();

  const DXGIHandleToken& token() const { return token_; }

  const base::UnsafeSharedMemoryRegion& region() const { return region_; }
  base::UnsafeSharedMemoryRegion TakeRegion() { return std::move(region_); }

 private:
  friend mojo::StructTraits<mojom::DXGIHandleDataView, DXGIHandle>;

  base::win::ScopedHandle buffer_handle_;
  DXGIHandleToken token_;
  base::UnsafeSharedMemoryRegion region_;
};
#endif  // BUILDFLAG(IS_WIN)

// TODO(crbug.com/40584691): Convert this to a proper class to ensure the state
// is always consistent, particularly that the only one handle is set at the
// same time and it corresponds to |type|.
struct COMPONENT_EXPORT(GFX) GpuMemoryBufferHandle {
  GpuMemoryBufferHandle();
  explicit GpuMemoryBufferHandle(base::UnsafeSharedMemoryRegion region);
#if BUILDFLAG(IS_WIN)
  explicit GpuMemoryBufferHandle(DXGIHandle handle);
#elif BUILDFLAG(IS_OZONE)
  explicit GpuMemoryBufferHandle(gfx::NativePixmapHandle native_pixmap_handle);
#elif BUILDFLAG(IS_APPLE)
  explicit GpuMemoryBufferHandle(ScopedIOSurface io_surface);
#endif
  GpuMemoryBufferHandle(GpuMemoryBufferHandle&& other);
  GpuMemoryBufferHandle& operator=(GpuMemoryBufferHandle&& other);
  ~GpuMemoryBufferHandle();

  GpuMemoryBufferHandle Clone() const;
  bool is_null() const { return type == EMPTY_BUFFER; }

  // The shared memory region may only be used with SHARED_MEMORY_BUFFER and
  // DXGI_SHARED_HANDLE. In the case of DXGI handles, the actual contents of the
  // buffer can only be accessed from the GPU process, so `Map()`ing the buffer
  // into memory actually requires an IPC to the GPU process, which then copies
  // the contents into the shmem region so it can be accessed from other
  // processes.
  const base::UnsafeSharedMemoryRegion& region() const& {
    CHECK_EQ(type, SHARED_MEMORY_BUFFER);
    return region_;
  }
  base::UnsafeSharedMemoryRegion region() && {
    CHECK_EQ(type, SHARED_MEMORY_BUFFER);
    type = EMPTY_BUFFER;
    return std::move(region_);
  }

#if BUILDFLAG(IS_OZONE)
  const NativePixmapHandle& native_pixmap_handle() const& {
    CHECK_EQ(type, NATIVE_PIXMAP);
    return native_pixmap_handle_;
  }
  NativePixmapHandle native_pixmap_handle() && {
    CHECK_EQ(type, NATIVE_PIXMAP);
    type = EMPTY_BUFFER;
    return std::move(native_pixmap_handle_);
  }
#endif  // BUILDFLAG(IS_OZONE)

#if BUILDFLAG(IS_WIN)
  const DXGIHandle& dxgi_handle() const& {
    CHECK_EQ(type, DXGI_SHARED_HANDLE);
    return dxgi_handle_;
  }
  DXGIHandle dxgi_handle() && {
    CHECK_EQ(type, DXGI_SHARED_HANDLE);
    type = EMPTY_BUFFER;
    return std::move(dxgi_handle_);
  }
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_APPLE)
  const ScopedIOSurface& io_surface() const& { return io_surface_; }
  ScopedIOSurface io_surface() && {
    CHECK_EQ(type, IO_SURFACE_BUFFER);
    type = EMPTY_BUFFER;
    return std::move(io_surface_);
  }
#endif  // BUILDFLAG(IS_APPLE)

  GpuMemoryBufferType type = GpuMemoryBufferType::EMPTY_BUFFER;

  uint32_t offset = 0;
  uint32_t stride = 0;


 private:
  friend mojo::UnionTraits<mojom::GpuMemoryBufferPlatformHandleDataView,
                           GpuMemoryBufferHandle>;

  // This naming isn't entirely styleguide-compliant, but per the TODO, the end
  // goal is to make `this` an encapsulated class.
  base::UnsafeSharedMemoryRegion region_;

#if BUILDFLAG(IS_OZONE)
  NativePixmapHandle native_pixmap_handle_;
#endif  // BUILDFLAG(IS_OZONE)

#if BUILDFLAG(IS_WIN)
  DXGIHandle dxgi_handle_;
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_APPLE)
  ScopedIOSurface io_surface_;
#endif  // BUILDFLAG(IS_APPLE)
};

}  // namespace gfx

#endif  // UI_GFX_GPU_MEMORY_BUFFER_HANDLE_H_
