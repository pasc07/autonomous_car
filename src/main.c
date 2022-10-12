#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <string.h>
#include "esp_timer.h"

struct data_vehicle
{
    unsigned int id;
    int distance;
    bool turn_left;
    bool turn_right;
    bool forward;
    bool backward;
    uint8_t battery_state;
};

struct data_vehicle Data = {
    .distance = 0,
    .turn_right = false,
    .turn_left = false,
    .forward = true,
    .backward = false,
    .battery_state = 73 // 73%
};



/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO GPIO_NUM_15

// Ultra-sonic distance sensor
#define DIST_ECHO GPIO_NUM_18
#define DIST_TRIG GPIO_NUM_5
// DC motor command signal 3V3
#define FORWARD_SIGNAL GPIO_NUM_17
#define BACKWARD_SIGNAL GPIO_NUM_16
#define RIGHTARD_SIGNAL GPIO_NUM_0
#define LEFTWARD_SIGNAL GPIO_NUM_2

#define HIGH 1
#define LOW 0

// Functions prototype declation
void config_pins(void);
void forward_vehicle(void);
void backward_vehicle(void);
void stop_vehicle(void);
void rightward_vehicle(void);
void leftward_vehicle(void);
void init_steer(void);
void vehicle_controller(void);
void get_distance(void);

void vTaskBlink_led15(void * pvParameters);
void vTaskLogs(void * pvParameters);
void vTaskGet_distance(void * pvParameters);
void vTaskVehicle_controller(void * pvParameters);


// Main program
void app_main() {

    // add main program here
    config_pins();

    xTaskCreate(vTaskBlink_led15, "vTaskBlink_led15", 10000, NULL, 2, NULL);
    xTaskCreate(vTaskLogs, "vTaskLogs", 10000, NULL, 2, NULL);
    xTaskCreatePinnedToCore(vTaskGet_distance, "vTaskGet_distance", 10000, NULL, 3, NULL,1);
    xTaskCreate(vTaskVehicle_controller, "vTaskVehicle_controller", 10000, NULL, 2, NULL);
    while (1){
        // get_distance();
        vTaskDelay(10);
    }

}


// Functions definitions
void config_pins(void){
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(DIST_ECHO);
    gpio_set_direction(DIST_ECHO, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(DIST_TRIG);
    gpio_set_direction(DIST_TRIG, GPIO_MODE_OUTPUT);
    gpio_set_level(DIST_TRIG,LOW);

    gpio_pad_select_gpio(FORWARD_SIGNAL);
    gpio_set_direction(FORWARD_SIGNAL, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(BACKWARD_SIGNAL);
    gpio_set_direction(BACKWARD_SIGNAL, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(RIGHTARD_SIGNAL);
    gpio_set_direction(RIGHTARD_SIGNAL, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LEFTWARD_SIGNAL);
    gpio_set_direction(LEFTWARD_SIGNAL, GPIO_MODE_OUTPUT);

    stop_vehicle();
    init_steer();

}

void vehicle_controller(void){
    forward_vehicle();
    if (Data.distance < 50){
        rightward_vehicle();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }else{
        // To be implemented
        init_steer();
    }
}

void vTaskVehicle_controller(void * pvParameters){
    const char *pcTaskName = "Controller";
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1){
        vehicle_controller();
        vTaskDelay( pdMS_TO_TICKS(5) ); // toutes les 2000 ms
    }
    
}

void forward_vehicle(void){
    gpio_set_level(FORWARD_SIGNAL, HIGH);
    gpio_set_level(BACKWARD_SIGNAL, LOW);
    vTaskDelay(5 / portTICK_PERIOD_MS);
}

void backward_vehicle(void){
    gpio_set_level(FORWARD_SIGNAL, LOW);
    gpio_set_level(BACKWARD_SIGNAL, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void stop_vehicle(void){
    gpio_set_level(FORWARD_SIGNAL, LOW);
    gpio_set_level(BACKWARD_SIGNAL, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void rightward_vehicle(void){
    printf("Turning right \n");
    gpio_set_level(RIGHTARD_SIGNAL, HIGH);
    gpio_set_level(LEFTWARD_SIGNAL, LOW);
}
void leftward_vehicle(void){
    printf("Turning left \n");
    gpio_set_level(RIGHTARD_SIGNAL, LOW);
    gpio_set_level(LEFTWARD_SIGNAL, HIGH);
}
void init_steer(void){
    gpio_set_level(RIGHTARD_SIGNAL, LOW);
    gpio_set_level(LEFTWARD_SIGNAL, LOW);
}

/* 
    Get distance from ultranic sensor 
*/

void delay_us(uint64_t number_of_us) {
    uint64_t microseconds = (uint64_t)esp_timer_get_time();
    if (number_of_us) {
        while ((uint64_t)esp_timer_get_time() - microseconds < number_of_us) ;

    }
    
    // printf(" timer duration : %llu \n",(uint64_t)esp_timer_get_time() - microseconds);
}

void get_distance(void){
    gpio_set_level(DIST_TRIG,LOW);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    //Trigger
    gpio_set_level(DIST_TRIG,HIGH);
    delay_us(10);
    // vTaskDelay(2000 / portTICK_PERIOD_MS);
    gpio_set_level(DIST_TRIG,LOW);

    // Wait ECHO to go high
    while(gpio_get_level(DIST_ECHO) == 0);
    uint64_t trig_start = (uint64_t)esp_timer_get_time();
    // Wait ECHO to go LOW
    while(gpio_get_level(DIST_ECHO) == 1);
    uint64_t delta_t = (uint64_t)esp_timer_get_time() - trig_start;

    int distance= (int)(delta_t*0.034/2); // cm
    Data.distance = distance;
    // printf("Distance get distance: %d \n",distance);
}

void vTaskGet_distance(void * pvParameters){
    const char *pcTaskName = "Distance measuresement periodic task";
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1){
        get_distance();
        vTaskDelay( pdMS_TO_TICKS( 100 ) ); // toutes les 2000 ms
    }
    
}

void vTaskBlink_led15(void * pvParameters){
    const char *pcTaskName = "Periodic blink task";
     for( ;; ) {
        /* Blink off (output low) */
	    // printf("Turning off the LED: %d\n",0);
        gpio_set_level(BLINK_GPIO, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
	    // printf("Turning on the LED: %d\n",1);
        gpio_set_level(BLINK_GPIO, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void vTaskLogs(void * pvParameters){
    const char *pcTaskName = "Periodic log task";
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    char logs_str[200]="";
    for( ;; )
    {
        char str[50] = "";
        sprintf(logs_str,"\n--------------------%s------------------\n",pcTaskName);
        sprintf(str,"Vehicle distance to obstacle: %d cm \n",Data.distance);
        strcat(logs_str,str);
        if(Data.forward == true){
            sprintf(str,"Vehicle direction: Forward \n");
        }else if(Data.backward == true){
            sprintf(str,"Vehicle direction: Backward \n");
        } 
        strcat(logs_str,str);
        memset(str,0,strlen(str));
        sprintf(str,"Vehicle battery state: %u percent \n",Data.battery_state);
        strcat(logs_str,str);
        memset(str,0,strlen(str));
        printf("%s\n", logs_str);
        memset(logs_str,0,strlen(logs_str));
        xTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 1000 ) ); // toutes les 2000 ms
    }
}
