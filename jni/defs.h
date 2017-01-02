#include "glesutil.h"

#define distance(x1,x2,y1,y2) (sqrtf(powf((x2)-(x1),2)+powf((y2)-(y1),2)))
#define xcorrect(x) ((x)-(state->player.base.x+(PLAYER_WIDTH/2.0f)))
#define ycorrect(y) ((y)-(state->player.base.y+(PLAYER_HEIGHT/2.0f)))

// gameplay
#define TID_BACKGROUND 0
#define TID_PLAYER 1
#define TID_CLOUD 2
#define TID_BULLET 3
#define TID_SMOKE 2
#define TID_ENEMY 1
#define TID_MISSILE 4
#define TID_FLASH 5

// ui
#define TID_JOYBASE 0
#define TID_JOYTOP 1
#define TID_JOYFIRE 2

// sounds
#define SID_BACKGROUND 0

#define JOYBASE_SIZE 2.0f
#define JOYTOP_SIZE 1.0f
#define JOYTOP_DIST 1.2f
#define JOYFIRE_SIZE 1.5f
struct base{
	float x,y,w,h,rot;
	float count,frame;
};

#define PLAYER_WIDTH 0.658f
#define PLAYER_HEIGHT 0.8f
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

#define ENEMY_WIDTH 1.0f
#define ENEMY_HEIGHT 1.0f
#define ENEMY_SPEED 0.045f
struct enemy{
	struct base base;
	struct base target;
	int timer_smoke;
	struct enemy *next;
};

#define MISSILE_WIDTH 0.4f
#define MISSILE_HEIGHT 0.09f
#define MISSILE_SPEED 0.116f
#define MISSILE_TURN_SPEED 0.04f
#define MISSILE_SMOKE 4
#define MISSILE_MAX_BOUNDARY 0.5f
#define MISSILE_MAX_INCREMENT 0.1f
struct missile{
	struct base base;
	int timer_smoke;
	int dead;
	struct missile *next;
};

#define BULLET_WIDTH 0.3f
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

struct state{
	int running;
	int fire;

	int vao,vbo,program;
	struct device device,screen;
	struct pack assets,uiassets;
	struct apack aassets;
	struct{float left,right,bottom,top;}rect;
	struct{ftfont *main;}font;
	struct{int vector,size,texcoords,rot,rgba,projection;}uniform;
	struct crosshair pointer[2];
	
	EGLSurface surface;
	EGLContext context;
	EGLDisplay display;
	struct android_app *app;
	struct jni_info jni_info;

	struct base background,joy_base,joy_top,joy_fire;
	struct player player;
	struct enemy *enemylist;
	struct missile *missilelist;
	struct bullet *bulletlist;
	struct smoke *smokelist;
	struct cloud *cloudlist;
	struct explosion *explosionlist;
};

int process(struct android_app*);
int32_t inputproc(struct android_app*,AInputEvent*);
void cmdproc(struct android_app*,int32_t);
void init_display(struct state*);
void term_display(struct state*);

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

