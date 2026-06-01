namespace IIr77APP
{
    static runtimeclass IDDIr77APP
    {
        static Guid HVIDDIr77APP    { get; };
        static Guid HVIDDIr77STATE  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}