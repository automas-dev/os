#!/bin/bash

failures=0
while IFS= read -r file; do
    service_name=${file##*src/kernel/src/}
    service_name=${service_name%.c}
    service_name=${service_name^^}
    # echo "check source $file has log service name $service_name"
    if grep -e '#include <kernel/logs.h>' "$file" >/dev/null; then
        echo "[WARNING] Using <> to import kernel/logs.h in $file"
        failure=1
    elif ! grep -e '#include "kernel/logs.h"' "$file" >/dev/null; then
        echo "Skipping log service name check in file $file"
    elif ! head -1 "$file" | grep "#define KLOG_SERVICE \"${service_name}\"" >/dev/null; then
        echo "$file failed service name check"
        failures=1
    fi
done < <(find src/kernel/src -name '*.c')

if [ "$failures" -ne 0 ]; then
    exit 1
fi
