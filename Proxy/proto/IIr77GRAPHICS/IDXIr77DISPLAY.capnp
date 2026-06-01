namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77DISPLAY
    {
        static Guid HVIDXIr77DISPLAY         { get; };
        static Guid HVIDXIr77DISPLAYSCALED   { get; };
        static Guid HVIDXIr77DISPLAY480P     { get; };
        static Guid HVIDXIr77DISPLAY720P     { get; };
        static Guid HVIDXIr77DISPLAY1024P    { get; };
        static Guid HVIDXIr77DISPLAY1080P    { get; };
        static Guid HVIDXIr77DISPLAY1080P3D  { get; };
        static Guid HVIDXIr77DISPLAY1080PF3D { get; };
        static Guid HVIDXIr77DISPLAY1200P    { get; };
        static Guid HVIDXIr77DISPLAY1440P    { get; };
        static Guid HVIDXIr77DISPLAY1600P    { get; };
        static Guid HVIDXIr77DISPLAY2160P    { get; };
        static Guid HVIDXIr77DISPLAY2160P3D  { get; };
        static Guid HVIDXIr77DISPLAY2160PF3D { get; };
        static Guid HVIDXIr77DISPLAY2540P    { get; };
        static Guid HVIDXIr77DISPLAY4000P    { get; };
        static Guid HVIDXIr77DISPLAY4320P    { get; };
        static Guid HVIDXIr77DISPLAYVR       { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}