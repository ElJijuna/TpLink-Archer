import SwiftUI
import AppKit

@main struct ArcherControlApp: App {
    @StateObject private var model=AppModel()
    init(){NSApplication.shared.setActivationPolicy(.regular)}
    var body:some Scene {
        WindowGroup("Archer Control") {RootView().environmentObject(model).preferredColorScheme(.dark).frame(minWidth:1120,minHeight:740)}
        .defaultSize(width:1360,height:880)
        .windowStyle(.hiddenTitleBar)
        .commands {
            CommandGroup(replacing:.newItem){Button("Abrir captura PCAP…"){model.importCapture()}.keyboardShortcut("o");Button("Seleccionar proyecto…"){model.chooseProject()}}
            CommandGroup(after:.toolbar){Button("Actualizar"){model.refresh()}.keyboardShortcut("r");Button("Exportar paquetes CSV…"){model.exportCSV()}.keyboardShortcut("e")}
        }
    }
}
