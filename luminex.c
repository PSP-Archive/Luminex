//by pointbat. http://pbat.cjb.net

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
 
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psputils.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psppower.h>
#include <psprtc.h>
  
#include <png.h>

#include "luminex.h"
#include "callbacks.h"
#include "graphics.h"
#include "screenshot.h" 
#include "mp3player.h"
 

//images
Image *bg=NULL;
Image *ui=NULL;
Image *font=NULL;
Image *square=NULL;

//variables utilisées pour le calcul du framerate
u64 last_tick;
u32 tick_res;
int frame_count;
float curr_fps;
float curr_ms;
 
int button_pressed;
int bottom_pressed;
int exit_game;
int frame_index;
int posX;
int posY;
int piece;
int next_piece;
Square board[BOARD_SIZEY][BOARD_SIZEX];
int left_landed;
int right_landed;
int seconds;
int level;
int score;
int deleted;
int total_squares;
int fusions;
int metronome_pos;
int metronome_col;
int move_timer;
int move_speed;
int fall_timer;
int paused;
int game_over;
int menu_selection;
Color color1= RGB(255,0,0);
Color color2= RGB(0,0,255);
char themes[50][255];
int theme_count;
int theme_index;


//chaine hexa=>valeur numérique
u32 atox(const char *str)
{	
	u32 len = strlen(str);
	if (len == 0)
		return 0;
	u32 mult=1;
	u32 val;
	u32 nb=0;
	u32 c;
	while (((int)(--len))>=0)
	{
		c = (u32)str[len];
		if (c >= '0' && c <= '9')
			val = c - '0';
		else if (c >= 'A' && c <= 'F')
			val = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			val = c - 'a' + 10;
		else
			return nb;
		nb += (val * mult);
		mult<<=4;
	}
	return nb;
} 
 
 
//la même que dans graphics.c, avec le blend :)
//Rq: mode 888 uniquement
void blitAlphaImageToImage2(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination)
{
	Color* destinationData = &destination->data[destination->textureWidth * dy + dx];
	int destinationSkipX = destination->textureWidth - width;
	Color* sourceData = &source->data[source->textureWidth * sy + sx];
	int sourceSkipX = source->textureWidth - width;
	int x, y;
	for (y = 0; y < height; y++, destinationData += destinationSkipX, sourceData += sourceSkipX) 
	{
		for (x = 0; x < width; x++, destinationData++, sourceData++) 
		{
			Color c1 = *sourceData;
			
			u32 a1 = (c1 >> 24);
			if (a1 != 0)
			{
				u32 r1 = (c1 & 0x00FF0000) >> 16;
				u32 g1 = (c1 & 0x0000FF00) >> 8;
				u32 b1 = c1 & 0xFF; 
				
				Color c2 = *destinationData;
				u32 a2 = 255 - a1;
				u32 r2 = (c2 & 0x00FF0000) >> 16;
				u32 g2 = (c2 & 0x0000FF00) >> 8;
				u32 b2 = c2 & 0xFF; 
			 
			 	u32 r3 = (r2*a2)/255 + (r1*a1)/255;
			 	if (r3 > 255)
			 		r3 = 255;
			 		
			 	u32 g3 = (g2*a2)/255 + (g1*a1)/255;
			 	if (g3 > 255)
			 		g3 = 255;	
			 
			 	u32 b3 = (b2*a2)/255 + (b1*a1)/255;
			 	if (b3 > 255)
			 		b3 = 255;	
			
				*destinationData = 0xFF000000 | (r3<<16) | (g3<<8) | b3;
			}
		}
	}
} 

void LoadTheme(const char *name)
{
	char buff[255];
	
	sprintf(buff,"data\\themes\\%s\\bg.png",name);
	Image *tmp = loadImage(buff);
	if (tmp != NULL)
	{ 
		if (bg != NULL)
			freeImage(bg);
		bg = tmp;
	}
	else
	{
		DebugLog("Unable to load bg.png");
	}
		
	sprintf(buff,"data\\themes\\%s\\ui.png",name);
	tmp = loadImage(buff);
	if (tmp != NULL)
	{
		if (ui != NULL)
			freeImage(ui);
		ui = tmp;
	}
	else
	{
		DebugLog("Unable to load ui.png");
	}
	
	blitAlphaImageToImage2(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,ui,0,0,bg);
		
	sprintf(buff,"data\\themes\\%s\\square.png",name);		
	tmp = loadImage(buff);
	if (tmp != NULL)
	{
		if (square != NULL)
			freeImage(square);
		square = tmp;	
	}
	else
	{
		DebugLog("Unable to load square.png");
	}
		
	sprintf(buff,"data\\themes\\%s\\theme.conf",name);	
	FILE *fp = fopen(buff,"r");
	if (fp != NULL)
	{
		while (fgets(buff,sizeof(buff),fp) != NULL)
		{
			char *value = strchr(buff,'=');
			if (value != NULL)
			{
				char *key = buff;
				*value++=0;
				value[strlen(value)-1]=0;
				if (!strcmp(key,"square_color1"))
					color1 = atox(&value[2]);
				else if (!strcmp(key,"square_color2"))
					color2 = atox(&value[2]);
			}
		}
		fclose(fp);
	}	
	else
	{
		DebugLog("Unable to load theme.conf");
	}
}

void EnumThemes()
{
	theme_index=-1;
	theme_count=0;
	memset(themes,0,sizeof(themes));
	/*int dfd = sceIoDopen("./data/themes/");
	if(dfd > 0)
	{
		printf("3\n");
		int i=0;
		SceIoDirent dir;
		while(sceIoDread(dfd, &dir) > 0 && i++<49)
		{
			printf("%d=>\n",i);
			printf(dir.d_name);
			printf("\n");
			if(dir.d_stat.st_attr & FIO_SO_IFDIR)
			{
				if(dir.d_name[0] != '.')
				{		
					if (!strcmp(dir.d_name,"default"))
						theme_index = theme_count;
					strcpy(themes[theme_count++],dir.d_name);
				}
			} 				
		}
		sceIoDclose(dfd);
	}*/
	
	DIR *dir = opendir("./data/themes/");
	if(dir != NULL)
	{
		struct dirent *direntry;
		while((direntry = readdir(dir)) != NULL)
		{		
			if (direntry->d_name[0] != '.')
			{
				if (!strcmp(direntry->d_name,"DEFAULT"))
					theme_index = theme_count;
				strcpy(themes[theme_count++],direntry->d_name);			
			}
		}
		closedir(dir);
	}
}

void NextPiece()
{
	posY=-2;
	posX=BOARD_SIZEX/2;
	piece=next_piece;
	next_piece=(((float)rand())/RAND_MAX) * 16;
	left_landed=0;
	right_landed=0;
	move_timer=0;
	move_speed=0;
	fall_timer=(int)((float)FRAME_RATE*(float)WAIT_SECONDS);
}

void CheckFusion(int x, int y)
{
	int c1 = board[y][x].color;
	int c2 = board[y][x+1].color;
	int c3 = board[y+1][x+1].color;
	int c4 = board[y+1][x].color;
	if (c1 && c2 && c3 && c4)
	{
		int fusion=0;
		if (c1==1 && c2==1 && c3==1 && c4==1)
		{
			fusion=1;
		}
		else if (c1==2 && c2==2 && c3==2 && c4==2)
		{
			fusion=1;
		}
		if (fusion)
		{
			board[y][x].fusion=1;
			board[y][x].master=1;
			
			board[y][x+1].fusion=1;
			board[y+1][x+1].fusion=1;
			board[y+1][x].fusion=1;
		}
	}
}
 
void LandTest()
{
	int check_fusions=0;
	if (!left_landed)
	{
		if (posY>=(int)(BOARD_SIZEY-2) || board[posY+2][posX].color || board[posY+2][posX].fusion)
		{
			left_landed=1; 
			
			//pose le coté gauche de la piece
			board[posY][posX].color = ((piece&1)!=0)+1;
			board[posY+1][posX].color = ((piece&8)!=0)+1;
			
			check_fusions=1;
		} 
	}	
	if (!right_landed)
	{
		if (posY>=(int)(BOARD_SIZEY-2) || board[posY+2][posX+1].color || board[posY+2][posX+1].fusion)
		{
			right_landed=1;
			
			//pose le coté droit de la piece
			board[posY][posX+1].color = ((piece&2)!=0)+1;
			board[posY+1][posX+1].color = ((piece&4)!=0)+1;
			
			check_fusions=1;
		}
	}
	
	if (posY<0 && (left_landed || right_landed))
	{
		left_landed=1;
		right_landed=1;
		game_over=1;	
		return;
	}
	
	if (check_fusions)
	{
		CheckFusion(posX, posY); 
		if (posX>0)
		{
			CheckFusion(posX-1, posY);
			if (posY<BOARD_SIZEY-2)
				CheckFusion(posX-1, posY+1);
		}
		if (posY<BOARD_SIZEY-2)
			CheckFusion(posX, posY+1);
		if (posX<BOARD_SIZEX-2)
		{
			if (posY<BOARD_SIZEY-2)
				CheckFusion(posX+1, posY+1);
			CheckFusion(posX+1, posY);
		}
	}
} 
  
int DestroyFusionRec(int i, int j)
{
	if (board[i][j].fusion && !board[i][j].color && !board[i][j].deleted)
	{
		if (board[i][j].master)
			deleted++;
		board[i][j].deleted=1;
		if (i>0)
			DestroyFusionRec(i-1,j);	
		if (j>0)
			DestroyFusionRec(i,j-1);
		if (i<BOARD_SIZEY-1)
			DestroyFusionRec(i+1,j);	
		if (j<BOARD_SIZEX-1)
			DestroyFusionRec(i,j+1);
		return 1;
	}
	return 0;
} 

int GetFusionDimensionsRec(int i, int j, Rect *pRect)
{ 
	if (board[i][j].fusion && !board[i][j].color && !board[i][j].explored)
	{
		board[i][j].explored=1;
		if (i>0)
		{
			if (GetFusionDimensionsRec(i-1,j,pRect))
			{
				if (i-1 < pRect->t)
					pRect->t = i-1;	
			}
		}
		if (j>0)
 		{
 			if (GetFusionDimensionsRec(i,j-1,pRect))
			{
				if (j-1 < pRect->l)
					pRect->l = j-1;
			}
		}
		if (i<BOARD_SIZEY-1)
		{
			if (GetFusionDimensionsRec(i+1,j,pRect))
			{
				if (i+1>pRect->b)
					pRect->b = i+1;
			}
		}
		if (j<BOARD_SIZEX-1)
		{
			if (GetFusionDimensionsRec(i,j+1,pRect))
			{
				if (j+1>pRect->r)
					pRect->r=j+1;
			}
		}
 		return 1;
	}
 	return 0;
}
  
int FallBlocs()
{
	int i,j;
	int combo=0;
	
	//fait tomber les blocs
	for (j=BOARD_SIZEX-1;j>=0;j--)
	{
		for (i=BOARD_SIZEY-1;i>=0;i--)
		{
			if (board[i][j].deleted)
			{
				int l;
				for (l=i;l>=1;l--)
				{
					board[l][j].color = board[l-1][j].color;
					board[l][j].deleted=board[l-1][j].deleted;
					board[l][j].fusion=0;
					board[l][j].master=0;
				} 
				combo++;
				board[0][j].color=0;
				board[0][j].deleted=0;
				board[0][j].fusion=0;
				board[0][j].master=0;
				i++; 
			}
		}
	}
			
	//reforme de nouvelles fusions
	for (j=0;j<BOARD_SIZEX-1;j++)
		for(i=0;i<BOARD_SIZEY-1;i++)
			CheckFusion(j,i);  
			
	return combo;
}
  
void DestroyFusions()
{
	int fusion_on_this_line=0;
	
	//marque les blocs fusionnés de la colonne courante comme "passés"
	int i,j;
	for (i=BOARD_SIZEY-1;i>=0;i--)
	{
		if (board[i][metronome_col].fusion)
		{
			fusion_on_this_line=1;
			if (board[i][metronome_col].master)
				fusions++;
			board[i][metronome_col].color=0;
		} 
	}
	
	if (!fusion_on_this_line)
	{
		fusions=0;
		int k = metronome_col>0? metronome_col-1 : BOARD_SIZEX-1;
		while(k>=0)
		{
			for(i=BOARD_SIZEY-1;i>=0;i--)
			{
				j=k;
				Rect r;
				r.l = j;
				r.r = j;
				r.t = i;
				r.b = i;
				if (GetFusionDimensionsRec(i,j,&r) && r.r>r.l && r.b>r.t) 
					DestroyFusionRec(i,j);
				
				//aucun bloc exploré pour l'instant
				int l;
				for (l=BOARD_SIZEY-1;l>=0;l--)
					for (j=BOARD_SIZEX-1;j>=0;j--)
						board[l][j].explored=0;
			}
			k--;
		}
	
		//Calcul du score
		int combo = FallBlocs();
		
		if (combo)
			score += 10 * (combo+combo-1);
	}
} 

void InitGame()
{
	sceRtcGetCurrentTick(&last_tick);
	tick_res = sceRtcGetTickResolution();
	frame_count = 0;
	curr_fps=0;
	curr_ms = 1.0f;
	
	srand(time(0));
	button_pressed=1;
	frame_index=0;
	exit_game=0;
	seconds=0;
	level=1;
	score=0;
	deleted=0;
	total_squares=0;
	metronome_pos=0;
	metronome_col=0;
	paused=0;
	game_over=0;
	menu_selection=0;
	fusions=0;

	next_piece=(((float)rand())/RAND_MAX) * 16;
	NextPiece();
	
	memset(board,0,sizeof(board));
}
 
int Init()
{  
	pspDebugScreenInit();
	SetupCallbacks();
	
	printf("Loading font...\n");
	font = loadImage("data\\font.png");
	if (font == NULL)
		return 0;	
	
	printf("Enumerating themes...\n");
	EnumThemes();
	if (theme_index == -1)
		return 0;
		
	printf("Loading default theme...\n");
	LoadTheme(themes[theme_index]);

	printf("Loading sound...\n");		
#ifdef WITH_SOUND
	scePowerSetClockFrequency(333, 333, 166);
	pspAudioInit();
	MP3_Init(1);
    MP3_Load("data\\themes\\default\\sound.mp3");
    MP3_Play();	
#endif		
		
	printf("Graphics initialisation...\n");	
	initGraphics();
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);	
	
	return 1;
}

void DrawText(int x, int y, char *str)
{
	int i,length = strlen(str);
    Vertex *vertices = sceGuGetMemory(sizeof(Vertex) * 2 * length);
    guStart();
    sceGuTexImage(0, font->textureWidth, font->textureHeight, font->textureWidth, (void*) font->data);
    for (i=0;i<length;i++)
    {
    	char c = str[i];
        int text_x;
        if (c>='0' && c<='9')
        	text_x = (c-'0'+26);
        else if (c==' ')
        	text_x = 63;
        else if (c==':')
        	text_x = 36;
        else
        	text_x = (c-'A');
        text_x *= 8;
        vertices[i*2].u = text_x;
        vertices[i*2].v = 0;
        vertices[i*2].c = 0;
        vertices[i*2].x = x;
        vertices[i*2].y = y;
        vertices[i*2].z = 0;
        vertices[i*2+1].u = text_x+8;
        vertices[i*2+1].v = 8;
        vertices[i*2+1].c = 0;
        vertices[i*2+1].x = x+8;
        vertices[i*2+1].y = y+8;
        vertices[i*2+1].z = 0;
        x += 8;
    }
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,length*2, 0, vertices);
    sceGuFinish();
    sceGuSync(0, 0); 
}

void DrawLine(int x1, int y1, int x2, int y2, Color color)
{
	guStart();
	ScePspSVector3 *vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 2);
    sceGuDisable(GU_TEXTURE_2D); 
    //sceGuEnable(GU_LINE_SMOOTH);
    sceGuColor(color);
    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 0;
    vertices[1].x = x2;
    vertices[1].y = y2;
    vertices[1].z = 0;
    sceGuDrawArray(GU_LINES,GU_VERTEX_16BIT | GU_TRANSFORM_2D,2, 0, vertices);
    //sceGuDisable(GU_LINE_SMOOTH);
    sceGuEnable(GU_TEXTURE_2D); 
    sceGuFinish();
    sceGuSync(0, 0); 
}

void DrawMenuSelection(int y)
{
	guStart();
	sceGuDisable(GU_TEXTURE_2D); 
	ScePspSVector3 *vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 4);
    vertices[0].x = GAME_MENU_X;
    vertices[0].y = y;
    vertices[0].z = 0;
    vertices[1].x = GAME_MENU_X+GAME_MENU_WIDTH;
    vertices[1].y = y;
    vertices[1].z = 0;
    vertices[2].x = GAME_MENU_X+GAME_MENU_WIDTH;
    vertices[2].y = y+8;
    vertices[2].z = 0;
    vertices[3].x = GAME_MENU_X;
    vertices[3].y = y+8;
    vertices[3].z = 0;
    sceGuColor(RGB(0,255,0));
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_VERTEX_16BIT | GU_TRANSFORM_2D,4,0, vertices);
    sceGuEnable(GU_TEXTURE_2D); 
	sceGuFinish();
	sceGuSync(0, 0);
}

void DrawMenu()
{
	//un petit rectangle noir transparent derrière
	guStart();
	sceGuDisable(GU_TEXTURE_2D);
	sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_ONE_MINUS_SRC_ALPHA, 0, 0); 
	sceGuColor(RGB(80,80,80));
	ScePspSVector3 *vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 4);
		
    vertices[0].x = GAME_MENU_X;
    vertices[0].y = GAME_MENU_Y;
    vertices[0].z = 0;
    
    vertices[1].x = GAME_MENU_X + GAME_MENU_WIDTH;
    vertices[1].y = GAME_MENU_Y;
    vertices[1].z = 0;
    
    vertices[2].x = GAME_MENU_X + GAME_MENU_WIDTH;
    vertices[2].y = GAME_MENU_Y + GAME_MENU_HEIGHT;
    vertices[2].z = 0;
    
    vertices[3].x = GAME_MENU_X;
    vertices[3].y = GAME_MENU_Y + GAME_MENU_HEIGHT;
    vertices[3].z = 0;
    
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_VERTEX_16BIT | GU_TRANSFORM_2D,4,0, vertices);
    sceGuEnable(GU_TEXTURE_2D); 
    
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0, 0);
	
	if (!game_over)
	{
		DrawText(220,120,"PAUSED");
		if (menu_selection==0)
	    	DrawMenuSelection(150);
	    DrawText(220,150,"RESUME");
	}
	else
	{
		DrawText(205,120,"GAME OVER");
		
		char buff[100];
		sprintf(buff, "SCORE:%d", (int)score);
    	DrawText(205,130,buff);
	}
    
    if (menu_selection==1)
    	DrawMenuSelection(160);
    DrawText(220,160,"RESTART");
    
    if (menu_selection==2)
    	DrawMenuSelection(170);
    DrawText(220,170,"EXIT");
}

void DrawMetronome()
{
	//ligne verticale verte
	int x = BOARD_STARTX+metronome_pos;
	int y = BOARD_STARTY-20;
	DrawLine(x,y,x,y + BOARD_HEIGHT+20, RGB(0,255,0));
	
	//un petit rectangle vert transparent derrière
	guStart();
	sceGuDisable(GU_TEXTURE_2D);
	sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_ONE_MINUS_SRC_ALPHA, 0, 0); 
	sceGuColor(RGB(0,255,0));
	ScePspSVector3 *vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 4);
    vertices[0].x = x-3;
    vertices[0].y = y;
    vertices[0].z = 0;
    vertices[1].x = x;
    vertices[1].y = y;
    vertices[1].z = 0;
    vertices[2].x = x;
    vertices[2].y = y+BOARD_HEIGHT+20;
    vertices[2].z = 0;
    vertices[3].x = x-3;
    vertices[3].y = y+BOARD_HEIGHT+20;
    vertices[3].z = 0;
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_VERTEX_16BIT | GU_TRANSFORM_2D,4,0, vertices);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0, 0);
	
	//rectangle derrière du metronome
	guStart();
	vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 4);
    vertices[0].x = x-10;
    vertices[0].y = y;
    vertices[0].z = 0;
    vertices[1].x = x;
    vertices[1].y = y;
    vertices[1].z = 0;
    vertices[2].x = x;
    vertices[2].y = y+10;
    vertices[2].z = 0;
    vertices[3].x = x-10;
    vertices[3].y = y+10;
    vertices[3].z = 0;
    sceGuColor(RGB(0,0,0));
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_VERTEX_16BIT | GU_TRANSFORM_2D,4,0, vertices);
    sceGuEnable(GU_TEXTURE_2D); 
    sceGuFinish();
	sceGuSync(0, 0);
	  
	char buff[5];
	sprintf(buff,"%d",fusions);
	DrawText(x-9,y,buff);
    
    //flèche du métronome
    guStart();
	vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 4);
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0;
    vertices[1].x = x+5;
    vertices[1].y = y+4;
    vertices[1].z = 0;
    vertices[2].x = x; 
    vertices[2].y = y+10;
    vertices[2].z = 0;
    sceGuDisable(GU_TEXTURE_2D); 
    sceGuDrawArray(GU_TRIANGLES,GU_VERTEX_16BIT | GU_TRANSFORM_2D,3,0, vertices);
	sceGuEnable(GU_TEXTURE_2D); 
	sceGuFinish();
	sceGuSync(0, 0);
}

 
void DrawSquare(int dx, int dy, int type)
{
	guStart();
    Vertex *vertices = sceGuGetMemory(sizeof(Vertex) * 2);
    sceGuTexImage(0, square->textureWidth, square->textureHeight, square->textureWidth, (void*) square->data);
    int text_x = type?SQUARE_SIZE:0;
    vertices[0].u = text_x;
    vertices[0].v = 0;
    vertices[0].c = 0;
    vertices[0].x = dx+1;
    vertices[0].y = dy+1;
    vertices[0].z = 0;
    vertices[1].u = text_x+SQUARE_SIZE;
    vertices[1].v = SQUARE_SIZE;
    vertices[1].c = 0;
    vertices[1].x = dx+SQUARE_SIZE;
    vertices[1].y = dy+SQUARE_SIZE;
    vertices[1].z = 0;
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,2, 0, vertices);
    sceGuFinish();
    sceGuSync(0, 0); 
}


void DrawSquares()
{
	guStart();
	Vertex *vertices = sceGuGetMemory(sizeof(Vertex) * BOARD_SIZEY * BOARD_SIZEX * 2);
	int i,j,k=0;
	for (i=0;i<BOARD_SIZEY;i++)
	{
		for (j=0;j<BOARD_SIZEX;j++)
		{
			if (board[i][j].color)
			{	
				int x = BOARD_STARTX + j * SQUARE_SIZE;
				int y = BOARD_STARTY + i * SQUARE_SIZE;	
				int text_x = (board[i][j].color-1)*SQUARE_SIZE;
			    vertices[k].u = text_x;
			    vertices[k].v = 0;
			    vertices[k].c = 0;
			    vertices[k].x = x+1;
			    vertices[k].y = y+1;
			    vertices[k].z = 0;
			    k++;
			    vertices[k].u = text_x+SQUARE_SIZE;
			    vertices[k].v = SQUARE_SIZE;
			    vertices[k].c = 0;
			    vertices[k].x = x+SQUARE_SIZE;
			    vertices[k].y = y+SQUARE_SIZE;
			    vertices[k].z = 0;
			    k++;
			}
		}
	}
	if (k>0)
	{
		sceGuTexImage(0, square->textureWidth, square->textureHeight, square->textureWidth, (void*) square->data);
		sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,k,0, vertices);
	}
	sceGuFinish();
	sceGuSync(0, 0);
}

void DrawFusions()
{
	int i,j;
	for (i=0;i<BOARD_SIZEY;i++)
	{
		for (j=0;j<BOARD_SIZEX;j++)
		{
			if (board[i][j].master && board[i][j].fusion)
			{	
				guStart();
				sceGuDisable(GU_TEXTURE_2D); 
				ScePspSVector3 *vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 5);
				int x = BOARD_STARTX + j * SQUARE_SIZE;
				int y = BOARD_STARTY + i * SQUARE_SIZE;	
			    vertices[0].x = x;
			    vertices[0].y = y;
			    vertices[0].z = 0;
			    vertices[1].x = x+SQUARE_SIZE*2;
			    vertices[1].y = y;
			    vertices[1].z = 0;
			    vertices[2].x = x+SQUARE_SIZE*2;
			    vertices[2].y = y+SQUARE_SIZE*2;
			    vertices[2].z = 0;
			    vertices[3].x = x;
			    vertices[3].y = y+SQUARE_SIZE*2;
			    vertices[3].z = 0;
			    if (board[i][j].color)
			    {
			    	sceGuColor((board[i][j].color==1)?color1:color2);
			    	sceGuDrawArray(GU_TRIANGLE_FAN,GU_VERTEX_16BIT | GU_TRANSFORM_2D,4,0, vertices);
			    }
			    vertices[4].x = x;
			    vertices[4].y = y;
			    vertices[4].z = 0;
			    sceGuColor(RGB(255,255,255));
			    sceGuDrawArray(GU_LINE_STRIP,GU_VERTEX_16BIT | GU_TRANSFORM_2D,5,0, vertices);
			    sceGuEnable(GU_TEXTURE_2D); 
				sceGuFinish();
				sceGuSync(0, 0);
			}
		}
	}
}

void Redraw()
{
	char buff[100];
	
	//Dessin du fond
	blitImageToScreen(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,bg,0,0);

	//TODO: opti (lignes horizontales et verticales donc pas besoin de bresenham)
	int i,j;
	
	//lignes horizontales
	for (i=1;i<=BOARD_SIZEY;i++)
		DrawLine(BOARD_STARTX, BOARD_STARTY + i*SQUARE_SIZE, BOARD_STARTX + BOARD_SIZEX*SQUARE_SIZE, BOARD_STARTY + i*SQUARE_SIZE, BOARD_COLOR); 	
	
	//lignes verticales
	for(j=0;j<=BOARD_SIZEX;j++)
		DrawLine(BOARD_STARTX + j*SQUARE_SIZE, BOARD_STARTY, BOARD_STARTX + j*SQUARE_SIZE, BOARD_STARTY + BOARD_SIZEY*SQUARE_SIZE, BOARD_COLOR); 
	
//#ifdef DEBUG
	sprintf(buff, "FPS:%d", (int)curr_fps);
	printTextScreen(10, 10, buff, RGB(200, 55, 55 ));
//#endif	
	
	//Dessine toutes les pieces en une seule fois
	if (total_squares>0)
	{
		DrawSquares();
		DrawFusions();
	}
	
	//Dessin de la pièce courante
	int x = BOARD_STARTX + posX * SQUARE_SIZE;
	int y = BOARD_STARTY + posY * SQUARE_SIZE;	
	
	//coté gauche
	if (!left_landed)
	{
		DrawSquare(x, y,(piece&1)!=0);
		DrawSquare(x, y+SQUARE_SIZE,(piece&8)!=0);
	}
	
	//coté droit
	if (!right_landed)
	{
		DrawSquare(x+SQUARE_SIZE, y, (piece&2)!=0);
		DrawSquare(x+SQUARE_SIZE, y+SQUARE_SIZE,(piece&4)!=0);
	}
	
	DrawMetronome();
	
	//Level
	sprintf(buff, "%d", level);
	DrawText(420,115,buff);
	
	//Time
	sprintf(buff, "%d", seconds);
	DrawText(420,155,buff);

	//Score
	sprintf(buff, "%d", score);
	DrawText(420,195,buff);	
	
	//Deleted
	sprintf(buff, "%d", deleted);
	DrawText(420,238,buff);	
	
	//Next
	DrawText(32,130,"NEXT");
	x=30;
	y=140; 
	DrawSquare(x, y,(next_piece&1)!=0);
	DrawSquare(x, y+SQUARE_SIZE,(next_piece&8)!=0);
	DrawSquare(x+SQUARE_SIZE, y, (next_piece&2)!=0);
	DrawSquare(x+SQUARE_SIZE, y+SQUARE_SIZE,(next_piece&4)!=0);
	
	if (paused || game_over)
		DrawMenu();

	sceDisplayWaitVblankStart();
	flipScreen();
}

void HandleInput()
{
	SceCtrlData pad;
	sceCtrlReadBufferPositive(&pad, 1);
	
	if (pad.Buttons & PSP_CTRL_START)
	{
		if (!button_pressed)
		{
			if (!paused)
			{
#ifdef WITH_SOUND
				MP3_Stop();
#endif				
				paused=1;
				menu_selection=0;	
			}
			else
			{
#ifdef WITH_SOUND
				MP3_Play();
#endif	
				paused = 0;
			}
			button_pressed=1;
			return;
		}
	}
	else
	{ 
		if (!pad.Buttons)
			button_pressed=0;
	}
	
	if (pad.Buttons & PSP_CTRL_SELECT)
	{
		screenshot("screen.png");
		/*DebugLog("Dump");
		int i,j;
		for (i=0;i<BOARD_SIZEY;i++)
			for (j=0;j<BOARD_SIZEX;j++)
			{
				char buff[100];
				sprintf(buff, "%d, %d, couleur %d, fusion %d, master %d, deleted %d, explored %d", 
					i,j,board[i][j].color,board[i][j].fusion, board[i][j].master, board[i][j].deleted, board[i][j].explored);
				DebugLog(buff);
			}*/
	}
	
	if (paused || game_over)
	{
		//la fleche haut ne sert que dans le menu pause
		if (pad.Buttons & PSP_CTRL_UP)
		{
			if (!button_pressed)
			{
				if (menu_selection>game_over)
					menu_selection--;
				button_pressed=1;
			}
		}
		else if (pad.Buttons & PSP_CTRL_DOWN)
		{
			if (!button_pressed)
			{
				if (menu_selection<2)
					menu_selection++;
				button_pressed=1;
			}
		}
		else if (pad.Buttons & PSP_CTRL_CROSS)
		{
			if (!button_pressed)
			{
				switch(menu_selection)
				{
				case 0:
					paused=0;
					break;
				case 1:
					InitGame();
					break;
				case 2:
					exit_game=1;
				}
				button_pressed=1;
			}
		}
		else
		{
			if (!pad.Buttons)
				button_pressed=0;
		}
		return;
	}
		
	if (left_landed || right_landed)
	{
		if (!pad.Buttons)
			button_pressed=0;
		return;
	}
		
	if (!button_pressed)
	{
		if (pad.Buttons & PSP_CTRL_CROSS)
		{
			piece=((piece&1)<<1) | ((piece&2)<<1) | ((piece&4)<<1) | ((piece&8)>>3);
			button_pressed=1;
		}
		else if (pad.Buttons & PSP_CTRL_SQUARE)
		{
			piece=((piece&8)>>1) | ((piece&4)>>1) | ((piece&2)>>1) | ((piece&1)<<3);
			button_pressed=1;
		}
	}
	
	//Déplacements latéraux
	if (pad.Buttons & PSP_CTRL_LEFT)
	{
		if (posX>0)
		{
			if (posY==-2 || (!board[posY][posX-1].color && !board[posY][posX-1].fusion 
							&& !board[posY+1][posX-1].color && !board[posY+1][posX-1].fusion))
			{
				if (move_speed>=0)
				{
					posX--;
					move_speed=-MOVE_SPEED;	
					move_timer = FRAME_RATE/(-move_speed);
				}
				else if (!move_timer)
				{
					posX--;	
					move_speed*=2;
					move_timer = FRAME_RATE/(-move_speed);
				}
			}
		}
	}
	else if (move_speed<0)
	{
		move_speed=0;
		move_timer=0;
	}
	
	if (pad.Buttons & PSP_CTRL_RIGHT) 
	{
		if (posX<BOARD_SIZEX-2)
		{
			if (posY==-2 || (!board[posY][posX+2].color && !board[posY][posX+2].fusion 
							&& !board[posY+1][posX+2].color && !board[posY+1][posX+2].fusion))
			{
				if (move_speed<=0)
				{
					posX++; 
					move_speed=MOVE_SPEED;	
					move_timer=FRAME_RATE/move_speed;
				}
				else if (!move_timer)
				{
					posX++;	
					move_speed*=2;
					move_timer = FRAME_RATE/(move_speed);
				}
			}
		}
	}
	else if (move_speed>0)
	{
		move_speed=0;	
		move_timer=0;
	}
	
	//accélération de la chute
	if (pad.Buttons & PSP_CTRL_DOWN)
	{
		if (!button_pressed || (posY != -2 && !fall_timer))
		{
			LandTest();
			if (!left_landed && !right_landed)
				posY++;
			fall_timer = (int)((float)FRAME_RATE / FALL_SPEED_FAST);
		}
		move_speed=0;
		button_pressed=1;
		bottom_pressed=1;
	}
	else
	{
		bottom_pressed=0;
	}
	
	if (!pad.Buttons)
		button_pressed=0;
}

void ProcessFrame()
{
	if (paused || game_over)
		return;
	
	frame_count++;
	frame_index++;
	
	//chute de la pièce courante
	if (!fall_timer)
	{
		LandTest();
		
		if (left_landed && right_landed)
		{
			if (posY<0) 
			{
				paused=1;
				menu_selection=1;
				game_over=1;	//gameover	
			}
			else
			{
				NextPiece();
				total_squares += 4;
				score += 10;
				if (score > 1000 * level)
				{
					level++;
					theme_index = (theme_index+1) % theme_count;
					LoadTheme(themes[theme_index]);
				}
			}
		}
		else
		{
			if (left_landed || right_landed || bottom_pressed)
				fall_timer = (int)((float)FRAME_RATE / FALL_SPEED_FAST);
			else
				fall_timer = (int)((float)FRAME_RATE / (float)(sqrt(level)));
			posY++;
		}
	}
	else
	{
		fall_timer--;	
	}
	 
	if (move_timer>0)
		move_timer--;
		
	//deplace le metronome
	int speed = (int)((float)FRAME_RATE / (float)(METRONOME_SPEED*SQUARE_SIZE));
	if (!(frame_index % speed))
	{
		//supprime les blocs sur lesquels le metronome vient de passer
		metronome_pos = (metronome_pos+1)%BOARD_WIDTH;
		int col = (metronome_pos) / SQUARE_SIZE;
		if (col != metronome_col)
		{
			metronome_col = col;
			DestroyFusions();
		}
	}
	
	//actualise le compteur de secondes
	if (frame_index == FRAME_RATE)
	{
		frame_index=0;
		seconds++;
	}
	
	//calcul framerate
	u64 curr_tick;
	sceRtcGetCurrentTick(&curr_tick);
	if ((curr_tick-last_tick) >= tick_res)
	{	
		curr_fps = 1.0f / curr_ms;	
		double time_span = ((curr_tick-last_tick)) / (float)tick_res;
		curr_ms = time_span / frame_count;
		frame_count = 0;
		last_tick=curr_tick;	
	}
}

void Clean()
{
#ifdef WITH_SOUND
	MP3_Stop();
    MP3_FreeTune();
#endif
	
	if (font != NULL)
		freeImage(font);
	if (ui != NULL)
		freeImage(ui);
	if (bg != NULL)
		freeImage(bg);
	if (square != NULL)
		freeImage(square);
	
	clearScreen(0);
	sceKernelExitGame();
}

#ifdef SPLASH_SCREEN
int ShowSplash()
{
	Image *splash = loadImage("data\\splash.png");
	if (splash == NULL)
		return -1;	

	int i,frame_count = SPLASH_TIME*FRAME_RATE;
	for(i=0;i<frame_count;i++)
	{
		blitImageToScreen(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,splash,0,0);
	    
	    guStart();
		sceGuDisable(GU_TEXTURE_2D); 
		ScePspSVector3 *vertices = sceGuGetMemory(sizeof(ScePspSVector3) * 4);
	    vertices[0].x = 0;
	    vertices[0].y = 0;
	    vertices[0].z = 0;
	    vertices[1].x = SCREEN_WIDTH;
	    vertices[1].y = 0;
	    vertices[1].z = 0;
	    vertices[2].x = SCREEN_WIDTH;
	    vertices[2].y = SCREEN_HEIGHT;
	    vertices[2].z = 0;
	    vertices[3].x = 0;
	    vertices[3].y = SCREEN_HEIGHT;
	    vertices[3].z = 0; 
	    u32 c;
	    if (i<frame_count/2)
	    	c = (i * 255 * 2)/frame_count;
	    else
	    	c = 255-(((i-(frame_count/2)) * 255 * 2)/frame_count);
	    sceGuColor(RGB(c,c,c));
	    sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_ONE_MINUS_SRC_ALPHA, 0, 0); 
	    sceGuDrawArray(GU_TRIANGLE_FAN,GU_VERTEX_16BIT | GU_TRANSFORM_2D,4,0, vertices);
	    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	    sceGuEnable(GU_TEXTURE_2D); 
		sceGuFinish();
		sceGuSync(0, 0);
		
		sceDisplayWaitVblankStart();
		flipScreen();
	}
	
	freeImage(splash);
	
	return 1;
}
#endif

void ShowStartScreen()
{
	Image *splash = loadImage("data//start.png");
	if (splash == NULL)
		return;	

	SceCtrlData pad;
	while(!(pad.Buttons & PSP_CTRL_START))
	{
		sceCtrlReadBufferPositive(&pad, 1);
		
		blitImageToScreen(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,splash,0,0);
		
		sceDisplayWaitVblankStart();
		flipScreen();
	}
	
	freeImage(splash);
}

int main() 
{
	if (!Init())
		return -1;
	
#ifdef SPLASH_SCREEN
	if (!ShowSplash())
		return -1;
#endif

	ShowStartScreen();
			
	InitGame();
		
	while(!exit_game)
	{
		Redraw();
		HandleInput();
		ProcessFrame();
	} 
	
	Clean();
	
	return 0;
} 
