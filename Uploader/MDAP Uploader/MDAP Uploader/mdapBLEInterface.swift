//
//  mdapBLEInterface.swift
//  Medtronic 600 series controller
//
//  Created by Jesus Berian on 23/09/2018.
//  Copyright © 2018 Jesus Berian. All rights reserved.
//

import UIKit
import CoreBluetooth

enum mdapBLEState {
    case disconnected
    case connected
}

protocol mdapBLEInterfaceDelegate: class {
    func mdapBLE(_ interface: mdapBLEInterface, message didReceiveMessage: Data)
    func mdapBLE(_ interface: mdapBLEInterface, status changedStatusTo: mdapBLEState)
    func mdapBLE(_ interface: mdapBLEInterface, numBytes didReceiveBytes: Int)
}

class mdapBLEInterface: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    
    weak var delegate: mdapBLEInterfaceDelegate?
    var manager:CBCentralManager!
    var peripheral:CBPeripheral!
    var mdapReady: Bool = false
    var mdapWriteCharacteristic: CBCharacteristic? = nil
    var bluetoothDataRx = Data.init()
    var bluetoothDataRxTimer = Timer.init()
    
    let MDAP_SERVICE_UUID = CBUUID(string: "FFE0")
    let MDAP_CHARACTERISTIC_UUID = CBUUID(string: "FFE1")
    
    override init () {
        super.init()
        mdapReady = false
        delegate = nil
        if (manager == nil) {
            manager = CBCentralManager(delegate: self, queue: nil, options:[CBCentralManagerOptionRestoreIdentifierKey: "es.berian.MDAPUploader.centralManager"])
        }
    }
    
    func encodeRestorableState(with coder: NSCoder) {
        if let delegate = delegate {
            coder.encode(delegate, forKey: "BLEInterface.delegate")
        }
        
        if let mdapWriteCharacteristic = mdapWriteCharacteristic {
            coder.encode(mdapWriteCharacteristic, forKey: "BLEInterface.mdapWriteCharacteristic")
        }
    }
    
    func decodeRestorableState(with coder: NSCoder) {
        
        self.delegate = coder.decodeObject(forKey: "BLEInterface.delegate") as? mdapBLEInterfaceDelegate
        self.mdapWriteCharacteristic = coder.decodeObject(forKey: "BLEInterface.mdapWriteCharacteristic") as? CBCharacteristic
        print("Variables restored")
    }
    
    func set(delegate: mdapBLEInterfaceDelegate) {
        self.delegate = delegate
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("checking state")
        switch (central.state) {
        case .poweredOff:
            print("MDAP: CoreBluetooth BLE hardware is powered off")
            mdapReady = false
            
        case .poweredOn:
            print("MDAP: CoreBluetooth BLE hardware is powered on and ready")
            let connectedPeripherals = manager.retrieveConnectedPeripherals(withServices: [MDAP_SERVICE_UUID])
            if (connectedPeripherals.count == 0) {
                print("MDAP: Discovering devices")
                manager.scanForPeripherals(withServices: [MDAP_SERVICE_UUID], options:  nil)
                mdapReady = false
            } else {
                mdapReady = true
                delegate?.mdapBLE(self, status: .connected)
            }
            
        case .resetting:
            print("MDAP: CoreBluetooth BLE hardware is resetting")
            mdapReady = false
            
        case .unauthorized:
            print("MDAP: CoreBluetooth BLE state is unauthorized")
            mdapReady = false
            
        case .unknown:
            print("MDAP: CoreBluetooth BLE state is unknown");
            mdapReady = false
            
        case .unsupported:
            print("MDAP: CoreBluetooth BLE hardware is unsupported on this platform");
            mdapReady = false
        }
    }
    
    func centralManager(_ central: CBCentralManager,
                        didDiscover peripheral: CBPeripheral,
                        advertisementData: [String : Any],
                        rssi RSSI: NSNumber) {
        print("MDAP: Discovered \(String(describing: peripheral.name))")
        
        self.manager.stopScan()
        
        self.peripheral = peripheral
        self.peripheral.delegate = self
        
        manager.connect(peripheral, options: nil)
    }
    
    func centralManager(
        _ central: CBCentralManager,
        didConnect peripheral: CBPeripheral) {
        print("MDAP: Connected to \(String(describing: peripheral.name))")
        print("MDAP: Discovering services")
        peripheral.discoverServices(nil)
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        delegate?.mdapBLE(self, status: .disconnected)
        manager.connect(peripheral, options: nil)
        //central.scanForPeripherals(withServices: [MDAP_SERVICE_UUID], options: nil)
    }
    
    public func centralManager(_ central: CBCentralManager, willRestoreState dict: [String : Any]) {
        
        if let peripherals = dict[CBCentralManagerRestoredStatePeripheralsKey] as? [CBPeripheral] {
            peripherals.forEach { (awakedPeripheral) in
                guard let localName = awakedPeripheral.name,
                    localName == "DSD TECH" else { // Look for services!!
                        return
                }
                
                self.peripheral = awakedPeripheral
                self.peripheral.delegate = self
                self.peripheral.discoverServices(nil)
                
                print("MDAP: Recovered APP")
                
                centralManagerDidUpdateState(self.manager)
                
                //manager.connect(awakedPeripheral, options: nil)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral,
                    didDiscoverServices error: Error?) {
        for service in peripheral.services! {
            let thisService = service as CBService
            
            if service.uuid == MDAP_SERVICE_UUID {
                print("MDAP: Discovering characteristics")
                peripheral.discoverCharacteristics(
                    nil,
                    for: thisService
                )
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral,
                    didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        for characteristic in service.characteristics! {
            let thisCharacteristic = characteristic as CBCharacteristic
            
            switch (thisCharacteristic.uuid) {
            
            case MDAP_CHARACTERISTIC_UUID:
                print("MDAP: Setting up characteristic for writing")
                print("MDAP: Setting up notifications for characteristic")
                self.mdapWriteCharacteristic = thisCharacteristic
                self.peripheral.setNotifyValue(
                    true,
                    for: thisCharacteristic
                )
                mdapReady = true
                delegate?.mdapBLE(self, status: .connected)
            
            default:
                print("MDAP: Unexpected characteristic")
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral,
                    didUpdateValueFor characteristic: CBCharacteristic,
                    error: Error?) {
        var messageLength : Int
        var rxArrayLength : Int
        var theresSomethingElseToRx : Bool
        var messageReceived = Data.init()
        var tempArray : [UInt8] = [0,0,0,0,0,0,0,0]
        
        switch (characteristic.uuid) {
        case MDAP_CHARACTERISTIC_UUID:
            if (bluetoothDataRxTimer.isValid) {
                bluetoothDataRxTimer.invalidate()
            }
            bluetoothDataRx.append(characteristic.value!)
            rxArrayLength = bluetoothDataRx.count
            delegate?.mdapBLE(self, numBytes: bluetoothDataRx.count)
            theresSomethingElseToRx = true
            while ((rxArrayLength > 1) && theresSomethingElseToRx) {
                tempArray[0] = bluetoothDataRx.first!
                messageLength = Int(littleEndian: Data(bytes: tempArray).withUnsafeBytes { $0.pointee })
                if (rxArrayLength > messageLength) {
                    messageReceived.append(bluetoothDataRx.subdata(in: 0..<messageLength+1))
                    delegate?.mdapBLE(self, message: messageReceived.subdata(in: 0..<messageLength+1))
                    messageReceived.removeAll()
                    bluetoothDataRx.removeSubrange(0..<messageLength+1)
                    rxArrayLength = bluetoothDataRx.count
                } else {
                    theresSomethingElseToRx = false
                }
            }
            if (theresSomethingElseToRx == false) {
                // Start timeout timer
                bluetoothDataRxTimer = Timer.scheduledTimer(timeInterval: 1, target: self,   selector: (#selector(bluetoothDataRxTimeout)), userInfo: nil, repeats: false)
                
            }
        default:
            print("MDAP: Unhandled Characteristic UUID: \(characteristic.uuid)")
        }
    }
    
    @objc func bluetoothDataRxTimeout () {
        bluetoothDataRx.removeAll()
        print("MDAP: Bluetooth RX Timeout")
    }
    
    func isConnected () -> Bool {
        return(mdapReady)
    }
    
    func requestSGVBackfill () {
        var message = Data.init()
        message.append([0x13], count: 1)
        print("Requesting SGV Backfill")
        send(message)
    }
    
    func requestRawSGVBackfill () {
        var message = Data.init()
        message.append([0x14], count: 1)
        print("Requesting Raw SGV Backfill")
        send(message)
    }
    
    func send(_ message: Data) {
        let messageToTx = composeMessage(message)
        if (self.mdapWriteCharacteristic != nil) {
            self.peripheral.writeValue(messageToTx, for: self.mdapWriteCharacteristic!, type: CBCharacteristicWriteType.withoutResponse)
        } else {
            self.peripheral.discoverServices(nil)
        }
    }
    
    private func composeMessage (_ message: Data) -> Data{
        var outputArray: [UInt8] = Array.init(repeating: 0, count: message.count + 1)
        message.copyBytes(to: &outputArray[1], count: message.count)
        outputArray[0] = UInt8(message.count)
        let finalMessage = Data.init(bytes: outputArray, count: message.count + 1)
        return(finalMessage)
    }
    
}
