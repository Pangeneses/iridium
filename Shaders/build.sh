#!/bin/bash
cd "$(dirname "$0")"   # run from the script's own folder -- put the script in Shaders/
shopt -s nullglob
for f in *.vert *.frag *.comp *.task *.mesh; do
    if glslangValidator -V --target-env vulkan1.3 "$f" -o "$f.spv" > /tmp/glsl.log 2>&1; then
        echo "ok    $f"
    else
        echo "FAIL  $f"; cat /tmp/glsl.log
    fi
done
