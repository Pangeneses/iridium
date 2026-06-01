namespace IIr77PROJECT
{
    static runtimeclass IDIIr77PROJECT
    {
        static Guid HVIDIIr77PROJECT    { get; };
        static Guid HVIDIr77ACCOUNTS    { get; };
        static Guid HVIDIr77VERSIONING  { get; };
        static Guid HVIDIr77MESH        { get; };
        static Guid HVIDIr77CAD         { get; };
        static Guid HVIDIr77MATERIAL    { get; };
        static Guid HVIDIr77GENERATOR   { get; };
        static Guid HVIDIr77SCRIPT      { get; };
        static Guid HVIDIr77BOM         { get; };
        static Guid HVIDIr77GCODE       { get; };
        static Guid HVIDIr77PAPERS      { get; };
        static Guid HVIDIr77DATABASE    { get; };
        static Guid HVIDIr77GPU         { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}