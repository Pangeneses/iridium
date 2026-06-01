namespace IIr77PROJECT
{
    static runtimeclass IDOIr77GPU
    {
        static Guid HVIDOIr77GPU          { get; };
        static Guid HVIDOIr77ATTPIPELINE  { get; };
        static Guid HVIDOIr77DETPIPELINE  { get; };
        static Guid HVIDOIr77ATTCONTEXT   { get; };
        static Guid HVIDOIr77DETCONTEXT   { get; };
        static Guid HVIDOIr77NEWCONTEXT   { get; };
        static Guid HVIDOIr77DELCONTEXT   { get; };
        static Guid HVIDOIr77SETCONTEXT   { get; };
        static Guid HVIDOIr77NEWNILOBJ    { get; };
        static Guid HVIDOIr77DELNILOBJ    { get; };
        static Guid HVIDOIr77NEWTEXTURE   { get; };
        static Guid HVIDOIr77DELTEXTURE   { get; };
        static Guid HVIDOIr77NEWSHADER    { get; };
        static Guid HVIDOIr77DELSHADER    { get; };
        static Guid HVIDOIr77NEWBUFFER    { get; };
        static Guid HVIDOIr77DELBUFFER    { get; };
        static Guid HVIDOIr77NEWSAMPLER   { get; };
        static Guid HVIDOIr77DELSAMPLER   { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}