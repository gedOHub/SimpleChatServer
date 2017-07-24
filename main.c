/* 
 * File:   main.c
 * Author: gGediminas Jančys
 *
 * Created on July 19, 2017, 11:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include <sys/types.h>  // setsockopt,
#include <sys/socket.h> // setsockopt,
#include <netdb.h>
#include <arpa/inet.h>  // inet_ntop,
#include <string.h>     // sterror
#include <errno.h>      // errno


#define CONFIG_FILE "server.conf"
#define TRUE 1
#define PENDDING_CONECTIONS 15
/*
 * 
 */
int main(int argc, char** argv) {
    const char* ip;
    const char* port;
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
    if(!config_lookup_string(config, "PORT", &port)){
        fprintf(stderr, "No 'PORT' setting in configuration file.\n");
        
        free(config);
        free((char*)port);
        free((char *)ip);
        
        return (EXIT_FAILURE);
    }
    
    // Daugiau nereikes config kintamojo, atlaisvinu
    free(config);
    
    printf("Server settings: %s:%s\n", ip, port);
    
    // Sisteminiu funkciju rezultatu kintamasis
    int* rezult = malloc(sizeof(int));
    
    /* ===== Adreso duomenu paieska ===== */
    // Ieskau adreso informacijos kuriuo turiu klausytis
    struct addrinfo *hints = malloc(sizeof(struct addrinfo)), *res;
    // Nustatau kokio tipo socketo noriu
    hints->ai_family = AF_UNSPEC;   // Nesvarbu IPv4 ar IPv6
    hints->ai_socktype = SOCK_STREAM;   // Noriu TCP socketo
    hints->ai_flags = AI_PASSIVE;   // Uzpildo likusius duomensi pagal norima IP
    
    // Bandau gauti norimo adreso informacija
    if( (*rezult = getaddrinfo(ip, port, hints, &res)) != 0 ){
        // Jei napavyko gauti adreso inforamcijos
        fprintf(stderr, "[getaddrinfo] Unable to get %s:%s information. "
                "Error: %s\n", ip, port, gai_strerror(*rezult));
        
        free(rezult);
        free((char *)ip);
        free((char *)port);
        free(hints);
        free(res);
        
        exit(EXIT_FAILURE);
    }
    // Pasalinu nereikalingus kintamuosius
    free((char *)ip);
    free((char *)port);
    freeaddrinfo(hints);
    /* ===== END Adreso duomenu paieska =====  */
    
    /* ===== Socket kurimas ===== */
    // Serverio failu deskriptoriaus kintamasis
    int* serverSocket = malloc(sizeof(int));
    *serverSocket = -1;
    
    int* option = malloc(sizeof(int)); 
    *option = 1;
    
    // Begu per galimus variantus. Bandau kurti socketa
    for(struct addrinfo * p = res;p != NULL; p = p->ai_next) {
          //  Bandau kurti socketa
        if ((*serverSocket = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("[Socket] Unable create socket");
            // Nepavyko, bandysiu kitus nustatymus
            continue;
        }
        
        /* Nustatau socket opcija, kad leistu bindinti jei jau uzbindintas. Del 
         * nuluzinejimo ir apejimo klaidos kad adresas jau nuadojamas */
        if (setsockopt(*serverSocket, SOL_SOCKET, SO_REUSEADDR, option, sizeof(option)) == -1) {
            perror("[Setsockopt] Unable to set socket reuse");
            // Nepasiseke, baigiu programa
            
            free(rezult);
            freeaddrinfo(res); // free the linked list
            free(res);
            free(p);
            free(serverSocket);
            
            exit(EXIT_FAILURE);
        }
        
        /* Bandau bindinti nurodyta porta ir adresa prie socketo */
        if (bind(*serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(*serverSocket);
            perror("[Bind] Unable to bind");
            continue;
        }
        
        break;
    }
    
    free(option);
    /* Atlaisvinu rezultatu strukturas */
    freeaddrinfo(res); 
    
    /* Bandau klausytis nurodytu adresu */
    if (listen(*serverSocket, PENDDING_CONECTIONS) == -1) {
        perror("[Listen] Unable to listen for connections");
        
        free(serverSocket);
        
        exit(EXIT_FAILURE);
    }
    
    printf("Waitting for clients to conenct");
    
    /* ===== Main loop ===== */
    fd_set* visiSkaitomiSocket, *skaitomiSocket;
    // Nustatau i nulius
    FD_ZERO(visiSkaitomiSocket);
    FD_ZERO(skaitomiSocket);
    
    // Pridedu serverio socketa prei skaitomu saraso
    FD_SET(*serverSocket, visiSkaitomiSocket);
    
    // Kintamasis nurodantis kas kiek laiko tikrinti ar yra skaitomu socketu
    struct timeval* time;
    time = malloc(sizeof(struct timeval));
    // Tikrinsiu kas 10 ms
    time->tv_sec = 0;       // sekundes
    time->tv_usec = 10000;  // mikso sekundes
    
    // Maksimalus socket numeris
    int* maxSocket;
    // Inicizacija
    maxSocket = malloc(sizeof(int));
    *maxSocket = -1;
    
    // Kliento adresas
    struct sockaddr_storage* clientAddr = malloc(sizeof(struct sockaddr_storage));
    struct socklen_t* addressLeng = malloc(sizeof(socklen_t));
    addressLeng = sizeof(struct sockaddr_storage);
    int* clientFD = malloc(sizeof(int));
    
    while(TRUE) {
        // Nustatau kurius deskriptroius tikrinsiu
        *skaitomiSocket = *visiSkaitomiSocket;
        
        // Gaunu visus socketus, kurie turi ka nors nuskaityti
        if (select(maxSocket + 1, skaitomiSocket, NULL, NULL, time)
                == -1) {
            fprintf("[Select] %s", strerror(errno));
        }
        
        addressLeng = sizeof &clientAddr;
        clientFD = accept(serverSocket, (struct sockaddr *)clientAddr, addressLeng);
    }
    
    /* ===== END Main loop ===== */
    
    // Baigesi nustatyami, tikrinu ar pavyko sukurti socketa
    
    /*
    // Kuriu skaitomu socketu sarasa ir einamu metu skaitomu socketu sarasa
    fd_set* allSockets, *readSockets;
    
    // Inicijuoju visų socketų sąrašą
    allSockets = malloc(sizeof(fd_set));
    FD_ZERO(allSockets);
    // Inicijuoju einamu momentu skaitomu socketu sarasa
    readSockets = malloc(sizeof(fd_set));
    FD_ZERO(readSockets);
    
    // Iteratorius
    int* iterator;
    iterator = malloc(sizeof(int));
    *iterator = 0;

    
    
    
     * */
    
//    free(readSockets);
//    free(maxSocket);
//    free(iterator);
//    free(time);
    free(rezult);
    //free(res);
    
    
    return (EXIT_SUCCESS);
}

