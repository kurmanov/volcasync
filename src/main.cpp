
/* 
 *  Copyright (c) Samat Kurmanov - https://kurmanov.me
 *  https://github.com/kurmanov
 *  Volca Sync Sketch - Connect your Volca to Arduino and use this code to 
 *  detect beats and measure start
 */

#include <Arduino.h>
#include "RingBuffer.cpp"

#define LED_PIN 13
#define SYNC_IN_PIN 2
#define SEQ_DETECTION_THRESHOLD 4
#define DISCONNECT_THRESHOLD_MS 2000

// led state
volatile byte state = LOW;
// subdivision of measure, 8 signals per measure
volatile int stage = 0;
// last millis() value
volatile int lastTick;
// init flag for playback, false = we don't know measure bounds
volatile bool measureDetected = false;


// buffer with raw time deltas between interrupts for fast sequence detection
// elements = time deltas between interrupts
RingBuffer<int> seqDetectionBuffer(3, SEQ_DETECTION_THRESHOLD);

// using different ring buffer for precise frequency measurement
// elements = instant measured frequency values
RingBuffer<double> freqMeasureBuffer(5, 0);

void reportMeasure()
{
  if (!measureDetected)
    return;

  stage++;
  //8 sync signals per measure (2 per quarter note)
  if (stage == 8) { 
    stage = 0;
    char buffer[32];
    char bpmStr[8];
    // 2 signals per quarter means we only need to multiply by 30 and not 60 
    double bpm = freqMeasureBuffer.Average() * 30;

    dtostrf(bpm, 4, 2, bpmStr);
    sprintf(buffer, "Measure start, BPM = %s\n", bpmStr);
    Serial.print(buffer);
    state = true;
    
  }
}


void volcaSync() {
  int newTime = millis();

  int delta = newTime - lastTick;

  // debounce
  if (delta < 5 ) {
    return;
  }

  if (delta > DISCONNECT_THRESHOLD_MS) {
    
    Serial.print("Volca has been disconnected, waiting for new measure start\n");
    measureDetected = false;
    stage = 0;
  }

  reportMeasure();

  seqDetectionBuffer.Put(delta);

  // raw value, can only be used after averaging
  double instantFreq = 1000.0 / (double)delta;
  lastTick = newTime;
  freqMeasureBuffer.Put(instantFreq);

  // when play is pressed there will be a irregularity in sync signal: [A, B, C] where A = C, and B != A
  int a = seqDetectionBuffer.Get(seqDetectionBuffer.Size() - 1); // last
  int b = seqDetectionBuffer.Get(seqDetectionBuffer.Size() - 2); // 1 before last
  int c = seqDetectionBuffer.Get(seqDetectionBuffer.Size() - 3); // 2 before last

  if (abs(a - c) < SEQ_DETECTION_THRESHOLD && abs(b - a) > SEQ_DETECTION_THRESHOLD)
  {
    Serial.print("PLAYBACK START DETECTED\n");
    // current measure started 1 sync signal before
    stage = 1;
    seqDetectionBuffer.Fill(a);
    measureDetected = true;
  }
}


void setup() {
  Serial.begin(115200);
  lastTick = millis();
  pinMode(LED_PIN, OUTPUT);
  pinMode(SYNC_IN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SYNC_IN_PIN), volcaSync, RISING);
}

void loop() {
  // flash LED on every measure start
  digitalWrite(LED_PIN, state);
  delay(100);
  state = false;
}
