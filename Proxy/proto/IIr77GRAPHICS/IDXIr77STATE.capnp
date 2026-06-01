namespace IIr77GRAPHICS
{
    static runtimeclass IDXIr77STATE
    {
        static Guid HVIDXIr77STATE                   { get; };
        static Guid HVIDXIr77API                     { get; };
        static Guid HVIDXIr77SURFACE                 { get; }; 
        static Guid HVIDXIr77TIMEOUT                 { get; };
        static Guid HVIDXIr77REFRESH                 { get; }; 
        static Guid HVIDXIr77DISPLAY                 { get; }; 
        static Guid HVIDXIr77RGBA                    { get; }; 
        static Guid HVIDXIr77DELTA                   { get; }; 
        static Guid HVIDXIr77EVENT                   { get; };                                             
        static Guid HVIDIr77TOP                      { get; };
        static Guid HVIDIr77LEFT                     { get; };
        static Guid HVIDIr77RIGHT                    { get; };
        static Guid HVIDIr77BOTTOM                   { get; };
        static Guid HVIDXIr77TOPLEFTX                { get; };
        static Guid HVIDXIr77TOPLEFTY                { get; };
        static Guid HVIDXIr77WIDTH                   { get; };
        static Guid HVIDXIr77HEIGHT                  { get; };
        static Guid HVIDXIr77MINDEPTH                { get; };
        static Guid HVIDXIr77MAXDEPTH                { get; };
        static Guid HVIDXIr77FRAMES                  { get; };   
        static Guid HVIDXIr77STEREO                  { get; };
        static Guid HVIDXIr77COUNT                   { get; };
        static Guid HVIDXIr77QUALITY                 { get; };
        static Guid HVIDXIr77BUFFERUSAGE             { get; };
        static Guid HVIDXIr77BUFFERCOUNT             { get; };
        static Guid HVIDXIr77SCALING                 { get; };
        static Guid HVIDXIr77SWAPEFFECT              { get; };
        static Guid HVIDXIr77ALPHAMODE               { get; };
        static Guid HVIDXIr77FLAGS                   { get; };
        static Guid HVIDXIr77FILLMODE                { get; };
        static Guid HVIDXIr77CULLMODE                { get; };
        static Guid HVIDXIr77FRONTCOUNTERCLOCKWISE   { get; };
        static Guid HVIDXIr77DEPTHBIAS               { get; };
        static Guid HVIDXIr77DEPTHBIASCLAMP          { get; };
        static Guid HVIDXIr77SLOPESCALEDDEPTHBIAS    { get; };
        static Guid HVIDXIr77DEPTHCLIPENABLE         { get; };
        static Guid HVIDXIr77MULTISAMPLEENABLE       { get; };
        static Guid HVIDXIr77ANTIALIASEDLINEENABLE   { get; };
        static Guid HVIDXIr77FORCEDSAMPLECOUNT       { get; };
        static Guid HVIDXIr77CONSERVATIVERASTER      { get; };
        static Guid HVIDXIr77VSYNC                   { get; };  
        static Guid HVIDXIr77EXPOSURE                { get; };
        static Guid HVIDXIr77GAMMA                   { get; };
        static Guid HVIDXIr77CHROMA                  { get; };
        static Guid HVIDXIr77AA                      { get; };
        static Guid HVIDXIr77AAX                     { get; };
        static Guid HVIDXIr77FXAA                    { get; };
        static Guid HVIDXIr77FILTERING               { get; };
        static Guid HVIDXIr77HDR                     { get; };
        static Guid HVIDXIr77BLOOM                   { get; };
        static Guid HVIDXIr77OCCLUSION               { get; };
        static Guid HVIDXIr77CAUSTIC                 { get; };
        static Guid HVIDXIr77VOLUMETRIC              { get; };
        static Guid HVIDXIr77REFLECTION              { get; };
        static Guid HVIDXIr77RADIOSITY               { get; };
        static Guid HVIDXIr77RAYTRACING              { get; };
        static Guid HVIDXIr77LOD                     { get; };
        static Guid HVIDXIr77SL                      { get; };
        static Guid HVIDXIr77TESSELLATION            { get; };
        static Guid HVIDXIr77CODEC                   { get; };

        static Windows.Foundation.Collections.IVector<Guid> UUIDLIST();
    }
}