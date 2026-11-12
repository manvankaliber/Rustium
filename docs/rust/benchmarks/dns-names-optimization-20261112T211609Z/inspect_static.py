from pathlib import Path
p=Path('net/dns/dns_names_ffi.rs');s=p.read_text().replace('mod dns_names_core;', 'pub mod dns_names_core;');p.write_text(s)
w=Path('.scratch/net-wave1/dns-names/optimization');s=next((w/'assembly-target/release/deps').glob('dns_names*.s')).read_text();a=s.index('net_dns_names_less:');b=s.index('.seh_endproc',a);print(s[a:b+12])
