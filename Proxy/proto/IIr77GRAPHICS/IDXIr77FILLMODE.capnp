namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77FILLMODE
    {
        static Guid HVIDX_D3D12_FILL_MODE           { get; };
        static Guid HVIDX_D3D12_FILL_MODE_WIREFRAME { get; };
        static Guid HVIDX_D3D12_FILL_MODE_SOLID     { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}