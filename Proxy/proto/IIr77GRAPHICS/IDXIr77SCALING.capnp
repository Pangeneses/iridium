namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77SCALING
    {
        static Guid HVIDX_DXGI_SCALING                      { get; };
        static Guid HVIDX_DXGI_SCALING_STRETCH              { get; };
        static Guid HVIDX_DXGI_SCALING_NONE                 { get; };
        static Guid HVIDX_DXGI_SCALING_ASPECT_RATIO_STRETCH { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}