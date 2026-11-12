from pathlib import Path
root=Path('D:/Rustium'); orig=root/'.scratch/net-wave1/auth-challenges/original/third_party/modp_b64'; dest=root/'net/http'
for src,out in [('modp_b64.cc','auth_challenge_reference_modp.cc'),('modp_b64.h','auth_challenge_reference_modp.h'),('modp_b64_data.h','auth_challenge_reference_modp_data.h')]:
 s=(orig/src).read_text()
 for a,b in [('modp_b64_data.h','auth_challenge_reference_modp_data.h'),('modp_b64.h','auth_challenge_reference_modp.h'),('modp_b64_encode_data','auth_challenge_ref_modp_encode_data'),('modp_b64_encode','auth_challenge_ref_modp_encode'),('modp_b64_decode','auth_challenge_ref_modp_decode'),('do_decode_padding','auth_challenge_ref_do_decode_padding'),('ModpDecodePolicy','AuthChallengeRefDecodePolicy'),('MODP_B64_ERROR','AUTH_CHALLENGE_REF_B64_ERROR'),('MODP_B64_MAX_INPUT_LEN','AUTH_CHALLENGE_REF_B64_MAX_INPUT_LEN'),('MODP_B64','AUTH_CHALLENGE_REF_MODP_B64')]: s=s.replace(a,b)
 (dest/out).write_text(s)
