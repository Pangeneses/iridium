#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Device.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Compute {

// A Compute program (services/Ir77Stages.hpp) run on the device, on a worker thread -- one run at a time.
//   SetDevice -> Start -> [GetState ...] -> finished: TakeResult -> (Start again) ; Cancel any time
typedef struct IIr77Program : virtual public IIr77Enlisted {
    IIr77Program() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77Device> const& device) = 0;

    // a run starts on the worker -- fails while one is running
    virtual std::shared_ptr<IIr77Return const> Start(Ir77ProgramJob const& job) = 0;

    // asks the running program to stop before its next step
    virtual std::shared_ptr<IIr77Return const> Cancel() = 0;

    virtual std::shared_ptr<IIr77Return const> GetState(Ir77JobState& state) const = 0;

    // the finished run -- its job, result and what went wrong, moved out (a second call gets them empty). Fails while running or before the first.
    virtual std::shared_ptr<IIr77Return const> TakeResult(Ir77ProgramJob& job, Ir77ProgramResult& result, std::string& error) = 0;

    virtual ~IIr77Program() = default;
}* pIIr77Program;
}  // namespace NSIr77Compute
