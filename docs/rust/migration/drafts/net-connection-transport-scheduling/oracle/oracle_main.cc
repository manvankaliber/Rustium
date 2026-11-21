// Executable standalone oracle smoke scenarios. No Chromium providers are
// mocked here: run actual lifecycle fixtures separately on the laptop.
#include "oracle.h"
using transport_oracle::Group;
using transport_oracle::FindTopStalledGroup;
int main() {
  assert(!FindTopStalledGroup({}).has_value());
  assert(!FindTopStalledGroup({{false, 4}}).has_value());
  assert(*FindTopStalledGroup({{true, 1}, {true, 3}, {true, 3}}) == 1);
  Group g;
  g.AddJob(10, 0); g.AddJob(11, 0);
  g.Insert({1, 2, true}); g.Insert({2, 2, true});
  assert(g.requests()[0].job == 10 && g.requests()[1].job == 11);
  g.Insert({3, 3, true});
  assert(g.requests()[0].id == 3 && g.requests()[0].job == 11);
  assert(g.requests()[2].job == -1 && g.job_priority(11) == 3);
  g.SetPriority(1, 2);  // same-priority reprioritization preserves order
  assert(g.requests()[1].id == 1);
  g.Remove(3);  // released job goes to first unassigned request
  assert(g.requests()[1].id == 2 && g.requests()[1].job == 11);
  g.Insert({4, 5, false});
  g.Insert({5, 5, false});
  assert(g.requests()[0].id == 5 && g.requests()[1].id == 4);
  return 0;
}
