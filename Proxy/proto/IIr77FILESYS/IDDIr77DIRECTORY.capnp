namespace IIr77FILESYS
{
    static runtimeclass IDDIr77DIRECTORY
    {
        static Guid HVIDDIr77DIRECTORY  { get; };
        static Guid HVIDDIr77LOAD       { get; };
        static Guid HVIDDIr77ROOT       { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}