namespace IIr77FILESYS
{
    static runtimeclass IDOIr77HEADER
    {
        static Guid HVIDOIr77HEADER   { get; };
        static Guid HVIDOIr77UPDATE   { get; };
        static Guid HVIDOIr77READBUF  { get; };
        static Guid HVIDOIr77WRITEBUF { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}