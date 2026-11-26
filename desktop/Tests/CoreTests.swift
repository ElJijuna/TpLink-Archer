import Foundation
import CryptoKit

@main struct CoreTests {
    static var checks=0
    static func check(_ condition:@autoclosure()->Bool,_ title:String){checks += 1;if !condition(){fputs("FAIL: \(title)\n",stderr);exit(1)}}
    static func rejects(_ title:String,_ action:()->Void) { action() }
    static func u32(_ n:UInt32)->[UInt8] { [UInt8(n&255),UInt8((n>>8)&255),UInt8((n>>16)&255),UInt8(n>>24)] }
    static func pcap(_ packets:[[UInt8]],nano:Bool=false)->Data {
        var bytes:[UInt8]=nano ? [0x4d,0x3c,0xb2,0xa1]:[0xd4,0xc3,0xb2,0xa1]
        bytes += [2,0,4,0]+[UInt8](repeating:0,count:8)+u32(2048)+u32(127)
        for (index,p) in packets.enumerated(){bytes += u32(100)+u32(index==0 ? 0 : nano ? 500_000_000:500_000)+u32(UInt32(p.count))+u32(UInt32(p.count))+p}
        return Data(bytes)
    }
    static func beacon(corrupt:Bool=false,extended:Bool=false)->[UInt8]{
        var frame=[UInt8](repeating:0,count:36);frame[0]=0x80
        for i in 4..<10{frame[i]=255};let ap:[UInt8]=[0x50,0x92,0x6a,0x11,0x06,0x66]
        frame.replaceSubrange(10..<16,with:ap);frame.replaceSubrange(16..<22,with:ap);frame[34]=0x11
        let name=[UInt8]("Test Net".utf8);frame += [0,UInt8(name.count)]+name+[3,1,9,48,2,1,0]
        frame += u32(PcapReader.crc(frame[...]))
        if corrupt{frame[36]^=1}
        let radio:[UInt8]=extended ? [0,0,19,0,0x2a,0,0,0x80,0,0,0,0,0x10,0,0x94,9,0xc0,0,0xd6]:[0,0,15,0,0x2a,0,0,0,0x10,0,0x94,9,0xc0,0,0xd6]
        return radio+frame
    }
    static func main() throws {
        check(PcapReader.crc(Array("123456789".utf8)[...])==0xcbf43926,"independent CRC32 reference")
        let parsed=try PcapReader.parse(pcap([beacon(),beacon(corrupt:true)]))
        check(parsed.packets.count==2,"two records");check(parsed.packets[0].signal == -42,"radiotap RSSI");check(parsed.packets[0].channel==9,"radiotap/channel IE");check(parsed.packets[0].crcValid==true,"valid FCS");check(parsed.packets[1].crcValid==false,"bad FCS")
        check(parsed.networks.count==1 && parsed.networks[0].ssid=="Test Net" && parsed.networks[0].beacons==1,"invalid CRC does not create a network observation")
        check(parsed.packets[1].relative==0.5,"microseconds")
        let ns=try PcapReader.parse(pcap([beacon(extended:true),beacon()],nano:true));check(ns.packets.first?.signal == -42,"extended presence alignment");check(ns.packets[1].relative==0.5,"nanosecond timestamps")
        let ack:[UInt8]=[0,0,8,0,0,0,0,0,0xd4,0,0,0,2,0,0,0x89,0x22,0xa1]
        let ackCapture=try PcapReader.parse(pcap([ack]));check(ackCapture.packets.first?.protocolName=="ACK","short ACK accepted")
        var protected=[UInt8](repeating:0,count:64);protected[0]=8;protected[1]=0x41
        let encrypted=try PcapReader.parse(pcap([[0,0,8,0,0,0,0,0]+protected]));check(encrypted.packets[0].encrypted && encrypted.packets[0].protocolName=="Datos cifrados","protected payload never advertised as decrypted")
        do{_ = try PcapReader.parse(Data([1,2,3]));check(false,"short header rejected")}catch{check(true,"short header rejected")}
        var truncated=pcap([beacon()]);truncated.removeLast();do{_ = try PcapReader.parse(truncated);check(false,"truncation rejected")}catch{check(true,"truncation rejected")}
        let partial=try PcapReader.parse(truncated,allowPartial:true);check(partial.packets.isEmpty,"partial tail is not decoded")
        let bad=[UInt8](repeating:0,count:5);let limited=try PcapReader.parse(pcap(Array(repeating:bad,count:20)),allowPartial:true,limit:3);check(limited.warnings.count<=4,"malformed records are bounded by record limit")
        let native=NativeSummary.parse(Data("{\"operation\":\"secure_unicast_arp_reply\",\"pairwise\":true,\"ccmp_authenticated\":true,\"identity_matched\":true,\"local_replay_rejected\":true}\n{\"operation\":\"secure_data_summary\",\"pairwise_frames\":1,\"authenticated_frames\":9}\n{\"result\":\"context_cleanup_acknowledged\"}\n".utf8))
        check(native.arp && native.cleanup && native.pairwise==1,"native ARP proof and cleanup")
        check(CSVExport.field("=1+1")=="\"'=1+1\"","CSV formula guard");check(CSVExport.field("a\"b")=="\"a\"\"b\"","CSV quoting")
        check(shellQuote("a'b")=="'a'\"'\"'b'","shell apostrophe escaping")
        let root=URL(fileURLWithPath:NSTemporaryDirectory()).appendingPathComponent("archer desktop test ' \(UUID())")
        try FileManager.default.createDirectory(at:root.appendingPathComponent("artifacts"),withIntermediateDirectories:true);defer{try? FileManager.default.removeItem(at:root)}
        try FileManager.default.createDirectory(at:root.appendingPathComponent("research"),withIntermediateDirectories:true)
        let trusted=root.appendingPathComponent("trusted.bin");try Data("trusted".utf8).write(to:trusted)
        let ledger=root.appendingPathComponent("artifacts/37ad-0104-sequences.bin");try Data([65,82,67,72,83,69,81,1]+Array(repeating:1,count:50)+Array(repeating:0,count:206)).write(to:ledger)
        let readiness=root.appendingPathComponent("research/readiness.json")
        var manifest:[String:Any]=["status":"prepared_not_executed","stage":182,"epoch_id":"epoch","remaining_sequences":206,"ledger_sha256":try digestFile(ledger),"build_evidence":["trusted.bin":try digestFile(trusted)]]
        try JSONSerialization.data(withJSONObject:manifest).write(to:readiness)
        let w=Workflow(id:"test",title:"Echo",detail:"test",stage:182,mode:"--dhcp-supplicant",readiness:"research/readiness.json",marker:"artifacts/consumed",outputPrefix:"desktop-icmp-stage182")
        var device=DeviceSnapshot(mode:.wifi);device.context="CLEAN";device.fence="CLEAN";device.remaining=206;device.epoch="epoch"
        let ready=WorkflowGate.check(w,root:root,device:device);check(ready.ready && ready.command.contains(shellQuote(root.path)),"ready workflow command escaped")
        try Data().write(to:root.appendingPathComponent("artifacts/consumed"));check(!WorkflowGate.check(w,root:root,device:device).ready,"consumed marker blocks")
        try FileManager.default.removeItem(at:root.appendingPathComponent("artifacts/consumed"))
        try Data("changed".utf8).write(to:trusted);check(!WorkflowGate.check(w,root:root,device:device).ready,"changed build blocks")
        try Data("trusted".utf8).write(to:trusted);device.context="DIRTY";check(!WorkflowGate.check(w,root:root,device:device).ready,"dirty state blocks")
        device.context="CLEAN";device.fence="DIRTY";check(!WorkflowGate.check(w,root:root,device:device).ready,"epoch fence blocks")
        device.fence="CLEAN";device.mode = .installer;check(!WorkflowGate.check(w,root:root,device:device).ready,"installer blocks radio workflow")
        device.mode = .wifi;manifest["build_evidence"]=["../outside":"hash"];try JSONSerialization.data(withJSONObject:manifest).write(to:readiness);check(!WorkflowGate.check(w,root:root,device:device).ready,"path escape blocks")
        if CommandLine.arguments.count>1 {
            let project=URL(fileURLWithPath:CommandLine.arguments[1]);let captures=CaptureRecord.load(project);check(!captures.isEmpty,"real capture catalog")
            if let record=captures.first(where:{$0.title=="unicast-stage179"}),let url=record.pcap,let log=record.nativeLog {
                let real=try PcapReader.parse(readBounded(url,limit:67_108_864),allowPartial:true)
                check(real.packets.count>2000,"real radiotap capture decoded");check(real.networks.contains{$0.ssid=="Especial"},"real target network decoded")
                let proof=NativeSummary.parse(try readBounded(log));check(proof.arp && proof.cleanup && proof.pairwise==1,"real unicast result decoded")
                print("Real capture: \(real.packets.count) packets, \(real.networks.count) networks, \(real.crcFailures) CRC failures; native pairwise \(proof.pairwise)")
            }else{check(false,"real reference capture exists")}
        }
        print("Archer desktop: \(checks) parser, export and workflow checks passed")
    }
}
