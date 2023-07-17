
#!/bin/bash
APP_DIR=$(dirname $(readlink -f "$0"))
export LD_LIBRARY_PATH=$APP_DIR../api/v6.6.9_traderapi_20220920/v6.6.9_20220914_api_linux64:$LD_LIBRARY_PATH

example runtime