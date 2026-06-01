namespace IIr77FILESYS
{
    static runtimeclass IDOIr77FOLDER
    {
        static Guid HVIDOIr77FOLDER  { get; };
        static Guid HVIDOIr77OPEN    { get; };
        static Guid HVIDOIr77CLOSE   { get; };
        static Guid HVIDOIr77ADD     { get; };
        static Guid HVIDOIr77RENAME  { get; };
        static Guid HVIDOIr77DELETE  { get; };
        static Guid HVIDOIr77NEWSEG  { get; };
        static Guid HVIDOIr77DELSEG  { get; };
        static Guid HVIDOIr77LOCKSEG { get; };
        static Guid HVIDOIr77PUTSEG  { get; };
        static Guid HVIDOIr77FREESEG { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}