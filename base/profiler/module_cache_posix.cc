// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/profiler/module_cache.h"

#include <dlfcn.h>
#include <elf.h>

#include <optional>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/debug/elf_reader.h"
#include "build/build_config.h"


namespace base {

namespace {

// Returns the unique build ID for a module loaded at |module_addr|. Returns the
// empty string if the function fails to get the build ID.
//
// Build IDs follow a cross-platform format consisting of several fields
// concatenated together:
// - the module's unique ID, and
// - the age suffix for incremental builds.
//
// On POSIX, the unique ID is read from the ELF binary located at |module_addr|.
// The age field is always 0.
//
// PRECONDITIONS: `module_addr` must point to the start of an ELF image that is
// fully mapped into memory (see base/debug/elf_reader.h).
UNSAFE_BUFFER_USAGE std::string GetUniqueBuildId(const void* module_addr) {
  // SAFETY: Required from caller, see UNSAFE_BUFFER_USAGE above.
  std::string build_id =
      UNSAFE_BUFFERS(debug::ReadElfBuildId(module_addr, true));
  if (!build_id.empty()) {
    // Append 0 for the age value.
    build_id += '0';
  }
  return build_id;
}

// Returns the offset from |module_addr| to the first byte following the last
// executable segment from the ELF file mapped at |module_addr|.
// It's defined this way so that any executable address from this module is in
// range [addr, addr + GetLastExecutableOffset(addr)).
// If no executable segment is found, returns 0.
//
// PRECONDITIONS: `module_addr` must point to the start of an ELF image that is
// fully mapped into memory (see base/debug/elf_reader.h).
UNSAFE_BUFFER_USAGE size_t GetLastExecutableOffset(const void* module_addr) {
  // SAFETY: Required from caller, see UNSAFE_BUFFER_USAGE above.
  const size_t relocation_offset =
      UNSAFE_BUFFERS(debug::GetRelocationOffset(module_addr));
  size_t max_offset = 0;
  // SAFETY: Required from caller, see UNSAFE_BUFFER_USAGE above.
  for (const Phdr& header :
       UNSAFE_BUFFERS(debug::GetElfProgramHeaders(module_addr))) {
    if (header.p_type != PT_LOAD || !(header.p_flags & PF_X)) {
      continue;
    }

    max_offset = std::max(
        max_offset, static_cast<size_t>(
                        header.p_vaddr + relocation_offset + header.p_memsz -
                        reinterpret_cast<uintptr_t>(module_addr)));
  }

  return max_offset;
}

// PRECONDITIONS: `base_address` must point to the start of an ELF image that is
// fully mapped into memory (see base/debug/elf_reader.h).
UNSAFE_BUFFER_USAGE FilePath GetDebugBasenameForModule(const void* base_address,
                                                       std::string_view file) {


  return FilePath(file).BaseName();
}

class PosixModule : public ModuleCache::Module {
 public:
  PosixModule(uintptr_t base_address,
              const std::string& build_id,
              const FilePath& debug_basename,
              size_t size);

  PosixModule(const PosixModule&) = delete;
  PosixModule& operator=(const PosixModule&) = delete;

  // ModuleCache::Module
  uintptr_t GetBaseAddress() const override { return base_address_; }
  std::string GetId() const override { return id_; }
  FilePath GetDebugBasename() const override { return debug_basename_; }
  size_t GetSize() const override { return size_; }
  bool IsNative() const override { return true; }

 private:
  uintptr_t base_address_;
  std::string id_;
  FilePath debug_basename_;
  size_t size_;
};

PosixModule::PosixModule(uintptr_t base_address,
                         const std::string& build_id,
                         const FilePath& debug_basename,
                         size_t size)
    : base_address_(base_address),
      id_(build_id),
      debug_basename_(debug_basename),
      size_(size) {}

}  // namespace

// static
std::unique_ptr<const ModuleCache::Module> ModuleCache::CreateModuleForAddress(
    uintptr_t address) {
  Dl_info info;
  if (!dladdr(reinterpret_cast<const void*>(address), &info)) {
    return nullptr;
  }

  // SAFETY: `dladdr()` succeeded, so `dli_fbase` is the base address of the
  // shared object containing `address`, which the dynamic loader has fully
  // mapped into memory.
  return std::make_unique<PosixModule>(
      reinterpret_cast<uintptr_t>(info.dli_fbase),
      UNSAFE_BUFFERS(GetUniqueBuildId(info.dli_fbase)),
      UNSAFE_BUFFERS(GetDebugBasenameForModule(info.dli_fbase, info.dli_fname)),
      UNSAFE_BUFFERS(GetLastExecutableOffset(info.dli_fbase)));
}

}  // namespace base
