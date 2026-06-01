namespace IIr77FILESYS
{
    static runtimeclass IDOIr77DIRECTORY
    {
        static Guid HVIDOIr77DIRECTORY  { get; };
        static Guid HVIDOIr77LOADIMG     { get; };
        static Guid HVIDOIr77PARKIMG   { get; };
        static Guid HVIDOIr77REFACTOR { get; };
        static Guid HVIDOIr77ROOT     { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}