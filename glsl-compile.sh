#!/bin/bash
set -u
cd "$(dirname "$0")" || exit 1

dirInclude="shaders/include"
dirIn="shaders/src"
dirOut="shaders/out"
dirCache="shaders/cache"
mkdir -p "$dirOut" "$dirCache/include" "$dirCache/src"

sameContent() {
    [[ -f "$2" ]] && [[ "$(< "$1")" == "$(< "$2")" ]]
}

includesChanged=false
for file in "$dirInclude"/*; do
    if ! sameContent "$file" "$dirCache/include/$(basename "$file")"; then
        includesChanged=true
        break
    fi
done

success=0
skip=0
fail=0
for target in "$dirIn"/*; do
    [ -f "$target" ] || continue
    name=$(basename "$target")
    compiled="$dirOut/$name.spv"
    cache="$dirCache/src/$name"
    if [ "$includesChanged" = false ] && [ -f "$compiled" ] && sameContent "$target" "$cache"; then
        skip=$((skip + 1))
        continue
    fi
    if glslc -O "$target" -I "$dirInclude" -o "$compiled.tmp"; then
        mv "$compiled.tmp" "$compiled"
        cp "$target" "$cache"
        success=$((success + 1))
        printf 'Compiled %s\n' "$name"
    else
        fail=$((fail + 1))
        printf 'Failed to compile %s\n' "$name" >&2
    fi
done

# Failed includes must invalidate the cache on the next attempt too.
if [ "$fail" -eq 0 ]; then
    for file in "$dirInclude"/*; do
        cp "$file" "$dirCache/include/$(basename "$file")"
    done
fi
printf 'Shaders: %d compiled, %d cached, %d failed\n' "$success" "$skip" "$fail"
[ "$fail" -eq 0 ]
