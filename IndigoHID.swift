//
//  IndigoHID.swift
//  XRGyroControls
//
//  Created by James Gill on 6/25/23.
//

import Foundation
import CoreSimulator

class IndigoHIDMessage {
    /// You must NOT change the length to anything other than 0xC0
    public var data: [UInt8] = []
    public var test: Int32 = 0
    
    public func as_struct() -> UnsafeMutablePointer<IndigoHIDMessageStruct> {
        //print("data: \(data)")
        // Make sure that the backing data is the correct size
        guard data.count == MemoryLayout<IndigoHIDMessageStruct>.size else {
            fatalError("IndigoHIDMessage backing data is not the correct size")
        }
        
        // Allocate a new struct and copy the bytes to it
        let ptr = UnsafeMutablePointer<IndigoHIDMessageStruct>.allocate(capacity: 1)
        ptr.initialize(to: data.withUnsafeBufferPointer { $0.baseAddress!.withMemoryRebound(to: IndigoHIDMessageStruct.self, capacity: 1) { $0.pointee } })
        return ptr
    
    }
    
    init() {
        // Initialize the backing data to be the correct size, and zeroed out
        data = [UInt8](repeating: 0, count: MemoryLayout<IndigoHIDMessageStruct>.size)
        
        // Set the timestamp to the current time
        let timestamp = mach_absolute_time()
        let timestamp_bytes = withUnsafeBytes(of: timestamp) { Array($0) }
        // Write it at offset 0x24, it's 64 bits
        data[0x24..<0x24+8] = timestamp_bytes[0..<8]
        
        // Write the constant values
        data[0x18] = 0xA0
        data[0x1C] = 0x01
        data[0x20] = 0x01
        
        data[0x30..<0x30+4] = [0x2C, 0x01, 0x00, 0x00] // Int32: 300
        //data[0x34..<0x34+4] = [0x2C, 0x01, 0x00, 0x00] // Int32: 300
        // another Int32 at 0x34?
    }
    
    private func write_float(_ float: Float, offset: Int) {
        let bytes: [UInt8] = withUnsafeBytes(of: float) { Array($0) }
        data[offset..<offset+bytes.count] = bytes[...]
    }

    private func write_int32(_ val: Int32, offset: Int) {
        let bytes: [UInt8] = withUnsafeBytes(of: val) { Array($0) }
        data[offset..<offset+bytes.count] = bytes[...]
    }

    //v44@0:8^{
    //    IndigoHIDMessageStruct={?=IIIIIi}IC
    //      [0{?=IQI(
    //         _event={_extended=IIQ(?={?=I[64c]}{?=I}{?=IC})}
    //                {_touch_event=IIIdddddIIIIIdddddI}
    //                {_pointer_event=dddII}
    //                {_velocity_event=IdddI}
    //                {_wheel_event=IdddIIII}
    //                {_translation_event=dddI}
    //                {_rotation_event=dddI}
    //                {_scale_event=dddI}
    //                {_dock_swipe_event=IIdddI}
    //                {_button_event=IIIIII}
    //                {_pointer_button_event=IIII}
    //                {_accelerometer_event=I[40C]}
    //                {_force_event=IdId}
    //                {_gamecontroller_event={_dpad=dddd}{_face=dddd}{_shoulder=dddd}{_joystick=dddd}}
    //                {_generic_vendor_defined_event=I}
    //                {_paloma_pose_event=II[28C]{_translation=fff}{_orientation=ffff}}
    //                {_paloma_collection_event=II[3C]I[3C]CSS{_origin_g=fff}{_orientation_g=ffff}{_translation_l=fff}{_orientation_l=ffff}{_translation_r=fff}{_orientation_r=ffff}}
    //                )cC[2C]Q}]
    //}16B24@28@?36
    //

    /*
    reason: 'Encountered HID event with unexpected target 0 not in known targets: (
        13,
        11,
        51,
        302,
        1,
        300,
        200,
        14,
        12,
        60,
        100,
        50,
        301
    )'
    *** First throw call stack:
    (
        0   CoreFoundation                      0x00000001804a4238 __exceptionPreprocess + 172
        1   libobjc.A.dylib                     0x00000001800830ac objc_exception_throw + 56
        2   Foundation                          0x0000000180cf0614 -[NSMutableDictionary(NSMutableDictionary) classForCoder] + 0
        3   SimulatorHID                        0x000000010550d404 -[SimHIDVirtualServiceManager serviceForIndigoHIDData:] + 476
*/

    // 300 - pose
    // 301 - raycast?
    // 302 - raycast?

    public func pose(x: Float, y: Float, z: Float, qx: Float, qy: Float, qz: Float, qw: Float) {
        write_int32(50, offset: 0x30)
        /*test += 1
        if (test > 310) {
            test = 0
        }*/

        write_float(x, offset: 0x54)
        write_float(y, offset: 0x58)
        write_float(z, offset: 0x5C)
        
        write_float(qx, offset: 0x64)
        write_float(qy, offset: 0x68)
        write_float(qz, offset: 0x6C)
        write_float(qw, offset: 0x70) // Not sure why, but this is important
    }
}
