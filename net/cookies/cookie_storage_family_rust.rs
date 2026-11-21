// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![deny(unsafe_op_in_unsafe_fn)]
#[path = "cookie_family_types.rs"]
pub mod types;
#[path = "cookie_family_util.rs"]
pub mod util;
#[path = "cookie_family_base.rs"]
pub mod base;
#[path = "cookie_family_options.rs"]
pub mod options;
#[path = "cookie_family_collection.rs"]
pub mod collection;
#[path = "cookie_partition_key_rust_core.rs"]
pub mod partition;
// Existing safe policy sources; do not compile their separately exported FFI
// roots into this crate. Parent pins their exact source identity in the build.
#[path = "cookie_inclusion_status_rust_core.rs"]
pub mod status;
#[path = "parsed_cookie_rust_core.rs"]
pub mod parsed;
#[path = "cookie_family_ffi.rs"]
mod ffi;
#[path = "cookie_partition_key_rust_ffi.rs"]
mod partition_ffi;
// Files below belong exclusively to the paired implementation owner and are
// installed together at the family milestone, never stubbed to compile a leaf.
#[path = "cookie_family_canonical.rs"]
pub mod canonical;
#[path = "cookie_family_monster.rs"]
pub mod monster;
#[path = "cookie_family_canonical_dispatch.rs"]
mod canonical_dispatch;
#[path = "cookie_family_monster_dispatch.rs"]
mod monster_dispatch;
#[cfg(test)]
#[path = "cookie_family_tests.rs"]
mod family_tests;
