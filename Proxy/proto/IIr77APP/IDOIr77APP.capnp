namespace IIr77APP
{
    static runtimeclass IDOIr77APP
    {
        static Guid HVIDOIr77APP        { get; };
        static Guid HVIDOIr77LOAD       { get; };
        static Guid HVIDOIr77UNLOAD     { get; };
        static Guid HVIDOIr77PARK       { get; };
        static Guid HVIDOIr77SETSTATE   { get; };
        static Guid HVIDOIr77GETSTATE   { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}