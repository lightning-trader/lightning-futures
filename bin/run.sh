
#!/bin/bash
APP_DIR=$(dirname $(readlink -f "$0"))
export LD_LIBRARY_PATH=$APP_DIR../api/CTP_V6.7.9_20250319/linux64:$LD_LIBRARY_PATH

example runtime