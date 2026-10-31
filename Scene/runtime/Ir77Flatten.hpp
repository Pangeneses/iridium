#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Flatten -- an imported .glb made into one entity's file: everything the file draws becomes one mesh on one root node, so the editor
// sees one entity per file (its parts are entities of their own only when they come in as files of their own).
//
//   geometry    every mesh instance in the default scene, its node's world transform baked into the vertices (positions, normals, tangents,
//               morph deltas; mirrored nodes get their winding flipped). Each source primitive stays a primitive (its material kept).
//   skeleton    every joint of every skin, and their ancestors, stay nodes under the root with their own transforms -- one merged skin.
//               Skinned primitives keep their weights (joint indices remapped). In a skinned file, geometry that isn't skinned is bound
//               rigidly to its nearest kept ancestor (a sword under a hand follows the hand), or to a fixed joint when it has none.
//   clips       translation / rotation / scale channels on kept nodes stay. Channels on any other node are dropped and reported -- a part
//               that moves on its own is its own file. Morph weight channels move to the root, one channel per clip, over the merged
//               targets (clips keyframed apart are resampled together).
//   morphs      every mesh's targets are kept, merged into one target list (a primitive gets zero deltas for the targets that aren't its own);
//               their names (extras.targetNames) too.
//   dropped     the file's cameras and lights (an import is geometry, materials and animation), and its other scenes.
//
// Materials, textures and samplers are copied from the source's JSON as they are (every extension and extra they carry survives); images
// move into the new buffer. The .glb is written here -- fastgltf parses the source, but its 0.8 exporter drops morph targets and rounds
// accessor bounds. The report says what was left out -- never silently.
// =============================================================================================================================================
class Ir77Flatten {
   public:
    // path -> glb (the bytes to write). name = the root node's name (the model's). false + error when the file can't be read or draws nothing.
    bool Run(std::string const& path, std::string const& name, std::vector<std::byte>& glb, std::vector<std::string>& report, std::string& error) {
        m_report.clear();
        m_bin.clear();
        m_views.clear();
        m_accessors.clear();
        m_images.clear();
        m_animations.clear();
        m_primitives.clear();
        m_target_names.clear();

        if (!Parse(path, error)) return false;

        Walk();
        BuildSkeleton();
        if (!BuildGeometry(error)) return false;
        BuildMorphs();
        BuildAnimations();
        if (!BuildImages(error)) return false;

        glb = WriteGlb(name);
        report = m_report;
        return true;
    }

   private:
    static constexpr std::size_t NONE{SIZE_MAX};

    using Attributes = std::vector<std::pair<std::string, std::size_t>>;  // attribute -> accessor

    // one output primitive -- its own targets wait for the merged target count
    struct Primitive {
        Attributes attributes{};
        std::size_t indices{NONE};
        std::optional<std::size_t> material{};
        fastgltf::PrimitiveType mode{fastgltf::PrimitiveType::Triangles};
        std::size_t vertices{0};
        std::size_t target_base{0};
        std::vector<Attributes> targets{};      // its mesh's targets
        std::vector<Attributes> all_targets{};  // the merged list -- what's written
    };

    // a mesh on a node -- its slice of the merged targets
    struct Instance {
        std::size_t node{0};
        std::size_t mesh{0};
        std::size_t target_base{0};
        std::size_t target_count{0};
    };

    struct View {
        std::size_t offset{0};
        std::size_t length{0};
        int target{0};  // 34962 vertices, 34963 indices, 0 neither
    };

    struct Accessor {
        std::size_t view{0};
        std::size_t count{0};
        fastgltf::AccessorType type{fastgltf::AccessorType::Scalar};
        int component{5126};
        std::vector<float> min{};
        std::vector<float> max{};
    };

    struct Sampler {
        std::size_t input{0};
        std::size_t output{0};
        fastgltf::AnimationInterpolation interpolation{fastgltf::AnimationInterpolation::Linear};
    };

    struct Channel {
        std::size_t sampler{0};
        std::size_t node{0};
        fastgltf::AnimationPath path{fastgltf::AnimationPath::Translation};
    };

    struct Animation {
        std::string name{};
        std::vector<Sampler> samplers{};
        std::vector<Channel> channels{};
    };

    struct Image {
        std::string name{};
        std::size_t view{0};
        std::string mime{};
    };

    // ---- reading ----------------------------------------------------------------------------------------------------------------------------

    bool Parse(std::string const& path, std::string& error) {
        constexpr auto extensions =
            fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform;
        constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices |
                                 fastgltf::Options::DecomposeNodeMatrices;

        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (data.error() != fastgltf::Error::None) {
            error = "couldn't read " + path + ": " + std::string{fastgltf::getErrorMessage(data.error())};
            return false;
        }

        fastgltf::Parser parser(extensions);
        auto asset = parser.loadGltf(data.get(), std::filesystem::path(path).parent_path(), options);
        if (asset.error() != fastgltf::Error::None) {
            error = "couldn't parse " + path + ": " + std::string{fastgltf::getErrorMessage(asset.error())};
            return false;
        }

        m_asset = std::move(asset.get());

        std::size_t const scene = m_asset.defaultScene.value_or(0);
        if (scene >= m_asset.scenes.size()) {
            error = path + " has no scene";
            return false;
        }

        m_scene = scene;
        if (m_asset.scenes.size() > 1) m_report.push_back("only the default scene was imported (the file has " + std::to_string(m_asset.scenes.size()) + ")");

        // the source's JSON text -- materials, textures and samplers are copied from it as written
        if (!ReadJson(path, error)) return false;

        return true;
    }

    // a .glb's JSON chunk
    bool ReadJson(std::string const& path, std::string& error) {
        std::ifstream file{path, std::ios::binary};
        std::array<std::uint32_t, 5> header{};
        file.read(reinterpret_cast<char*>(header.data()), sizeof(header));

        if (!file || header[0] != 0x46546C67u || header[4] != 0x4E4F534Au) {  // 'glTF', chunk 'JSON'
            error = path + " isn't a .glb";
            return false;
        }

        m_json.assign(header[3], '\0');
        file.read(m_json.data(), static_cast<std::streamsize>(m_json.size()));
        if (!file) {
            error = path + " is cut short";
            return false;
        }

        return true;
    }

    // a node's own transform -- TRS (DecomposeNodeMatrices)
    glm::mat4 Local(std::size_t const node) const {
        auto const* trs = std::get_if<fastgltf::TRS>(&m_asset.nodes[node].transform);
        if (!trs) return glm::mat4(1.0f);

        glm::vec3 const t(trs->translation[0], trs->translation[1], trs->translation[2]);
        glm::quat const r(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);
        glm::vec3 const s(trs->scale[0], trs->scale[1], trs->scale[2]);

        return glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
    }

    // the default scene's nodes, parents first, with their rest world transforms; the mesh instances; the file's cameras and lights counted
    void Walk() {
        std::size_t const count = m_asset.nodes.size();

        m_parent.assign(count, NONE);
        m_world.assign(count, glm::mat4(1.0f));
        m_in_scene.assign(count, false);
        m_order.clear();
        m_instances.clear();
        m_instance_of.assign(count, NONE);

        std::size_t cameras{0};
        std::size_t lights{0};

        std::vector<std::pair<std::size_t, std::size_t>> stack{};  // node, parent
        auto const& roots = m_asset.scenes[m_scene].nodeIndices;
        for (auto it = roots.rbegin(); it != roots.rend(); ++it) stack.push_back({*it, NONE});

        while (!stack.empty()) {
            auto const [node, parent] = stack.back();
            stack.pop_back();

            if (node >= count || m_in_scene[node]) continue;  // a broken or cyclic file -- once each

            m_in_scene[node] = true;
            m_parent[node] = parent;
            m_world[node] = (parent == NONE ? glm::mat4(1.0f) : m_world[parent]) * Local(node);
            m_order.push_back(node);

            fastgltf::Node const& n = m_asset.nodes[node];
            if (n.cameraIndex.has_value()) cameras++;
            if (n.lightIndex.has_value()) lights++;

            if (n.meshIndex.has_value() && n.meshIndex.value() < m_asset.meshes.size()) {
                m_instance_of[node] = m_instances.size();
                m_instances.push_back(Instance{node, n.meshIndex.value(), 0, 0});
            }

            for (auto it = n.children.rbegin(); it != n.children.rend(); ++it) stack.push_back({*it, node});
        }

        if (cameras > 0) m_report.push_back(std::to_string(cameras) + " camera(s) in the file left out -- add cameras in the editor");
        if (lights > 0) m_report.push_back(std::to_string(lights) + " light(s) in the file left out -- add lights in the editor");
    }

    // ---- the skeleton -----------------------------------------------------------------------------------------------------------------------

    // kept nodes = every joint of every skin a mesh uses, and their ancestors. Output nodes: 0 the root, 1.. the kept ones in walk order.
    // Joints of the merged skin = the kept nodes (joint j = output node j + 1), then the fixed joint when unskinned geometry needs one.
    void BuildSkeleton() {
        std::size_t const count = m_asset.nodes.size();

        m_kept.assign(count, false);
        m_out_index.assign(count, NONE);
        m_kept_order.clear();
        m_skins_used.clear();

        for (Instance const& instance : m_instances) {
            auto const& skin = m_asset.nodes[instance.node].skinIndex;
            if (skin.has_value() && skin.value() < m_asset.skins.size()) m_skins_used.insert(skin.value());
        }

        for (std::size_t const s : m_skins_used) {
            for (std::size_t const joint : m_asset.skins[s].joints) {
                for (std::size_t n = joint; n < count && m_in_scene[n] && !m_kept[n]; n = m_parent[n]) {
                    m_kept[n] = true;
                    if (m_parent[n] == NONE) break;
                }
            }
        }

        for (std::size_t const n : m_order) {
            if (!m_kept[n]) continue;
            m_out_index[n] = 1 + m_kept_order.size();
            m_kept_order.push_back(n);
        }

        m_skinned = !m_kept_order.empty();

        // inverse binds -- the first skin that lists a joint gives it its bind; an ancestor that's no skin's joint binds at rest
        m_inverse_bind.assign(m_kept_order.size(), glm::mat4(1.0f));
        std::vector<bool> bound(m_kept_order.size(), false);
        std::map<std::size_t, std::vector<glm::mat4>> skin_binds{};

        for (std::size_t const s : m_skins_used) {
            std::vector<glm::mat4>& binds = skin_binds[s];
            fastgltf::Skin const& skin = m_asset.skins[s];

            binds.assign(skin.joints.size(), glm::mat4(1.0f));
            if (skin.inverseBindMatrices.has_value()) {
                std::size_t j{0};
                fastgltf::iterateAccessor<glm::mat4>(m_asset, m_asset.accessors[skin.inverseBindMatrices.value()], [&](glm::mat4 m) {
                    if (j < binds.size()) binds[j] = m;
                    j++;
                });
            }

            for (std::size_t j = 0; j < skin.joints.size(); j++) {
                std::size_t const node = skin.joints[j];
                if (node >= count || m_out_index[node] == NONE) continue;

                std::size_t const k = m_out_index[node] - 1;
                if (!bound[k]) {
                    m_inverse_bind[k] = binds[j];
                    bound[k] = true;
                }
            }
        }

        for (std::size_t k = 0; k < m_kept_order.size(); k++) {
            if (!bound[k]) m_inverse_bind[k] = glm::inverse(m_world[m_kept_order[k]]);
        }

        // each skin's correction onto the merged binds -- one matrix for all its joints, or the skin can't merge
        m_skin_fix.clear();
        for (std::size_t const s : m_skins_used) {
            fastgltf::Skin const& skin = m_asset.skins[s];
            std::vector<glm::mat4> const& binds = skin_binds[s];

            bool first{true};
            bool consistent{true};
            glm::mat4 fix(1.0f);

            for (std::size_t j = 0; j < skin.joints.size(); j++) {
                std::size_t const node = skin.joints[j];
                if (node >= count || m_out_index[node] == NONE) continue;

                glm::mat4 const c = glm::inverse(m_inverse_bind[m_out_index[node] - 1]) * binds[j];
                if (first) {
                    fix = c;
                    first = false;
                } else if (!Close(c, fix)) {
                    consistent = false;
                    break;
                }
            }

            if (consistent) {
                m_skin_fix[s] = fix;
            } else {
                m_report.push_back("skin '" + std::string{skin.name} + "' is bound in a different pose from the others it shares bones with -- its meshes were left out");
            }
        }

        m_fixed_joint = NONE;
    }

    static bool Close(glm::mat4 const& a, glm::mat4 const& b) {
        float scale{1.0f};
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++) scale = std::max(scale, std::abs(b[c][r]));

        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                if (std::abs(a[c][r] - b[c][r]) > 1e-3f * scale) return false;

        return true;
    }

    // the nearest kept node at or above node -- NONE when it has none
    std::size_t KeptAncestor(std::size_t node) const {
        while (node != NONE && !m_kept[node]) node = m_parent[node];
        return node;
    }

    // the fixed joint -- made on first use: a node under the root that never moves, for unskinned geometry with no kept ancestor
    std::size_t FixedJoint() {
        if (m_fixed_joint == NONE) m_fixed_joint = m_kept_order.size();
        return m_fixed_joint;
    }

    // ---- geometry ---------------------------------------------------------------------------------------------------------------------------

    bool BuildGeometry(std::string& error) {
        m_primitives.clear();

        std::size_t skipped{0};
        std::size_t target_base{0};

        for (Instance& instance : m_instances) {
            fastgltf::Mesh const& mesh = m_asset.meshes[instance.mesh];
            fastgltf::Node const& node = m_asset.nodes[instance.node];

            // the mesh's target count -- its primitives must agree; the widest wins, the rest get zeros
            std::size_t targets{mesh.weights.size()};
            for (auto const& p : mesh.primitives) targets = std::max(targets, p.targets.size());

            instance.target_base = target_base;
            instance.target_count = targets;
            target_base += targets;

            std::optional<std::size_t> skin{};
            if (node.skinIndex.has_value() && m_skin_fix.count(node.skinIndex.value())) skin = node.skinIndex.value();
            bool const skin_dropped = node.skinIndex.has_value() && m_skins_used.count(node.skinIndex.value()) && !skin.has_value();

            for (fastgltf::Primitive const& source : mesh.primitives) {
                if (source.findAttribute("POSITION") == source.attributes.end()) {
                    skipped++;
                    continue;
                }

                bool const skinned = skin.has_value() && source.findAttribute("JOINTS_0") != source.attributes.end() &&
                                     source.findAttribute("WEIGHTS_0") != source.attributes.end();
                if (skin_dropped && source.findAttribute("JOINTS_0") != source.attributes.end()) continue;  // reported with its skin

                // where its vertices go: a skinned primitive into the merged binds; anything else to its world place -- bound to a joint when
                // the file has a skeleton
                glm::mat4 matrix(1.0f);
                std::size_t joint{NONE};

                if (skinned) {
                    matrix = m_skin_fix[skin.value()];
                } else if (m_skinned) {
                    std::size_t const kept = KeptAncestor(instance.node);
                    if (kept == NONE) {
                        joint = FixedJoint();
                        matrix = m_world[instance.node];
                    } else {
                        joint = m_out_index[kept] - 1;
                        matrix = glm::inverse(m_inverse_bind[joint]) * glm::inverse(m_world[kept]) * m_world[instance.node];
                    }
                } else {
                    matrix = m_world[instance.node];
                }

                Primitive out{};
                if (!BuildPrimitive(source, skinned ? &m_asset.skins[skin.value()] : nullptr, joint, matrix, targets, out, error)) return false;

                out.target_base = instance.target_base;
                m_primitives.push_back(std::move(out));
            }
        }

        m_target_count = target_base;

        if (skipped > 0) m_report.push_back(std::to_string(skipped) + " primitive(s) without positions left out");

        if (m_primitives.empty()) {
            error = "nothing to draw -- the file has no meshes in its scene";
            return false;
        }

        return true;
    }

    bool BuildPrimitive(fastgltf::Primitive const& source, fastgltf::Skin const* skin, std::size_t const joint, glm::mat4 const& matrix,
                        std::size_t const targets, Primitive& out, std::string& error) {
        glm::mat3 const linear(matrix);
        glm::mat3 const normal = glm::transpose(glm::inverse(linear));
        bool const mirrored = glm::determinant(linear) < 0.0f;

        out.mode = source.type;
        if (source.materialIndex.has_value()) out.material = source.materialIndex.value();

        // positions
        fastgltf::Accessor const& positions = m_asset.accessors[source.findAttribute("POSITION")->accessorIndex];
        out.vertices = positions.count;

        std::vector<glm::vec3> position{};
        position.reserve(positions.count);
        fastgltf::iterateAccessor<glm::vec3>(m_asset, positions, [&](glm::vec3 v) { position.push_back(glm::vec3(matrix * glm::vec4(v, 1.0f))); });
        out.attributes.push_back({"POSITION", WriteVec3(position, true)});

        for (auto const& attribute : source.attributes) {
            std::string const name{attribute.name};
            fastgltf::Accessor const& accessor = m_asset.accessors[attribute.accessorIndex];

            if (name == "POSITION") continue;

            if (name == "NORMAL") {
                std::vector<glm::vec3> values{};
                fastgltf::iterateAccessor<glm::vec3>(m_asset, accessor, [&](glm::vec3 v) { values.push_back(SafeNormalize(normal * v)); });
                out.attributes.push_back({name, WriteVec3(values, false)});
            } else if (name == "TANGENT") {
                std::vector<glm::vec4> values{};
                fastgltf::iterateAccessor<glm::vec4>(m_asset, accessor, [&](glm::vec4 v) {
                    values.push_back(glm::vec4(SafeNormalize(linear * glm::vec3(v)), mirrored ? -v.w : v.w));
                });
                out.attributes.push_back({name, WriteFloats(Flatten(values), fastgltf::AccessorType::Vec4, values.size(), 34962)});
            } else if (name.rfind("JOINTS_", 0) == 0) {
                if (!skin) continue;  // bound below

                // its weights -- a joint with no weight stays 0, as it was
                std::vector<glm::vec4> weights{};
                auto const set = source.findAttribute("WEIGHTS_" + name.substr(7));
                if (set != source.attributes.end())
                    fastgltf::iterateAccessor<glm::vec4>(m_asset, m_asset.accessors[set->accessorIndex], [&](glm::vec4 w) { weights.push_back(w); });

                std::vector<std::uint16_t> values{};
                fastgltf::iterateAccessor<glm::u16vec4>(m_asset, accessor, [&](glm::u16vec4 v) {
                    std::size_t const vertex = values.size() / 4;
                    for (int c = 0; c < 4; c++) {
                        std::size_t const node = v[c] < skin->joints.size() ? skin->joints[v[c]] : NONE;
                        std::size_t const out_node = node < m_out_index.size() ? m_out_index[node] : NONE;
                        bool const unused = vertex < weights.size() && weights[vertex][c] == 0.0f;
                        values.push_back(static_cast<std::uint16_t>(out_node == NONE || unused ? 0 : out_node - 1));
                    }
                });
                out.attributes.push_back({name, WriteJoints(values)});
            } else if (name.rfind("WEIGHTS_", 0) == 0) {
                if (!skin) continue;  // bound below

                out.attributes.push_back({name, CopyFloats(accessor, 34962)});
            } else {
                std::size_t const copied = CopyFloats(accessor, 34962);
                if (copied != NONE) out.attributes.push_back({name, copied});
            }
        }

        // unskinned geometry in a skinned file -- all of it on one joint
        if (!skin && joint != NONE) {
            std::vector<std::uint16_t> joints(out.vertices * 4, 0);
            std::vector<float> weights(out.vertices * 4, 0.0f);
            for (std::size_t v = 0; v < out.vertices; v++) {
                joints[v * 4] = static_cast<std::uint16_t>(joint);
                weights[v * 4] = 1.0f;
            }

            out.attributes.push_back({"JOINTS_0", WriteJoints(joints)});
            out.attributes.push_back({"WEIGHTS_0", WriteFloats(weights, fastgltf::AccessorType::Vec4, out.vertices, 34962)});
        }

        // indices -- GenerateMeshIndices gives every primitive some; strips and fans become lists so a mirror can flip them
        if (source.indicesAccessor.has_value()) {
            std::vector<std::uint32_t> indices{};
            fastgltf::iterateAccessor<std::uint32_t>(m_asset, m_asset.accessors[source.indicesAccessor.value()], [&](std::uint32_t i) { indices.push_back(i); });

            if (out.mode == fastgltf::PrimitiveType::TriangleStrip || out.mode == fastgltf::PrimitiveType::TriangleFan) {
                indices = ToTriangles(indices, out.mode);
                out.mode = fastgltf::PrimitiveType::Triangles;
            }

            if (mirrored && out.mode == fastgltf::PrimitiveType::Triangles) {
                for (std::size_t t = 0; t + 2 < indices.size(); t += 3) std::swap(indices[t + 1], indices[t + 2]);
            }

            out.indices = WriteIndices(indices);
        }

        // morph targets -- deltas through the same matrix (no translation)
        out.targets.assign(targets, {});
        for (std::size_t t = 0; t < source.targets.size() && t < targets; t++) {
            for (auto const& attribute : source.targets[t]) {
                std::string const name{attribute.name};
                fastgltf::Accessor const& accessor = m_asset.accessors[attribute.accessorIndex];

                std::size_t written{NONE};
                if (name == "POSITION" || name == "NORMAL" || name == "TANGENT") {
                    glm::mat3 const& m = name == "NORMAL" ? normal : linear;
                    std::vector<glm::vec3> values{};
                    fastgltf::iterateAccessor<glm::vec3>(m_asset, accessor, [&](glm::vec3 v) { values.push_back(m * v); });
                    written = WriteVec3(values, name == "POSITION");
                } else {
                    written = CopyFloats(accessor, 34962);
                }

                if (written != NONE) out.targets[t].push_back({name, written});
            }
        }

        (void)error;
        return true;
    }

    static std::vector<std::uint32_t> ToTriangles(std::vector<std::uint32_t> const& indices, fastgltf::PrimitiveType const type) {
        std::vector<std::uint32_t> list{};
        if (indices.size() < 3) return list;

        for (std::size_t i = 2; i < indices.size(); i++) {
            if (type == fastgltf::PrimitiveType::TriangleFan) {
                list.insert(list.end(), {indices[0], indices[i - 1], indices[i]});
            } else if (i % 2 == 0) {
                list.insert(list.end(), {indices[i - 2], indices[i - 1], indices[i]});
            } else {
                list.insert(list.end(), {indices[i - 1], indices[i - 2], indices[i]});
            }
        }

        return list;
    }

    static glm::vec3 SafeNormalize(glm::vec3 const& v) {
        float const length = glm::length(v);
        return length > 1e-20f ? v / length : glm::vec3(0.0f, 0.0f, 1.0f);
    }

    template <typename T>
    static std::vector<float> Flatten(std::vector<T> const& values) {
        std::vector<float> floats(values.size() * T::length());
        std::memcpy(floats.data(), values.data(), floats.size() * sizeof(float));
        return floats;
    }

    // ---- morph targets ----------------------------------------------------------------------------------------------------------------------

    // every primitive gets the full target list: its own, and zero deltas for the rest. The root mesh's default weights = each instance's;
    // the target names = each mesh's extras.targetNames (or mesh_n).
    void BuildMorphs() {
        m_weights.assign(m_target_count, 0.0f);
        if (m_target_count == 0) return;

        m_target_names.assign(m_target_count, "");

        for (Instance const& instance : m_instances) {
            fastgltf::Node const& node = m_asset.nodes[instance.node];
            fastgltf::Mesh const& mesh = m_asset.meshes[instance.mesh];
            std::vector<std::string> const names = TargetNames(instance.mesh);
            std::string const mesh_name = mesh.name.empty() ? "mesh" + std::to_string(instance.mesh) : std::string{mesh.name};

            for (std::size_t t = 0; t < instance.target_count; t++) {
                float w{0.0f};
                if (node.weights.size() == instance.target_count) {
                    w = static_cast<float>(node.weights[t]);
                } else if (mesh.weights.size() == instance.target_count) {
                    w = static_cast<float>(mesh.weights[t]);
                }
                m_weights[instance.target_base + t] = w;

                m_target_names[instance.target_base + t] = t < names.size() && !names[t].empty() ? names[t] : mesh_name + "_" + std::to_string(t);
            }
        }

        std::map<std::size_t, std::size_t> zeros{};  // vertex count -> its zero accessor

        for (Primitive& primitive : m_primitives) {
            primitive.all_targets.assign(m_target_count, {});

            for (std::size_t t = 0; t < m_target_count; t++) {
                bool const own = t >= primitive.target_base && t - primitive.target_base < primitive.targets.size();
                if (own) primitive.all_targets[t] = primitive.targets[t - primitive.target_base];

                if (primitive.all_targets[t].empty()) {
                    auto found = zeros.find(primitive.vertices);
                    if (found == zeros.end()) found = zeros.emplace(primitive.vertices, WriteVec3(std::vector<glm::vec3>(primitive.vertices, glm::vec3(0.0f)), true)).first;

                    primitive.all_targets[t].push_back({"POSITION", found->second});
                }
            }
        }
    }

    // a source mesh's extras.targetNames -- from its JSON
    std::vector<std::string> TargetNames(std::size_t const mesh) const {
        std::vector<std::string> names{};

        std::string_view const list = Member(Member(Element(Member(m_json, "meshes"), mesh), "extras"), "targetNames");
        for (std::size_t i = 0;; i++) {
            std::string_view const item = Element(list, i);
            if (item.empty()) break;
            names.push_back(Unquote(item));
        }

        return names;
    }

    // ---- animation --------------------------------------------------------------------------------------------------------------------------

    struct Track {
        std::vector<float> times{};
        std::vector<float> values{};  // keys x components (x3 for cubic: in-tangent, value, out-tangent)
        fastgltf::AnimationInterpolation interpolation{fastgltf::AnimationInterpolation::Linear};
        std::size_t input{NONE};
    };

    void BuildAnimations() {
        m_input_copies.clear();

        for (fastgltf::Animation const& animation : m_asset.animations) {
            Animation out{};
            out.name = std::string{animation.name};

            std::set<std::string> dropped{};
            std::vector<std::pair<Instance const*, Track>> weights{};

            for (fastgltf::AnimationChannel const& channel : animation.channels) {
                if (!channel.nodeIndex.has_value() || channel.samplerIndex >= animation.samplers.size()) continue;

                std::size_t const node = channel.nodeIndex.value();
                if (node >= m_asset.nodes.size() || !m_in_scene[node]) continue;

                fastgltf::AnimationSampler const& sampler = animation.samplers[channel.samplerIndex];

                if (channel.path == fastgltf::AnimationPath::Weights) {
                    if (m_instance_of[node] == NONE || m_instances[m_instance_of[node]].target_count == 0) continue;

                    Track track{};
                    track.interpolation = sampler.interpolation;
                    track.input = sampler.inputAccessor;
                    fastgltf::iterateAccessor<float>(m_asset, m_asset.accessors[sampler.inputAccessor], [&](float v) { track.times.push_back(v); });
                    fastgltf::iterateAccessor<float>(m_asset, m_asset.accessors[sampler.outputAccessor], [&](float v) { track.values.push_back(v); });

                    weights.push_back({&m_instances[m_instance_of[node]], std::move(track)});
                    continue;
                }

                if (!m_kept[node]) {
                    dropped.insert(m_asset.nodes[node].name.empty() ? "node " + std::to_string(node) : std::string{m_asset.nodes[node].name});
                    continue;
                }

                Sampler copy{};
                copy.input = CopyInput(sampler.inputAccessor);
                copy.output = CopyFloats(m_asset.accessors[sampler.outputAccessor], 0);
                copy.interpolation = sampler.interpolation;

                if (copy.output == NONE) continue;

                out.channels.push_back(Channel{out.samplers.size(), m_out_index[node], channel.path});
                out.samplers.push_back(copy);
            }

            if (!weights.empty()) MergeWeights(out.name, weights, out);

            std::string const label = out.name.empty() ? "an unnamed clip" : "clip '" + out.name + "'";

            if (!dropped.empty()) {
                std::string list{};
                for (std::string const& d : dropped) list += (list.empty() ? "" : ", ") + d;
                m_report.push_back(label + ": dropped the motion of " + list + " (not bones) -- export a part that moves on its own as its own file");
            }

            if (out.channels.empty()) {
                if (!animation.channels.empty()) m_report.push_back(label + " left out -- nothing it animates is left");
                continue;
            }

            m_animations.push_back(std::move(out));
        }
    }

    // a clip's morph weight channels -> one channel on the root over all the merged targets
    void MergeWeights(std::string const& name, std::vector<std::pair<Instance const*, Track>>& tracks, Animation& out) {
        using Interpolation = fastgltf::AnimationInterpolation;

        bool same_keys{true};
        bool any_cubic{false};
        bool all_step{true};

        for (auto const& [instance, track] : tracks) {
            if (track.input != tracks[0].second.input || track.interpolation != tracks[0].second.interpolation) same_keys = false;
            if (track.interpolation == Interpolation::CubicSpline) any_cubic = true;
            if (track.interpolation != Interpolation::Step) all_step = false;
        }

        if (!same_keys && any_cubic) {
            m_report.push_back("clip '" + name + "': morph tracks keyframed apart with cubic curves can't merge -- only the first mesh's kept");
            tracks.resize(1);
            same_keys = true;
        }

        std::vector<float> times{};
        std::vector<float> values{};
        Interpolation interpolation{tracks[0].second.interpolation};

        if (same_keys) {
            times = tracks[0].second.times;
            std::size_t const per_key = interpolation == Interpolation::CubicSpline ? 3 : 1;

            values.assign(times.size() * per_key * m_target_count, 0.0f);
            for (std::size_t k = 0; k < times.size() * per_key; k++) {
                bool const value_slot = per_key == 1 || k % 3 == 1;
                if (value_slot) std::copy(m_weights.begin(), m_weights.end(), values.begin() + static_cast<std::ptrdiff_t>(k * m_target_count));
            }

            for (auto const& [instance, track] : tracks) {
                for (std::size_t k = 0; k < times.size() * per_key; k++) {
                    for (std::size_t t = 0; t < instance->target_count; t++) {
                        std::size_t const from = k * instance->target_count + t;
                        if (from < track.values.size()) values[k * m_target_count + instance->target_base + t] = track.values[from];
                    }
                }
            }
        } else {
            interpolation = all_step ? Interpolation::Step : Interpolation::Linear;

            std::set<float> keys{};
            for (auto const& [instance, track] : tracks) keys.insert(track.times.begin(), track.times.end());
            times.assign(keys.begin(), keys.end());

            values.assign(times.size() * m_target_count, 0.0f);
            for (std::size_t k = 0; k < times.size(); k++) std::copy(m_weights.begin(), m_weights.end(), values.begin() + static_cast<std::ptrdiff_t>(k * m_target_count));

            for (auto const& [instance, track] : tracks) {
                for (std::size_t k = 0; k < times.size(); k++) {
                    for (std::size_t t = 0; t < instance->target_count; t++) {
                        values[k * m_target_count + instance->target_base + t] = Sample(track, instance->target_count, t, times[k]);
                    }
                }
            }
        }

        if (times.empty()) return;

        Sampler sampler{};
        sampler.input = WriteInput(times);
        sampler.output = WriteFloats(values, fastgltf::AccessorType::Scalar, values.size(), 0);
        sampler.interpolation = interpolation;

        out.channels.push_back(Channel{out.samplers.size(), 0, fastgltf::AnimationPath::Weights});
        out.samplers.push_back(sampler);
    }

    // one weight of a linear / step track at time
    static float Sample(Track const& track, std::size_t const width, std::size_t const t, float const time) {
        auto value = [&](std::size_t k) { return k * width + t < track.values.size() ? track.values[k * width + t] : 0.0f; };

        if (track.times.empty()) return 0.0f;
        if (time <= track.times.front()) return value(0);
        if (time >= track.times.back()) return value(track.times.size() - 1);

        std::size_t const next = static_cast<std::size_t>(std::upper_bound(track.times.begin(), track.times.end(), time) - track.times.begin());
        std::size_t const prev = next - 1;

        if (track.interpolation == fastgltf::AnimationInterpolation::Step) return value(prev);

        float const span = track.times[next] - track.times[prev];
        float const f = span > 0.0f ? (time - track.times[prev]) / span : 0.0f;
        return value(prev) + (value(next) - value(prev)) * f;
    }

    // keyframe times -- shared across channels that shared them
    std::size_t CopyInput(std::size_t const accessor) {
        auto found = m_input_copies.find(accessor);
        if (found != m_input_copies.end()) return found->second;

        std::vector<float> times{};
        fastgltf::iterateAccessor<float>(m_asset, m_asset.accessors[accessor], [&](float v) { times.push_back(v); });

        return m_input_copies[accessor] = WriteInput(times);
    }

    // keyframe times -- min / max are required
    std::size_t WriteInput(std::vector<float> const& times) {
        std::size_t const index = WriteFloats(times, fastgltf::AccessorType::Scalar, times.size(), 0);

        if (!times.empty()) {
            auto const [lo, hi] = std::minmax_element(times.begin(), times.end());
            m_accessors[index].min = {*lo};
            m_accessors[index].max = {*hi};
        }

        return index;
    }

    // ---- images -----------------------------------------------------------------------------------------------------------------------------

    // every image into the new buffer, in the same order -- the copied textures still point at them by index
    bool BuildImages(std::string& error) {
        for (std::size_t i = 0; i < m_asset.images.size(); i++) {
            fastgltf::Image const& image = m_asset.images[i];

            std::vector<std::byte> bytes{};
            fastgltf::MimeType mime{fastgltf::MimeType::None};

            bool const ok = std::visit(fastgltf::visitor{
                                           [&](fastgltf::sources::BufferView const& view) {
                                               auto const span = fastgltf::DefaultBufferDataAdapter{}(m_asset, view.bufferViewIndex);
                                               bytes.assign(span.data(), span.data() + span.size());
                                               mime = view.mimeType;
                                               return true;
                                           },
                                           [&](fastgltf::sources::Array const& array) {
                                               bytes.assign(array.bytes.begin(), array.bytes.end());
                                               mime = array.mimeType;
                                               return true;
                                           },
                                           [&](fastgltf::sources::Vector const& vector) {
                                               bytes = vector.bytes;
                                               mime = vector.mimeType;
                                               return true;
                                           },
                                           [&](auto const&) { return false; },
                                       },
                                       image.data);

            if (!ok) {
                error = "image " + (image.name.empty() ? std::to_string(i) : std::string{image.name}) + " couldn't be read -- re-export the .glb with its textures embedded";
                return false;
            }

            Image out{};
            out.name = std::string{image.name};
            out.view = WriteView(bytes.data(), bytes.size(), 0);
            out.mime = MimeName(mime, bytes);
            m_images.push_back(std::move(out));
        }

        return true;
    }

    static std::string MimeName(fastgltf::MimeType const mime, std::vector<std::byte> const& bytes) {
        switch (mime) {
            case fastgltf::MimeType::PNG: return "image/png";
            case fastgltf::MimeType::JPEG: return "image/jpeg";
            case fastgltf::MimeType::KTX2: return "image/ktx2";
            case fastgltf::MimeType::DDS: return "image/vnd-ms.dds";
            default: break;
        }

        auto at = [&](std::size_t i) { return i < bytes.size() ? static_cast<unsigned char>(bytes[i]) : 0u; };
        if (at(0) == 0xFF && at(1) == 0xD8) return "image/jpeg";
        if (at(0) == 0xAB && at(1) == 'K' && at(2) == 'T' && at(3) == 'X') return "image/ktx2";
        if (at(0) == 'R' && at(1) == 'I' && at(2) == 'F' && at(3) == 'F') return "image/webp";
        return "image/png";
    }

    // ---- the new file -----------------------------------------------------------------------------------------------------------------------

    std::vector<std::byte> WriteGlb(std::string const& name) {
        // inverse binds -- the kept nodes', then the fixed joint's
        std::size_t binds{NONE};
        if (m_skinned) {
            std::vector<glm::mat4> matrices = m_inverse_bind;
            if (m_fixed_joint != NONE) matrices.push_back(glm::mat4(1.0f));

            std::vector<float> floats(matrices.size() * 16);
            std::memcpy(floats.data(), matrices.data(), floats.size() * sizeof(float));
            binds = WriteFloats(floats, fastgltf::AccessorType::Mat4, matrices.size(), 0);
        }

        while (m_bin.size() % 4 != 0) m_bin.push_back(std::byte{0});

        std::size_t const fixed_node = 1 + m_kept_order.size();
        std::string j = "{\"asset\":{\"version\":\"2.0\",\"generator\":\"Iridium Aurum import\"}";

        // extensions -- the source's, less lights and quantization (nothing's quantized now)
        for (char const* key : {"extensionsUsed", "extensionsRequired"}) {
            std::string list{};
            std::string_view const source = Member(m_json, key);
            for (std::size_t i = 0;; i++) {
                std::string_view const item = Element(source, i);
                if (item.empty()) break;

                std::string const e = Unquote(item);
                if (e == "KHR_lights_punctual" || e == "KHR_mesh_quantization") continue;
                list += (list.empty() ? "" : ",") + Quote(e);
            }
            if (!list.empty()) j += ",\"" + std::string{key} + "\":[" + list + "]";
        }

        j += ",\"scene\":0,\"scenes\":[{\"name\":" + Quote(name) + ",\"nodes\":[0]}]";

        // nodes -- the root, the kept nodes, the fixed joint
        j += ",\"nodes\":[{\"name\":" + Quote(name) + ",\"mesh\":0";
        if (m_skinned) j += ",\"skin\":0";
        {
            std::vector<std::size_t> children{};
            for (std::size_t const n : m_kept_order)
                if (m_parent[n] == NONE) children.push_back(m_out_index[n]);
            if (m_fixed_joint != NONE) children.push_back(fixed_node);
            if (!children.empty()) j += ",\"children\":" + List(children);
        }
        j += "}";

        for (std::size_t const n : m_kept_order) {
            fastgltf::Node const& source = m_asset.nodes[n];
            j += ",{";
            j += "\"name\":" + Quote(std::string{source.name});

            if (auto const* trs = std::get_if<fastgltf::TRS>(&source.transform)) {
                j += ",\"translation\":" + Floats({trs->translation[0], trs->translation[1], trs->translation[2]});
                j += ",\"rotation\":" + Floats({trs->rotation[0], trs->rotation[1], trs->rotation[2], trs->rotation[3]});
                j += ",\"scale\":" + Floats({trs->scale[0], trs->scale[1], trs->scale[2]});
            }

            std::vector<std::size_t> children{};
            for (std::size_t const child : source.children)
                if (child < m_kept.size() && m_kept[child] && m_parent[child] == n) children.push_back(m_out_index[child]);
            if (!children.empty()) j += ",\"children\":" + List(children);

            j += "}";
        }

        if (m_fixed_joint != NONE) j += ",{\"name\":" + Quote(name + "_fixed") + "}";
        j += "]";

        // the mesh
        j += ",\"meshes\":[{\"name\":" + Quote(name) + ",\"primitives\":[";
        for (std::size_t p = 0; p < m_primitives.size(); p++) {
            Primitive const& primitive = m_primitives[p];

            j += std::string{p ? "," : ""} + "{\"attributes\":" + AttributeObject(primitive.attributes);
            if (primitive.indices != NONE) j += ",\"indices\":" + std::to_string(primitive.indices);
            if (primitive.material.has_value()) j += ",\"material\":" + std::to_string(primitive.material.value());
            j += ",\"mode\":" + std::to_string(static_cast<int>(primitive.mode));

            if (!primitive.all_targets.empty()) {
                j += ",\"targets\":[";
                for (std::size_t t = 0; t < primitive.all_targets.size(); t++) j += std::string{t ? "," : ""} + AttributeObject(primitive.all_targets[t]);
                j += "]";
            }

            j += "}";
        }
        j += "]";

        if (m_target_count > 0) {
            j += ",\"weights\":" + Floats(m_weights);

            std::string names{};
            for (std::string const& n : m_target_names) names += (names.empty() ? "" : ",") + Quote(n);
            j += ",\"extras\":{\"targetNames\":[" + names + "]}";
        }
        j += "}]";

        // the skin
        if (m_skinned) {
            std::vector<std::size_t> joints{};
            for (std::size_t k = 0; k < m_kept_order.size(); k++) joints.push_back(k + 1);
            if (m_fixed_joint != NONE) joints.push_back(fixed_node);

            j += ",\"skins\":[{\"name\":" + Quote(name) + ",\"inverseBindMatrices\":" + std::to_string(binds) + ",\"joints\":" + List(joints) + "}]";
        }

        // clips
        if (!m_animations.empty()) {
            j += ",\"animations\":[";
            for (std::size_t a = 0; a < m_animations.size(); a++) {
                Animation const& animation = m_animations[a];

                j += std::string{a ? "," : ""} + "{";
                if (!animation.name.empty()) j += "\"name\":" + Quote(animation.name) + ",";

                j += "\"samplers\":[";
                for (std::size_t s = 0; s < animation.samplers.size(); s++) {
                    Sampler const& sampler = animation.samplers[s];
                    j += std::string{s ? "," : ""} + "{\"input\":" + std::to_string(sampler.input) + ",\"output\":" + std::to_string(sampler.output) +
                         ",\"interpolation\":\"" + InterpolationName(sampler.interpolation) + "\"}";
                }

                j += "],\"channels\":[";
                for (std::size_t c = 0; c < animation.channels.size(); c++) {
                    Channel const& channel = animation.channels[c];
                    j += std::string{c ? "," : ""} + "{\"sampler\":" + std::to_string(channel.sampler) + ",\"target\":{\"node\":" + std::to_string(channel.node) +
                         ",\"path\":\"" + PathName(channel.path) + "\"}}";
                }

                j += "]}";
            }
            j += "]";
        }

        // materials, textures, samplers -- as the source wrote them
        for (char const* key : {"materials", "textures", "samplers"}) {
            std::string_view const source = Member(m_json, key);
            if (!source.empty()) j += ",\"" + std::string{key} + "\":" + std::string{source};
        }

        if (!m_images.empty()) {
            j += ",\"images\":[";
            for (std::size_t i = 0; i < m_images.size(); i++) {
                j += std::string{i ? "," : ""} + "{";
                if (!m_images[i].name.empty()) j += "\"name\":" + Quote(m_images[i].name) + ",";
                j += "\"bufferView\":" + std::to_string(m_images[i].view) + ",\"mimeType\":" + Quote(m_images[i].mime) + "}";
            }
            j += "]";
        }

        // accessors, views, the buffer
        j += ",\"accessors\":[";
        for (std::size_t a = 0; a < m_accessors.size(); a++) {
            Accessor const& accessor = m_accessors[a];
            j += std::string{a ? "," : ""} + "{\"bufferView\":" + std::to_string(accessor.view) + ",\"componentType\":" + std::to_string(accessor.component) +
                 ",\"count\":" + std::to_string(accessor.count) + ",\"type\":\"" + std::string{fastgltf::getAccessorTypeName(accessor.type)} + "\"";
            if (!accessor.min.empty()) j += ",\"min\":" + Floats(accessor.min) + ",\"max\":" + Floats(accessor.max);
            j += "}";
        }

        j += "],\"bufferViews\":[";
        for (std::size_t v = 0; v < m_views.size(); v++) {
            View const& view = m_views[v];
            j += std::string{v ? "," : ""} + "{\"buffer\":0,\"byteOffset\":" + std::to_string(view.offset) + ",\"byteLength\":" + std::to_string(view.length);
            if (view.target != 0) j += ",\"target\":" + std::to_string(view.target);
            j += "}";
        }

        j += "],\"buffers\":[{\"byteLength\":" + std::to_string(m_bin.size()) + "}]}";

        while (j.size() % 4 != 0) j += ' ';

        // header, JSON chunk, BIN chunk
        std::vector<std::byte> glb{};
        auto put = [&](std::uint32_t const value) {
            for (int b = 0; b < 4; b++) glb.push_back(static_cast<std::byte>((value >> (8 * b)) & 0xFFu));
        };

        put(0x46546C67u);  // 'glTF'
        put(2);
        put(static_cast<std::uint32_t>(12 + 8 + j.size() + 8 + m_bin.size()));

        put(static_cast<std::uint32_t>(j.size()));
        put(0x4E4F534Au);  // 'JSON'
        for (char const c : j) glb.push_back(static_cast<std::byte>(c));

        put(static_cast<std::uint32_t>(m_bin.size()));
        put(0x004E4942u);  // 'BIN\0'
        glb.insert(glb.end(), m_bin.begin(), m_bin.end());

        return glb;
    }

    static char const* InterpolationName(fastgltf::AnimationInterpolation const i) {
        switch (i) {
            case fastgltf::AnimationInterpolation::Step: return "STEP";
            case fastgltf::AnimationInterpolation::CubicSpline: return "CUBICSPLINE";
            default: return "LINEAR";
        }
    }

    static char const* PathName(fastgltf::AnimationPath const p) {
        switch (p) {
            case fastgltf::AnimationPath::Rotation: return "rotation";
            case fastgltf::AnimationPath::Scale: return "scale";
            case fastgltf::AnimationPath::Weights: return "weights";
            default: return "translation";
        }
    }

    // ---- JSON -------------------------------------------------------------------------------------------------------------------------------

    // floats as they'll read back exactly (9 significant digits round-trips a float)
    static std::string Floats(std::vector<float> const& values) {
        std::string out = "[";
        char text[32]{};
        for (std::size_t i = 0; i < values.size(); i++) {
            float const v = std::isfinite(values[i]) ? values[i] : 0.0f;
            std::snprintf(text, sizeof(text), "%.9g", static_cast<double>(v));
            out += std::string{i ? "," : ""} + text;
        }
        return out + "]";
    }

    static std::string List(std::vector<std::size_t> const& values) {
        std::string out = "[";
        for (std::size_t i = 0; i < values.size(); i++) out += std::string{i ? "," : ""} + std::to_string(values[i]);
        return out + "]";
    }

    static std::string AttributeObject(Attributes const& attributes) {
        std::string out = "{";
        for (std::size_t i = 0; i < attributes.size(); i++) out += std::string{i ? "," : ""} + Quote(attributes[i].first) + ":" + std::to_string(attributes[i].second);
        return out + "}";
    }

    static std::string Quote(std::string const& text) {
        std::string out = "\"";
        for (unsigned char const c : text) {
            if (c == '"' || c == '\\') {
                out += '\\';
                out += static_cast<char>(c);
            } else if (c < 0x20) {
                char escaped[8]{};
                std::snprintf(escaped, sizeof(escaped), "\\u%04x", c);
                out += escaped;
            } else {
                out += static_cast<char>(c);
            }
        }
        return out + "\"";
    }

    // a JSON string's text -- the common escapes undone (\uXXXX kept as written)
    static std::string Unquote(std::string_view const value) {
        std::string out{};
        if (value.size() < 2 || value.front() != '"') return out;

        for (std::size_t i = 1; i + 1 < value.size(); i++) {
            if (value[i] == '\\' && i + 2 < value.size()) {
                char const e = value[++i];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': out += "\\u"; break;
                    default: out += e; break;
                }
            } else {
                out += value[i];
            }
        }
        return out;
    }

    static std::size_t SkipSpace(std::string_view const s, std::size_t i) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) i++;
        return i;
    }

    // the end of the value starting at i (i at its first character)
    static std::size_t SkipValue(std::string_view const s, std::size_t i) {
        if (i >= s.size()) return i;

        if (s[i] == '"') {
            for (i++; i < s.size(); i++) {
                if (s[i] == '\\') {
                    i++;
                } else if (s[i] == '"') {
                    return i + 1;
                }
            }
            return i;
        }

        if (s[i] == '{' || s[i] == '[') {
            int depth{0};
            for (; i < s.size(); i++) {
                char const c = s[i];
                if (c == '"') {
                    i = SkipValue(s, i) - 1;
                } else if (c == '{' || c == '[') {
                    depth++;
                } else if (c == '}' || c == ']') {
                    if (--depth == 0) return i + 1;
                }
            }
            return i;
        }

        while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' && s[i] != ' ' && s[i] != '\n' && s[i] != '\r' && s[i] != '\t') i++;
        return i;
    }

    // an object's member by key -- empty when it isn't one, or has none
    static std::string_view Member(std::string_view const object, std::string_view const key) {
        std::size_t i = SkipSpace(object, 0);
        if (i >= object.size() || object[i] != '{') return {};

        for (i = SkipSpace(object, i + 1); i < object.size() && object[i] == '"';) {
            std::size_t const key_end = SkipValue(object, i);
            std::string_view const found = object.substr(i + 1, key_end - i - 2);

            i = SkipSpace(object, key_end);
            if (i >= object.size() || object[i] != ':') return {};
            i = SkipSpace(object, i + 1);

            std::size_t const value_end = SkipValue(object, i);
            if (found == key) return object.substr(i, value_end - i);

            i = SkipSpace(object, value_end);
            if (i < object.size() && object[i] == ',') i = SkipSpace(object, i + 1);
        }

        return {};
    }

    // an array's element by index -- empty past the end
    static std::string_view Element(std::string_view const array, std::size_t const index) {
        std::size_t i = SkipSpace(array, 0);
        if (i >= array.size() || array[i] != '[') return {};

        i = SkipSpace(array, i + 1);
        for (std::size_t n = 0; i < array.size() && array[i] != ']'; n++) {
            std::size_t const end = SkipValue(array, i);
            if (n == index) return array.substr(i, end - i);

            i = SkipSpace(array, end);
            if (i < array.size() && array[i] == ',') i = SkipSpace(array, i + 1);
        }

        return {};
    }

    // ---- writing ----------------------------------------------------------------------------------------------------------------------------

    std::size_t WriteView(void const* data, std::size_t const size, int const target) {
        while (m_bin.size() % 4 != 0) m_bin.push_back(std::byte{0});

        View view{};
        view.offset = m_bin.size();
        view.length = size;
        view.target = target;

        auto const* bytes = static_cast<std::byte const*>(data);
        m_bin.insert(m_bin.end(), bytes, bytes + size);

        m_views.push_back(view);
        return m_views.size() - 1;
    }

    std::size_t WriteAccessor(std::size_t const view, std::size_t const count, fastgltf::AccessorType const type, int const component) {
        Accessor accessor{};
        accessor.view = view;
        accessor.count = count;
        accessor.type = type;
        accessor.component = component;

        m_accessors.push_back(std::move(accessor));
        return m_accessors.size() - 1;
    }

    std::size_t WriteFloats(std::vector<float> const& floats, fastgltf::AccessorType const type, std::size_t const count, int const target) {
        std::size_t const view = WriteView(floats.data(), floats.size() * sizeof(float), target);
        return WriteAccessor(view, count, type, 5126);
    }

    // bounds = POSITION's min / max (the spec wants them)
    std::size_t WriteVec3(std::vector<glm::vec3> const& values, bool const bounds) {
        std::size_t const index = WriteFloats(Flatten(values), fastgltf::AccessorType::Vec3, values.size(), 34962);

        if (bounds && !values.empty()) {
            glm::vec3 lo{values[0]};
            glm::vec3 hi{values[0]};
            for (glm::vec3 const& v : values) {
                lo = glm::min(lo, v);
                hi = glm::max(hi, v);
            }

            m_accessors[index].min = {lo.x, lo.y, lo.z};
            m_accessors[index].max = {hi.x, hi.y, hi.z};
        }

        return index;
    }

    std::size_t WriteJoints(std::vector<std::uint16_t> const& joints) {
        std::size_t const view = WriteView(joints.data(), joints.size() * sizeof(std::uint16_t), 34962);
        return WriteAccessor(view, joints.size() / 4, fastgltf::AccessorType::Vec4, 5123);
    }

    std::size_t WriteIndices(std::vector<std::uint32_t> const& indices) {
        std::size_t const view = WriteView(indices.data(), indices.size() * sizeof(std::uint32_t), 34963);
        return WriteAccessor(view, indices.size(), fastgltf::AccessorType::Scalar, 5125);
    }

    // any scalar / vec accessor as floats (normalized integers become 0..1) -- NONE for matrices
    std::size_t CopyFloats(fastgltf::Accessor const& accessor, int const target) {
        std::vector<float> floats{};

        switch (accessor.type) {
            case fastgltf::AccessorType::Scalar:
                fastgltf::iterateAccessor<float>(m_asset, accessor, [&](float v) { floats.push_back(v); });
                break;
            case fastgltf::AccessorType::Vec2:
                fastgltf::iterateAccessor<glm::vec2>(m_asset, accessor, [&](glm::vec2 v) { floats.insert(floats.end(), {v.x, v.y}); });
                break;
            case fastgltf::AccessorType::Vec3:
                fastgltf::iterateAccessor<glm::vec3>(m_asset, accessor, [&](glm::vec3 v) { floats.insert(floats.end(), {v.x, v.y, v.z}); });
                break;
            case fastgltf::AccessorType::Vec4:
                fastgltf::iterateAccessor<glm::vec4>(m_asset, accessor, [&](glm::vec4 v) { floats.insert(floats.end(), {v.x, v.y, v.z, v.w}); });
                break;
            default:
                return NONE;
        }

        return WriteFloats(floats, accessor.type, accessor.count, target);
    }

    // ---- state ------------------------------------------------------------------------------------------------------------------------------

    fastgltf::Asset m_asset{};
    std::string m_json{};
    std::size_t m_scene{0};
    std::vector<std::string> m_report{};

    // the source's default scene
    std::vector<std::size_t> m_parent{};
    std::vector<glm::mat4> m_world{};
    std::vector<bool> m_in_scene{};
    std::vector<std::size_t> m_order{};
    std::vector<Instance> m_instances{};
    std::vector<std::size_t> m_instance_of{};  // node -> its instance

    // the skeleton
    std::set<std::size_t> m_skins_used{};
    std::vector<bool> m_kept{};
    std::vector<std::size_t> m_kept_order{};
    std::vector<std::size_t> m_out_index{};  // source node -> output node (NONE = baked away)
    std::vector<glm::mat4> m_inverse_bind{};
    std::map<std::size_t, glm::mat4> m_skin_fix{};
    std::size_t m_fixed_joint{NONE};
    bool m_skinned{false};

    // the output
    std::vector<Primitive> m_primitives{};
    std::size_t m_target_count{0};
    std::vector<float> m_weights{};
    std::vector<std::string> m_target_names{};
    std::vector<Animation> m_animations{};
    std::map<std::size_t, std::size_t> m_input_copies{};
    std::vector<Image> m_images{};

    std::vector<std::byte> m_bin{};
    std::vector<View> m_views{};
    std::vector<Accessor> m_accessors{};
};

}  // namespace NSIr77Scene
