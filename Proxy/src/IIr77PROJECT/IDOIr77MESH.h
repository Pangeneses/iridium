#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77MESH.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77MESH {
   public:
    IDOIr77MESH() = default;

   public:
    static winrt::guid HVIDOIr77MESH();
    static winrt::guid HVIDOIr77MESHOP();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77MESH(), HVIDOIr77MESHOP()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77MESH : IDOIr77MESHT<IDOIr77MESH, m_implementation::IDOIr77MESH> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
