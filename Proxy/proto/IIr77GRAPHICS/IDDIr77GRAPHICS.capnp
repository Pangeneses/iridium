namespace IIr77GRAPHICS
{
    static runtimeclass IDDIr77GRAPHICS
    {
        static Guid HVIDDIr77GRAPHICS { get; };
        static Guid HVIDDIr77GFXSTATE { get; };
        static Guid HVIDDIr77GFXOP    { get; };
        static Guid HVIDDIr77GFXIA    { get; };
        static Guid HVIDDIr77GFXSLOT  { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}