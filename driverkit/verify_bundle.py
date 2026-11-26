"""Validate the built DEXT without loading or executing it."""
from pathlib import Path
import plistlib
import struct

ROOT = Path(__file__).resolve().parents[1]
BUNDLE = ROOT / 'build/driverkit/service/ArcherUSB.dext'
info = plistlib.loads((BUNDLE/'Contents/Info.plist').read_bytes())
assert info['CFBundlePackageType']=='DEXT'
assert len(info['IOKitPersonalities'])==1
personality=next(iter(info['IOKitPersonalities'].values()))
expected={'IOClass':'IOUserNetworkEthernet','IOProviderClass':'IOUSBHostInterface',
          'IOUserClass':'ArcherUSB','CFBundleIdentifierKernel':'com.apple.iokit.IOSkywalkFamily',
          'idVendor':0x37ad,'idProduct':0x0104,'bConfigurationValue':1,
          'bInterfaceNumber':0,'bInterfaceClass':255,'bInterfaceSubClass':255,'bInterfaceProtocol':255}
assert all(personality.get(k)==v for k,v in expected.items())
assert personality['IOUserServerName']==personality['CFBundleIdentifier']==info['CFBundleIdentifier']
binary=(BUNDLE/'Contents/MacOS'/info['CFBundleExecutable']).read_bytes()
magic,count=struct.unpack_from('>II',binary)
assert magic==0xcafebabe and count==2
architectures=[]
for i in range(count):
    cpu,subtype,offset,size,align=struct.unpack_from('>IIIII',binary,8+20*i)
    thin=binary[offset:offset+size]
    assert len(thin)==size
    header=struct.unpack_from('<IiiIIIII',thin)
    assert header[0]==0xfeedfacf and header[1]==cpu and header[3]==2
    at=32;platforms=[];libraries=[]
    for _ in range(header[4]):
        command,length=struct.unpack_from('<II',thin,at)
        assert length>=8 and at+length<=len(thin)
        if command==0x32:
            platform,minimum,sdk,tools=struct.unpack_from('<IIII',thin,at+8)
            platforms.append(platform)
            assert platform==10 and minimum==0x180000  # DriverKit, 24.0
        if command==0xc:
            name_offset=struct.unpack_from('<I',thin,at+8)[0]
            libraries.append(thin[at+name_offset:at+length].split(b'\0',1)[0].decode())
        at+=length
    assert platforms==[10]
    for framework in ['DriverKit','USBDriverKit','NetworkingDriverKit']:
        assert any(f'/{framework}.framework/{framework}' in lib for lib in libraries)
    architectures.append(cpu)
assert set(architectures)=={0x01000007,0x0100000c}
for architecture in ['arm64','x86_64']:
    header=(ROOT/f'build/driverkit/service/{architecture}/ArcherUSB.h').read_text()
    for method in ['GetBoundFirmwarePlan','ReadBootControls','ReadRegister','WriteRegister','SubmitBulk','CopyBulkResult','ReleaseBulk','GetTransportStatus']:
        assert f'#define ArcherUSB_{method}_ID' not in header, f'{method} unexpectedly exported as RPC'
    assert '#define ArcherUSB_BulkComplete_ID' in header
print('DEXT bundle: exact Wi-Fi matching, DriverKit platform and framework links plus LOCALONLY transport boundary verified for ARM/Intel; not installed.')
