from pathlib import Path
p=Path('.scratch/net-wave1/dns-names/native_seams.cc');p.write_text('#include "net/dns/public/dns_protocol.h"\n#include "base/numerics/byte_conversions.h"\n'+p.read_text())
