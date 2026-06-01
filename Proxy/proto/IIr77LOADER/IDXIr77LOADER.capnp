namespace IIr77LOADER
{
    static runtimeclass IDXIr77LOADER
    {
        static Guid HVIDXIr77LOADER  { get; };
        static Guid HVIDXIr77PATH    { get; };
        static Guid HVIDXIr77PROJECT { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}