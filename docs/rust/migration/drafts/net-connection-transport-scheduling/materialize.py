from pathlib import Path
import hashlib, json, os, subprocess

ROOT = Path(__file__).resolve().parent
BASE = '72ee5ddd7129a311a92b49f3705a031e68cbf4ca'
MAIN = ['net/socket/transport_client_socket_pool.cc',
        'net/socket/transport_client_socket_pool_scheduling_rust.rs',
        'net/socket/transport_client_socket_pool_scheduling_original.h',
        'net/socket/transport_client_socket_pool_scheduling_unittest.cc']
SHARED = ['net/BUILD.gn',
          'net/socket/transport_client_socket_pool_scheduling_ffi.h']

def run(*args, data=None, env=None):
    return subprocess.check_output(['git', *args], input=data, env=env)

def sha(data): return hashlib.sha256(data).hexdigest()

def main():
    index = ROOT / '.proposal-index'
    if index.exists(): raise RuntimeError('Refusing to overwrite existing proposal index')
    env = dict(os.environ, GIT_INDEX_FILE=str(index))
    try:
        run('read-tree', BASE, env=env)
        pins = {}
        for paths, name in [(MAIN, 'production.patch'), (SHARED, 'director-linkage.patch')]:
            # Each patch is independently based on the same pinned tree.
            run('read-tree', BASE, env=env)
            for path in paths:
                raw = (ROOT / 'material' / path).read_bytes()
                oid = run('hash-object', '-w', '--stdin', data=raw).decode().strip()
                run('update-index', '--add', '--cacheinfo', '100644', oid, path, env=env)
                before = subprocess.run(['git', 'show', BASE + ':' + path], capture_output=True)
                pins[path] = {'before_sha256': sha(before.stdout) if before.returncode == 0 else None,
                              'after_sha256': sha(raw), 'after_git_blob': oid}
            patch = run('diff', '--cached', '--binary', '--full-index', BASE, '--', *paths, env=env)
            (ROOT / name).write_bytes(patch)
        # Apply into an isolated index; never inspect or change the checkout.
        run('read-tree', BASE, env=env)
        for name in ('production.patch', 'director-linkage.patch'):
            patch = ROOT / name
            run('apply', '--cached', '--check', str(patch), env=env)
            run('apply', '--cached', str(patch), env=env)
        run('diff', '--cached', '--check', BASE, env=env)
        record = {'production_base': BASE, 'files': pins,
                  'patches': {name: sha((ROOT/name).read_bytes()) for name in ('production.patch','director-linkage.patch')},
                  'validation': 'Applied in sequence to isolated Git index at pinned base; no build or test.'}
        (ROOT/'materialization.json').write_text(json.dumps(record, indent=2)+'\n', encoding='utf-8', newline='\n')
        print(json.dumps(record, indent=2))
    finally:
        if index.exists(): index.unlink()

if __name__ == '__main__': main()
