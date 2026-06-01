namespace IIr77WIDGET
{
    static runtimeclass IDOIr77WIDGET
    {
        static Guid HVIDOIr77WIDGET         { get; };
        static Guid HVIDOIr77SETVISSTATE    { get; };
        static Guid HVIDOIr77GETVISSTATE    { get; };
        static Guid HVIDOIr77SETSTATE       { get; };
        static Guid HVIDOIr77GETSTATE       { get; };
        static Guid HVIDOIr77INHOOK         { get; };
        static Guid HVIDOIr77OUTHOOK        { get; };

        static Windows.Foundation.Collections.IVector<Guid> HVIDLIST();
    }
}