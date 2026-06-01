namespace IIr77FILESYS
{
    static runtimeclass IDDIr77FOLDER
    {
        static Guid HVIDDIr77FOLDER  { get; };
        static Guid HVIDDIr77SEGMENT { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}