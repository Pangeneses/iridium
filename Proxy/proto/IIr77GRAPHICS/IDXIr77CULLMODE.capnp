namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77CULLMODE
    {
        static Guid HVIDX_D3D12_CULL_MODE       { get; };
        static Guid HVIDX_D3D12_CULL_MODE_NONE  { get; };
        static Guid HVIDX_D3D12_CULL_MODE_FRONT { get; };
        static Guid HVIDX_D3D12_CULL_MODE_BACK  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}