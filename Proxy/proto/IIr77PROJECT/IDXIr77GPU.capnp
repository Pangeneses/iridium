namespace IIr77PROJECT
{
    static runtimeclass IDXIr77GPU
    {
        static Guid HVIDXIr77GPU      { get; };
        static Guid HVIDXIr77CONTEXT  { get; };
        static Guid HVIXIr77NILOBJ    { get; };
        static Guid HVIDXIr77TEXTURE  { get; };
        static Guid HVIDXIr77SHADER   { get; };
        static Guid HVIDXIr77BUFFER   { get; };
        static Guid HVIDXIr77SAMPLER  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}