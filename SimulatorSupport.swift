import Foundation
import SimulatorKit
import CoreSimulator
import AppKit

import os

@objc class SimulatorSupport : NSObject, SimDeviceUserInterfacePlugin {
    
    private let device: SimDevice
    private let hid_client: SimDeviceLegacyHIDClient
    private let openxr_wrapper: OpenXRWrapper
        
    @objc init(with device: SimDevice) {
        self.device = device
        os_log("XRGyroControls: Initialized with device: \(device)")
        self.hid_client = try! SimDeviceLegacyHIDClient(device: device)
        self.openxr_wrapper = OpenXRWrapper()
        os_log("XRGyroControls: Initialized HID client")
        super.init()

        DispatchQueue.global().async {
            self.openxr_thread()
        }
    }

    @objc func openxr_thread() {
        Thread.sleep(forTimeInterval: 1.0)

        print("OpenXR thread start!")
        while (true) {
            DispatchQueue.main.async {
                self.openxr_wrapper.loop()
            }
            if(self.openxr_wrapper.done()) { break }

            let pose = self.openxr_wrapper.get_data()
            let message = IndigoHIDMessage()

            message.pose(x: pose.x, y: pose.y, z: pose.z, qx: pose.qx, qy: pose.qy, qz: pose.qz, qw: pose.qw)
            hid_client.send(message: message.as_struct())

            Thread.sleep(forTimeInterval: 0.032)
        }
        print("OpenXR thread end!")
        self.openxr_wrapper.cleanup()
    }
    
    func send_test_message(_ cnt: Int) {
        //os_log("XRGyroControls: Sending HID message")
        let message = IndigoHIDMessage()
        // Should create a very slow rise
        message.pose(x: 0.0, y: Float(cnt) / 1000, z: 0.0, qx: 0.0, qy: 0.0, qz: 0.0, qw: 1.0)
        hid_client.send(message: message.as_struct())
    }
    
    @objc func overlayView() -> NSView {
        return NSView()
    }
    
    @objc func toolbar() -> NSToolbar {
        return NSToolbar()
    }
}
