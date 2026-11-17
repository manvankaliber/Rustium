// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
use crate::types::{Context, Options};
pub fn inclusive(setting: bool) -> Context {
    if setting {
        Context::inclusive_for_set()
    } else {
        Context::inclusive()
    }
}
pub fn context_for_inclusion(context: Context) -> (i32, bool) {
    (context.schemeful_context, context.schemeful_context <= context.context)
}
pub fn equivalent(left: Context, right: Context, metadata: bool) -> bool {
    if metadata {
        left == right
    } else {
        left.same_contexts(right)
    }
}
pub fn all_inclusive() -> Options {
    Options::inclusive()
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn context_equality_ignores_metadata_unless_complete() {
        let left = Context::inclusive();
        let mut right = left;
        right.metadata.null_initiator = true;
        assert!(equivalent(left,right,false));
        assert!(!equivalent(left,right,true));
    }
    #[test]
    fn defaults_and_inclusive_preserve_update_policy() {
        let default = Options::original_default();
        assert!(default.exclude_httponly&&default.update_access_time);
        let all = all_inclusive();
        assert!(!all.exclude_httponly&&!all.update_access_time&&!all.return_excluded);
        assert_eq!(all.context.context,3);
        assert_eq!(inclusive(true).context,2);
    }
}
