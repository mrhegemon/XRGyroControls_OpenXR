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

        //let thread = MyThread(self.openxr_wrapper)
        //thread.start()

        //Thread.detachNewThreadSelector(#selector(openxr_thread), toTarget: self, with: 1)
        /*DispatchQueue.main.async { 
          self.openxr_thread()
        }*/

        DispatchQueue.global().async {
            self.openxr_thread()
        }

        
        //var cnt = 0
        // Schedule a HID message to be sent every 5 seconds
        //Timer.scheduledTimer(withTimeInterval: 0.01, repeats: true) { timer in
        //    cnt += 1
            //self.send_test_message(cnt)
            //self.openxr_wrapper.loop()
        //}
    
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
            // Should create a very slow rise
            message.pose(x: pose.x, y: pose.y, z: pose.z, pitch: pose.pitch, yaw: pose.yaw, roll: pose.roll)
            hid_client.send(message: message.as_struct())

            Thread.sleep(forTimeInterval: 0.1)
        }
        print("OpenXR thread end!")
        self.openxr_wrapper.cleanup()
    }
    
    func send_test_message(_ cnt: Int) {
        //os_log("XRGyroControls: Sending HID message")
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
