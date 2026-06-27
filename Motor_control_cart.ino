/*
 * DC Motor Control for a cart. Allows you to vary distance, acceleration, minimum pwm, and utilizes a soft-approach window. 
 * Designed for an Arduino UNO and L298N with a DC brushed motor, where 240 pulses to the encoder yields 1 revolution
 */

// ============================================================
//  BANG-BANG MOTOR CONTROL  –  Arduino Uno + L298N (Soft Approach)
// ============================================================


// ─────────────────────────────────────────────────────────────
//  ★  USER-CONFIGURABLE VARIABLES  ★
// ─────────────────────────────────────────────────────────────
const float ACCELERATION   = 0.51;    // PWM units added per 10 ms tick (accel phase only)
const float END_DISTANCE   = 69.0;  // One-way travel distance in inches
const unsigned long PHASE_DELAY_MS = 500; // Pause time at the turn-around point (in milliseconds) //end adjustment
// ─────────────────────────────────────────────────────────────


// ── Minimum PWM before the motor begins to spin ──────────────
const int MIN_PWM = 30;


// ── Pin assignments ───────────────────────────────────────────
const int PIN_ENC_A = 2;   // Encoder channel A – interrupt-capable
const int PIN_ENC_B = 3;   // Encoder channel B – direction sense
const int PIN_FWD   = 6;   // analogWrite here → wheels move forward
const int PIN_REV   = 5;   // analogWrite here → wheels move reverse


// ── Derived constants (auto-calculated; do not edit) ──────────
const float PULSES_PER_INCH = 240.0f / 15.865f;
const long  END_PULSES      = (long)(END_DISTANCE * PULSES_PER_INCH + 0.5f);
const long  HALF_PULSES     = END_PULSES / 2;

// ── Soft-Approach Window Configuration ────────────────────────
const long APPROACH_WINDOW_PULSES = 70; 
// ──────────────────────────────────────────────────────────────


// ── Control and reporting intervals ──────────────────────────
const unsigned int CONTROL_MS = 10;   // Bang-bang update rate (ms)
const unsigned int REPORT_MS  = 100;  // Serial print interval  (ms)


// ── Direction labels ──────────────────────────────────────────
#define FORWARD  1
#define REVERSE -1


// ── Runtime state ─────────────────────────────────────────────
volatile long increm = 0;    // Encoder pulse count

// Motion phases:
//    0 = forward, accelerating
//    1 = forward, decelerating (soft approach)
//    2 = turn-around pause window (non-blocking delay)
//    3 = reverse, accelerating
//    4 = reverse, decelerating (soft approach)
//    5 = complete
int   phase       = 0;
float current_pwm = 0.0f;   // Logical PWM (0 … 255 – MIN_PWM)
float peak_pwm    = 0.0f;   // Captured peak PWM

unsigned long lastControlMs  = 0;
unsigned long lastReportMs   = 0;
unsigned long delayStartTime = 0; // Tracks when the intermission pause started
long          lastCounts     = 0;
double        currentSpeed   = 0.0;


// =============================================================
void setup() {
  pinMode(PIN_ENC_A, INPUT);
  pinMode(PIN_ENC_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), onEncoder, RISING);

  Serial.begin(9600);
  Serial.println("Bang-bang motor control ready.");
  Serial.print("END_PULSES  = "); Serial.println(END_PULSES);
  Serial.print("HALF_PULSES = "); Serial.println(HALF_PULSES);

  pinMode(PIN_FWD, OUTPUT);
  pinMode(PIN_REV, OUTPUT);
  driveMotor(0.0f, FORWARD);
}


// =============================================================
void loop() {
  unsigned long now = millis();

  // ── Bang-bang control tick ────────────────────────────────
  if (now - lastControlMs >= CONTROL_MS) {
    lastControlMs = now;
    noInterrupts();
    long pos = increm;
    interrupts();
    bangBang(pos);
  }

  // ── Serial telemetry ──────────────────────────────────────
  if (now - lastReportMs >= REPORT_MS) {
    noInterrupts();
    long counts = increm;
    interrupts();

    double dt     = (now - lastReportMs) / 1000.0;
    long   delta  = counts - lastCounts;
    currentSpeed  = abs((delta / 240.0) / dt);
    int    pwmOut = (current_pwm > 0.0f) ? (int)current_pwm + MIN_PWM : 0;

    Serial.print("Phase: ");           Serial.print(phase);
    Serial.print("  PWM: ");           Serial.print(pwmOut);
    Serial.print("  Speed (RPS): ");   Serial.print(currentSpeed, 2);
    Serial.print("  Pos (in): ");      Serial.print(counts / PULSES_PER_INCH, 2);
    Serial.print("  Pos (pulses): ");  Serial.println(counts);

    lastCounts   = counts;
    lastReportMs = now;
  }
}


// =============================================================
//  Bang-bang state machine
// =============================================================
void bangBang(long pos) {
  switch (phase) {

    // ── Phase 0: Forward, accelerating ───────────────────────
    case 0:
      current_pwm += ACCELERATION;
      if (current_pwm > 255 - MIN_PWM) current_pwm = 255 - MIN_PWM;
      driveMotor(current_pwm, FORWARD);
      if (pos >= HALF_PULSES) {
        peak_pwm = current_pwm;   
        phase = 1;
      }
      break;

    // ── Phase 1: Forward, decelerating ───────────────────────
    case 1: {
      long remaining = END_PULSES - pos;
      float progress = (float)remaining / (float)HALF_PULSES;
      if (progress > 1.0f) progress = 1.0f;
      if (progress < 0.0f) progress = 0.0f;

      current_pwm = peak_pwm * progress;

      if (remaining <= APPROACH_WINDOW_PULSES && remaining > 0) {
        float window_factor = (float)remaining / (float)APPROACH_WINDOW_PULSES;
        current_pwm *= sqrt(window_factor); 
      }

      driveMotor(current_pwm, FORWARD);
      
      if (pos >= END_PULSES) {
        current_pwm = 0.0f;
        driveMotor(0.0f, FORWARD);
        
        // Setup non-blocking turn-around delay
        delayStartTime = millis(); 
        phase = 2; 
      }
      break;
    }

    // ── Phase 2: Intermission Delay (Forward to Reverse) ─────
    case 2:
      // Hold motor hard at 0 until our variable timer expires
      driveMotor(0.0f, FORWARD); 
      if (millis() - delayStartTime >= PHASE_DELAY_MS) {
        phase = 3; // Time up! Transition to reverse travel
      }
      break;

    // ── Phase 3: Reverse, accelerating ───────────────────────
    case 3:
      current_pwm += ACCELERATION;
      if (current_pwm > 255 - MIN_PWM) current_pwm = 255 - MIN_PWM;
      driveMotor(current_pwm, REVERSE);
      if (pos <= HALF_PULSES) {
        peak_pwm = current_pwm;   
        phase = 4;
      }
      break;

    // ── Phase 4: Reverse, decelerating ───────────────────────
    case 4: {
      long remaining = pos; 
      float progress = (float)remaining / (float)HALF_PULSES;
      if (progress > 1.0f) progress = 1.0f;
      if (progress < 0.0f) progress = 0.0f;

      current_pwm = peak_pwm * progress;

      if (remaining <= APPROACH_WINDOW_PULSES && remaining > 0) {
        float window_factor = (float)remaining / (float)APPROACH_WINDOW_PULSES;
        current_pwm *= sqrt(window_factor); 
      }

      driveMotor(current_pwm, REVERSE);
      
      if (pos <= 0) {
        current_pwm = 0.0f;
        driveMotor(0.0f, FORWARD);
        phase = 5;
        Serial.println("== Motion complete ==");
      }
      break;
    }

    // ── Phase 5: Complete ─────────────────────────────────────
    case 5:
      break;
  }
}


// =============================================================
void driveMotor(float pwm_level, int direction) {
  int out = (pwm_level > 0.0f)
            ? constrain((int)pwm_level + MIN_PWM, MIN_PWM, 255)
            : 0;

  if (direction == FORWARD) {
    analogWrite(PIN_REV, 0);
    analogWrite(PIN_FWD, out);
  } else {
    analogWrite(PIN_FWD, 0);
    analogWrite(PIN_REV, out);
  }
}


// =============================================================
void onEncoder() {
  if (digitalRead(PIN_ENC_B) == LOW) increm++;
  else                               increm--;
}
