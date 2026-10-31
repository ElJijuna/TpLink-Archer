#include "archer/macos_register_bus.hpp"
#include "archer/context_reservation.hpp"
#include "archer/ifs_clm.hpp"
#include "archer/xtal_identity.hpp"
int main(int argc,char** argv){
 if(argc!=2||std::string(argv[1])!="--passive-channel9-baseline"){std::cerr<<"Usage: ifs-clm --passive-channel9-baseline\n";return 2;}
 try{
  archer::SequenceLedger ledger("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
  archer::with_wlan_io_window(bus,[&]{
   if(archer::read_chip_identity(bus).analog_id!=0x6933||
      (archer::channel_bus_read(bus,0x1e0)&0xc00003f)!=0xc000022||
      (archer::channel_bus_read(bus,0x84)&0x300007)!=7||
      (archer::channel_bus_read(bus,0x842c)&0xffff)!=0x7ff9||
      (archer::channel_bus_read(bus,0x26b4c)&0x1fff)!=2452)throw std::runtime_error("Channel9 firmware profile absent");
   archer::BeRfTransport rf(bus);
   for(unsigned p=0;p<2;++p)if((rf.read_full(p,0x10018)&0x31fff)!=9)throw std::runtime_error("RF9 absent");
   ContextReservation reservation;
   archer::with_ifs_clm_window(bus,[&]{
    for(unsigned window=0;window<2;++window){
     const auto start=std::chrono::steady_clock::now();
     archer::trigger_ifs_clm(bus);
     if(archer::channel_bus_read(bus,0x20eec)&0x10000)throw std::runtime_error("Stale DONE after trigger");
     bool done=false;
     while(std::chrono::steady_clock::now()-start<std::chrono::milliseconds(2300)){
      if(archer::channel_bus_read(bus,0x20eec)&0x10000){done=true;break;}
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
     }
     if(!done)throw std::runtime_error("IFS monitor deadline without DONE");
     const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
     if(elapsed<1900)throw std::runtime_error("IFS DONE too early for configured2000ms");
     const auto r=archer::read_ifs_clm(bus);
     std::cout<<"{\"operation\":\"ifs_clm_window\",\"index\":"<<window<<",\"period\":62500,\"unit_us\":32,\"elapsed_ms\":"<<elapsed
      <<",\"total_ifs\":"<<r.total<<",\"tx_units\":"<<r.tx<<",\"edcca_units\":"<<r.edcca<<",\"cck_cca_units\":"<<r.cck_cca<<",\"ofdm_cca_units\":"<<r.ofdm_cca
      <<",\"cck_fa_units\":"<<r.cck_fa<<",\"ofdm_fa_units\":"<<r.ofdm_fa<<",\"host_network_submissions\":0}"<<std::endl;
    }
   });
   if(archer::channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("Scheduler changed");
   reservation.clean();
   std::cout<<"{\"result\":\"ifs_clm_passive_baseline_completed\",\"controls_restored\":true,\"network_tx_paused\":true,\"h2c_submissions\":0}"<<std::endl;
  });return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"; no retry or TX\n";return 2;}
}
