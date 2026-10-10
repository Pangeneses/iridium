#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Animation.hpp"

#include "../PeregrineV/types//Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Animation -- glTF animations as clips. Clip data is read-only and shared; Sample() writes one clip at one time into a pose
// (per-node translation / rotation / scale arrays). Tracks only ever write into the pose -- the scene's world pass does the rest.
//
//   clip    = name + samplers + channels ("tracks")
//   sampler = key times + key values + interpolation (STEP / LINEAR / CUBICSPLINE)
//   channel = target node + property (T / R / S) + which sampler feeds it
//
// Morph-target weights (path "weights") are loaded-but-skipped until morph targets exist.
// =============================================================================================================================================
class Ir77Animation : public Ir77Enlisted, public IIr77Animation, public std::enable_shared_from_this<Ir77Animation> {
   private:
    enum class Ir77AnimPath : std::uint8_t { Translation, Rotation, Scale, Weights };
    enum class Ir77AnimInterp : std::uint8_t { Step, Linear, CubicSpline };

    struct Ir77AnimSampler {
        std::vector<float> times{};
        std::vector<glm::vec4> values{};  // vec3 in xyz for T / S; quaternion as xyzw for R. CUBICSPLINE: 3 per key (in, value, out)
        Ir77AnimInterp interpolation{Ir77AnimInterp::Linear};
    };

    struct Ir77AnimChannel {
        std::uint32_t node{UINT32_MAX};
        std::uint32_t sampler{UINT32_MAX};
        Ir77AnimPath path{Ir77AnimPath::Translation};
    };

    struct Ir77AnimClip {
        std::string name{};
        float duration{0.0f};
        std::vector<Ir77AnimSampler> samplers{};
        std::vector<Ir77AnimChannel> channels{};
    };

   public:
    Ir77Animation() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Animation() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Animation>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Animation>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Animation)
            obj = std::shared_ptr<IIr77Animation>(shared_from_this(), static_cast<IIr77Animation*>(this));

        else if (iid == GUIDIr77Animation)
            obj = std::shared_ptr<Ir77Animation>(shared_from_this(), static_cast<Ir77Animation*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // One clip per glTF animation, same index as asset.animations
    std::shared_ptr<IIr77Return const> LoadAnimationsFromAsset(fastgltf::Asset const& asset) {
        m_clips.clear();
        m_clips.reserve(asset.animations.size());

        for (fastgltf::Animation const& animation : asset.animations) {
            Ir77AnimClip clip{};
            clip.name = std::string(animation.name);

            // samplers -- key times and values
            for (fastgltf::AnimationSampler const& sampler : animation.samplers) {
                Ir77AnimSampler out{};

                switch (sampler.interpolation) {
                    case fastgltf::AnimationInterpolation::Step:
                        out.interpolation = Ir77AnimInterp::Step;
                        break;
                    case fastgltf::AnimationInterpolation::CubicSpline:
                        out.interpolation = Ir77AnimInterp::CubicSpline;
                        break;
                    default:
                        out.interpolation = Ir77AnimInterp::Linear;
                        break;
                }

                fastgltf::Accessor const& input = asset.accessors.at(sampler.inputAccessor);
                fastgltf::Accessor const& output = asset.accessors.at(sampler.outputAccessor);

                out.times.reserve(input.count);
                fastgltf::iterateAccessor<float>(asset, input, [&](float t) { out.times.push_back(t); });

                // T / S are vec3, R is vec4 (xyzw); weights are scalars -- read by the accessor's own shape
                out.values.reserve(output.count);
                if (output.type == fastgltf::AccessorType::Vec4) {
                    fastgltf::iterateAccessor<fastgltf::math::fvec4>(
                        asset, output, [&](fastgltf::math::fvec4 v) { out.values.push_back(glm::vec4(v.x(), v.y(), v.z(), v.w())); });
                } else if (output.type == fastgltf::AccessorType::Vec3) {
                    fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                        asset, output, [&](fastgltf::math::fvec3 v) { out.values.push_back(glm::vec4(v.x(), v.y(), v.z(), 0.0f)); });
                } else {
                    fastgltf::iterateAccessor<float>(asset, output, [&](float v) { out.values.push_back(glm::vec4(v, 0.0f, 0.0f, 0.0f)); });
                }

                if (!out.times.empty()) clip.duration = std::max(clip.duration, out.times.back());

                clip.samplers.push_back(std::move(out));
            }

            // channels -- which node / property each sampler drives
            for (fastgltf::AnimationChannel const& channel : animation.channels) {
                if (!channel.nodeIndex.has_value()) continue;  // KHR_animation_pointer targets -- not handled yet

                Ir77AnimChannel out{};
                out.node = static_cast<std::uint32_t>(channel.nodeIndex.value());
                out.sampler = static_cast<std::uint32_t>(channel.samplerIndex);

                switch (channel.path) {
                    case fastgltf::AnimationPath::Translation:
                        out.path = Ir77AnimPath::Translation;
                        break;
                    case fastgltf::AnimationPath::Rotation:
                        out.path = Ir77AnimPath::Rotation;
                        break;
                    case fastgltf::AnimationPath::Scale:
                        out.path = Ir77AnimPath::Scale;
                        break;
                    default:
                        out.path = Ir77AnimPath::Weights;
                        break;
                }

                clip.channels.push_back(out);
            }

            m_clips.push_back(std::move(clip));
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetClipCount(std::uint32_t& count) {
        count = static_cast<std::uint32_t>(m_clips.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetClipIndex(std::string const& name, std::uint32_t& index) {
        for (std::size_t c = 0; c < m_clips.size(); c++) {
            if (m_clips[c].name == name) {
                index = static_cast<std::uint32_t>(c);
                return Ir77RETURN<Ir77OperationSucceeded>();
            }
        }

        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animation: no clip with that name.");
    }

    std::shared_ptr<IIr77Return const> GetClipDuration(std::uint32_t const& index, float& duration) {
        if (index >= m_clips.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animation: clip index out of range.");

        duration = m_clips[index].duration;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetClipName(std::uint32_t const& index, std::string& name) {
        if (index >= m_clips.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animation: clip index out of range.");

        name = m_clips[index].name;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Writes clip `index` at `time` into the pose. Only the slots the clip has tracks for are touched -- the caller starts from the rest pose.
    std::shared_ptr<IIr77Return const> Sample(std::uint32_t const& index, float const& time, std::vector<glm::vec3>& translations,
                                              std::vector<glm::quat>& rotations, std::vector<glm::vec3>& scales) {
        if (index >= m_clips.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Animation: clip index out of range.");

        Ir77AnimClip const& clip = m_clips[index];

        for (Ir77AnimChannel const& channel : clip.channels) {
            if (channel.sampler >= clip.samplers.size()) continue;

            Ir77AnimSampler const& sampler = clip.samplers[channel.sampler];

            switch (channel.path) {
                case Ir77AnimPath::Translation:
                    if (channel.node < translations.size()) translations[channel.node] = glm::vec3(Evaluate(sampler, time, false));
                    break;

                case Ir77AnimPath::Rotation:
                    if (channel.node < rotations.size()) {
                        glm::vec4 const q = Evaluate(sampler, time, true);
                        rotations[channel.node] = glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
                    }
                    break;

                case Ir77AnimPath::Scale:
                    if (channel.node < scales.size()) scales[channel.node] = glm::vec3(Evaluate(sampler, time, false));
                    break;

                case Ir77AnimPath::Weights:
                    break;  // morph targets -- later
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // Value of one sampler at time t. Before the first key -> first value, after the last -> last value.
    static glm::vec4 Evaluate(Ir77AnimSampler const& sampler, float const& t, bool const& rotation) {
        std::size_t const keys = sampler.times.size();
        if (keys == 0 || sampler.values.empty()) return rotation ? glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(0.0f);

        bool const cubic = sampler.interpolation == Ir77AnimInterp::CubicSpline;
        if (cubic && sampler.values.size() < keys * 3) return sampler.values.front();
        if (!cubic && sampler.values.size() < keys) return sampler.values.front();

        auto const value = [&](std::size_t k) { return cubic ? sampler.values[k * 3 + 1] : sampler.values[k]; };

        if (keys == 1 || t <= sampler.times.front()) return value(0);
        if (t >= sampler.times.back()) return value(keys - 1);

        // keys k0 / k1 either side of t
        std::size_t const k1 = static_cast<std::size_t>(std::upper_bound(sampler.times.begin(), sampler.times.end(), t) - sampler.times.begin());
        std::size_t const k0 = k1 - 1;

        float const t0 = sampler.times[k0];
        float const dt = sampler.times[k1] - t0;
        float const u = dt > 0.0f ? (t - t0) / dt : 0.0f;

        switch (sampler.interpolation) {
            case Ir77AnimInterp::Step:
                return value(k0);

            case Ir77AnimInterp::Linear: {
                if (!rotation) return glm::mix(value(k0), value(k1), u);

                glm::vec4 const a = value(k0);
                glm::vec4 const b = value(k1);
                glm::quat const q = glm::slerp(glm::quat(a.w, a.x, a.y, a.z), glm::quat(b.w, b.x, b.y, b.z), u);  // shortest path
                return glm::vec4(q.x, q.y, q.z, q.w);
            }

            case Ir77AnimInterp::CubicSpline: {
                // Hermite: value + out-tangent at k0, value + in-tangent at k1; tangents are scaled by the key spacing
                glm::vec4 const p0 = sampler.values[k0 * 3 + 1];
                glm::vec4 const m0 = sampler.values[k0 * 3 + 2] * dt;
                glm::vec4 const p1 = sampler.values[k1 * 3 + 1];
                glm::vec4 const m1 = sampler.values[k1 * 3 + 0] * dt;

                float const u2 = u * u;
                float const u3 = u2 * u;

                glm::vec4 const v = (2.0f * u3 - 3.0f * u2 + 1.0f) * p0 + (u3 - 2.0f * u2 + u) * m0 + (-2.0f * u3 + 3.0f * u2) * p1 + (u3 - u2) * m1;

                return rotation ? glm::normalize(v) : v;
            }
        }

        return value(k0);
    }

   private:
    std::vector<Ir77AnimClip> m_clips{};
};
}  // namespace NSIr77Scene