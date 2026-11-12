from pathlib import Path
import re,json,subprocess
r=Path('D:/Rustium');w=r/'.scratch/net-wave1/content-disposition';o=w/'original'
def body(s,sig):
 a=s.index(sig);a=s.index('{',a);i=a+1;level=1
 # Use token braces, ignoring comments and literals.
 ts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27|\w+|[^\s\w]',s[a:])
 result=[];level=0
 for t in ts:
  if t.startswith(('//','/*')):continue
  result.append(t)
  if t=='{':level+=1
  elif t=='}':
   level-=1
   if not level:return result
 raise ValueError(sig)
legacy=(r/'net/http/content_disposition_http_util_reference.cc').read_text().replace('LegacyHttpUtil','HttpUtil').replace('StringViewTokenizer','base::StringViewTokenizer').replace('base::base::','base::').replace('StringTokenizer::','base::StringTokenizer::')
orig=subprocess.check_output(['D:/RustiumBuild/src/buildtools/win-format/clang-format.exe','--assume-filename=D:/Rustium/net/http/http_util.cc'],input=(o/'net/http/http_util.cc').read_bytes()).decode()
sigs=['std::string_view HttpUtil::TrimLWS(', 'void HttpUtil::TrimLWS(std::string_view','bool HttpUtil::IsTokenChar(', 'bool HttpUtil::IsToken(', 'bool UnquoteImpl(', 'std::string HttpUtil::Unquote(', 'bool HttpUtil::StrictUnquote(', 'HttpUtil::ValuesIterator::ValuesIterator(std::string_view', 'bool HttpUtil::ValuesIterator::GetNext(', 'HttpUtil::NameValuePairsIterator::NameValuePairsIterator(std::string_view','bool HttpUtil::NameValuePairsIterator::GetNext(', 'bool HttpUtil::NameValuePairsIterator::ParseNameValuePair(']
# Whitespace in signatures is irrelevant; search on normalized strings.
checks=[]
for sig in sigs:
 a=' '.join(orig.split());b=' '.join(legacy.split());s=' '.join(sig.split())
 # Formatting may introduce a space/newline following '('.
 compact=lambda t:re.sub(r'\s+','',t)
 # Locate signatures by a compact prefix, then compare original body tokens.
 def find(src,sig):
  words=[re.escape(t) for t in re.findall(r'\w+|[^\s\w]',sig)]
  m=re.search(r'\s*'.join(words),src)
  if not m:raise ValueError(sig)
  return body(src,src[m.start():m.end()])
 eq=find(orig,sig)==find(legacy,sig);checks.append({'helper':sig,'equal_tokens':eq});assert eq,sig
# Baseline production algorithm remains exact modulo deliberate renamed calls.
s=(r/'net/http/content_disposition_reference.cc').read_text().replace('ContentDispositionReference','HttpContentDisposition').replace('content_disposition_reference::LegacyHttpUtil','HttpUtil').replace('content_disposition_reference::StringViewTokenizer','base::StringViewTokenizer').replace('content_disposition_reference::StringTokenizer','base::StringTokenizer').replace('content_disposition_reference::Base64Decode','base::Base64Decode').replace('content_disposition_reference::UnescapeBinaryURLComponent','base::UnescapeBinaryURLComponent').replace('content_disposition_reference::ConvertTo','ConvertTo')
orig=subprocess.check_output(['D:/RustiumBuild/src/buildtools/win-format/clang-format.exe','--assume-filename=D:/Rustium/net/http/http_content_disposition.cc'],input=(o/'net/http/http_content_disposition.cc').read_bytes()).decode()
for sig in ['bool DecodeQEncoding(', 'bool DecodeBQEncoding(', 'bool DecodeWord(', 'bool DecodeFilenameValue(', 'bool ParseExtValueComponents(', 'bool DecodeExtValue(', 'std::string_view HttpContentDisposition::ConsumeDispositionType(', 'void HttpContentDisposition::Parse(']:
 eq=body(s,sig)==body(orig,sig);checks.append({'helper':sig,'equal_tokens':eq});assert eq,sig
services=(r/'net/http/content_disposition_services_reference.cc').read_text().replace('content_disposition_modp_b64_', 'modp_b64_')
for source, signatures in [
 ('base/base64.cc',['ModpDecodePolicy GetModpPolicy(', 'bool Base64Decode(std::string_view']),
 ('base/strings/escape.cc',['bool UnescapeUnsignedByteAtIndex(', 'std::string UnescapeBinaryURLComponent(']),
 ('net/base/net_string_util_icu.cc',['bool ConvertToUtf8(', 'bool ConvertToUtf8AndNormalize(', 'bool ConvertToUTF16('])]:
 orig=subprocess.check_output(['D:/RustiumBuild/src/buildtools/win-format/clang-format.exe','--assume-filename=D:/Rustium/'+source],input=(o/source).read_bytes()).decode()
 for sig in signatures:
  eq=body(services,sig)==body(orig,sig);checks.append({'helper':sig,'equal_tokens':eq});assert eq,sig
(w/'reference-body-verification.json').write_text(json.dumps(checks,indent=2)+'\n');print(len(checks),'reference bodies match frozen originals')
