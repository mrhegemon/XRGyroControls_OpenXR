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
    public static var default_gaze: Int = 0
    public static var gaze_x1: Float = 0.0
    public static var gaze_y1: Float = 0.0
    public static var gaze_z1: Float = 0.0

    public static var gaze_x2: Float = 0.0
    public static var gaze_y2: Float = 0.0
    public static var gaze_z2: Float = 0.0

    public static var gaze_qx: Float = 0.0
    public static var gaze_qy: Float = 0.0
    public static var gaze_qz: Float = 0.0
    public static var gaze_qw: Float = 0.0

    public static var gaze_hud_x: Float = 0.0
    public static var gaze_hud_y: Float = 0.0
    public static var gaze_hud_z: Float = 0.0

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

    private func write<T>(_ of: T, at offset: Int) {
        let bytes: [UInt8] = withUnsafeBytes(of: of) { Array($0) }
        data[offset..<offset+bytes.count] = bytes[...]
    }
    
    init() {
        // Initialize the backing data to be the correct size, and zeroed out
        data = [UInt8](repeating: 0, count: MemoryLayout<IndigoHIDMessageStruct>.size)
        
        // Write it at offset 0x24, it's 64 bits
        write(mach_absolute_time(), at: 0x24)
        
        // Write the constant values
        data[0x18] = 0xA0
        data[0x1C] = 0x01
        data[0x20] = 0x01
        
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
        302, // manipulator
        1,
        300, // pose
        200, // digitaldial
        14,
        12,
        60,
        100, // keyboard
        50,
        301 // manipulator?
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

    public static func manipulator(_ test: Int, _ pose: sharedmem_data) -> IndigoHIDMessage {
        // HIDMessage - sending .manipulator with pinching (L=false, R=false), touching (L=false, R=false), matrix: simd_float4x4([[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]), gaze=RERay(origin: SIMD3<Float>(0.0, 0.0, 0.0), direction: SIMD3<Float>(-0.03173566, -0.04234394, -0.99859893), length: 10.0)
        
        let message = IndigoHIDMessage()
        
        // Could maybe be moved to an rcp::generate_synthetic_events hook? (RealitySystemSupport)
        message.write(302, at: 0x30)

        //print("Test is:", test)
        
        // TODO: Expose pinching, figure out all the different possibilities (what is 'touching' for?)
        message.write(3, at: 0x34) // size of the next 3 bytes, setting this high causes a stack overflow in backboardd lol
        
        // [3C]
        message.data[0x38] = pose.grab_val.0 > 0.75 ? 1 : 0 // pinch left
        message.data[0x39] = 2
        message.data[0x3A] = pose.left_touch_button != 0 ? 1 : 0 // ?

        // I
        message.write(3, at: 0x3B) // size of the next 3 bytes, setting this high causes a stack overflow in backboardd lol
        
        // [3C]
        message.data[0x3F] = pose.grab_val.1 > 0.75 ? 1 : 0 // pinch right
        message.data[0x40] = 1 // UInt8(test & 0xFF)
        message.data[0x41] = pose.right_touch_button != 0 ? 1 : 0 // ?

        // u8 0x42
        message.data[0x42] = 0
        message.write(Int16(0), at: 0x43)
        message.write(Int16(0), at: 0x45)

        if (pose.menu_button != 0 ) {
            if (pose.grip_val.0 > 0.75 && pose.grip_val.1 > 0.75)
            {
                default_gaze = 3
            }
            else if (pose.grip_val.1 > 0.75)
            {
                default_gaze = 2
            }
            else if (pose.grip_val.0 > 0.75)
            {
                default_gaze = 1
            }
            else {
                default_gaze = 0
            }
        }

        var which_gaze: Int = default_gaze

        if (pose.grip_val.0 > 0.75 && pose.grip_val.1 > 0.75)
        {
            which_gaze = 3
        }
        else if (pose.grip_val.1 > 0.75)
        {
            which_gaze = 2
        }
        else if (pose.grip_val.0 > 0.75)
        {
            which_gaze = 1
        }
        else {
            which_gaze = default_gaze
        }
        
        if (which_gaze == 3)
        {
            // Gaze fixed to center of vision
            gaze_x1 = pose.l_x
            gaze_y1 = pose.l_y
            gaze_z1 = pose.l_z

            gaze_x2 = -pose.l_view.8
            gaze_y2 = -pose.l_view.9
            gaze_z2 = -pose.l_view.10

            gaze_qx = pose.l_qx
            gaze_qy = pose.l_qy
            gaze_qz = pose.l_qz
            gaze_qw = pose.l_qw
        }
        else if (which_gaze == 2)
        {
            // Gaze fixed to right controller
            gaze_x1 = pose.r_controller.12
            gaze_y1 = pose.r_controller.13
            gaze_z1 = pose.r_controller.14

            gaze_x2 = -pose.r_controller.8
            gaze_y2 = -pose.r_controller.9
            gaze_z2 = -pose.r_controller.10

            gaze_qx = pose.r_controller_quat.0
            gaze_qy = pose.r_controller_quat.1
            gaze_qz = pose.r_controller_quat.2
            gaze_qw = pose.r_controller_quat.3
        }
        else if (which_gaze == 1)
        {
            // Gaze fixed to left controller
            gaze_x1 = pose.l_controller.12
            gaze_y1 = pose.l_controller.13
            gaze_z1 = pose.l_controller.14

            gaze_x2 = -pose.l_controller.8
            gaze_y2 = -pose.l_controller.9
            gaze_z2 = -pose.l_controller.10

            gaze_qx = pose.l_controller_quat.0
            gaze_qy = pose.l_controller_quat.1
            gaze_qz = pose.l_controller_quat.2
            gaze_qw = pose.l_controller_quat.3
        }
        else {
            // Gaze fixed to eye tracking
            gaze_x1 = pose.l_x
            gaze_y1 = pose.l_y
            gaze_z1 = pose.l_z

            gaze_x2 = pose.gaze_vec.0
            gaze_y2 = pose.gaze_vec.1
            gaze_z2 = pose.gaze_vec.2

            gaze_qx = pose.gaze_quat.0
            gaze_qy = pose.gaze_quat.1
            gaze_qz = pose.gaze_quat.2
            gaze_qw = pose.gaze_quat.3

            /*gaze_qx = pose.l_qx
            gaze_qy = pose.l_qy
            gaze_qz = pose.l_qz
            gaze_qw = pose.l_qw*/
        }

        // Gaze fixed to eye tracking
        // (I assume) gaze origin
        message.write(gaze_x1, at: 0x47)
        message.write(gaze_y1, at: 0x4B)
        message.write(gaze_z1, at: 0x4F)
        message.write(0.0,      at: 0x53)

        // XYZ ray offset from gaze origin
        message.write(gaze_x2, at: 0x57) // yaw
        message.write(gaze_y2, at: 0x5B) // pitch pose.l_ep
        message.write(gaze_z2, at: 0x5F) // roll pose.l_er
        message.write(1.0, at: 0x63)

        //gaze_x1 += gaze_x2 * 1.0
        //gaze_y1 += gaze_y2 * 1.0
        //gaze_z1 += gaze_z2 * 1.0

        // Touch ray origin
        message.write(pose.l_controller.12, at: 0x67)
        message.write(pose.l_controller.13,  at: 0x6B)
        message.write(pose.l_controller.14,  at: 0x6F)
        message.write(pose.l_controller.15,   at: 0x73)
        
        // Touch ray XYZ end offset from origin
        message.write(-pose.l_controller.8,   at: 0x77)
        message.write(-pose.l_controller.9,   at: 0x7B)
        message.write(-pose.l_controller.10,   at: 0x7F)
        message.write(pose.l_controller.11,   at: 0x83)
        
        // Pinch ray origin
        message.write(pose.r_controller.12, at: 0x87)
        message.write(pose.r_controller.13,  at: 0x8B)
        message.write(pose.r_controller.14,  at: 0x8F)
        message.write(pose.r_controller.15,   at: 0x93)
        
        // Pinch ray XYZ end offset from origin
        message.write(-pose.r_controller.8,   at: 0x97)
        message.write(-pose.r_controller.9,   at: 0x9B)
        message.write(-pose.r_controller.10,   at: 0x9F)
        message.write(pose.r_controller.11,   at: 0xA3)
        
        return message
    }

    public static func pose(_ x: Float, _ y: Float, _ z: Float, _ qx: Float, _ qy: Float, _ qz: Float, _ qw: Float) -> IndigoHIDMessage {
        let message = IndigoHIDMessage()

        message.write(300, at: 0x30)

        message.write(x, at: 0x54)
        message.write(y, at: 0x58)
        message.write(z, at: 0x5C)
        
        message.write(qx, at: 0x64)
        message.write(qy, at: 0x68)
        message.write(qz, at: 0x6C)
        message.write(qw, at: 0x70)

        return message
    }

    public static func digitaldial(_ dial: Double) -> IndigoHIDMessage {
        let message = IndigoHIDMessage()

        message.write(0x06, at:0x20)

        message.write(0x10, at:0x2C)
        message.write(0, at:0x34)
        message.write(0, at:0x38)
        message.write(dial, at:0x3C)
        message.write(0, at:0x44)
        message.write(0, at:0x48)
        message.write(200, at:0x4C)

        return message
    }

    public static func digitalcrown(_ val: Double) -> IndigoHIDMessage {
        let message = IndigoHIDMessage()

        message.write(0x06, at:0x20)

        message.write(0x10, at:0x2C)
        message.write(0, at:0x34)
        message.write(0, at:0x38)
        message.write(val, at:0x3C)
        message.write(0, at:0x44)
        message.write(0, at:0x48)
        message.write(52, at:0x4C)

        return message
    }

    public static func keyboard(_ val1: Int, _ val2: Int) -> IndigoHIDMessage {
        let message = IndigoHIDMessage()

        let keyboard_type = my_LMGetKbdType()//IndigoHIDGetKeyboardType();
        message.write(0x02, at:0x20)

        message.write(10000, at:0x30)
        message.write(val1, at:0x34) // keycode?
        message.write(100, at:0x38) // target
        message.write(val2, at:0x3C) // HIDButtonOp
        message.write(keyboard_type, at:0x40)
        message.write(0, at:0x44)
        message.write(0, at:0x48)
        message.write(52, at:0x4C)

        return message
    }
}
