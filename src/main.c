#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/timing/timing.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

//#define ECHO_ON			//Used for manual testing

// Define the timer structure
static struct k_timer my_timer;
static struct k_timer blink_timer;

// I/O pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Condition Variables
K_MUTEX_DEFINE(ON_mutex);
K_CONDVAR_DEFINE(ON_signal);

K_MUTEX_DEFINE(OFF_mutex);
K_CONDVAR_DEFINE(OFF_signal);

K_MUTEX_DEFINE(BLINK_mutex);
K_CONDVAR_DEFINE(BLINK_signal);

K_MUTEX_DEFINE(TRAFFIC_mutex);
K_CONDVAR_DEFINE(TRAFFIC_signal);

K_MUTEX_DEFINE(UART_mutex);
K_CONDVAR_DEFINE(UART_signal);

// INT DEFINITIONS
#define COLOR_ERROR		1
#define TIME_ERROR		2
#define COMMAND_ERROR	3
#define COMMAND_OK		0
#define CUSTOM_COMMAND 10

// Vars
char color_string[10];
char seconds_string[4];
int seconds_int;
char command_string[10];
char command[100];
char c=0;
int buffer = 30;
int command_index = 0;

//Flag variables
int lightModeEnabled =0;
int blinkSwitch =0;
int blinkSwitch2 =1;
int trafficON =0;
int debugEnabled = 0;
int blinkTimerStarted;
int colorOK = 0;
int secondsOK = 0;
int commandOK = 0;

//Runtime timer variables
uint64_t start_time, end_time, time_S;

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5

void ON_task(void *, void *, void*);
void OFF_task(void *, void *, void*);
void BLINK_task(void *, void *, void*);
void TRAFFIC_task(void *, void *, void*);
void UART_task(void *, void *, void*);

K_THREAD_DEFINE(ON_thread, STACKSIZE,ON_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(OFF_thread, STACKSIZE,OFF_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(BLINK_thread, STACKSIZE,BLINK_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(TRAFFIC_thread, STACKSIZE,TRAFFIC_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(UART_thread, STACKSIZE,UART_task,NULL,NULL,NULL,PRIORITY,0,0);

// Runtime Timer functions
uint64_t timer_start();
void timer_stop (uint64_t);
// Timer start
uint64_t timer_start(){
start_time = timing_counter_get();
return start_time;
}
// Timer stop
void timer_stop(uint64_t start_time){
uint64_t total_cycles;
uint64_t time_ns;

end_time = timing_counter_get();
total_cycles = timing_cycles_get(&start_time, &end_time);
time_ns = timing_cycles_to_ns(total_cycles);
time_S = time_ns *1e-9;
printk("Runtime: ");
printk("%lld", time_S);
printk(" seconds\n");
}
// All Leds Off
void leds_off(){
        gpio_pin_set_dt(&green,0);
        gpio_pin_set_dt(&red,0);
        gpio_pin_set_dt(&blue,0); 
}
// Timer interrupt functions
void timerStop_leds_off(struct k_timer *timer_id)
{
	lightModeEnabled = 0;
	leds_off();
}
void timerStop_leds_off2(struct k_timer *timer_id)
{
	leds_off();
	blinkSwitch2 = 1;
	k_yield();
	k_condvar_broadcast(&BLINK_signal);
}
void blinkON_timer(struct k_timer *timer_id)
{
	blinkSwitch2=0;
		if (strncmp(color_string,"RED",3)== 0){
			gpio_pin_set_dt(&red,1);
		} else if (strncmp(color_string,"YELLOW",6)== 0){
			gpio_pin_set_dt(&red,1);
			gpio_pin_set_dt(&green,1);
		} else if (strncmp(color_string,"GREEN",5)== 0){
			gpio_pin_set_dt(&green,1);
		}
	k_timer_init(&blink_timer, timerStop_leds_off2, NULL);				
	k_timer_start(&blink_timer, K_MSEC(500), K_NO_WAIT);
	k_yield();
}

// DEBUG print enable / disable
void debugToggle(){
	if ( debugEnabled == 0){
		printk("DEBUG prints enabled\n");
		debugEnabled = 1;
	} else if ( debugEnabled == 1){
		printk("DEBUG prints disabled\n");
		debugEnabled = 0;
	}
}
//PARSER FUNCTION
int parser(char *command) {
	int ret = COMMAND_ERROR;
	const char delim[2] = ",";
	char *saveptr;
	char *endptr;
	char *token = strtok_r(command, delim, &saveptr);

	colorOK = 0;
	secondsOK = 0;
	commandOK = 0;
	// counter for tokens
	int cnt = 0;
	// walk through other tokens
	while( token != NULL ) {
		if (cnt == 0) { // COLOR
			if (strncmp(token,"RED",3) == 0){
				strcpy(color_string,token);
				colorOK = 1;
			} else if (strncmp(token,"YELLOW",6) == 0){
				strcpy(color_string,token);
				colorOK = 1;
			} else if (strncmp(token,"GREEN",5) == 0){
				strcpy(color_string,token);
				colorOK = 1;
			} else {
				colorOK =0;
			return COLOR_ERROR;
			}
		} else if (cnt == 1) { // SECONDS
			int seconds = strtol(token, &endptr, 10);
			if (seconds  > 0 && seconds < 61){
				secondsOK =1;
				strcpy(seconds_string,token);						//string, need to use stoi(); for seconds again
				seconds_int = seconds;
			} else {
				secondsOK=0;
			return TIME_ERROR;
			}
		} else if (cnt == 2) {
			if (strncmp(token,"ON",2) == 0){
			commandOK=1;
			strcpy(command_string,token);
			} else if (strncmp(token,"OFF",3) == 0){
				commandOK=1;
			strcpy(command_string,token);
			} else if (strncmp(token,"BLINK",5) == 0){
				commandOK=1;
			strcpy(command_string,token);
			} else if (strncmp(token,"EASTEREGG",9) == 0){
				commandOK=1;
			strcpy(command_string,token);	
			} else {
			commandOK=0;
			return COMMAND_ERROR;
			}
		}
		if (colorOK ==1 && secondsOK==1 && commandOK ==1){
		ret = COMMAND_OK;
	}
		// increase counter
		cnt++;
		// next token
		token = strtok_r(NULL, delim, &saveptr);
	}
		
	return ret;
}
//CUSTOM COMMANDS
int customCommand(char *command){	
	if (strncmp(command,"!TRAFFIC",8) == 0){												// TRAFFIC LIGHTS
		strcpy(command_string,command);
		return CUSTOM_COMMAND;
	} else if (strncmp(command,"!YELLOW",7 )== 0){											// YELLOW BLINK MODE
		strcpy(command_string,command);
		return CUSTOM_COMMAND;
	} else if (strncmp(command,"!OFF",3) == 0){												// TURN LIGHTS OFF IMMEDIATELY
		strcpy(command_string,command);
		return CUSTOM_COMMAND;
	} else if (strncmp(command,"?DEBUG",6) == 0){											// DEBUG PRINTS ENABLE / DISABLE
		strcpy(command_string,command);
		return CUSTOM_COMMAND;
	} else if (strncmp(command,"?TIME",5) == 0){											// DEBUG PRINTS ENABLE / DISABLE
		strcpy(command_string,command);
		return CUSTOM_COMMAND;
	} 
}
int main(void)
{
    // Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
	// set led off
	gpio_pin_set_dt(&red,0);

        ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
	// set led off
	gpio_pin_set_dt(&green,0);

        ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
	// set led off
	gpio_pin_set_dt(&blue,0);

        // UART initialization
	if (!device_is_ready(uart_dev)) {
		printk("UART initialization failed!\r\n");
		return 0;
	} 

        // Wait for everything to initialize and threads to start
	k_sleep(K_SECONDS(1));
	// Sanity check
        #ifdef DEBUG
	printk("Started traffic lights\n");
	#endif

	timing_init();
	timing_start();
	start_time = timing_counter_get();

	printk("__________________________________________________________________________\n");
	printk("|	Commands			Function\n");										
	printk("|	!TRAFFIC			turns on the traffic lights\n");
	printk("| 	!YELLOW				Yellow blink program that runs indefinitely\n");
	printk("|	!OFF				Turns off any program or light mode\n");
	printk("|\n");
	printk("|	?DEBUG				Turns debug prints on and off\n");
	printk("|	?TIME				Prints how long the device has been on\n");
	printk("|\n");
	printk("|	<COLOR,SECONDS,ON/OFF/BLINK>	F.ex RED,10,ON\n");
	printk("|\n");
	printk("|	ON turns light on for ## seconds\n");
	printk("|	OFF turns light off after ## seconds\n");
	printk("|	BLINK blinks the light for ## seconds\n");
	printk("| 	Allowed colors: RED, GREEN, YELLOW\n");
	printk("|	Allowed time range: 1-60 seconds\n");
	printk("|__________________________________________________________________________\n");
    return 0;
}
void ON_task(void *, void *, void*){
	while (true){

		if (k_condvar_wait(&ON_signal, &ON_mutex, K_FOREVER) == 0){
			k_timer_init(&my_timer, timerStop_leds_off, NULL);
			leds_off();

			if (strncmp(color_string,"RED",3)== 0){
			gpio_pin_set_dt(&red,1);
			} else if (strncmp(color_string,"YELLOW",6)== 0){
				gpio_pin_set_dt(&red,1);
				gpio_pin_set_dt(&green,1);
			} else if (strncmp(color_string,"GREEN",5)== 0){
				gpio_pin_set_dt(&green,1);
			}

			k_timer_start(&my_timer, K_SECONDS(seconds_int), K_NO_WAIT);
		}
	}
}
void OFF_task(void *, void *, void*){
	while (true){
		if (k_condvar_wait(&OFF_signal, &OFF_mutex, K_FOREVER) == 0){
			k_timer_init(&my_timer, timerStop_leds_off, NULL);
			k_timer_start(&my_timer, K_SECONDS(seconds_int), K_NO_WAIT);
		}
	}
}
void BLINK_task(void *, void *, void*){
	while (true){
		if (k_condvar_wait(&BLINK_signal, &BLINK_mutex, K_FOREVER) == 0){
			leds_off();			
			if (blinkSwitch ==1 && trafficON ==0 && blinkTimerStarted ==0) {
				blinkTimerStarted =1;
				k_timer_init(&my_timer, timerStop_leds_off, NULL);				//Turns off the blink function after seconds_int
				k_timer_start(&my_timer, K_SECONDS(seconds_int), K_NO_WAIT);	//Turns off the blink function after seconds_int
			}

			
			if (lightModeEnabled ==1 && blinkSwitch2 ==1 && trafficON ==0){
				k_timer_init(&blink_timer, blinkON_timer, NULL);				
				k_timer_start(&blink_timer, K_MSEC(500), K_NO_WAIT);
			}
		}	
		k_yield();		
	}
}
void TRAFFIC_task(void *, void *, void*){
	while (true){
		if (k_condvar_wait(&TRAFFIC_signal, &TRAFFIC_mutex, K_FOREVER) == 0){
			leds_off();
			
			while (lightModeEnabled ==1 && trafficON ==1){
				if(lightModeEnabled==1){
					gpio_pin_set_dt(&red,1);		//RED
					k_msleep(1000);
				}
				if(lightModeEnabled==1){
					gpio_pin_set_dt(&red,1);		//YELLOW
					gpio_pin_set_dt(&green,1);
					k_msleep(1000);
				}
				if(lightModeEnabled==1){
					gpio_pin_set_dt(&red,0);		//GREEN
					gpio_pin_set_dt(&green,1);		
					k_msleep(1000);
				}
				if(lightModeEnabled==1){
					gpio_pin_set_dt(&red,1);		//YELLOW
					k_msleep(1000);
					leds_off();
				}
			}			
		}
	}
}
void UART_task(void *, void *, void*){							
	while (true){
		if (uart_poll_in(uart_dev,&c) == 0) {
                        if (c == '\r' || c =='\n'){
							#ifdef ECHO_ON
							printk("\n");
							#endif
							
							int ret = parser(command);
							int customCommandGiven = customCommand(command);
							if (customCommandGiven == CUSTOM_COMMAND){
							
								if (strncmp(command_string,"?DEBUG",6)!=0){
									leds_off();
									lightModeEnabled = 0;
									k_timer_stop(&my_timer);					//This stops the timer if a timer is already running
								}
							
								if (strncmp(command_string,"!TRAFFIC",8)==0){		//LIIKENNEVALOT
									if (debugEnabled ==1){
									printk("!TRAFFIC_debug\n");
									}
									blinkSwitch2 = 0;
									lightModeEnabled = 1;
									trafficON =1;
									k_condvar_broadcast(&TRAFFIC_signal);
								} else if (strncmp(command_string,"!YELLOW",7)==0){			//KELTANEN VILKKU
									if (debugEnabled ==1){
									printk("YELLOW_debug\n");
									}
									strcpy(color_string,"YELLOW");
									blinkSwitch =0;
									blinkSwitch2 = 1;
									trafficON =0;
									lightModeEnabled = 1;
									k_condvar_broadcast(&BLINK_signal);
								} else if (strncmp(command_string,"!OFF",4)==0){			//INSTANT OFF
									if (debugEnabled ==1){
									printk("!OFF_debug\n");
									}
									leds_off();
									lightModeEnabled = 0;
								} else if (strncmp(command_string,"?DEBUG",6)==0){			//DEBUG TOGGLE
									debugToggle();
								} else if (strncmp(command_string,"?TIME",5)==0){			//RUNTIME
								timer_stop(start_time);
								}
							} else if (ret == COMMAND_OK){
								k_timer_stop(&my_timer);					//This stops the timer if a timer is already running
								if (debugEnabled ==1){
								printk("COMMAND_OK\n");		//TÄMÄ PRINTTAANTU
								}
								if (strncmp(command_string,"ON",2)==0){
								
									k_condvar_broadcast(&ON_signal);		//LÄHTEE
								} else if (strncmp(command_string,"OFF",3)==0){
								
									k_condvar_broadcast(&OFF_signal);		//LÄHTEE
								} else if (strncmp(command_string,"BLINK",5)==0){
									blinkTimerStarted =0;
									blinkSwitch =1;
									blinkSwitch2 = 1;
									lightModeEnabled = 1;
									trafficON =0;
									k_condvar_signal(&BLINK_signal);
								}
							} else if (ret == COLOR_ERROR){
								printk("COLOR_ERROR\n");
							} else if (ret == TIME_ERROR){
								printk("TIME_ERROR\n");
							} else if (ret == COMMAND_ERROR){
								printk("COMMAND_ERROR\n");
							}
								
							command_index = 0;
							memset(command,0,100);
						
                		} else {
							#ifdef ECHO_ON
							printk("%c",c);
							#endif
							command[command_index] = c ;
							command_index++;
						}
                }
		k_yield();
	}
}
