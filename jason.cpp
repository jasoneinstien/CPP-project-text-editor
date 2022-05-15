#include <unistd.h> //read , stdin_fileno
#include <termios.h>//ECHO  ,struct termios 
#include <stdlib.h> 

struct termios orig_termios;


//disable raw mode when quit
void disableRawMode(){
    tcsetattr(STDIN_FILENO ,TCSAFLUSH, &asynchronous);

}


//turn off 4 flags 
void enableReadMode(){

    tcgetattr(STDIN_FILENO , &orig_termios);

    atexit(disableRawMode);

    struct termios raw = orig_termios; 

    

    //ECHO: It is used to shwo what you are typing in the terminal , cannot be used in texteditor , so turn it off      
    // ~ means not ==> this will disable the echo function  
    //raw struct c_falg , it is a local flag 
    raw.c_lflag &= ~(ECHO);

    //TCSAFLUSH is discrible when to apply the change 
    //bitwise and this value 
    tcsetattr(STDIN_FILENO , TCSAFLUSH , &raw);
}



int main(){

    enableReadMode();

    //c is the character for tracking the input of the file
    char c;
    //while there are still bytes exist , continue read 
    //if there is not more byte to read &c will become 0 , != 0 , end the loop
    //or when q button is press and end the while loop will end 
    while (read(STDIN_FILENO , &c , 1) == 1 && c != 'q');

    return 0;
}
