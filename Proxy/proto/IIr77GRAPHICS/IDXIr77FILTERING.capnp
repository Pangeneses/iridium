namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77FILTERING
    {
        static Guid HVIDXIr77FILTERING          { get; };
        static Guid HVIDXIr77FILTERINGBI        { get; };
        static Guid HVIDXIr77FILTERINGTRI       { get; };
        static Guid HVIDXIr77FILTERINGANISO021  { get; };
        static Guid HVIDXIr77FILTERINGANISO041  { get; };
        static Guid HVIDXIr77FILTERINGANISO081  { get; };
        static Guid HVIDXIr77FILTERINGANISO161  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}