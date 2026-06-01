@0x887096a470a370b7;

using Cxx      = import "/capnp/c++.capnp";

$Cxx.namespace("Ir77");

struct GUID {
  high @0 :UInt64;
  low  @1 :UInt64;
}

struct HVID {
  name @0 :Text;
  guid @1 :GUID;
}

interface Ir77Object   { get @0 () -> (field :HVID);    set @1 (field :HVID)    -> (); }
interface Ir77GUID     { get @0 () -> (field :GUID);    set @1 (field :GUID)    -> (); }
interface Ir77HVID     { get @0 () -> (field :HVID);    set @1 (field :HVID)    -> (); }
interface Ir77Boolean  { get @0 () -> (field :Bool);    set @1 (field :Bool)    -> (); }
interface Ir77Char     { get @0 () -> (field :UInt32);  set @1 (field :UInt32)  -> (); }
interface Ir77WChar    { get @0 () -> (field :UInt32);  set @1 (field :UInt32)  -> (); }
interface Ir77String   { get @0 () -> (field :Text);    set @1 (field :Text)    -> (); }
interface Ir77UINT8    { get @0 () -> (field :UInt8);   set @1 (field :UInt8)   -> (); }
interface Ir77UINT16   { get @0 () -> (field :UInt16);  set @1 (field :UInt16)  -> (); }
interface Ir77UINT32   { get @0 () -> (field :UInt32);  set @1 (field :UInt32)  -> (); }
interface Ir77UINT64   { get @0 () -> (field :UInt64);  set @1 (field :UInt64)  -> (); }
interface Ir77INT16    { get @0 () -> (field :Int16);   set @1 (field :Int16)   -> (); }
interface Ir77INT32    { get @0 () -> (field :Int32);   set @1 (field :Int32)   -> (); }
interface Ir77INT64    { get @0 () -> (field :Int64);   set @1 (field :Int64)   -> (); }
interface Ir77SINGLE   { get @0 () -> (field :Float32); set @1 (field :Float32) -> (); }
interface Ir77DOUBLE   { get @0 () -> (field :Float64); set @1 (field :Float64) -> (); }

interface Ir77RUNTIME {
  get @0 () -> (field :Text);
  set @1 (field :Text) -> ();
}

interface Ir77TAG {
  getGuid @0 () -> (field :GUID);
  getData @1 () -> (field :Data);
  setGuid @2 (field :GUID) -> ();
  setData @3 (field :Data) -> ();
}

interface Ir77COLLECT {
  get @0 () -> (collection :List(Ir77TAG));
  set @1 (collection :List(Ir77TAG)) -> ();
}

interface Ir77ITEM {
  getTag  @0 () -> (tag  :Float64);
  getItem @1 () -> (item :Float64);
  setTag  @2 (tag  :Float64) -> ();
  setItem @3 (item :Float64) -> ();
}

interface Ir77MPVM {
  get @0 () -> (field :AnyPointer);
  set @1 (field :AnyPointer) -> ();
}

interface Ir77RETVAR {
  get @0 () -> (field :Float64);
  set @1 (field :Float64) -> ();
}