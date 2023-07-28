import Foundation
import SimulatorKit
import CoreSimulator
import AppKit

import os

@objc class SimulatorSupport : NSObject, SimDeviceUserInterfacePlugin {
    
    private let device: SimDevice
    private let hid_client: SimDeviceLegacyHIDClient

    @objc init(with device: SimDevice) {
        self.device = device
        os_log("XRGyroControls: Initialized with device: \(device)")
        self.hid_client = try! SimDeviceLegacyHIDClient(device: device)
        os_log("XRGyroControls: Initialized HID client")
        super.init()

        DispatchQueue.global().async {
            self.openxr_thread()
        }
    }

    @objc func openxr_thread() {
        Thread.sleep(forTimeInterval: 1.0)

        //print("OpenXR thread start!")

        ObjCBridge_Startup();

        var home_x: Float = 0.0
        var home_y: Float = 0.0
        var home_z: Float = 0.0
        var home_qx: Float = 0.0
        var home_qy: Float = 0.0
        var home_qz: Float = 0.0
        var home_qw: Float = 1.0

        var asdf: Int = 0
        var last_system_button: Int32 = 0;
        while (true) {
            let pose = ObjCBridge_Loop().pointee;
            //let pose = self.openxr_wrapper.get_data()

            if (pose.system_button != 0 && pose.system_button != last_system_button) {
                ObjCBridge_HomeButtonPress()

                home_x = IndigoHIDMessage.gaze_x1
                home_y = IndigoHIDMessage.gaze_y1
                home_z = IndigoHIDMessage.gaze_z1
                home_qx = IndigoHIDMessage.gaze_qx
                home_qy = IndigoHIDMessage.gaze_qy
                home_qz = IndigoHIDMessage.gaze_qz
                home_qw = IndigoHIDMessage.gaze_qw
            }
            last_system_button = pose.system_button
            
            //hid_client.send(message: IndigoHIDMessage.pose(home_x, home_y, home_z, home_qx, home_qy, home_qz, home_qw).as_struct())
            //hid_client.send(message: IndigoHIDMessage.pose(IndigoHIDMessage.gaze_x1+IndigoHIDMessage.gaze_x2, IndigoHIDMessage.gaze_y1+IndigoHIDMessage.gaze_y2, IndigoHIDMessage.gaze_z1+IndigoHIDMessage.gaze_z2, IndigoHIDMessage.gaze_qx, IndigoHIDMessage.gaze_qy, IndigoHIDMessage.gaze_qz, IndigoHIDMessage.gaze_qw).as_struct())
            //hid_client.send(message: IndigoHIDMessage.pose(IndigoHIDMessage.gaze_x1, IndigoHIDMessage.gaze_y1, IndigoHIDMessage.gaze_z1, IndigoHIDMessage.gaze_qx, IndigoHIDMessage.gaze_qy, IndigoHIDMessage.gaze_qz, IndigoHIDMessage.gaze_qw).as_struct())
            //hid_client.send(message: IndigoHIDMessage.pose(IndigoHIDMessage.gaze_x1, IndigoHIDMessage.gaze_y1, IndigoHIDMessage.gaze_z1, 0.0, 0.0, 0.0, 1.0).as_struct())
            //hid_client.send(message: IndigoHIDMessage.pose(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0).as_struct())
            //hid_client.send(message: IndigoHIDMessage.pose(pose.l_x, pose.l_y, pose.l_z, 0.0, 0.0, 0.0, 1.0).as_struct())
            hid_client.send(message: IndigoHIDMessage.pose(pose.l_x, pose.l_y, pose.l_z, pose.l_qx, pose.l_qy, pose.l_qz, pose.l_qw).as_struct())
            hid_client.send(message: IndigoHIDMessage.manipulator(asdf, pose).as_struct())
            //hid_client.send(message: IndigoHIDMessage.digitaldial(1.0).as_struct())
            if (asdf < 60) {
                //hid_client.send(message: IndigoHIDMessage.digitaldial(1.0).as_struct())
                //hid_client.send(message: IndigoHIDMessage.keyboard(4, 2).as_struct())
                //ObjCBridge_HomeButtonPress();
            }
            else {
                //hid_client.send(message: IndigoHIDMessage.digitaldial(-1.0).as_struct())
                //hid_client.send(message: IndigoHIDMessage.keyboard(4, 3).as_struct())
            }

            asdf += 1
            if (asdf > 120) {
                asdf = 0
                //ObjCBridge_HomeButtonPressUp();
            }

            Thread.sleep(forTimeInterval: 0.004)
            //Thread.sleep(forTimeInterval: 0.5)
        }

        ObjCBridge_Shutdown();
        //print("OpenXR thread end!")
        //self.openxr_wrapper.cleanup()
    }
    
    func send_test_message(_ cnt: Int) {
        hid_client.send(message: IndigoHIDMessage.pose(0.0, Float(cnt) / 1000, 0.0, 0.0, 0.0, 0.0, 1.0).as_struct())
    }
    
    @objc func overlayView() -> NSView {
        return NSView()
    }
    
    @objc func toolbar() -> NSToolbar {
        return NSToolbar()
    }
}
