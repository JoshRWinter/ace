#include "glesutil.h"

// #define SHOW_FPS
#define distance(x1,x2,y1,y2) (sqrtf(powf((x2)-(x1),2)+powf((y2)-(y1),2)))
#define xcorrect(a) ((a)-(state->player.base.x+(PLAYER_WIDTH/2.0f)))
#define ycorrect(b) ((b)-(state->player.base.y+(PLAYER_HEIGHT/2.0f)))
#define onscreen(target) (target->base.x+target->base.w>PLAYER_LEFT_BOUNDARY&&target->base.x<PLAYER_RIGHT_BOUNDARY&&target->base.y+target->base.h>PLAYER_TOP_BOUNDARY&&target->base.y<PLAYER_BOTTOM_BOUNDARY)
#define DATAPATH "/data/data/joshwinter.ace/files"

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
#define TID_HEALTHINDICATOR 8
#define TID_HEALTH 9
#define TID_GROUP 10
#define TID_BOMB 11

// ui
#define TID_JOYBASE 0
#define TID_JOYTOP 1
#define TID_JOYFIRE 2
#define TID_TITLE 3
#define TID_BUTTON 4
#define TID_BLOB 5
#define TID_JOYBOMB 6
#define TID_AWDDFC 7
#define TID_LARGECLOUD 8
#define TID_AWDAM 9
#define TID_AWDDFCSMALL 10
#define TID_AWDAMSMALL 11
#define TID_FORMATION 12

// sounds
#define SID_SILENCE 0
#define SID_THEME 1
#define SID_DISTEXPLOSION 2
#define SID_EXPLOSION 3
#define SID_FIRE 4
#define SID_HIT 5
#define SID_SMALLEXPLOSION 6
#define SID_MESSAGE 7
#define SID_HUGEEXPLOSION 8
#define SID_DROP 9
#define SID_ENGINE 10
#define SID_WOOSH 11

// point values
#define POINTS_MISSILES_COLLIDE 10
#define POINTS_MISSILE_SHOT_DOWN 12
#define POINTS_MISSILE_EVADED 15
#define POINTS_ENEMY_SHOT_DOWN 30
#define POINTS_GROUP_DESTROYED 150

#define AWARD_DFC 1
#define AWARD_AM 2

#define GAMEOVER_DELAY 1
#define DEATH_RATTLE 300
#define HIT_RATTLE 30
#define CARRIER_RATTLE 700
#define HIGHSCORE_COUNT 5
#define HIGHSCORE_HIGHLIGHT 0.3f,0.9f,0.1f,1.0f

#define DFC_WIDTH 1.86667f
#define DFC_HEIGHT 3.5f
#define DFCSMALL_WIDTH 1.0667f
#define DFCSMALL_HEIGHT 2.0f
#define AM_WIDTH 1.8f
#define AM_HEIGHT 3.4f
#define AMSMALL_WIDTH 1.058333f
#define AMSMALL_HEIGHT 2.0f
#define JOYBASE_SIZE 2.0f
#define JOYTOP_SIZE 1.0f
#define JOYTOP_DIST 1.2f
#define JOYFIRE_SIZE 1.5f
#define JOYBOMB_SIZE 0.85f
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
#define PLAYER_WIDTH 0.6f
#define PLAYER_HEIGHT 0.7583333f
#define PLAYER_SPEED 0.1f
#define PLAYER_RELOAD 18
#define PLAYER_TURN_SPEED 0.06f
#define PLAYER_SMOKE 5
#define PLAYER_TOLERANCE 0.2f // collision tolerance
struct player{
	struct base base;
	struct audioplayer *engine;
	float targetrot;
	float xv,yv;
	float timer_bomb;
	float timer_frame;
	int reload;
	int health;
	int bombs;
	int dead;
	int victories; // enemy combatants shot down
	float timer_smoke;
};

#define ENEMY_WIDTH 0.633f
#define ENEMY_HEIGHT 0.7f
#define ENEMY_SPEED (PLAYER_SPEED-0.003f)
#define ENEMY_MODE_NONE 0
#define ENEMY_MODE_MISSILES 1
#define ENEMY_MODE_DOGFIGHT 2
#define ENEMY_CONE 0.75f
#define ENEMY_FIRE_DIST 7.0f
#define ENEMY_SPIN_TIMER 110.0f
struct enemy{
	struct base base;
	struct base target;
	float timer_smoke;
	float timer_reload;
	float timer_frame;
	float xv,yv;
	int dead;
	float dying; // timer
	int health;
	struct enemy *next;
};

#define GROUP_WIDTH 2.2f
#define GROUP_HEIGHT 1.78333f
#define GROUP_DEPTH 1.75f
struct group{
	struct base base;
	int health;
	int dead;
	float timer_explosions;
	struct group *next;
};

#define BOMB_COUNT 21
#define BOMB_DRAG 0.99f
#define BOMB_ALTITUDE 72
#define BOMB_WIDTH 0.55f
#define BOMB_HEIGHT 0.175f
#define BOMB_RECHARGE 420.0f // bomb it
struct bomb{
	struct base base;
	float xv,yv;
	int altitude;
	float depth;
	struct bomb *next;
};

#define MISSILE_WIDTH 0.25f
#define MISSILE_HEIGHT 0.0833f
#define INDICATOR_WIDTH 0.7f
#define INDICATOR_HEIGHT 0.55f
#define MISSILE_SPEED 0.116f
#define MISSILE_TURN_SPEED 0.04f
#define MISSILE_SMOKE 3
#define MISSILE_TTL 1000
#define MISSILE_READY 50
struct missile{
	struct base base;
	float xv,yv;
	float timer_smoke;
	int dead;
	float ttl;
	float sway;
	struct missile *next;
};

#define BULLET_WIDTH 0.3f
#define BULLET_DMG 16,26
#define BULLET_ENEMY_DMG 10,15
#define BULLET_HEIGHT 0.05f
#define BULLET_SPEED 0.4f
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
	float gray;
	float alpha;
	float xv,yv;
	struct smoke *next;
};

#define HEALTH_SIZE 0.5f
struct health{
	struct base base;
	float xv,yv;
	struct health *next;
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
	float timer_delay;
};
struct explosion{
	struct flash flash[EXPLOSION_FLASH_COUNT];
	struct flash cloud; // background cloud
	int sealevel;
	struct explosion *next;
};

#define CLOUD_SIZE 2.25f
#define CLOUD_RMDIST 14.0f
struct cloud{
	struct base base;
	float alpha;
	float parallax;
	struct cloud *next;
};

#define LARGECLOUD_SIZE 4.25f
struct largecloud{
	struct base base;
	float yoffset;
	float xv,yv;
	float alpha;
	int top;
	struct largecloud *next;
};

#define FORMATION_WIDTH 1.0f
#define FORMATION_HEIGHT 2.85f
struct formation{
	struct base base;
	float yoffset;
	struct formation *next;
};

#define MESSAGE_TTL 190
#define MESSAGE_MAX 50
#define MESSAGE_PINCH 10
#define MESSAGE_Y 3.0f
struct message{
	int ttl;
	char text[MESSAGE_MAX+1];
	struct message *next;
};

struct stat{
	int dfc; // distinguished flying cross
	int am; // air medals
};

struct state{
	int running,showmenu,back,highscore[HIGHSCORE_COUNT];
	int vibrate,sounds,music; // global settings
	int fire,bomb,gameoverdelay;
	float points,gamespeed;
	float radarblink;
	struct stat stat;

	unsigned vao,vbo,program;
	struct device device,screen;
	struct pack assets,uiassets;
	struct apack aassets;
	slesenv *soundengine;
	struct{float left,right,bottom,top;}rect;
	struct{ftfont *main,*main_gothic,*button,*header;}font;
	struct{int vector,size,texcoords,rot,rgba,projection;}uniform;
	struct crosshair pointer[2];
	
	EGLSurface surface;
	EGLContext context;
	EGLDisplay display;
	struct android_app *app;
	struct jni_info jni_info;

	struct base background,joy_base,joy_top,joy_fire,joy_bomb;
	struct player player;
	struct enemy *enemylist,*focused_enemy;
	struct group *grouplist;
	struct bomb *bomblist;
	struct missile *missilelist;
	struct bullet *bulletlist;
	struct smoke *smokelist;
	struct health *healthlist;
	struct cloud *cloudlist;
	struct largecloud *largecloudlist;
	struct formation *formationlist;
	struct explosion *explosionlist;
	struct message *messagelist;
};

int process(struct android_app*);
int32_t inputproc(struct android_app*,AInputEvent*);
void cmdproc(struct android_app*,int32_t);
void init_display(struct state*);
void term_display(struct state*);
void load_settings(struct state*);
void save_settings(struct state*);
void load_stats(struct state*);
void save_stats(struct state*);
void selection(int*);
int get_pilot_skill(struct state*);
char *get_pilot_class(struct state*);

int button_process(struct crosshair*,struct button*);
void button_draw(struct state*,struct button*);

int menu_main(struct state*);
int menu_conf(struct state*);
int menu_pause(struct state*);
int menu_end(struct state*);
int menu_message(struct state*,const char*,const char*);
int menu_transition(struct state*);
int menu_award(struct state*,int);

int core(struct state *state);
void render(struct state *state);
void init(struct state*);
void reset(struct state*);

int collide(struct base*,struct base*,float);
void draw(struct state*,struct base*);
void uidraw(struct state*,struct base*);

void newenemy(struct state*);
struct enemy *deleteenemy(struct state*,struct enemy*,struct enemy*);
void newgroup(struct state*);
struct group *deletegroup(struct state*,struct group*,struct group*);
void newbomb(struct state*);
struct bomb *deletebomb(struct state*,struct bomb*,struct bomb*);
void newmissile(struct state*,struct enemy*);
struct missile *deletemissile(struct state*,struct missile*,struct missile*);
void newbullet(struct state*,struct base*);
struct bullet *deletebullet(struct state*,struct bullet*,struct bullet*);
void newsmoke(struct state*,struct base*,float,float,float);
struct smoke *deletesmoke(struct state*,struct smoke*,struct smoke*);
void newhealth(struct state*,struct enemy*);
struct health *deletehealth(struct state*,struct health*,struct health*);
void newcloud(struct state*);
struct cloud *deletecloud(struct state*,struct cloud*,struct cloud*);
void newlargecloud(struct state*,int);
struct largecloud *deletelargecloud(struct state*,struct largecloud*,struct largecloud*);
void newformation(struct state*);
struct formation *deleteformation(struct state*,struct formation*,struct formation*);
void newexplosion(struct state*,float,float,float,int);
struct explosion *deleteexplosion(struct state*,struct explosion*,struct explosion*);
void newmessage(struct state*,char*);
struct message *deletemessage(struct state*,struct message*,struct message*);

