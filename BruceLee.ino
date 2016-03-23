//
// RIBB@TACKING Bruce Lee v1.0 - C64 game tribute and Bruce Lee homage
// Copyright (C) 2016 Axel Dietrich <foobar@zeropage.io>
// http://zeropage.io
//
// An IKEA Ribba frame with a 3D rendered voxel picture from the C64 game "Bruce
// Lee" and some cookoo's clock bells'n'whistles on the flipside.
// http://zeropage.io/ribbatacking-bruce-lee-c64-game-tribute-bruce-lee-homage/
// https://www.youtube.com/watch?v=-i-FtUF5Uvc
//
// Ein Ikea RIBBA Rahmen Kunstprojekt mit 3D Voxel Bild des C64 Spieleklassikers
// "Bruce Lee" und einigem Arduino-Elektronik-Firlefanz für einen huldvollen
// Kuckusuhr-Effekt.
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version. This program is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details. You should have received a copy of the GNU
// General Public License along with this program (file LICENSE). If not, see
// <http://www.gnu.org/licenses/>.
//
#include <SD.h>
#include <TMRpcm.h>          // https://github.com/TMRh20/TMRpcm
#include <SPI.h>
#include <Streaming.h>
#include <ADCTouch.h>        // http://playground.arduino.cc/Code/ADCTouch
#include <DS3232RTC.h>       // http://github.com/JChristensen/DS3232RTC
#include <Time.h>            // http://playground.arduino.cc/Code/Time
#define ARRAY_SIZE(arr)      (sizeof(arr)/sizeof(char*))
#define SD_CHIP_SELECT_PIN   4
#define SPEAKER_PIN          9
#define SENSOR_PIN1          A1
#define SENSOR_PIN2          A2
#define BIGBEN_START         8   // Bells'n'Whistles only from 8am
#define BIGBEN_END           19  // till 7pm
#define DEBUG                0

TMRpcm tmrpcm;
uint16_t gTouchRef0, gTouchRef1;
char *gWaveFiles[ ]   = {
  "bruc0116.wav", // Title song
  "bruc0316.wav", // The sumo Yamo is calling
  "bruc0416.wav", // Yamo or Black Ninja appeared
  "bruc0516.wav", // ?
  "bruc0616.wav", // Jump kick
  "bruc0716.wav", // Ninja stick/sword strike
  "bruc0816.wav", // Someone has been hit or kicked
  "bruc0916.wav", // Bruce hit by Ninja sword
  "bruc1022.wav", // ?
  "bruc1216.wav", // Door opened
  "bruc1316.wav", // ?
  "bruc1416.wav", // Bruce electrocuted
  "bruc1516.wav", // erupting Geysir
  "bruc1616.wav", // Lantern collected
  "kungfu16.wav"  // Carl Douglas' Kung Fu Fighting
};
byte gVolume       = 5; // vol 5 rendered best playback quality for my setup
byte gCurrentTrack = 0;
bool gQuality      = 1; // 2x oversampling
bool gLoop         = 0;
bool gStopped      = 0;
bool gPaused       = 0;
bool gAVRStarted   = 1;
int  lastHour      = -1;

void setup( ) {

  Serial.begin( 9600 );
  Serial.setTimeout( 200 ); // 200ms should be enough

  // RTC setup
  setSyncProvider( RTC.get );
  if ( DEBUG ) {
    timeStatus_t ts = timeStatus( );
    if ( ts != timeSet )
      Serial << "Time Status: " << ( ts==timeNotSet ? F("Time not set!") : F("Time set but sync did not succeed!") ) << endl;
  }

  // Initialize SD card
  if ( !SD.begin( SD_CHIP_SELECT_PIN ) ) {
    Serial << F( "Failed to initialize SD card." ) << endl;
    while( 1 );
  }

  // Sound setup. Presets for speaker pin, looping, quality (oversampling), and volume.
  tmrpcm.speakerPin = SPEAKER_PIN;
  tmrpcm.loop     ( gLoop    );
  tmrpcm.quality  ( gQuality );
  tmrpcm.setVolume( gVolume  );

  // Create reference values to account for the capacitance of our control wires.
  gTouchRef0 = ADCTouch.read( SENSOR_PIN1, 500 );
  gTouchRef1 = ADCTouch.read( SENSOR_PIN2, 500 );

  // Set lastHour to current hour on boot.
  time_t t = now( );
  lastHour = hour( t );

  // Welcome to the pleasure dome!
  Serial << F( "Welcome to RIBB@TACKING Bruce Lee v1.0" ) << endl;
  Serial << F( "(C) 2016 Axel Dietrich <foobar@zeropage.io>" ) << endl;
  if ( DEBUG ) Serial << F( "lastHour set to ") << lastHour << endl;
  Serial << F( "Type h for a list of commands." ) << endl;
}

void fShowHelp( ) {
  Serial << F( "=====================================================" ) << endl;
  Serial << F( "COMMANDS" ) << endl;
  Serial << F( "h     This help." ) << endl;
  Serial << F( "l     Toggle looping. (" ) << (gLoop?"on":"off") << ")" << endl;
  Serial << F( "p     Pause/Unpause." ) << (gPaused?" (paused)":"") << endl;
  Serial << F( "q     Toggle quality. (" ) << (gQuality?"2":"1") << F( "x oversampling" ) << ")" << endl;
  Serial << F( "s     Stop the music." ) << (gStopped?"(stopped, timer still running)":"") << endl;
  Serial << F( "tx    Play track, x out of {1.." ) << ARRAY_SIZE( gWaveFiles ) << "}." << endl;
  fShowTracks( );
  Serial << F( "+     Volume up." ) << endl;
  Serial << F( "-     Volume down." ) << endl;
  Serial << F( "0..7  Set volume." ) << endl;
  Serial << F( "?     Check if a wave file is being played, right now '" ) << (tmrpcm.isPlaying( )?"yes":"no") << "'." << endl;
  Serial << F( "Current volume is " ) << gVolume << "." << endl;
  Serial << F( "-----------------------------------------------------" ) << endl;
  Serial << F( "Tyy,m,d,h,m,s    Set time and date of the RTC." ) << endl;
  Serial << F( "S                Show current time and date." ) << endl;
  Serial << F( "=====================================================" ) << endl;
}

void fShowTracks( ) {
  for ( int i = 0; i < ARRAY_SIZE( gWaveFiles ); i++ )
    Serial << "      " << (i+1) << " - " << gWaveFiles[ i ] << ( i==gCurrentTrack ? " (current track)":"" ) << endl;
}

// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
// Also bei einer auf Winterzeit laufenden Uhr mit tzHours=1 und bei einer auf Sommerzeit umgestellten Uhr mit tzHours=2.
boolean fIsSummertime( byte y, byte month, byte day, byte hour, byte tzHours = 1 ) {
  uint16_t year = 2000 + y;
  return (month>3 && month<10)
      || (month== 3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) )
      || (month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7)) );
}

//
// set the Real Time CLock (RTC)
void fSetRTC( byte year /*two digits*/, byte month, byte day, byte hour, byte minute, byte second ) {
  tmElements_t tm;
  tm.Year   = y2kYearToTm( year );
  tm.Month  = month;
  tm.Day    = day;
  tm.Hour   = hour;
  tm.Minute = minute;
  tm.Second = second;
  time_t t = makeTime( tm );
  RTC.set( t ); // set time on real time clock
  setTime( t ); // set time on Arduino
}

//
// Show the current time and date with info about summer-/wintertime
void fShowTimeAndDate( ) {
  time_t    t = now  (   );
  byte     ho = hour ( t );
  byte     da = day  ( t );
  byte     mo = month( t );
  uint16_t ye = year ( t );
  Serial << ho << ":" << minute( t ) << " " << da << "." << mo << "." << ye;
  Serial << " (" << (fIsSummertime(ye,mo,da,ho)?"Summertime":"Wintertime") << ")" << endl;
}

void loop( ) {
  static uint32_t ii         = 0;
  static uint8_t  debounce   = 0;
  static uint16_t lastMillis = 0;
  static int      formerMi   = -1;

  // Check if touch wires have been, eh, touched.
  int touchVal0 = ADCTouch.read( SENSOR_PIN1 );
  int touchVal1 = ADCTouch.read( SENSOR_PIN2 );
  // Remove offset.
  touchVal0 -= gTouchRef0;
  touchVal1 -= gTouchRef1;
  if ( touchVal0 > 40 && ++debounce > 5 ) {
    debounce = 0;
    if ( tmrpcm.isPlaying( ) )
      tmrpcm.stopPlayback( );
    tmrpcm.play( gWaveFiles[ ii++%ARRAY_SIZE(gWaveFiles) ] );
  }

  // Big Ben bells'n'whistles stuff
  // check once every 5 secs
  uint16_t m = millis( );
  if ( ( m - lastMillis ) > (1000*5) ) {
    lastMillis = m;
    time_t t = now( );
    byte ho    = hour  ( t );
    byte mi    = minute( t );
    byte da    = day   ( t );
    byte mo    = month ( t );
    byte ye    = year  ( t );
    byte hoDST = ( fIsSummertime(ye,mo,da,ho) ? ho+1 : ho );
    byte repeat, waveIndex;
    // Is this Big Ben's operation time?
    if ( !tmrpcm.isPlaying( ) && ( hoDST >= BIGBEN_START ) && ( hoDST <= BIGBEN_END ) ) {
      // It is. Quarter of an hour announcement?
      if ( ( mi % 15 ) == 0 ) {
        if ( mi != formerMi ) {
          formerMi = mi;
          // Are we on the hour?
          if ( ho != lastHour ) {
            // We are on the hour.
            lastHour = ho;
            if ( hoDST==12 || hoDST==19 ) {
              // Title song at high noon and 7pm.
              repeat = 1;
              waveIndex = 0;
            } else {
              // Carl Douglas starts the hour
              tmrpcm.play( gWaveFiles[ 14 ] );
              while ( tmrpcm.isPlaying( ) )
                ; /* wait until Carl is finished */
              // Yamo's call hoDST number of times.
              repeat = ( hoDST>12 ? hoDST-12 : hoDST );
              waveIndex = 1;
            }
          } else {
            // Quarter of an hour.
            if ( hoDST < BIGBEN_END )
              repeat = mi / 15;
            else {
              // If hoDST == BIGBEN_END we do want the hourly announcement at BIGBEN_END o'clock.
              // But we do _not_ want the quarterly announcements at BIGBEN_END:15, BIGBEN_END:30, and BIGBEN_END:45.
              repeat = 0;
            }
            waveIndex = 7; // Black Ninja striking
          }
          for ( byte i = 0; i < repeat; i++ ) {
            tmrpcm.play( gWaveFiles[ waveIndex ] );
            while ( tmrpcm.isPlaying( ) )
              ; /* wait until track is finished */
            delay( 400 );
          }
        }
      }
    }
  }

  // Check for serial command.
  if ( Serial.available( ) ) {
    char ch;
    byte trackNo = 0, chdec;
    switch ( ch = Serial.read( ) ) {
      case 'h':
        fShowHelp( );
        break;
      case 'l':
        // 0 or 1. Can be changed during playback for full control of looping.
        gLoop = !gLoop;
        tmrpcm.loop( gLoop );
        Serial << F( "Looping is switched " ) << ( gLoop ? "on": "off" ) << "." << endl;
        break;
      case 'L':
        // set last hour for debugging purposes
        while ( !Serial.available( ) ) {}
        lastHour = Serial.parseInt( );
        Serial << F( "lastHour set to " ) << lastHour << endl;
        break;
      case 'M':
        // set former minute for debugging purposes
        while ( !Serial.available( ) ) {}
        formerMi = Serial.parseInt( );
        Serial << F( "formerMi set to " ) << formerMi << endl;
        break;
      case 'p':
        // Pauses/unpauses playback.
        if ( tmrpcm.isPlaying( ) ) {
          tmrpcm.pause( );
          gPaused = 1;
          Serial << F( "Playback paused." ) << endl;
        } else if ( gPaused ) {
          tmrpcm.pause( );
          gPaused = 0;
          Serial << F( "Playback continued." ) << endl;
        } else
          Serial << F( "No song is played, nothing to pause." ) << endl;
        break;
      case 'q':
        // Set 1 for 2x oversampling.
        gQuality = !gQuality;
        tmrpcm.quality( gQuality );
        Serial << F( "Quality set to " ) << ( gQuality ? "2": "1" ) << F( "x oversampling." ) << endl;
        break;
      case 's':
        // Stops the music, but leaves the timer running.
        if ( tmrpcm.isPlaying( ) ) {
          tmrpcm.stopPlayback( );
          gStopped = 1;
          gPaused = 0;
          Serial << F( "Playback of current track '" ) << gWaveFiles[ gCurrentTrack ] << F( "' stopped." ) << endl;
        } else
          Serial << F( "No song is played, nothing to stop." ) << endl;
        break;
      case 'S':
        fShowTimeAndDate( );
        break;
      case 't':
        // t{1..x} play tracks 1..x.
        while ( !Serial.available( ) ) {}
        trackNo = Serial.parseInt( ) - 1;
        if ( (trackNo>=0) && (trackNo<ARRAY_SIZE(gWaveFiles)) ) {
          tmrpcm.play( gWaveFiles[ trackNo ] );
          gStopped = gPaused = 0;
          gCurrentTrack = trackNo;
          Serial << F( "Playing track '" ) << gWaveFiles[ gCurrentTrack ] << "'." << endl;
        } else {
          Serial << F( "Track number must be in the range {1.." ) << ARRAY_SIZE( gWaveFiles ) << "}." << endl;
          fShowTracks( );
        }
        break;
      case 'T': {
        byte year   = Serial.parseInt( );
        byte month  = Serial.parseInt( );
        byte day    = Serial.parseInt( );
        byte hour   = Serial.parseInt( );
        byte minute = Serial.parseInt( );
        byte second = Serial.parseInt( );
        fSetRTC( year, month, day, hour, minute, second );
        fShowTimeAndDate( );
        //dump any extraneous input
        while ( Serial.available( ) > 0 ) Serial.read( );
        break;
      }
      case '?':
        // returns 1 if music playing, 0 if not.
        if ( tmrpcm.isPlaying( ) )
          Serial << F( "Track '" ) << gWaveFiles[ gCurrentTrack ] << F( "' is currently being played." ) << endl;
        else
          Serial << F( "Currently no track playing." ) << endl;
        break;
      case '+':
        // Increase volume by one.
        if ( gVolume < 7 ) {
          tmrpcm.volume( 1 );
          gVolume++;
          Serial << F( "Volume up, current volume is " ) << gVolume << "." << endl;
        } else
          Serial << F( "Maximum volume is 7." ) << endl;
        break;
      case '-':
        // Decrease volume by one.
        if ( gVolume > 0 ) {
          tmrpcm.volume( 0 );
          gVolume--;
          Serial << F( "Volume down, current volume is " ) << gVolume << "." << endl;
        } else
          Serial << F( "Minimum volume is 0." ) << endl;
        break;
      default:
        // 0 to 7. Set volume level.
        chdec = ch - 48;
        if ( chdec>=0 && chdec<=7 ) {
          tmrpcm.setVolume( chdec );
          gVolume = chdec;
          Serial << F( "Volume set to " ) << gVolume << "." << endl;
        } else
          Serial << F( "Volume must be in the range 0 to 7." ) << endl;
        break;
    }
  }
}
