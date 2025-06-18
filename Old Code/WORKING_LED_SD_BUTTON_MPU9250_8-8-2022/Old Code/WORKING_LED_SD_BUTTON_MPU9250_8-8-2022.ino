#include <mpu9250.h>

//  Cole Schreiner
//  Started around ~ 6/30/2022
//  Link/code used for inspiration for sensor:
/*
  Basic_I2C.ino
  Brian R Taylor
  brian.taylor@bolderflight.com
  https://www.youtube.com/watch?v=mzwovYcozvI
  https://robojax.com/learn/arduino/?vid=robojax-MPU9250
  --->>> used "Get Library from Robojax.com" link from above website
*/

//  NEW VERSION created to start/stop with button press
//  7/5/2022 Finished Button start Stop //  next step is setting up SD card
//---------------------------
//  7/28/2022 MPU9250 w/ Magnetometer working after many issues with MPU6050
/*  Issues included:
    Drift for gyroscope
    Accelerometer sensitivity/instability   */
//  Next steps:
/*  Button for start/stop
    Write to micro SD card   */
//    WORKING AS OF 7/28/2022
//---------------------------
//    Finished Button & SD card code
//    WORKING AS OF 8/1/2022
//---------------------------
//    Added OLED Display
//    WORKING AS OF 8/2/2022
//---------------------------
//    Removed OLED Display
//    Adding LED instead
//    8/8/2022
//---------------------------
//   New Management in town!
//   01/29/2025


//  ---------------------------------------   //
//  -------------   LED   -----------------   //
//  ---------------------------------------   //
int LED = 8; //assign blue LED to pin 8

//  ---------------------------------------   //
//  -------------   SD card   -------------   //
//  ---------------------------------------   //
#include <SPI.h> //standard libraries
#include <SD.h>

File myFile;
// change this to match your SD shield or module;
const int chipSelect = 10;


//  ----------    For Button    ----------    //
#include <Wire.h>  // Wire library - used for I2C communication
const int button = 2 ; //button with pin
boolean buttonState = LOW;
boolean lastState = LOW;
bool loopRunning = false;


//  ------------    Gyro Compass --------   //
#include "MPU9250.h"

// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68 (0x69 when pin ADO is connected to power)
MPU9250 IMU(Wire, 0x69);
int status;

void setup() {
  pinMode(button, INPUT);
  pinMode(LED, OUTPUT);
  //----- from robjax begin
  // serial to display data
  Serial.begin(115200);//
  while (!Serial) {}//ensures if serial monitor isn't working, the program will end

  // start communication with IMU
  status = IMU.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    while (1) {}
  }
  //------robojax end
  delay(100);
 

  /////////////////////////     For SD Card     /////////////////////////////////
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print("Initializing SD card...");


  //    COMMENTED OUT BECAUSE IT WILL ALWAYS FAIL
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  //


  Serial.println("initialization done.");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  //||||||||||||||||||||||||||||| Placed in (Void Loop) ||||||||||||||||
  if (myFile) {
    Serial.print("Writing to test.txt...");
    //  myFile.println("testing 1, 2, 3.");
    // close the file:
    //  myFile.close();
    //  Serial.println("done.");
  }
  //||||||||||||||||||||||||||||| Placed in (Void Loop) ||||||||||||||||

  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  // re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  }
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  /////////////////////////////////////End of SD Section//////////////////////////////////////////////////////////////

}


//  ----------------------------------------------------------------------------------------------------------------//
/////////////   -------------------------------    LOOP   -------------------------------    //////////////////////////
//  ----------------------------------------------------------------------------------------------------------------//
void loop() {


  // Button press to start and stop recording
  buttonState = digitalRead(button);
  if ((buttonState == HIGH) && (lastState == LOW)) {
    buttonState = LOW;
    lastState = LOW;
    delay (300);
          digitalWrite(LED, HIGH);

    myFile = SD.open("test.txt", FILE_WRITE);
    while (lastState == LOW) {


      //  if (myFile) {

      //---Robojax begin
      // read the sensor
      IMU.readSensor();
      // display the data
      Serial.print(IMU.getAccelX_mss(), 6); //gets meli G aka Gravity 
      Serial.print("\t");
      myFile.print(IMU.getAccelX_mss(), 6);
      myFile.print("\t");

      Serial.print(IMU.getAccelY_mss(), 6);
      Serial.print("\t");
      myFile.print(IMU.getAccelY_mss(), 6);
      myFile.print("\t");

      Serial.print(IMU.getAccelZ_mss(), 6);
      Serial.print("\t");
      myFile.print(IMU.getAccelZ_mss(), 6);
      myFile.print("\t");

      Serial.print(IMU.getGyroX_rads(), 6);
      Serial.print("\t");
      myFile.print(IMU.getGyroX_rads(), 6);
      myFile.print("\t");

      Serial.print(IMU.getGyroY_rads(), 6);
      Serial.print("\t");
      myFile.print(IMU.getGyroY_rads(), 6);
      myFile.print("\t");

      Serial.print(IMU.getGyroZ_rads(), 6);
      Serial.print("\t");
      myFile.print(IMU.getGyroZ_rads(), 6);
      myFile.print("\t");

      Serial.print(IMU.getMagX_uT(), 6);
      Serial.print("\t");
      myFile.print(IMU.getMagX_uT(), 6);
      myFile.print("\t");

      Serial.print(IMU.getMagY_uT(), 6);
      Serial.print("\t");
      myFile.print(IMU.getMagY_uT(), 6);
      myFile.print("\t");

      Serial.println(IMU.getMagZ_uT(), 6);
      myFile.println(IMU.getMagZ_uT(), 6);

      //no need for temperature
      //  Serial.print("\t");
      //  Serial.println(IMU.getTemperature_C(),6);
      delay(50);
      //-----end Robojax

      //  Button to stop recording
      //  delay(300);
      buttonState = digitalRead(button);
      if (buttonState == HIGH) {
        lastState = HIGH;
        Serial.println("EOF" );
        myFile.println("EOF" );
        myFile.close();
        digitalWrite(LED, LOW);
        return;


      }
    }
  }
}
