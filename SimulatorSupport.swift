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
        var asdf: Int = 0
        while (true) {
            DispatchQueue.main.async {
                self.openxr_wrapper.loop()
            }
            if(self.openxr_wrapper.done()) { break }

            let pose = self.openxr_wrapper.get_data()
            
            hid_client.send(message: IndigoHIDMessage.pose(pose.x, pose.y, pose.z, pose.qx, pose.qy, pose.qz, pose.qw).as_struct())
            hid_client.send(message: IndigoHIDMessage.manipulator(asdf, pose.x, pose.y, pose.z, pose.qx, pose.qy, pose.qz, pose.qw).as_struct())

            asdf += 1
            if (asdf > 255) {
                asdf = 0
            }

            Thread.sleep(forTimeInterval: 0.032)
            //Thread.sleep(forTimeInterval: 0.5)
        }
        print("OpenXR thread end!")
        self.openxr_wrapper.cleanup()
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
