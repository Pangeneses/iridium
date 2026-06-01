namespace IIr77FILESYS
{
    static runtimeclass IDIIr77FILESYS
    {
        static Guid HVIDIIr77FILESYS   { get; };
        static Guid HVIDIIr77DIRECTORY { get; };
        static Guid HVIDIIr77FOLDER    { get; };
        static Guid HVIDIIr77HEADER    { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}