#include <Arduino.h>
#include "Wire.h"
#include <SI4735.h>

#define SI4732_RESET_PIN 8

#define AM_FUNCTION 1
#define FM_FUNCTION 0

uint16_t currentFrequency;
uint16_t previousFrequency;
uint8_t bandwidthIdx = 0;
const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};


SI4735 rx;

void showHelp();
void showStatus();

void setup() 
{
    pinMode(SI4732_RESET_PIN, OUTPUT);
    digitalWrite(SI4732_RESET_PIN, LOW);
    delay(100);
    digitalWrite(SI4732_RESET_PIN, HIGH);

    SerialUSB.begin(9600);
    Wire = MbedI2C(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    Wire.begin();

    Serial.println("AM and FM station tuning test.");

  showHelp();

  // Look for the Si47XX I2C bus address
  int16_t si4735Addr = rx.getDeviceI2CAddress(SI4732_RESET_PIN);
  if ( si4735Addr == 0 ) {
    Serial.println("Si473X not found!");
    Serial.flush();
    while (1);
  } else {
    Serial.print("The SI473X / SI474X I2C address is 0x");
    Serial.println(si4735Addr, HEX);
  }


  delay(500);
  rx.setup(SI4732_RESET_PIN, FM_FUNCTION);
  // rx.setup(RESET_PIN, -1, 1, SI473X_ANALOG_AUDIO);
  // Starts defaul radio function and band (FM; from 84 to 108 MHz; 103.9 MHz; step 100kHz)
  rx.setFM(8400, 10800, 10650, 10);
  delay(500);
  currentFrequency = previousFrequency = rx.getFrequency();
  rx.setVolume(45);
  showStatus();
}

void showHelp()
{
  Serial.println("The SI473X / SI474X I2C circuit test");
  Serial.println("Type F to FM; A to MW; 1 to All Band (100kHz to 30MHz)");
  Serial.println("Type U to increase and D to decrease the frequency");
  Serial.println("Type S or s to seek station Up or Down");
  Serial.println("Type + or - to volume Up or Down");
  Serial.println("Type 0 to show current status");
  Serial.println("Type B to change Bandwidth filter");
  Serial.println("Type 4 to 8 (4 to step 1; 5 to step 5kHz; 6 to 10kHz; 7 to 100kHz; 8 to 1000kHz)");
  Serial.println("Type ? to this help.");
  Serial.println("==================================================");
  delay(1000);
}

// Show current frequency
void showStatus()
{
  // rx.getStatus();
  previousFrequency = currentFrequency = rx.getFrequency();
  rx.getCurrentReceivedSignalQuality();
  Serial.print("You are tuned on ");
  if (rx.isCurrentTuneFM())
  {
    Serial.print(String(currentFrequency / 100.0, 2));
    Serial.print("MHz ");
    Serial.print((rx.getCurrentPilot()) ? "STEREO" : "MONO");
  }
  else
  {
    Serial.print(currentFrequency);
    Serial.print("kHz");
  }
  Serial.print(" [SNR:");
  Serial.print(rx.getCurrentSNR());
  Serial.print("dB");

  Serial.print(" Signal:");
  Serial.print(rx.getCurrentRSSI());
  Serial.println("dBuV]");
}

void showFrequency( uint16_t freq ) {

  if (rx.isCurrentTuneFM())
  {
    Serial.print(String(freq / 100.0, 2));
    Serial.println("MHz ");
  }
  else
  {
    Serial.print(freq);
    Serial.println("kHz");
  }
  
}


// Main
void loop()
{
  if (Serial.available() > 0)
  {
    char key = Serial.read();
    switch (key)
    {
    case '+':
      rx.volumeUp();
      break;
    case '-':
      rx.volumeDown();
      break;
    case 'a':
    case 'A':
      rx.setAM(520, 1750, 810, 10);
      rx.setAvcAmMaxGain(48); // Sets the maximum gain for automatic volume control on AM mode.
      rx.setSeekAmLimits(520, 1750);
      rx.setSeekAmSpacing(10); // spacing 10kHz
      break;
    case 'f':
    case 'F':
      rx.setFM(8600, 10800, 10390, 50);
      rx.setSeekAmRssiThreshold(0);
      rx.setSeekAmSNRThreshold(10);
      break;
    case '1':
      rx.setAM(100, 30000, 7200, 5);
      rx.setSeekAmLimits(100, 30000);   // Range for seeking.
      rx.setSeekAmSpacing(1); // spacing 1kHz
      Serial.println("\nALL - LW/MW/SW");
      break;
    case 'U':
    case 'u':
      rx.frequencyUp();
      break;
    case 'D':
    case 'd':
      rx.frequencyDown();
      break;
    case 'b':
    case 'B':
      if (rx.isCurrentTuneFM())
      {
        Serial.println("Not valid for FM");
      }
      else
      {
        if (bandwidthIdx > 6)
          bandwidthIdx = 0;
        rx.setBandwidth(bandwidthIdx, 1);
        Serial.print("Filter - Bandwidth: ");
        Serial.print(String(bandwidth[bandwidthIdx]));
        Serial.println(" kHz");
        bandwidthIdx++;
      }
      break;
    case 'S':
      rx.seekStationProgress(showFrequency,1);
      // rx.seekStationUp();
      break;
    case 's':
      rx.seekStationProgress(showFrequency,0);
      // rx.seekStationDown();
      break;
    case '0':
      showStatus();
      break;
    case '4':
      rx.setFrequencyStep(1);
      Serial.println("\nStep 1");
      break;  
    case '5':
      rx.setFrequencyStep(5);
      Serial.println("\nStep 5");
      break;    
    case '6':
      rx.setFrequencyStep(10);
      Serial.println("\nStep 10");
      break;
    case '7':
      rx.setFrequencyStep(100);
      Serial.println("\nStep 100");      
      break;
    case '8':
      rx.setFrequencyStep(1000);
      Serial.println("\nStep 1000");    
      break;
    case '?':
      showHelp();
      break;
    default:
      break;
    }
  }
  delay(100);
  currentFrequency = rx.getCurrentFrequency();
  if (currentFrequency != previousFrequency)
  {
    previousFrequency = currentFrequency;
    showStatus();
    delay(300);
  }
}

void loop1()
{
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(500);           // wait 5 seconds for next scan
}
