import platform

Import('env')

if platform.system() == 'Windows':
    env.Execute('wsl sh scripts/get_sha.sh')
else: # Linux
    env.Execute('sh scripts/get_sha.sh')
