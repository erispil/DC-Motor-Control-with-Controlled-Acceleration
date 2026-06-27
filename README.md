# DC-Motor-Control-with-Controlled-Acceleration
Designed for an ME360 course at Boston University, this code was made to move a cart with controlled acceleration a user-defined distance. Allows you to vary distance, acceleration, minimum pwm, and utilizes a soft-approach window.  

Designed for an Arduino UNO and L298N with a DC brushed motor, where 240 pulses to the encoder yields 1 revolution. The distance is based on a one-to-one relationship between the DC output shaft and the axle, where wheels are 5.05 inches in diameter. The background of this project was to move a vertical, unfixed 80/20 beam a set distance without it falling. The code is written in C++, and may be modified, feel free to use it! It uses a modified bang-bang control method to optimize for speed, and has brief pauses and a slow approach to reduce acceleration on the ends due to the motor's constraints. 

We calculated the theoretical maximum acceleration to move the beam without it falling by hand first and then verified via assembly, whose results can be seen below. We modeled the system in SolidWorks before calculating the wheel diameter and coding to get an idea of if our initial calculations were accurate, utilizing the fact that when the reaction force on the front mate was 0N, the bar would be about to tip. Varying the acceleration allowed us to find this:
<img width="944" height="872" alt="image" src="https://github.com/user-attachments/assets/aa03f720-183e-4aa7-ab4f-8bd6e6bd109a" />
<img width="1042" height="834" alt="image" src="https://github.com/user-attachments/assets/6c5ae0e0-1eb3-4c8e-9cd8-d43125934d83" />

We concluded that our initial calculations were correct, as they determined that the maximum acceleration would be 816.7  $mm/s^2$. This matched with the assembly result of the maximum acceleration falling in the range of 802  $mm/s^2$ to 912  $mm/s^2$
