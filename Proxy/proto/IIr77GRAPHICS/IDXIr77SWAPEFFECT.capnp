namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77SWAPEFFECT
    {
        static Guid DXGI_SWAP_EFFECT                 { get; };
        static Guid DXGI_SWAP_EFFECT_DISCARD         { get; };
        static Guid DXGI_SWAP_EFFECT_SEQUENTIAL      { get; };
        static Guid DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL { get; };
        static Guid DXGI_SWAP_EFFECT_FLIP_DISCARD    { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}