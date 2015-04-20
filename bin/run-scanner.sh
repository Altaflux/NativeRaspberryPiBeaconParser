#!/bin/bash

# Absolute path to this script
SCRIPT=$(readlink -f $0)
BIN=`dirname $SCRIPT`
ROOT=`dirname $BIN`

# Source the scanner configuration
# Typically this should have:
# - scannerID ; the name by which the scanner is defined in the beacon event messages
# - heartbeatUUID ; the uuid of the beacon used as the heartbeat for the scanner
# - brokerURL ; the url of the activemq broker
. ~/scanner.conf

# Aliases for backward compatibility to previous variables used from scanner.conf
scannerID=${scannerID:=$SCANNER_ID}
heartbeatUUID=${HEARTBEAT_UUID:=$heartbeatUUID}
brokerURL=${BROKER_URL:=$brokerURL}

# If scanner.conf defined a systemType, export it as SYSTEM_TYPE
if [ -n "$systemType" ]; then
        echo "Exporting SYSTEM_TYPE=$systemType"
        export SYSTEM_TYPE=$systemType
fi

# Bring up bluetooth interface
hciconfig hci0 up

ps -eaf | grep lescan | grep -v grep >/dev/null
found=$?
# non-zero return means lescan not seen running
if [ $found -ne 0 ]; then
        hcitool lescan --duplicates >/dev/null &
else
        echo lescan already running
fi

CMD="${ROOT}/Debug/src2/NativeScannerBlueZ"
ARGS="--scannerID "${scannerID:-`hostname`}" --brokerURL "${brokerURL:-192.168.1.107:5672}" --heartbeatUUID "${heartbeatUUID}""
BACKGROUND=""
# Check for a background argument
if [ "$1" == "-background" ]; then
        BACKGROUND=" >& /var/log/beacon-scanner.log &"
        shift
fi
ARGS="${ARGS} $*"
CMD="${CMD} ${ARGS} ${BACKGROUND}"

# Start the scanner
echo "Running: $CMD"
eval $CMD
