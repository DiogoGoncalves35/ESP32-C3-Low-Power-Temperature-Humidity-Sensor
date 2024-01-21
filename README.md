# ESP32-C3-Low-Power-Temperature-Humidity-Sensor


![Kicad](https://img.shields.io/badge/KiCad-314CB0.svg?style=for-the-badge&logo=KiCad&logoColor=white) ![Espressif](https://img.shields.io/badge/Espressif-E7352C.svg?style=for-the-badge&logo=Espressif&logoColor=white) ![PlatformIO](https://img.shields.io/badge/PlatformIO-F5822A.svg?style=for-the-badge&logo=PlatformIO&logoColor=white)



This board is designed arround SHT31 sensor and ESP32-C3 microcontroller. It's a low power board, arround 6 µA during deep sleep, it's super efficient during communication, makes it perfect to be powered with a Li-Ion battery. The next image shows version 2.0.

<img src="/Images/Board.png"> <img src="/Images/Board_Back.png">

Two versions were designed, the first was a experimental board and the second the final one. The second version solved the problems found on the first and also added a circuit to control the voltage divider, this helped reduce the deep sleep current by 9 µA.

|                           | V1.0          | V2.0          | 
| ------------              | ------------  | ------------  | 
| Current Deep Sleep (µA)   | 13.33         | 6.03          | 

The microcontroller wakes every 5 minutes to do the measurement and send the data. With this value, the device can last up to 1800 days with a 550 mAh Li-Ion battery. This is a estimated  value because it's not easy to calculate the exact one, the system doesn't consume the exact ammount every cycle and the battery datasheet doesn't state the BMS drawn current. 

------------

### Communication Protocol 

In order to choose the most efficient protocol, three tests were executed between BLE, ESP-NOW and MQTT. The first test was about message delivering, each test corresponds to sending messages every 5 minutes for 24 hours (288 messages per day). 

The values on the table show the average result for 10 tests.

|               | ESP-NOW       | MQTT          | BLE (NimBLE)  |
| ------------  | ------------  | ------------  | ------------  |
| Average (%)   | 100.0         | 97.4          | 99.8          |

ESP-NOW was the only protocol to delivery every message. 

MQTT couldn't get 100% in any test, it lost about 3 to 8 messages per test, this loss could be related to the library used not support QoS 0. In case the broker didn't receive the packet 750 ms after the first send, another one was published and if after 750 ms the broker still didn't received, the board entered deep sleep. Wi-Fi connection with fixed IP.

BLE in 6 tests delivered every message and in the others lost about 2 to 4 messages, for the test NimBLE library was used, the default BLE library has many problems related to memory.

The last test is about the average current and time during execution. The values on the table show the average result for 10 measurements. The time corresponds to the moment the ESP wakes from deep sleep and goes into deep sleep.

|               | ESP-NOW       | MQTT          | BLE (NimBLE)  |
| ------------  | ------------  | ------------  | ------------  |
| Current (mA)  | 28.76         | 69.65         | 80.57         |
| Time (ms)     | 85.31         | 906.26        | 737.57        |

ESP-NOW is by far the most efficient protocol, the way it's built allows communication to be direct and fast. Since it only uses the antenna to send the message, it's possible to achive low current during execution.

<img src="/Images/ESP-NOW.png">

MQTT needs about 300-400 ms to connect to Wi-Fi and MQTT server, without fixed IP, connection time increases in 2 to 3 seconds. Average current increases compared to ESP-NOW because the antenna is used several times.

<img src="/Images/MQTT.png">

BLE uses the antenna quite often because needs to adverstise the device, services and then transmit data to the other device. Eventhought it uses lower current, the average current is higher compared to MQTT.

<img src="/Images/BLE.png">

|                   | ESP-NOW       | MQTT          | BLE (NimBLE)  |
| ------------      | ------------  | ------------  | ------------  |
| Difference (%)    | ---           | 96.10         | 95.90         |

ESP-NOW is 96.10% more efficient then MQTT and 95.90% then BLE.

After the results of both tests, ESP-NOW protocol was chose.

---

### Code 

The microcontroller saves the last reading in the RTC memory, by doing this it's possible to compare the values just measured to the ones stored in the memory. This allows the system to not send the message in case the measurements are the same and increases battery life.

```arduino
//Variables to store last measurement
RTC_DATA_ATTR double    RTC_Temperature = 0;
RTC_DATA_ATTR byte      RTC_Humidity    = 0;

// Block code after measurement
if(Data.Temperature == RTC_Temperature && Data.Humidity == RTC_Humidity)
{
    // Start deep sleep
    esp_deep_sleep_start();
}
else
{
    // Store values 
    RTC_Temperature = Data.Temperature;
    RTC_Humidity    = Data.Humidity;
}
```

The board responsible for receiving data needs to have the same data structure.

```arduino
// ESP-NOW data structure   
typedef struct struct_message 
{
  byte   ID = X;               // Board identifier
  float  Temperature;          // Temperature variable
  byte   Humidity;             // Humidity variable
  byte   Battery_Percentage;   // Battery percentage variable
} struct_message;

struct_message Data;
```


The frequency that the code runs impacts current and time of execution. The code was divide in two phases, the first is from the moment the board wakes from deep sleep to the moment it starts the ESP-NOW function, the second is from the start of ESP-NOW function to the moment it enters in deep sleep. After some measurements done, the results showed 40-160 MHz was the most efficient combination. In case the microcontroller sends every message, running with fixed 160 MHz is more efficient.


---

### Schematic 

The [schematic](/Hardware/PCB/Schematic.pdf) is arround ESP32-C3 microcontroller and SHT31 sensor. The P-Channel Mosfet BSS84 and the N-Channel Transistor S8050 control the voltage for the voltage divider. The board is designed to be powered with a Li-Ion battery that includes a BMS.


<img src="/Images/Schematic.png">

------------

### PCB

[This](Hardware/PCB/) 1.6mm board contains 4 layers with the stackup Signal+Power/GND/GND/Signal+Power. GND area was removed closer to the sensor in order to not affect readings. The pogo pin connector is for programming the microcontroller. The reset button is located on the back.

<img src="/Images/PCB.png">

---

### Known issues

- None


------------

### To-Do

- TPL5110 Timer to control the microcontroller and reduce deep sleep current


