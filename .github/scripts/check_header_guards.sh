#!/bin/bash

find src tests/src -name '*.c' -or -name '*.h' -or -name '*.cpp' -or -name '*.hpp' | xargs clang-format --dry-run --Werror --sort-includes

failures=0
while IFS= read -r file; do
    guard=${file##*include/}
    guard=${guard^^}
    guard=$(echo "$guard" | tr . _ | tr / _)
    # echo "header check $file has guard $guard"
    if ! head -1 "$file" | grep "#ifndef ${guard}" >/dev/null; then
        echo "$file failed typeguard check"
        failures=1
    elif ! head -2 "$file" | tail -1 | grep "#define ${guard}" >/dev/null; then
        echo "$file failed typeguard check"
        failures=1
    elif ! tail -1 "$file" | grep "#endif // ${guard}" >/dev/null; then
        echo "$file failed typeguard check"
        failures=1
    fi
done < <(find src -name '*.h')

if [ "$failures" -ne 0 ]; then
    exit 1
fi
