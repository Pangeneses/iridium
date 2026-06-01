#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77MESH.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77MESH {
   public:
    IDDIr77MESH() = default;

   public:
    static winrt::guid HVIDDIr77MESH();
    static winrt::guid HVIDDIr77MESHOP();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77MESH(), HVIDDIr77MESHOP()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77MESH : IDDIr77MESHT<IDDIr77MESH, m_implementation::IDDIr77MESH> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
