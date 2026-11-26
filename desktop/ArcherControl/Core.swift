import Foundation
import CryptoKit

struct ArcherError: LocalizedError { let message: String; var errorDescription: String? { message } }
func readBounded(_ url: URL, limit: Int = 16_777_216) throws -> Data {
    let size = try url.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0
    guard size <= limit else { throw ArcherError(message: "Archivo demasiado grande: \(url.lastPathComponent)") }
    return try Data(contentsOf: url)
}
func jsonObject(_ url: URL) -> [String: Any] { (try? JSONSerialization.jsonObject(with: readBounded(url))) as? [String: Any] ?? [:] }
func fileDate(_ url: URL) -> Date { (try? url.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate) ?? .distantPast }
func textFile(_ url: URL, limit: Int = 4096) -> String { String(data: (try? readBounded(url, limit: limit)) ?? Data(), encoding: .utf8)?.trimmingCharacters(in: .whitespacesAndNewlines) ?? "" }
func safeChild(_ root: URL, _ relative: String) throws -> URL {
    let base = root.standardizedFileURL.resolvingSymlinksInPath()
    let target = root.appendingPathComponent(relative).standardizedFileURL.resolvingSymlinksInPath()
    guard !relative.hasPrefix("/"), target.path.hasPrefix(base.path + "/") else { throw ArcherError(message: "Ruta fuera del proyecto") }
    return target
}
func digestFile(_ url: URL) throws -> String {
    let handle = try FileHandle(forReadingFrom: url); defer { try? handle.close() }
    var hash = SHA256(); var total = 0
    while let data = try handle.read(upToCount: 1_048_576), !data.isEmpty {
        total += data.count; guard total <= 67_108_864 else { throw ArcherError(message: "Archivo de admisión demasiado grande") }
        hash.update(data: data)
    }
    return hash.finalize().map { String(format: "%02x", $0) }.joined()
}
func shellQuote(_ s: String) -> String { "'" + s.replacingOccurrences(of: "'", with: "'\"'\"'") + "'" }
func appleScriptString(_ s: String) -> String { "\"" + s.replacingOccurrences(of: "\\", with: "\\\\").replacingOccurrences(of: "\"", with: "\\\"").replacingOccurrences(of: "\n", with: "\\n") + "\"" }

struct RadioPacket: Identifiable {
    let id: Int; let timestamp: Double; let relative: Double; let bytes: Int
    let kind: String; let protocolName: String; let source: String; let destination: String; let bssid: String
    let encrypted: Bool; let signal: Int?; let channel: Int?; let frequency: Int?; let crcValid: Bool?
    let raw: [UInt8]; let ssid: String?; let privacy: String?; let truncated: Bool
    var direction: String { source == "02:00:00:89:22:a1" ? "TX estación" : destination == "02:00:00:89:22:a1" ? "RX estación" : "Ambiente" }
    var searchText: String { "\(id) \(kind) \(protocolName) \(source) \(destination) \(bssid) \(ssid ?? "") \(direction)".lowercased() }
    var hexDump: String {
        stride(from: 0, to: min(raw.count, 1536), by: 16).map { offset in
            let row = Array(raw[offset..<min(offset + 16, raw.count)])
            let hex = row.map { String(format: "%02x", $0) }.joined(separator: " ")
            let ascii = row.map { (32...126).contains($0) ? String(UnicodeScalar(Int($0))!) : "." }.joined()
            return String(format: "%04x  ", offset) + hex.padding(toLength: 48, withPad: " ", startingAt: 0) + "  " + ascii
        }.joined(separator: "\n") + (raw.count > 1536 ? "\n… vista limitada a 1536 bytes" : "")
    }
}
struct ObservedNetwork: Identifiable { var id: String { bssid }; let bssid: String; var ssid: String; var channel: Int?; var signal: Int?; var beacons: Int; var security: String }
struct RateBucket: Identifiable { var id: Int { index }; let index: Int; let time: Double; var management: Int = 0; var data: Int = 0; var control: Int = 0; var other: Int = 0; var total: Int { management + data + control + other } }
struct SignalPoint: Identifiable { let id: Int; let time: Double; let signal: Int }
struct PacketCapture {
    var packets: [RadioPacket] = []; var warnings: [String] = []; var networks: [ObservedNetwork] = []
    var duration: Double { packets.map(\.relative).max() ?? 0 }
    var byteCount: Int { packets.reduce(0) { $0 + $1.bytes } }
    var crcFailures: Int { packets.filter { $0.crcValid == false }.count }
    var protectedCount: Int { packets.filter(\.encrypted).count }
    var rate: [RateBucket] {
        let width = max(1.0, ceil(duration / 90)); var buckets: [Int: RateBucket] = [:]
        for p in packets { let i = Int(p.relative / width); var b = buckets[i] ?? RateBucket(index: i, time: Double(i) * width)
            if p.kind == "Gestión" { b.management += 1 } else if p.kind == "Control" { b.control += 1 } else if p.kind == "Datos" { b.data += 1 } else { b.other += 1 }; buckets[i] = b
        }
        return buckets.values.sorted { $0.index < $1.index }
    }
    var signals: [SignalPoint] { packets.compactMap { p in p.signal.map { SignalPoint(id: p.id, time: p.relative, signal: $0) } }.enumerated().compactMap { $0.offset % max(1, packets.count / 700) == 0 ? $0.element : nil } }
}

enum PcapReader {
    static let crcTable: [UInt32] = (0..<256).map { n in var c = UInt32(n); for _ in 0..<8 { c = c & 1 == 1 ? 0xedb88320 ^ (c >> 1) : c >> 1 }; return c }
    static func crc(_ b: ArraySlice<UInt8>) -> UInt32 { var c: UInt32 = 0xffffffff; for v in b { c = crcTable[Int((c ^ UInt32(v)) & 255)] ^ (c >> 8) }; return c ^ 0xffffffff }
    static func parse(_ data: Data, allowPartial: Bool = false, limit: Int = 25_000) throws -> PacketCapture {
        let b = [UInt8](data)
        guard b.count >= 24 else { throw ArcherError(message: "Cabecera PCAP incompleta") }
        let magic = Array(b[0..<4]); let little: Bool; let nano: Bool
        switch magic { case [0xd4,0xc3,0xb2,0xa1]: little = true; nano = false
        case [0xa1,0xb2,0xc3,0xd4]: little = false; nano = false
        case [0x4d,0x3c,0xb2,0xa1]: little = true; nano = true
        case [0xa1,0xb2,0x3c,0x4d]: little = false; nano = true
        default: throw ArcherError(message: "Se necesita PCAP clásico con radiotap; PCAPNG todavía no está admitido") }
        func u32(_ at: Int) -> UInt32 { let p = Array(b[at..<at+4]); return little ? UInt32(p[0]) | UInt32(p[1])<<8 | UInt32(p[2])<<16 | UInt32(p[3])<<24 : UInt32(p[3]) | UInt32(p[2])<<8 | UInt32(p[1])<<16 | UInt32(p[0])<<24 }
        guard u32(16) > 0, u32(16) <= 1_048_576 else { throw ArcherError(message: "Snaplen PCAP inválido") }
        let snaplen = Int(u32(16))
        guard u32(20) & 0xffff == 127 else { throw ArcherError(message: "Tipo de enlace no admitido: se necesita radiotap (127)") }
        var result = PacketCapture(); var cursor = 24; var start: Double?; var recordCount = 0; var networks: [String: ObservedNetwork] = [:]
        while cursor < b.count {
            if recordCount >= limit { result.warnings.append("Vista limitada a \(limit) paquetes; el PCAP original se conserva completo."); break }
            guard cursor + 16 <= b.count else { if allowPartial { result.warnings.append("Último registro incompleto; la captura puede seguir escribiéndose."); break }; throw ArcherError(message: "Registro PCAP incompleto") }
            let seconds = u32(cursor), fraction = u32(cursor+4), count = Int(u32(cursor+8)), original = Int(u32(cursor+12)); cursor += 16
            guard count <= original, count <= snaplen, count <= 1_048_576, fraction < (nano ? 1_000_000_000 : 1_000_000) else { throw ArcherError(message: "Longitud o timestamp PCAP inválido") }
            guard count <= b.count - cursor else { if allowPartial { result.warnings.append("Último paquete incompleto; se muestra lo ya guardado."); break }; throw ArcherError(message: "Paquete PCAP truncado en el archivo") }
            let bytes = Array(b[cursor..<cursor+count]); cursor += count; recordCount += 1
            let timestamp = Double(seconds) + Double(fraction) / (nano ? 1e9 : 1e6); if start == nil { start = timestamp }
            guard let p = decode(bytes, id: result.packets.count+1, timestamp: timestamp, relative: max(0, timestamp - (start ?? timestamp)), truncated: count != original) else { if result.warnings.count < 16 { result.warnings.append("Registro \(recordCount) sin cabecera radiotap/802.11 válida.") }; continue }
            result.packets.append(p)
            if p.kind == "Gestión", p.ssid != nil, !p.bssid.isEmpty, p.crcValid != false, !p.truncated {
                var n = networks[p.bssid] ?? ObservedNetwork(bssid: p.bssid, ssid: p.ssid!.isEmpty ? "Red oculta" : p.ssid!, channel: p.channel, signal: p.signal, beacons: 0, security: p.privacy ?? "Desconocida")
                if let ssid = p.ssid, !ssid.isEmpty { n.ssid = ssid }; n.beacons += 1
                n.channel = p.channel ?? n.channel; n.signal = p.signal ?? n.signal; networks[p.bssid] = n
            }
        }
        result.networks = networks.values.sorted { ($0.signal ?? -200) > ($1.signal ?? -200) }; return result
    }
    static func decode(_ b: [UInt8], id: Int, timestamp: Double, relative: Double, truncated: Bool) -> RadioPacket? {
        guard b.count >= 8, b[0] == 0 else { return nil }
        func le16(_ at: Int) -> Int { Int(b[at]) | Int(b[at+1]) << 8 }
        func le32(_ at: Int) -> UInt32 { UInt32(b[at]) | UInt32(b[at+1])<<8 | UInt32(b[at+2])<<16 | UInt32(b[at+3])<<24 }
        let header = le16(2); guard header >= 8, header <= b.count else { return nil }
        let first = le32(4); var word = first; var offset = 8; var words = 1
        while word & 0x80000000 != 0 { guard offset+4 <= header, words < 8 else { return nil }; word = le32(offset); offset += 4; words += 1 }
        var flags: UInt8?; var frequency: Int?; var signal: Int?
        for (bit, align, size) in [(0,8,8),(1,1,1),(2,1,1),(3,2,4),(4,2,2),(5,1,1)] {
            if first & (1 << bit) == 0 { continue }; offset = (offset + align - 1) / align * align
            guard offset + size <= header else { return nil }
            if bit == 1 { flags = b[offset] }; if bit == 3 { frequency = le16(offset) }; if bit == 5 { signal = Int(Int8(bitPattern: b[offset])) }; offset += size
        }
        var frame = Array(b[header...]); guard frame.count >= 10 else { return nil }
        var valid: Bool? = nil
        if flags.map({ $0 & 0x10 != 0 }) == true, frame.count >= 14, !truncated {
            let n = frame.count; let recorded = UInt32(frame[n-4]) | UInt32(frame[n-3])<<8 | UInt32(frame[n-2])<<16 | UInt32(frame[n-1])<<24
            if flags.map({$0 & 0x20 == 0}) == true { valid = crc(frame[0..<n-4]) == recorded }; frame.removeLast(4)
        }
        let fc = Int(frame[0]) | Int(frame[1]) << 8; let type = (fc >> 2) & 3, subtype = (fc >> 4) & 15
        func mac(_ at: Int) -> String { guard at+6 <= frame.count else { return "" }; return frame[at..<at+6].map { String(format: "%02x", $0) }.joined(separator: ":") }
        let a1 = mac(4), a2 = mac(10), a3 = mac(16); var source = a2; var destination = a1; var bssid = ""; var name = "802.11"; var kind = "Otro"; var ssid: String?; var privacy: String?; var channel: Int?
        if let f = frequency { if f == 2484 { channel = 14 } else if (2412...2472).contains(f) { channel = (f-2407)/5 } else if (5000...5895).contains(f) { channel = (f-5000)/5 } }
        if type == 0 {
            guard frame.count >= 24 else { return nil }; kind = "Gestión"; bssid = a3
            name = [0:"Asociación",1:"Respuesta asociación",2:"Reasociación",3:"Respuesta reasociación",4:"Probe request",5:"Probe response",8:"Beacon",10:"Desasociación",11:"Autenticación",12:"Desautenticación",13:"Acción"][subtype] ?? "Gestión \(subtype)"
            if (subtype == 8 || subtype == 5), frame.count >= 36 {
                let protectedNetwork = (Int(frame[34]) | Int(frame[35])<<8) & 0x10 != 0; privacy = protectedNetwork ? "Protegida" : "Abierta"
                var at = 36
                while at+2 <= frame.count { let field = Int(frame[at]), n = Int(frame[at+1]); at += 2; guard at+n <= frame.count else { break }
                    if field == 0, n <= 32 { ssid = String(bytes: frame[at..<at+n], encoding: .utf8) ?? frame[at..<at+n].map { String(format:"%02x",$0) }.joined() }
                    if field == 3, n == 1 { channel = Int(frame[at]) }; if field == 48 { privacy = "RSN" }; at += n
                }
            }
        } else if type == 1 {
            kind = "Control"; name = [8:"Block ACK request",9:"Block ACK",10:"PS-Poll",11:"RTS",12:"CTS",13:"ACK",14:"CF-End"][subtype] ?? "Control \(subtype)"; source = (subtype == 12 || subtype == 13) ? "" : a2
        } else if type == 2 {
            guard frame.count >= 24 else { return nil }; kind = "Datos"; let toDS = fc & 0x100 != 0, fromDS = fc & 0x200 != 0
            if toDS && !fromDS { source = a2; destination = a3; bssid = a1 } else if fromDS && !toDS { source = a3; destination = a1; bssid = a2 } else if !toDS && !fromDS { bssid = a3 } else { source = mac(24); destination = a3 }
            name = fc & 0x4000 != 0 ? "Datos cifrados" : "Datos"
            var dataHeader = 24 + (toDS && fromDS ? 6 : 0) + (subtype & 8 != 0 ? 2 : 0) + (subtype & 8 != 0 && fc & 0x8000 != 0 ? 4 : 0)
            if flags.map({$0 & 0x20 != 0}) == true { dataHeader = (dataHeader+3)/4*4 }
            if fc & 0x4000 == 0, frame.count >= dataHeader+8, Array(frame[dataHeader..<dataHeader+6]) == [0xaa,0xaa,3,0,0,0] {
                let ether = Int(frame[dataHeader+6])<<8 | Int(frame[dataHeader+7]); if ether == 0x888e { name = "EAPOL" }; if ether == 0x806 { name = "ARP" }; if ether == 0x86dd { name = "IPv6" }
                if ether == 0x800, frame.count >= dataHeader+28 { let ip = dataHeader+8; name = [1:"ICMP",6:"TCP",17:"UDP"][Int(frame[ip+9])] ?? "IPv4" }
            }
        }
        return RadioPacket(id: id, timestamp: timestamp, relative: relative, bytes: b.count, kind: kind, protocolName: name, source: source, destination: destination, bssid: bssid, encrypted: fc & 0x4000 != 0, signal: signal, channel: channel, frequency: frequency, crcValid: valid, raw: b, ssid: ssid, privacy: privacy, truncated: truncated)
    }
}

struct NativeEvent: Identifiable { let id: Int; let title: String; let detail: String; let success: Bool }
struct TxReport: Identifiable { var id: Int { tag }; let tag: Int; let state: Int; let count: Int }
struct NativeSummary {
    var authenticated = 0; var pairwise = 0; var group = 0; var rejected = 0; var reports: [TxReport] = []; var events: [NativeEvent] = []
    var security = false; var lease = false; var arp = false; var echo = false; var released = false; var cleanup = false; var malformedLines = 0
    static func parse(_ data: Data) -> NativeSummary {
        var s = NativeSummary(); guard let text = String(data: data, encoding: .utf8) else { return s }
        for (index,line) in text.split(separator: "\n").enumerated() {
            guard let bytes = String(line).data(using:.utf8), let r = (try? JSONSerialization.jsonObject(with: bytes)) as? [String:Any] else { s.malformedLines += 1; continue }
            let op = r["operation"] as? String ?? ""; var title: String?; var detail = ""; var success = true
            if op == "secure_data_summary" { s.authenticated = r["authenticated_frames"] as? Int ?? 0; s.pairwise = r["pairwise_frames"] as? Int ?? 0; s.group = r["group_frames"] as? Int ?? 0; s.rejected = r["rejected_frames"] as? Int ?? 0 }
            if op == "single_probe_tx_report" { s.reports.append(TxReport(tag:r["tag"] as? Int ?? 0,state:r["state"] as? Int ?? -1,count:r["tx_count"] as? Int ?? 0)) }
            switch op {
            case "secure_m3_authenticated": s.security = true; title = "WPA2 autenticado"; detail = "M3 verificado por el ejecutor"
            case "secure_dhcp_offer": title = "Oferta DHCP coincidente"; detail = "XID y cliente comprobados"
            case "secure_dhcp_ack": s.lease = r["lease_acknowledged"] as? Bool == true; title = "Lease temporal reconocido"; success = s.lease
            case "secure_unicast_arp_reply": s.arp = r["pairwise"] as? Bool == true && r["ccmp_authenticated"] as? Bool == true && r["identity_matched"] as? Bool == true; title = "ARP pairwise"; detail = r["local_replay_rejected"] as? Bool == true ? "Replay local rechazado" : "Sin prueba de replay local"; success = s.arp
            case "secure_unicast_echo_reply": s.echo = r["pairwise"] as? Bool == true && r["echo_cookie_matched"] as? Bool == true && r["ccmp_authenticated"] as? Bool == true; title = "ICMP de ida y vuelta"; detail = "Respuesta pairwise coincidente"; success = s.echo
            case "secure_dhcp_release": s.released = r["tx_done"] as? Bool == true; title = "RELEASE enviado"; detail = "No confirma liberación del servidor"; success = s.released
            case "secure_m2_response_retry": title = "Respuesta adicional a M1"; detail = "Replay creciente"
            case "secure_request_retry": title = "Retransmisión DHCP REQUEST"; detail = "PN y secuencia nuevos"
            case "secure_dhcp_nak": title = "DHCP NAK"; detail = "Solicitud rechazada"; success = false
            default: break
            }
            if r["result"] as? String == "context_cleanup_acknowledged" { s.cleanup = true; title = "Contexto retirado"; detail = "ACK de firmware y TX pausado" }
            if let title = title { s.events.append(NativeEvent(id:index,title:title,detail:detail,success:success)) }
        }
        return s
    }
}
struct CaptureRecord: Identifiable, Hashable {
    let id: String; let title: String; let date: Date; let status: String; let attempt: String; let monitorExit: Int?
    let folder: URL; let pcap: URL?; let nativeLog: URL?; let validation: [String:Bool]
    static func ==(a:Self,b:Self)->Bool { a.id == b.id }; func hash(into h: inout Hasher) { h.combine(id) }
    var statusTitle: String { ["completed":"Completado","stopped":"Detenido","preflight":"Preparación","prepared":"Preparado","imported":"PCAP importado"][status] ?? "Sin resultado final" }
    static func load(_ root: URL) -> [CaptureRecord] {
        let fm = FileManager.default; let artifacts = root.appendingPathComponent("artifacts")
        guard let folders = try? fm.contentsOfDirectory(at:artifacts,includingPropertiesForKeys:[.isDirectoryKey,.contentModificationDateKey],options:[.skipsHiddenFiles]) else { return [] }
        return folders.compactMap { folder -> CaptureRecord? in
            guard (try? folder.resourceValues(forKeys:[.isDirectoryKey]).isDirectory) == true else { return nil }
            guard (try? safeChild(root,"artifacts/"+folder.lastPathComponent)) != nil else { return nil }
            let paired = jsonObject(folder.appendingPathComponent("paired.json")); let attempt = jsonObject(folder.appendingPathComponent("probe/attempt.json"))
            let pcap = folder.appendingPathComponent("monitor/monitor-channel9.pcap"); let log = folder.appendingPathComponent("probe/probe.jsonl")
            guard !paired.isEmpty || !attempt.isEmpty || fm.fileExists(atPath:pcap.path) else { return nil }
            return CaptureRecord(id:folder.path,title:folder.lastPathComponent,date:max(fileDate(folder.appendingPathComponent("paired.json")),fileDate(log)),status:attempt["status"] as? String ?? "unknown",attempt:attempt["attempt"] as? String ?? paired["attempt"] as? String ?? "captura",monitorExit:paired["monitor_exit"] as? Int,folder:folder,pcap:fm.fileExists(atPath:pcap.path) ? pcap : nil,nativeLog:fm.fileExists(atPath:log.path) ? log : nil,validation:(attempt["validation"] as? [String:Any] ?? [:]).compactMapValues{$0 as? Bool})
        }.sorted { $0.date > $1.date }
    }
}
struct DeviceSnapshot {
    enum Mode { case wifi, installer, absent, error }
    var mode: Mode; var session: String = ""; var context: String = "Desconocido"; var epoch: String = ""; var fence: String = "Desconocido"; var remaining: Int?; var contextDate: Date = .distantPast; var error: String = ""
    var title: String { switch mode { case .wifi:return "USB Wi-Fi enumerado"; case .installer:return "Modo instalador"; case .absent:return "Archer desconectado"; case .error:return "USB sin verificar" } }
    static func read(_ root:URL)->DeviceSnapshot {
        var s=DeviceSnapshot(mode:.absent); let p=Process();p.executableURL=URL(fileURLWithPath:"/usr/sbin/ioreg");p.arguments=["-p","IOUSB","-l","-w","0"];let pipe=Pipe();p.standardOutput=pipe;p.standardError=Pipe()
        do { try p.run();let data=pipe.fileHandleForReading.readDataToEndOfFile();p.waitUntilExit();guard p.terminationStatus==0 else { throw ArcherError(message:"ioreg falló") }
            let text=String(decoding:data,as:UTF8.self)
            func number(_ pattern:String,_ text:String)->String? { guard let re=try? NSRegularExpression(pattern:pattern),let m=re.firstMatch(in:text,range:NSRange(text.startIndex...,in:text)),let range=Range(m.range(at:1),in:text) else{return nil};return String(text[range]) }
            for block in text.components(separatedBy:"+-o ") { if let v=number("\"idVendor\" = ([0-9]+)",block),let id=number("\"idProduct\" = ([0-9]+)",block), (v=="14253" && id=="260" || v=="3034" && id=="6699") { s.mode = v=="14253" ? .wifi : .installer;s.session=number("\"sessionID\" = ([0-9]+)",block) ?? "" } }
        } catch { s.mode = .error;s.error=error.localizedDescription }
        let context=root.appendingPathComponent("artifacts/37ad-0104-context-reservation");let recorded=textFile(context)
        s.context=["CLEAN","DIRTY"].contains(recorded) ? recorded : "Desconocido";s.contextDate=fileDate(context);s.fence=textFile(root.appendingPathComponent("artifacts/37ad-0104-sequences.bin.epoch-transition"));s.epoch=textFile(root.appendingPathComponent("artifacts/37ad-0104-sequences.bin.epoch-id"))
        if let data=try? readBounded(root.appendingPathComponent("artifacts/37ad-0104-sequences.bin"),limit:264),data.count==264,Array(data.prefix(8))==[65,82,67,72,83,69,81,1],data.dropFirst(8).allSatisfy({$0<=1}) { s.remaining=data.dropFirst(8).filter{$0==0}.count }
        return s
    }
}
struct Workflow: Decodable, Identifiable { let id:String;let title:String;let detail:String;let stage:Int;let mode:String;let readiness:String;let marker:String;let outputPrefix:String }
struct WorkflowList: Decodable { let version:Int;let workflows:[Workflow] }
struct WorkflowCheck { let ready:Bool;let reason:String;let command:String;let title:String;let detail:String }
enum WorkflowGate {
    static func check(_ workflow:Workflow,root:URL,device:DeviceSnapshot)->WorkflowCheck {
        func result(_ ready:Bool,_ reason:String,_ command:String="")->WorkflowCheck { WorkflowCheck(ready:ready,reason:reason,command:command,title:workflow.title,detail:workflow.detail) }
        do {
            guard workflow.mode=="--dhcp-supplicant", workflow.stage==182, workflow.outputPrefix=="desktop-icmp-stage182" else { return result(false,"Flujo no admitido por esta versión de la app") }
            let readiness=try safeChild(root,workflow.readiness),marker=try safeChild(root,workflow.marker)
            guard !FileManager.default.fileExists(atPath:marker.path) else { return result(false,"Intento ya reservado. No se puede repetir.") }
            guard device.mode == .wifi else { return result(false,"Se necesita el Archer en interfaz USB Wi-Fi") }
            guard device.context=="CLEAN", device.fence=="CLEAN" else { return result(false,"Contexto guardado no CLEAN: requiere recuperación verificada") }
            let r=jsonObject(readiness)
            guard r["status"] as? String=="prepared_not_executed",r["stage"] as? Int==workflow.stage,r["epoch_id"] as? String==device.epoch,r["remaining_sequences"] as? Int==device.remaining else { return result(false,"La preparación no coincide con la época o las reservas actuales") }
            guard try digestFile(safeChild(root,"artifacts/37ad-0104-sequences.bin"))==r["ledger_sha256"] as? String else { return result(false,"El ledger cambió respecto a la preparación") }
            guard let evidence=r["build_evidence"] as? [String:String],!evidence.isEmpty else { return result(false,"Falta evidencia de compilación y pruebas") }
            for (name,sha) in evidence { guard try digestFile(safeChild(root,name))==sha else { return result(false,"La preparación cambió: \(URL(fileURLWithPath:name).lastPathComponent)") } }
            let output="artifacts/\(workflow.outputPrefix)-\(UUID().uuidString.lowercased().prefix(8))"
            let command="cd \(shellQuote(root.path)) && sudo -v && python3 tools/external_probe_capture.py \(workflow.mode) \(shellQuote(output))"
            return result(true,"Preparación comprobada. El coordinador volverá a validar antes de transmitir.",command)
        } catch { return result(false,error.localizedDescription) }
    }
}
enum CSVExport {
    static func field(_ input:String)->String { var value=input;if let first=value.first,"=+-@".contains(first){value="'"+value};return "\""+value.replacingOccurrences(of:"\"",with:"\"\"")+"\"" }
    static func packets(_ packets:[RadioPacket])->String { "numero,segundos,protocolo,tipo,origen,destino,bssid,bytes,rssi_dbm,canal,cifrado,crc\n" + packets.map { p in [String(p.id),String(format:"%.6f",p.relative),p.protocolName,p.kind,p.source,p.destination,p.bssid,String(p.bytes),p.signal.map(String.init) ?? "",p.channel.map(String.init) ?? "",p.encrypted ? "si":"no",p.crcValid.map{$0 ? "valido":"invalido"} ?? "no disponible"].map(field).joined(separator:",") }.joined(separator:"\n") }
}
