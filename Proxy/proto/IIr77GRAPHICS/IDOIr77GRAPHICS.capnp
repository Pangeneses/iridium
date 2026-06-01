namespace IIr77GRAPHICS
{
    static runtimeclass IDOIr77GRAPHICS
    {
        static Guid HVIDOIr77GRAPHICS   { get; };
        static Guid HVIDOIr77GETSTATE   { get; };
        static Guid HVIDOIr77SETSTATE   { get; };
        static Guid HVIDOIr77SEATGPU    { get; };
        static Guid HVIDOIr77RESETGPU   { get; };
        static Guid HVIDOIr77RUNGPU     { get; };
        static Guid HVIDOIr77STOPGPU    { get; };
        static Guid HVIDOIr77KILLGPU    { get; };
        static Guid HVIDOIr77BINDIA     { get; };
        static Guid HVIDOIr77UNBINDIA   { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}