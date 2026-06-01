namespace IIr77APP
{
    static runtimeclass IDXIr77APP
    {
        static Guid HVIDXIr77APP      { get; };
        static Guid HVIDXIr77HEALTH   { get; };
        static Guid HVIDXIr77NAV      { get; };
        static Guid HVIDXIr77PAGE     { get; };
        static Guid HVIDXIr77PROJECT  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}