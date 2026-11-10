#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace NSIr77Compute {

// =============================================================================================================================================
// Compute's types -- a program's run and what it made. Compute runs programs; it knows nothing of what they compute. Every number a program
// makes leaves as plain values, in the format the program names (its reader -- a Fields program -- knows what they mean; Compute doesn't).
// =============================================================================================================================================

// where programs write and read -- <project>/data/<name>.json (Fields' field.data.load reads the same folder)
inline constexpr char const* COMPUTE_DATA_FOLDER{"data"};

inline constexpr std::uint64_t COMPUTE_STAGE_CHUNK{1u << 18};             // a kernel's work items per launch -- every launch short (a display
                                                                          // driver kills kernels that run for seconds)
inline constexpr std::uint32_t COMPUTE_EXTRAPOLATE_SIZE{8};               // extrapolate -- the history it keeps
inline constexpr double COMPUTE_ORTHOGONAL_DROP{1e-7};                    // orthogonalize -- S's eigenvalues under this are dropped (unless the step says)
inline constexpr std::uint64_t COMPUTE_KEEP_LIMIT{16u * 1024u * 1024u};   // a buffer's values kept for the inspector up to this many
inline constexpr std::uint32_t COMPUTE_VIEW_SIDE{256};                    // compute.buffer -- a matrix or slice sampled down to this many a side
inline constexpr std::uint32_t COMPUTE_VIEW_LENGTH{4096};                 // ... a vector to this many
inline constexpr std::uint64_t COMPUTE_WRITE_LIMIT{64u * 1024u * 1024u};  // a write's values
inline constexpr std::uint32_t COMPUTE_INCLUDE_DEPTH{8};                  // @include inside @include, at most this deep

// one run -- the program, and what its @include lines may name
struct Ir77ProgramJob {
    std::string source{};
    std::string program{};                         // its name, as the panel had it -- reported back
    std::map<std::string, std::string> library{};  // the project's saved programs (name -> source) -- @include takes from these
    std::string data{};                            // <project>/data -- read takes from here
};

// one entry of a data file's "summary" -- a symbol's number, a buffer's values, or a string constant's text
struct Ir77ProgramSummary {
    enum class Kind : std::uint8_t { Number, List, Text };
    std::string name{};
    Kind kind{Kind::Number};
    std::vector<double> values{};  // Number: one
    std::string text{};
};

// one entry of a data file's "sliders" -- a reader's parameter, its values named
struct Ir77ProgramSlider {
    std::uint32_t index{0};
    std::vector<std::string> labels{};
};

// a data file a program made -- <project>/data/<name>.json: {format, version, summary, sliders, values}
struct Ir77ProgramOutput {
    std::string name{};
    std::string format{};
    std::uint32_t version{0};
    std::vector<Ir77ProgramSummary> summary{};
    std::vector<Ir77ProgramSlider> sliders{};
    std::vector<float> values{};
};

// a buffer as the run left it -- the inspector's
struct Ir77ProgramBuffer {
    std::string name{};
    bool ints{false};
    std::vector<std::size_t> dims{};  // d0 fastest
    std::size_t size{0};
    double minimum{0.0};
    double maximum{0.0};
    std::vector<double> values{};  // empty above COMPUTE_KEEP_LIMIT
};

// what it made
struct Ir77ProgramResult {
    double seconds{0.0};
    std::string device{};
    std::vector<std::pair<std::string, double>> report{};  // its @report symbols, in order
    std::vector<Ir77ProgramOutput> outputs{};
    std::vector<Ir77ProgramBuffer> buffers{};
};

// the job queue's state
enum class Ir77JobState : std::uint8_t { Idle, Running, Done, Failed, Cancelled };

}  // namespace NSIr77Compute
