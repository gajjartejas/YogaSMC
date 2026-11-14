static void batteryBuffers(){
 for(size_t length: {0,1,2,7,8,19,20}){
  auto data=new OSData(length);
  assert(extractBatteryCycle(data)==(length>=2?0x0101:0xffff));
  assert(extractBatteryID(data)==(length>=8?0x0101010101010101ULL:0xffff));
  data->release();
  FakeEC ec;IdeaVPC v;v.vpc=&ec;ec.response=new OSData(length);
  auto result=v.extractBatteryInfo(0);assert(bool(result)==(length>=20));if(result)result->release();
 }
 assert(extractBatteryCycle(nullptr)==0xffff);assert(extractBatteryID(nullptr)==0xffff);
 auto number=new OSNumber(12345);assert(extractBatteryCycle(number)==12345);number->release();
 FakeEC ec;IdeaVPC v;v.vpc=&ec;ec.response=new OSNumber(2);assert(!v.extractBatteryInfo(0));
 ec.objectResult=3;assert(!v.extractBatteryInfo(0));
}
static void decodedBatteryInfo(){
 FakeEC ec;IdeaVPC v;v.vpc=&ec;auto bat0=new OSDictionary;auto bat1=new OSDictionary;
 ec.response=new OSData(19);assert(!v.updateBatteryInfo(bat0,bat1));assert(bat0->texts.empty());
 auto data=new OSData(20);data->bytes.assign(20,0);data->bytes[0]=1;
 data->bytes[14]=0xe0;data->bytes[15]=0x0b;ec.response=data;
 assert(v.updateBatteryInfo(bat0,bat1));assert(bat0->texts["Temperature"]=="30.9℃");
 bat0->release();bat1->release();
}
static void charging(){
 for(int conservation=0;conservation<2;++conservation)for(int rapid=0;rapid<2;++rapid){
  FakeEC ec;ec.value=(conservation?BIT(5):0)|(rapid?BIT(2):0);IdeaVPC v;v.vpc=&ec;
  assert(v.updateBattery(false));assert(v.toggleRapidCharge());
  assert(v.rapidChargeMode==!rapid);assert(v.booleans["RapidChargeMode"]==!rapid);
  assert(v.conservationMode==bool(conservation));assert(v.toggleConservation());
  assert(v.conservationMode==!conservation);assert(v.notifications==1);
 }
 FakeEC ec;ec.value=0;IdeaVPC v;v.vpc=&ec;v.ignoreWrite=true;
 assert(!v.toggleRapidCharge());assert(!v.rapidChargeMode);assert(!v.booleans["RapidChargeMode"]);
 v.ignoreWrite=false;v.writeError=3;assert(!v.toggleConservation());assert(v.notifications==0);
 v.writeError=0;v.failReadAfterWrite=true;assert(!v.toggleRapidCharge());assert(!v.rapidChargeMode);
 v.rapidChargeCap=false;int writes=v.writes;assert(!v.toggleRapidCharge());assert(v.writes==writes);
}
static void requests(){
 FakeEC ec;ec.value=0;IdeaVPC v;v.vpc=&ec;
 assert(v.setPropertiesResultGated(nullptr)==kIOReturnBadArgument);
 auto dict=new OSDictionary;dict->add("RapidChargeMode",new OSString("true"));
 assert(v.setPropertiesResultGated(dict)==kIOReturnBadArgument);assert(v.writes==0);dict->release();
 dict=new OSDictionary;dict->add("RapidChargeMode",new OSBoolean(true));v.rapidChargeCap=false;
 assert(v.setPropertiesResultGated(dict)==kIOReturnUnsupported);assert(v.writes==0);
 v.rapidChargeCap=true;v.ignoreWrite=true;assert(v.setPropertiesResultGated(dict)==kIOReturnIOError);
 v.ignoreWrite=false;assert(v.setPropertiesResultGated(dict)==0);int writes=v.writes;
 assert(v.setPropertiesResultGated(dict)==0);assert(v.writes==writes);dict->release();
 dict=new OSDictionary;dict->add("ConservationMode",new OSBoolean(true));v.conservationModeLock=true;
 assert(v.setPropertiesResultGated(dict)==kIOReturnSuccess);
 assert(v.conservationMode && v.conservationModeLock); // Manual success must not unlock automatic SMC writes.
 writes=v.writes;assert(v.setPropertiesResultGated(dict)==kIOReturnSuccess);assert(v.writes==writes);
 dict->release();
 dict=new OSDictionary;dict->add("ConservationMode",new OSString("bad"));dict->add("Other",new OSBoolean(true));
 assert(v.setPropertiesResultGated(dict)==kIOReturnBadArgument);assert(v.legacyCalls==1);dict->release();
 dict=new OSDictionary;dict->add("RapidChargeMode",new OSBoolean(false));ec.integerResult=3;
 assert(v.setPropertiesResultGated(dict)==kIOReturnIOError);dict->release();
}
static void namesAndEC(){
 YogaSMCUserClient client;UInt8 output=99;char name[4]={'E','C','A','V'};
 assert(client.readECName(name,&output)==kIOReturnNotReady);
 FakeEC ec;YogaBaseService provider;client.fProvider=&provider;
 assert(client.readECName(name,&output)==kIOReturnNoDevice);provider.ec=&ec;
 assert(client.readECName(name,&output)==0);assert(output==42);assert(ec.lastName=="ECAV");
 char invalid[4]={'1','A','A','A'};assert(client.readECName(invalid,&output)==kIOReturnBadArgument);
 char path[4]={'A','.','B','C'};assert(client.readECName(path,&output)==kIOReturnBadArgument);
 assert(client.readECName(nullptr,&output)==kIOReturnBadArgument);
 provider.validateEC();assert(provider.strings["EC Capability"]=="Basic");
 ec.single=ec.bulk=true;provider.validateEC();assert(provider.ECAccessCap==ECReadCap);
 ec.write=true;provider.validateEC();assert(provider.strings["EC Capability"]=="RW");
 ec.single=false;provider.validateEC();assert(provider.ECAccessCap==0);
 ec.single=true;provider.validateEC();ec.integerResult=3;output=99;
 assert(provider.method_re1b(0,&output)==3);assert(output==99);
 assert(provider.method_re1b(256,&output)==kIOReturnBadArgument);
 OSData* data=reinterpret_cast<OSData*>(1);ec.objectResult=3;
 assert(provider.method_recb(0,2,&data)!=0);assert(!data);
 ec.objectResult=0;ec.response=new OSData(1);
 assert(provider.method_recb(0,2,&data)==kIOReturnNoBandwidth);assert(!data);
 assert(provider.method_recb(255,2,&data)==kIOReturnBadArgument);
 ec.response=new OSData(2);assert(provider.method_recb(0,2,&data)==0);data->release();
 provider.ec=nullptr;provider.validateEC();assert(provider.ECAccessCap==0);
 assert(provider.strings["EC Capability"]=="Unavailable");
}
static void userReadRanges(){
 FakeEC ec;ec.single=ec.bulk=true;YogaBaseService provider;provider.ec=&ec;provider.validateEC();
 YogaSMCUserClient client;client.fProvider=&provider;UInt8 output[2]={0};IOByteCount size=2;
 assert(client.readEC(UINT64_MAX,output,&size)==kIOReturnBadArgument);
 assert(client.readEC(255,output,&size)==kIOReturnBadArgument);
 size=0;assert(client.readEC(0,output,&size)==kIOReturnBadArgument);
 size=1;assert(client.readEC(255,output,&size)==0);assert(output[0]==42);
}
static void invalidPropertyMacros(){
 auto dict=new OSDictionary;dict->add("Flag",new OSString("wrong type"));
 int reached=0;for(int i=0;i<2;++i){OSBoolean* value;getPropertyBoolean("Flag");++reached;}
 assert(reached==0);dict->release();
}
int main(){batteryBuffers();decodedBatteryInfo();charging();requests();namesAndEC();userReadRanges();invalidPropertyMacros();
 puts("PASS: battery lengths/types, charging read-back/errors, request results, EC names/ranges/capabilities, invalid-property control flow");}
