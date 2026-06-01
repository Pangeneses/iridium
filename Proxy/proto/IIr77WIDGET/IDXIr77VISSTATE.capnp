namespace IIr77WIDGET
{
    static runtimeclass IDXIr77VISSTATE
    {
        static Guid HVIDXVISSTATE    { get; };
        static Guid HVIDXTERMINATE   { get; };
        static Guid HVIDXVISIBLE     { get; };
        static Guid HVIDXREFRESH     { get; };
        static Guid HVIDXTHEME       { get; };

        static Windows.Foundation.Collections.IVector<Guid> HVIDLIST();
    }
}