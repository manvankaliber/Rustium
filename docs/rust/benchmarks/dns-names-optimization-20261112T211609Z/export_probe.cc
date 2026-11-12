#include "net/dns/dns_names_util.h"
bool DnsNamesExportProbe(std::string_view a, std::string_view b) { return net::dns_names_util::DomainNameComparator()(a,b); }
