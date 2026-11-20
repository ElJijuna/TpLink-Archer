// Included inside the owned context scope; bounded one-shot WPA2/data experiment.
                    bool secure_deauth_completed=false,secure_pending=false,secure_lease_acquired=false;
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
                    const auto secure_wait=[&](const auto& ready,unsigned budget_ms=3000,bool cleanup=false,bool allow_timeout=false){
                        const auto started=std::chrono::steady_clock::now();
                        const auto end=archer::secure_response_deadline(started,secure_deadline,budget_ms,cleanup);
                        while(!ready()&&std::chrono::steady_clock::now()<end){receive(0,true);receive(1,true);}
                        const bool received=ready();
                        const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
                        std::cout<<"{\"operation\":\"secure_response_wait\",\"budget_ms\":"<<budget_ms<<",\"elapsed_ms\":"<<elapsed<<",\"cleanup\":"<<(cleanup?"true":"false")<<",\"response_received\":"<<(received?"true":"false")<<"}"<<std::endl;
                        if(!received&&!allow_timeout)throw std::runtime_error("Secure phase response timeout; no retry");
                        return received;
                    };
                    const auto secure_send=[&](archer::SecureFramePhase phase,const std::vector<std::uint8_t>& packet){
                        if(std::chrono::steady_clock::now()>secure_deadline+(phase==archer::SecureFramePhase::deauthenticate?std::chrono::seconds(3):std::chrono::seconds(0)))throw std::runtime_error("Secure capture deadline expired; no submission");
                        std::ofstream file(dir/("secure-tx-"+std::to_string(unsigned(phase))+".bin"),std::ios::binary);
                        file.write(reinterpret_cast<const char*>(packet.data()),packet.size());file.close();
                        if(!file)throw std::runtime_error("Secure wire packet preservation failed");
                        const bool data=archer::secure_data_phase(phase);
                        secure_phy_before=archer::read_phy_tx_snapshot(bus);secure_phy_phase=unsigned(phase);
                        if(!secure_phy_before->enabled())throw std::runtime_error("Secure PHY counters disabled");
                        probe_report.reset();probe_submitted=archer::secure_report_requested(phase);station_tx_tag=archer::secure_report_tag(phase);secure_pending=true;
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
#ifdef ARCHER_EAPOL_RETRY
                                    secure_m1_profile=archer::decode_target_eapol_m1(secure_m1.data(),secure_m1.size());secure_last_m1_replay=secure_m1_profile->replay;
#endif
                                    const auto m2=handshake.issue_m2();m2_sent=true;
                                    secure_send(archer::SecureFramePhase::m2,archer::secure_eapol_packet(archer::SecureFramePhase::m2,m2));
#ifdef ARCHER_EAPOL_RETRY
                                    unsigned response_retries=0;
                                    while(secure_m3.empty()){
                                        secure_wait([&]{return !secure_m3.empty()||!secure_m1_retry.empty();});
                                        if(!secure_m3.empty())break;
                                        if(response_retries>=3)throw std::runtime_error("M1 response budget exhausted");
                                        auto retry_frame=std::move(secure_m1_retry);secure_m1_retry.clear();
                                        const auto response=handshake.issue_m2_for_m1_retry(retry_frame.data(),retry_frame.size());
                                        secure_last_m1_replay=archer::decode_target_eapol_m1(retry_frame.data(),retry_frame.size())->replay;
                                        archer::channel_bus_write(bus,0x10398,0x3ffff,0);secure_capture_phy();
                                        const auto phase=static_cast<archer::SecureFramePhase>(unsigned(archer::SecureFramePhase::m2_retry1)+response_retries++);
                                        std::cout<<"{\"operation\":\"secure_m2_response_retry\",\"ordinal\":"<<response_retries<<",\"same_anonce\":true,\"replay_advanced\":true}"<<std::endl;
                                        secure_send(phase,archer::secure_eapol_packet(phase,response));
                                    }
                                    handshake.accept_m3(secure_m3);secure_confirm();
#else
                                    secure_wait([&]{return !secure_m3.empty();});handshake.accept_m3(secure_m3);secure_confirm();
#endif
                                    std::cout<<"{\"operation\":\"secure_m3_authenticated\",\"security_established\":false}"<<std::endl;
                                    const auto m4=handshake.issue_m4();ccmp.emplace(handshake);
                                    secure_send(archer::SecureFramePhase::m4,archer::secure_eapol_packet(archer::SecureFramePhase::m4,m4));
                                    // Different queues share BE here. Give M4 a bounded transmit window;
                                    // only a matched authenticated OFFER below completes this experiment.
                                    const auto settle=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
                                    while(std::chrono::steady_clock::now()<settle){receive(0,true);receive(1,true);}
                                    archer::channel_bus_write(bus,0x10398,0x3ffff,0);
                                    secure_capture_phy();
                                    const std::array<std::uint8_t,6> broadcast{255,255,255,255,255,255};
                                    const auto discover=ccmp->encrypt_to_ap(archer::dhcp_discover_msdu(dhcp_xid,archer::secure_dhcp_broadcast),broadcast,archer::secure_frame_sequence(archer::SecureFramePhase::discover));
                                    secure_send(archer::SecureFramePhase::discover,archer::secure_tx_packet(archer::SecureFramePhase::discover,discover));
#ifdef ARCHER_DHCP_LEASE
                                    const auto acquisition_started=std::chrono::steady_clock::now();
                                    const unsigned first_wait=archer::dhcp_discover_backoff_ms(0,arc4random_uniform(2001));
#ifdef ARCHER_EAPOL_RETRY
                                    const bool initial_offer=secure_wait([&]{return bool(dhcp_offer)&&bool(probe_report);},first_wait,false,true);
                                    archer::require_secure_discover_report(probe_report);
                                    std::cout<<"{\"operation\":\"secure_discover_report\",\"tag\":12,\"tx_done\":true,\"server_delivery_confirmed\":false}"<<std::endl;
                                    probe_submitted=false;
#else
                                    const bool initial_offer=secure_wait([&]{return bool(dhcp_offer);},first_wait,false,true);
#endif
                                    if(!initial_offer){
                                        // Drain once more before consuming the single retry slot.
                                        receive(0,true);receive(1,true);
                                        if(!dhcp_offer){
                                            const auto seconds=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-acquisition_started).count();
                                            archer::channel_bus_write(bus,0x10398,0x3ffff,0);secure_capture_phy();
                                            const auto retry=ccmp->encrypt_to_ap(archer::dhcp_discover_msdu(dhcp_xid,true,unsigned(seconds)),broadcast,archer::secure_frame_sequence(archer::SecureFramePhase::discover_retry));
                                            std::cout<<"{\"operation\":\"secure_discover_retry\",\"ordinal\":1,\"same_xid\":true,\"fresh_pn_and_sequence\":true,\"secs\":"<<seconds<<"}"<<std::endl;
                                            secure_send(archer::SecureFramePhase::discover_retry,archer::secure_tx_packet(archer::SecureFramePhase::discover_retry,retry));
                                            secure_wait([&]{return bool(dhcp_offer);},archer::dhcp_discover_backoff_ms(1,arc4random_uniform(2001)));
                                        }
                                    }
                                    secure_confirm();
#else
                                    secure_wait([&]{return bool(dhcp_offer);},archer::secure_dhcp_wait_ms);secure_confirm();
#endif
                                    std::cout<<"{\"operation\":\"secure_dhcp_offer\",\"matched_transaction\":true,\"ccmp_authenticated\":true,\"security_established\":true,\"data_path_established\":true,\"lease_acquired\":false}"<<std::endl;

#ifdef ARCHER_DHCP_LEASE
                                    const auto request=ccmp->encrypt_to_ap(dhcp_selecting.issue_request(unsigned(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-acquisition_started).count())),broadcast,archer::secure_frame_sequence(archer::SecureFramePhase::request));
                                    secure_send(archer::SecureFramePhase::request,archer::secure_tx_packet(archer::SecureFramePhase::request,request));
                                    secure_wait([&]{return dhcp_ack;},10000);secure_confirm();
                                    secure_lease_acquired=true;
                                    std::cout<<"{\"operation\":\"secure_dhcp_ack\",\"ccmp_authenticated\":true,\"lease_acknowledged\":true,\"os_interface\":false}"<<std::endl;
                                    if(dhcp_server_mac[0]&1)throw std::runtime_error("DHCP server source MAC is not unicast");
                                    const auto release=ccmp->encrypt_to_ap(dhcp_selecting.issue_release(),dhcp_server_mac,archer::secure_frame_sequence(archer::SecureFramePhase::release));
                                    secure_send(archer::SecureFramePhase::release,archer::secure_tx_packet(archer::SecureFramePhase::release,release));
                                    secure_wait([&]{return bool(probe_report);});secure_confirm();probe_submitted=false;
                                    if(probe_report->state!=0||probe_report->count!=1)throw std::runtime_error("DHCP RELEASE not TX_DONE/count1");
                                    std::cout<<"{\"operation\":\"secure_dhcp_release\",\"tx_done\":true,\"server_release_confirmed\":false}"<<std::endl;
#endif
                                }catch(...){secure_error=std::current_exception();}
                                station_expected_reply.reset();
                                archer::channel_bus_write(bus,0x10398,0x3ffff,0);
                                secure_capture_phy();
                                std::cout<<"{\"operation\":\"secure_data_summary\",\"authenticated_frames\":"<<secure_data.authenticated<<",\"rejected_frames\":"<<secure_data.rejected<<",\"group_frames\":"<<secure_data.group<<",\"pairwise_frames\":"<<secure_data.pairwise<<",\"reflected_discover_frames\":"<<secure_data.reflected_discover<<",\"matched_offers\":"<<secure_data.offers<<",\"lease_acquired\":"<<(secure_lease_acquired?"true":"false")<<"}"<<std::endl;
                                if(auth_attempted&&!secure_pending){
                                    try{
                                        secure_send(archer::SecureFramePhase::deauthenticate,archer::secure_management_packet(archer::SecureFramePhase::deauthenticate));
                                        secure_wait([&]{return bool(probe_report);},3000,true);secure_confirm();probe_submitted=false;
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
