#!/usr/bin/env bash
# Compiles every shader source in a directory to <name>.<stage>.spv beside it (what Ir77PVShader::LoadShaders loads).
# Stage comes from the extension: vert tesc tese geom frag comp task mesh rgen rint rahit rchit rmiss rcall.
# Runtime-compiled shaders (field.comp / field.frag) live in <root>/generated and are not touched.
# frag.frag is also built with -DIR77_RAY_SHADOWS as frag_ray.frag.spv (ray-traced shadows on ray-query devices).
# Usage: ./shader.sh [shader dir]   (default: the current directory)

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

# ray-shadow builds -- the same source again with IR77_RAY_SHADOWS, as <name>_ray.<stage>.spv (the manifest's "fragment_ray"). On a device
# without ray queries the registry skips them with a warning and the plain builds are used.
for src in frag.frag; do
    out="${src%.*}_ray.${src##*.}.spv"

    if output=$(glslangValidator -V --target-env vulkan1.3 -I. -DIR77_RAY_SHADOWS "$src" -o "$out" 2>&1); then
        echo "ok      $src (ray)"
    else
        echo "FAILED  $src (ray)"
        echo "$output"
        failed=$((failed + 1))
    fi
done

[ "$failed" -eq 0 ] && echo "all compiled" || echo "$failed failed"
exit "$failed"
