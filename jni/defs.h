#include "glesutil.h"

#define distance(x1,x2,y1,y2) (sqrtf(powf((x2)-(x1),2)+powf((y2)-(y1),2)))
#define xcorrect(a) ((a)-(state->player.base.x+(PLAYER_WIDTH/2.0f)))
#define ycorrect(b) ((b)-(state->player.base.y+(PLAYER_HEIGHT/2.0f)))

// gameplay
#define TID_BACKGROUND 0
#define TID_PLAYER 1
#define TID_CLOUD 2
#define TID_BULLET 3
#define TID_SMOKE 2
#define TID_MISSILE 4
#define TID_FLASH 5
#define TID_INDICATOR 6
#define TID_ENEMY 7

// ui
#define TID_JOYBASE 0
#define TID_JOYTOP 1
#define TID_JOYFIRE 2
#define TID_TITLE 3
#define TID_BUTTON 4

// sounds
#define SID_BACKGROUND 0

// point values
#define POINTS_MISSILES_COLLIDE 10
#define POINTS_MISSILE_SHOT_DOWN 12
#define POINTS_MISSILE_EVADED 15
#define POINTS_ENEMY_SHOT_DOWN 25

#define GAMEOVER_DELAY 70

#define JOYBASE_SIZE 2.0f
#define JOYTOP_SIZE 1.0f
#define JOYTOP_DIST 1.2f
#define JOYFIRE_SIZE 1.5f
struct base{
	float x,y,w,h,rot;
	float count,frame;
};

#define BUTTON_WIDTH 3.0f
#define BUTTON_HEIGHT 1.19166f
#define BUTTON_PRESS 1
#define BUTTON_ACTIVATE 2
struct button{
	struct base base;
	char *label;
	int active;
};

#define PLAYER_LEFT_BOUNDARY ((state->player.base.x+(PLAYER_WIDTH/2.0f))-state->rect.right)
#define PLAYER_RIGHT_BOUNDARY ((state->player.base.x+(PLAYER_WIDTH/2.0f))+state->rect.right)
#define PLAYER_BOTTOM_BOUNDARY ((state->player.base.y+(PLAYER_HEIGHT/2.0f))+state->rect.bottom)
#define PLAYER_TOP_BOUNDARY ((state->player.base.y+(PLAYER_HEIGHT/2.0f))-state->rect.bottom)
#define PLAYER_WIDTH 0.575f
#define PLAYER_HEIGHT 0.7f
#define PLAYER_SPEED 0.1f
#define PLAYER_RELOAD 20
#define PLAYER_TURN_SPEED 0.06f
#define PLAYER_SMOKE 5
#define PLAYER_TOLERANCE 0.2f // collision tolerance
struct player{
	struct base base;
	float targetrot;
	float xv,yv;
	int reload;
	int dead;
	int timer_smoke;
};

#define ENEMY_WIDTH 0.633f
#define ENEMY_HEIGHT 0.7f
#define ENEMY_SPEED (PLAYER_SPEED-0.003f)
#define ENEMY_MODE_NONE 0
#define ENEMY_MODE_MISSILES 1
#define ENEMY_MODE_DOGFIGHT 2
#define ENEMY_CONE 0.75f
#define ENEMY_FIRE_DIST 5.0f
struct enemy{
	struct base base;
	struct base target;
	int timer_smoke;
	int timer_reload;
	int dead;
	int health;
	struct enemy *next;
};

#define MISSILE_WIDTH 0.3083f
#define MISSILE_HEIGHT 0.1083f
#define INDICATOR_WIDTH 0.7f
#define INDICATOR_HEIGHT 0.55f
#define MISSILE_SPEED 0.116f
#define MISSILE_TURN_SPEED 0.04f
#define MISSILE_SMOKE 4
#define MISSILE_TTL 1400
struct missile{
	struct base base;
	float xv,yv;
	int timer_smoke;
	int dead;
	int ttl;
	float sway;
	struct missile *next;
};

#define BULLET_WIDTH 0.3f
#define BULLET_DMG 9,16
#define BULLET_HEIGHT 0.15f
#define BULLET_SPEED 0.3f
struct bullet{
	struct base base;
	float xv,yv;
	struct base *owner;
	struct bullet *next;
};

#define SMOKE_FADE 0.01f
#define SMOKE_SHRINK 0.002f
struct smoke{
	struct base base;
	float alpha;
	float xv,yv;
	struct smoke *next;
};

#define EXPLOSION_FLASH_COUNT 8
#define EXPLOSION_FLASH_MIN_TIMER 3
#define EXPLOSION_FLASH_MAX_TIMER 18
#define EXPLOSION_FLASH_SIZE_MIN_MULTIPLIER 2.0f
#define EXPLOSION_FLASH_SIZE_MAX_MULTIPLIER 1.7f
#define EXPLOSION_FLASH_MAX_GROW_RATE 0.05f
struct flash{
	struct base base;
	float rgb[3];
	float maxsize;
	int growing;
	int timer_delay;
};
struct explosion{
	struct flash flash[EXPLOSION_FLASH_COUNT];
	struct flash cloud; // background cloud
	struct explosion *next;
};

#define CLOUD_SIZE 2.0f
#define CLOUD_RMDIST 15.0f
struct cloud{
	struct base base;
	struct cloud *next;
};

#define MESSAGE_TTL 120
#define MESSAGE_MAX 50
#define MESSAGE_PINCH 10
#define MESSAGE_Y 3.0f
struct message{
	int ttl;
	char text[MESSAGE_MAX+1];
	struct message *next;
};

struct state{
	int running,showmenu;
	int fire,gameoverdelay;
	float points;

	int vao,vbo,program;
	struct device device,screen;
	struct pack assets,uiassets;
	struct apack aassets;
	struct{float left,right,bottom,top;}rect;
	struct{ftfont *main,*button,*header;}font;
	struct{int vector,size,texcoords,rot,rgba,projection;}uniform;
	struct crosshair pointer[2];
	
	EGLSurface surface;
	EGLContext context;
	EGLDisplay display;
	struct android_app *app;
	struct jni_info jni_info;

	struct base background,joy_base,joy_top,joy_fire;
	struct player player;
	struct enemy *enemylist,*focused_enemy;
	struct missile *missilelist;
	struct bullet *bulletlist;
	struct smoke *smokelist;
	struct cloud *cloudlist;
	struct explosion *explosionlist;
	struct message *messagelist;
};

int process(struct android_app*);
int32_t inputproc(struct android_app*,AInputEvent*);
void cmdproc(struct android_app*,int32_t);
void init_display(struct state*);
void term_display(struct state*);

int button_process(struct crosshair*,struct button*);
void button_draw(struct state*,struct button*);

int menu_main(struct state*);
int menu_end(struct state*);
int menu_message(struct state*,const char*,const char*);

int core(struct state *state);
void render(struct state *state);
void init(struct state*);
void reset(struct state*);

int collide(struct base*,struct base*,float);
void draw(struct state*,struct base*);
void uidraw(struct state*,struct base*);

void newenemy(struct state*);
struct enemy *deleteenemy(struct state*,struct enemy*,struct enemy*);
void newmissile(struct state*,struct enemy*);
struct missile *deletemissile(struct state*,struct missile*,struct missile*);
void newbullet(struct state*,struct base*);
struct bullet *deletebullet(struct state*,struct bullet*,struct bullet*);
void newsmoke(struct state*,struct base*,float,float);
struct smoke *deletesmoke(struct state*,struct smoke*,struct smoke*);
void newcloud(struct state*);
struct cloud *deletecloud(struct state*,struct cloud*,struct cloud*);
void newexplosion(struct state*,float,float,float);
struct explosion *deleteexplosion(struct state*,struct explosion*,struct explosion*);
void newmessage(struct state*,char*);
struct message *deletemessage(struct state*,struct message*,struct message*);

