#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using UInt8=uint8_t; using UInt16=uint16_t; using UInt32=uint32_t; using UInt64=uint64_t;
using IOReturn=int; using IOByteCount=size_t; using SInt16=int16_t;
constexpr int kIOReturnSuccess=0, kIOReturnBadArgument=1, kIOReturnUnsupported=2,
 kIOReturnIOError=3, kIOReturnNoMemory=4, kIOReturnNotPermitted=5,
 kIOReturnNotReady=6, kIOReturnNoDevice=7, kIOReturnInvalid=8, kIOReturnNoBandwidth=9;
#define BIT(n) (1U << (n))
#define ECReadCap BIT(0)
#define ECWriteCap BIT(1)
#define AlwaysLog(...) ((void)0)
#define DebugLog(...) ((void)0)
#define OSDynamicCast(type, value) dynamic_cast<type*>(value)
#define OSSafeReleaseNULL(value) do { if (value) (value)->release(); (value)=nullptr; } while(0)
struct OSObject {
 int refs=1; virtual ~OSObject()=default;
 void retain(){++refs;} void release(){if(!--refs) delete this;}
};
struct OSNumber:OSObject {
 UInt64 value; explicit OSNumber(UInt64 v):value(v){}
 static OSNumber* withNumber(UInt64 v,int){return new OSNumber(v);}
 UInt8 unsigned8BitValue(){return value;} UInt16 unsigned16BitValue(){return value;}
 UInt64 unsigned64BitValue(){return value;}
};
struct OSBoolean:OSObject { bool value; explicit OSBoolean(bool v):value(v){} bool getValue(){return value;} };
struct OSString:OSObject {
 std::string value; explicit OSString(const char* v):value(v){}
 bool isEqualTo(const char* v){return value==v;}
};
struct OSData:OSObject {
 std::vector<UInt8> bytes; explicit OSData(size_t n):bytes(n,1){}
 size_t getLength(){return bytes.size();} const void* getBytesNoCopy(){return bytes.data();}
};
struct OSDictionary:OSObject {
 std::map<std::string,OSObject*> values; std::map<std::string,std::string> texts;
 ~OSDictionary(){for(auto p:values)p.second->release();}
 static OSDictionary* withCapacity(int){return new OSDictionary;}
 OSObject* getObject(const char* key){auto it=values.find(key);return it==values.end()?nullptr:it->second;}
 OSObject* getObject(OSString* key){return getObject(key->value.c_str());}
 void setObject(OSString* key,OSObject* value){value->retain();values[key->value]=value;}
 void add(const char* key, OSObject* value){values[key]=value;}
};
struct OSCollectionIterator:OSObject {
 std::vector<OSString*> keys; size_t index=0;
 ~OSCollectionIterator(){for(auto key:keys)key->release();}
 static OSCollectionIterator* withCollection(OSDictionary* dict){
  auto i=new OSCollectionIterator;
  for(auto p:dict->values)i->keys.push_back(new OSString(p.first.c_str()));return i;
 }
 OSObject* getNextObject(){return index<keys.size()?keys[index++]:nullptr;}
};
struct FakeEC {
 bool single=false, bulk=false, write=false;
 int integerResult=0, objectResult=0; UInt32 value=42;
 OSObject* response=nullptr; std::string lastName;
 int validateObject(const char* name){return (std::string(name)=="RE1B"?single:std::string(name)=="RECB"?bulk:write)?0:1;}
 int evaluateInteger(const char* name,UInt32* output,OSObject** =nullptr,int=0){
  lastName=name;if(integerResult)return integerResult;*output=value;return 0;
 }
 int evaluateObject(const char*,OSObject** output,OSObject**,int){
  if(objectResult)return objectResult;*output=response;response=nullptr;return 0;
 }
};
struct YogaBaseService {
 FakeEC* ec=nullptr; UInt8 ECAccessCap=0;
 const char* readECOneByte="RE1B"; const char* readECBytes="RECB"; const char* writeECOneByte="WE1B";
 std::map<std::string,std::string> strings;std::map<std::string,bool> booleans;
 void setProperty(const char* key,const char* value){strings[key]=value;}
 void setProperty(const char* key,bool value){booleans[key]=value;}
 void setProperty(const char*,OSObject*){}
 int readECName(const char*,UInt32*);int method_re1b(UInt32,UInt8*);int method_recb(UInt32,UInt32,OSData**);
 void validateEC();bool isOpen(void*){return true;}
};
struct YogaSMCUserClient {YogaBaseService* fProvider=nullptr;int readECName(char*,UInt8*);int readEC(UInt64,UInt8*,IOByteCount*);};
constexpr int BM0_CONSERVATION_BIT=5,BM_RAPIDCHARGE_BIT=2;
constexpr int BMCMD0_CONSERVATION_ON=3,BMCMD0_CONSERVATION_OFF=5,
 BMCMD0_RAPIDCHARGE_ON=7,BMCMD0_RAPIDCHARGE_OFF=8;
struct IdeaVPC:YogaBaseService {
 FakeEC* vpc=nullptr;
 bool conservationMode=false,rapidChargeMode=false,rapidChargeCap=true,conservationModeLock=true;
 bool ignoreWrite=false,failReadAfterWrite=false;
 int writeError=0,writes=0,notifications=0,legacyCalls=0;
 const char* getBatteryInfo="GSBI";const char* getBatteryMode="GBMD";const char* setBatteryMode="SBMC";
 const char* conservationPrompt="ConservationMode";const char* rapidChargePrompt="RapidChargeMode";
 int evaluateIntegerParam(const char*,UInt32*,int command){
  ++writes;if(writeError)return writeError;
  if(!ignoreWrite){
   if(command==3)vpc->value|=BIT(5);if(command==5)vpc->value&=~BIT(5);
   if(command==7)vpc->value|=BIT(2);if(command==8)vpc->value&=~BIT(2);
  }
  if(failReadAfterWrite)vpc->integerResult=kIOReturnIOError;return 0;
 }
 void notifyBattery(){++notifications;}
 void setPropertiesGated(OSObject*){++legacyCalls;}
 bool updateBattery(bool=true);bool toggleConservation();bool toggleRapidCharge();
 bool updateBatteryInfo(OSDictionary*,OSDictionary*);
 OSData* extractBatteryInfo(UInt32);int setPropertiesResultGated(OSObject*);
};

#define setPropertyString(dict, key, text) ((dict)->texts[key]=(text))
