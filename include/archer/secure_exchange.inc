// Included inside the owned context scope; bounded one-shot WPA2/data experiment.
                    bool secure_deauth_completed=false,secure_pending=false;
                    std::optional<archer::PhyTxSnapshot> secure_phy_before;unsigned secure_phy_phase=0;
                    const auto secure_capture_phy=[&]{
                        if(!secure_phy_before)return;
                        const auto before=*secure_phy_before,after=archer::read_phy_tx_snapshot(bus);
                        const auto delta=archer::phy_tx_modulo_delta(before,after);
                        secure_phy_before.reset();
                        std::cout<<"{\"operation\":\"secure_phy_delta\",\"phase\":"<<secure_phy_phase<<",\"before\":[";
                        for(unsigned i=0;i<4;++i){if(i)std::cout<<',';std::cout<<before.counts()[i];}
                        std::cout<<"],\"after\":[";for(unsigned i=0;i<4;++i){if(i)std::cout<<',';std::cout<<after.counts()[i];}
                        std::cout<<"],\"modulo_delta\":[";for(unsigned i=0;i<4;++i){if(i)std::cout<<',';std::cout<<delta[i];}
                        std::cout<<"],\"antenna_emission_proven\":false}"<<std::endl;
                    };
                    const auto secure_wait=[&](const auto& ready){
                        const auto end=std::min(secure_deadline+std::chrono::seconds(3),std::chrono::steady_clock::now()+std::chrono::seconds(3));
                        while(!ready()&&std::chrono::steady_clock::now()<end){receive(0,true);receive(1,true);}
                        if(!ready())throw std::runtime_error("Secure phase response timeout; no retry");
                    };
                    const auto secure_send=[&](archer::SecureFramePhase phase,const std::vector<std::uint8_t>& packet){
                        if(std::chrono::steady_clock::now()>secure_deadline+(phase==archer::SecureFramePhase::deauthenticate?std::chrono::seconds(3):std::chrono::seconds(0)))throw std::runtime_error("Secure capture deadline expired; no submission");
                        std::ofstream file(dir/("secure-tx-"+std::to_string(unsigned(phase))+".bin"),std::ios::binary);
                        file.write(reinterpret_cast<const char*>(packet.data()),packet.size());file.close();
                        if(!file)throw std::runtime_error("Secure wire packet preservation failed");
                        const bool data=unsigned(phase)>=2&&unsigned(phase)<=4;
                        const bool cleanup=phase==archer::SecureFramePhase::deauthenticate;
                        secure_phy_before=archer::read_phy_tx_snapshot(bus);secure_phy_phase=unsigned(phase);
                        if(!secure_phy_before->enabled())throw std::runtime_error("Secure PHY counters disabled");
                        probe_report.reset();probe_submitted=cleanup;secure_pending=true;
                        try{
                            archer::channel_bus_write(bus,0x10398,0x3ffff,data?1:0x100);
                            usb.send_secure_once(phase,packet);
                            // Keep this scheduler available while waiting for its AP response.
                            // The enclosing phase wait pauses it before any subsequent submission.
                        }catch(...){archer::channel_bus_write(bus,0x10398,0x3ffff,0);probe_submitted=false;throw;}
                    };
                    const auto secure_confirm=[&]{archer::channel_bus_write(bus,0x10398,0x3ffff,0);secure_capture_phy();secure_pending=false;};
                    try{
                        archer::with_scc_wlan_grant_window(bus,[&]{
                            archer::log_probe_grant_policy(bus,"enabled",std::cout);
                            archer::with_station_data_rx_window(bus,[&]{
                                std::exception_ptr secure_error;bool auth_attempted=false;
                                try{
                                    station_expected_reply=archer::StationManagementKind::authenticate;auth_attempted=true;
                                    secure_send(archer::SecureFramePhase::authenticate,archer::secure_management_packet(archer::SecureFramePhase::authenticate));
                                    secure_wait([&]{return bool(station_auth);});secure_confirm();
                                    if(station_auth->status)throw std::runtime_error("AP rejected authentication");
                                    station_expected_reply=archer::StationManagementKind::associate;
                                    secure_send(archer::SecureFramePhase::associate,archer::secure_management_packet(archer::SecureFramePhase::associate));
                                    secure_wait([&]{return bool(station_association);});secure_confirm();
                                    if(station_association->status)throw std::runtime_error("AP rejected association");
                                    secure_wait([&]{return !secure_m1.empty();});handshake.accept_m1(secure_m1.data(),secure_m1.size());
                                    std::cout<<"{\"operation\":\"secure_m1_accepted\"}"<<std::endl;
                                    const auto m2=handshake.issue_m2();m2_sent=true;
                                    secure_send(archer::SecureFramePhase::m2,archer::secure_eapol_packet(archer::SecureFramePhase::m2,m2));
                                    secure_wait([&]{return !secure_m3.empty();});handshake.accept_m3(secure_m3);secure_confirm();
                                    std::cout<<"{\"operation\":\"secure_m3_authenticated\",\"security_established\":false}"<<std::endl;
                                    const auto m4=handshake.issue_m4();ccmp.emplace(handshake);
                                    secure_send(archer::SecureFramePhase::m4,archer::secure_eapol_packet(archer::SecureFramePhase::m4,m4));
                                    // Different queues share BE here. Give M4 a bounded transmit window;
                                    // only a matched authenticated OFFER below confirms AP forwarding.
                                    const auto settle=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
                                    while(std::chrono::steady_clock::now()<settle){receive(0,true);receive(1,true);}
                                    archer::channel_bus_write(bus,0x10398,0x3ffff,0);
                                    secure_capture_phy();
                                    const std::array<std::uint8_t,6> broadcast{255,255,255,255,255,255};
                                    const auto discover=ccmp->encrypt_to_ap(archer::dhcp_discover_msdu(dhcp_xid),broadcast,archer::secure_frame_sequence(archer::SecureFramePhase::discover));
                                    secure_send(archer::SecureFramePhase::discover,archer::secure_tx_packet(archer::SecureFramePhase::discover,discover));
                                    secure_wait([&]{return bool(dhcp_offer);});secure_confirm();
                                    std::cout<<"{\"operation\":\"secure_dhcp_offer\",\"matched_transaction\":true,\"ccmp_authenticated\":true,\"security_established\":true,\"data_path_established\":true,\"lease_acquired\":false}"<<std::endl;
                                }catch(...){secure_error=std::current_exception();}
                                station_expected_reply.reset();
                                archer::channel_bus_write(bus,0x10398,0x3ffff,0);
                                secure_capture_phy();
                                if(auth_attempted&&!secure_pending){
                                    try{
                                        secure_send(archer::SecureFramePhase::deauthenticate,archer::secure_management_packet(archer::SecureFramePhase::deauthenticate));
                                        secure_wait([&]{return bool(probe_report);});secure_confirm();probe_submitted=false;
                                        if(probe_report->state!=0||probe_report->count!=1)throw std::runtime_error("Secure deauthentication not TX_DONE/count1");
                                        secure_deauth_completed=true;
                                    }catch(...){if(!secure_error)secure_error=std::current_exception();}
                                }
                                archer::channel_bus_write(bus,0x10398,0x3ffff,0);probe_submitted=false;
                                if(secure_error)std::rethrow_exception(secure_error);
                            });
                        });
                        archer::log_probe_grant_policy(bus,"restored",std::cout);
                    }catch(...){if(secure_deauth_completed)completed_tx_failure=std::current_exception();throw;}
