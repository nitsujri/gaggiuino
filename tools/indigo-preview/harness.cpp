// Runs the production renderer without an attached display. No hardware I/O.
#include "indigo/ui.h"
#include "../../src/eeprom_data/legacy/eeprom_data_v12.h"
#include "../../src/eeprom_data/default_profiles.h"
#include "../../test/fixtures/config_v12.h"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace Indigo;
eepromValues_t cfg{};SensorState sensor{};Telemetry data{};uint32_t tick=3000;
bool action(Command c,int i){if(c==Command::SelectProfile)cfg.activeProfile=i;return true;}
void drain(Ui& ui,const char* file=nullptr){ui.tick(tick,data);tick+=20;std::ofstream out;if(file)out.open(std::string("tmp/indigo-native-preview/")+file+".txt");while(const auto* s=ui.front()){if(file)out<<*s<<'\n';ui.pop();}}
void tap(Ui& ui,const std::string& name){const auto list=ui.targets();auto i=std::find_if(list.begin(),list.end(),[&](const Hit& h){return h.label==name;});if(i==list.end()){std::cerr<<"No target: "<<name<<'\n';std::exit(1);}ui.touch(i->x+i->w/2,i->y+i->h/2,true);ui.touch(i->x+i->w/2,i->y+i->h/2,false);}
int main(){
 cfg.tankFullDistanceMm=30;cfg.brightness=90;cfg.trayMinWeightG=100;
 eepromValues_t_v12 old;std::memcpy(&old,capturedConfigV12,sizeof(old));upgradeSchema_v12(cfg,old);
 for(unsigned i=0;i<2;i++){cfg.profiles[i]=defaultsProfile[i];cfg.profiles[i].shotRatioX100=defaultsProfile[i].shotPreset*100;}
 cfg.activeProfile=0;sensor.temperature=92.0;sensor.weight=0;sensor.smoothedPressure=.3;sensor.waterLvl=100;sensor.scalesPresent=true;data.waterValid=true;data.distance=30;data.startup=true;data.trayStatus=0;data.gross=170;
 Ui ui(cfg,sensor,action);ui.begin();drain(ui,"home");tap(ui,"Menu");drain(ui,"menu");tap(ui,"Profiles");drain(ui,"profiles");ui.touch(280,56,true);ui.touch(280,56,false);drain(ui,"profile");
 const char* steps[]={"Temp & dose","Preinfusion","Soak","Ramp","Transition","Brew profile"};const char* files[]={"dose","pi","soak","ramp","transition","brew-profile"};
 for(unsigned i=0;i<6;i++){tap(ui,steps[i]);drain(ui,files[i]);if(i==1||i==4||i==5){const auto hits=ui.targets();for(const auto& h:hits)if(h.label=="Pressure / flow"||h.label=="Flow / pressure"){ui.touch(h.x+5,h.y+5,true);ui.touch(h.x+5,h.y+5,false);break;}std::string f=std::string(files[i])+"-other-bank";drain(ui,f.c_str());}tap(ui,"All");drain(ui);}
 ui.returnHome();drain(ui);tap(ui,"Menu");drain(ui);tap(ui,"Water tank");drain(ui,"water");tap(ui,"< Back");drain(ui);
 tap(ui,"Tray guard");drain(ui,"tray");tap(ui,"< Back");drain(ui);
 tap(ui,"Machine settings");drain(ui,"system");
 const char* groups[]={"Boiler","Scales & pump","Screen & warmup","Brew preferences","Power & lighting"};
 for(unsigned i=0;i<5;i++){tap(ui,groups[i]);std::string f="settings-"+std::to_string(i);drain(ui,f.c_str());const auto hits=ui.targets();if(std::any_of(hits.begin(),hits.end(),[](const Hit& h){return h.label==">";})){tap(ui,">");f+="-page2";drain(ui,f.c_str());}tap(ui,"< Back");drain(ui);}
 tap(ui,"< Back");drain(ui);tap(ui,"Cleaning");drain(ui,"cleaning");tap(ui,"Backflush");drain(ui,"backflush");tap(ui,"< Back");drain(ui);tap(ui,"Descale");drain(ui,"descale");
 ui.returnHome();drain(ui);tap(ui,"Temperature");drain(ui);tap(ui,"Temperature C: 92.0");drain(ui,"keyboard-number");tap(ui,"Cancel");drain(ui);tap(ui,"All");drain(ui);tap(ui,"Rename");drain(ui,"keyboard-name");tap(ui,"Cancel");drain(ui);
 ui.message("Flush stopped: check tray / tank / display");drain(ui,"notice");
 ui.returnHome();drain(ui);tap(ui,"Menu");drain(ui);tap(ui,"Manual flow");drain(ui,"manual");
 const uint8_t value[]={0x71,50,0,0,0,255,255,255};for(auto b:value)ui.receive(b);data.brewing=true;data.elapsed=12.3;sensor.smoothedPressure=4.8;sensor.smoothedPumpFlow=5;sensor.weight=18.4;tick+=300;drain(ui,"manual-live");
 return 0;
}
