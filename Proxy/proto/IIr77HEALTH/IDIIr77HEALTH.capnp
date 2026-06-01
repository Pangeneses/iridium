namespace IIr77HEALTH
{
    static runtimeclass IDIIr77HEALTH
    {
        static Guid HVIDIr77HEALTH  { get; };
        static Guid HVIDIr77MEDIC   { get; };
        static Guid HVIDIr77POKE    { get; };
        static Guid HVIDIr77ECHO    { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}