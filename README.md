# DC-Motor-Control-with-Controlled-Acceleration
Designed for an ME360 course at Boston University, this code was made to move a cart with controlled acceleration a user-defined distance. Allows you to vary distance, acceleration, minimum pwm, and utilizes a soft-approach window.  

Designed for an Arduino UNO and L298N with a DC brushed motor, where 240 pulses to the encoder yields 1 revolution. The distance is based on a one-to-one relationship between the DC output shaft and the axle, where wheels are 5.05 inches in diameter. The background of this project was to move a vertical, unfixed 80/20 beam a set distance without it falling. The code is written in C++, and may be modified, feel free to use it! 
