import AppKit
import Foundation
let directory=URL(fileURLWithPath:CommandLine.arguments[1]);try FileManager.default.createDirectory(at:directory,withIntermediateDirectories:true)
func render(_ pixels:Int,_ name:String)throws{
    let rep=NSBitmapImageRep(bitmapDataPlanes:nil,pixelsWide:pixels,pixelsHigh:pixels,bitsPerSample:8,samplesPerPixel:4,hasAlpha:true,isPlanar:false,colorSpaceName:.deviceRGB,bytesPerRow:0,bitsPerPixel:0)!
    let context=NSGraphicsContext(bitmapImageRep:rep)!;NSGraphicsContext.saveGraphicsState();NSGraphicsContext.current=context
    context.cgContext.scaleBy(x:CGFloat(pixels)/1024,y:CGFloat(pixels)/1024)
    let square=NSBezierPath(roundedRect:NSRect(x:44,y:44,width:936,height:936),xRadius:210,yRadius:210)
    NSGradient(starting:NSColor(calibratedRed:0.10,green:0.15,blue:0.21,alpha:1),ending:NSColor(calibratedRed:0.025,green:0.045,blue:0.07,alpha:1))!.draw(in:square,angle:-80)
    NSColor.white.withAlphaComponent(0.08).setStroke();square.lineWidth=3;square.stroke()
    NSColor(calibratedRed:0.29,green:0.91,blue:0.78,alpha:1).setStroke()
    for radius in [170.0,280.0,390.0]{let p=NSBezierPath();p.appendArc(withCenter:NSPoint(x:512,y:340),radius:radius,startAngle:38,endAngle:142);p.lineWidth=48;p.lineCapStyle = .round;p.stroke()}
    NSColor(calibratedRed:0.29,green:0.91,blue:0.78,alpha:1).setFill();NSBezierPath(ovalIn:NSRect(x:474,y:290,width:76,height:76)).fill()
    NSGraphicsContext.restoreGraphicsState();try rep.representation(using:.png,properties:[:])!.write(to:directory.appendingPathComponent(name))
}
for size in [16,32,128,256,512]{try render(size,"icon_\(size)x\(size).png");try render(size*2,"icon_\(size)x\(size)@2x.png")}
