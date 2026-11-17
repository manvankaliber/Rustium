// Copyright 2026 The Rustium Authors
// Ordinary validation utility only; not a production ABI.
// Matches the scalar expression in FileSystemUsageCache::AtomicUpdateUsageByDelta.
static_assert(sizeof(long long) == 8);
extern "C" __declspec(dllexport) long long UsageDelta(long long usage,
                                                    long long delta) {
  return usage + delta;
}
