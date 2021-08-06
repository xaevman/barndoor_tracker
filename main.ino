// **** mount ****
// 13.625in from hinge to threaded rod on both boards
// assuming 1in of threaded rod
// cos(b) = (13.625^2 + 13.625^2 - 1^2) / 2 * 13.625 * 13.625
// b = 4.2035 degrees per inch
// 1/4-20 threads
// .2102 degrees per rev

// **** motor ****
// 200 steps / rev
// 8 microsteps per step
// 1600 steps per rev

// **** system ****
// .2102 / 1600   = .0001314 degrees per step
// 360 / .0001314 = 2,739,726 steps / rev
// 2,739,726 / 86164 = 31.7098 steps / sec for 24 hrs
// 1000 / 31.7966 = 31.5 ms between steps
const int ms_per_step = 32; // good for 1min@300mm with no detectable trailing

// pins
const int shutter_pin = 4;
const int step_pin    = 5;
const int dir_pin     = 6;
const int fwd_sw_pin  = 9;
const int bck_sw_pin  = 10;
const int speed_pin   = 11;
const int enable_pin  = 3;
const int m1_pin      = 7;
const int m2_pin      = 8;

// camera observation plan
const unsigned long startup_delay = 5000; // 5 sec
const unsigned long shot_delay    = 5000; // 5 sec
const unsigned long shot_timer    = 25000; // 60 sec
const int           exposures     = 60;

// vars
int           exposure_count = 0;
unsigned long next_toggle    = startup_delay;
bool          shutter_open   = false;
int           steps          = 0;
int           step_speed     = ms_per_step;
unsigned long start_loop     = 0;

// ==============================================================================
void doStep() {
  ++steps;

  digitalWrite(step_pin, HIGH);
  delay(1);
  digitalWrite(step_pin, LOW);
  
  // compensate for the time taken to run the loop
  unsigned long elapsed      = millis() - start_loop;
  int           speed_adjust = step_speed - elapsed;
  
  delay(constrain(speed_adjust, 1, 32767));
}

// ==============================================================================
void getSpeed() {
  if (digitalRead(speed_pin)) {
    // high speed, used to reset position
    digitalWrite(m1_pin, LOW);
    digitalWrite(m2_pin, LOW);
    step_speed = 1;
  } else {
    // precise speed, uses 8 microsteps
    digitalWrite(m1_pin, HIGH);
    digitalWrite(m2_pin, HIGH);
    
    // flip flop between 32 and 31 ms delay to avg 31.5
    if (steps % 2 == 0) {
      step_speed = ms_per_step;
    } else {
      step_speed = ms_per_step - 1; 
    }
  }
}

// ==============================================================================
void getDir() {
  if (digitalRead(fwd_sw_pin)) {
    digitalWrite(enable_pin, LOW);
    digitalWrite(dir_pin,    HIGH);
  } else if (digitalRead(bck_sw_pin)) {
    digitalWrite(enable_pin, LOW);
    digitalWrite(dir_pin,    LOW);
  } else {
    // reset steps to 0 on switch actuation
    steps = 0;
    
    // disable camera control
    digitalWrite(shutter_pin, LOW);
    exposure_count = exposures + 1;
    
    // disable motor control
    digitalWrite(enable_pin, HIGH);
  }
}


// ==============================================================================
void toggleShutter() {
  if (shutter_open) {
    digitalWrite(shutter_pin, LOW);
    shutter_open = false;
    exposure_count++;  
    next_toggle = start_loop + shot_delay;  
    Serial.print("end exposure ");
    Serial.println(exposure_count);
  } else {
    digitalWrite(shutter_pin, HIGH);
    shutter_open = true;    
    Serial.println("start exposure");
  }  
}

// ==============================================================================
void runCamera() {  
  // bail early if we're done shooting
  if (exposure_count >= exposures) {
    return;
  }
  
  // bail early if we're not to our next scheduled event yet
  if (start_loop < next_toggle) {
    return;
  }
  
  // made it to next scheduled event. toggle!
  next_toggle = start_loop + shot_timer;
  toggleShutter();
}

// ==============================================================================
void setup() {
  pinMode(shutter_pin, OUTPUT);
  pinMode(step_pin,    OUTPUT);
  pinMode(dir_pin,     OUTPUT);
  pinMode(enable_pin,  OUTPUT);
  pinMode(m1_pin,      OUTPUT);
  pinMode(m2_pin,      OUTPUT);

  pinMode(fwd_sw_pin, INPUT);
  pinMode(bck_sw_pin, INPUT);
  pinMode(speed_pin,  INPUT);
  
  digitalWrite(shutter_pin, LOW);
  digitalWrite(step_pin,    LOW);
  digitalWrite(dir_pin,     LOW);
  digitalWrite(enable_pin,  HIGH);
  digitalWrite(m1_pin,      HIGH);
  digitalWrite(m2_pin,      HIGH);
  
  Serial.begin(9600);
  
  // only engage camera control if we're intialized with the motor
  // switch engaged. (a hack for now)
  if (digitalRead(fwd_sw_pin) || digitalRead(bck_sw_pin)) {
    exposure_count = 0;  
  } else {
    exposure_count = exposures + 1; 
  }
}

// ==============================================================================
void loop() {
  start_loop = millis();
  
  runCamera();
  
  getSpeed();
  getDir();  
  doStep();
}
