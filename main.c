/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: gedinskas
 *
 * Created on July 19, 2017, 11:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>


#define CONFIG_FILE "server.conf"
/*
 * 
 */
int main(int argc, char** argv) {
    const char* ip;
    int* port;
    config_t *config = malloc(sizeof(config_t));
    
    // Inicijuoju nustatymu nuskaitymo objekta
    config_init( config );
    
    // Nuskaitau nustatymu faila
    if(! config_read_file(config, CONFIG_FILE))
    {
      fprintf(stderr, "%s:%d - %s\n", config_error_file(config),
              config_error_line(config), config_error_text(config));
      config_destroy(config);
      return(EXIT_FAILURE);
    }
    
    // Nuskaitau IP reiksme
    if(!config_lookup_string(config, "IP", &ip)){
        fprintf(stderr, "No 'IP' setting in configuration file.\n");
        free(config);
        return (EXIT_FAILURE);
    }
    
    port = malloc(sizeof(int));
    // Nuskaitau PORT reiksme
    if(!config_lookup_int(config, "PORT", port)){
        fprintf(stderr, "No 'PORT' setting in configuration file.\n");
        
        free(config);
        free(port);
        free((char *)ip);
        
        return (EXIT_FAILURE);
    }
    
    printf("Server settings: %s:%d\n", ip, *port);

    
    return (EXIT_SUCCESS);
}

