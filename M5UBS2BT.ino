#include <M5Stack.h>
#include "config.h"

#include <esp_log.h>
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif
// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <string>
#include <vector>
#include <SPI.h>
#include <Ticker.h>
#include "sdkconfig.h"

#include <hidboot.h>
#include <usbhub.h>
#include "HIDKeyboardTypes.h"

#include "Task.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"


static char LOG_TAG[] = "SampleHIDDevice";
const char *BT_NAME = M5Stack_NAME;

static BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
bool isConnected = false;

class KeyTask;
std::vector<KeyTask *> tasks;

extern void DisplayStatusText(const char *text);
extern void DisplayConnectionText();
extern void SendKey(uint8_t mod, uint8_t key);

#ifdef LONG_PRESS
extern void KeepSendingKey();
#endif 

#ifdef LONG_PRESS
class KeyEvent {
 public:
  bool isPressing;
  int8_t count;
  int8_t mod;
  int8_t key;

 public:
  KeyEvent(){
    release();
  }
  
  void release() {
    isPressing = false;
    count = 0;
    this->mod = 0;
    this->key = 0;
  }

  void set(int8_t mod, int8_t key) {
    isPressing = true;
    count = 0;
    this->mod = mod;
    this->key = key;
  }
};
#endif //LONG_PRESS

#ifdef LONG_PRESS
static KeyEvent *pressKeyEvent = new KeyEvent();
#endif //LONG_PRESS


class KeyTask : public Task {
  private:
  KEYMAP payload[256];
  int length = 0;
  
  public:
  KeyTask(const char *text) {
    this->setString(text);
  }
  
  KeyTask(const KEYMAP *payload, int length) {
    this->setKeys(payload, length);
  }

  KeyTask() {
    this->length = 0;
  }

  void setString(const char *text) {
    this->length = 0;
    const char *pointer = text;
    while(*pointer){
        KEYMAP map = keymap[(uint8_t)*pointer];
        this->payload[this->length++] = map;
        pointer++;
    }
  }
  
  void setKeys(const KEYMAP *payload, int length) {
    int realLen = min(256, length);
    for (int i=0 ; i<realLen ; i++) {
      this->payload[i] = payload[i];
    }
    this->length = realLen;
  }

  void setKey(const KEYMAP payload) {
    this->payload[0] = payload;
    this->length = 1;
  }

  void deleteMe() {
    vTaskDelete(NULL);
  }
  
  private:
  void run(void*){
      ESP_LOGD(LOG_TAG, "sending keys.");
      for(int i=0 ; i<this->length ; i++) {
        KEYMAP map = this->payload[i];
        /*
         * simulate keydown, we can send up to 6 keys
         */
        uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
        input->setValue(a,sizeof(a));
        input->notify();

        /*
         * simulate keyup
         */
        uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
        input->setValue(v, sizeof(v));
        input->notify();

        vTaskDelay(100/portTICK_PERIOD_MS);
      }
      ESP_LOGD(LOG_TAG, "sent keys.");
  }
};

// class MouseRptParser : public MouseReportParser
// {
//   protected:
//     void OnMouseMove(MOUSEINFO *mi);
//     void OnLeftButtonUp(MOUSEINFO *mi);
//     void OnLeftButtonDown(MOUSEINFO *mi);
//     void OnRightButtonUp(MOUSEINFO *mi);
//     void OnRightButtonDown(MOUSEINFO *mi);
//     void OnMiddleButtonUp(MOUSEINFO *mi);
//     void OnMiddleButtonDown(MOUSEINFO *mi);
// };
// void MouseRptParser::OnMouseMove(MOUSEINFO *mi)
// {
//   Serial.print("dx=");
//   Serial.print(mi->dX, DEC);
//   Serial.print(" dy=");
//   Serial.println(mi->dY, DEC);
// };
// void MouseRptParser::OnLeftButtonUp  (MOUSEINFO *mi)
// {
//   Serial.println("L Butt Up");
// };
// void MouseRptParser::OnLeftButtonDown (MOUSEINFO *mi)
// {
//   Serial.println("L Butt Dn");
// };
// void MouseRptParser::OnRightButtonUp  (MOUSEINFO *mi)
// {
//   Serial.println("R Butt Up");
// };
// void MouseRptParser::OnRightButtonDown  (MOUSEINFO *mi)
// {
//   Serial.println("R Butt Dn");
// };
// void MouseRptParser::OnMiddleButtonUp (MOUSEINFO *mi)
// {
//   Serial.println("M Butt Up");
// };
// void MouseRptParser::OnMiddleButtonDown (MOUSEINFO *mi)
// {
//   Serial.println("M Butt Dn");
// };


class KbdRptParser : public KeyboardReportParser
{
    void PrintKey(uint8_t mod, uint8_t key);

  public:
    bool controlEventFlg = false;

  protected:
    void OnControlKeysChanged(uint8_t before, uint8_t after);
    void OnKeyDown  (uint8_t mod, uint8_t key);
    void OnKeyUp  (uint8_t mod, uint8_t key);
    void OnKeyPressed(uint8_t key);    
};

void KbdRptParser::PrintKey(uint8_t m, uint8_t key)
{
  MODIFIERKEYS mod;
  *((uint8_t*)&mod) = m;
  Serial.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
  Serial.print((mod.bmLeftShift  == 1) ? "S" : " ");
  Serial.print((mod.bmLeftAlt    == 1) ? "A" : " ");
  Serial.print((mod.bmLeftGUI    == 1) ? "G" : " ");

  Serial.print(" >");
  PrintHex<uint8_t>(key, 0x80);
  Serial.print("< ");

  Serial.print((mod.bmRightCtrl   == 1) ? "C" : " ");
  Serial.print((mod.bmRightShift  == 1) ? "S" : " ");
  Serial.print((mod.bmRightAlt    == 1) ? "A" : " ");
  Serial.println((mod.bmRightGUI    == 1) ? "G" : " ");
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  Serial.print("DN ");

  SendKey(mod, key);
  this->controlEventFlg = false;
#ifdef LONG_PRESS
  pressKeyEvent->set(mod, key);
#endif //LONG_PRESS
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {

  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;


  if(before == 0 && after != 0) {
    this->controlEventFlg = true;
  }


  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
    Serial.println("LeftCtrl changed");
  }
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
    Serial.println("LeftShift changed");
  }
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
    Serial.println("LeftAlt changed");
  }
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
    Serial.println("LeftGUI changed");
    if(this->controlEventFlg) {
      if(after == 0 && before == 8 ) { //release
          //TODO: WA
          KEYMAP payload[1];
          payload[0] = {0x29, KEY_CTRL};
          KeyTask *task = new KeyTask(payload, 1);
          task->start();
      }
    }
  }

  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
    Serial.println("RightCtrl changed");
  }
  if (beforeMod.bmRightShift != afterMod.bmRightShift) {
    Serial.println("RightShift changed");
  }
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
    Serial.println("RightAlt changed");
  }
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
    Serial.println("RightGUI changed");
  }

}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
  Serial.print("UP ");
  PrintKey(mod, key);
#ifdef LONG_PRESS
  pressKeyEvent->release();
#endif //LONG_PRESS
}

void KbdRptParser::OnKeyPressed(uint8_t key)
{
  Serial.print("ASCII: ");
  Serial.println((char)key);
};


class BLEOutputCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* me){
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    ESP_LOGI(LOG_TAG, "special keys: %d", *value);
  }
};

class BLECallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    BLE2902* desc;
    desc = (BLE2902*) input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
    isConnected = true;
    DisplayConnectionText();
    ESP_LOGD(LOG_TAG, "diaplay connection");
  }

  void onDisconnect(BLEServer* pServer){
    isConnected = false;
    ESP_LOGD(LOG_TAG, "diaplay connection");
    DisplayConnectionText();

     for(int i=0 ; i<tasks.size(); i++) {
       KeyTask *task = tasks[i];
       task->stop();
       task->deleteMe();
       //delete task;
     }
     tasks.clear();
  }
};

class MainBLEServer: public Task {
   BLEServer *pServer;

public:
   BLEServer *getServer() {
    return pServer;
   }
  
  void run(void *data) {
    ESP_LOGD(LOG_TAG, "Starting BLE work!");

    BLEDevice::init(BT_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BLECallbacks());

    /*
     * Instantiate hid device
     */
    hid = new BLEHIDDevice(pServer);


    input = hid->inputReport(1); // <-- input REPORTID from report map
    output = hid->outputReport(1); // <-- output REPORTID from report map

    output->setCallbacks(new BLEOutputCallbacks());

    /*
     * Set manufacturer name (OPTIONAL)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
     */
    std::string name = "esp-community";
    hid->manufacturer()->setValue(name);

    /*
     * Set pnp parameters (MANDATORY)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
     */

    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);

    /*
     * Set hid informations (MANDATORY)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
     */
    hid->hidInfo(0x00,0x01);


    /*
     * Keyboard
     */
    const uint8_t reportMap[] = {
      USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
      USAGE(1),           0x06,       // Keyboard
      COLLECTION(1),      0x01,       // Application
      REPORT_ID(1),   0x01,   // REPORTID
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0xE0,
      USAGE_MAXIMUM(1),   0xE7,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
      REPORT_COUNT(1),    0x08,
      HIDINPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
      REPORT_SIZE(1),     0x08,
      HIDINPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
      REPORT_SIZE(1),     0x01,
      USAGE_PAGE(1),      0x08,       //   LEDs
      USAGE_MINIMUM(1),   0x01,       //   Num Lock
      USAGE_MAXIMUM(1),   0x05,       //   Kana
      HIDOUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
      REPORT_SIZE(1),     0x03,
      HIDOUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
      REPORT_SIZE(1),     0x08,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0x00,
      USAGE_MAXIMUM(1),   0x65,
      HIDINPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      END_COLLECTION(0)
    };
    /*
     * Set report map (here is initialized device driver on client side) (MANDATORY)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
     */
    hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));

    /*
     * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
     * Also before we start, if we want to provide battery info, we need to prepare battery service.
     * We can setup characteristics authorization
     */
    hid->startServices();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    ESP_LOGD(LOG_TAG, "Wait a bit ...");
    delay(5000);

    /*
     * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
     */
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

    ESP_LOGD(LOG_TAG, "Advertising started!");
    delay(1000000);
  }
};

MainBLEServer* pMainBleServer = NULL;
void StartBLEServer(void)
{
  pMainBleServer = new MainBLEServer();
  pMainBleServer->setStackSize(20000);
  pMainBleServer->start();

}

USB     Usb;
USBHub     Hub(&Usb);

HIDBoot < USB_HID_PROTOCOL_KEYBOARD | USB_HID_PROTOCOL_MOUSE > HidComposite(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);
// HIDBoot<USB_HID_PROTOCOL_MOUSE>    HidMouse(&Usb);

KbdRptParser KbdPrs;
// MouseRptParser MousePrs;

#ifdef LONG_PRESS
Ticker ticker;
#endif //LONG_PRESS

void setup()
{
  Serial.begin(SERIAL_SPEED);
  M5.begin();                   // M5STACK INITIALIZE  
  M5.Lcd.setBrightness(20);    // BRIGHTNESS = MAX 255
  M5.Lcd.fillScreen(BLACK);     // CLEAR SCREEN
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);

  DisplayStatusText("Initializing...");
  DisplayConnectionText();
  StartBLEServer();
  
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  delay( 200 );

  HidComposite.SetReportParser(0, &KbdPrs);
  // HidComposite.SetReportParser(1, &MousePrs);
  HidKeyboard.SetReportParser(0, &KbdPrs);
  // HidMouse.SetReportParser(0, &MousePrs);
  DisplayStatusText("Initialized");

#ifdef LONG_PRESS
  ticker.attach_ms(INTERVAL, KeepSendingKey);
#endif//LONG_PRESS
}

void loop()
{
  Usb.Task();
  for(int i=0 ; i<tasks.size(); i++) {
    KeyTask *task = tasks[i];
    task->start();
  }
  tasks.clear();

#ifdef PASS_UNLOCK
  if (isConnected && M5.BtnB.wasPressed()) {
    KeyTask *passTask = new KeyTask(PASSWORD);
    passTask->start();
  }
#endif //PASS_UNLOCK

  M5.update();
}


void DisplayStatusText(const char *text) {
  int yStart = 100;
  int height = 50;
  M5.Lcd.setCursor(0, yStart);
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.printf(text);
}

void DisplayConnectionText() {
  int yStart = 0;
  int height = 50;
  M5.Lcd.setCursor(0, yStart);
  const char *text = isConnected ? "Connected" : "Not connected.";
  std::string str = (std::string)BT_NAME + "\n" + (std::string)text;
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.printf(str.c_str());
}

const int8_t MAX_BUF_SIZE =10;
int8_t current = 0;
KeyTask task_buf[10];
KEYMAP payload_buf[10];

void SendKey(uint8_t mod, uint8_t key) {
  payload_buf[current] = {key, mod};
  task_buf[current].setKey(payload_buf[current]);
    
  tasks.push_back(&task_buf[current]);
  current++;
  if(current >= MAX_BUF_SIZE) {
    current = 0;
  }
}


#ifdef LONG_PRESS
void KeepSendingKey() {
  if(pressKeyEvent->count < START_COUNT) {
    delay(WAIT);
    pressKeyEvent->count++;
    return;
  }
  if(pressKeyEvent->isPressing) {
    SendKey(pressKeyEvent->mod, pressKeyEvent->key);
  }
};
#endif //LONG_PRESS


