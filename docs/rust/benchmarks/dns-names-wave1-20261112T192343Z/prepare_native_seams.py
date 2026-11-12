from pathlib import Path
p=Path('.scratch/net-wave1/dns-names/build_native.py');s=p.read_text();s=s.replace("add(root/'net/base/url_util.cc','url_util');add(root/'net/base/ip_address.cc','ip_address');", "add(work/'native_seams.cc','native_seams');");p.write_text(s)
def function(text,mark):
 a=text.index(mark);b=text.index('{',a);level=1;i=b+1
 while level:
  if text[i]=='{':level+=1
  if text[i]=='}':level-=1
  i+=1
 return text[a:i]
r=Path('D:/Rustium');u=(r/'net/base/url_util.cc').read_text();ip=(r/'net/base/ip_address.cc').read_text();d=(r/'net/dns/dns_util.cc').read_text()
seams='#include "net/base/url_util.h"\n#include "net/base/ip_address.h"\n#include "net/dns/dns_util.h"\n#include "base/strings/string_util.h"\nnamespace net {\n'
for t,mark in [(u,'bool IsHostCharAlphanumeric('),(u,'bool IsNormalizedLocalhostTLD('),(u,'bool IsCanonicalizedHostCompliant('),(u,'bool HostStringIsLocalhost('),(u,'bool IsLocalHostname('),(ip,'bool IPAddress::IsLoopback()'),(ip,'bool ParseURLHostnameToAddress('),(d,'std::array<uint8_t, 2> CreateNamePointer(')]:seams+=function(t,mark)+'\n'
seams+='}\n';(r/'.scratch/net-wave1/dns-names/native_seams.cc').write_text(seams)
