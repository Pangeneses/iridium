namespace IIr77PROJECT
{
    static runtimeclass IDDIr77CAD
    {
        static Guid HVIDDIr77CAD       { get; };
        static Guid HVIDDIr77GEOMETRY  { get; };
        static Guid HVIDDIr77OPERATION { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}