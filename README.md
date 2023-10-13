# TrafficLightControl_Zephyr_RTOS

Acceptance testing is done with using Robot Framework with a serial connection library.

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
