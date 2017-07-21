//Created by Alessandro Nichelini in 20/07/2017

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXWIDTH 1024
#define MAXDEPTH 255


int main(void){
  //Imposto il main in modo che l'input venga analizzato carattere per carattere.
  //Inizializzo un buffer di dimensione variabile che incremento ogni volta che eccede
  //passando alla successiva potenza di due.
  int buffer_lenght = 2; //la lunghezza iniziale del buffer è di 2
  int buffer_lenght_index = 1;
  int actual_buffer_size = 0;
  char c = getc(stdin);
  char *buffer = calloc(buffer_lenght, sizeof(char));
  buffer[actual_buffer_size] = c;

  while( 1 != 0 ){

    while( c != '\n'){ //ciclo che viene ripetuto per ogni riga del file di input
      c = getc(stdin); //prendo il carattere successivo
      if( actual_buffer_size < buffer_lenght) //controllo di non ecceddere il buffer
        actual_buffer_size++;
      else{ //in caso contrario lo espamdo riallocando lo spazio necessario
        buffer_lenght_index++;
        buffer_lenght = pow(2, buffer_lenght_index);
        buffer = realloc(buffer, buffer_lenght);
        printf("reallocating: %i\n", buffer_lenght);
        actual_buffer_size++;
      }
      buffer[actual_buffer_size] = c; //salvo nel buffer l'effettivo carattere letto
    } //fine del while annidato

    printf("%s\n", buffer );
    printf("here: %s\n", buffer);
    if( strcmp(buffer, "exit\n") == 0){ //condizione di uscita dal ciclo e dal programma
      exit(1);
    } //chiusura della condizione di uscita

    free(buffer); //libero la memoria allocata per il buffer del comando precedente
    buffer_lenght = 2; //ripristino parametri
    buffer_lenght_index = 1; //ripristino parametri
    actual_buffer_size = 0; //ripristino parametri
    buffer = calloc(buffer_lenght, sizeof(char));
    c = getc(stdin);
    buffer[actual_buffer_size] = c;

  }//fine del while principale
}//fine del main
