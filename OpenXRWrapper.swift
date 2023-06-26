//
//  OpenXRWrapper.swift
//  XRGyroControls
//
//  Created by Max Thomas on 6/26/23.
//

import Foundation
//import Main

@objc class OpenXRWrapper : NSObject
{

    @objc override init() {
        openxr_main();
    }

    @objc func loop() -> Bool {
        openxr_loop() == 1
    }

    @objc func done() -> Bool {
        openxr_done() == 1
    }

    @objc func cleanup() {
        openxr_cleanup();
    }

    @objc func get_data() -> openxr_headset_data {
        var data = openxr_headset_data(x: 0.0, y: 0.0, z: 0.0, pitch: 0.0, yaw: 0.0, roll: 0.0);

        openxr_headset_get_data(&data);

        data.y -= 2.0

        return data
    }
}
