// esp32 reads BME280 sensor, puts this readings on a SD Card and
// prints out everything nicely on a tft screen
// you need: TFT 2.2 -> 2.4 " screen with a SD Card
// esp32 has 3.3 V -> solder the jumper on den TFT SD Card to bypass the voltage
// regulator
// the SD card needs to be formated
// use the standard SPI pins clk=18, miso=12, mosi=23
// in User_Setup.h within the TFT_eSPI folder you have to define the pinout
// for the TFT Display
// SD.begin(4, SPI) defines the SD Card cs ( clockselect ) pin 

#include <TFT_eSPI.h> // Hardware-specific library
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <SD.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

#define DEG2RAD 0.0174532925

byte inc = 0;
unsigned int col = 0;

// barograph defines
float temperature, pressure, altitude;
void computePressureTrend();
void appendPressureInHistoric();
void updateDataToDisplay();

Adafruit_BMP280 bmp; // I2C

/*
 * SPI pins are set via TFT_eSPI library
//#define BMP280_I2C_ALT_ADDR 0x76// alternative adresse 0x76  // I2C address of BMP280
#define TFT_sclk 18      // TFT/SD: sclk pin 13 used from TFT SPI display and SD card read
#define TFT_miso 12      // TFT/SD: miso pin 12 used from TFT SPI display and SD card read
#define TFT_mosi 23      // TFT/SD: mosi pin 11 used from TFT SPI display and SD card read
#define TFT_cs 5        // TFT:    cs used from TFT SPI display
#define TFT_dc 25         // TFT/SD: dc used from TFT SPI display
#define TFT_rst 14        // TFT/SD: rst used from TFT SPI display
*/
/*
#define sd_cs 4       // SD:     sd_cs 4 used from SD card reader
#define FREQBUZ 2500
#define BUZ 7
*/

const unsigned char OSS = 0;  // Oversampling Setting
unsigned long lastMeasurementTime =0;  
unsigned long lastRefreshDisplay =0;
byte heightsToDisplay[320];

// touch on TFT SPI board

// for LED Alarm
//int led =6;


File historicFile;
File TestHistoricFile;


void init_screen(void)
{
  tft.init();
  tft.fillScreen(ILI9341_BLUE);
  tft.setRotation(1);

  //Drawing frames
  tft.fillScreen(ILI9341_BLUE);
  tft.drawRect(1,1,318,238,ILI9341_WHITE);    // white rectangle at display border
  tft.drawFastVLine(317, 1, 238,ILI9341_WHITE); 
  tft.drawFastVLine(316, 1, 238,ILI9341_WHITE); 
  tft.drawRect(10,50, tft.width()-70, 150, ILI9341_WHITE);   // frame for the graph
  tft.drawFastHLine(10,125,tft.width()-70, ILI9341_WHITE);   //HL in the middle of the graph at 1007
 
  // hPa scale indication
  tft.setCursor(tft.width()-55, tft.height()-45);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
////  tft.println("980");
  tft.println("900");
  tft.setCursor(tft.width()-55, tft.height()-120);
  tft.println("1007");
  tft.setCursor(tft.width()-55, 50);
  tft.println("1035");
  
  // Value units indication
  
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.setCursor(88, tft.height()-214);
  tft.println("hPa");
  //tft.setCursor(197, tft.height()-214);
  tft.setCursor(189, tft.height()-233);
  tft.println("Pa/H");
  tft.setCursor(10, tft.height()-25);
  tft.println("48H");
  tft.setCursor(120, tft.height()-25);
  tft.println("24H");
  tft.setCursor(tft.width()-85, tft.height()-25);
  tft.println("0H");
  
  // puts C on the screen
  tft.fillRect(140,4,50,43,ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.setCursor(tft.width()-25, 10);
  tft.println("C");
  tft.drawCircle(tft.width()-35,12,3,ILI9341_WHITE);  
}

void setup(void) {
  Serial.begin(115200);
  //Serial.begin(9600);
  
  // SD on the TFT Display module 
  if (!SD.begin(4, SPI)) {
    Serial.println("initialization failed!");
   return;
  }
  Serial.println("initialization done.");

  // I am using BME280 to measure pressure, temp. It's a more modern sensor as bmp085
  if (!bmp.begin(0x76)) {
      Serial.println(F("Could not find a valid BMP280 sensor"));
    while(1);
  }
  
  // Default settings from datasheet.
  
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     //Operating Mode.
                    Adafruit_BMP280::SAMPLING_X2,     // Temp. oversampling
                    Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling 
                    Adafruit_BMP280::FILTER_X16,      // Filtering. 
                    Adafruit_BMP280::STANDBY_MS_500); // Standby time.
  bmp.readAltitude(1023.05);

  init_screen();
}



void loop() {
  //Serial.println(bmp.readPressure());
  //sd_card_read();
  delay(1000);
  
  unsigned long gap = millis() - lastMeasurementTime;
  
  if (gap>=5000){  
     lastMeasurementTime +=gap;
  
    // read the bmp280
    if ( bmp.readPressure() )
    { 
      pressure = bmp.readPressure();
      temperature = bmp.readTemperature();
      altitude = bmp.readAltitude();
      
      tft.fillRect(10,4,75,43, ILI9341_BLUE);//clear rectangle pressure value
      //tft.fillRect(10,4,75,43, ILI9341_YELLOW);//clear rectangle pressure value
      tft.setCursor(10,tft.height()-220);
      tft.setTextColor(ILI9341_WHITE); tft.setTextSize(2);
      tft.print(pressure/100);// clear rectangle trend value
      tft.fillRect(tft.width()-54,25,50,20,ILI9341_BLUE);//clear rectangle temperature value
      tft.setCursor(tft.width()-54, tft.height()-210);
      tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
      tft.println(temperature,1); 
    } 
      computePressureTrend();
      appendPressureInHistoric();
      gap = millis() - lastRefreshDisplay;//refresh graph every minute
         if (gap>60000){
         lastRefreshDisplay += gap;
         updateDataToDisplay();
      }
   }
}
 
void updateDataToDisplay()
{
    tft.fillRect(11,51, 248, 74, ILI9341_BLUE);//clear screen  graph rectangles
    tft.fillRect(11, 126, 248, 73, ILI9341_BLUE);  
    int i=0;
    //File historicFile = SD.open("/test15.txt");//data saved in file test15
    historicFile = SD.open("/test15.txt");   //data saved in file test15
    if(!historicFile){
    return;
    }
    long fileSize = historicFile.size();
   
    long pos = ( fileSize - 276480);// 12 records /min* 60 min/h*48h * 8 bits /record
    
    char pressureArray[7];                       
    while(i<248 && pos <  4000000000 ){                                   
    historicFile.seek(pos);
     for(int j = 0; j < 6; j++){         // reads the record bytes one by one
     pressureArray[j] = historicFile.read();
     
     if(pressureArray[j]==' '){
          pressureArray[j] =0;
         }
      }
     pressureArray[6]=0;
     long pressureValue1 = atol(pressureArray); 
     long pressureValue = pressureValue1 + 3000;
     
     // Graph boarder limits
     if(pressureValue<90000){    
       pressureValue = 90000;
     }
     if(pressureValue>103500){
       pressureValue=103500;
     }
     heightsToDisplay[i] = map(pressureValue, 98000, 103500,tft.height()-42,tft.height()-189 );
     tft.drawPixel(tft.width()-310 +i, heightsToDisplay[i], ILI9341_WHITE);
     // on esp32 there is enough space for extra line, not on nano
     tft.drawPixel(tft.width()-310 +i, heightsToDisplay[i]-1, ILI9341_WHITE);// to increase thikness of the graph
     
     i++; 
     pos = pos+1112; // 276480 bits spread over 248 observations
    }
    historicFile.close();
}

void appendPressureInHistoric()
{
    //long pressure = bmp085GetPressure(bmp085ReadUP()); 
    Serial.print("pressure: "); Serial.println(pressure);  
    long druck = pressure;
    
    char pressureArray[7];
    String pressureStr = String (druck);
    pressureStr.toCharArray(pressureArray,7);
    if (pressure<100000){
      pressureArray[5] =' ';
     }
    pressureArray[6]=0;
    historicFile = SD.open("/test15.txt", FILE_WRITE);
    if(!historicFile){   
    return;
    
    }
    historicFile.println(pressureArray);
    Serial.println(pressureArray);
    historicFile.close();
}


//compute the 1 h pressure change
void computePressureTrend()
{   
    long druck = pressure; // convert float pressure -> long druck // bmp280.getPressure(pressure);
    //Serial.print("computePressureTrend(pressure/druck "); 
    //Serial.print(pressure); Serial.print(" "); Serial.println(druck);  
    //File historicFile = SD.open("/test15.txt");
    historicFile = SD.open("/test15.txt", FILE_WRITE); 
    if(!historicFile){
      //Serial.println("Couldn't open SD Card");
    return;
    }
    
    long fileSize = historicFile.size();
    long pos = ( fileSize - 8*12*60);  //record(8)*records per min(12)*min/h(60)
    char pressureArray[7];                       
    historicFile.seek(pos);
   
    for(int j = 0; j < 6; j++){    
    pressureArray[j] = historicFile.read();
    if(pressureArray[j]==' '){
           pressureArray[j] =0;
          }
     }
    pressureArray[6]=0;
    long pressure1HValue = atol(pressureArray);    
    //int OneHourTrend = int( pressure - pressure1HValue);
    int OneHourTrend = int( druck - pressure1HValue);
    //Serial.print(druck); Serial.print(" "); Serial.print(pressure); Serial.print(" "); 
    //Serial.println(OneHourTrend);
    
    if(OneHourTrend < -140){ // sound buzzer if pressure drop < -140 Pa/h
     //tone ( 6, 4000,500);
     //delay(500);
     //tone (6, 4500, 500);
     //delay(500);
     //digitalWrite(7,HIGH);//flash LED
     //delay(500);
     //digitalWrite(7,LOW);
     
     Serial.println("OneHourTrend < -140");
    }
    
    //noTone(6);
    //digitalWrite(led,LOW);
    
    //tft.fillRect(140,4,47,43,ILI9341_BLUE);        
    //tft.setCursor(140 ,tft.height()-214);           
    tft.fillRect(155,22,87,26,ILI9341_BLUE); // fill with blue to update number
    tft.setCursor(155 ,tft.height()-214);           
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(3);
    tft.println(OneHourTrend);
    Serial.print("OneHourTrend: "); Serial.println(OneHourTrend);
    historicFile.close();
}

void sd_card_read(){
  File file2 = SD.open("/test15.txt", FILE_READ);
 
  if (!file2) {
    Serial.println("Opening file to read failed");
    return;
  }
  Serial.println("File Content:");
  while (file2.available()) {
    Serial.write(file2.read());
  }
  file2.close();
}
