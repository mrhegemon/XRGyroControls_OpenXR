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
        
        var cnt = 0
        // Schedule a HID message to be sent every 5 seconds
        Timer.scheduledTimer(withTimeInterval: 0.01, repeats: true) { timer in
            cnt += 1
            self.send_test_message(cnt)
        }
    
    }
    
    func send_test_message(_ cnt: Int) {
        os_log("Sending HID message")
        let message = IndigoHIDMessage()
        // Should create a very slow rise
        message.pose(x: 0.0, y: Float(cnt) / 1000, z: 0.0, pitch: 0.0, yaw: 0.0, roll: 0.0)
        hid_client.send(message: message.as_struct())
    }
    
    @objc func overlayView() -> NSView {
        return NSView()
    }
    
    @objc func toolbar() -> NSToolbar {
        return NSToolbar()
    }
}
