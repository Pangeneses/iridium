#!/usr/bin/env bash
# Compiles every shader source in a directory to <name>.<stage>.spv beside it (what Ir77PVShader::LoadShaders loads).
# Stage comes from the extension: vert tesc tese geom frag comp task mesh rgen rint rahit rchit rmiss rcall.
# Runtime-compiled shaders (field.comp / field.frag) live in <root>/generated and are not touched.
# Usage: ./compile_shaders.sh [shader dir]   (default: the current directory)

dir="${1:-.}"
cd "$dir" || { echo "no such directory: $dir"; exit 1; }

command -v glslangValidator >/dev/null || { echo "glslangValidator not found"; exit 1; }

shopt -s nullglob
failed=0

for src in *.vert *.tesc *.tese *.geom *.frag *.comp *.task *.mesh *.rgen *.rint *.rahit *.rchit *.rmiss *.rcall; do
    if output=$(glslangValidator -V --target-env vulkan1.3 -I. "$src" -o "$src.spv" 2>&1); then
        echo "ok      $src"
    else
        echo "FAILED  $src"
        echo "$output"
        failed=$((failed + 1))
    fi
done

[ "$failed" -eq 0 ] && echo "all compiled" || echo "$failed failed"
exit "$failed"
