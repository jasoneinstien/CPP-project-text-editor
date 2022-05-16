/*include */
#include <ctype.h> //iscntrl
#include <stdio.h> //printf , perror
#include <unistd.h> //read , stdin_fileno
#include <termios.h>//ECHO  ,struct termios  , ICANON , ISIG
#include <stdlib.h> //atexit , exit
#include <errno.h> //errno , define error to interger 
#include <sys/ioctl.h> //iotcl , TIOCHWINSI , struct winsize
#include <string.h>

//define
#define CTRL_KEY(k) ((k) & 0x1f)


/*initial*/
//store as global variable such that every where can use it

struct editorConfig{
    int screenrows;
    int screencols;
    struct termios orig_termios;
};
//allow user to change config
struct editorConfig E;

//error handling
void die(const char *s){
    //given an error msg when s is not legal
    //clear screen when exit 
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit (1);
}   


//disable raw mode when quit
//tcsaflush means when it is input it will change 
void disableRawMode(){
    if(tcsetattr(STDIN_FILENO ,TCSAFLUSH, &E.orig_termios) == -1){
        die("tcsetattr");
    }
}


//turn off 4 flags 
void enableReadMode(){
    if(tcgetattr(STDIN_FILENO , &E.orig_termios) == -1){
        die("tcgetattr");
    }
    
    //know about which termios in the terminal now 
    tcgetattr(STDIN_FILENO , &E.orig_termios);

    //when the programe is exit it move to disable mode 
    //it can be quit by "quit()"
    atexit(disableRawMode);

    //make a copy and change raw instead of orig_termios
    struct termios raw = E.orig_termios; 

    //IXON is a input function this diable cntrol-s and q
    //ICRNL fix control-M and enter become 13 differnet from enter 
    //iflag means input value 
    //umm other are juse umm kind of useless 

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    //disable the /r function , type it yourself if you want this output new line = \n\r
    raw.c_oflag &= ~(OPOST);

    //ECHO: print word on screen 
    //ICANON: no need enter   
    //ISIG: disable control-c control-z function
    //IEXTEN: disable control-c and control-v function
    // ~ means not ==> this will disable the echo function  
    //raw struct c_falg , it is a local flag 

    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO|ISIG|IEXTEN|ICANON);
    //VMIN means the minimum number of bytes need 
    raw.c_cc[VMIN] = 0;

    //set maximum amount of time need 
    raw.c_cc[VTIME] = 1;

    //TCSAFLUSH is discrible when to apply the change 
    //bitwise and this value 
    //test die function , if this fail return error 1
   if(tcsetattr(STDIN_FILENO , TCSAFLUSH , &raw) == -1){
        die("tcsetattr");
   }
}
//read key function , wait for kepress , return it
char editorReadKey(){
    int nread;
    char c; 
    //while there are still bytes exist , continue read 
    //if there is not more byte to read &c will become 0 , != 0 , end the loop
    //or when q button is press and end the while loop will end 

    while((nread = read(STDIN_FILENO , &c , 1)) != 1 ){
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    return c;
}

int getCursorPosition(int *rows , int*cols){ 

    char buf[32]; 
    unsigned int i = 0;

    if(write(STDIN_FILENO , "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buf)-1){
        if(read(STDIN_FILENO , &buf[i] , 1) != 1 ) break; 
        if(buf[i] == 'R') break; 
        i++;
    }
    //return last character as null character 
    buf[i] = '\0';
    //make sure it is responsive for the esc sequence
    if(buf[0] != '\x1b' || buf[1] != '[')return -1;

    if(sscanf(&buf[2] , "%d;%d" , rows , cols) != 2)return -1;

    return 0; 
}



int getWindowSize(int* rows , int *cols){
    struct winsize ws;

    //ioctl will give the number of roles and column , high etc information of the termnial 
    if(ioctl(STDOUT_FILENO , TIOCGWINSZ , &ws) == -1 || ws.ws_col == 0){
        //b: move the cursor to the right , c: move the cursor down , 999 ensure it move enough time to see rows and columns
        if(write(STDIN_FILENO , "\x1b[999C\x1b[999B" , 12) != 12) return -1;
        return getCursorPosition(rows , cols);
       return -1; 
    }else{
        //ws_col and ws_row is defined in sturct winsize
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0; 
    }
}
//dynamic string --> not in c but int cpp :)
struct abuf{
    char *b;
    int len; 
};

#define ABUF_INIT {NULL , 0}; 


void abAppend(struct abuf *ab , const char *s , int len){
    char *new = realloc(ab ->b , ab->len + len);

    if(new == NULL) return;
    memcpy(&new[ab->len] , s , len);
    ab -> b = new;      
    ab -> len += len;
}

void abFree(struct abuf* ab){
    free(ab -> b); 
}

void editorDrawRow(){
    //open 24 new line with ~ (when you use wrtie function , STDOUT_FILENO means standard input of keyboard , what you want to show , byte)
    int y; 
    for(y = 0 ; y <E.screenrows ; y++){
        write(STDOUT_FILENO , "~\r\n" , 3);
    }
    if(y <E.screenrows -1){
        write(STDOUT_FILENO , "\r\n" , 2);
    }
}

//by the tutorial this is basically the basic structure of clear the screen
void editorRefreshScreen(){
    //write means write 4 bytes out to the terminal 
    //4 means 4 bite 
    //x1b means esc key 
    //most important key is J , which is clear whole screen  key 2 means clear all , key 1 mean clear to the cursor 
    write(STDIN_FILENO , "\x1b[2J", 4);//clear screen 
    write(STDIN_FILENO , "\x1b[H" , 3);//reposition the curson , H function is locate curson default to 1 , 1 (row , column)
    editorDrawRow();

    write(STDOUT_FILENO, "\x1b[H", 3);
}




//read the key handle special function like ctrl-q  

void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

//initialize all the field of E ==> use & such that change all the stuff when pass through the function 
void initEditor(){
    if(getWindowSize(&E.screenrows , &E.screencols) == -1) die("getWindowSize");
}

//main loop 
int main(){

    enableReadMode();

    initEditor();
    //c is the character for tracking the input of the file
    char c;

    while (1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
