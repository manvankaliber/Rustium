from pathlib import Path
import re
p=Path('net/dns/dns_names_util.cc');s=p.read_text();s=re.sub(r'result = dns_names_internal::network_name_reader\(context, require_complete,\s*output\);',r'result = DecodeResult(dns_names_internal::network_name_reader(context, require_complete, output));',s);s=re.sub(r'result = dns_names_internal::network_name\(base::SpanToRustSlice\(input\),\s*require_complete, output\);',r'result = DecodeResult(dns_names_internal::network_name(base::SpanToRustSlice(input), require_complete, output));',s);p.write_text(s)
