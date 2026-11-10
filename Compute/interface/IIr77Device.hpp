#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Compute {

// The OpenCL device Compute's programs run on -- the first GPU with double precision, else any device with it (POCL gives the CPU one).
//   Open -> [RunProgram ...] -> (destroyed: released)
typedef struct IIr77Device : virtual public IIr77Enlisted {
    IIr77Device() = default;

    // find a device -- fails with the reason (no platform, no doubles); calling again retries
    virtual std::shared_ptr<IIr77Return const> Open() = 0;

    // opened = a device is ready; name = which ('' when none); error = why not, from the last Open
    virtual std::shared_ptr<IIr77Return const> GetDevice(bool& opened, std::string& name, std::string& error) const = 0;

    // every OpenCL device on the machine, "platform: device (doubles / no doubles)"
    virtual std::shared_ptr<IIr77Return const> ListDevices(std::vector<std::string>& devices) const = 0;

    // a program (services/Ir77Stages.hpp) built and run to its end (its script, or cancel) -- fails with error
    virtual std::shared_ptr<IIr77Return const> RunProgram(Ir77ProgramJob const& job, std::atomic<bool> const& cancel, Ir77ProgramResult& result,
                                                          std::string& error) = 0;

    virtual ~IIr77Device() = default;
}* pIIr77Device;
}  // namespace NSIr77Compute
