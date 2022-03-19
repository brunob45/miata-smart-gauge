import platform
import shutil

Import('env')

if platform.system() == 'Windows':
    env.Execute('wsl sh scripts/get_sha.sh')
else:  # Linux
    env.Execute('sh scripts/get_sha.sh')

shutil.copyfile('include/lv_conf.h', 'lib/lvgl/lv_conf.h')
