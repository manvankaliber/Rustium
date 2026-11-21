// Independent transcription of Group::GetFirstRequestWithoutJob and the
// contiguous assigned walk in TryToAssignJobToRequest. Original Git blobs and
// region identities are referenced by source-identity.json.
#ifndef TRANSPORT_ASSIGNMENT_PREFIX_ORIGINAL_H_
#define TRANSPORT_ASSIGNMENT_PREFIX_ORIGINAL_H_
#include <cstdint>
namespace transport_assignment_original {
// Call on up to 16 observations in native queue order. Return the first
// unassigned index, or count when the observed run is entirely assigned.
inline int FirstWithoutJob(uint64_t assigned, unsigned count) {
  unsigned i = 0;
  for (; i < count && ((assigned >> i) & 1u); ++i) {}
  return static_cast<int>(i);
}
inline bool UseFront(bool has_unassigned) { return has_unassigned; }
}  // namespace transport_assignment_original
#endif
