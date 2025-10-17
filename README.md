# This branch belongs to Mark

# FreeRTOS tasks


## 1. modbus_task

This reads the all sensors every 1000ms and pushes co2 reading to the queue (co2Queue). When it reads, it takes the mutex and releases it after reading. 

fan_speed is shared between modbus and controller task. 

Other sensor readings shared between modbus_task and ui_task.


## 2. controller_task

Has the highest priority. It controls the fan and co2 injection. When there is a new data in queue, it reads it. 

The queue receives data every 1000ms from modbus_task. 

If no dat arrives within that time it skips the process for that cycle. 

Based on the co2 reading, it decides whether to run the fan or inject co2. 

When injecting, it is limited to 1s and then stabilizes for 30s.

Whenever the controller writes to the fan, it takes the mutex, writes the new value, and then releases it


## 3. button_task

This helps user to adjust the co2 setpoint. 

It handles 3 buttons - increase, decrease and confirm. 

When a button is pressed, it sends a button event to the button queue.


## 4. ui_task

This responsible for displaying data on Oled display.

It reads the latest sensor values and setpoints, and refreshes display every 500ms. Eventhough the display refreshes every 500ms, sensor readings update only every 1000ms. The faster refresh helps to show button changes quickly. If we used 1000ms, button queue might oveflow and feel slow to respond.

It also listens for button events from the button queue to update the co2 setpoint in real time. 


