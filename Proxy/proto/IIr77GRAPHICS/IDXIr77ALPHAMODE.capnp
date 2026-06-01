namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77ALPHAMODE
    {
        static Guid Ir77X_DXGI_ALPHA_MODE                  { get; };
        static Guid Ir77X_DXGI_ALPHA_MODE_UNSPECIFIED      { get; };
        static Guid Ir77X_DXGI_ALPHA_MODE_PREMULTIPLIED    { get; };
        static Guid Ir77X_DXGI_ALPHA_MODE_STRAIGHT         { get; };
        static Guid Ir77X_DXGI_ALPHA_MODE_IGNORE           { get; };
        static Guid Ir77X_DXGI_ALPHA_MODE_FORCE_DWORD      { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}