namespace IIr77PROJECT
{
    static runtimeclass IDDIr77GPU
    {
        static Guid HVIDDIr77GPU      { get; };
        static Guid HVIDDIr77PIPELINE { get; };
        static Guid HVIDDIr77CONTEXT  { get; };
        static Guid HVIDDIr77NILOBJ   { get; };
        static Guid HVIDDIr77TEXTURE  { get; };
        static Guid HVIDDIr77SHADER   { get; };
        static Guid HVIDDIr77BUFFER   { get; };
        static Guid HVIDDIr77SAMPLER  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}