namespace IIr77PROJECT
{
    static runtimeclass IDOIr77MATERIAL
    {
        static Guid HVIDOIr77MATERIAL  { get; };
        static Guid HVIDOIr77CREATE    { get; };
        static Guid HVIDOIr77DESTROY   { get; };
        static Guid HVIDOIr77READ      { get; };
        static Guid HVIDOIr77WRITE     { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}