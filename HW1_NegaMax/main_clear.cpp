#include <stdio.h>
#include <string.h>

// commands enumerate
enum COMMANDS{
  PROTOCOL_VERSION = 0, // 0
  NAME, // 1
  VERSION, // 2
  KNOWN_COMMAND, // 3
  LIST_COMMANDS, // 4
  QUIT, // 5
  BOARDSIZE, // 6
  RESET_BOARD, // 7
  NUM_REPETITION, // 8
  NUM_MOVES_TO_DRAW, // 9
  MOVE, // 10
  FLIP, // 11
  GENMOVE, // 12
  GAME_OVER, // 13
  READY, // 14
  TIME_SETTINGS, // 15
  TIME_LEFT, // 16
  SHOWBOARD // 17
};

int main(){
  char read[1024], write[1024], output[1024], *token;
  const char *data[10];
  int id;
  bool isFailed;

  do{
    // read command
    fgets(read, 1024, stdin);
    fprintf(stderr, "%s", read);
    // remove newline(\n)
    read[strlen(read) - 1] = '\0';
    // get command id
    token = strtok(read, " ");
    sscanf(token, "%d", &id);
    // get command name
    token = strtok(NULL, " ");
    // get command data
    int i = 0;
    while((token = strtok(NULL, " ")) != NULL){
      data[i++] = token;
    }

    write[0] = '\0'; // empty the char array
    
    //TODO: Call AI Function

    if(strlen(write) > 0){
      if(isFailed){
        sprintf(output, "?%d %s\n", id, write);
      }else{
        sprintf(output, "=%d %s\n", id, write);
      }
    }else{
      if(isFailed){
        sprintf(output, "?%d\n", id);
      }else{
        sprintf(output, "=%d\n", id);
      }
    }

    fprintf(stdout, "%s", output);
    fprintf(stderr, "%s", output);
    // important, do not delete
    fflush(stdout);
    fflush(stderr);

  }while(id != QUIT);

  return 0;
}