// =============================================================================================================================================
// orbitals -- a molecule's orbitals or its electron density from field.data (orbitals.glsl's layout, version 1), on a slice through it.
//
// The slice is a plane through the molecule, cut along world XY, XZ or YZ and set off along its normal; the surface grid lies on it in its
// true place. Each point's value is the orbital (signed) or the density there -- the colour map and contours show it, and height lifts the
// sheet off the plane by it (0: flat). Nothing uploaded: a flat sheet.
//
// Panel settings:
//   topology surface, dims [512, 512, 1], x [-4, 4], y [-4, 4] (bohr -- the slice's extent), z [-1, 1]
//   shading contours, colormap turbo, value [-0.4, 0.4] (an orbital; the density: [0, 0.5]), contour spacing 0.05, width 1
//   params [-1, 0, 0, -1, 0, 0, 0, 0]
//
// Params -- the panel's sliders run -1 .. 1, so each is mapped:
//   p[0] what    -1 the total density; then the orbitals in the data's order, evenly across to the last at +1
//   p[1] height  x 8 -- bohr the sheet lifts per unit of value; 0 flat (try 0.5 for an orbital)
//   p[2] scale   2^(2 p) world units per bohr -- 0: 1, +1: 4, -1: 0.25
//   p[3] plane   below -1/3: XY (upright, facing Z); to +1/3: XZ (level); above: YZ (upright, facing X) -- world axes, Y up
//   p[4] offset  x 4 -- the plane's distance along its normal, bohr
// =============================================================================================================================================

#include "include/orbitals.glsl"

Ir77FieldPoint Field(Ir77FieldIn f) {
    Ir77Orbitals o = Ir77OrbitalsLayout();

    // the sliders' -1 .. 1 -> what each means
    int what = int((f.params[0] + 1.0) * 0.5 * float(o.orbitals) + 0.5);  // 0 density, 1.. orbital
    float height = f.params[1] * 8.0;
    float scale = exp2(f.params[2] * 2.0);
    int plane = f.params[3] < -0.333 ? 0 : f.params[3] < 0.333 ? 1 : 2;
    float offset = f.params[4] * 4.0;

    // the same params and data as last frame -- every point is where it was (big bases are costly per point)
    float key = f.params[0] * 1.0 + f.params[1] * 7.13 + f.params[2] * 3.17 + f.params[3] * 11.3 + f.params[4] * 5.71 + float(Ir77FieldDataCount()) * 0.001 +
                o.energy * 0.37;
    if (!f.first && f.prev.data.y == key) return f.prev;

    // the grid on the plane -- u, v in bohr
    vec2 uv = f.coord.xy;
    vec3 p = plane == 0 ? vec3(uv.x, uv.y, offset) : plane == 1 ? vec3(uv.x, offset, uv.y) : vec3(offset, uv.y, uv.x);
    vec3 normal = plane == 0 ? vec3(0.0, 0.0, 1.0) : plane == 1 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);

    float value = 0.0;
    if (o.valid) value = what <= 0 ? Ir77OrbitalDensity(o, p) : Ir77Orbital(o, uint(what - 1), p);

    Ir77FieldPoint point;
    point.position = (p + normal * value * height) * scale;
    point.size = 0.0;
    point.color = vec4(0.85, 0.85, 0.85, 1.0);
    point.data = vec4(value, key, 0.0, 0.0);

    return point;
}
