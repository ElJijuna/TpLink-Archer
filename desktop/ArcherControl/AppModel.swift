import SwiftUI
import AppKit
import UniformTypeIdentifiers
import CryptoKit

struct CaptureDetails { var capture = PacketCapture(); var native = NativeSummary(); var pcapDigest = ""; var pcapVerified: Bool?; var nativeVerified: Bool?; var error = ""; var monitorWarning = "" }
enum WorkspacePage: String, CaseIterable, Identifiable {
    case overview = "Resumen", networks = "Redes", traffic = "Tráfico", captures = "Capturas", modes = "Modos", activity = "Actividad"
    var id:String { rawValue }
    var icon:String { switch self { case .overview:return "square.grid.2x2";case .networks:return "wifi";case .traffic:return "waveform.path";case .captures:return "tray.full";case .modes:return "slider.horizontal.3";case .activity:return "list.bullet.rectangle" } }
}
@MainActor final class AppModel: ObservableObject {
    @Published var project: URL
    @Published var device = DeviceSnapshot(mode:.absent)
    @Published var captures: [CaptureRecord] = []
    @Published var selected: CaptureRecord?
    @Published var details = CaptureDetails()
    @Published var page:WorkspacePage = .overview
    @Published var search = ""
    @Published var packetKind = "Todos"
    @Published var stationOnly = false
    @Published var selectedPacket:Int?
    @Published var refreshing = false
    @Published var loadingCapture = false
    @Published var message = ""
    @Published var workflow:WorkflowCheck?
    @Published var progress = 0
    @Published var analysisMode = "Inspección"
    private var generation = UUID()
    private var loadedModification = Date.distantPast
    private var workflowDefinition:Workflow?
    init() {
        let args=CommandLine.arguments
        if let index=args.firstIndex(of:"--project"),args.count>index+1 { project=URL(fileURLWithPath:args[index+1]) }
        else if let saved=UserDefaults.standard.string(forKey:"projectPath") { project=URL(fileURLWithPath:saved) }
        else if let url=Bundle.main.url(forResource:"DefaultProject",withExtension:"txt") { project=URL(fileURLWithPath:textFile(url)) }
        else { project=URL(fileURLWithPath:FileManager.default.currentDirectoryPath) }
        refresh()
    }
    var packets:[RadioPacket] {
        details.capture.packets.filter { p in
            (search.isEmpty || p.searchText.contains(search.lowercased())) &&
            (packetKind == "Todos" || p.kind == packetKind) &&
            (!stationOnly || p.direction != "Ambiente")
        }
    }
    var inspected:RadioPacket? { details.capture.packets.first { $0.id == selectedPacket } }
    var networks:[ObservedNetwork] { details.capture.networks.filter { search.isEmpty || "\($0.ssid) \($0.bssid)".localizedCaseInsensitiveContains(search) } }
    func refresh() {
        guard !refreshing else{return};refreshing=true;let root=project
        Task {
            let loaded=await Task.detached(priority:.userInitiated) { () -> (DeviceSnapshot,[CaptureRecord],Workflow?,WorkflowCheck?,Int) in
                let device=DeviceSnapshot.read(root);let records=CaptureRecord.load(root)
                let local=root.appendingPathComponent("desktop/workflows.json")
                let config=FileManager.default.fileExists(atPath:local.path) ? local : Bundle.main.url(forResource:"workflows",withExtension:"json")
                let definition=config.flatMap { try? JSONDecoder().decode(WorkflowList.self,from:readBounded($0)) }.flatMap { $0.version==1 ? $0.workflows.first : nil }
                let workflow=definition.map { WorkflowGate.check($0,root:root,device:device) }
                let readme=textFile(root.appendingPathComponent("README.md"),limit:1_048_576)
                let pattern="avance estimado: \\*\\*([0-9]+) %"
                var progress=0
                if let re=try? NSRegularExpression(pattern:pattern),let m=re.firstMatch(in:readme,range:NSRange(readme.startIndex...,in:readme)),let range=Range(m.range(at:1),in:readme){progress=Int(readme[range]) ?? 0}
                return (device,records,definition,workflow,progress)
            }.value
            guard project==root else{refreshing=false;return}
            device=loaded.0;captures=loaded.1;workflowDefinition=loaded.2;workflow=loaded.3;progress=loaded.4;refreshing=false
            if let selected=selected,let updated=captures.first(where:{$0.id==selected.id}) {
                self.selected=updated
                let changed=max(updated.pcap.map(fileDate) ?? .distantPast,updated.nativeLog.map(fileDate) ?? .distantPast)
                if changed>loadedModification { load(updated) }
            } else if selected?.status != "imported", let first=captures.first { select(first) }
        }
    }
    func select(_ record:CaptureRecord){selected=record;selectedPacket=nil;search="";load(record)}
    func load(_ record:CaptureRecord){
        loadingCapture=true;let token=UUID();generation=token
        Task {
            let loaded=await Task.detached(priority:.userInitiated) { () -> CaptureDetails in
                var result=CaptureDetails();let attempt=record.status=="imported" ? [:]:jsonObject(record.folder.appendingPathComponent("probe/attempt.json"));let paired=record.status=="imported" ? [:]:jsonObject(record.folder.appendingPathComponent("paired.json"))
                if let code=record.monitorExit,code != 0 {
                    let log=textFile(record.folder.appendingPathComponent("monitor.stderr"),limit:65_536)
                    if log.contains("packets captured") && log.contains("Reconnect the Mac Wi-Fi") {result.monitorWarning="El PCAP se guardó; falló la restauración del Wi-Fi del Mac. Revisa en0 si no se reconectó automáticamente."}
                    else {result.monitorWarning="El monitor terminó con código \(code). Consulta monitor.stderr antes de interpretar esta captura."}
                }
                if let log=record.nativeLog,let data=try? readBounded(log) {result.native=NativeSummary.parse(data);if let expected=attempt["stdout_sha256"] as? String {result.nativeVerified=SHA256.hash(data:data).map{String(format:"%02x",$0)}.joined()==expected}}
                if let pcap=record.pcap {
                    do {let data=try readBounded(pcap,limit:67_108_864);result.capture=try PcapReader.parse(data,allowPartial:true);result.pcapDigest=SHA256.hash(data:data).map{String(format:"%02x",$0)}.joined();if let expected=paired["pcap_sha256"] as? String{result.pcapVerified=result.pcapDigest==expected}}
                    catch {result.error=error.localizedDescription}
                }else{result.error="Este ensayo no incluye un PCAP independiente."}
                return result
            }.value
            guard generation==token else{return};details=loaded;loadingCapture=false
            loadedModification=max(record.pcap.map(fileDate) ?? .distantPast,record.nativeLog.map(fileDate) ?? .distantPast)
        }
    }
    func chooseProject(){
        let panel=NSOpenPanel();panel.title="Seleccionar el proyecto Tp-link-Archer";panel.canChooseDirectories=true;panel.canChooseFiles=false;panel.allowsMultipleSelection=false
        if panel.runModal() == .OK,let url=panel.url {project=url;UserDefaults.standard.set(url.path,forKey:"projectPath");selected=nil;captures=[];details=CaptureDetails();loadedModification = .distantPast;refresh()}
    }
    func importCapture(){
        let panel=NSOpenPanel();panel.title="Abrir una captura PCAP";panel.allowedContentTypes=[UTType(filenameExtension:"pcap") ?? .data];panel.allowsMultipleSelection=false
        if panel.runModal() == .OK,let url=panel.url {
            let record=CaptureRecord(id:url.path,title:url.lastPathComponent,date:fileDate(url),status:"imported",attempt:"PCAP importado",monitorExit:nil,folder:url.deletingLastPathComponent(),pcap:url,nativeLog:nil,validation:[:]);select(record);page = .traffic
        }
    }
    func exportCSV(){
        guard !packets.isEmpty else{return};let panel=NSSavePanel();panel.nameFieldStringValue="archer-paquetes.csv";panel.allowedContentTypes=[.commaSeparatedText]
        if panel.runModal() == .OK,let url=panel.url {do{try CSVExport.packets(packets).write(to:url,atomically:true,encoding:.utf8);message="CSV exportado: \(packets.count) paquetes"}catch{message=error.localizedDescription}}
    }
    func exportPCAP(){
        guard let original=selected?.pcap else{return};let panel=NSSavePanel();panel.nameFieldStringValue="archer-captura.pcap";panel.allowedContentTypes=[UTType(filenameExtension:"pcap") ?? .data]
        if panel.runModal() == .OK,let url=panel.url {do{guard original.standardizedFileURL != url.standardizedFileURL else{throw ArcherError(message:"Elige otra ruta para conservar el original")};try readBounded(original,limit:67_108_864).write(to:url,options:.atomic);message="PCAP exportado"}catch{message=error.localizedDescription}}
    }
    func copy(_ value:String){NSPasteboard.general.clearContents();NSPasteboard.general.setString(value,forType:.string);message="Copiado al portapapeles"}
    func revealCapture(){if let url=selected?.folder{NSWorkspace.shared.activateFileViewerSelecting([url])}}
    var installerCommand:String { "cd \(shellQuote(project.path)) && sudo ./build/archer-optical-eject-universal --eject-installer" }
    func launchInstaller(){
        let root=project
        Task {
            let snapshot=await Task.detached{DeviceSnapshot.read(root)}.value
            guard snapshot.mode == .installer,let helper=try? safeChild(root,"build/archer-optical-eject-universal"),FileManager.default.isExecutableFile(atPath:helper.path) else {message="El instalador no está presente o falta el helper verificado.";return}
            let command=installerCommand
            let error=await Task.detached{()->String? in
                let process=Process();process.executableURL=URL(fileURLWithPath:"/usr/bin/osascript");process.arguments=["-e","tell application \"Terminal\"\nactivate\ndo script \(appleScriptString(command))\nend tell"]
                let pipe=Pipe();process.standardError=pipe;process.standardOutput=Pipe()
                do{try process.run();let data=pipe.fileHandleForReading.readDataToEndOfFile();process.waitUntilExit();return process.terminationStatus==0 ? nil:String(decoding:data,as:UTF8.self)}catch{return error.localizedDescription}
            }.value
            message=error.map{"No se abrió Terminal. \($0)"} ?? "Expulsión abierta en Terminal; la contraseña sudo solo se introduce allí."
        }
    }
    func launchWorkflow(){
        guard let definition=workflowDefinition else{return};let root=project;message="Revalidando el ensayo…"
        Task {
            let checked=await Task.detached {WorkflowGate.check(definition,root:root,device:DeviceSnapshot.read(root))}.value
            workflow=checked;guard checked.ready else{message=checked.reason;return}
            let command=checked.command
            let error=await Task.detached {()->String? in
                let process=Process();process.executableURL=URL(fileURLWithPath:"/usr/bin/osascript");process.arguments=["-e","tell application \"Terminal\"\nactivate\ndo script \(appleScriptString(command))\nend tell"]
                let pipe=Pipe();process.standardError=pipe;process.standardOutput=Pipe()
                do{try process.run();let data=pipe.fileHandleForReading.readDataToEndOfFile();process.waitUntilExit();return process.terminationStatus==0 ? nil : String(decoding:data,as:UTF8.self)}catch{return error.localizedDescription}
            }.value
            if let error=error{message="No se abrió Terminal. Puedes copiar el comando. \(error)"}else{message="Ensayo abierto en Terminal. La contraseña solo se introduce allí."}
        }
    }
}
