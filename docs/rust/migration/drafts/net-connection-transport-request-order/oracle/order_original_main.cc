// Standalone executable oracle scenarios; native lifecycle is not simulated.
#include "order_original.h"

int main() {
  transport_order_original::Model model;
  model.Insert({1, 3, true});
  model.Insert({2, 3, true});
  model.SetPriority(1, 3);
  assert(model.requests()[0].id == 1 && model.requests()[1].id == 2);
  model.Insert({3, 5, true});
  model.Insert({4, 5, false});
  model.Insert({5, 5, false});
  assert(model.requests()[0].id == 5 && model.requests()[1].id == 4 &&
         model.requests()[2].id == 3);
  model.SetPriority(3, 3);
  assert(model.requests()[4].id == 3);
  model.Erase(4);
  model.AddJob(true);
  model.AddJob(true);
  assert(model.ClaimPreconnect() && model.ClaimPreconnect());
  assert(!model.ClaimPreconnect());
  model.AddJob(true);
  model.RemoveJob();
  model.RemoveAllJobs();
  assert(model.never_claimed() == 0);
  return 0;
}
