namespace IIr77LOADER
{
    static runtimeclass IDOIr77LOADER
    {
        static Guid HVIDOIr77LOADER   { get; };
        static Guid HVIDOIr77LOAD     { get; };
        static Guid HVIDOIr77UNLOAD   { get; };
        static Guid HVIDOIr77SETSTATE { get; };
        static Guid HVIDOIr77GETSTATE { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}