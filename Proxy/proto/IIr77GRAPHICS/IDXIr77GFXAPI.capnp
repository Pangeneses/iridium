namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77GFXAPI
    {
        static Guid HVIDXIr77GFXAPI         { get; };
        static Guid HVIDXIr77DIRECTX09_C    { get; };
        static Guid HVIDXIr77DIRECTX10_1    { get; };
        static Guid HVIDXIr77DIRECTX11_0    { get; };
        static Guid HVIDXIr77DIRECTX11_1    { get; };
        static Guid HVIDXIr77DIRECTX11_2    { get; };
        static Guid HVIDXIr77DIRECTX11_X    { get; };
        static Guid HVIDXIr77DIRECTX11_3    { get; };
        static Guid HVIDXIr77DIRECTX12_U    { get; };
        static Guid HVIDXIr77OPENGL03_0     { get; };
        static Guid HVIDXIr77OPENGL03_1     { get; };
        static Guid HVIDXIr77OPENGL03_2     { get; };
        static Guid HVIDXIr77OPENGL03_3     { get; };
        static Guid HVIDXIr77OPENGL04_0     { get; };
        static Guid HVIDXIr77OPENGL04_1     { get; };
        static Guid HVIDXIr77OPENGL04_2     { get; };
        static Guid HVIDXIr77OPENGL04_3     { get; };
        static Guid HVIDXIr77OPENGL04_4     { get; };
        static Guid HVIDXIr77OPENGL04_5     { get; };
        static Guid HVIDXIr77OPENGL04_6     { get; };
        static Guid HVIDXIr77VULKAN01_3     { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}