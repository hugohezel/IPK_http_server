#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Funkcia na kontrolu argumentov
bool test_for_arguments( int argc, char* argv[] ){

    // Kontrola, ci program dostal prave jeden argument
    if( argc != 2 ){

        return false;

    }

    // Kontrola, ci dany argument je cislo
    for( int i = 0; i < strlen( argv[1] ); i++ ){
        if( !isdigit( argv[1][i] ) ){
            
            return false;

        }

    }

    return true;

}

// Funckia pre ziskanie hostname-u
bool get_hostname( char *hostname ){

    // Otvor subor pre precitanie hostname-u
    FILE *hostname_file = fopen( "/proc/sys/kernel/hostname", "r" );

    // Detekcia chyby otvorenia subora
    if( hostname_file == NULL ){

        fprintf(stderr, "Chyba pri otvarani suboru /proc/sys/kernel/hostname");
        return false;

    }

    // Ulož si hostname
    fscanf( hostname_file, "%s", hostname );

    // Zatvor subor
    fclose( hostname_file );

    return true;

}

// Funkcia pre ziskanie nazvu procesora
bool get_cpu_name( char *cpu_name ){

    // Otvor subor pre precitanie nazvu procesora
    FILE *cpu_name_file = popen( "cat /proc/cpuinfo | grep 'model name' -m 1 | awk 'BEGIN{FS=\":\"}{printf \"%s\", $2}'", "r" );

    // Detekcia chyby otvorenia subora
    if( cpu_name_file == NULL ){

        fprintf(stderr, "Chyba pri otvarani suboru /proc/cpuinfo");
        return false;

    }

    // Ulož si nazov procesora
    fgets( cpu_name, 2048, cpu_name_file );

    // Ak je prvy znak medzera, vymaz ju
    if( cpu_name[0] == ' ' ){

        memmove( cpu_name, cpu_name + 1, strlen( cpu_name ) );

    }

    // Zavri subor
    pclose( cpu_name_file );

    return true;

}

// Funkcia dosadi hodnoty hovoriace o vytazenosti procesoru zo suboru /proc/stat do pola cpu_usage_values
bool get_cpu_usage_values( int *cpu_usage_values ){

    // Deklaruj string pre docasne ulozenie hodnot
    char cpu_usage_string[2048];

    // Otvor subor pre precitanie hodnot
    FILE *cpu_usage_file = popen( "cat /proc/stat | grep cpu -m 1 | awk 'BEGIN{FS = \" \"}{printf \"%s %s %s %s %s %s %s %s %s %s\", $2, $3, $4, $5, $6, $7, $8, $9, $10, $11}'", "r" );

    // Detekcia chyby otvorenia subora
    if( cpu_usage_file == NULL ){

        perror("Chyba pri otvarani suboru /proc/stat");
        return false;

    }

    // Ulož si nazov procesora
    fgets( cpu_usage_string, 2048, cpu_usage_file );

    // Priprav si premenne pre prechadzanie hodnotami oddelenymi medzerou
    char *value = strtok( cpu_usage_string, " " );
    int i = 0;

    while( value != NULL ){

        // Ulož hodnotu do pola cpu_usage_values
        cpu_usage_values[i] = atoi( value );

        i++;
        value = strtok( NULL, " " );

    }

    // Zavri subor
    pclose( cpu_usage_file );

    return true;

}

// Funkcia vyrata vytazenost procesoru a ulozi hodnotu do cpu_usage
bool get_cpu_usage( double *cpu_usage ){

    // Deklaruj pole pre uchovanie hodnot o cinnosti procesora
    int cpu_usage_first_values[10];
    int cpu_usage_second_values[10];

    // Napln prve pole aktualnymi hodnotami o cinnosti procesora
    if( !get_cpu_usage_values( cpu_usage_first_values ) ){

        return false;

    }

    // Pockaj jednu sekundu
    sleep(1);

    // Napln druhe pole aktualnymi hodnotami o cinnosti procesora
    if( !get_cpu_usage_values( cpu_usage_second_values ) ){

        return false;

    }

    unsigned long long int prevuser = cpu_usage_first_values[0];
    unsigned long long int prevnice = cpu_usage_first_values[1];
    unsigned long long int prevsystem = cpu_usage_first_values[2];
    unsigned long long int previdle = cpu_usage_first_values[3];   
    unsigned long long int previowait = cpu_usage_first_values[4];
    unsigned long long int previrq = cpu_usage_first_values[5];
    unsigned long long int prevsoftirq = cpu_usage_first_values[6];
    unsigned long long int prevsteal = cpu_usage_first_values[7];
    unsigned long long int prevguest = cpu_usage_first_values[8];
    unsigned long long int prevguest_nice = cpu_usage_first_values[9];

    unsigned long long int user = cpu_usage_second_values[0];
    unsigned long long int nice = cpu_usage_second_values[1];
    unsigned long long int system = cpu_usage_second_values[2];
    unsigned long long int idle = cpu_usage_second_values[3];   
    unsigned long long int iowait = cpu_usage_second_values[4];
    unsigned long long int irq = cpu_usage_second_values[5];
    unsigned long long int softirq = cpu_usage_second_values[6];
    unsigned long long int steal = cpu_usage_second_values[7];
    unsigned long long int guest = cpu_usage_second_values[8];
    unsigned long long int guest_nice = cpu_usage_second_values[9];

    unsigned long long int PrevIdle = previdle + previowait;
    unsigned long long int Idle = idle + iowait;

    unsigned long long int PrevNonIdle = prevuser + prevnice + prevsystem + previrq + prevsoftirq + prevsteal;
    unsigned long long int NonIdle = user + nice + system + irq + softirq + steal;

    unsigned long long int PrevTotal = PrevIdle + PrevNonIdle;
    unsigned long long int Total = Idle + NonIdle;

    unsigned long long int totald = Total - PrevTotal;
    unsigned long long int idled = Idle - PrevIdle;

    *cpu_usage = ((double)totald - (double)idled) / (double)totald * (double)100;
    
    return true;

}

// Hlavna funkcia programu
int main( int argc, char* argv[] ){

    // Otestovanie argumentov
    if( !test_for_arguments( argc, argv ) ){

        fprintf(stderr, "Chybne vstupne argumenty.");
        return 1;

    }

    // Uloženie portu
    int port = atoi( argv[1] );

    // Deklarovanie potrebnych premennych
    char hostname[2048];
    char cpu_name[2048] = "unknown";
    double cpu_usage = 0;

    struct sockaddr_in address;
    int address_len = sizeof( address );

    char message[10000];

    char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n";
    char hostname_response[10000];
    char cpu_name_response[10000];
    char cpu_usage_response[10000];

    // Dostan hostname a skontroluj vysledok operacie
    if( !get_hostname( hostname ) ){

        return 1;

    }

    // Vytvor HTTP odpoved pre hostname
    strcpy( hostname_response, http_response );
    strcat( hostname_response, hostname );


    // Dostan nazov procesoru a skontroluj vysledok operacie
    if( !get_cpu_name( cpu_name ) ){

        return 1;

    }

    // Vytvor HTTP odpoved pre nazov procesora
    strcpy( cpu_name_response, http_response );
    strcat( cpu_name_response, cpu_name );

    // Vytvorenie socketu
    int socket_des = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;

    // Nastavenie socketu
    setsockopt( socket_des, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) );
    setsockopt( socket_des, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof( opt ) );

    // Nastavenie adresy socketu
    address.sin_family = AF_INET; // mozno vymenit za AF_INET
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );

    // Nastavenie socketu na adresu
    if( bind( socket_des, (struct sockaddr *)&address, sizeof(address) ) < 0 ){

        perror("Chyba pri funckii bind()");
        return 1;

    }

    if( listen( socket_des, 5) < 0 ){

        perror("Chyba pri funckii listen()");
        return 1;

    }

    // Nekonečný cyklus
    while( true ){

        int new_socket = accept( socket_des, (struct sockaddr *)&address, (socklen_t *)&address_len);

        if( new_socket >= 0 ){

            FILE * socket_fd = fdopen( new_socket, "r" );

            // Dostan iba prvy riadok z poziadavku
            char *first_line = fgets( message, 10000, socket_fd );

            // Dostan z prveho riadku prve slovo
            char *word = strtok( first_line, " " );

            // Ak je prve slovo "GET"
            if( strcmp( word, "GET" ) == 0 ){
                
                // Vymaz z prveho riadku "GET "
                first_line = first_line + 4;

            }else{

                // Posli odpoved o zlej ziadosti
                send( new_socket, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\n", 56, 0 );

            }

            // Dostan z prveho riadku druhe slovo
            word = strtok( first_line, " " );

            // Rozpoznaj poziadavok
            if( strcmp( word, "/hostname" ) == 0 ){
                
                // Create a HTTP response for hostname
                send( new_socket,  hostname_response, strlen( hostname_response ), 0 );

            }else if( strcmp( word, "/cpu-name" ) == 0 ){

                // Send the HTTP response for cpu name
                send( new_socket, cpu_name_response, strlen( cpu_name_response ), 0 );

            }else if( strcmp( word, "/load" ) == 0 ){

                // Dostan vyuzitie procesora a skontroluj vysledok operacie
                if( get_cpu_usage( &cpu_usage ) ){

                    // Vytvor HTTP odpoved pre vyuzitie procesora
                    strcpy( cpu_usage_response, http_response );
                    char cpu_usage_string[1024];
                    sprintf( cpu_usage_string, "%f", cpu_usage );
                    strcat( cpu_usage_response, cpu_usage_string );
                    strcat( cpu_usage_response, "%" );

                    // Posli HTTP odpoved pre vyuzitie procesora
                    send( new_socket, cpu_usage_response, strlen( cpu_usage_response ), 0 );

                }else {

                    send( new_socket, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain;\r\n\r\n", 66, 0 );

                }

            }else{

                send( new_socket, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\n", 56, 0 );

            }

            close( new_socket );

        }

    }

    return 0;

}