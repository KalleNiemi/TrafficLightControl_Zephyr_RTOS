# TrafficLightControl_Zephyr_RTOS

* This is a embedded software development project using Zephyr RTOS and Nordic Semiconductor NRF5340 AUDIO board.  
  
* Acceptance testing is done with using Robot Framework with a serial connection library.
<img src="/files/Robot_Framework_SS.png" alt="Screenshot of completed test." style="height: 400px; width:400px;"/> <img src="/files/nRF5340Audio_board.jpg" alt="Picture of the Nordic Semiconductor Development Kit" style="height: 208px; width:319px;"/>
  
  
> #### Serial COM commands  
> Commands - Function  
> !TRAFFIC - Turns on the traffic lights  
> !YELLOW - Yellow blink program that runs indefinitely  
> !OFF - Turns off any program or light mode  
>   
> ?DEBUG - Turns debug prints on and off  
> ?TIME - Prints how long the device has been on  
>   
> <COLOR,SECONDS,ON/OFF/BLINK> - F.ex RED,10,ON  
> 
> ON turns light on for ## seconds  
> OFF turns light off after ## seconds       
> BLINK blinks the light for ## seconds  
> Allowed colors: RED, GREEN, YELLOW  
> Allowed time range: 1-60 seconds  
