//
//  ViewController.swift
//  MDAP Uploader
//
//  Created by Jesus Berian on 18/03/2019.
//  Copyright © 2019 Jesus Berian. All rights reserved.
//

import UIKit
import CommonCrypto
import Foundation

class ViewController: UIViewController, mdapBLEInterfaceDelegate {
    
    var APInterface = mdapBLEInterface.init()
    var lastBG: Int = 0
    var lastReservoir: Double?
    var lastPumpIOB: Double?
    var lastPumpBattery: Double?
    var lastUploadDate: Date?
    var backfillDateInit: Date?
    var backfillDateEnd: Date?
    var backfillTimeOffset: Int?
    
    @IBOutlet weak var StateText: UITextField!
    @IBOutlet weak var LastConnectionText: UITextField!
    @IBOutlet weak var InitiatedText: UITextField!
    @IBOutlet weak var LastBLERxText: UITextField!
    @IBOutlet weak var LastUpdateText: UITextField!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.
        APInterface.set(delegate: self)
        
        updateInitiatedText()
        getLastSGVUploaded()
    }
    
    override func encodeRestorableState(with coder: NSCoder) {
        
        APInterface.encodeRestorableState(with: coder)
        super.encodeRestorableState(with: coder)
    }
    
    override func decodeRestorableState(with coder: NSCoder) {
        
        APInterface.decodeRestorableState(with: coder)
        super.decodeRestorableState(with: coder)
    }
    
    override func applicationFinishedRestoringState() {
        print("Restoration finished.")
    }
    
    func updateInitiatedText () {
        var info = String.init(stringLiteral: "Initiated: ")
        let formatter = DateFormatter()
        formatter.dateFormat = "dd-MM-yyyy HH:mm:ss"
        info.append(contentsOf: formatter.string(from: NSDate() as Date))
        InitiatedText.text = info
    }
    
    func mdapBLE(_ interface: mdapBLEInterface, status changedStatusTo: mdapBLEState) {
        if (changedStatusTo == mdapBLEState.connected) {
            StateText.text = "Artificial Pancreas: connected"
            print("mDrip connection state change: connected")
            
            var info = String.init(stringLiteral: "Last Connection: ")
            let formatter = DateFormatter()
            formatter.dateFormat = "dd-MM-yyyy HH:mm:ss"
            info.append(contentsOf: formatter.string(from: NSDate() as Date))
            LastConnectionText.text = info
        } else {
            StateText.text = "Artificial Pancreas: disconnected"
            print("mDrip connection state change: disconnected")
        }
    }
    
    func updateLastDataUpdateText () {
        var info = String.init(stringLiteral: "Last Data Update: ")
        let formatter = DateFormatter()
        formatter.dateFormat = "dd-MM-yyyy HH:mm:ss"
        info.append(contentsOf: formatter.string(from: NSDate() as Date))
        LastUpdateText.text = info
    }
    
    func mdapBLE(_ interface: mdapBLEInterface, numBytes didReceiveBytes: Int) {
        var info = String.init(stringLiteral: "Last BLE Data Rx: ")
        let formatter = DateFormatter()
        formatter.dateFormat = "dd-MM-yyyy HH:mm:ss"
        info.append(contentsOf: formatter.string(from: NSDate() as Date))
        LastBLERxText.text = info
    }
        
    func mdapBLE(_ interface: mdapBLEInterface, message didReceiveMessage: Data) {
        print("mDRIP Received message: \(String(describing: didReceiveMessage))")
        
        let messageLength = didReceiveMessage.count
        var messageBytes = [UInt8](repeating: 0, count: messageLength)
        didReceiveMessage.copyBytes(to: &messageBytes, count: messageLength)
        
        
        if ((messageBytes[0]&0x80 == 0x00) && (messageLength > 1)) {
            updateLastDataUpdateText ()
            if (messageBytes[1] == 0x01) { // Sensor Update
                let sourceFlags = messageBytes[2];
                if ((sourceFlags & 0x02) == 0x02) {
                    let timeOffset = Int(messageBytes[3])*256 + Int(messageBytes[4])
                    let historySgv = Int(messageBytes[5])*256 + Int(messageBytes[6])
                    let historyRawSgv = Int(messageBytes[7])*256 + Int(messageBytes[8])
                    
                    var delta: Int = 0
                    if (lastBG > 0) {
                        delta = historySgv - lastBG
                    }
                    
                    let pumpIOB = Double(UInt32(messageBytes[12])*256 + UInt32(messageBytes[13]) )*0.025
                    let pumpBattery = Double(UInt32(messageBytes[14])*256 + UInt32(messageBytes[15]))/1000.0
                    let pumpReservoir = Double(UInt32(messageBytes[17])*256 + UInt32(messageBytes[18]))/10.0
                    
                    postSensorData(withTimeOffset: timeOffset, sgv: historySgv, rawsgv: historyRawSgv, delta: delta)
                    postPumpInfoUpdate(pumpIOB: pumpIOB, pumpBattery: pumpBattery, pumpReservoir: pumpReservoir)
                    
                    if (self.lastUploadDate != nil) {
                        let timeGap = Date().timeIntervalSince(self.lastUploadDate!)
                        if (timeGap > 60*7.5) {
                            self.backfillDateEnd = Date().addingTimeInterval(TimeInterval(-timeOffset)/5.0)
                            self.backfillDateInit = self.lastUploadDate
                            self.backfillTimeOffset = timeOffset
                            APInterface.requestSGVBackfill()
                        }
                    }
                    
                    lastBG = historySgv
                    self.lastUploadDate = Date()
                    
                    if (self.lastReservoir != nil) {
                        if (pumpReservoir > self.lastReservoir!) {
                            postInsulinChange()
                        }
                    }
                    self.lastReservoir = pumpReservoir
                    
                    if (self.lastPumpIOB != nil) {
                        if (pumpIOB > self.lastPumpIOB!) {
                            let bolusAmmount = pumpIOB - self.lastPumpIOB!
                            postBolus(bolusAmmount)
                        }
                    }
                    self.lastPumpIOB = pumpIOB
                    
                    if (self.lastPumpBattery != nil) {
                        if ((pumpBattery > self.lastPumpBattery!) &&
                            (pumpBattery > 1.3) &&
                            (self.lastPumpBattery! < 1.25)) {
                            postBatteryChange()
                        }
                    }
                    self.lastPumpBattery = pumpBattery
                    
                } else {
                    lastBG = 0
                }
                
            } else if (messageBytes[1] == 0x02) { // BG Reading
                let BGReadingValue = Int(messageBytes[3])*256 + Int(messageBytes[4])
                print("BG Reading received: \(String(describing: BGReadingValue)) mg/dl.")
                postBGReading(BGReadingValue)
                
            } else if ((messageBytes[1] == 0x03) && (messageBytes[2] == 0x04)) { // Action: Set Temp Basal
                let tempBasalRate = Double(Int64(messageBytes[3])*256 + Int64(messageBytes[4]))*0.025
                let tempBasalMinutes = Int(messageBytes[5])*30
                print("Set temp basal received: \(String(describing: tempBasalRate)) U/h for \(String(describing: tempBasalMinutes)) minutes.")
                postTempBasal (tempBasalRate, forMinutes: tempBasalMinutes)
                
            } else if ((messageBytes[1] == 0x03) && (messageBytes[2] == 0x08)) { // Action: Cancel Temp Basal
                print("Cancel temp basal received.")
                postCancelTempBasal ()
                
            } else if (messageBytes[1] == 0x13) { // SGV Backfill
                print("SGV Backfill received.")
                
                let gap = self.backfillDateEnd?.timeIntervalSince(self.backfillDateInit!)
                var entriesLeft = Int(gap! / (60*5.0))
                if (entriesLeft > 15) {
                    entriesLeft = 15
                }
                
                var historySgv = 0
                for i in (1...entriesLeft) {
                    if ((messageBytes[3+2*i] & 0x80) != 0) {
                        historySgv = Int(messageBytes[3+2*i] & 0x07F)*256 + Int(messageBytes[3+2*i+1] & 0x0FF)
                        postSensorData(withTimeOffset: (i*5*60*5 + self.backfillTimeOffset!), sgv: historySgv, rawsgv: historySgv, delta: 0)
                    }
                }
                
                //APInterface.requestRawSGVBackfill()
                
            } else if (messageBytes[1] == 0x14) { // Raw SGV Backfill
                print("Raw SGV Backfill received.")
                
                /*
                 case 0x14:
                 i = 0;
                 tempMsg[i++] = 0x00;
                 tempMsg[i++] = message[0];
                 tempMsg[i++] = lastMinilinkSeqNum;
                 for (j=0;j<16;j++) {
                 tempMsg[i++] = ((historyRawSgv[j] >> 8) & 0x001) |
                 ((historyRawSgvValid[j] & 0x01) << 7) ;
                 tempMsg[i++] = (historyRawSgv[j] & 0x0FF);
                 }
                 tempMsg[0] = i-1;
                 break;
                 */
                
            } else if (messageBytes[1] == 0x35) { // Controller Status
                let expectedSgv = Int(messageBytes[2])*256 + Int(messageBytes[3])
                let expectedSgvWoIOB = Int(messageBytes[4])*256 + Int(messageBytes[5])
                let controllerState = Int(messageBytes[6])
                
                var tempBytes:Array<UInt8> = [messageBytes[7], messageBytes[8], messageBytes[9], messageBytes[10]]
                var tempFloat: Float = 0.0
                memcpy(&tempFloat,tempBytes, 4)
                let bolusIOB = tempFloat
                
                tempBytes = [messageBytes[11], messageBytes[12], messageBytes[13], messageBytes[14]]
                memcpy(&tempFloat,tempBytes, 4)
                let totalIOB = tempFloat
                
                tempBytes = [messageBytes[15], messageBytes[16], messageBytes[17], messageBytes[18]]
                memcpy(&tempFloat,tempBytes, 4)
                let tbdBasalIOB = tempFloat
                
                tempBytes = [messageBytes[19], messageBytes[20], messageBytes[21], messageBytes[22]]
                memcpy(&tempFloat,tempBytes, 4)
                let basalIOB = tempFloat
                
                tempBytes = [messageBytes[23], messageBytes[24], messageBytes[25], messageBytes[26]]
                memcpy(&tempFloat,tempBytes, 4)
                let instantIOB = tempFloat
                
                let timeCounterBolusSnooze = Int(messageBytes[27])*256 + Int(messageBytes[28])
                
                tempBytes = [messageBytes[29], messageBytes[30], messageBytes[31], messageBytes[32]]
                memcpy(&tempFloat,tempBytes, 4)
                let measuredBasal = tempFloat
                
                tempBytes = [messageBytes[33], messageBytes[34], messageBytes[35], messageBytes[36]]
                memcpy(&tempFloat,tempBytes, 4)
                let measuredSensitivity = tempFloat
                
                tempBytes = [messageBytes[37], messageBytes[38], messageBytes[39], messageBytes[40]]
                memcpy(&tempFloat,tempBytes, 4)
                let basalUsed = tempFloat
                
                let sensitivityUsed = Int(messageBytes[41])*256 + Int(messageBytes[42])
                
                tempBytes = [messageBytes[43], messageBytes[44], messageBytes[45], messageBytes[46]]
                memcpy(&tempFloat,tempBytes, 4)
                let tempBasalRate = tempFloat
                
                let targetSgv = Int(messageBytes[47])*256 + Int(messageBytes[48])
                
                postAPInfoUpdate(expectedIOB: expectedSgv, expectedSgvWoIOB: expectedSgvWoIOB, controllerState: controllerState, bolusIOB: bolusIOB, totalIOB: totalIOB,  tbdBasalIOB: tbdBasalIOB, basalIOB: basalIOB, instantIOB: instantIOB, timeCounterBolusSnooze: timeCounterBolusSnooze, measuredBasal: measuredBasal, measuredSensitivity: measuredSensitivity, basalUsed: basalUsed, sensitivityUsed: sensitivityUsed, tempBasalRate: tempBasalRate, targetSgv: targetSgv)
            }
            
        }
    }
    
    func postSensorData (withTimeOffset timeOffset: Int, sgv cooked: Int, rawsgv raw: Int, delta: Int) {
        print("Posting sensor data: \(String(describing: cooked)) \(String(describing: raw))")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        var yourDate = formatter.date(from: myString)
        // I should apply the TimeOffset!!!!
        yourDate = yourDate?.addingTimeInterval(TimeInterval(-timeOffset/5))
        
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        var directionText = ""
        if (delta >= 15) {
            directionText = "DoubleUp"
        } else if (delta >= 10) {
            directionText = "SingleUp"
        } else if (delta >= 5) {
            directionText = "FortyFiveUp"
        } else if (delta > -5) {
            directionText = "Flat"
        } else if (delta > -10) {
            directionText = "FortyFiveDown"
        } else if (delta > -15) {
            directionText = "SingleDown"
        } else {
            directionText = "DoubleDown"
        }
        
        let params = ["device":"mDrip Uploader iOS",
                      "dateString": myStringafd,
                      "sysTime": myStringafd,
                      "date":yourDate!.timeIntervalSince1970*1000,
                      "type":"sgv",
                      "sgv":cooked,
                      "filtered":cooked*1000,
                      "unfiltered":raw*1000,
                      "noise":1,
                      "rssi":100,
                      "direction":directionText
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "entries")
    }
    
    func postInsulinChange () {
        print("Posting Insulin change.")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let params = ["enteredBy":"mDrip Uploader iOS",
                      "created_at": myStringafd,
                      "eventType": "Insulin Change",
                      "reason":"",
                      "duration":0
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "treatments")
    }
    
    func postBatteryChange () {
        print("Posting battery change.")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let params = ["enteredBy":"mDrip Uploader iOS",
                      "created_at": myStringafd,
                      "eventType": "Pump Battery Change",
                      "reason":"",
                      "duration":0
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "treatments")
    }
    
    func postBGReading (_ reading: Int) {
        print("Posting BG reading: \(String(describing: reading))")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let params = ["device":"mDrip Uploader iOS",
                      "dateString": myStringafd,
                      "sysTime": myStringafd,
                      "date":yourDate!.timeIntervalSince1970*1000,
                      "type":"mbg",
                      "mbg":reading
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "entries")
    }
    
    func postBolus (_ insulin: Double) {
        print("Posting Bolus data.")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        
        let params = ["enteredBy":"mDrip Uploader iOS",
                      "timestamp": myStringafd,
                      "created_at": myStringafd,
                      "eventType": "Meal Bolus",
                      "carbs": 0,
                      "insulin": insulin,
                      "duration": 0,
                      "reason": ""
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "treatments")
    }
    
    func postTempBasal (_ rate: Double, forMinutes duration: Int) {
        print("Posting AP data.")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let rate = round(rate*40.0)/40.0
        
        let params = ["enteredBy":"mDrip Uploader iOS",
                      "timestamp": myStringafd,
                      "created_at": myStringafd,
                      "eventType": "Temp Basal",
                      "absolute": rate,
                      "duration": duration,
                      "reason": ""
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "treatments")
    }
    
    func postCancelTempBasal () {
        print("Posting AP data: Cancel Temporary Basal")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let params = ["enteredBy":"mDrip Uploader iOS",
                      "timestamp": myStringafd,
                      "created_at": myStringafd,
                      "eventType": "Temp Basal",
                      "duration": 0,
                      "reason": ""
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "treatments")
    }
    
    func postAPInfoUpdate(expectedIOB: Int, expectedSgvWoIOB: Int, controllerState: Int, bolusIOB: Float, totalIOB: Float,  tbdBasalIOB: Float, basalIOB: Float, instantIOB: Float, timeCounterBolusSnooze: Int, measuredBasal: Float, measuredSensitivity: Float, basalUsed: Float, sensitivityUsed: Int, tempBasalRate: Float, targetSgv: Int) {
        print("Posting AP Info Update")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let tempBasalRate = round(tempBasalRate*40.0)/40.0
        let totalIOB = round(totalIOB*40.0)/40.0
        let measuredBasal = round(measuredBasal*100.0)/100.0
        let measuredSensitivity = round(measuredSensitivity*100.0)/100.0
        let basalUsed = round(basalUsed*100.0)/100.0
        
        let loopDeviceString = "eBasal: \(String(describing: basalUsed)) | eSens: \(String(describing: sensitivityUsed)) | Target: \(String(describing: targetSgv))"
        
        let enactedParams = ["received":true,
                             "duration": 30,
                             "rate": tempBasalRate,
                             "timestamp": myStringafd
            ] as Dictionary<String, AnyObject>
        let iobParams = ["timestamp": myStringafd,
                         "iob": totalIOB
            ] as Dictionary<String, AnyObject>
        let loopParams = ["name":loopDeviceString,
                          "recommendedBolus": 0,
                          "enacted": enactedParams,
                          "version": "1.0",
                          "timestamp": myStringafd,
                          "iob": iobParams
            ] as Dictionary<String, AnyObject>
        let mDripParams = ["State": controllerState,
                           "IOB": totalIOB,
                           "ToBeDeliveredIOB": tbdBasalIOB,
                           "DeliveredBasalIOB": basalIOB,
                           "DeliveredBolusIOB": bolusIOB,
                           "InstantIOB": instantIOB,
                           "EstimatedBasal": measuredBasal,
                           "EstimatedSensitivity":
                        measuredSensitivity,
                           "Basal": basalUsed,
                           "Sensitivity": sensitivityUsed,
                           "Target": targetSgv
            ] as Dictionary<String, AnyObject>
        let params = ["date":yourDate!.timeIntervalSince1970*1000,
                      "created_at": myStringafd,
                      "device":"mDrip",
                      "reason":"",
                      "loop":loopParams,
                      "mDrip":mDripParams
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "deviceStatus")
    }

    func postPumpInfoUpdate(pumpIOB: Double, pumpBattery: Double, pumpReservoir: Double) {
        print("Posting Pump Info Update")
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        
        let myString = formatter.string(from: Date())
        let yourDate = formatter.date(from: myString)
        formatter.timeZone = TimeZone(identifier: "UTC")
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
        let myStringafd = formatter.string(from: yourDate!)
        
        let uploaderParams = ["timestamp": myStringafd,
                              "name": "iPhone",
                              "battery": 100
            ] as Dictionary<String, AnyObject>
        let batteryParams = ["voltage": pumpBattery,
                             "status": "normal"
            ] as Dictionary<String, AnyObject>
        let pumpParams = ["pumpID": "000000",
                          "battery": batteryParams,
                          "reservoir": pumpReservoir,
                          "clock":myStringafd
            ] as Dictionary<String, AnyObject>
        let params = ["created_at": myStringafd,
                      "device":"mDrip",
                      "uploader":uploaderParams,
                      "pump":pumpParams
            ] as Dictionary<String, AnyObject>
        postInformation(params, toEndPoint: "deviceStatus")
    }

    func postInformation (_ params: Dictionary<String, AnyObject>, toEndPoint endpoint: String) {
        let nightscoutURL: String = "https://jberian.herokuapp.com/api/v1/" + endpoint
        let apiSecret: String = "3292byj7815fwk"
        
        let NSurl = URL.init(string: nightscoutURL);
        var request : URLRequest = URLRequest(url: NSurl!)
        request.url = NSurl
        request.httpMethod = "POST"
        request.timeoutInterval = 30
        request.addValue("application/json", forHTTPHeaderField: "Content-Type")
        request.addValue("application/json", forHTTPHeaderField: "Accept")
        
        var digest = [UInt8](repeating: 0, count: Int(CC_SHA1_DIGEST_LENGTH))
        if let data = apiSecret.data(using: String.Encoding.utf8) {
            CC_SHA1(data.reversed().reversed(), CC_LONG(data.count), &digest)
            let digestString = digest.map { String(format: "%02hhx", $0)}
            request.addValue(digestString.joined(), forHTTPHeaderField: "api-secret")
        }
        
        let sendData = try! JSONSerialization.data(withJSONObject: params, options: [])
        
        let dataTask = URLSession.shared.uploadTask(with: request, from: sendData, completionHandler: { (data, response, error) in
            if error != nil {
                return
            }
            
            guard let httpResponse = response as? HTTPURLResponse else {
                return
            }
            
            if httpResponse.statusCode != 200 {
                return
            }
            
            guard let data = data, !data.isEmpty else {
                return
            }
            
            do {
                _ = try JSONSerialization.jsonObject(with: data, options: JSONSerialization.ReadingOptions())
                
            } catch {
                return
            }
        })
        dataTask.resume()
    }
    
    func getLastSGVUploaded ()
    {
        let endpoint: String = "entries/current.json"
        let nightscoutURL: String = "https://jberian.herokuapp.com/api/v1/" + endpoint
        let apiSecret: String = "3292byj7815fwk"
        
        let NSurl = URL.init(string: nightscoutURL);
        var request : URLRequest = URLRequest(url: NSurl!)
        var lastDate : String = "2019-01-01'T'00:00:00"
        request.url = NSurl
        request.httpMethod = "GET"
        request.timeoutInterval = 30
        
        var digest = [UInt8](repeating: 0, count: Int(CC_SHA1_DIGEST_LENGTH))
        if let data = apiSecret.data(using: String.Encoding.utf8) {
            CC_SHA1(data.reversed().reversed(), CC_LONG(data.count), &digest)
            let digestString = digest.map { String(format: "%02hhx", $0)}
            request.addValue(digestString.joined(), forHTTPHeaderField: "api-secret")
        }
        
        let dataTask = URLSession.shared.dataTask(with: request, completionHandler: { (data, response, error) in
            if error != nil {
                return
            }
            
            guard let httpResponse = response as? HTTPURLResponse else {
                return
            }
            
            if httpResponse.statusCode != 200 {
                return
            }
            
            guard let data = data, !data.isEmpty else {
                return
            }
            
            do {
                let lastDataUploaded = try JSONSerialization.jsonObject(with: data, options: JSONSerialization.ReadingOptions())
                guard let lastDataArray = lastDataUploaded as? [[String: Any]] else {
                    return
                }
                if lastDataArray.count > 0 {
                    lastDate = lastDataArray[0]["dateString"] as! String
                }
                
                let dateFormatter = DateFormatter()
                dateFormatter.timeZone = TimeZone(identifier: "UTC")
                dateFormatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSXXX"
                //according to date format your date string
                self.lastUploadDate = dateFormatter.date(from: lastDate)
                print("Last data upload occured on: \(String(describing: self.lastUploadDate)) ")
                
            } catch {
                return
            }
        })
        dataTask.resume()
        
        
    }
}

