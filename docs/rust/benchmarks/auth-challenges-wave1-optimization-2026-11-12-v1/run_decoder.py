from pathlib import Path
import subprocess,json,hashlib
w=Path('D:/Rustium/.scratch/net-wave1/auth-challenges/optimization');cmd=['python',str(w/'decoder_diff.py')];p=subprocess.run(cmd,cwd='D:/Rustium',capture_output=True,text=True);(w/'decoder-diff.log').write_text(p.stdout+p.stderr);(w/'decoder-execution.json').write_text(json.dumps({'command':cmd,'exit_code':p.returncode,'binary_sha256':hashlib.sha256((w/'decoder_diff.exe').read_bytes()).hexdigest(),'benchmark':False},indent=2)+'\n');print(p.stdout);p.check_returncode()
