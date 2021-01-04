
PSP_MODULE_INFO("Luminex", 0, 1, 1);

//#define DEBUG				1

#ifndef DEBUG

#define SPLASH_TIME			5.0f
#define SPLASH_SCREEN		1
#define WITH_SOUND			1
#define DebugLog(str)		/* */

#else

void DebugLog(const char *str)
{	
	FILE *fp = fopen("log.txt","a");
	fwrite(str,strlen(str),1,fp);
	fwrite("\r\n",2,1,fp);
	fclose(fp);
}
#endif

#define printf pspDebugScreenPrintf

#define RGB(r, g, b) 		((b << 16) | (g << 8) | r)
#define FRAME_RATE			60
#define SQUARE_SIZE			20
#define BOARD_SIZEX			16
#define BOARD_SIZEY			10
#define BOARD_WIDTH			(SQUARE_SIZE * BOARD_SIZEX)
#define BOARD_HEIGHT		(SQUARE_SIZE * BOARD_SIZEY)
#define BOARD_STARTX		80
#define BOARD_STARTY		60
#define BOARD_COLOR			RGB(150,150,150)
#define FALL_SPEED_FAST		40.0f
#define METRONOME_SPEED		1.3f
#define MOVE_SPEED			6.0f
#define WAIT_SECONDS		1.2f
#define GAME_MENU_X			140
#define GAME_MENU_Y			110
#define GAME_MENU_WIDTH		200
#define GAME_MENU_HEIGHT	80


typedef struct Vertex
{
	float u,v;
	unsigned int c;
	float x,y,z;
}Vertex;

typedef struct Rect
{
	int l;
	int r;
	int t;
	int b;
}Rect;

typedef struct Square
{
	int color;
	int fusion;
	int master;
	int deleted;
	int explored;	
}Square;

