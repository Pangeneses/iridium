namespace IIr77PROJECT
{
    static runtimeclass IDOIr77CAD
    {
        static Guid HVIDOIr77CAD      { get; };
        static Guid HVIDOIr77ADDGEOM  { get; };
        static Guid HVIDOIr77DELGEOM  { get; };
        static Guid HVIDOIr77EDITGEOM { get; };
        static Guid HVIDOIr77GEOMOP   { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}