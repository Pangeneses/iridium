#pragma once

#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 120
#endif
#include <CL/cl.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "Ir77Stages.hpp"

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77OpenCL -- one OpenCL device and the programs run on it. OpenCL 1.2 calls only, so any driver will do (AMD, NVIDIA, Intel, POCL). The
// device: the first GPU with doubles (cl_khr_fp64), else any device with doubles.
//   Open -> [Run ...] -> Close (the destructor too)
// =============================================================================================================================================
class Ir77OpenCL {
   public:
    Ir77OpenCL() = default;

    Ir77OpenCL(Ir77OpenCL const&) = delete;
    Ir77OpenCL& operator=(Ir77OpenCL const&) = delete;

    ~Ir77OpenCL() { Close(); }

    // a device found, its context and queue made -- false: error says why (no platform, no doubles)
    bool Open(std::string& error) {
        Close();

        cl_uint platforms_count = 0;
        if (clGetPlatformIDs(0, nullptr, &platforms_count) != CL_SUCCESS || platforms_count == 0) {
            error = "no OpenCL platform";
            return false;
        }
        std::vector<cl_platform_id> platforms(platforms_count);
        clGetPlatformIDs(platforms_count, platforms.data(), nullptr);

        // a GPU first, anything after -- with doubles either way
        for (cl_device_type const type : {static_cast<cl_device_type>(CL_DEVICE_TYPE_GPU), static_cast<cl_device_type>(CL_DEVICE_TYPE_ALL)}) {
            for (cl_platform_id const platform : platforms) {
                cl_uint count = 0;
                if (clGetDeviceIDs(platform, type, 0, nullptr, &count) != CL_SUCCESS || count == 0) continue;

                std::vector<cl_device_id> devices(count);
                clGetDeviceIDs(platform, type, count, devices.data(), nullptr);

                for (cl_device_id const device : devices) {
                    cl_device_fp_config doubles = 0;
                    clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(doubles), &doubles, nullptr);
                    if (doubles == 0) continue;

                    m_device = device;
                    break;
                }
                if (m_device) break;
            }
            if (m_device) break;
        }

        if (!m_device) {
            error = "no OpenCL device with double precision";
            return false;
        }

        m_name = DeviceText(CL_DEVICE_NAME);

        cl_int status = CL_SUCCESS;
        m_context = clCreateContext(nullptr, 1, &m_device, nullptr, nullptr, &status);
        if (status != CL_SUCCESS) return Fail("clCreateContext", status, error);

        m_queue = clCreateCommandQueue(m_context, m_device, 0, &status);
        if (status != CL_SUCCESS) return Fail("clCreateCommandQueue", status, error);

        return true;
    }

    void Close() {
        if (m_queue) clReleaseCommandQueue(m_queue);
        if (m_context) clReleaseContext(m_context);

        m_queue = nullptr;
        m_context = nullptr;
        m_device = nullptr;
    }

    bool IsOpen() const { return m_queue != nullptr; }

    // every device on every platform -- "platform: device (doubles)" / "(no doubles)"
    static std::vector<std::string> List() {
        std::vector<std::string> out{};

        cl_uint platforms_count = 0;
        if (clGetPlatformIDs(0, nullptr, &platforms_count) != CL_SUCCESS || platforms_count == 0) return out;
        std::vector<cl_platform_id> platforms(platforms_count);
        clGetPlatformIDs(platforms_count, platforms.data(), nullptr);

        for (cl_platform_id const platform : platforms) {
            std::size_t size = 0;
            clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, nullptr, &size);
            std::string platform_name(size, '\0');
            clGetPlatformInfo(platform, CL_PLATFORM_NAME, size, platform_name.data(), nullptr);
            while (!platform_name.empty() && platform_name.back() == '\0') platform_name.pop_back();

            cl_uint count = 0;
            if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &count) != CL_SUCCESS || count == 0) continue;
            std::vector<cl_device_id> devices(count);
            clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, count, devices.data(), nullptr);

            for (cl_device_id const device : devices) {
                clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size);
                std::string name(size, '\0');
                clGetDeviceInfo(device, CL_DEVICE_NAME, size, name.data(), nullptr);
                while (!name.empty() && name.back() == '\0') name.pop_back();

                cl_device_fp_config doubles = 0;
                clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(doubles), &doubles, nullptr);

                out.push_back(platform_name + ": " + name + (doubles ? " (doubles)" : " (no doubles)"));
            }
        }

        return out;
    }

    std::string const& Name() const { return m_name; }

    // a program built and run (services/Ir77Stages.hpp) -- false: error says why (the compiler's messages, a step's problem, cancelled)
    bool Run(Ir77ProgramJob const& job, std::atomic<bool> const& cancel, Ir77ProgramResult& result, std::string& error) {
        result = Ir77ProgramResult{};
        result.device = m_name;
        if (!IsOpen()) {
            error = "no OpenCL device";
            return false;
        }
        Ir77Stages stages{m_context, m_queue, m_device};
        return stages.Run(job, cancel, result, error);
    }

   private:
    std::string DeviceText(cl_device_info const what) const {
        std::size_t size = 0;
        clGetDeviceInfo(m_device, what, 0, nullptr, &size);
        std::string text(size, '\0');
        clGetDeviceInfo(m_device, what, size, text.data(), nullptr);
        while (!text.empty() && text.back() == '\0') text.pop_back();
        return text;
    }

    bool Fail(char const* call, cl_int const status, std::string& error) {
        error = std::string{call} + " failed: OpenCL " + std::to_string(status);
        Close();
        return false;
    }

    cl_device_id m_device{nullptr};
    cl_context m_context{nullptr};
    cl_command_queue m_queue{nullptr};
    std::string m_name{};
};

}  // namespace NSIr77Compute
